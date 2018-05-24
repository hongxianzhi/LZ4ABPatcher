#include <ostream>
#include "EndianBinaryWriter.h"

EndianBinaryWriter* EndianBinaryWriter::Create(const char* path, const char* mode)
{
	EndianBinaryWriter* writer = NULL;
	FILE* file = fopen(path, mode);
	if (file != NULL)
	{
		writer = new EndianBinaryWriter(file);
	}
	return writer;
}

EndianBinaryWriter* EndianBinaryWriter::Create(FILE* file)
{
	return file != NULL ? new EndianBinaryWriter(file) : NULL;
}

bool EndianBinaryWriter::Reverse(void* var, int length)
{
	return EndianBinaryReader::Reverse(var, length);
}

int EndianBinaryWriter::WriteBoolean(bool var)
{
	return WriteWithEndian(&var, sizeof(var));
}

int EndianBinaryWriter::WriteByte(unsigned char var)
{
	return WriteWithEndian(&var, sizeof(var));
}

int EndianBinaryWriter::AlignStream(int alignment)
{
	if (alignment > 0)
	{
		int pos = GetPosition();
		int mod = pos % alignment;
		if (mod != 0)
		{
			pos = pos + alignment - mod;
			while (pos > GetPosition())
			{
				WriteByte(0);
			}
		}
	}
	return GetPosition();
}

int EndianBinaryWriter::WriteInt16(short var)
{
	return WriteWithEndian(&var, sizeof(var));
}

int EndianBinaryWriter::WriteInt32(int var)
{
	return WriteWithEndian(&var, sizeof(var));
}

int EndianBinaryWriter::WriteUInt32(unsigned int var)
{
	return WriteWithEndian(&var, sizeof(var));
}

int EndianBinaryWriter::WriteInt64(long long var)
{
	return WriteWithEndian(&var, sizeof(var));
}

int EndianBinaryWriter::WriteStringToNull(const char* var)
{
	return WriteBytes(var, strlen(var) + 1);
}

int EndianBinaryWriter::WriteBytes(const void* buf, int length)
{
	int writed = fwrite(buf, 1, length, m_file);
	m_length += writed;
	return writed;
}

int EndianBinaryWriter::WriteBytes(int position, const void* var, int length)
{
	int pos = GetPosition();
	SetPosition(position);
	int writed = WriteBytes(var, length);
	SetPosition(pos);
	return writed;
}

int EndianBinaryWriter::GetLength()
{
	return m_length;
}

int EndianBinaryWriter::GetPosition()
{
	return ftell(m_file);
}

void EndianBinaryWriter::SetPosition(int position)
{
	fseek(m_file, position, SEEK_SET);
}

void EndianBinaryWriter::Flush()
{
	fflush(m_file);
}

void EndianBinaryWriter::Close()
{
	fclose(m_file);
}

EndianType EndianBinaryWriter::GetEndianType()
{
	return m_endian;
}

void EndianBinaryWriter::SetEndianType(EndianType type)
{
	m_endian = type;
}

int EndianBinaryWriter::WriteWithEndian(void* ptr, int length)
{
	if (m_endian == BigEndian)
	{
		Reverse(ptr, length);
	}
	return WriteBytes(ptr, length);
}

EndianBinaryWriter::EndianBinaryWriter(FILE* file)
{
	m_file = file;
	m_length = 0;
	m_endian = BigEndian;
}

EndianBinaryWriter::~EndianBinaryWriter()
{
	fclose(m_file);
}