#include <assert.h>
#include "Parser.h"
#include "EntryItem.h"
#include "AssetChunk.h"
#include "EndianBinaryReader.h"

struct PreloadHeader
{
	int pos;
	long long pathID;
	int offset;
	int size;
};

EntryItem::EntryItem(EndianBinaryReader* assetsFileReader, std::string name, long long entryinfo_offset, long long entryinfo_size, int flag)
{
	m_name = name;
	m_flag = flag;
	m_entryinfo_offset = entryinfo_offset;
	assetsFileReader->SetEndianType(BigEndian);
	assetsFileReader->SetPosition(m_entryinfo_offset);
	m_assetsFileReader = assetsFileReader;
}

EntryItem::~EntryItem()
{
	m_assetsFileReader = NULL;
}

bool EntryItem::Parse(int index, BundleFileParser* parser)
{
	assert(parser != NULL);
	int tableStartPos = m_assetsFileReader->GetPosition();
	m_assetsFileReader->LockTrim(true);

	m_tableSize = m_assetsFileReader->ReadInt32();
	m_dataEnd = m_assetsFileReader->ReadInt32();
	m_fileGen = m_assetsFileReader->ReadInt32();
	m_dataOffset = m_assetsFileReader->ReadUInt32();

	switch (m_fileGen)
	{
	case 14: //5.0.0 beta and final
	case 15: //5.0.1 - 5.4
	case 16: //??.. no sure
	case 17: //5.5.0 and up
	{
		m_unknow = m_assetsFileReader->ReadInt32();
		m_version = m_assetsFileReader->ReadStringToNull();
		m_platform = m_assetsFileReader->ReadInt32();
		m_baseDefinitions = m_assetsFileReader->ReadBoolean();
	}
	break;

	default:
		m_assetsFileReader->LockTrim(false);
		return false;
	}

	if (m_platform > 255 || m_platform < 0)
	{
		m_assetsFileReader->SetEndianType(LittleEndian);
	}

	parser->EnterEntryItem(index, this);

	//[依赖的脚本信息
	int baseCount = m_assetsFileReader->ReadInt32();
	parser->BeginParseEntryBase(this, baseCount);
	for (int i = 0; i < baseCount; i++)
	{
		m_assetsFileReader->LockTrim(true);
		int pos = m_assetsFileReader->GetPosition();
		int classID = m_assetsFileReader->ReadInt32();
		if (m_fileGen > 15)	//5.5.0 and up
		{
			m_assetsFileReader->Prepare(1);
			int type1 = m_assetsFileReader->ReadInt16();
			if (type1 >= 0)
			{
				type1 = -1 - type1;
			}
			else
			{
				type1 = classID;
			}
			if (classID == 114)
			{
				m_assetsFileReader->Prepare(0x10);
			}
			classID = type1;
		}
		else if (classID < 0)
		{
			m_assetsFileReader->Prepare(0x10);
		}
		m_assetsFileReader->Prepare(0x10);

		int position_end = m_assetsFileReader->GetPosition();
		if (m_baseDefinitions)
		{
			int varCount = m_assetsFileReader->ReadInt32();
			int stringSize = m_assetsFileReader->ReadInt32();
			int datalen = varCount * 24 + stringSize;
			m_assetsFileReader->Prepare(datalen);
			position_end += datalen;
		}

		parser->ParseEntryBase(this, i, pos, position_end - pos);
		m_assetsFileReader->LockTrim(false);
	}
	parser->EndParseEntryBase(this);
	//]依赖的脚本信息

	//[Preload描述头
	int preloadoffset = -1;
	std::vector<PreloadHeader*> preloadHeaders;
	int preloadCount = m_assetsFileReader->ReadInt32();
	parser->BeginParseEntryPreloadInfo(this, preloadCount);
	if (preloadCount > 0)
	{
		preloadHeaders.resize(preloadCount);
		for (int i = 0; i < preloadCount; i++)
		{
			if (m_fileGen >= 14)
			{
				m_assetsFileReader->AlignStream(4);
			}

			m_assetsFileReader->LockTrim(true);
			PreloadHeader* header = new PreloadHeader();
			header->pos = m_assetsFileReader->GetPosition();
			header->pathID = m_fileGen < 14 ? m_assetsFileReader->ReadInt32() : m_assetsFileReader->ReadInt64();
			int offset = m_assetsFileReader->ReadUInt32();
			header->offset = offset + m_dataOffset + m_entryinfo_offset;
			header->size = m_assetsFileReader->ReadInt32();

			int addtionlen = 0;
			if (m_fileGen > 15)
			{
				addtionlen = 4;	//preload使用的classid信息
			}
			else
			{
				addtionlen = 8;	//preload使用的classid信息
				if (m_fileGen == 15)
				{
					addtionlen += 1;
				}
			}
			m_assetsFileReader->Prepare(addtionlen);

			parser->ParseEntryPreloadInfo(this, i, header->pathID, header->offset, header->size, addtionlen);

			preloadHeaders[i] = header;
			m_assetsFileReader->LockTrim(false);
		}
	}
	parser->EndParseEntryPreloadInfo(this);
	//]Preload描述头

	//[preloaded block
	if (m_fileGen >= 14)
	{
		//this looks like a list of assets that need to be preloaded in memory before anytihng else
		int someCount = m_assetsFileReader->ReadInt32();
		parser->BeginParseEntryUnkonw(this, someCount);
		for (int i = 0; i < someCount; i++)
		{
			int num1 = m_assetsFileReader->ReadInt32();
			m_assetsFileReader->AlignStream(4);
			long long pathID = m_assetsFileReader->ReadInt64();
			parser->ParseEntryUnkonw(this, i, num1, pathID);
		}
		parser->EndParseEntryUnkonw(this);
	}
	//]preloaded block

	//[sharedFile
	int sharedFileCount = m_assetsFileReader->ReadInt32();
	parser->BeginParseEntryShareBlock(this, sharedFileCount);
	for (int i = 0; i < sharedFileCount; i++)
	{
		m_assetsFileReader->LockTrim(true);
		int pos = m_assetsFileReader->GetPosition();
		std::string name = m_assetsFileReader->ReadStringToNull();
		m_assetsFileReader->Prepare(20);
		std::string sharedFilePath = m_assetsFileReader->ReadStringToNull(); //relative path
		parser->ParseEntryShareBlock(this, i, pos, m_assetsFileReader->GetPosition() - pos);
		m_assetsFileReader->LockTrim(false);
	}
	parser->EndParseEntryShareBlock(this);
	//]sharedFile

	int tableEndPos = m_assetsFileReader->GetPosition() - m_entryinfo_offset;
	if (m_dataOffset > tableEndPos)
	{
		m_assetsFileReader->Prepare(m_dataOffset - tableEndPos);
	}
	parser->EndParseEntryTable(this, tableStartPos, m_dataOffset + m_entryinfo_offset - tableStartPos);
	m_assetsFileReader->LockTrim(false);

	//[Preload数据块
	if (parser->BeginParseEntryPreloadData(this, preloadHeaders.size()))
	{
		for (int i = 0; i < preloadHeaders.size(); i++)
		{
			m_assetsFileReader->LockTrim(true);
			PreloadHeader* header = preloadHeaders[i];
			m_assetsFileReader->SetPosition(header->offset);
			m_assetsFileReader->Prepare(header->size);
			parser->ParseEntryPreloadData(this, i, header->offset, header->size);
			m_assetsFileReader->LockTrim(false);
			SAFE_DELETE(header);
		}
	}
	parser->EndParseEntryPreloadData(this);
	//]Preload数据块
}