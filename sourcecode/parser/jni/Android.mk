LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_C_INCLUDES := ../AssetBundleParser
LOCAL_MODULE    := AssetBundleParser
LOCAL_CFLAGS    := -DLUA_ANSI
LOCAL_SRC_FILES :=  ../AssetBundleParser/AsignParser.cpp \
					../AssetBundleParser/AssetChunk.cpp \
					../AssetBundleParser/BundleFile.cpp \
					../AssetBundleParser/ComparePaser.cpp \
					../AssetBundleParser/EndianBinaryReader.cpp \
					../AssetBundleParser/EndianBinaryWriter.cpp \
					../AssetBundleParser/EntryItem.cpp \
					../AssetBundleParser/Importer.cpp \
					../AssetBundleParser/lz4.c \
					../AssetBundleParser/lz4hc.c \
					../AssetBundleParser/md5.c \
					../AssetBundleParser/MergeParser.cpp \
					../AssetBundleParser/Parser.cpp
					
include $(BUILD_SHARED_LIBRARY)
