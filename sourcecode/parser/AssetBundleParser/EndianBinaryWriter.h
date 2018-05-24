#ifndef _ENDIAN_BINARY_WRITER_H_
#define _ENDIAN_BINARY_WRITER_H_

#include "EndianBinaryReader.h"

class AssetChunk;

class EndianBinaryWriter
{
public:
	static EndianBinaryWriter* Create(FILE* file);
	static EndianBinaryWriter* Create(const char* path, const char* mode);

	static bool Reverse(void* var, int length);
	virtual int WriteBoolean(bool var);
	virtual int WriteByte(unsigned char var);
	virtual int AlignStream(int alignment);
	virtual int WriteInt16(short var);
	virtual int WriteInt32(int var);
	virtual int WriteUInt32(unsigned int var);
	virtual int WriteInt64(long long var);
	virtual int WriteStringToNull(const char* var);
	virtual int WriteBytes(const void* var, int length);
	virtual int WriteBytes(int position, const void* var, int length);
	virtual int GetLength();
	virtual int GetPosition();
	virtual void SetPosition(int position);
	virtual void Flush();
	virtual void Close();

public:
	EndianType GetEndianType();
	void SetEndianType(EndianType type);
	virtual ~EndianBinaryWriter();
protected:
	int WriteWithEndian(void* ptr, int length);
	EndianBinaryWriter(FILE* file);
protected:
	FILE * m_file;
	int m_length;
	EndianType m_endian;
};

#endif // !_ENDIAN_BINARY_WRITER_H_