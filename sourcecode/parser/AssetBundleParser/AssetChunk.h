class EndianBinaryReader;

class AssetChunk
{
public:
	AssetChunk(EndianBinaryReader* reader, int compressedSize, int uncompressedSize, short flag);
	~AssetChunk();

public:
	int GetBlockStartPos();
	int GetCompressedSize();
	int GetUnCompressedSize();
	void SetBlockPosition(int position);
	int GetUnCompressData(void* buf_compressed, int buf_compressed_len, void* buf_uncompressed, int buf_uncompressed_len);

protected:
	short m_flag;
	int m_uncompressedSize;
	int m_compressedSize;
	int m_compressedDataBeginPos;
	EndianBinaryReader * m_reader;
};