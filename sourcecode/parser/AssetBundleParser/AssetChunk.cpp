#include "lz4.h"
#include "lz4hc.h"
#include <assert.h>
#include "AssetChunk.h"
#include "EndianBinaryReader.h"

AssetChunk::AssetChunk(EndianBinaryReader* reader, int compressedSize, int uncompressedSize, short flag)
{
	m_flag = flag;
	m_reader = reader;
	m_compressedDataBeginPos = 0;
	m_compressedSize = compressedSize;
	m_uncompressedSize = uncompressedSize;
	if (m_reader != NULL)
	{
		m_compressedDataBeginPos = m_reader->GetPosition();
	}
}

AssetChunk::~AssetChunk()
{

}

int AssetChunk::GetBlockStartPos()
{
	return m_compressedDataBeginPos;
}

int AssetChunk::GetCompressedSize()
{
	return m_compressedSize;
}

int AssetChunk::GetUnCompressedSize()
{
	return m_uncompressedSize;
}

void AssetChunk::SetBlockPosition(int position)
{
	m_compressedDataBeginPos = position;
}

int AssetChunk::GetUnCompressData(void* buf_compressed, int buf_compressed_len, void* buf_uncompressed, int buf_uncompressed_len)
{
	int uncompressed_len = 0;
	if (m_reader != NULL && buf_compressed != NULL && buf_compressed_len > 0 && buf_uncompressed != NULL && buf_uncompressed_len > 0)
	{
		int position = m_reader->GetPosition();
		buf_compressed_len = buf_compressed_len <= m_compressedSize ? buf_compressed_len : m_compressedSize;
		m_reader->SetPosition(m_compressedDataBeginPos);
		buf_compressed_len = m_reader->ReadBytes(buf_compressed, buf_compressed_len);
		if (buf_compressed_len > 0)
		{
			switch (m_flag & 0x3F)
			{
			case 1://LZMA
				uncompressed_len = 0;
				break;
			case 2://LZ4
			case 3://LZ4HC
			{
				uncompressed_len = LZ4_decompress_safe((const char*)buf_compressed, (char*)buf_uncompressed, buf_compressed_len, buf_uncompressed_len);
			}
			break;

			default://None
			{
				memcpy(buf_uncompressed, buf_compressed, buf_compressed_len);
				uncompressed_len = buf_compressed_len;
			}
			break;
			}
		}
	}
	return uncompressed_len;
}