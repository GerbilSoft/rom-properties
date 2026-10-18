## Overview
ZArchive is yet another file archive format. Think of zip, tar, 7z, etc. but with the requirement of allowing random-access reads and supporting compression.

## Features / Specifications
- Supports random-access reads within stored files
- Uses zstd compression (64KiB blocks)
- Scales reasonably well up to multiple terabytes with millions of files
- The theoretical size limit per-file is 2^48-1 (256 Terabyte)
- The encoding for paths within the archive is Windows-1252 (case-insensitive)
- Contains a SHA256 hash of the whole archive for integrity checks
- Endian-independent. The format always uses big-endian internally
- Stateless file and directory iterator handles which don't require memory allocation

## Example - Basic read file

```c++
#include "zarchive/zarchivereader.h"

int main()
{
	ZArchiveReader* reader = ZArchiveReader::OpenFromFile("archive.zar");
	if (!reader)
	 	return -1;
	ZArchiveNodeHandle fileHandle = reader->LookUp("myfolder/example.bin");
	if (reader->IsFile(fileHandle))
	{
		uint8_t buffer[1000];
		uint64_t n = reader->ReadFromFile(fileHandle, 0, 1000, buffer);
		// buffer now contains the first n (up to 1000) bytes of example.bin
	}
	delete reader;
	return 0;
}
```

For a more detailed example see [main.cpp](/src/main.cpp)

## Limitations
- Not designed for adding, removing or modifying files after the archive has been created

## No-seek creation
When creating new archives only byte append operations are used. No file seeking is necessary. This makes it possible to create archives on storage which is write-once. It also simplifies streaming ZArchive creation over network.

## UTF8 paths
UTF8 for file and folder paths is theoretically supported as paths are just binary blobs. But the case-insensitive comparison only applies to latin letters (a-z).

## Wii U specifics
Originally this format was created to store Wii U games dumps. These use the file extension .wua (Wii U Archive) but are otherwise regular ZArchive files. To allow multiple Wii U titles to be stored inside a single archive, each title must be placed in a subfolder following the naming scheme: 16-digit titleId followed by \_v and then the version as decimal. For example: 0005000e10102000_v32

## License
The ZArchive library is licensed under [MIT No Attribution](https://github.com/Exzap/ZArchive/blob/master/LICENSE), with the exception of [sha_256.c](/src/sha_256.c) and [sha_256.h](/src/sha_256.h) which are public domain, see: [ https://github.com/amosnier/sha-2]( https://github.com/amosnier/sha-2).
