#ifndef _ENDIAN_BINARY_READER_H_
#define _ENDIAN_BINARY_READER_H_

#include <list>
#include <vector>
#include <string>
#include <stdio.h>
#include <malloc.h>

class AssetChunk;

#define CHUNK_LENGTH (128 * 1024)
#define SAFE_FREE(POINTER) if(POINTER != NULL){ free(POINTER); POINTER = NULL;}
#define SAFE_DELETE(POINTER) if(POINTER != NULL){ delete(POINTER); POINTER = NULL;}

enum EndianType
{
	BigEndian,
	LittleEndian
};

class EndianBinaryReader
{
public:
	static EndianBinaryReader* Create(FILE* file);
	static EndianBinaryReader* Create(const void* buf, int buf_len);
	static EndianBinaryReader* Create(const std::vector<AssetChunk*>& chunks, int discard, int chunk_length);

	static bool Reverse(void* var, int length);
	virtual bool ReadBoolean();
	virtual unsigned char ReadByte();
	virtual void AlignStream(int alignment);
	virtual short ReadInt16();
	virtual int ReadInt32();
	virtual unsigned int ReadUInt32();
	virtual long long ReadInt64();
	virtual char* ReadStringToNull() = 0;
	virtual void ReadStringToNull(char* buf, int length) = 0;
	virtual void* ReadBytes(int length) = 0;
	virtual int ReadBytes(void* buf, int length) = 0;
	virtual int GetLength() = 0;
	virtual int GetPosition() = 0;
	virtual void SetPosition(int position) = 0;
	virtual EndianBinaryReader* Clone() = 0;
	virtual void Trim();
	virtual void LockTrim(bool lock);
	virtual const void* RetPointer(int pos);
	virtual int Prepare(int length);

public:
	EndianType GetEndianType();
	void SetEndianType(EndianType type);
	EndianBinaryReader();
	virtual ~EndianBinaryReader();
protected:
	EndianType m_endian;
};

class EndianBinaryReaderForFile : public EndianBinaryReader
{
public:
	virtual char* ReadStringToNull();
	virtual void ReadStringToNull(char* buf, int length);
	virtual void* ReadBytes(int length);
	virtual int ReadBytes(void* buf, int length);
	virtual int GetLength();
	virtual int GetPosition();
	virtual void SetPosition(int position);
	virtual EndianBinaryReader* Clone();
protected:
	friend class EndianBinaryReader;
	EndianBinaryReaderForFile(FILE* file);
	virtual ~EndianBinaryReaderForFile();
protected:
	FILE* m_file;
	int m_file_length;
};

class EndianBinaryReaderForMemory : public EndianBinaryReader
{
public:
	virtual const void* RetPointer(int offset);
	virtual char* ReadStringToNull();
	virtual void ReadStringToNull(char* buf, int length);
	virtual void* ReadBytes(int length);
	virtual int ReadBytes(void* buf, int length);
	virtual int GetLength();
	virtual int GetPosition();
	virtual void SetPosition(int position);
	virtual EndianBinaryReader* Clone();

protected:
	friend class EndianBinaryReader;
	EndianBinaryReaderForMemory(const char* buf, int buf_len);
	virtual ~EndianBinaryReaderForMemory();
protected:
	int m_position;
	const char* m_buf;
	int m_buf_len;
};

class EndianBinaryReaderForChunk : public EndianBinaryReader
{
public:
	virtual const void* RetPointer(int offset);
	virtual char* ReadStringToNull();
	virtual void ReadStringToNull(char* buf, int length);
	virtual void* ReadBytes(int length);
	virtual int ReadBytes(void* buf, int length);
	virtual int GetLength();
	virtual int GetPosition();
	virtual void SetPosition(int position);
	virtual EndianBinaryReader* Clone();
	virtual void Trim();
	virtual void LockTrim(bool lock);
	virtual int Prepare(int length);

protected:
	friend class EndianBinaryReader;
	int PositionToChunkIndex(int position);
	int PositionToBufferOffset(int position);
	void Ensure(int index, int length);
	EndianBinaryReaderForChunk(EndianBinaryReaderForChunk* reader);
	EndianBinaryReaderForChunk(std::vector<AssetChunk*> chunks, int discard, int chunk_length);
	virtual ~EndianBinaryReaderForChunk();
protected:
	int m_trim_lock;

	//[chunk数据
	struct AssetChunkData
	{
		AssetChunk* chunk;
		int virtualpos;
		int memorypos;
	};
	int m_bytes_discard;						//第一个块之前的冗余内存，读取时需要排除掉；
	int m_chunks_bytes_length;					//所有chunk解压后大小总和；
	std::vector<AssetChunkData*> m_chunks;		//原始数据块和在缓存里对应的数据
	//]chunk数据

	//[内存池
	/* 0:读取原始数据，初始大小CHUNK_LENGTH
	// 1:存储解压后的数据，大小根据实际情况会随时扩充
	*/
	int m_position;
	int m_buffer_used;							//已经使用的缓存大小;
	int m_chunk_readed_start;					//已经读取的chunk起始块;
	int m_chunk_readed_count;					//已经读取的chunk块数量;
	void* m_buffers[2];
	int m_buffer_length[2];
	//]内存池
};

#endif // !_ENDIAN_BINARY_READER_H_