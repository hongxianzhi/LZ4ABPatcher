#ifndef _COMPARE_PARSER_H_
#define _COMPARE_PARSER_H_

#include "uthash.h"
#include "AsignParser.h"

#define PATCH_SIGN "UnityFS_Patch"

extern "C"
{
	struct AssetBundleDiff_CTX
	{
		const char* path;
		int diff_count_pos;
		int diff_count;
		int bytes_writed;
		void* writer;
		FILE* file;
	};
	AssetBundleDiff_CTX* assetbundle_diff_init(const char* patch);
	void assetbundle_diff_update(AssetBundleDiff_CTX*, const char* path_from, const char* path_to, const char* bundlename);
	int assetbundle_diff_final(AssetBundleDiff_CTX* ctx);
}

class BundleFileParserForCompress : public BundleFileParserForAsign
{
public:
	struct PreloadMD5Code
	{
		int index;
		char md5code[16];
		UT_hash_handle hh;
	};

	class EntryData
	{
	public:
		std::string name;
		EntryItemVoidData* entryHeader;
		std::vector<long long > pathids;
		std::vector<PreloadMD5Code*> preloadhashs;
		std::vector<EntryItemVoidData*> preloads;

		virtual ~EntryData();
	};

	static void UpdatePatch(AssetBundleDiff_CTX* ctx, const char* path_from, const char* path_to, const char* bundlename);

	virtual void EndParseBundleTable(BundleFile* bundle, EndianBinaryReader* reader, int length);

	virtual void BeginParserEntrys(int count);
	virtual void EnterEntryItem(int index, EntryItem* item);

	virtual void BeginParseEntryPreloadInfo(EntryItem* item, int count);
	virtual void ParseEntryPreloadInfo(EntryItem* item, int index, long long pathID, int position, int size, int addtionlen);

	virtual void EndParseEntryTable(EntryItem* item, int position, int datalen);

	virtual bool BeginParseEntryPreloadData(EntryItem* item, int count);
	virtual void ParseEntryPreloadData(EntryItem* item, int index, int position, int length);

	static bool SaveToFile(EntryData* old_entry, EntryData* new_entry, EndianBinaryWriter* writer);
	static bool SaveToFile(BundleFileParserForCompress* old_bundle, BundleFileParserForCompress* new_bundle, EndianBinaryWriter* writer, const char* bundlename);

	BundleFileParserForCompress();
	virtual ~BundleFileParserForCompress();

protected:
	static void BuildPatchForAddFile(FILE* file_to, EndianBinaryWriter* writer, const char* bundlename);
	static void BuildPatchForDeleteFile(EndianBinaryWriter* writer, const char* bundlename);
	static int ComparessAndBuildPatch(FILE* file_from, FILE* file_to, EndianBinaryWriter* writer, const char* bundlename);

protected:
	static bool m_error;
	EntryData * m_current;
	int m_tableOffset;
	int m_tableUnCompressed;
	EntryItemVoidData* m_bundleHeader;
	std::vector<EntryData*> m_EntryDatas;
};

#endif