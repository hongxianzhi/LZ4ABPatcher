#if !defined(__APPLE__) && !defined(__ANDROID__)
#include <io.h>
#include<direct.h>
#define ACCESS(PATH, MODE) _access(PATH, MODE)
#define MKDIR(PATH, MODE) _mkdir(PATH)
#else
#include <unistd.h>
#include <sys/stat.h>
#define ACCESS(PATH, MODE) access(PATH, MODE)
#define MKDIR(PATH, MODE) mkdir(PATH, MODE)
#endif
#include "md5.h"
#include <assert.h>
#include <cstdio>
#include "lz4hc.h"
#include <algorithm>
#include "EntryItem.h"
#include "BundleFile.h"
#include "MergeParser.h"
#include "EndianBinaryWriter.h"

BundlePatchFile::BundlePatchFile(EndianBinaryReader* reader)
{
	m_datastart = 0;
	m_reader = reader;
	m_patchtype = DIFFERENCE_MODIFY;
	if (reader == NULL)
	{
		return;
	}
	int patchbegin = m_reader->GetPosition();
	m_reader->SetEndianType(LittleEndian);
	m_bundlename = m_reader->ReadStringToNull();
	int patchsize = m_reader->ReadInt32();
	if (m_reader->GetLength() - patchbegin < patchsize)
	{
		assert(false /*"文件尺寸和记录的不符合，已经损坏。"*/);
		return;
	}
	m_patchtype = m_reader->ReadInt16();
	int version = m_reader->ReadInt32();
	switch (version)
	{
	case 1:
		break;

	default:
		assert(false /*"不支持的patch版本"*/);
		return;
	}
	m_datastart = m_reader->GetPosition();

	if (m_patchtype == DIFFERENCE_MODIFY)
	{
		m_reader->ReadBytes(m_digestold, 16);
		m_reader->ReadBytes(m_digestnew, 16);
		m_tableOffset = m_reader->ReadInt32();
		m_tableUnCompressed = m_reader->ReadInt32();
		bundleHeaders = new EntryItemPatchInfo();
		GetEntryItemPatchInfo(m_reader, bundleHeaders->position, bundleHeaders->length);

		int count = m_reader->ReadInt32();
		m_entrys.resize(count);
		for (int index = 0; index < count; index++)
		{
			EntryPatch* patch = new EntryPatch();
			if (LoadEntryPatch(m_reader, patch))
			{
				m_entrys[index] = patch;
			}
			else
			{
				assert(false /*"从patch中读取entry信息失败"*/);
				delete patch;
			}
		}
		if (m_reader->GetPosition() - patchbegin != patchsize)
		{
			assert(false);
			push_message("反序列化patch异常。");
		}
	}
}

BundlePatchFile::~BundlePatchFile()
{
	for (int i = 0; i < m_entrys.size(); i++)
	{
		EntryPatch* info = m_entrys[i];
		SAFE_DELETE(info->m_entrytables);
		std::map<int, PreloadPatchItem *>::iterator it = info->m_preloads.begin();
		while (it != info->m_preloads.end())
		{
			PreloadPatchItem* item = it++->second;
			SAFE_DELETE(item);
		}
		SAFE_FREE(info->m_md5codes);
	}
}

EntryPatch* BundlePatchFile::GetEntryPatch(int index)
{
	return (index >= 0 && index < m_entrys.size()) ? m_entrys[index] : NULL;
}

bool BundlePatchFile::GetEntryItemPatchInfo(EndianBinaryReader* reader, int& offset, int& length)
{
	length = 0;
	if (reader == NULL)
	{
		return false;
	}
	
	unsigned char type = reader->ReadByte();
	if (type != DIFFERENCE_KEEP)
	{
		length = reader->ReadInt32();
		offset = reader->GetPosition();
		reader->Prepare(length);
	}
	return true;
}

bool BundlePatchFile::LoadEntryPatch(EndianBinaryReader* reader, EntryPatch* patch)
{
	patch->m_name = reader->ReadStringToNull();
	EntryItemPatchInfo* entrytables = new EntryItemPatchInfo();
	GetEntryItemPatchInfo(m_reader, entrytables->position, entrytables->length);
	patch->m_entrytables = entrytables;
	int count = m_reader->ReadInt32();
	if (count > 0)
	{
		unsigned char* md5codes = (unsigned char*)malloc(count * 16);
		unsigned char* md5ptr = md5codes;
		for (int i = 0; i < count; i++)
		{
			reader->ReadBytes(md5ptr, 16);
			md5ptr += 16;
			PreloadPatchItem* item = new PreloadPatchItem();
			item->fromsrc = reader->ReadBoolean();
			item->index_src = reader->ReadInt32();
			item->index_dst = reader->ReadInt32();
			if (item->fromsrc == false && item->index_dst == item->index_src)
			{
				GetEntryItemPatchInfo(m_reader, item->position, item->length);
			}
			patch->m_preloads[item->index_dst] = item;
		}
		patch->m_md5codes = md5codes;
	}
	return true;
}

int BundlePatchFile::ReadBytes(void* buf, int buflen, int position)
{
	m_reader->SetPosition(position);
	if (m_reader->GetPosition() != position)
	{
		return 0;
	}
	return m_reader->ReadBytes(buf, buflen);
}

ParserForWriteBundleHeader::ParserForWriteBundleHeader(BundlePatchFile* patcher, EndianBinaryWriter* writer)
{
	m_patcher = patcher;
	m_writer = writer;
}

ParserForWriteBundleHeader::~ParserForWriteBundleHeader()
{

}

void formatpath(char* pathbuff, const char* dir, const char* filename)
{
	sprintf(pathbuff, dir[strlen(dir) - 1] == '/' ? "%s%s" : "%s/%s", dir, filename);
}

void ParserForWriteBundleHeader::MergeFormPatch(const char* patch, const char* searchpath, const char* newpath, PROGRESSHANDLE handler)
{
	if (patch == NULL || *patch == 0 || searchpath == NULL || *searchpath == 0 || newpath == NULL || *newpath == 0)
	{
		push_message("MergeFormPatch: error params");
		return;
	}

	if (ACCESS(newpath, 06) == -1)
	{
		push_message("MergeFormPatch: mkdir '%s'", newpath);
		MKDIR(newpath, 777);
	}

	FILE* file_patch = fopen(patch, "rb");
	if (file_patch != NULL)
	{
		EndianBinaryReader* reader = EndianBinaryReader::Create(file_patch);
		reader->SetPosition(0);
		reader->SetEndianType(LittleEndian);

		int lastposition = 0;
		int signlen = strlen(PATCH_SIGN) + 1;
		void* sign_data = malloc(signlen);
		if (reader->ReadBytes(sign_data, signlen) && memcmp(sign_data, PATCH_SIGN, signlen) == 0)
		{
			int patchBlock = reader->ReadInt32();
			if (patchBlock > 0)
			{
				lastposition = reader->GetPosition();
				char* merger_path = (char*)malloc(512);
				for (int index = 0; index < patchBlock; index++)
				{
					reader->SetPosition(lastposition);
					BundlePatchFile* bundle_patch = new BundlePatchFile(reader);
					lastposition = reader->GetPosition();
					const char* bundlename = bundle_patch->GetBundleName();
					if (handler != NULL)
					{
						handler(bundlename, index, patchBlock);
					}
					formatpath(merger_path, newpath, bundlename);
					switch (bundle_patch->GetPatchType())
					{
					case DIFFERENCE_ADD:
					{
						void* buff = malloc(CHUNK_LENGTH);
						int position = bundle_patch->GetDataStartPosition();
						EndianBinaryWriter* writer = EndianBinaryWriter::Create(merger_path, "wb+");
						int len = bundle_patch->ReadBytes(buff, CHUNK_LENGTH, position);
						while (len > 0)
						{
							writer->WriteBytes(buff, len);
							position += len;
							len = bundle_patch->ReadBytes(buff, CHUNK_LENGTH, position);
						}
						writer->Flush();
						writer->Close();
						SAFE_FREE(buff);
						SAFE_DELETE(writer);
					}
					break;

					case DIFFERENCE_DELETE:
					{
						remove(merger_path);
					}
					break;

					case  DIFFERENCE_MODIFY:
					{
						char* path_from = (char*)malloc(512);
						formatpath(path_from, searchpath, bundlename);
						FILE* file_from = fopen(path_from, "rb");
						if (file_from != NULL)
						{
							char digest[16] = { 0 };
							fseek(file_from, -16, SEEK_END);
							if (fread(digest, 1, 16, file_from) != 16 || memcmp(digest, bundle_patch->m_digestold, 16) != 0)
							{
								push_message("assetbundle sign unmatch.");
							}
							else
							{
								fseek(file_from, 0, SEEK_SET);
								sprintf(path_from, newpath[strlen(newpath) - 1] == '/' ? "%s%s.tmp" : "%s/%s.tmp", newpath, bundlename);
								EndianBinaryWriter* tempfile = EndianBinaryWriter::Create(path_from, "wb+");
								if (tempfile != NULL)
								{
									BundleFile* bundle_from = new BundleFile(EndianBinaryReader::Create(file_from));
									ParserForWriteBundleHeader* parser = new ParserForWriteBundleHeader(bundle_patch, tempfile);
									bundle_from->Parse(parser);

									tempfile->Flush();
									tempfile->Close();
									SAFE_DELETE(tempfile);

									remove(merger_path);
									rename(path_from, merger_path);

									SAFE_DELETE(parser);
									SAFE_DELETE(bundle_from);
								}
								else
								{
									push_message("create '%s' failed", path_from);
								}
							}
							fclose(file_from);
						}
						SAFE_FREE(path_from);
					}
					break;;

					default:
					{
						assert(false);
					}
					break;
					}

					SAFE_DELETE(bundle_patch);
				}
				free(merger_path);
			}
		}
		SAFE_FREE(sign_data);
		fclose(file_patch);
	}
	else
	{
		push_message("open '%s' failed.", patch);
	}
}

int ParserForWriteBundleHeader::ReadEntryData(const std::string& entryName, int index, void*& bytes)
{
	std::map<std::string, std::vector<EntryItemPatchInfo *> *>::iterator it = m_entrys.find(entryName);
	assert(it != m_entrys.end());
	std::vector<EntryItemPatchInfo*>* vector_ptr = it->second;
	assert(index >= 0 && index < vector_ptr->size());
	EntryItemPatchInfo* info = (*vector_ptr)[index];
	m_srcreader->SetPosition(info->position);
	bytes = malloc(info->length);
	return m_srcreader->ReadBytes(bytes, info->length);
}

void ParserForWriteBundleHeader::BeginParseEntryPreloadInfo(EntryItem* item, int count)
{
	m_currentEntryData = new std::vector<EntryItemPatchInfo *>();
	m_currentEntryData->resize(count);
	m_entrys[item->getName()] = m_currentEntryData;
}

void ParserForWriteBundleHeader::ParseEntryPreloadInfo(EntryItem* item, int index, long long pathID, int position, int size, int addtionlen)
{
	EntryItemPatchInfo* info = new EntryItemPatchInfo();
	info->position = position;
	info->length = size;
	(*m_currentEntryData)[index] = info;
}

bool ParserForWriteBundleHeader::BeginParseEntryPreloadData(EntryItem* item, int count)
{
	return false;
}

void ParserForWriteBundleHeader::EndParseBundleInfo(BundleFile* file)
{
	m_srcreader = file->GetChunkReader();

	//写文件头
	m_writer->SetPosition(0);
	
	//复制table数据
	EntryItemPatchInfo* info = m_patcher->bundleHeaders;
	void* headerWidthTableInfo = malloc(info->length);	
	m_patcher->ReadBytes(headerWidthTableInfo, info->length, info->position);

	EndianBinaryReader* reader = EndianBinaryReader::Create(headerWidthTableInfo, info->length);
	BundleFile* bundle = new BundleFile(reader);
	BundleFileParserForEntry* parser = new BundleFileParserForEntry(this, m_patcher, m_writer);
	bundle->Parse(parser);

	SAFE_DELETE(parser);
	SAFE_DELETE(bundle);
	SAFE_FREE(headerWidthTableInfo);
}

BundleFileParserForEntry::BundleFileParserForEntry(ParserForWriteBundleHeader* oldbundleparser, BundlePatchFile* patcher, EndianBinaryWriter* writer)
{
	m_buffer_used = 0;
	m_buffers[0] = m_buffers[1] = NULL;
	m_buffer_length[0] = m_buffer_length[1] = 0;
	m_bundleParserOld = oldbundleparser;
	m_patcher = patcher;
	m_writer = writer;
	m_lastentryinfo = NULL;

	m_bytespushed = 0;
	Ensure(0, CHUNK_LENGTH);
	Ensure(1, CHUNK_LENGTH);
}

BundleFileParserForEntry::~BundleFileParserForEntry()
{
	SAFE_FREE(m_lastentryinfo);
	for (int i = 0; i < m_chunks.size(); i++)
	{
		SAFE_DELETE(m_chunks[i]);
	}
}

void BundleFileParserForEntry::BeginParseChunksInfo(int count)
{
	if (m_tempfile != NULL)
	{
		fclose(m_tempfile);
		m_tempfile = NULL;
	}
	m_tempfile = fopen(tmpnam(m_tempfilename), "wb+");
}

void BundleFileParserForEntry::BeginParseEntryPreloadInfo(EntryItem* item, int count)
{
	m_preloads.resize(count);
}

void BundleFileParserForEntry::ParseEntryPreloadInfo(EntryItem* item, int index, long long pathID, int position, int size, int addtionlen)
{
	if (m_error)
	{
		return;
	}

	PreloadItemData* data = new PreloadItemData();
	data->index = index;
	data->position = position;
	m_preloads[index] = data;
}

bool BundleFileParserForEntry::SortEntryItem(PreloadItemData* v1, PreloadItemData* v2)
{
	return v1->position < v2->position;
}

void BundleFileParserForEntry::EndParseEntryPreloadInfo(EntryItem* item)
{
	std::sort(m_preloads.begin(), m_preloads.end(), SortEntryItem);
	for (int i = 0; m_error == false && i < m_preloads.size(); i++)
	{
		void* bytes = NULL;
		bool fromsrc = false;
		int readfromindex = 0;
		PreloadItemData* preload = m_preloads[i];
		int index = preload->index;
		PreloadPatchItem* patch = NULL;
		int position = preload->position;
		std::map<int, PreloadPatchItem *>::iterator it = m_currentPatch->m_preloads.find(index);
		if (it == m_currentPatch->m_preloads.end())
		{
			fromsrc = true;
			readfromindex = index;
		}
		else
		{
			patch = it->second;
			fromsrc = patch->fromsrc;
			readfromindex = patch->index_src;
		}

		int readed = 0;
		if (fromsrc)
		{
			readed = m_bundleParserOld->ReadEntryData(item->getName(), readfromindex, bytes);
		}
		else
		{
			while (patch->index_dst != patch->index_src)
			{
				it = m_currentPatch->m_preloads.find(patch->index_src);
				if (it == m_currentPatch->m_preloads.end())
				{
					break;
				}
				else
				{
					patch = it->second;
				}
			}
			bytes = malloc(patch->length);
			readed = m_patcher->ReadBytes(bytes, patch->length, patch->position);
		}

		char md5code[16];
		md5((const char*)bytes, readed, md5code);
		unsigned char* md5saved = m_currentPatch->m_md5codes + index * 16;
		if (memcmp(md5code, md5saved, 16) == 0)
		{
			PushEmptyBytes(position + m_entryOffset - m_bytespushed);
			PushBytes((const char*)bytes, readed);
		}
		else
		{
			m_error = true;
		}

		SAFE_FREE(bytes);
	}
}

EntryItem* BundleFileParserForEntry::CreateEntryItem(int index, std::string& entryname, EndianBinaryReader* assetsFileReader, long long entryinfo_offset, long long entryinfo_size, int flag)
{
	assert(entryinfo_offset == m_bytespushed);
	m_entryOffset = entryinfo_offset;
	m_currentPatch = m_patcher->GetEntryPatch(index);
	EntryItemPatchInfo* info = m_currentPatch->m_entrytables;
	SAFE_FREE(m_lastentryinfo);
	m_lastentryinfo = malloc(info->length);
	m_patcher->ReadBytes(m_lastentryinfo, info->length, info->position);
	PushBytes((const char*)m_lastentryinfo, info->length);
	assetsFileReader = EndianBinaryReader::Create(m_lastentryinfo, info->length);
	return BundleFileParser::CreateEntryItem(index, entryname, assetsFileReader, 0, 0, flag);
}

void BundleFileParserForEntry::EndParseBundleInfo(BundleFile* bundle)
{
	WriteTempFile();

	//填写block数据
	EndianBinaryReader* blockReader = bundle->GetBlockReader();
	blockReader->SetPosition(0x10);
	int count = blockReader->ReadInt32();
	assert(count == m_chunks.size());
	int pos = blockReader->GetPosition();
	for (int i = 0; i < count; i++)
	{
		int uncompressedSize = m_chunks[i]->uncompressed;
		EndianBinaryReader::Reverse(&uncompressedSize, 4);
		int compressedSize = m_chunks[i]->compressed;
		EndianBinaryReader::Reverse(&compressedSize, 4);
		short flag = 3;
		EndianBinaryReader::Reverse(&flag, 2);
		
		*(int*)blockReader->RetPointer(pos) = uncompressedSize;
		pos += 4;
		*(int*)blockReader->RetPointer(pos) = compressedSize;
		pos += 4;
		*(short*)blockReader->RetPointer(pos) = flag;
		pos += 2;
	}

	int compressedBuffSize = blockReader->GetLength();
	void* tableCompressed = malloc(compressedBuffSize);
	int compressedSize = LZ4_compress_HC((const char*)blockReader->RetPointer(0), (char*)tableCompressed, blockReader->GetLength(), compressedBuffSize, LZ4HC_CLEVEL_MAX);

	int size = compressedSize;
	EndianBinaryReader::Reverse(&size, 4);

	pos = bundle->getBlockStartPosition();
	EndianBinaryReader* header_reader = bundle->GetHeaderReader();
	*(int*)header_reader->RetPointer(pos) = size;
	pos += 12;
	m_writer->WriteBytes(header_reader->RetPointer(0), pos);

	m_writer->WriteBytes(tableCompressed, compressedSize);
	SAFE_FREE(tableCompressed);

	fseek(m_tempfile, 0, SEEK_SET);
	while (true)
	{
		int readed = fread(m_buffers[0], 1, m_buffer_length[0], m_tempfile);
		m_writer->WriteBytes(m_buffers[0], readed);
		if (readed < m_buffer_length[0])
		{
			break;
		}
	}
	fclose(m_tempfile);
	remove(m_tempfilename);
	long long bundleSize = m_writer->GetLength();
	m_writer->WriteBytes(m_patcher->m_digestnew, 16);
	m_writer->SetPosition(bundle->getBundleSizePosition());
	m_writer->WriteInt64(bundleSize);
}

void BundleFileParserForEntry::Ensure(int index, int length)
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

void BundleFileParserForEntry::PushEmptyBytes(int length)
{
	if (length <= 0)
	{
		return;
	}

	m_bytespushed += length;
	while (length > 0)
	{
		int pushed = ChunkLeft();
		pushed = pushed > length ? length : pushed;
		length -= pushed;
		memset((char*)m_buffers[1] + m_buffer_used, 0, pushed);
		m_buffer_used += pushed;
		if (ChunkLeft() == 0)
		{
			WriteTempFile();
		}
	}
}

void BundleFileParserForEntry::PushBytes(const char* bytes, int length)
{
	m_bytespushed += length;
	while (length > 0)
	{
		int pushed = ChunkLeft();
		pushed = pushed > length ? length : pushed;
		length -= pushed;
		memcpy((char*)m_buffers[1] + m_buffer_used, bytes, pushed);
		m_buffer_used += pushed;
		bytes += pushed;

		if (ChunkLeft() == 0)
		{
			WriteTempFile();
		}
	}
}

void BundleFileParserForEntry::WriteTempFile()
{
	if (m_buffer_used > 0)
	{
		ChunkItemData* item = new ChunkItemData();
		int size = LZ4_compress_HC((const char*)m_buffers[1], (char*)m_buffers[0], m_buffer_used, m_buffer_length[0], LZ4HC_CLEVEL_MIN);
		item->uncompressed = m_buffer_used;
		item->compressed = size;
		m_chunks.push_back(item);
		fwrite(m_buffers[0], 1, size, m_tempfile);
		fflush(m_tempfile);
		m_buffer_used = 0;
	}
}

void assetbundle_merge(const char* patch, const char* searchpath, const char* newpath, PROGRESSHANDLE handler)
{
	ParserForWriteBundleHeader::MergeFormPatch(patch, searchpath, newpath, handler);
}