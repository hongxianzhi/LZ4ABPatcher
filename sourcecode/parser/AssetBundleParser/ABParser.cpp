#include "uthash.h"
#include "cmdline.h"
#include <experimental/filesystem>
#include "MergeParser.h"
#include "BundleFile.h"
#include "AsignParser.h"
#include "EndianBinaryWriter.h"

namespace fs = std::experimental::filesystem;

struct FileInfoItem
{
	char* filename;
	UT_hash_handle hh;
};

#define CHECKRETURN(CONDITION, RET) if(CONDITION) return RET;

bool checkpath(std::string& path)
{
	if (path.size() <= 0)
	{
		push_message("null path");
		return false;
	}
 	char c = path[path.size() - 1];
	if (c == '\\' || c == '/')
	{
		return false;
	}
	path += '/';
	return true;
}

void printmessage(const char* message)
{
	printf(message);
}

int main(int argc, char* argv[])
{
	cmdline::parser parser;
	parser.add<std::string>("do", 'd', "work type, 'compare or merge'", true, "compare", cmdline::oneof<std::string>("compare", "merge"));
	parser.add<std::string>("old", 'o', "directory for previous AssetBundle", false, "");
	parser.add<std::string>("new", 'n', "directory for latest AssetBundle", false, "");
	parser.add<std::string>("patch", 'p', "full path for patch file", false, "");
	parser.add<std::string>("mergeto", 'm', "directory to hold assetbundle merged", false, "");
	parser.parse_check(argc, argv);

	MESSAGEHANDLE handler = &printmessage;
	set_messagehandler(handler);

	FileInfoItem* files = NULL;
	char* buffer = (char*)malloc(512);
	std::string cmd = parser.get<std::string>("do");
	if (cmd[0] == 'c')
	{
		std::string from = parser.get<std::string>("old");
		CHECKRETURN(checkpath(from) == false, 0);		;
		push_message("from : %s", from.c_str());

		std::string to = parser.get<std::string>("new");
		CHECKRETURN(checkpath(to) == false, 0);
		push_message("to : %s", to.c_str());

		std::string saveto = parser.get<std::string>("patch");
		if (saveto.size() <= 0) return 0;
		push_message("saveto : %s", saveto.c_str());
		AssetBundleDiff_CTX* ctx = assetbundle_diff_init(saveto.c_str());
		if (ctx == NULL)
		{
			return 0;
		}

		push_message("-----------------------------");
		std::string* paths[] = {&from, &to};
		for (int i = 0; i < 2; i++)
		{
			FileInfoItem* item = NULL;
			push_message("parse directory '%s'", paths[i]->c_str());
			for (auto&fe : fs::directory_iterator(paths[i]->c_str()))
			{
				auto fp = fe.path();
				std::string fullpath = fp.string();
				std::string name = fp.filename().string();

				if (BundleFileParserForAsign::AsignBundle(fullpath.c_str()))
				{
					HASH_FIND_STR(files, name.c_str(), item);
					if (item == NULL)
					{
						item = new FileInfoItem();
						int namelen = name.size() + 1;
						item->filename = (char*)malloc(namelen);
						memcpy(item->filename, name.c_str(), namelen);
						HASH_ADD_STR(files, filename, item);
					}
				}
			}
		}
		push_message("parse directorys done, start compare and update patch");

		char name_to[512];
		while (files != NULL)
		{
			FileInfoItem* next = (FileInfoItem*)files->hh.next;
			sprintf(buffer, "%s%s", from.c_str(), files->filename);
			sprintf(name_to, "%s%s", to.c_str(), files->filename);
			assetbundle_diff_update(ctx, buffer, name_to, files->filename);
			SAFE_DELETE(files);
			files = next;
		}
		assetbundle_diff_final(ctx);
	}
	else
	{
		std::string from = parser.get<std::string>("old");
		CHECKRETURN(checkpath(from) == false, 0);
		push_message("bundle source : %s", from.c_str());

		std::string patch = parser.get<std::string>("patch");
		if (patch.size() <= 0) return 0;
		push_message("patch file : %s", patch.c_str());

		std::string saveto = parser.get<std::string>("mergeto");
		CHECKRETURN(checkpath(saveto) == false, 0);
		push_message("saveto : %s", saveto.c_str());

		assetbundle_merge(patch.c_str(), from.c_str(), saveto.c_str(), NULL);
	}
	SAFE_FREE(buffer);
}