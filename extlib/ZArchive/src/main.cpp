#include "zarchive/zarchivewriter.h"
#include "zarchive/zarchivereader.h"

#include <vector>
#include <fstream>
#include <filesystem>
#include <cassert>
#include <optional>

#include <stdio.h>

namespace fs = std::filesystem;

void PrintHelp()
{
	puts("Usage:\n");
	puts("zarchive.exe input_path [output_path]");
	puts("If input_path is a directory, then output_path will be the ZArchive output file path");
	puts("If input_path is a ZArchive file path, then output_path will be the output directory");
	puts("output_path is optional");
}

bool ExtractFile(ZArchiveReader* reader, std::string_view srcPath, const fs::path& path)
{
	ZArchiveNodeHandle fileHandle = reader->LookUp(srcPath, true, false);
	if (fileHandle == ZARCHIVE_INVALID_NODE)
	{
		puts("Unable to extract file:");
		puts(std::string(srcPath).c_str());
		return false;
	}

	std::vector<uint8_t> buffer;
	buffer.resize(64 * 1024);

	std::ofstream fileOut(path, std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
	if (!fileOut.is_open())
	{
		puts("Unable to write file:");
		puts(path.generic_string().c_str());
	}
	uint64_t readOffset = 0;
	while (true)
	{
		uint64_t bytesRead = reader->ReadFromFile(fileHandle, readOffset, buffer.size(), buffer.data());
		if (bytesRead == 0)
			break;
		fileOut.write((const char*)buffer.data(), bytesRead);
		readOffset += bytesRead;
	}
	if (readOffset != reader->GetFileSize(fileHandle))
		return false;

	return true;
}

bool ExtractRecursive(ZArchiveReader* reader, std::string srcPath, fs::path outputDirectory)
{
	ZArchiveNodeHandle dirHandle = reader->LookUp(srcPath, false, true);
	if (dirHandle == ZARCHIVE_INVALID_NODE)
		return false;
	std::error_code ec;
	fs::create_directories(outputDirectory);
	uint32_t numEntries = reader->GetDirEntryCount(dirHandle);
	for (uint32_t i = 0; i < numEntries; i++)
	{
		ZArchiveReader::DirEntry dirEntry;
		if (!reader->GetDirEntry(dirHandle, i, dirEntry))
		{
			puts("Directory contains invalid node");
			return false;
		}
		puts(std::string(srcPath).append("/").append(dirEntry.name).c_str());
		if (dirEntry.isDirectory)
		{
			ExtractRecursive(reader, std::string(srcPath).append("/").append(dirEntry.name), outputDirectory / dirEntry.name);
		}
		else
		{
			// extract file
			if (!ExtractFile(reader, std::string(srcPath).append("/").append(dirEntry.name), outputDirectory / dirEntry.name))
				return false;
		}
	}
	return true;
}

int Extract(fs::path inputFile, fs::path outputDirectory)
{
	std::error_code ec;
	if (!fs::exists(inputFile, ec))
	{
		puts("Unable to find archive file");
		return -10;
	}

	ZArchiveReader* reader = ZArchiveReader::OpenFromFile(inputFile);
	if (!reader)
	{
		puts("Failed to open ZArchive");
		return -11;
	}
	bool r = ExtractRecursive(reader, "", outputDirectory);
	if (!r)
	{
		puts("Extraction failed");
		delete reader;
		return -12;
	}
	delete reader;
	return 0;
}

struct PackContext
{
	fs::path outputFilePath;
	std::ofstream currentOutputFile;
	bool hasError{false};
};

void _pack_NewOutputFile(const int32_t partIndex, void* ctx)
{
	PackContext* packContext = (PackContext*)ctx;
	packContext->currentOutputFile = std::ofstream(packContext->outputFilePath, std::ios::binary);
	if (!packContext->currentOutputFile.is_open())
	{
		printf("Failed to create output file: %s\n", packContext->outputFilePath.string().c_str());
		packContext->hasError = true;
	}
}

void _pack_WriteOutputData(const void* data, size_t length, void* ctx)
{
	PackContext* packContext = (PackContext*)ctx;
	packContext->currentOutputFile.write((const char*)data, length);
}

int Pack(fs::path inputDirectory, fs::path outputFile)
{
	std::vector<uint8_t> buffer;
	buffer.resize(64 * 1024);

	std::error_code ec;
	PackContext packContext;
	packContext.outputFilePath = outputFile;
	ZArchiveWriter zWriter(_pack_NewOutputFile, _pack_WriteOutputData, &packContext);
	if (packContext.hasError)
		return -16;
	for (auto const& dirEntry : fs::recursive_directory_iterator(inputDirectory))
	{
		fs::path pathEntry = fs::relative(dirEntry.path(), inputDirectory, ec);
		if (dirEntry.is_directory())
		{
			if (!zWriter.MakeDir(pathEntry.generic_string().c_str(), false))
			{
				printf("Failed to create directory %s\n", pathEntry.string().c_str());
				return -13;
			}
		}
		else if (dirEntry.is_regular_file())
		{
			if (dirEntry == outputFile) {
				continue;
			}
			printf("Adding %s\n", pathEntry.string().c_str());
			if (!zWriter.StartNewFile(pathEntry.generic_string().c_str()))
			{
				printf("Failed to create archive file %s\n", pathEntry.string().c_str());
				return -14;
			}
			std::ifstream inputFile(inputDirectory / pathEntry, std::ios::binary);
			if (!inputFile.is_open())
			{
				printf("Failed to open input file %s\n", pathEntry.string().c_str());
				return -15;
			}
			while( true )
			{
				inputFile.read((char*)buffer.data(), buffer.size());
				int32_t readBytes = (int32_t)inputFile.gcount();
				if (readBytes <= 0)
					break;
				zWriter.AppendData(buffer.data(), readBytes);
			}
		}
		if (packContext.hasError)
			return -16;
	}
	zWriter.Finalize();
	return 0;
}

int main(int argc, char* argv[])
{
	if (argc <= 1)
	{
		PrintHelp();
		return 0;
	}
	std::optional<std::string> strInput;
	std::optional<std::string> strOutput;
	for (int i = 1; i < argc; i++)
	{
		if (strInput)
		{
			if (strOutput)
			{
				puts("Too many paths specified");
				return -1;
			}
			else
			{
				strOutput = argv[i];
			}
		}
		else
		{
			strInput = argv[i];
		}
	}

	if (strInput)
	{
		std::error_code ec;
		fs::path p(*strInput);
		if (fs::is_regular_file(p, ec))
		{
			// extract
			fs::path outputDirectory;
			if (!strOutput)
			{
				fs::path defaultOutputPath = p.parent_path() / (p.stem().filename().string().append("_extracted"));
				outputDirectory = defaultOutputPath;
				printf("Extracting to: %s\n", outputDirectory.generic_string().c_str());
			}
			else
				outputDirectory = *strOutput;
			if (fs::exists(outputDirectory, ec) && !fs::is_directory(outputDirectory, ec))
			{
				puts("The specified output path is not a valid directory");
				return -3;
			}
			fs::create_directories(outputDirectory, ec);
			if (!fs::exists(outputDirectory, ec))
			{
				puts("Failed to create output directory");
				return -4;
			}
			return Extract(p, outputDirectory);
		}
		else if(fs::is_directory(p, ec))
		{
			// pack
			fs::path outputFile;
			if (!strOutput)
			{
				fs::path defaultOutputPath = p.parent_path() / (p.stem().filename().string().append(".zar"));
				outputFile = defaultOutputPath;
				printf("Outputting to: %s\n", outputFile.generic_string().c_str());
			}
			else
				outputFile = *strOutput;
			if ((fs::exists(outputFile, ec) && !fs::is_regular_file(outputFile, ec)))
			{
				puts("The specified output path is not a valid file");
				return -10;
			}
			if ((fs::exists(outputFile, ec) && fs::is_regular_file(outputFile, ec)))
			{
				puts("The output file already exists");
				return -11;
			}
			int r = Pack(p, outputFile);
			if (r != 0)
			{
				// delete incomplete output file
				fs::remove(outputFile, ec);
			}
			return r;
		}
		else
		{
			puts("Input path is not a valid file or directory");
			return -1;
		}
	}
	return 0;
}
