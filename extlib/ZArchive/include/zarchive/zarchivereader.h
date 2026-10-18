#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <string_view>
#include <unordered_map>
#include <mutex>

#include <filesystem>
#include <istream>

#include "zarchivecommon.h"

using ZArchiveNodeHandle = uint32_t;

static inline ZArchiveNodeHandle ZARCHIVE_INVALID_NODE = 0xFFFFFFFF;

class ZArchiveReader
{
public:
	struct DirEntry
	{
		std::string_view name;
		bool isFile;
		bool isDirectory;
		uint64_t size; // only valid for directories
	};

	static ZArchiveReader* OpenFromFile(const std::filesystem::path& path);
	static ZArchiveReader* OpenFromStream(std::unique_ptr<std::istream>&& stream);

	~ZArchiveReader();

	ZArchiveNodeHandle LookUp(std::string_view path, bool allowFile = true, bool allowDirectory = true);
	bool IsDirectory(ZArchiveNodeHandle nodeHandle) const;
	bool IsFile(ZArchiveNodeHandle nodeHandle) const;

	// directory operations
	uint32_t GetDirEntryCount(ZArchiveNodeHandle nodeHandle) const;
	bool GetDirEntry(ZArchiveNodeHandle nodeHandle, uint32_t index, DirEntry& dirEntry) const;

	// file operations
	uint64_t GetFileSize(ZArchiveNodeHandle nodeHandle);
	uint64_t ReadFromFile(ZArchiveNodeHandle nodeHandle, uint64_t offset, uint64_t length, void* buffer);

private:
	struct CacheBlock
	{
		uint8_t* data;
		uint64_t blockIndex;
		// linked-list for LRU
		CacheBlock* prev;
		CacheBlock* next;
	};

	std::mutex m_accessMutex;

	std::vector<uint8_t> m_cacheDataBuffer;
	std::vector<CacheBlock> m_cacheBlocks;
	CacheBlock* m_lruChainFirst;
	CacheBlock* m_lruChainLast;
	std::unordered_map<uint64_t, CacheBlock*> m_blockLookup;

	ZArchiveReader(std::unique_ptr<std::istream>&& stream, std::vector<_ZARCHIVE::CompressionOffsetRecord>&& offsetRecords, std::vector<uint8_t>&& nameTable, std::vector<_ZARCHIVE::FileDirectoryEntry>&& fileTree, uint64_t compressedDataOffset, uint64_t compressedDataSize);

	CacheBlock* GetCachedBlock(uint64_t blockIndex);
	CacheBlock* RecycleLRUBlock(uint64_t newBlockIndex);
	void MarkBlockAsMRU(CacheBlock* block);

	void RegisterBlock(CacheBlock* block, uint64_t blockIndex);
	void UnregisterBlock(CacheBlock* block);
	bool LoadBlock(CacheBlock* block);

	static std::string_view GetName(const std::vector<uint8_t>& nameTable, uint32_t nameOffset);

	std::unique_ptr<std::istream> m_stream;
	std::vector<_ZARCHIVE::CompressionOffsetRecord> m_offsetRecords;
	std::vector<uint8_t> m_nameTable;
	std::vector<_ZARCHIVE::FileDirectoryEntry> m_fileTree;
	uint64_t m_compressedDataOffset;
	uint64_t m_compressedDataSize;
	uint64_t m_blockCount;

	std::vector<uint8_t> m_blockDecompressionBuffer;
};