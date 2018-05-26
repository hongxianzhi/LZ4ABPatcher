#include "md5.h"
#include <assert.h>
#include "BundleFile.h"
#include "EntryItem.h"
#include "ComparePaser.h"
#include "EndianBinaryWriter.h"

BundleFileParserForCompress::EntryData::~EntryData()
{
	SAFE_DELETE(entryHeader);

	for (int i = 0; i < preloadhashs.size(); i++)
	{
		SAFE_DELETE(preloadhashs[i]);
	}

	for (int i = 0; i < preloads.size(); i++)
	{
		SAFE_DELETE(preloads[i]);
	}
}

BundleFileParserForCompress::~BundleFileParserForCompress()
{
	SAFE_DELETE(m_bundleHeader);
	for (int i = 0; i < m_EntryDatas.size(); i++)
	{
		SAFE_DELETE(m_EntryDatas[i]);
	}
	m_EntryDatas.clear();
}

void BundleFileParserForCompress::BuildPatchForAddFile(FILE* file_to, EndianBinaryWriter* writer, const char* bundlename)
{
	int patchbegin = writer->GetPosition();
	writer->SetEndianType(LittleEndian);
	writer->WriteStringToNull(bundlename);

	long sizepos = writer->GetPosition();
	writer->WriteInt32(0);
	writer->WriteInt16(DIFFERENCE_ADD);

	int version = 1;
	writer->WriteInt32(version);

	void* buffer = malloc(CHUNK_LENGTH);
	fseek(file_to, 0, SEEK_SET);
	int readed = fread(buffer, 1, CHUNK_LENGTH, file_to);
	while (readed > 0)
	{
		writer->WriteBytes(buffer, readed);
		readed = fread(buffer, 1, CHUNK_LENGTH, file_to);
	}
	int patchsize = writer->GetPosition() - patchbegin;
	writer->WriteBytes(sizepos, &patchsize, 4);
	free(buffer);
}

void BundleFileParserForCompress::BuildPatchForDeleteFile(EndianBinaryWriter* writer, const char* bundlename)
{
	int patchbegin = writer->GetPosition();
	writer->SetEndianType(LittleEndian);
	writer->WriteStringToNull(bundlename);
	int sizepos = writer->GetPosition();
	writer->WriteInt32(0);
	writer->WriteInt16(DIFFERENCE_DELETE);
	int version = 1;
	writer->WriteInt32(version);
	int patchsize = writer->GetPosition() - patchbegin;
	writer->WriteBytes(sizepos, &patchsize, 4);
	push_message("'%s' : %d", bundlename, patchsize);
}

int BundleFileParserForCompress::ComparessAndBuildPatch(FILE* file_from, FILE* file_to, EndianBinaryWriter* writer, const char* bundlename)
{
	int errorcode = 0;
	bool success = false;
	BundleFile* bundle_to = NULL;
	BundleFile* bundle_from = new BundleFile(EndianBinaryReader::Create(file_from));

	BundleFileParserForCompress* parser_to = NULL;
	BundleFileParserForCompress* parser_from = new BundleFileParserForCompress();
	if (bundle_from->Parse(parser_from) == false)
	{
		errorcode = -1;
		goto parseend;
	}

	bundle_to = new BundleFile(EndianBinaryReader::Create(file_to));
	parser_to = new BundleFileParserForCompress();
	if (bundle_to->Parse(parser_to) == false)
	{
		errorcode = -1;
		goto parseend;
	}

	success = BundleFileParserForCompress::SaveToFile(parser_from, parser_to, writer, bundlename);
	errorcode = success ? 1 : 0;
	writer->Flush();

parseend:
	SAFE_DELETE(bundle_from);
	SAFE_DELETE(bundle_to);
	SAFE_DELETE(parser_from);
	SAFE_DELETE(parser_to);
	return errorcode;
}

void BundleFileParserForCompress::UpdatePatch(AssetBundleDiff_CTX* ctx, const char* path_from, const char* path_to, const char* bundlename)
{
	EndianBinaryWriter* writer = (EndianBinaryWriter*)ctx->writer;
	assetbundle_sign(path_from);
	assetbundle_sign(path_to);

	bool compress_success = false;
	FILE* file_from = fopen(path_from, "rb+");
	FILE* file_to = fopen(path_to, "rb+");
	if (file_from == NULL && file_to == NULL)
	{
		return;
	}

	writer->SetEndianType(LittleEndian);
	if (file_from == NULL)
	{
		BuildPatchForAddFile(file_to, writer, bundlename);
		push_message("add bundle '%s', size %d", bundlename, writer->GetPosition() - ctx->bytes_writed);
		writer->Flush();
		ctx->bytes_writed = writer->GetPosition();
		ctx->diff_count++;
		fclose(file_to);
	}
	else if (file_to == NULL)
	{
		BuildPatchForDeleteFile(writer, bundlename);
		writer->Flush();
		push_message("delete bundle '%s', record size %d", bundlename, writer->GetPosition() - ctx->bytes_writed);
		ctx->bytes_writed = writer->GetPosition();
		ctx->diff_count++;
		fclose(file_from);
	}
	else
	{
		switch (ComparessAndBuildPatch(file_from, file_to, writer, bundlename))
		{
		case -1:	//格式不符合预期
		{
			//先二进制比较
			bool samebundle = true;
			fseek(file_from, 0, SEEK_END);
			fseek(file_to, 0, SEEK_END);
			if (ftell(file_from) != ftell(file_to))
			{
				samebundle = false;
			}
			else
			{
				fseek(file_from, 0, SEEK_SET);
				fseek(file_to, 0, SEEK_SET);

				int bufflen = CHUNK_LENGTH;
				char* buff1 = (char*)malloc(bufflen * 2);
				char* buff2 = buff1 + bufflen;
				int read1 = fread(buff1, 1, bufflen, file_from);
				int read2 = fread(buff2, 1, bufflen, file_to);
				while (read1 > 0 && read1 == read2)
				{
					if (memcmp(buff1, buff2, read1) != 0)
					{
						samebundle = false;
						break;
					}
					read1 = fread(buff1, 1, bufflen, file_from);
					read2 = fread(buff2, 1, bufflen, file_to);
				}
				free(buff1);
			}

			if (samebundle == false)
			{
				BuildPatchForAddFile(file_to, writer, bundlename);
				writer->Flush();
				push_message("can't compare '%s', copy binary content, size %d", bundlename, writer->GetPosition() - ctx->bytes_writed);
				ctx->bytes_writed = writer->GetPosition();
				ctx->diff_count++;
			}
			else
			{
				writer->SetPosition(ctx->bytes_writed);
			}
		}
		break;

		case 0:
		{
			writer->SetPosition(ctx->bytes_writed);
		}
		break;

		default:
		{
			writer->Flush();
			ctx->diff_count++;
			push_message("'%s' changed, size %d", bundlename, writer->GetPosition() - ctx->bytes_writed);
			ctx->bytes_writed = writer->GetPosition();
		}
		break;
		}

		fclose(file_from);
		fclose(file_to);
	}
}

void BundleFileParserForCompress::EndParseBundleTable(BundleFile* bundle, EndianBinaryReader* reader, int length)
{
	BundleFileParserForAsign::EndParseBundleTable(bundle, reader, length);
	m_tableUnCompressed = bundle->getUnCompressedSize();
	m_bundleHeader = new EntryItemVoidData();
	m_tableOffset = length - bundle->getCompressedSize();
	m_bundleHeader->data = reader->ReadBytes(length);
	m_bundleHeader->datalen = length;
}

void BundleFileParserForCompress::BeginParserEntrys(int count)
{
	BundleFileParserForAsign::BeginParserEntrys(count);
	m_EntryDatas.resize(count);
}

void BundleFileParserForCompress::EnterEntryItem(int index, EntryItem* item)
{
	BundleFileParserForAsign::EnterEntryItem(index, item);
	m_current = new EntryData();
	m_current->name = item->getName();
	m_EntryDatas[index] = m_current;
}

void BundleFileParserForCompress::BeginParseEntryPreloadInfo(EntryItem* item, int count)
{
	BundleFileParserForAsign::BeginParseEntryPreloadInfo(item, count);
	m_current->pathids.resize(count);
}

void BundleFileParserForCompress::ParseEntryPreloadInfo(EntryItem* item, int index, long long pathID, int position, int size, int addtionlen)
{
	BundleFileParserForAsign::ParseEntryPreloadInfo(item, index, pathID, position, size, addtionlen);
	m_current->pathids[index] = pathID;
}

void BundleFileParserForCompress::EndParseEntryTable(EntryItem* item, int position, int datalen)
{
	BundleFileParserForAsign::EndParseEntryTable(item, position, datalen);
	const void* data = item->GetReader()->RetPointer(position);
	m_current->entryHeader = new EntryItemVoidData(data, datalen);
}

bool BundleFileParserForCompress::BeginParseEntryPreloadData(EntryItem* item, int count)
{
	BundleFileParserForAsign::BeginParseEntryPreloadData(item, count);
	m_current->preloads.resize(count);
	m_current->preloadhashs.resize(count);
	return true;
}

void BundleFileParserForCompress::ParseEntryPreloadData(EntryItem* item, int index, int position, int length)
{
	BundleFileParserForAsign::ParseEntryPreloadData(item, index, position, length);
	PreloadMD5Code* code = new PreloadMD5Code();
	const void* data = item->GetReader()->RetPointer(position);
	md5((const char*)data, length, code->md5code);
	code->index = index;
	m_current->preloadhashs[index] = code;
	m_current->preloads[index] = new EntryItemVoidData(data, length);
}

bool BundleFileParserForCompress::SaveToFile(BundleFileParserForCompress* old_bundle, BundleFileParserForCompress* new_bundle, EndianBinaryWriter* writer, const char* bundlename)
{
	if (old_bundle == NULL || new_bundle == NULL || writer == NULL)
	{
		return false;
	}
	
	bool ret = false;
	int patchbegin = writer->GetPosition();
	writer->WriteStringToNull(bundlename);

	int patchsize = 0;
	long sizepos = writer->GetPosition();
	writer->WriteInt32(patchsize);
	writer->WriteInt16(DIFFERENCE_MODIFY);

	int version = 1;
	writer->WriteInt32(version);
	writer->WriteBytes(old_bundle->Retdigest(), 16);
	writer->WriteBytes(new_bundle->Retdigest(), 16);
	writer->WriteInt32(new_bundle->m_tableOffset);
	writer->WriteInt32(new_bundle->m_tableUnCompressed);
	EntryItemVoidData::CompareAndSaveToFile(NULL, new_bundle->m_bundleHeader, writer);

	std::vector<EntryData*>& new_entrys = new_bundle->m_EntryDatas;
	std::vector<EntryData*>& old_entrys = old_bundle->m_EntryDatas;
	
	//所有涉及到的entry
	int entrycount = new_entrys.size();
	writer->WriteInt32(entrycount);
	for (int i = 0; i < entrycount; i++)
	{
		EntryData* parser_old = NULL;
		EntryData* parser_new = new_entrys[i];
		std::string entryName = parser_new->name;
		for (int j = 0; j < old_entrys.size(); j++)
		{
			if (old_entrys[j]->name == entryName)
			{
				parser_old = old_entrys[j];
				break;
			}
		}

		ret |= SaveToFile(parser_old, parser_new, writer);
	}
	patchsize = writer->GetPosition() - patchbegin;
	writer->WriteBytes(sizepos, &patchsize, 4);
	return ret;
}

bool BundleFileParserForCompress::SaveToFile(EntryData* old_entry, EntryData* new_entry, EndianBinaryWriter* writer)
{
	bool ret = false;
	writer->WriteStringToNull(new_entry->name.c_str());
	if (old_entry == NULL || old_entry->entryHeader->datalen != new_entry->entryHeader->datalen ||
		memcmp(old_entry->entryHeader->data, new_entry->entryHeader->data, new_entry->entryHeader->datalen) != 0)
	{
		ret = true;
	}
	EntryItemVoidData::CompareAndSaveToFile(NULL, new_entry->entryHeader, writer);

	const std::vector<PreloadMD5Code*>* str_new = &new_entry->preloadhashs;
	const std::vector<PreloadMD5Code*>* str_old = old_entry != NULL ? &old_entry->preloadhashs : NULL;
	const std::vector<EntryItemVoidData *>* preloads_new = &new_entry->preloads;
	const std::vector<EntryItemVoidData *>* preloads_old = old_entry != NULL ? &old_entry->preloads : NULL;

	struct PreloadMD5Code* maps[2] = { NULL, NULL };
	const std::vector<PreloadMD5Code*>* vectors[2] = { str_old, str_new };
	const std::vector<EntryItemVoidData*>* preload_array[2] = { preloads_old, preloads_new };
	for (int i = 0; i < 2; i++)
	{
		struct PreloadMD5Code*& hash_map = maps[i];
		const std::vector<PreloadMD5Code*>* vector_ptr = vectors[i];
		if (vector_ptr != NULL)
		{
			const std::vector<EntryItemVoidData *>& preload = *preload_array[i];
			const std::vector<struct PreloadMD5Code*> vector = *vector_ptr;
			for (int j = 0; j < vector_ptr->size(); j++)
			{
				PreloadMD5Code* finded = NULL;
				PreloadMD5Code* code = vector[j];
				HASH_FIND(hh, hash_map, code->md5code, 16, finded);
				if (finded == NULL)
				{
					HASH_ADD(hh, hash_map, md5code, 16, code);
				}
				else
				{
					bool same = EntryItemVoidData::Compare(preload[j], preload[finded->index]);
					assert(same);
				}
			}
		}
	}

	int countPosition = writer->GetPosition();
	std::vector<EntryItemVoidData *>& preloads = new_entry->preloads;
	writer->WriteInt32(preloads.size());
	for (int i = 0; i < preloads.size(); i++)
	{
		int var = -1;
		bool fromold = false;
		PreloadMD5Code* founded = NULL;
		PreloadMD5Code* preload_md5 = new_entry->preloadhashs[i];
		HASH_FIND(hh, maps[0], preload_md5->md5code, 16, founded);
		if (founded != NULL)
		{
			fromold = true;
			var = founded->index;
		}

		if (var < 0)
		{
			HASH_FIND(hh, maps[1], preload_md5->md5code, 16, founded);
			if (founded != NULL)
			{
				fromold = false;
				var = founded->index;
			}
		}
		assert(var >= 0);

		writer->WriteBytes(founded->md5code, 16);
		writer->WriteBoolean(fromold);
		writer->WriteInt32(var);
		writer->WriteInt32(i);
		if (fromold == false && var == i)
		{
			EntryItemVoidData::CompareAndSaveToFile(NULL, preloads[i], writer);
		}
	}
	return ret;
}

BundleFileParserForCompress::BundleFileParserForCompress()
{
	m_tableOffset = 0;
	m_tableUnCompressed = 0;
}

AssetBundleDiff_CTX* assetbundle_diff_init(const char* patch)
{
	AssetBundleDiff_CTX* ctx = NULL;
	if (patch != NULL && *patch != 0)
	{
		FILE* file = fopen(patch, "wb+");
		if (file != NULL)
		{
			push_message("create '%s' success", patch);
			ctx = (AssetBundleDiff_CTX*)calloc(sizeof(AssetBundleDiff_CTX), 1);
			int len = strlen(patch) + 1;
			ctx->path = (const char*)malloc(len);
			memcpy((void*)ctx->path, patch, len);
			ctx->file = file;

			EndianBinaryWriter* writer = EndianBinaryWriter::Create(file);
			writer->WriteStringToNull(PATCH_SIGN);
			ctx->diff_count_pos = writer->GetPosition();
			writer->WriteInt32(0);
			ctx->bytes_writed = writer->GetPosition();
			ctx->writer = writer;
		}
		else
		{
			push_message("create '%s' failed", patch);
		}
	}
	return ctx;
}

void assetbundle_diff_update(AssetBundleDiff_CTX* ctx, const char* path_from, const char* path_to, const char* bundlename)
{
	if (ctx == NULL)
	{
		push_message("assetbundle_diff_init failed.");
		return;
	}
	BundleFileParserForCompress::UpdatePatch(ctx, path_from, path_to, bundlename);
}

int assetbundle_diff_final(AssetBundleDiff_CTX* ctx)
{
	int writed = ctx->diff_count;
	EndianBinaryWriter* writer = (EndianBinaryWriter*)ctx->writer;
	writer->SetPosition(ctx->diff_count_pos);
	writer->WriteInt32(ctx->diff_count);
	SAFE_DELETE(writer);
	if (writed <= 0)
	{
		remove(ctx->path);
	}
	SAFE_FREE(ctx);
	return writed;
}