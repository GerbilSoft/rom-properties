/***************************************************************************
 * ROM Properties Page shell extension. (libromdata/tests)                 *
 * RomHeaderTest.cpp: ROM header test                                      *
 *                                                                         *
 * Parses various sample ROM headers and compares them to reference        *
 * text and JSON files.                                                    *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Google Test
#include "gtest/gtest.h"
#include "tcharx.h"

// For .tar.zst
#include "microtar_zstd.h"

// Other rom-properties libraries
#include "libromdata/data/AmiiboData.hpp"
#include "libromdata/RomDataFactory.hpp"
#include "librpbase/RomData.hpp"
#include "librpbase/TextOut.hpp"
#include "librpfile/FileSystem.hpp"
#include "librpfile/MemFile.hpp"
using namespace LibRpBase;
using namespace LibRpFile;

// C includes (C++ namespace)
#include "ctypex.h"

// C++ includes
#include <array>
#include <forward_list>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
using std::array;
using std::forward_list;
using std::ostringstream;
using std::shared_ptr;
using std::string;

// Uninitialized vector class
#include "uvector.h"

namespace LibRomData { namespace Tests {

struct tar_files_t {
	mtar_t bin_tar;		// .bin.tar file
	mtar_t txt_tar;		// .txt.tar file
	mtar_t json_tar;	// .json.tar file

	// Current files loaded from each .tar file.
	mtar_header_t bin_file_header;
	mtar_header_t txt_file_header;
	mtar_header_t json_file_header;
};

struct RomHeaderTest_mode {
	// TODO: Use a string table offset instead of std::string to save memory?
	std::string bin_filename;		// ROM header filename in the .bin.tar file
	const tar_files_t *const p_tar_files;	// .tar files

	RomHeaderTest_mode(const char *bin_filename, const tar_files_t *p_tar_files)
		: bin_filename(bin_filename)
		, p_tar_files(p_tar_files)
	{ }
};

/**
 * Formatting function for RomHeaderTest_mode.
 */
inline ::std::ostream& operator<<(::std::ostream& os, const RomHeaderTest_mode& mode) {
        return os << mode.bin_filename;
};

// Maximum file size for files within the .tar archives.
static constexpr uint64_t MAX_BIN_FILESIZE  =  4U*1024U*1024U;	// 4 MB (for MD lock-on)
static constexpr uint64_t MAX_TXT_FILESIZE  = 32U*1024U;	// 32 KB
static constexpr uint64_t MAX_JSON_FILESIZE = 32U*1024U;	// 32 KB

class RomHeaderTest : public ::testing::TestWithParam<RomHeaderTest_mode>
{
	protected:
		RomHeaderTest()
			: ::testing::TestWithParam<RomHeaderTest_mode>()
		{}

	public:
		// Opened .tar files.
		// These are opened by ReadTestCasesFromDisk().
		static forward_list<tar_files_t> all_tar_files;

	protected:
		// Last read file.
		// NOTE: Not storing the source .tar filename.
		// There shouldn't be any conflicts, though...
		static string last_bin_filename;
		static rp::uvector<uint8_t> last_bin_data;
		static rp::uvector<uint8_t> last_txt_data;
		static rp::uvector<uint8_t> last_json_data;

		/**
		 * Read the next set of files from the .tar files.
		 * @param RomHeaderTest_mode
		 * @return 0 on success; non-zero on error.
		 */
		int read_next_files(const RomHeaderTest_mode &mode);

	public:
		/** Test case parameters **/

		/**
		 * Open the .tar files and create the test case list.
		 * @param bin_tar_filename .tar file contianing the header files
		 * @param txt_tar_filename .tar file containing the text files
		 * @param json_tar_filename .tar file containing the JSON files
		 * @return Header filenames
		 */
		static forward_list<RomHeaderTest_mode> ReadTestCasesFromDisk(const char *bin_tar_filename, const char *txt_tar_filename, const char *json_tar_filename);

		/**
		 * Test case suffix generator.
		 * @param info Test parameter information.
		 * @return Test case suffix.
		 */
		static string test_case_suffix_generator(const ::testing::TestParamInfo<RomHeaderTest_mode> &info);
};

// Opened .tar files.
// These are opened by ReadTestCasesFromDisk().
forward_list<tar_files_t> RomHeaderTest::all_tar_files;

// Last read files.
// NOTE: Not storing the source .tar filename.
// There shouldn't be any conflicts, though...
string RomHeaderTest::last_bin_filename;
rp::uvector<uint8_t> RomHeaderTest::last_bin_data;
rp::uvector<uint8_t> RomHeaderTest::last_txt_data;
rp::uvector<uint8_t> RomHeaderTest::last_json_data;

/**
 * Read the next set of files from the .tar files.
 * @param RomHeaderTest_mode
 * @return 0 on success; non-zero on error.
 */
int RomHeaderTest::read_next_files(const RomHeaderTest_mode &mode)
{
	// TODO: Only Text *xor* JSON is needed, since we're only processing
	// one set at a time. (...usually)
	int ret1, ret2, ret3;

	mtar_header_t *const bin_file_header = const_cast<mtar_header_t*>(&mode.p_tar_files->bin_file_header);
	mtar_header_t *const txt_file_header = const_cast<mtar_header_t*>(&mode.p_tar_files->txt_file_header);
	mtar_header_t *const json_file_header = const_cast<mtar_header_t*>(&mode.p_tar_files->json_file_header);

	do {
		// Read the next files from the .tar files.
		// NOTE: Errors may cause the .tar files to get out of sync.
		ret1 = mtar_read_header(const_cast<mtar_t*>(&mode.p_tar_files->bin_tar), bin_file_header);
		ret2 = mtar_read_header(const_cast<mtar_t*>(&mode.p_tar_files->txt_tar), txt_file_header);
		ret3 = mtar_read_header(const_cast<mtar_t*>(&mode.p_tar_files->json_tar), json_file_header);

		if (ret1 != MTAR_ESUCCESS) {
			return ret1;
		}
		if (ret2 != MTAR_ESUCCESS) {
			return ret2;
		}
		if (ret3 != MTAR_ESUCCESS) {
			return ret3;
		}

		// Make sure all three files have the same type.
		if (bin_file_header->type != txt_file_header->type ||
		    bin_file_header->type != json_file_header->type)
		{
			return -EIO;
		}

		// If this is a regular file, process it.
		if (bin_file_header->type == 0 /*MTAR_TREG*/)
			break;

		// If this isn't a regular file, go to the next file.
		ret1 = mtar_next(const_cast<mtar_t*>(&mode.p_tar_files->bin_tar));
		ret2 = mtar_next(const_cast<mtar_t*>(&mode.p_tar_files->txt_tar));
		ret3 = mtar_next(const_cast<mtar_t*>(&mode.p_tar_files->json_tar));

		if (ret1 != MTAR_ESUCCESS) {
			return ret1;
		}
		if (ret2 != MTAR_ESUCCESS) {
			return ret2;
		}
		if (ret3 != MTAR_ESUCCESS) {
			return ret3;
		}
	} while (true);

	// Verify file sizes.
	if (bin_file_header->size > MAX_BIN_FILESIZE) {
		return -ENOMEM;
	}
	if (txt_file_header->size > MAX_TXT_FILESIZE) {
		return -ENOMEM;
	}
	if (json_file_header->size > MAX_JSON_FILESIZE) {
		return -ENOMEM;
	}

	// Verify the .bin filename.
	// TODO: Also .txt and .json?
	// TODO: If incorrect, do a search for the correct filename.
	if (mode.bin_filename != bin_file_header->name) {
		return -ENOENT;
	}

	last_bin_filename = bin_file_header->name;

	last_bin_data.resize(bin_file_header->size);
	if (bin_file_header->size > 0) {
		ret1 = mtar_read_data(const_cast<mtar_t*>(&mode.p_tar_files->bin_tar), last_bin_data.data(), bin_file_header->size);
	}
	last_txt_data.resize(txt_file_header->size+1);
	if (txt_file_header->size > 0) {
		ret2 = mtar_read_data(const_cast<mtar_t*>(&mode.p_tar_files->txt_tar), last_txt_data.data(), txt_file_header->size);
	}
	last_json_data.resize(json_file_header->size+1);
	if (json_file_header->size > 0) {
		ret3 = mtar_read_data(const_cast<mtar_t*>(&mode.p_tar_files->json_tar), last_json_data.data(), json_file_header->size);
	}

	// SNES: Ensure the BIN file is at least 64 KB.
	if (mode.bin_filename.size() > 4 &&
	    mode.bin_filename.compare(mode.bin_filename.size() - 4, string::npos, ".sfc") == 0)
	{
		static constexpr size_t MIN_BIN_DATA_SIZE = 64U * 1024U;
		if (last_bin_data.size() < MIN_BIN_DATA_SIZE) {
			const size_t cur_size = last_bin_data.size();
			last_bin_data.resize(MIN_BIN_DATA_SIZE);
			memset(&last_bin_data[cur_size], 0, MIN_BIN_DATA_SIZE - cur_size);
		}
	}

	// Ensure the text and JSON data arrays are NULL-terminated.
	last_txt_data[last_txt_data.size()-1] = 0;
	last_json_data[last_json_data.size()-1] = 0;
	last_txt_data.resize(last_txt_data.size()-1);
	last_json_data.resize(last_json_data.size()-1);

	if (ret1 != MTAR_ESUCCESS) {
		return ret1;
	}
	if (ret2 != MTAR_ESUCCESS) {
		return ret2;
	}
	if (ret3 != MTAR_ESUCCESS) {
		return ret3;
	}

	ret1 = mtar_next(const_cast<mtar_t*>(&mode.p_tar_files->bin_tar));
	ret2 = mtar_next(const_cast<mtar_t*>(&mode.p_tar_files->txt_tar));
	ret3 = mtar_next(const_cast<mtar_t*>(&mode.p_tar_files->json_tar));

	if (ret1 != MTAR_ESUCCESS) {
		return ret1;
	}
	if (ret2 != MTAR_ESUCCESS) {
		return ret2;
	}
	if (ret3 != MTAR_ESUCCESS) {
		return ret3;
	}

	return MTAR_ESUCCESS;
}

TEST_P(RomHeaderTest, Text)
{
	// Parameterized test
	const RomHeaderTest_mode &mode = GetParam();

	if (last_bin_filename != mode.bin_filename) {
		// Need to read the next set of files.
		int ret = read_next_files(mode);
		ASSERT_EQ(ret, 0) << "Incorrect files loaded from the .tar file.";
	}

	// Make sure the binary file isn't empty.
	ASSERT_GT(last_bin_data.size(), 0U) << "Binary file is empty.";

	// Get the text output for this binary file, e.g. as if we're running `rpcli`.
	const shared_ptr<MemFile> memFile = std::make_shared<MemFile>(last_bin_data.data(), last_bin_data.size());
	ASSERT_NE(memFile, nullptr) << "Unable to create MemFile object for binary data.";
	memFile->setFilename(mode.bin_filename);	// needed for SNES
	const RomDataPtr romData = RomDataFactory::create(memFile);

	if (romData) {
		// RomData object was created.
		ASSERT_GT(last_txt_data.size(), 0U) << "Binary file is valid RomData, but text file is empty.";

		ostringstream oss;
		oss << ROMOutput(romData.get(), 0, 0);

		// FIXME: On Windows, ROMOutput adds an extra newline at the end.
		// The string should end with a single newline.
		string str = oss.str();
		while (!str.empty()) {
			if (str[str.size()-1] == '\n') {
				str.resize(str.size()-1);
			} else {
				break;
			}
		}
		str += '\n';

		ASSERT_EQ(reinterpret_cast<const char*>(last_txt_data.data()), str) << "Text output does not match the expected value.";
	} else {
		// No RomData object. Verify that the text file is empty.
		ASSERT_EQ(last_txt_data.size(), 0U) << "Binary file is not valid RomData, but text file is not empty.";
	}
}

TEST_P(RomHeaderTest, JSON)
{
	// Parameterized test
	const RomHeaderTest_mode &mode = GetParam();

	if (last_bin_filename != mode.bin_filename) {
		// Need to read the next set of files.
		int ret = read_next_files(mode);
		ASSERT_EQ(ret, 0) << "Incorrect files loaded from the .tar file.";
	}

	// Make sure the binary file isn't empty.
	ASSERT_GT(last_bin_data.size(), 0U) << "Binary file is empty.";

	// Get the JSON output for this binary file, e.g. as if we're running `rpcli -j`.
	const shared_ptr<MemFile> memFile = std::make_shared<MemFile>(last_bin_data.data(), last_bin_data.size());
	ASSERT_NE(memFile, nullptr) << "Unable to create MemFile object for binary data.";
	memFile->setFilename(mode.bin_filename);	// needed for SNES
	const RomDataPtr romData = RomDataFactory::create(memFile);

	if (romData) {
		// RomData object was created.
		ASSERT_GT(last_json_data.size(), 0U) << "Binary file is valid RomData, but JSON file is empty.";

		ostringstream oss;
		oss << JSONROMOutput(romData.get(), 0, LibRpBase::OF_JSON_NoPrettyPrint);

		// The expected JSON files have trailing newlines, but RapidJSON
		// does not add a trailing newline when not pretty-printing.
		string actual_json = oss.str();
		actual_json += '\n';

		const char *const expected_json = reinterpret_cast<const char*>(last_json_data.data());
		ASSERT_EQ(expected_json, actual_json);
	} else {
		// No RomData object. Verify that the JSON file is correct.
		ASSERT_EQ(last_json_data.size(), 33U) << "Binary file is not valid RomData, but JSON file does not have an error message.";
		ASSERT_STREQ(reinterpret_cast<const char*>(last_json_data.data()), "{\"error\":\"rom is not supported\"}\n")
			<< "Binary file is not valid RomData, but JSON file does not have an error message.";
	}
}

/** Test case parameters. **/

/**
 * Open the .tar files and create the test case list.
 * @param bin_tar_filename .tar file contianing the header files
 * @param txt_tar_filename .tar file containing the text files
 * @param json_tar_filename .tar file containing the JSON files
 * @return Header filenames
 */
forward_list<RomHeaderTest_mode> RomHeaderTest::ReadTestCasesFromDisk(const char *bin_tar_filename, const char *txt_tar_filename, const char *json_tar_filename)
{
	// NOTE: Cannot use ASSERT_TRUE() here.
	forward_list<RomHeaderTest_mode> files;

	// Open the .tar files.
	all_tar_files.emplace_front();
	tar_files_t *const p_tar_files = &(*(all_tar_files.begin()));

	int ret = mtar_zstd_open_ro(&p_tar_files->bin_tar, bin_tar_filename);
	EXPECT_EQ(ret, 0) << "Could not open '" << bin_tar_filename << "', check the test directory!";
	if (ret != 0) {
		all_tar_files.pop_front();
		return files;
	}

	ret = mtar_zstd_open_ro(&p_tar_files->txt_tar, txt_tar_filename);
	EXPECT_EQ(ret, 0) << "Could not open '" << bin_tar_filename << "', check the test directory!";
	if (ret != 0) {
		mtar_close(&p_tar_files->bin_tar);
		all_tar_files.pop_front();
		return files;
	}

	ret = mtar_zstd_open_ro(&p_tar_files->json_tar, json_tar_filename);
	EXPECT_EQ(ret, 0) << "Could not open '" << bin_tar_filename << "', check the test directory!";
	if (ret != 0) {
		mtar_close(&p_tar_files->bin_tar);
		mtar_close(&p_tar_files->txt_tar);
		all_tar_files.pop_front();
		return files;
	}

	// Read the headers .tar file and get all the filenames.
	// The .txt and .json .tar files should have the same filenames,
	// but with added .txt and .json extensions.
	bool found_any_files = false;
	mtar_header_t h;
	for (; ; mtar_next(&p_tar_files->bin_tar)) {
		int err = mtar_read_header(&p_tar_files->bin_tar, &h);
		if (err == MTAR_ENULLRECORD) {
			// Finished reading the .tar file.
			break;
		}

		EXPECT_EQ(err, MTAR_ESUCCESS) << "Error reading from the .bin.tar file.";
		if (err != MTAR_ESUCCESS) {
			mtar_close(&p_tar_files->bin_tar);
			mtar_close(&p_tar_files->txt_tar);
			mtar_close(&p_tar_files->json_tar);
			all_tar_files.pop_front();
			files.clear();
			return files;
		}

		if (h.type != 0 /*MTAR_TREG*/) {
			// Not a regular file.
			continue;
		}

		// NOTE: Checking .txt and .json sizes later.
		EXPECT_LE(h.size, MAX_BIN_FILESIZE);
		if (h.size > MAX_BIN_FILESIZE)
			continue;

		files.emplace_front(h.name, p_tar_files);
		found_any_files = true;
	}

	// NOTE: std::forward_list adds items in reverse-order.
	// We'll need to reverse it before returning the list.
	files.reverse();

	// Rewind the .bin header .tar file for the actual tests.
	mtar_rewind(&p_tar_files->bin_tar);
	EXPECT_TRUE(found_any_files) << "No files were read from the .bin.tar file.";
	return files;
}

/**
 * Test case suffix generator.
 * @param info Test parameter information.
 * @return Test case suffix.
 */
string RomHeaderTest::test_case_suffix_generator(const ::testing::TestParamInfo<RomHeaderTest_mode> &info)
{
	string suffix = info.param.bin_filename;

	// Replace all non-alphanumeric characters with '_'.
	// See gtest-param-util.h::IsValidParamName().
	// NOTE: Need to replace '+' with 'x' to prevent duplicates.
	for (char &c : suffix) {
		if (c == '+') {
			c = 'x';
		} else if (!ISALNUM(c)) {
			c = '_';
		}
	}

	return suffix;
}

/* Audio */

// ADX: Music from Sonic Adventure DX, plus some JP voices.
INSTANTIATE_TEST_SUITE_P(ADX_SADX, RomHeaderTest,
	testing::ValuesIn(RomHeaderTest::ReadTestCasesFromDisk(
		"Audio/ADX.SADX.bin.tar.zst",
		"Audio/ADX.SADX.txt.tar.zst",
		"Audio/ADX.SADX.json.tar.zst"))
	, RomHeaderTest::test_case_suffix_generator);

/* Console */

INSTANTIATE_TEST_SUITE_P(DreamcastSave, RomHeaderTest,
	testing::ValuesIn(RomHeaderTest::ReadTestCasesFromDisk(
		"Console/DreamcastSave.bin.tar.zst",
		"Console/DreamcastSave.txt.tar.zst",
		"Console/DreamcastSave.json.tar.zst"))
	, RomHeaderTest::test_case_suffix_generator);

// GameCube: A few WIA/RVZ headers to ensure they don't break again.
INSTANTIATE_TEST_SUITE_P(GameCubeWiaRvz, RomHeaderTest,
	testing::ValuesIn(RomHeaderTest::ReadTestCasesFromDisk(
		"Console/GameCube.wia-rvz.bin.tar.zst",
		"Console/GameCube.wia-rvz.txt.tar.zst",
		"Console/GameCube.wia-rvz.json.tar.zst"))
	, RomHeaderTest::test_case_suffix_generator);

INSTANTIATE_TEST_SUITE_P(MegaDrive, RomHeaderTest,
	testing::ValuesIn(RomHeaderTest::ReadTestCasesFromDisk(
		"Console/MegaDrive.bin.tar.zst",
		"Console/MegaDrive.txt.tar.zst",
		"Console/MegaDrive.json.tar.zst"))
	, RomHeaderTest::test_case_suffix_generator);

INSTANTIATE_TEST_SUITE_P(MegaDrive_32X, RomHeaderTest,
	testing::ValuesIn(RomHeaderTest::ReadTestCasesFromDisk(
		"Console/MegaDrive_32X.bin.tar.zst",
		"Console/MegaDrive_32X.txt.tar.zst",
		"Console/MegaDrive_32X.json.tar.zst"))
	, RomHeaderTest::test_case_suffix_generator);

INSTANTIATE_TEST_SUITE_P(MegaDrive_Pico, RomHeaderTest,
	testing::ValuesIn(RomHeaderTest::ReadTestCasesFromDisk(
		"Console/MegaDrive_Pico.bin.tar.zst",
		"Console/MegaDrive_Pico.txt.tar.zst",
		"Console/MegaDrive_Pico.json.tar.zst"))
	, RomHeaderTest::test_case_suffix_generator);

INSTANTIATE_TEST_SUITE_P(NES, RomHeaderTest,
	testing::ValuesIn(RomHeaderTest::ReadTestCasesFromDisk(
		"Console/NES.bin.tar.zst",
		"Console/NES.txt.tar.zst",
		"Console/NES.json.tar.zst"))
	, RomHeaderTest::test_case_suffix_generator);

INSTANTIATE_TEST_SUITE_P(N64, RomHeaderTest,
	testing::ValuesIn(RomHeaderTest::ReadTestCasesFromDisk(
		"Console/N64.bin.tar.zst",
		"Console/N64.txt.tar.zst",
		"Console/N64.json.tar.zst"))
	, RomHeaderTest::test_case_suffix_generator);

INSTANTIATE_TEST_SUITE_P(Sega8Bit_SMS, RomHeaderTest,
	testing::ValuesIn(RomHeaderTest::ReadTestCasesFromDisk(
		"Console/Sega8Bit_SMS.bin.tar.zst",
		"Console/Sega8Bit_SMS.txt.tar.zst",
		"Console/Sega8Bit_SMS.json.tar.zst"))
	, RomHeaderTest::test_case_suffix_generator);

INSTANTIATE_TEST_SUITE_P(Sega8Bit_SMS_SDSC, RomHeaderTest,
	testing::ValuesIn(RomHeaderTest::ReadTestCasesFromDisk(
		"Console/Sega8Bit_SMS_SDSC.bin.tar.zst",
		"Console/Sega8Bit_SMS_SDSC.txt.tar.zst",
		"Console/Sega8Bit_SMS_SDSC.json.tar.zst"))
	, RomHeaderTest::test_case_suffix_generator);

INSTANTIATE_TEST_SUITE_P(Sega8Bit_GG, RomHeaderTest,
	testing::ValuesIn(RomHeaderTest::ReadTestCasesFromDisk(
		"Console/Sega8Bit_GG.bin.tar.zst",
		"Console/Sega8Bit_GG.txt.tar.zst",
		"Console/Sega8Bit_GG.json.tar.zst"))
	, RomHeaderTest::test_case_suffix_generator);

INSTANTIATE_TEST_SUITE_P(Sega8Bit_GG_SDSC, RomHeaderTest,
	testing::ValuesIn(RomHeaderTest::ReadTestCasesFromDisk(
		"Console/Sega8Bit_GG_SDSC.bin.tar.zst",
		"Console/Sega8Bit_GG_SDSC.txt.tar.zst",
		"Console/Sega8Bit_GG_SDSC.json.tar.zst"))
	, RomHeaderTest::test_case_suffix_generator);

INSTANTIATE_TEST_SUITE_P(SNES, RomHeaderTest,
	testing::ValuesIn(RomHeaderTest::ReadTestCasesFromDisk(
		"Console/SNES.bin.tar.zst",
		"Console/SNES.txt.tar.zst",
		"Console/SNES.json.tar.zst"))
	, RomHeaderTest::test_case_suffix_generator);

INSTANTIATE_TEST_SUITE_P(SNES_BSX, RomHeaderTest,
	testing::ValuesIn(RomHeaderTest::ReadTestCasesFromDisk(
		"Console/SNES_BSX.bin.tar.zst",
		"Console/SNES_BSX.txt.tar.zst",
		"Console/SNES_BSX.json.tar.zst"))
	, RomHeaderTest::test_case_suffix_generator);

INSTANTIATE_TEST_SUITE_P(SufamiTurbo, RomHeaderTest,
	testing::ValuesIn(RomHeaderTest::ReadTestCasesFromDisk(
		"Console/SufamiTurbo.bin.tar.zst",
		"Console/SufamiTurbo.txt.tar.zst",
		"Console/SufamiTurbo.json.tar.zst"))
	, RomHeaderTest::test_case_suffix_generator);

/* Handheld */

INSTANTIATE_TEST_SUITE_P(DMG, RomHeaderTest,
	testing::ValuesIn(RomHeaderTest::ReadTestCasesFromDisk(
		"Handheld/DMG.bin.tar.zst",
		"Handheld/DMG.txt.tar.zst",
		"Handheld/DMG.json.tar.zst"))
	, RomHeaderTest::test_case_suffix_generator);

INSTANTIATE_TEST_SUITE_P(GameBoyAdvance, RomHeaderTest,
	testing::ValuesIn(RomHeaderTest::ReadTestCasesFromDisk(
		"Handheld/GameBoyAdvance.bin.tar.zst",
		"Handheld/GameBoyAdvance.txt.tar.zst",
		"Handheld/GameBoyAdvance.json.tar.zst"))
	, RomHeaderTest::test_case_suffix_generator);

INSTANTIATE_TEST_SUITE_P(Nintendo3DS_3DSident, RomHeaderTest,
	testing::ValuesIn(RomHeaderTest::ReadTestCasesFromDisk(
		"Handheld/Nintendo3DS-3DSident.bin.tar.zst",
		"Handheld/Nintendo3DS-3DSident.txt.tar.zst",
		"Handheld/Nintendo3DS-3DSident.json.tar.zst"))
	, RomHeaderTest::test_case_suffix_generator);

INSTANTIATE_TEST_SUITE_P(NintendoDS, RomHeaderTest,
	testing::ValuesIn(RomHeaderTest::ReadTestCasesFromDisk(
		"Handheld/NintendoDS.bin.tar.zst",
		"Handheld/NintendoDS.txt.tar.zst",
		"Handheld/NintendoDS.json.tar.zst"))
	, RomHeaderTest::test_case_suffix_generator);

/* Other */

INSTANTIATE_TEST_SUITE_P(Amiibo, RomHeaderTest,
	testing::ValuesIn(RomHeaderTest::ReadTestCasesFromDisk(
		"Other/Amiibo.bin.tar.zst",
		"Other/Amiibo.txt.tar.zst",
		"Other/Amiibo.json.tar.zst"))
	, RomHeaderTest::test_case_suffix_generator);

INSTANTIATE_TEST_SUITE_P(DirectDrawSurface, RomHeaderTest,
	testing::ValuesIn(RomHeaderTest::ReadTestCasesFromDisk(
		"Other/DirectDrawSurface.bin.tar.zst",
		"Other/DirectDrawSurface.txt.tar.zst",
		"Other/DirectDrawSurface.json.tar.zst"))
	, RomHeaderTest::test_case_suffix_generator);

} }

extern "C" int gtest_main(int argc, TCHAR *argv[])
{
	fprintf(stderr, "LibRomData test suite: RomHeader tests.\n\n");
	fflush(nullptr);

	// Check for amiibo-data.bin in the current directory or bin/.
	// If found, set the AmiiboData override.
	TCHAR amiibo_data_bin_path[4096];
	amiibo_data_bin_path[0] = _T('\0');
#define AMIIBO_DATA_BIN_BASE _T("amiibo-data.bin")
#define AMIIBO_DATA_BIN_WITH_BIN_DIR _T("bin") DIR_SEP_STR AMIIBO_DATA_BIN_BASE
#define AMIIBO_DATA_BIN_WITH_DOTDOT_DOTDOT_DOTDOT_BIN_DIR _T("..") DIR_SEP_STR _T("..") DIR_SEP_STR _T("..") DIR_SEP_STR AMIIBO_DATA_BIN_WITH_BIN_DIR
	if (!_taccess(AMIIBO_DATA_BIN_BASE, R_OK)) {
		// Found in the current directory.
		if (!_tgetcwd(amiibo_data_bin_path, ARRAY_SIZE(amiibo_data_bin_path))) {
			_ftprintf(stderr, _T("*** ERROR: getcwd() failed: %s\n"), _tcserror(errno));
			return EXIT_FAILURE;
		}
		if (_tcslen(amiibo_data_bin_path) < (ARRAY_SIZE(amiibo_data_bin_path) - 32)) {
			_tcscat(amiibo_data_bin_path, DIR_SEP_STR AMIIBO_DATA_BIN_BASE);
		}
	} else if (!_taccess(AMIIBO_DATA_BIN_WITH_BIN_DIR, R_OK)) {
		// Found in the current directory.
		if (!_tgetcwd(amiibo_data_bin_path, ARRAY_SIZE(amiibo_data_bin_path))) {
			_ftprintf(stderr, _T("*** ERROR: getcwd() failed: %s\n"), _tcserror(errno));
			return EXIT_FAILURE;
		}
		if (_tcslen(amiibo_data_bin_path) < (ARRAY_SIZE(amiibo_data_bin_path) - 32)) {
			_tcscat(amiibo_data_bin_path, DIR_SEP_STR AMIIBO_DATA_BIN_WITH_BIN_DIR);
		}
	}
#ifndef _WIN32
	else if (!_taccess(AMIIBO_DATA_BIN_WITH_DOTDOT_DOTDOT_DOTDOT_BIN_DIR, R_OK)) {
		// We're in ${CMAKE_CURRENT_BINARY_DIR}.
		if (!_tgetcwd(amiibo_data_bin_path, ARRAY_SIZE(amiibo_data_bin_path))) {
			_ftprintf(stderr, _T("*** ERROR: getcwd() failed: %s\n"), _tcserror(errno));
			return EXIT_FAILURE;
		}
		if (_tcslen(amiibo_data_bin_path) < (ARRAY_SIZE(amiibo_data_bin_path) - 32)) {
			_tcscat(amiibo_data_bin_path, DIR_SEP_STR AMIIBO_DATA_BIN_WITH_DOTDOT_DOTDOT_DOTDOT_BIN_DIR);
		}
	}
#endif /* !_WIN32 */
	if (amiibo_data_bin_path[0] != _T('\0')) {
		fputs("Setting amiibo-data.bin override to:\n", stderr);
		_fputts(amiibo_data_bin_path, stderr);
		fputs("\n\n", stderr);

		LibRomData::AmiiboData::overrideAmiiboDataBinFilename(amiibo_data_bin_path);
	}

	// Check for the RomHeaders directory and chdir() into it.
#ifdef _WIN32
	static constexpr array<const TCHAR*, 11> subdirs = {{
		_T("RomHeaders"),
		_T("bin\\RomHeaders"),
		_T("src\\libromdata\\tests\\RomHeaders"),
		_T("..\\src\\libromdata\\tests\\RomHeaders"),
		_T("..\\..\\src\\libromdata\\tests\\RomHeaders"),
		_T("..\\..\\..\\src\\libromdata\\tests\\RomHeaders"),
		_T("..\\..\\..\\..\\src\\libromdata\\tests\\RomHeaders"),
		_T("..\\..\\..\\..\\..\\src\\libromdata\\tests\\RomHeaders"),
		_T("..\\..\\..\\bin\\RomHeaders"),
		_T("..\\..\\..\\bin\\Debug\\RomHeaders"),
		_T("..\\..\\..\\bin\\Release\\RomHeaders"),
	}};
#else /* !_WIN32 */
	static constexpr array<const TCHAR* ,9> subdirs = {{
		_T("RomHeaders"),
		_T("bin/RomHeaders"),
		_T("src/libromdata/tests/RomHeaders"),
		_T("../src/libromdata/tests/RomHeaders"),
		_T("../../src/libromdata/tests/RomHeaders"),
		_T("../../../src/libromdata/tests/RomHeaders"),
		_T("../../../../src/libromdata/tests/RomHeaders"),
		_T("../../../../../src/libromdata/tests/RomHeaders"),
		_T("../../../bin/RomHeaders"),
	}};
#endif /* _WIN32 */

	bool is_found = false;
	for (const TCHAR *const subdir : subdirs) {
		if (!_taccess(subdir, R_OK)) {
			if (_tchdir(subdir) == 0) {
				is_found = true;
				break;
			}
		}
	}

	if (!is_found) {
		fputs("*** ERROR: Cannot find the png_data test images directory.\n", stderr);
		return EXIT_FAILURE;
	}

	// "--gtest_shuffle" is not supported due to the use of .tar.zst files.
	// If it's specified, remove it.
	bool warning_shown = false;
	for (int i = 1; i < argc; i++) {
		if (!_tcsncmp(argv[i], _T("--gtest_shuffle"), 15)) {
			// NOTE: argv[i] may be dynamically allocated.
			// We can't simply reassign the pointer, so NULL out the string instead.
			argv[i][0] = _T('\0');
			if (!warning_shown) {
				fputs("*** WARNING: --gtest_shuffle is not supported by RomHeaderTest.\n\n", stderr);
				warning_shown = true;
			}
		}
	}

	// coverity[fun_call_w_exception]: uncaught exceptions cause nonzero exit anyway, so don't warn.
	::testing::InitGoogleTest(&argc, argv);
	int ret = RUN_ALL_TESTS();

	// Close all .tar files.
	for (auto &tar_files : LibRomData::Tests::RomHeaderTest::all_tar_files) {
		mtar_close(&tar_files.bin_tar);
		mtar_close(&tar_files.txt_tar);
		mtar_close(&tar_files.json_tar);
	}

	return ret;
}
