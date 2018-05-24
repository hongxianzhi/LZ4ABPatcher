#include <map>
#include <list>
#include <vector>
#include <stdio.h>
#include <string>

class EntryItem;
class AssetChunk;
class BundleFileParser;
class EndianBinaryReader;

class BundleFile
{
public:
	BundleFile(EndianBinaryReader* reader);
	~BundleFile();

public:
	bool Parse(BundleFileParser* parser);
	static void PartitionChunks(std::vector<AssetChunk*>& src, std::vector<AssetChunk*>& dst, int offset, int length, int& bytes_trimed, int& bytes_compress, int& bytes_uncompress);

public:
	std::string getSignature() { return m_signature; }
	int getFormat() { return m_format; }
	std::string getVersionPlayer() { return m_versionPlayer; }
	std::string getVersionEngine() { return m_versionEngine; }
	int getBundleSizePosition() { return m_bundleSizePos; }
	long long getBundleSize() { return m_bundleSize; }
	int getCompressedSize() { return m_tableCompressedSize; }
	int getUnCompressedSize() { return m_tableUncompressedSize; }
	int getFlag() { return m_flag; }
	int getBlockStartPosition() { return m_blockstartpos; }
	int getBlockCount() { return m_blockcount; }
	EndianBinaryReader* GetHeaderReader() { return m_headerReader; };
	EndianBinaryReader* GetBlockReader() { return m_blockReader; };
	EndianBinaryReader* GetChunkReader() { return m_chunkReader; };

protected:
	bool ReadFormat6(BundleFileParser* parser, EndianBinaryReader* reader);

protected:
	std::string m_signature;
	int m_format;
	std::string m_versionPlayer;
	std::string m_versionEngine;
	int m_bundleSizePos;
	long long m_bundleSize;
	int m_headerlen;
	int m_tablelen;
	int m_tableCompressedSize;
	int m_tableUncompressedSize;
	int m_flag;
	int m_blockstartpos;
	int m_blockcount;

	EndianBinaryReader* m_blockReader;
	EndianBinaryReader* m_headerReader;
	EndianBinaryReader* m_chunkReader;
	std::vector<AssetChunk*> m_blocklist;
	std::vector<EntryItem*> m_asset_packge;
};

extern "C"
{
	long long assetbundle_size(const char* path);
	bool assetbundle_check(const char* path);
}