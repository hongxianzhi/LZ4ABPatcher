#include "Parser.h"
#include <assert.h>
#include "AssetChunk.h"
#include "EndianBinaryReader.h"

EndianBinaryReader* EndianBinaryReader::Create(FILE* file)
{
	return new EndianBinaryReaderForFile(file);
}

EndianBinaryReader* EndianBinaryReader::Create(const void* buf, int buf_len)
{
	return new EndianBinaryReaderForMemory((const char*)buf, buf_len);
}

EndianBinaryReader* EndianBinaryReader::Create(const std::vector<AssetChunk*>& chunks, int discard, int chunk_length)
{
	return new EndianBinaryReaderForChunk(chunks, discard, chunk_length);
}

bool EndianBinaryReader::Reverse(void* var, int length)
{
	if (var == NULL || length <= 1)
	{
		return false;
	}
	char* ptr = (char*)var;
	int swapCount = length / 2;
	for (int i = 0; i < swapCount; i++)
	{
		char c = *(ptr + i);
		*(ptr + i) = *(ptr + length - i - 1);
		*(ptr + length - i - 1) = c;
	}
	return true;
}

bool EndianBinaryReader::ReadBoolean()
{
	bool var = false;
	ReadBytes(&var, 1);
	return var;
}

unsigned char EndianBinaryReader::ReadByte()
{
	unsigned char var = 0;
	ReadBytes(&var, 1);
	return var;
}

void EndianBinaryReader::AlignStream(int alignment)
{
	if (alignment > 0)
	{
		int pos = GetPosition();
		int mod = pos % alignment;
		if (mod != 0)
		{
			SetPosition(pos + alignment - mod);
		}
	}
}

short EndianBinaryReader::ReadInt16()
{
	short var = 0;
	ReadBytes(&var, sizeof(short));
	if (m_endian == BigEndian)
	{
		Reverse(&var, sizeof(short));
	}
	return var;
}

int EndianBinaryReader::ReadInt32()
{
	int var = 0;
	ReadBytes(&var, sizeof(int));
	if (m_endian == BigEndian)
	{
		Reverse(&var, sizeof(int));
	}
	return var;
}

unsigned int EndianBinaryReader::ReadUInt32()
{
	unsigned int var = 0;
	ReadBytes(&var, sizeof(unsigned int));
	if (m_endian == BigEndian)
	{
		Reverse(&var, sizeof(unsigned int));
	}
	return var;
}

long long EndianBinaryReader::ReadInt64()
{
	long long var = 0;
	ReadBytes(&var, sizeof(long long));
	if (m_endian == BigEndian)
	{
		Reverse(&var, sizeof(long long));
	}
	return var;
}

void EndianBinaryReader::Trim()
{

}

void EndianBinaryReader::LockTrim(bool lock)
{

}

const void* EndianBinaryReader::RetPointer(int pos)
{
	return NULL;
}

int EndianBinaryReader::Prepare(int length)
{
	int pos = GetPosition();
	SetPosition(GetPosition() + length);
	return GetPosition() - pos;
}

EndianType EndianBinaryReader::GetEndianType()
{
	return m_endian;
}

void EndianBinaryReader::SetEndianType(EndianType type)
{
	m_endian = type;
}

EndianBinaryReader::EndianBinaryReader()
{
	m_endian = BigEndian;
}

EndianBinaryReader::~EndianBinaryReader()
{

}

EndianBinaryReaderForFile::EndianBinaryReaderForFile(FILE * file)
{
	m_file = file;
	if (m_file != NULL)
	{
		int pos = ftell(m_file);
		fseek(m_file, 0, SEEK_END);
		m_file_length = ftell(m_file);
		fseek(m_file, pos, SEEK_SET);
	}
}

EndianBinaryReaderForFile::~EndianBinaryReaderForFile()
{

}

char * EndianBinaryReaderForFile::ReadStringToNull()
{
	char* str = NULL;
	if (m_file != NULL)
	{
		char c = 0;
		int position = ftell(m_file);
		while (fread(&c, 1, 1, m_file) == 1 && c != 0) {}
		int length = ftell(m_file) - position;
		str = (char*)malloc(length);
		fseek(m_file, -length, SEEK_CUR);
		fread(str, 1, length, m_file);
		str[length - 1] = 0;
	}
	return str;
}

void EndianBinaryReaderForFile::ReadStringToNull(char * buffer, int length)
{
	if (buffer != NULL && length > 0)
	{
		char c = 0;
		int position = ftell(m_file);
		while (fread(&c, 1, 1, m_file) == 1 && c != 0) {}
		int str_len = ftell(m_file) - position;
		str_len = str_len <= length ? str_len : length;
		fseek(m_file, position, SEEK_SET);
		fread(buffer, 1, str_len, m_file);
		buffer[str_len - 1] = 0;
	}
}

void* EndianBinaryReaderForFile::ReadBytes(int length)
{
	void* bytes = NULL;
	if(m_file != NULL && length > 0 && ftell(m_file) + length <= m_file_length)
	{
		bytes = malloc(length);
		fread(bytes, 1, length, m_file);
	}
	return bytes;
}

int EndianBinaryReaderForFile::ReadBytes(void * buffer, int length)
{
	if (m_file != NULL && length > 0)
	{
		return fread(buffer, 1, length, m_file);
	}
	return 0;
}

int EndianBinaryReaderForFile::GetLength()
{
	return m_file_length;
}

int EndianBinaryReaderForFile::GetPosition()
{
	return m_file != NULL ? ftell(m_file) : 0;
}

void EndianBinaryReaderForFile::SetPosition(int position)
{
	m_file != NULL ? fseek(m_file, position, SEEK_SET) : 0;
}

EndianBinaryReader* EndianBinaryReaderForFile::Clone()
{
	return new EndianBinaryReaderForFile(m_file);
}

EndianBinaryReaderForMemory::EndianBinaryReaderForMemory(const char * buffer, int buffer_length)
{
	m_buf = buffer;
	m_position = 0;
	m_buf_len = buffer_length > 0 ? buffer_length : 0;
}

EndianBinaryReaderForMemory::~EndianBinaryReaderForMemory()
{

}

const void* EndianBinaryReaderForMemory::RetPointer(int offset)
{
	return (offset >= 0 && offset < m_buf_len) ? m_buf + offset : NULL;
}

char* EndianBinaryReaderForMemory::ReadStringToNull()
{
	if (m_buf == NULL || m_buf_len <= 0 || m_position >= m_buf_len)
	{
		return NULL;
	}

	const char* str_begin = m_buf + m_position;
	const char* str_end = str_begin;
	int length = m_buf_len - m_position;
	while (*str_end++ != 0 && length-- > 0) {}
	
	length = str_end - str_begin;
	char* str = (char*)malloc(length);
	memcpy(str, str_begin, length);
	m_position += length;
	str[length - 1] = 0;
	return str;
}

void EndianBinaryReaderForMemory::ReadStringToNull(char* buf, int buf_len)
{
	if (buf != NULL && buf_len > 0)
	{
		const char* str_begin = m_buf + m_position;
		const char* str_end = str_begin;
		int length = m_buf_len - m_position;
		while (*str_end++ != 0 && length-- > 0) {}

		length = str_end - str_begin;
		length = length <= buf_len ? length : buf_len;
		memcpy(buf, str_begin, length);
		m_position += length;
		buf[length - 1] = 0;
	}
}

void* EndianBinaryReaderForMemory::ReadBytes(int length)
{
	void* ret = NULL;
	if (m_buf != NULL && m_buf_len > 0 && length > 0 && m_position + length <= m_buf_len)
	{
		ret = malloc(length);
		memcpy(ret, m_buf + m_position, length);
		m_position += length;
	}
	return ret;
}

int EndianBinaryReaderForMemory::ReadBytes(void* buffer, int length)
{
	if (m_buf != NULL && m_buf_len > 0 && length > 0 && m_position < m_buf_len)
	{
		length = m_position + length > m_buf_len ? m_buf_len - m_position : length;
		memcpy(buffer, m_buf + m_position, length);
		m_position += length;
	}
	else
	{
		length = 0;
	}
	return length;
}

int EndianBinaryReaderForMemory::GetLength()
{
	return m_buf_len;
}

int EndianBinaryReaderForMemory::GetPosition()
{
	return m_position;
}

void EndianBinaryReaderForMemory::SetPosition(int position)
{
	if (position >= 0 && position <= m_buf_len)
	{
		m_position = position;
	}
}

EndianBinaryReader* EndianBinaryReaderForMemory::Clone()
{
	EndianBinaryReaderForMemory* reader = new EndianBinaryReaderForMemory(m_buf, m_buf_len);
	reader->m_position = m_position;
	return reader;
}

EndianBinaryReaderForChunk::EndianBinaryReaderForChunk(std::vector<AssetChunk*> chunks, int discard, int chunk_length)
{
	int pos = 0;
	m_bytes_discard = discard;
	m_chunks_bytes_length = chunk_length;
	m_chunks.resize(chunks.size());
	for (int i = 0; i < m_chunks.size(); i++)
	{
		AssetChunkData* data = new AssetChunkData();
		data->chunk = chunks[i];
		data->memorypos = -1;
		data->virtualpos = pos;
		m_chunks[i] = data;
		pos += data->chunk->GetUnCompressedSize();
	}

	Ensure(0, CHUNK_LENGTH);
	Ensure(1, CHUNK_LENGTH);
}

EndianBinaryReaderForChunk::EndianBinaryReaderForChunk(EndianBinaryReaderForChunk* reader)
{
	m_buffers[0] = m_buffers[1] = NULL;
	m_buffer_length[0] = m_buffer_length[1] = 0;
	if (reader != NULL)
	{
		m_bytes_discard = reader->m_bytes_discard;
		m_chunks_bytes_length = reader->m_chunks_bytes_length;
		m_chunks.resize(reader->m_chunks.size());

		int startpos = 0;
		for (int i = 0; i < reader->m_chunks.size(); i++)
		{
			AssetChunkData* src = reader->m_chunks[i];
			AssetChunkData* dst = new AssetChunkData();
			dst->chunk = src->chunk;
			dst->memorypos = -1;
			dst->virtualpos = src->virtualpos;
			m_chunks[i] = dst;
		}
	}
}

EndianBinaryReaderForChunk::~EndianBinaryReaderForChunk()
{
	SAFE_FREE(m_buffers[0]);
	m_buffer_length[0] = 0;

	m_buffer_used = 0;
	SAFE_FREE(m_buffers[1]);
	m_buffer_length[1] = 0;
}

const void* EndianBinaryReaderForChunk::RetPointer(int pos)
{
	pos = PositionToBufferOffset(pos + m_bytes_discard);
	return (pos >= 0 && pos < m_buffer_length[1]) ? (char*)m_buffers[1] + pos : NULL;
}

char* EndianBinaryReaderForChunk::ReadStringToNull()
{
	char c = 0;
	int pos = GetPosition();
	LockTrim(true);
	while (ReadBytes(&c, 1) && c != 0) {};
	LockTrim(false);
	int length = GetPosition() - pos;
	char* str = (char*)malloc(length);
	const void* src = RetPointer(pos);
	memcpy(str, src, length);
	str[length - 1] = 0;
	return str;
}

void EndianBinaryReaderForChunk::ReadStringToNull(char* buf, int length)
{
	if (buf == NULL || length <= 0)
	{
		return;
	}

	char c = 0;
	int pos = GetPosition();
	LockTrim(true);
	while (ReadBytes(&c, 1) && c != 0) {};
	LockTrim(false);
	int maxlen = GetPosition() - pos;
	length = length <= maxlen ? length : maxlen;
	const void* src = RetPointer(pos);
	memcpy(buf, src, length);
	buf[length - 1] = 0;
}

void* EndianBinaryReaderForChunk::ReadBytes(int length)
{
	void* buf = NULL;
	if (length > 0)
	{
		buf = malloc(length);
		ReadBytes(buf, length);
	}
	return buf;
}

int EndianBinaryReaderForChunk::ReadBytes(void* buf, int length)
{
	if (buf == NULL || length <= 0)
	{
		return 0;
	}
	assert(m_chunks_bytes_length - m_position >= length);
	int prepared = Prepare(length);
	assert(prepared == length);
	const void* src = RetPointer(GetPosition() - prepared);
	memcpy(buf, src, length);
	return length;
}

int EndianBinaryReaderForChunk::GetLength()
{
	return m_chunks_bytes_length;
}

int EndianBinaryReaderForChunk::GetPosition()
{
	return m_position;
}

void EndianBinaryReaderForChunk::SetPosition(int position)
{
	position = position < m_chunks_bytes_length ? position : m_chunks_bytes_length;
	position = position >= 0 ? position : 0;
	m_position = position;
}

EndianBinaryReader* EndianBinaryReaderForChunk::Clone()
{
	return new EndianBinaryReaderForChunk(this);
}

void EndianBinaryReaderForChunk::Trim()
{
	if (m_trim_lock == 0)
	{
		int bytes_unused = 0;
		int firstchunkused = PositionToChunkIndex(m_position);
		for (int i = firstchunkused - 1; i >= 0; i--)
		{
			AssetChunkData* data = m_chunks[i];
			if (data->memorypos >= 0)
			{
				data->memorypos = -1;
				bytes_unused += data->chunk->GetUnCompressedSize();
			}
			else
			{
				break;
			}
		}

		if (bytes_unused > 0)
		{
			char* buf = (char*)m_buffers[1];
			for (int i = firstchunkused; i < m_chunks.size(); i++)
			{
				AssetChunkData* data = m_chunks[i];
				if (data->memorypos >= 0)
				{
					int memorypos = data->memorypos - bytes_unused;
					memmove(buf + memorypos, buf + data->memorypos, data->chunk->GetUnCompressedSize());
					data->memorypos = memorypos;
				}
				else
				{
					break;
				}
			}
		}
	}
}

void EndianBinaryReaderForChunk::LockTrim(bool lock)
{
	m_trim_lock += (lock ? 1 : -1);
	if (lock == false)
	{
		Trim();
	}
}

int EndianBinaryReaderForChunk::Prepare(int length)
{
	int pos = GetPosition();
	length = length > 0 ? length : 0;
	if (length >= 0)
	{
		bool found = true;
		int chunk_start = PositionToChunkIndex(m_position);
		int chunk_end = PositionToChunkIndex(m_position + length);
		for (int i = chunk_start; i <= chunk_end; i++)
		{
			AssetChunkData* data = m_chunks[i];
			if (data->memorypos < 0)
			{
				found = false;
				break;
			}
		}

		if (found == false)
		{
			for (int i = 0; i < chunk_start && i < m_chunks.size(); i++)
			{
				AssetChunkData* data = m_chunks[i];
				if (data->memorypos >= 0)
				{
					chunk_start = i;
					break;
				}
			}

			for (int i = m_chunks.size() - 1; i > chunk_end; i--)
			{
				AssetChunkData* data = m_chunks[i];
				if (data->memorypos >= 0)
				{
					chunk_end = i;
					break;
				}
			}
			Ensure(1, (chunk_end - chunk_start + 1) * CHUNK_LENGTH);

			int offset = 0;
			for (int i = chunk_start; i <= chunk_end; i++)
			{
				AssetChunkData* data = m_chunks[i];
				if (data->memorypos != offset)
				{
					data->memorypos = offset;
					offset += data->chunk->GetUnCompressData(m_buffers[0], m_buffer_length[0], (char*)m_buffers[1] + offset, CHUNK_LENGTH);
				}
				else
				{
					offset += data->chunk->GetUnCompressedSize();
				}
			}
		}
		SetPosition(GetPosition() + length);
	}
	return GetPosition() - pos;
}

int EndianBinaryReaderForChunk::PositionToChunkIndex(int position)
{
	assert(position >= 0);
	if (position < 0)
	{
		push_message("PositionToChunkIndex: error position %d", position);
	}
	position += m_bytes_discard;
	int index = position / CHUNK_LENGTH;
	if (index < 0 || index >= m_chunks.size())
	{
		push_message("PositionToChunkIndex: error transform, index %d", index);
		assert(false);
	}
	return index;
}

//ThunkÆ«ÒÆ -> ÄÚ´æÆ«ÒÆ
int EndianBinaryReaderForChunk::PositionToBufferOffset(int position)
{
	int index = PositionToChunkIndex(position);
	AssetChunkData* data = m_chunks[index];
	assert(data->memorypos >= 0/*"PositionToBufferOffset:Ã»ÓÐ»º´æµÄchunk¡£"*/);
	return data->memorypos + (position + m_bytes_discard) % CHUNK_LENGTH;
}

void EndianBinaryReaderForChunk::Ensure(int index, int length)
{
	if (m_buffer_length[index] < length)
	{
		length = (length / CHUNK_LENGTH + (length % CHUNK_LENGTH == 0 ? 0 : 1)) * CHUNK_LENGTH;
		void* buf = m_buffers[index];
		int lo = m_buffer_length[index];
		void* buf_new = malloc(length);
		if (buf != NULL)
		{
			memcpy(buf_new, buf, lo);
			free(buf);
		}
		m_buffers[index] = buf_new;
		m_buffer_length[index] = length;
	}
}