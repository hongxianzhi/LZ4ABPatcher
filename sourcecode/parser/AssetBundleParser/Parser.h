#ifndef _PARSER_H_
#define _PARSER_H_

#include <map>
#include <string>
#include <vector>
#include <assert.h>

class AssetChunk;
class EntryItem;
class BundleFile;
class EntryItemParser;
class EndianBinaryReader;
class EndianBinaryWriter;

enum DifferenceCode
{
	DIFFERENCE_KEEP = 0,
	DIFFERENCE_DELETE,
	DIFFERENCE_ADD,
	DIFFERENCE_MODIFY
};

class EntryItemVoidData
{
public:
	EntryItemVoidData();
	EntryItemVoidData(int numi, long long numl);
	EntryItemVoidData(const void* d, int dl);
	virtual ~EntryItemVoidData();

public:
	int GetInt();
	long long GetInt64();
	static bool Compare(EntryItemVoidData* from, EntryItemVoidData* to);
	static EntryItemVoidData* CreateFromPatch(EndianBinaryReader* reader);
	static bool CompareAndSaveToFile(EntryItemVoidData* from, EntryItemVoidData* to, EndianBinaryWriter* writer);

public:
	void* data;
	int datalen;
};

class BundleFileParser
{
public:
	BundleFileParser();
	virtual ~BundleFileParser();

public:
	EntryItem * GetEntry(int index);

	virtual void EndParseBundleTable(BundleFile* bundle, EndianBinaryReader* reader, int length);

	virtual void BeginParseChunksInfo(int count);
	virtual void ParseChunkInfo(int index, int uncompressedSize, int compressedSize, int flag);
	virtual void EndParseChunksInfo();

	virtual void BeginParserEntrys(int count);

	virtual EntryItem* CreateEntryItem(int index, std::string& entryname, EndianBinaryReader* assetsFileReader, long long entryinfo_offset, long long entryinfo_size, int flag);

	virtual void EnterEntryItem(int index, EntryItem* item);

	//entry依赖的脚本信息
	virtual void BeginParseEntryBase(EntryItem* item, int count);
	virtual void ParseEntryBase(EntryItem* item, int index, int position, int length);
	virtual void EndParseEntryBase(EntryItem* item);

	//entry的preload信息
	virtual void BeginParseEntryPreloadInfo(EntryItem* item, int count);
	virtual void ParseEntryPreloadInfo(EntryItem* item, int index, long long pathID, int position, int size, int addtionlen);
	virtual void EndParseEntryPreloadInfo(EntryItem* item);

	//entry中未知段
	virtual void BeginParseEntryUnkonw(EntryItem* item, int count);
	virtual void ParseEntryUnkonw(EntryItem* item, int index, int count, long long pathID);
	virtual void EndParseEntryUnkonw(EntryItem* item);

	//entry中shareblock信息
	virtual void BeginParseEntryShareBlock(EntryItem* item, int count);
	virtual void ParseEntryShareBlock(EntryItem* item, int index, int position, int datalen);
	virtual void EndParseEntryShareBlock(EntryItem* item);

	virtual void EndParseEntryTable(EntryItem* item, int position, int datalen);

	//preload数据块
	virtual bool BeginParseEntryPreloadData(EntryItem* item, int count);
	virtual void ParseEntryPreloadData(EntryItem* item, int index, int position, int length);
	virtual void EndParseEntryPreloadData(EntryItem* item);

	virtual void ExitEntryItem(int index, EntryItem* item);

	virtual void EndParserEntrys();

	virtual void EndParseBundleInfo(BundleFile*);

protected:
	void CreateEntryinfo(int count);
	bool FillEntryinfo(int index, std::string& entryname, EndianBinaryReader* assetsFileReader, int entryinfo_offset, long long entryinfo_size, int flag);

private:
	friend class BundleFile;
	EntryItem * m_current;
	std::vector<EntryItem*> m_entrys;
};

extern "C"
{
	typedef void (*MESSAGEHANDLE)(const char* message);
	void set_messagehandler(MESSAGEHANDLE handler);
	void push_message(const char* format, ...);
}

#endif