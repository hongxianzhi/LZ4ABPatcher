#include "ComparePaser.h"
#include "EndianBinaryWriter.h"

extern "C"
{
	typedef void(*PROGRESSHANDLE)(const char* message, int index, int size);
	void assetbundle_merge(const char* patch, const char* searchpath, const char* newpath, PROGRESSHANDLE handler);
}

struct EntryItemPatchInfo
{
	int position;
	int length;
};

struct PreloadPatchItem : EntryItemPatchInfo
{
	int index_src;
	int index_dst;
	bool fromsrc;
};

struct PreloadHeaderInfo : EntryItemPatchInfo
{
	long long pathid;
};

struct EntryPatch
{
	std::string m_name;
	EntryItemPatchInfo* m_entrytables;
	unsigned char* m_md5codes;
	std::map<int, PreloadPatchItem*> m_preloads;
};

class BundlePatchFile
{
public:
	BundlePatchFile(EndianBinaryReader* reader);
	virtual ~BundlePatchFile();

public:
	EntryPatch* GetEntryPatch(int index);
	int GetPatchType() { return m_patchtype; }
	int GetDataStartPosition() { return m_datastart; }
	const char* GetBundleName() { return m_bundlename.c_str(); }
	bool GetEntryItemPatchInfo(EndianBinaryReader* reader, int& offset, int& length);
	bool LoadEntryPatch(EndianBinaryReader* reader, EntryPatch* patch);
	int ReadBytes(void* buf, int buflen, int position);

protected:
	friend class BundleFileParserForEntry;
	friend class ParserForWriteBundleHeader;
	int m_patchtype;
	int m_datastart;
	std::string m_bundlename;
	unsigned char m_digestold[16];
	unsigned char m_digestnew[16];
	EndianBinaryReader * m_reader;
	int m_tableOffset;
	int m_tableUnCompressed;
	EntryItemPatchInfo* bundleHeaders;
	std::vector<EntryPatch*> m_entrys;
};

/*
1.比较老bundle和patch里保存的md5值
2.解析老bundle里的preload信息并保存到EntryItemParserForSrcPreloads中
*/
class ParserForWriteBundleHeader : public BundleFileParser
{
public:
	ParserForWriteBundleHeader(BundlePatchFile* patcher, EndianBinaryWriter* writer);
	virtual ~ParserForWriteBundleHeader();

public:
	static void MergeFormPatch(const char* patch, const char* searchpath, const char* newpath, PROGRESSHANDLE handler);
	int ReadEntryData(const std::string& entryName, int index, void*& bytes);
	virtual void BeginParseEntryPreloadInfo(EntryItem* item, int count);
	virtual void ParseEntryPreloadInfo(EntryItem* item, int index, long long pathID, int position, int size, int addtionlen);
	virtual bool BeginParseEntryPreloadData(EntryItem* item, int count);

	virtual void EndParseBundleInfo(BundleFile* bundle);

protected:
	EndianBinaryReader * m_srcreader;
	std::vector<EntryItemPatchInfo*>* m_currentEntryData;
	std::map<std::string, std::vector<EntryItemPatchInfo*>*> m_entrys;
	EndianBinaryWriter* m_writer;
	BundlePatchFile* m_patcher;
};

/*
读取patch里记录的新bundle头文件信息，还原出preload结构，结合老bundle信息，patch信息，重新生成bundle文件。
*/
class BundleFileParserForEntry : public BundleFileParser
{
public:
	struct PreloadItemData
	{
		int position;
		int index;
	};
	BundleFileParserForEntry(ParserForWriteBundleHeader* oldbundleparser, BundlePatchFile* patcher, EndianBinaryWriter* writer);
	virtual ~BundleFileParserForEntry();

	bool success() { return m_error == false; }

	virtual void BeginParseChunksInfo(int count);
	virtual void BeginParseEntryPreloadInfo(EntryItem* item, int count);
	virtual void ParseEntryPreloadInfo(EntryItem* item, int index, long long pathID, int position, int size, int addtionlen);
	static bool SortEntryItem(PreloadItemData* v1, PreloadItemData* v2);
	virtual void EndParseEntryPreloadInfo(EntryItem* item);
	virtual EntryItem* CreateEntryItem(int index, std::string& entryname, EndianBinaryReader* assetsFileReader, long long entryinfo_offset, long long entryinfo_size, int flag);
	virtual void EndParseBundleInfo(BundleFile* bundle);

protected:
	int ChunkLeft() { return CHUNK_LENGTH - m_buffer_used; };
	void Ensure(int index, int length);
	void PushEmptyBytes(int length);
	void PushBytes(const char* bytes, int length);
	void WriteTempFile();

protected:
	struct ChunkItemData
	{
		int uncompressed;
		int compressed;
	};
	int m_entryOffset;
	int m_bytespushed;

	void* m_lastentryinfo;
	FILE* m_tempfile;
	char m_tempfilename[512];
	EndianBinaryWriter* m_writer;
	BundlePatchFile* m_patcher;
	EntryPatch* m_currentPatch;
	std::vector<ChunkItemData*> m_chunks;
	std::vector<PreloadItemData*> m_preloads;
	ParserForWriteBundleHeader* m_bundleParserOld;

	bool m_error;
	int m_buffer_used;						//已经使用的缓存大小;
	void* m_buffers[2];
	int m_buffer_length[2];
};