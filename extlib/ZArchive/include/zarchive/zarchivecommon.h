#pragma once

#include <string>
#include <string_view>
#include <cstring>

/* Determine endianness */
/* Original code by https://github.com/rofl0r */
#if (defined __BYTE_ORDER__) && (defined __ORDER_LITTLE_ENDIAN__)
# if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#  define ENDIANNESS_LE 1
#  define ENDIANNESS_BE 0
# elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#  define ENDIANNESS_LE 0
#  define ENDIANNESS_BE 1
# endif
/* Try to derive from arch/compiler-specific macros */
#elif defined(_X86_) || defined(__x86_64__) || defined(__i386__) || \
      defined(__i486__) || defined(__i586__) || defined(__i686__) || \
      defined(__MIPSEL) || defined(_MIPSEL) || defined(MIPSEL) || \
      defined(__ARMEL__) || \
      defined(__MSP430__) || \
      (defined(__LITTLE_ENDIAN__) && __LITTLE_ENDIAN__ == 1) || \
      (defined(_LITTLE_ENDIAN) && _LITTLE_ENDIAN == 1) || \
      defined(_M_ARM) || defined(_M_ARM64) || \
      defined(_M_IX86) || defined(_M_AMD64) /* MSVC */
# define ENDIANNESS_LE 1
# define ENDIANNESS_BE 0
#elif defined(__MIPSEB) || defined(_MIPSEB) || defined(MIPSEB) || \
      defined(__MICROBLAZEEB__) || defined(__ARMEB__) || \
      (defined(__BIG_ENDIAN__) && __BIG_ENDIAN__ == 1) || \
      (defined(_BIG_ENDIAN) && _BIG_ENDIAN == 1)
# define ENDIANNESS_LE 0
# define ENDIANNESS_BE 1
/* Try to get it from a header */
#else
# if defined(__linux) || defined(__HAIKU__)
#  include <endian.h>
# elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || \
       defined(__DragonFly__)
#  include <sys/endian.h>
# elif defined(__APPLE__)
#  include <machine/endian.h>
# endif
#endif

#ifndef ENDIANNESS_LE
# undef ENDIANNESS_BE
# if defined(__BYTE_ORDER) && defined(__LITTLE_ENDIAN)
#  if __BYTE_ORDER == __LITTLE_ENDIAN
#   define ENDIANNESS_LE 1
#   define ENDIANNESS_BE 0
#  elif __BYTE_ORDER == __BIG_ENDIAN
#   define ENDIANNESS_LE 0
#   define ENDIANNESS_BE 1
#  endif
# elif defined(BYTE_ORDER) && defined(LITTLE_ENDIAN)
#  if BYTE_ORDER == LITTLE_ENDIAN
#   define ENDIANNESS_LE 1
#   define ENDIANNESS_BE 0
#  elif BYTE_ORDER == BIG_ENDIAN
#   define ENDIANNESS_LE 0
#   define ENDIANNESS_BE 1
#  endif
# endif
#endif

namespace _ZARCHIVE
{
	inline constexpr size_t COMPRESSED_BLOCK_SIZE = 64 * 1024; // 64KiB
	inline constexpr size_t ENTRIES_PER_OFFSETRECORD = 16; // must be aligned to two

	template<class T, std::size_t... N>
	constexpr T bswap_impl(T i, std::index_sequence<N...>)
	{
		return ((((i >> (N * 8)) & (T)(uint8_t)(-1)) << ((sizeof(T) - 1 - N) * 8)) | ...);
	}

	template<class T, class U = std::make_unsigned_t<T>>
	constexpr T bswap(T i)
	{
		return (T)bswap_impl<U>((U)i, std::make_index_sequence<sizeof(T)>{});
	}

	template<class T>
	inline T _store(const T src)
	{
#if ENDIANNESS_BE != 0
		return src;
#else
		return bswap<T>(src);
#endif
	}

	template<class T>
	inline T _load(const T src)
	{
#if ENDIANNESS_BE != 0
		return src;
#else
		return bswap<T>(src);
#endif
	}

	struct CompressionOffsetRecord
	{
		// for every Nth entry we store the full 64bit offset, the blocks in between calculate the offset from the size array
		uint64_t baseOffset;
		uint16_t size[ENTRIES_PER_OFFSETRECORD]; // compressed size - 1

		static void Serialize(const CompressionOffsetRecord* input, size_t count, CompressionOffsetRecord* output)
		{
			while (count)
			{
				output->baseOffset = _store(input->baseOffset);
				for (size_t i = 0; i < ENTRIES_PER_OFFSETRECORD; i++)
					output->size[i] = _store(input->size[i]);
				input++;
				output++;
				count--;
			}
		}

		static void Deserialize(CompressionOffsetRecord* input, size_t count, CompressionOffsetRecord* output)
		{
			Serialize(input, count, output);
		}
	};

	static_assert(std::is_standard_layout<CompressionOffsetRecord>::value);
	static_assert(sizeof(CompressionOffsetRecord) == (8 + 2 * 16));

	struct FileDirectoryEntry
	{
		uint32_t nameOffsetAndTypeFlag; // MSB is type. 0 -> directory, 1 -> file. Lower 31 bit are the offset into the node name table
		union
		{
			// note: Current serializer/deserializer code assumes both record types have the same data layout (three uint32_t) which allows for skipping a type check
			struct
			{
				uint32_t fileOffsetLow;
				uint32_t fileSizeLow;
				uint32_t fileOffsetAndSizeHigh; // upper 16 bits -> file size extension, lower 16 bits -> file offset extension
			}fileRecord;
			struct
			{
				uint32_t nodeStartIndex;
				uint32_t count;
				uint32_t _reserved;
			}directoryRecord;
		};

		void SetTypeAndNameOffset(bool isFile, uint32_t nameOffset)
		{
			nameOffsetAndTypeFlag = 0;
			if (isFile)
				nameOffsetAndTypeFlag |= 0x80000000;
			else
				nameOffsetAndTypeFlag &= ~0x80000000;
			nameOffsetAndTypeFlag |= (nameOffset & 0x7FFFFFFF);
		}

		uint32_t GetNameOffset() const
		{
			return nameOffsetAndTypeFlag & 0x7FFFFFFF;
		}

		uint64_t GetFileOffset() const
		{
			uint64_t fileOffset = fileRecord.fileOffsetLow;
			fileOffset |= ((uint64_t)(fileRecord.fileOffsetAndSizeHigh & 0xFFFF) << 32);
			return fileOffset;
		}

		uint64_t GetFileSize() const
		{
			uint64_t fileSize = fileRecord.fileSizeLow;
			fileSize |= ((uint64_t)(fileRecord.fileOffsetAndSizeHigh & 0xFFFF0000) << 16);
			return fileSize;
		}

		void SetFileOffset(uint64_t fileOffset)
		{
			fileRecord.fileOffsetLow = (uint32_t)fileOffset;
			fileRecord.fileOffsetAndSizeHigh &= 0xFFFF0000;
			fileRecord.fileOffsetAndSizeHigh |= ((uint32_t)(fileOffset >> 32) & 0xFFFF);
		}

		void SetFileSize(uint64_t fileSize)
		{
			fileRecord.fileSizeLow = (uint32_t)fileSize;
			fileRecord.fileOffsetAndSizeHigh &= 0x0000FFFF;
			fileRecord.fileOffsetAndSizeHigh |= ((uint32_t)(fileSize >> 16) & 0xFFFF0000);
		}

		bool IsFile() const
		{
			return (nameOffsetAndTypeFlag & 0x80000000) != 0;
		}

		static void Serialize(const FileDirectoryEntry* input, size_t count, FileDirectoryEntry* output)
		{
			// Optimized method where we exploit the fact that fileRecord and dirRecord have the same layout:
			while (count)
			{
				output->nameOffsetAndTypeFlag = _store(input->nameOffsetAndTypeFlag);
				output->fileRecord.fileOffsetLow = _store(input->fileRecord.fileOffsetLow);
				output->fileRecord.fileSizeLow = _store(input->fileRecord.fileSizeLow);
				output->fileRecord.fileOffsetAndSizeHigh = _store(input->fileRecord.fileOffsetAndSizeHigh);
				input++;
				output++;
				count--;
			}

			/* Generic implementation:
			while (count)
			{
				if (input->IsFile())
				{
					output->fileRecord.fileOffsetLow = _store(input->fileRecord.fileOffsetLow);
					output->fileRecord.fileSizeLow = _store(input->fileRecord.fileSizeLow);
					output->fileRecord.fileOffsetAndSizeHigh = _store(input->fileRecord.fileOffsetAndSizeHigh);
				}
				else
				{
					output->directoryRecord.nodeStartIndex = _store(input->directoryRecord.nodeStartIndex);
					output->directoryRecord.count = _store(input->directoryRecord.count);
					output->directoryRecord._reserved = _store(input->directoryRecord._reserved);
				}
				output->nameOffsetAndTypeFlag = _store(input->nameOffsetAndTypeFlag);
				input++;
				output++;
				count--;
			}
			*/
		}

		static void Deserialize(FileDirectoryEntry* input, size_t count, FileDirectoryEntry* output)
		{
			Serialize(input, count, output);
		}
	};

	static_assert(std::is_standard_layout<FileDirectoryEntry>::value);
	static_assert(sizeof(FileDirectoryEntry) == 16);

	struct Footer
	{
		static inline uint32_t kMagic = 0x169f52d6;
		static inline uint32_t kVersion1 = 0x61bf3a01; // also acts as an extended magic

		struct OffsetInfo
		{
			uint64_t offset;
			uint64_t size;

			bool IsWithinValidRange(uint64_t fileSize) const
			{
				return (offset + size) <= fileSize;
			}
		};

		OffsetInfo sectionCompressedData;
		OffsetInfo sectionOffsetRecords;
		OffsetInfo sectionNames;
		OffsetInfo sectionFileTree;
		OffsetInfo sectionMetaDirectory;
		OffsetInfo sectionMetaData;
		uint8_t integrityHash[32];
		uint64_t totalSize;
		uint32_t version;
		uint32_t magic;

		static void Serialize(const Footer* input, Footer* output)
		{
			output->magic = _store(input->magic);
			output->version = _store(input->version);
			output->totalSize = _store(input->totalSize);
			output->sectionCompressedData.offset = _store(input->sectionCompressedData.offset);
			output->sectionCompressedData.size = _store(input->sectionCompressedData.size);
			output->sectionOffsetRecords.offset = _store(input->sectionOffsetRecords.offset);
			output->sectionOffsetRecords.size = _store(input->sectionOffsetRecords.size);
			output->sectionNames.offset = _store(input->sectionNames.offset);
			output->sectionNames.size = _store(input->sectionNames.size);
			output->sectionFileTree.offset = _store(input->sectionFileTree.offset);
			output->sectionFileTree.size = _store(input->sectionFileTree.size);
			output->sectionMetaDirectory.offset = _store(input->sectionMetaDirectory.offset);
			output->sectionMetaDirectory.size = _store(input->sectionMetaDirectory.size);
			output->sectionMetaData.offset = _store(input->sectionMetaData.offset);
			output->sectionMetaData.size = _store(input->sectionMetaData.size);
			memcpy(output->integrityHash, input->integrityHash, 32);
		}

		static void Deserialize(Footer* input, Footer* output)
		{
			Serialize(input, output);
		}
	};

	static_assert(sizeof(Footer) == (16 * 6 + 32 + 8 + 4 + 4));

	static bool GetNextPathNode(std::string_view& pathParser, std::string_view& node)
	{
		// skip leading slashes
		while (!pathParser.empty() && (pathParser.front() == '/' || pathParser.front() == '\\'))
			pathParser.remove_prefix(1);
		if (pathParser.empty())
			return false;
		// the next slash is the delimiter
		size_t index = 0;
		for (index = 0; index < pathParser.size(); index++)
		{
			if (pathParser[index] == '/' || pathParser[index] == '\\')
				break;
		}
		node = std::basic_string_view<char>(pathParser.data(), index);
		pathParser.remove_prefix(index);
		return true;
	}

	static void SplitFilenameFromPath(std::string_view& pathInOut, std::string_view& filename)
	{
		if (pathInOut.empty())
		{
			filename = pathInOut;
			return;
		}
		// scan backwards until the first slash, this is where the filename starts
		// if there is no slash then stop at index zero
		size_t index = pathInOut.size() - 1;
		while (true)
		{
			if (pathInOut[index] == '/' || pathInOut[index] == '\\')
			{
				index++; // slash isn't part of the filename
				break;
			}
			if (index == 0)
				break;
			index--;
		}
		filename = std::basic_string_view<char>(pathInOut.data() + index, pathInOut.size() - index);
		pathInOut.remove_suffix(pathInOut.size() - index);
	}

	static bool CompareNodeNameBool(std::string_view n1, std::string_view n2)
	{
		if (n1.size() != n2.size())
			return false;
		for (size_t i = 0; i < n1.size(); i++)
		{
			char c1 = n1[i];
			char c2 = n2[i];
			if (c1 >= 'A' && c1 <= 'Z')
				c1 -= ('A' - 'a');
			if (c2 >= 'A' && c2 <= 'Z')
				c2 -= ('A' - 'a');
			if (c1 != c2)
				return false;
		}
		return true;
	}

	static int CompareNodeName(std::string_view n1, std::string_view n2)
	{
		for (size_t i = 0; i < std::min(n1.size(), n2.size()); i++)
		{
			char c1 = n1[i];
			char c2 = n2[i];
			if (c1 >= 'A' && c1 <= 'Z')
				c1 -= ('A' - 'a');
			if (c2 >= 'A' && c2 <= 'Z')
				c2 -= ('A' - 'a');
			if (c1 != c2)
				return (int)(uint8_t)c2 - (int)(uint8_t)c1;
		}
		if (n1.size() < n2.size())
			return 1;
		if (n1.size() > n2.size())
			return -1;
		return 0;
	}

};

