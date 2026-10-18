#include "zarchive/zarchivereader.h"
#include "zarchive/zarchivecommon.h"

#include <fstream>

#include <zstd.h>
#include <cassert>

static uint64_t _istream_getFileSize(std::istream& stream)
{
	stream.seekg(0, std::ios_base::end);
	return (uint64_t)stream.tellg();
}

static bool _istream_readBytes(std::istream& stream, uint64_t offset, void* buffer, uint32_t size)
{
	stream.seekg(offset, std::ios_base::beg);
	stream.read((char*)buffer, size);
	return stream.gcount() == size;
}

static uint64_t _getValidElementCount(uint64_t size, uint64_t elementSize)
{
	if ((size % elementSize) != 0)
		return 0;
	return size / elementSize;
}

ZArchiveReader* ZArchiveReader::OpenFromFile(const std::filesystem::path& path)
{
	auto file = std::make_unique<std::ifstream>(path, std::ios_base::in | std::ios_base::binary);
	if (!file->is_open())
		return nullptr;

	return ZArchiveReader::OpenFromStream(std::move(file));
}

ZArchiveReader* ZArchiveReader::OpenFromStream(std::unique_ptr<std::istream>&& stream)
{
	if(stream == nullptr)
		return nullptr;

	uint64_t fileSize = _istream_getFileSize(*stream);
	if (fileSize <= sizeof(_ZARCHIVE::Footer))
		return nullptr;
	// read footer
	_ZARCHIVE::Footer footer;
	if (!_istream_readBytes(*stream, fileSize - sizeof(_ZARCHIVE::Footer), &footer, sizeof(_ZARCHIVE::Footer)))
		return nullptr;
	_ZARCHIVE::Footer::Deserialize(&footer, &footer);
	// validate footer
	if (footer.magic != _ZARCHIVE::Footer::kMagic)
		return nullptr;
	if (footer.version != _ZARCHIVE::Footer::kVersion1)
		return nullptr;
	if (footer.totalSize != fileSize)
		return nullptr;
	if (!footer.sectionCompressedData.IsWithinValidRange(fileSize) ||
		!footer.sectionOffsetRecords.IsWithinValidRange(fileSize) ||
		!footer.sectionNames.IsWithinValidRange(fileSize) ||
		!footer.sectionFileTree.IsWithinValidRange(fileSize) ||
		!footer.sectionMetaDirectory.IsWithinValidRange(fileSize) ||
		!footer.sectionMetaData.IsWithinValidRange(fileSize))
		return nullptr;
	if (footer.sectionOffsetRecords.size > (uint64_t)0xFFFFFFFF)
		return nullptr;
	if (footer.sectionNames.size > (uint64_t)0x7FFFFFFF)
		return nullptr;
	if (footer.sectionFileTree.size > (uint64_t)0xFFFFFFFF)
		return nullptr;
	// read offset records
	std::vector<_ZARCHIVE::CompressionOffsetRecord> offsetRecords;
	offsetRecords.resize(_getValidElementCount(footer.sectionOffsetRecords.size, sizeof(_ZARCHIVE::CompressionOffsetRecord)));
	if (offsetRecords.empty() || !_istream_readBytes(*stream, footer.sectionOffsetRecords.offset, offsetRecords.data(), (uint32_t)(offsetRecords.size() * sizeof(_ZARCHIVE::CompressionOffsetRecord))))
		return nullptr;
	_ZARCHIVE::CompressionOffsetRecord::Deserialize(offsetRecords.data(), offsetRecords.size(), offsetRecords.data());
	// read name table
	std::vector<uint8_t> nameTable;
	nameTable.resize(footer.sectionNames.size);
	if (!_istream_readBytes(*stream, footer.sectionNames.offset, nameTable.data(), (uint32_t)(nameTable.size() * sizeof(uint8_t))))
		return nullptr;
	// read file tree
	std::vector<_ZARCHIVE::FileDirectoryEntry> fileTree;
	fileTree.resize(_getValidElementCount(footer.sectionFileTree.size, sizeof(_ZARCHIVE::FileDirectoryEntry)));
	if (fileTree.empty() || !_istream_readBytes(*stream, footer.sectionFileTree.offset, fileTree.data(), (uint32_t)(fileTree.size() * sizeof(_ZARCHIVE::FileDirectoryEntry))))
		return nullptr;
	_ZARCHIVE::FileDirectoryEntry::Deserialize(fileTree.data(), fileTree.size(), fileTree.data());
	// verify file tree
	if (fileTree[0].IsFile())
		return nullptr; // first entry must be root directory
	auto rootName = GetName(nameTable, fileTree[0].GetNameOffset());
	if (!rootName.empty())
		return nullptr; // root node must not have a name
	// read meta data
	// todo

	ZArchiveReader* cfs = new ZArchiveReader(std::move(stream), std::move(offsetRecords), std::move(nameTable), std::move(fileTree), footer.sectionCompressedData.offset, footer.sectionCompressedData.size);
	return cfs;
}

ZArchiveReader::ZArchiveReader(std::unique_ptr<std::istream>&& stream, std::vector<_ZARCHIVE::CompressionOffsetRecord>&& offsetRecords, std::vector<uint8_t>&& nameTable, std::vector<_ZARCHIVE::FileDirectoryEntry>&& fileTree, uint64_t compressedDataOffset, uint64_t compressedDataSize) :
	m_stream(std::move(stream)), m_offsetRecords(std::move(offsetRecords)), m_nameTable(std::move(nameTable)), m_fileTree(std::move(fileTree)),
	m_compressedDataOffset(compressedDataOffset), m_compressedDataSize(compressedDataSize)
{
	m_blockCount = (uint64_t)m_offsetRecords.size() * _ZARCHIVE::ENTRIES_PER_OFFSETRECORD;
	m_blockDecompressionBuffer.resize(_ZARCHIVE::COMPRESSED_BLOCK_SIZE);
	// init cache
	uint64_t cacheSize = 1024 * 1024 * 4; // 4MiB
	if ((cacheSize % _ZARCHIVE::COMPRESSED_BLOCK_SIZE) != 0)
		cacheSize += (_ZARCHIVE::COMPRESSED_BLOCK_SIZE - (cacheSize % _ZARCHIVE::COMPRESSED_BLOCK_SIZE));
	m_cacheDataBuffer.resize(cacheSize);
	// create cache blocks and init LRU chain
	m_cacheBlocks.resize(cacheSize / _ZARCHIVE::COMPRESSED_BLOCK_SIZE);
	m_lruChainFirst = m_cacheBlocks.data() + 0;
	m_lruChainLast = m_cacheBlocks.data() + m_cacheBlocks.size() - 1;
	CacheBlock* prevBlock = nullptr;
	for (size_t i = 0; i < m_cacheBlocks.size(); i++)
	{
		m_cacheBlocks[i].blockIndex = 0xFFFFFFFFFFFFFFFF;
		m_cacheBlocks[i].data = m_cacheDataBuffer.data() + i * _ZARCHIVE::COMPRESSED_BLOCK_SIZE;
		m_cacheBlocks[i].prev = prevBlock;
		m_cacheBlocks[i].next = m_cacheBlocks.data() + i + 1;
		prevBlock = m_cacheBlocks.data() + i;
	}
	m_cacheBlocks.back().next = nullptr;
}

ZArchiveReader::~ZArchiveReader()
{

}

ZArchiveNodeHandle ZArchiveReader::LookUp(std::string_view path, bool allowFile, bool allowDirectory)
{
	std::string_view pathParser = path;
	uint32_t currentNode = 0;
	while (true)
	{
		std::string_view pathNodeName;
		if (!_ZARCHIVE::GetNextPathNode(pathParser, pathNodeName))
			return (ZArchiveNodeHandle)currentNode; // end of path reached
		_ZARCHIVE::FileDirectoryEntry& entry = m_fileTree.at(currentNode);
		if (entry.IsFile())
			return ZARCHIVE_INVALID_NODE; // trying to iterate a file
		// linear scan
		// todo - we could accelerate this if we use binary search
		uint32_t currentIndex = entry.directoryRecord.nodeStartIndex;
		uint32_t endIndex = entry.directoryRecord.nodeStartIndex + entry.directoryRecord.count;
		_ZARCHIVE::FileDirectoryEntry* match = nullptr;
		while (currentIndex < endIndex)
		{
			_ZARCHIVE::FileDirectoryEntry& it = m_fileTree.at(currentIndex);
			std::string_view itName = GetName(m_nameTable, it.GetNameOffset());
			if (_ZARCHIVE::CompareNodeNameBool(pathNodeName, itName))
			{
				match = &it;
				break;
			}
			currentIndex++;
			continue;
		}
		if (!match)
			return ZARCHIVE_INVALID_NODE; // path not found
		currentNode = (uint32_t)(match - m_fileTree.data());
	}
	return ZARCHIVE_INVALID_NODE;
}

bool ZArchiveReader::IsDirectory(ZArchiveNodeHandle nodeHandle) const
{
	if (nodeHandle >= m_fileTree.size())
		return false;
	return !m_fileTree[nodeHandle].IsFile();
}

bool ZArchiveReader::IsFile(ZArchiveNodeHandle nodeHandle) const
{
	if (nodeHandle >= m_fileTree.size())
		return false;
	return m_fileTree[nodeHandle].IsFile();
}

uint32_t ZArchiveReader::GetDirEntryCount(ZArchiveNodeHandle nodeHandle) const
{
	if (nodeHandle >= m_fileTree.size())
		return 0;
	auto& entry = m_fileTree.at(nodeHandle);
	if (entry.IsFile())
		return 0;
	return entry.directoryRecord.count;
}

bool ZArchiveReader::GetDirEntry(ZArchiveNodeHandle nodeHandle, uint32_t index, DirEntry& dirEntry) const
{
	if (nodeHandle >= m_fileTree.size())
		return false;
	auto& dir = m_fileTree.at(nodeHandle);
	if (dir.IsFile())
		return false;
	if (index >= dir.directoryRecord.count)
		return false;
	auto& it = m_fileTree.at(dir.directoryRecord.nodeStartIndex + index);
	dirEntry.isFile = it.IsFile();
	dirEntry.isDirectory = !dirEntry.isFile;
	if (dirEntry.isFile)
		dirEntry.size = it.GetFileSize();
	else
		dirEntry.size = 0;
	dirEntry.name = GetName(m_nameTable, it.GetNameOffset());
	if (dirEntry.name.empty())
		return false; // bad name
	return true;
}

uint64_t ZArchiveReader::GetFileSize(ZArchiveNodeHandle nodeHandle)
{
	if (nodeHandle >= m_fileTree.size())
		return 0;
	auto& file = m_fileTree.at(nodeHandle);
	if (!file.IsFile())
		return 0;
	return file.GetFileSize();
}

uint64_t ZArchiveReader::ReadFromFile(ZArchiveNodeHandle nodeHandle, uint64_t offset, uint64_t length, void* buffer)
{
	if (nodeHandle >= m_fileTree.size())
		return 0;
	std::unique_lock<std::mutex> _lock(m_accessMutex);
	auto& file = m_fileTree.at(nodeHandle);
	if (!file.IsFile())
		return 0;
	uint64_t fileOffset = file.GetFileOffset();
	uint64_t fileSize = file.GetFileSize();
	if (offset >= fileSize)
		return 0;
	uint64_t bytesToRead = std::min(length, (fileSize - offset));

	uint64_t rawReadOffset = fileOffset + offset;
	uint64_t remainingBytes = bytesToRead;
	uint8_t* bufferU8 = (uint8_t*)buffer;
	while (remainingBytes > 0)
	{
		uint64_t blockIdx = rawReadOffset / _ZARCHIVE::COMPRESSED_BLOCK_SIZE;
		uint32_t blockOffset = (uint32_t)(rawReadOffset % _ZARCHIVE::COMPRESSED_BLOCK_SIZE);
		uint32_t stepSize = std::min<uint32_t>(remainingBytes, _ZARCHIVE::COMPRESSED_BLOCK_SIZE - blockOffset);
		CacheBlock* block = GetCachedBlock(blockIdx);
		if (!block)
			return 0;
		std::memcpy(bufferU8, block->data + blockOffset, stepSize);
		rawReadOffset += stepSize;
		remainingBytes -= stepSize;
		bufferU8 += stepSize;
	}
	return bytesToRead;
}

ZArchiveReader::CacheBlock* ZArchiveReader::GetCachedBlock(uint64_t blockIndex)
{
	auto it = m_blockLookup.find(blockIndex);
	if (it != m_blockLookup.end())
	{
		MarkBlockAsMRU(it->second);
		return it->second;
	}
	if (blockIndex >= m_blockCount)
		return nullptr;
	// not in cache
	CacheBlock* newBlock = RecycleLRUBlock(blockIndex);
	if (!LoadBlock(newBlock))
	{
		UnregisterBlock(newBlock);
		return nullptr;
	}
	return newBlock;
}

ZArchiveReader::CacheBlock* ZArchiveReader::RecycleLRUBlock(uint64_t newBlockIndex)
{
	CacheBlock* recycledBlock = m_lruChainFirst;
	UnregisterBlock(recycledBlock);
	RegisterBlock(recycledBlock, newBlockIndex);
	MarkBlockAsMRU(recycledBlock);
	return recycledBlock;
}

void ZArchiveReader::MarkBlockAsMRU(ZArchiveReader::CacheBlock* block)
{
	if (!block->next)
		return; // already at the end of the list (MRU)
	// remove from linked list
	if (!block->prev)
	{
		m_lruChainFirst = block->next;
		block->next->prev = nullptr;
	}
	else if (!block->next)
	{
		m_lruChainLast->next = block;
		m_lruChainLast = block;
	}
	else
	{
		block->prev->next = block->next;
		block->next->prev = block->prev;
	}
	// attach at the end
	block->prev = m_lruChainLast;
	block->next = nullptr;
	m_lruChainLast->next = block;
	m_lruChainLast = block;
}

void ZArchiveReader::RegisterBlock(CacheBlock* block, uint64_t blockIndex)
{
	block->blockIndex = blockIndex;
	m_blockLookup.emplace(blockIndex, block);
}

void ZArchiveReader::UnregisterBlock(CacheBlock* block)
{
	if (block->blockIndex != 0xFFFFFFFFFFFFFFFF)
		m_blockLookup.erase(block->blockIndex);
	block->blockIndex = 0xFFFFFFFFFFFFFFFF;
}

bool ZArchiveReader::LoadBlock(CacheBlock* block)
{
	uint32_t recordIndex = (uint32_t)(block->blockIndex / _ZARCHIVE::ENTRIES_PER_OFFSETRECORD);
	uint32_t recordSubIndex = (uint32_t)(block->blockIndex % _ZARCHIVE::ENTRIES_PER_OFFSETRECORD);
	if (recordIndex >= m_offsetRecords.size())
		return false;
	// determine offset and size of compressed block
	auto& record = m_offsetRecords[recordIndex];
	uint64_t offset = record.baseOffset;
	for (uint32_t i = 0; i < recordSubIndex; i++)
	{
		offset += (uint64_t)record.size[i];
		offset++;
	}
	uint32_t compressedSize = (uint32_t)record.size[recordSubIndex] + 1;
	// load file data
	if ((offset + compressedSize) > m_compressedDataSize)
		return false;
	offset += m_compressedDataOffset;
	if (compressedSize == _ZARCHIVE::COMPRESSED_BLOCK_SIZE)
	{
		// uncompressed block, read directly into cached block
		return _istream_readBytes(*m_stream, offset, block->data, compressedSize);
	}
	if (!_istream_readBytes(*m_stream, offset, m_blockDecompressionBuffer.data(), compressedSize))
		return false;
	// decompress
	size_t outputSize = ZSTD_decompress(block->data, _ZARCHIVE::COMPRESSED_BLOCK_SIZE, m_blockDecompressionBuffer.data(), compressedSize);
	return outputSize == _ZARCHIVE::COMPRESSED_BLOCK_SIZE;
}

// returns empty view on failure
std::string_view ZArchiveReader::GetName(const std::vector<uint8_t>& nameTable, uint32_t nameOffset)
{
	if (nameOffset == 0x7FFFFFFF || nameOffset > nameTable.size())
		return "";
	// parse header
	uint16_t nameLength = nameTable[nameOffset] & 0x7F;
	if (nameTable[nameOffset] & 0x80)
	{
		// extended 2-byte length
		if (nameOffset + 1 >= nameTable.size())
			return "";
		nameLength |= ((uint16_t)nameTable[nameOffset] << 7);
		nameOffset += 2;
	}
	else
		nameOffset++;
	// nameOffset can never exceed 0x7FFFFFFF so we don't have to worry about an overflow here
	if ((nameOffset + (uint32_t)nameLength) > nameTable.size())
		return "";
	return std::basic_string_view<char>((char*)nameTable.data() + nameOffset, nameLength);
}