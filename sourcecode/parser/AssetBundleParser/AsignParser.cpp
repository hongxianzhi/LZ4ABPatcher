#include "EntryItem.h"
#include "BundleFile.h"
#include "AsignParser.h"

BundleFileParserForAsign::~BundleFileParserForAsign()
{

}

bool BundleFileParserForAsign::AsignBundle(const char* path)
{
	if (path == NULL || *path == 0)
	{
		return false;
	}

	FILE* file = fopen(path, "rb+");
	if (file == NULL)
	{
		return false;
	}

	BundleFile* bundle = new BundleFile(EndianBinaryReader::Create(file));
	BundleFileParserForAsign* parser = new BundleFileParserForAsign();
	bool success = bundle->Parse(parser);
	if (parser->m_bundleSize > 0)
	{
		if (success == false)
		{
			MD5Init(&parser->m_md5ctx);
			fseek(file, 0, SEEK_SET);
			unsigned char chunk[64];
			long long leftbytes = parser->m_bundleSize;
			int readed = leftbytes > 64 ? 64 : leftbytes;
			readed = fread(chunk, 1, readed, file);
			while (readed > 0)
			{
				MD5Update(&parser->m_md5ctx, chunk, 64);
				leftbytes -= readed;
				if (leftbytes <= 0)
				{
					break;
				}
				readed = leftbytes > 64 ? 64 : leftbytes;
				readed = fread(chunk, 1, readed, file);
			}
			MD5Final(&parser->m_md5ctx, parser->m_digest);
		}

		success = true;
		fseek(file, parser->m_bundleSize, SEEK_SET);
		fwrite(parser->m_digest, 1, 16, file);
		fflush(file);
	}

	fclose(file);
	SAFE_DELETE(bundle);
	SAFE_DELETE(parser);
	return success;
}

unsigned char* BundleFileParserForAsign::Retdigest()
{
	return m_digest;
}

void BundleFileParserForAsign::EndParseBundleTable(BundleFile* bundle, EndianBinaryReader* reader, int length)
{
	MD5Init(&m_md5ctx);
	m_bundleSize = bundle->getBundleSize();
}

void BundleFileParserForAsign::ParseEntryBase(EntryItem* item, int index, int position, int length)
{
	const void* pointer = item->GetReader()->RetPointer(position);
	assert(pointer != NULL);
	MD5Update(&m_md5ctx, (unsigned char*)pointer, length);
}

void BundleFileParserForAsign::ParseEntryUnkonw(EntryItem* item, int index, int count, long long pathID)
{
	MD5Update(&m_md5ctx, (unsigned char*)&count, sizeof(count));
	MD5Update(&m_md5ctx, (unsigned char*)&pathID, sizeof(pathID));
}

void BundleFileParserForAsign::ParseEntryShareBlock(EntryItem* item, int index, int position, int datalen)
{
	const void* pointer = item->GetReader()->RetPointer(position);
	assert(pointer != NULL);
	MD5Update(&m_md5ctx, (unsigned char*)pointer, datalen);
}

bool BundleFileParserForAsign::BeginParseEntryPreloadData(EntryItem* item, int count)
{
	return true;
}

void BundleFileParserForAsign::ParseEntryPreloadData(EntryItem* item, int index, int position, int length)
{
	const void* pointer = item->GetReader()->RetPointer(position);
	assert(pointer != NULL);
	MD5Update(&m_md5ctx, (unsigned char*)pointer, length);
}

void BundleFileParserForAsign::EndParseBundleInfo(BundleFile*)
{
	MD5Final(&m_md5ctx, m_digest);
}

bool assetbundle_sign(const char* path)
{
	return BundleFileParserForAsign::AsignBundle(path);
}