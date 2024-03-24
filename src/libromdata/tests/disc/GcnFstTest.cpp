/***************************************************************************
 * ROM Properties Page shell extension. (libromdata/tests)                 *
 * GcnFstTest.cpp: GameCube/Wii FST test.                                  *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// for HAVE_ZLIB for mz_compat.h
#include "config.librpbase.h"

// Google Test
#include "gtest/gtest.h"
#include "tcharx.h"

// MiniZip
#include <zlib.h>
#include "mz_zip.h"
#include "mz_compat.h"

// Other rom-properties libraries
#include "librpfile/FileSystem.hpp"
#include "librptext/conversion.hpp"
#include "librptext/printf.hpp"
using namespace LibRpText;

// libromdata
#include "disc/GcnFst.hpp"
using LibRpBase::IFst;

// libwin32common
#ifdef _WIN32
#  include "libwin32common/RpWin32_sdk.h"
#endif

// FST printer
#include "FstPrint.hpp"

// C includes (C++ namespace)
#include "ctypex.h"

// C++ includes
#include <algorithm>
#include <array>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>
using std::array;
using std::istringstream;
using std::ostream;
using std::string;
using std::stringstream;
using std::unordered_set;
using std::vector;

// Uninitialized vector class
#include "uvector.h"

namespace LibRomData { namespace Tests {

struct GcnFstTest_mode
{
	std::string fst_filename;	// FST filename in the FST Zip file.
	uint8_t offsetShift;		// File offset shift. (0 == GCN, 2 == Wii)

	GcnFstTest_mode(
		const char *fst_filename,
		uint8_t offsetShift)
		: fst_filename(fst_filename)
		, offsetShift(offsetShift)
	{ }

	GcnFstTest_mode(
		const string &fst_filename,
		uint8_t offsetShift)
		: fst_filename(fst_filename)
		, offsetShift(offsetShift)
	{ }
};

/**
 * Formatting function for GcnFstTest_mode.
 */
inline ::std::ostream& operator<<(::std::ostream& os, const GcnFstTest_mode& mode) {
        return os << mode.fst_filename;
};

// Maximum file size for FST files.
static const uint64_t MAX_GCN_FST_BIN_FILESIZE = 1024UL*1024UL;	// 1.0 MB
static const uint64_t MAX_GCN_FST_TXT_FILESIZE = 1536UL*1024UL;	// 1.5 MB

class GcnFstTest : public ::testing::TestWithParam<GcnFstTest_mode>
{
	protected:
		GcnFstTest()
			: ::testing::TestWithParam<GcnFstTest_mode>()
			, m_fst(nullptr)
		{ }

		void SetUp(void) final;
		void TearDown(void) final;

		/**
		 * Open a Zip file for reading.
		 * @param filename Zip filename.
		 * @return Zip file, or nullptr on error.
		 */
		static unzFile openZip(const char *filename);

		/**
		 * Get a file from a Zip file.
		 * @param zip_filename	[in] Zip filename.
		 * @param int_filename	[in] Internal filename.
		 * @param buf		[out] uvector buffer for the data.
		 * @param max_filesize	[in,opt] Maximum file size. (default is MAX_GCN_FST_BIN_FILESIZE)
		 * @return Number of bytes read, or negative on error.
		 */
		static int getFileFromZip(const char *zip_filename,
			const char *int_filename,
			rp::uvector<uint8_t>& buf,
			uint64_t max_filesize = MAX_GCN_FST_BIN_FILESIZE);

	public:
		// FST data.
		rp::uvector<uint8_t> m_fst_buf;
		IFst *m_fst;

		/**
		 * Recursively check a subdirectory for duplicate filenames.
		 * @param subdir Subdirectory path.
		 */
		void checkNoDuplicateFilenames(const char *subdir);

	public:
		/** Test case parameters. **/

		/**
		 
		 * Get the list of FST files from a Zip file.
		 * @param offsetShift File offset shift. (0 == GCN, 2 == Wii)
		 * @return FST files.
		 */
		static vector<GcnFstTest_mode> ReadTestCasesFromDisk(uint8_t offsetShift);

		/**
		 * Test case suffix generator.
		 * @param info Test parameter information.
		 * @return Test case suffix.
		 */
		static string test_case_suffix_generator(const ::testing::TestParamInfo<GcnFstTest_mode> &info);
};

/**
 * SetUp() function.
 * Run before each test.
 */
void GcnFstTest::SetUp(void)
{
	// Parameterized test.
	const GcnFstTest_mode &mode = GetParam();

	// Open the Zip file.
	// NOTE: MiniZip 2.2.3's compatibility functions
	// take UTF-8 on Windows, not UTF-16.
	const char *zip_filename = nullptr;
	switch (mode.offsetShift) {
		case 0:
			zip_filename = "GameCube.fst.bin.zip";
			break;
		case 2:
			zip_filename = "Wii.fst.bin.zip";
			break;
		default:
			ASSERT_TRUE(false) << "offsetShift is " << (int)mode.offsetShift << "; should be either 0 or 2.";
	}

	ASSERT_GT(getFileFromZip(zip_filename, mode.fst_filename.c_str(), m_fst_buf, MAX_GCN_FST_BIN_FILESIZE), 0);

	// Check for NKit FST recovery data.
	// These FSTs have an extra header at the top, indicating what
	// disc the FST belongs to.
	unsigned int fst_start_offset = 0;
	static constexpr array<uint8_t, 10> root_dir_data = {{1,0,0,0,0,0,0,0,0,0}};
	if (m_fst_buf.size() >= 0x60) {
		if (!memcmp(&m_fst_buf[0x50], root_dir_data.data(), root_dir_data.size())) {
			// Found an NKit FST.
			fst_start_offset = 0x50;
		}
	}

	// Create the GcnFst object.
	m_fst = new GcnFst(&m_fst_buf[fst_start_offset],
		static_cast<uint32_t>(m_fst_buf.size() - fst_start_offset), mode.offsetShift);
	ASSERT_TRUE(m_fst->isOpen());
}

/**
 * TearDown() function.
 * Run after each test.
 */
void GcnFstTest::TearDown(void)
{
	delete m_fst;
	m_fst = nullptr;
}

/**
 * Open a Zip file for reading.
 * @param filename Zip filename.
 * @return Zip file, or nullptr on error.
 */
unzFile GcnFstTest::openZip(const char *filename)
{
#ifdef _WIN32
	// NOTE: MiniZip 3.0.2's compatibility functions
	// take UTF-8 on Windows, not UTF-16.
	zlib_filefunc64_def ffunc;
	fill_win32_filefunc64(&ffunc);
	return unzOpen2_64(filename, &ffunc);
#else /* !_WIN32 */
	return unzOpen(filename);
#endif /* _WIN32 */
}

/**
 * Get a file from a Zip file.
 * @param zip_filename	[in] Zip filename.
 * @param int_filename	[in] Internal filename.
 * @param buf		[out] uvector buffer for the data.
 * @param max_filesize	[in,opt] Maximum file size. (default is MAX_GCN_FST_BIN_FILESIZE)
 * @return Number of bytes read, or negative on error.
 */
int GcnFstTest::getFileFromZip(const char *zip_filename,
	const char *int_filename,
	rp::uvector<uint8_t>& buf,
	uint64_t max_filesize)
{
	// Open the Zip file.
	// NOTE: MiniZip 2.2.3's compatibility functions
	// take UTF-8 on Windows, not UTF-16.
	unzFile unz = openZip(zip_filename);
	EXPECT_TRUE(unz != nullptr) <<
		"Could not open '" << zip_filename << "', check the test directory!";
	if (!unz) {
		return -1;
	}

	// Locate the required FST file.
	// TODO: Always case-insensitive? (currently using OS-dependent value)
	int ret = unzLocateFile(unz, int_filename, nullptr);
	EXPECT_EQ(UNZ_OK, ret);
	if (ret != UNZ_OK) {
		unzClose(unz);
		return -2;
	}

	// Verify the FST file size.
	unz_file_info64 file_info;
	ret = unzGetCurrentFileInfo64(unz,
		&file_info,
		nullptr, 0,
		nullptr, 0,
		nullptr, 0);
	EXPECT_EQ(UNZ_OK, ret);
	if (ret != UNZ_OK) {
		unzClose(unz);
		return -3;
	}
	EXPECT_LE(file_info.uncompressed_size, max_filesize) <<
		"Compressed file '" << int_filename << "' is too big.";
	if (file_info.uncompressed_size > max_filesize) {
		unzClose(unz);
		return -4;
	}

	// Open the FST file.
	ret = unzOpenCurrentFile(unz);
	EXPECT_EQ(UNZ_OK, ret);
	if (ret != UNZ_OK) {
		unzClose(unz);
		return -5;
	}

	// Read the FST file.
	// NOTE: zlib and minizip are only guaranteed to be able to
	// read UINT16_MAX (64 KB) at a time, and the updated MiniZip
	// from https://github.com/nmoinvaz/minizip enforces this.
	buf.resize(static_cast<size_t>(file_info.uncompressed_size));
	uint8_t *p = buf.data();
	size_t size = buf.size();
	while (size > 0) {
		int to_read = static_cast<int>(size > UINT16_MAX ? UINT16_MAX : size);
		ret = unzReadCurrentFile(unz, p, to_read);
		EXPECT_EQ(to_read, ret);
		if (ret != to_read) {
			unzClose(unz);
			return -6;
		}

		// ret == number of bytes read.
		p += ret;
		size -= ret;
	}

	// Read one more byte to ensure unzReadCurrentFile() returns 0 for EOF.
	// NOTE: MiniZip will zero out tmp if there's no data available.
	uint8_t tmp;
	ret = unzReadCurrentFile(unz, &tmp, 1);
	EXPECT_EQ(0, ret);
	if (ret != 0) {
		unzClose(unz);
		return -7;
	}

	// Close the FST file.
	// An error will occur here if the CRC is incorrect.
	ret = unzCloseCurrentFile(unz);
	EXPECT_EQ(UNZ_OK, ret);
	if (ret != UNZ_OK) {
		unzClose(unz);
		return -8;
	}

	// Close the Zip file.
	unzClose(unz);
	return static_cast<int>(file_info.uncompressed_size);
}

/**
 * Recursively check a subdirectory for duplicate filenames.
 * @param subdir Subdirectory path.
 */
void GcnFstTest::checkNoDuplicateFilenames(const char *subdir)
{
	unordered_set<string> filenames;
	unordered_set<string> subdirs;

	IFst::Dir *dirp = m_fst->opendir(subdir);
	ASSERT_TRUE(dirp != nullptr) <<
		"Failed to open directory '" << subdir << "'.";

	IFst::DirEnt *dirent = m_fst->readdir(dirp);
	while (dirent != nullptr) {
		// Make sure we haven't seen this filename in
		// the current subdirectory yet.
		EXPECT_TRUE(filenames.find(dirent->name) == filenames.end()) <<
			"Directory '" << subdir << "' has duplicate filename '" << dirent->name << "'.";

		// Filename has been seen now.
		filenames.insert(dirent->name);

		// Is this a subdirectory?
		if (dirent->type == DT_DIR) {
			subdirs.insert(dirent->name);
		}

		// Next entry.
		dirent = m_fst->readdir(dirp);
	}

	// Check subdirectories.
	for (const string &p : subdirs) {
		string path = subdir;
		if (!path.empty() && path[path.size()-1] != '/') {
			path += '/';
		}
		path += p;
		checkNoDuplicateFilenames(path.c_str());
	}

	// End of directory.
	m_fst->closedir(dirp);
}

/**
 * Verify that '/' is collapsed correctly.
 */
TEST_P(GcnFstTest, RootDirectoryCollapse)
{
	char path_buf[17];
	memset(path_buf, 0, sizeof(path_buf));
	for (unsigned int i = 0; i < sizeof(path_buf)-1; i++) {
		path_buf[i] = '/';
		IFst::Dir *rootdir = m_fst->opendir(path_buf);
		EXPECT_TRUE(rootdir != nullptr);
		m_fst->closedir(rootdir);
	}
}

/**
 * Make sure there aren't any duplicate filenames in all subdirectories.
 */
TEST_P(GcnFstTest, NoDuplicateFilenames)
{
	ASSERT_NO_FATAL_FAILURE(checkNoDuplicateFilenames("/"));
	EXPECT_FALSE(m_fst->hasErrors());
}

/**
 * Print the FST directory structure and compare it to a known-good version.
 */
TEST_P(GcnFstTest, FstPrint)
{
	// Parameterized test.
	const GcnFstTest_mode &mode = GetParam();

	// Open the Zip file.
	// NOTE: MiniZip 2.2.3's compatibility functions
	// take UTF-8 on Windows, not UTF-16.
	const char *zip_filename = nullptr;
	switch (mode.offsetShift) {
		case 0:
			zip_filename = "GameCube.fst.txt.zip";
			break;
		case 2:
			zip_filename = "Wii.fst.txt.zip";
			break;
		default:
			ASSERT_TRUE(false) << "offsetShift is " << (int)mode.offsetShift << "; should be either 0 or 2.";
	}

	// Replace ".bin" in the FST filename with ".txt".
	string fst_txt_filename = mode.fst_filename;
	ASSERT_GT(fst_txt_filename.size(), 8U) <<
		"Internal filename '" << mode.fst_filename << "' doesn't have a '.fst.bin' extension.";
	ASSERT_EQ(".fst.bin", fst_txt_filename.substr(fst_txt_filename.size() - 8)) <<
		"Internal filename '" << mode.fst_filename << "' doesn't have a '.fst.bin' extension.";
	fst_txt_filename.replace(fst_txt_filename.size() - 8, 8, ".fst.txt");

	// Get the known-good FST printout.
	rp::uvector<uint8_t> fst_txt_buf;
	ASSERT_GT(getFileFromZip(zip_filename, fst_txt_filename.c_str(), fst_txt_buf, MAX_GCN_FST_TXT_FILESIZE), 0);

	// Import the FST text into an istringstream.
	istringstream fst_text_expected(
		string(reinterpret_cast<const char*>(fst_txt_buf.data()), fst_txt_buf.size()));

	// Print the FST.bin to a new stringstream.
	stringstream fst_text_actual;
	fstPrint(m_fst, fst_text_actual);

	// Compare the two stringstreams.
	// NOTE: Only Unix line endings are supported.
	char line_buf_actual[1024], line_buf_expected[1024];
	for (int line_num = 1; /* condition is defined in the loop */; line_num++) {
		bool ok_actual = (bool)fst_text_actual.getline(line_buf_actual, sizeof(line_buf_actual), '\n');
		bool ok_expected = (bool)fst_text_expected.getline(line_buf_expected, sizeof(line_buf_expected), '\n');
		if (!ok_actual && !ok_expected) {
			// End of both files.
			break;
		}

		EXPECT_TRUE(ok_actual) << "Unexpected EOF in FST text generated by fstPrint().";
		EXPECT_TRUE(ok_expected) << "Unexpected EOF in FST text from '" << fst_txt_filename << "'.";
		if (!ok_actual || !ok_expected) {
			// End of one of the files.
			break;
		}

		// Compare the two strings.
		// TODO: Eliminate one of the comparisons?
		EXPECT_STREQ(line_buf_expected, line_buf_actual) <<
			"Line " << line_num << " differs between fstPrint() and '" << fst_txt_filename << "'.";
		if (strcmp(line_buf_expected, line_buf_actual) != 0) {
			// Lines don't match.
			break;
		}
	};
	EXPECT_FALSE(m_fst->hasErrors());
}

/** Test case parameters. **/

/**
 * Get the list of FST files from a Zip file.
 * @param offsetShift File offset shift. (0 == GCN, 2 == Wii)
 * @return FST files.
 */
std::vector<GcnFstTest_mode> GcnFstTest::ReadTestCasesFromDisk(uint8_t offsetShift)
{
	// NOTE: Cannot use ASSERT_TRUE() here.
	std::vector<GcnFstTest_mode> files;

	// Open the Zip file.
	// NOTE: MiniZip 2.2.3's compatibility functions
	// take UTF-8 on Windows, not UTF-16.
	const char *zip_filename;
	switch (offsetShift) {
		case 0:
			zip_filename = "GameCube.fst.bin.zip";
			break;
		case 2:
			zip_filename = "Wii.fst.bin.zip";
			break;
		default:
			EXPECT_TRUE(false) << "offsetShift is " << (int)offsetShift << "; should be either 0 or 2.";
			return files;
	}

	unzFile unz = openZip(zip_filename);
	EXPECT_TRUE(unz != nullptr) <<
		"Could not open '" << zip_filename << "', check the test directory!";
	if (!unz) {
		return files;
	}

	// MiniZip 2.x (up to 2.2.3) doesn't automatically go to the first file.
	// Hence, we'll need to do that here.
	int ret = unzGoToFirstFile(unz);
	EXPECT_EQ(0, ret) << "unzGoToFirstFile failed in '" << zip_filename << "'.";
	if (ret != 0) {
		unzClose(unz);
		return files;
	}

	// Read the filenames.
	char filename[256];
	unz_file_info64 file_info;
	do {
		ret = unzGetCurrentFileInfo64(unz,
			&file_info,
			filename, sizeof(filename),
			nullptr, 0,
			nullptr, 0);
		if (ret != UNZ_OK)
			break;

		// Make sure the filename isn't empty.
		EXPECT_GT(file_info.size_filename, 0) << "A filename in the ZIP file has no name. Skipping...";

		// Make sure the file isn't too big.
		EXPECT_LE(file_info.uncompressed_size, MAX_GCN_FST_BIN_FILESIZE) <<
			"GCN FST file '" << filename << "' is too big. (maximum size is 1 MB)";

		if (file_info.size_filename > 0 &&
		    file_info.uncompressed_size <= MAX_GCN_FST_BIN_FILESIZE)
		{
			// Add this filename to the list.
			// NOTE: Filename might not be NULL-terminated,
			// so use the explicit length.
			GcnFstTest_mode mode(string(filename, file_info.size_filename), offsetShift);
			files.emplace_back(std::move(mode));
		}

		// Next file.
		ret = unzGoToNextFile(unz);
	} while (ret == UNZ_OK);
	unzClose(unz);

	// Handle "end of file list" as OK.
	if (ret == UNZ_END_OF_LIST_OF_FILE)
		ret = UNZ_OK;

	EXPECT_EQ(UNZ_OK, ret);
	EXPECT_GT(files.size(), 0U);
	return files;
}

/**
 * Test case suffix generator.
 * @param info Test parameter information.
 * @return Test case suffix.
 */
string GcnFstTest::test_case_suffix_generator(const ::testing::TestParamInfo<GcnFstTest_mode> &info)
{
	string suffix = info.param.fst_filename;

	// Replace all non-alphanumeric characters with '_'.
	// See gtest-param-util.h::IsValidParamName().
	std::replace_if(suffix.begin(), suffix.end(),
		[](char c) noexcept -> bool { return !ISALNUM(c); }, '_');

	return suffix;
}

INSTANTIATE_TEST_SUITE_P(GameCube, GcnFstTest,
	testing::ValuesIn(GcnFstTest::ReadTestCasesFromDisk(0))
	, GcnFstTest::test_case_suffix_generator);

INSTANTIATE_TEST_SUITE_P(Wii, GcnFstTest,
	testing::ValuesIn(GcnFstTest::ReadTestCasesFromDisk(2))
	, GcnFstTest::test_case_suffix_generator);

} }

extern "C" int gtest_main(int argc, TCHAR *argv[])
{
	fprintf(stderr, "LibRomData test suite: GcnFst tests.\n\n");
	fflush(nullptr);

	// Make sure the CRC32 table is initialized.
	get_crc_table();

#ifdef _WIN32
	// Check for the fst_data directory and chdir() into it.
	static const TCHAR *const subdirs[] = {
		_T("fst_data"),
		_T("bin\\fst_data"),
		_T("src\\libromdata\\tests\\disc\\fst_data"),
		_T("..\\src\\libromdata\\tests\\disc\\fst_data"),
		_T("..\\..\\src\\libromdata\\tests\\disc\\fst_data"),
		_T("..\\..\\..\\src\\libromdata\\tests\\disc\\fst_data"),
		_T("..\\..\\..\\..\\src\\libromdata\\tests\\disc\\fst_data"),
		_T("..\\..\\..\\..\\..\\src\\libromdata\\tests\\disc\\fst_data"),
		_T("..\\..\\..\\bin\\fst_data"),
		_T("..\\..\\..\\bin\\Debug\\fst_data"),
		_T("..\\..\\..\\bin\\Release\\fst_data"),
	};
#else /* !_WIN32 */
	static const TCHAR *const subdirs[] = {
		_T("fst_data"),
		_T("bin/fst_data"),
		_T("src/libromdata/tests/disc/fst_data"),
		_T("../src/libromdata/tests/disc/fst_data"),
		_T("../../src/libromdata/tests/disc/fst_data"),
		_T("../../../src/libromdata/tests/disc/fst_data"),
		_T("../../../../src/libromdata/tests/disc/fst_data"),
		_T("../../../../../src/libromdata/tests/disc/fst_data"),
		_T("../../../bin/fst_data"),
	};
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
		fputs("*** ERROR: Cannot find the fst_data test data directory.\n", stderr);
		return EXIT_FAILURE;
	}

	// coverity[fun_call_w_exception]: uncaught exceptions cause nonzero exit anyway, so don't warn.
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
