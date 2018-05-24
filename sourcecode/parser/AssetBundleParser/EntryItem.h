#ifndef _ENTRYITEM_H_
#define _ENTRYITEM_H_

#include <vector>
#include <string>
#include <stdio.h>
#include "EndianBinaryReader.h"

class AssetChunk;
class BundleFileParser;

class EntryItem
{
public:
	EntryItem(EndianBinaryReader* assetsFileReader, std::string name, long long entryinfo_offset, long long entryinfo_size, int flag);
	~EntryItem();

public:
	bool Parse(int index, BundleFileParser* parser);

public:
	std::string getName() { return m_name; }
	int getTableSize() { return m_tableSize; }
	int getDataEnd() { return m_dataEnd; }
	int getFileGen() { return m_fileGen; }
	int getdataOffset() { return m_dataOffset; }
	int getUnknow() { return m_unknow; }
	std::string getVersion() { return m_version; }
	int getPlatform() { return m_platform; }
	bool getBaseDefinitions() { return m_baseDefinitions; }
	int getFlag() { return m_flag; }
	EndianBinaryReader* GetReader() { return m_assetsFileReader; }

protected:
	std::string m_name;
	int m_tableSize;
	int m_dataEnd;
	int m_fileGen;
	int m_dataOffset;
	int m_unknow;
	std::string m_version;
	int m_platform;
	bool m_baseDefinitions;
	int m_flag;

	long long m_entryinfo_offset;
	long long m_entryinfo_size;
	EndianBinaryReader* m_assetsFileReader;
};

#endif