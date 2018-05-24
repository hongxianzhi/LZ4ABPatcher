#include <stdarg.h>
#include "Parser.h"
#include "AssetChunk.h"
#include "EntryItem.h"
#include "EndianBinaryWriter.h"

EntryItemVoidData::EntryItemVoidData(const void* d, int dl)
{
	if (d == NULL)
	{
		push_message("EntryItemVoidData:null pointer");
	}
	else if (dl <= 0)
	{
		push_message("EntryItemVoidData: error length %d", dl);
	}
	datalen = dl;
	data = malloc(datalen);
	memcpy(data, d, datalen);
}

EntryItemVoidData::EntryItemVoidData(int numi, long long numl)
{
	datalen = sizeof(int) + sizeof(long long);
	data = malloc(datalen);
	unsigned char* ptr = (unsigned char*)data;
	*(int*)ptr = numi;
	ptr += sizeof(int);
	*(long long*)ptr = numl;
}

EntryItemVoidData::EntryItemVoidData()
{
	data = NULL;
	datalen = 0;
}

EntryItemVoidData::~EntryItemVoidData()
{
	SAFE_FREE(data);
	datalen = 0;
}

int EntryItemVoidData::GetInt()
{
	assert(data != NULL && (datalen == sizeof(int) + sizeof(long long)));
	return *(int*)data;
}

long long EntryItemVoidData::GetInt64()
{
	assert(data != NULL && (datalen == sizeof(int) + sizeof(long long)));
	unsigned char* ptr = (unsigned char*)data;
	ptr += sizeof(int);
	return *(long long*)ptr;
}

bool EntryItemVoidData::Compare(EntryItemVoidData* from, EntryItemVoidData* to)
{
	return from == NULL || from->datalen != to->datalen || memcmp(from->data, to->data, to->datalen) == 0;
}

EntryItemVoidData* EntryItemVoidData::CreateFromPatch(EndianBinaryReader* reader)
{
	unsigned char type = reader->ReadByte();
	EntryItemVoidData* data = NULL;
	if (type != DIFFERENCE_KEEP)
	{
		int datalen = reader->ReadInt32();
		assert(datalen > 0);
		void* buf = reader->ReadBytes(datalen);
		data = new EntryItemVoidData(buf, datalen);
	}
	return data;
}

bool EntryItemVoidData::CompareAndSaveToFile(EntryItemVoidData* from, EntryItemVoidData* to, EndianBinaryWriter* writer)
{
	bool ret = false;
	if (writer != NULL)
	{
		unsigned char type = DIFFERENCE_KEEP;
		if (Compare(from, to))
		{
			ret = true;
			type = DIFFERENCE_MODIFY;
		}
		writer->WriteByte(type);
		if (type != DIFFERENCE_KEEP)
		{
			writer->WriteInt32(to->datalen);
			writer->WriteBytes(to->data, to->datalen);
		}
	}
	else
	{
		assert(false);
	}
	return ret;
}

BundleFileParser::BundleFileParser()
{

}

BundleFileParser::~BundleFileParser()
{
	for (int i = 0; i < m_entrys.size(); i++)
	{
		SAFE_DELETE(m_entrys[i]);
	}
	m_entrys.clear();
}

EntryItem * BundleFileParser::GetEntry(int index)
{
	return index >= 0 && index < m_entrys.size() ? m_entrys[index] : NULL;
}

void BundleFileParser::EndParseBundleTable(BundleFile* bundle, EndianBinaryReader* reader, int length)
{

}

void BundleFileParser::BeginParseChunksInfo(int count)
{

}

void BundleFileParser::ParseChunkInfo(int index, int uncompressedSize, int compressedSize, int flag)
{

}

void BundleFileParser::EndParseChunksInfo()
{

}

void BundleFileParser::BeginParserEntrys(int count)
{

}

EntryItem* BundleFileParser::CreateEntryItem(int index, std::string& entryname, EndianBinaryReader* assetsFileReader, long long entryinfo_offset, long long entryinfo_size, int flag)
{
	return new EntryItem(assetsFileReader, entryname, entryinfo_offset, entryinfo_size, flag);
}

void BundleFileParser::EnterEntryItem(int index, EntryItem* item)
{
	
}

void BundleFileParser::BeginParseEntryBase(EntryItem* item, int count)
{

}

void BundleFileParser::ParseEntryBase(EntryItem* item, int index, int position, int length)
{

}

void BundleFileParser::EndParseEntryBase(EntryItem* item)
{

}

void BundleFileParser::BeginParseEntryPreloadInfo(EntryItem* item, int count)
{

}

void BundleFileParser::ParseEntryPreloadInfo(EntryItem* item, int index, long long pathID, int position, int size, int addtionlen)
{

}

void BundleFileParser::EndParseEntryPreloadInfo(EntryItem* item)
{

}

void BundleFileParser::BeginParseEntryUnkonw(EntryItem* item, int count)
{

}

void BundleFileParser::ParseEntryUnkonw(EntryItem* item, int index, int count, long long pathID)
{

}

void BundleFileParser::EndParseEntryUnkonw(EntryItem* item)
{

}

void BundleFileParser::BeginParseEntryShareBlock(EntryItem* item, int count)
{

}

void BundleFileParser::ParseEntryShareBlock(EntryItem* item, int index, int position, int datalen)
{

}

void BundleFileParser::EndParseEntryShareBlock(EntryItem* item)
{

}

void BundleFileParser::EndParseEntryTable(EntryItem* item, int position, int datalen)
{

}

bool BundleFileParser::BeginParseEntryPreloadData(EntryItem* item, int count)
{
	return false;
}

void BundleFileParser::ParseEntryPreloadData(EntryItem* item, int index, int position, int length)
{

}

void BundleFileParser::EndParseEntryPreloadData(EntryItem* item)
{

}

void BundleFileParser::ExitEntryItem(int index, EntryItem* item)
{

}

void BundleFileParser::EndParserEntrys()
{

}

void BundleFileParser::EndParseBundleInfo(BundleFile*)
{

}


void BundleFileParser::CreateEntryinfo(int count)
{
	m_entrys.resize(count);
	BeginParserEntrys(count);
}

bool BundleFileParser::FillEntryinfo(int index, std::string& entryname, EndianBinaryReader* assetsFileReader, int entryinfo_offset, long long entryinfo_size, int flag)
{
	m_current = CreateEntryItem(index, entryname, assetsFileReader, entryinfo_offset, entryinfo_size, flag);

	m_entrys[index] = m_current;
	bool success = m_current->Parse(index, this);
	ExitEntryItem(index, m_current);
	return success;
}

char m_messagebuf[4096];
MESSAGEHANDLE m_messagehandler = NULL;
void push_message(const char* format, ...)
{
	if (format == NULL || *format == 0 || m_messagehandler == NULL)
	{
		return;
	}
	va_list args;
	va_start(args, format);
	vsprintf(m_messagebuf, format, args);
	va_end(args);
	m_messagehandler(m_messagebuf);
}

void set_messagehandler(MESSAGEHANDLE handler)
{
	m_messagehandler = handler;
}
