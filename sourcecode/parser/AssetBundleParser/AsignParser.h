#ifndef _ASIGN_PARSER_H_
#define _ASIGN_PARSER_H_

#include "md5.h"
#include "Parser.h"

class BundleFileParserForAsign : public BundleFileParser
{
public:
	virtual ~BundleFileParserForAsign();
	static bool AsignBundle(const char* path);
	unsigned char* Retdigest();
	virtual void EndParseBundleTable(BundleFile* bundle, EndianBinaryReader* reader, int length);
	virtual void ParseEntryBase(EntryItem* item, int index, int position, int length);
	virtual void ParseEntryUnkonw(EntryItem* item, int index, int count, long long pathID);
	virtual void ParseEntryShareBlock(EntryItem* item, int index, int position, int datalen);
	virtual bool BeginParseEntryPreloadData(EntryItem* item, int count);
	virtual void ParseEntryPreloadData(EntryItem* item, int index, int position, int length);
	virtual void EndParseBundleInfo(BundleFile*);

protected:
	long long m_bundleSize;
	unsigned char m_digest[16];
	MD5_CTX m_md5ctx;
};

extern "C" bool assetbundle_sign(const char* path);

#endif