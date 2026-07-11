/***************************************************************************
 * ROM Properties Page shell extension. (libromdata/tests)                 *
 * GcnFstTest.cpp: GameCube/Wii FST test.                                  *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// for HAVE_ZLIB for MiniZip
#include "config.librpbase.h"

// Google Test
#include "gtest_init.hpp"

// MiniZip
#include <zlib.h>
#include "mz.h"
#include "mz_zip.h"
#include "mz_strm.h"	// FIXME: Should be included by mz_zip_rw.h...
#include "mz_zip_rw.h"

// MiniZip-NG's native interface uses `void*` for handles.
// We'll typedef it to mzReader.
typedef void *mzReader;

// Other rom-properties libraries
#include "librpfile/FileSystem.hpp"
#include "librptext/conversion.hpp"
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

#ifdef _MSC_VER
// MSVC: Exception handling for /DELAYLOAD.
#  include "libwin32common/DelayLoadHelper.h"
#endif /* _MSC_VER */

#ifdef _MSC_VER
// DelayLoad test implementations
#  ifdef ZLIB_IS_DLL
DELAYLOAD_TEST_FUNCTION_IMPL0(get_crc_table);
#  endif /* ZLIB_IS_DLL */
#  ifdef MINIZIP_IS_DLL
// mz_zip_delete() can safely take nullptr; it won't do anything.
DELAYLOAD_TEST_FUNCTION_IMPL1(mz_zip_delete, nullptr);
#  endif /* MINIZIP_IS_DLL */
#endif /* _MSC_VER */

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
static constexpr off64_t MAX_GCN_FST_BIN_FILESIZE = 1024L*1024L;	// 1.0 MB
static constexpr off64_t MAX_GCN_FST_TXT_FILESIZE = 1536L*1024L;	// 1.5 MB

class GcnFstTest : public ::testing::TestWithParam<GcnFstTest_mode>
{
protected:
	GcnFstTest()
		: ::testing::TestWithParam<GcnFstTest_mode>()
		, m_fst(nullptr)
	{}

	void SetUp(void) final;
	void TearDown(void) final;

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
		off64_t max_filesize = MAX_GCN_FST_BIN_FILESIZE);

public:
	// FST data
	rp::uvector<uint8_t> m_fst_buf;
	IFst *m_fst;

	/**
	 * Recursively check a subdirectory for duplicate filenames.
	 * @param subdir Subdirectory path.
	 */
	void checkNoDuplicateFilenames(const char *subdir);

public:
	/** Test case parameters **/

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
			ASSERT_TRUE(false) << "offsetShift is " << static_cast<int>(mode.offsetShift) << "; should be either 0 or 2.";
	}

	ASSERT_GT(getFileFromZip(zip_filename, mode.fst_filename.c_str(), m_fst_buf, MAX_GCN_FST_BIN_FILESIZE), 0);

	// Check for NKit FST recovery data.
	// These FSTs have an extra header at the top, indicating what
	// disc the FST belongs to.
	static const off64_t NKIT_FST_HEADER_SIZE = 0x50;
	unsigned int fst_start_offset = 0;
	static constexpr array<uint8_t, 10> root_dir_data = {{1,0,0,0,0,0,0,0,0,0}};
	if (m_fst_buf.size() >= 0x60) {
		if (!memcmp(&m_fst_buf[NKIT_FST_HEADER_SIZE], root_dir_data.data(), root_dir_data.size())) {
			// Found an NKit FST.
			fst_start_offset = NKIT_FST_HEADER_SIZE;
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
	off64_t max_filesize)
{
	// Open the Zip file.
	mzReader zipReader = mz_zip_reader_create();
	EXPECT_TRUE(zipReader != nullptr) << "Could not create a MiniZip-NG ZIP reader instance!";
	if (!zipReader) {
		return -1;
	}
	int ret = mz_zip_reader_open_file(zipReader, zip_filename);
	EXPECT_TRUE(ret == MZ_OK) <<
		"Could not open '" << zip_filename << "', check the test directory!";
	if (ret != MZ_OK) {
		mz_zip_reader_delete(&zipReader);
		return -2;
	}

	// Locate the required FST file.
	// NOTE: Using case-sensitive lookups for performance.
	ret = mz_zip_reader_locate_entry(zipReader, int_filename, false);
	EXPECT_EQ(MZ_OK, ret);
	if (ret != MZ_OK) {
		mz_zip_reader_close(zipReader);
		mz_zip_reader_delete(&zipReader);
		return -3;
	}

	// Verify the FST file size.
	mz_zip_file *file_info;
	ret = mz_zip_reader_entry_get_info(zipReader, &file_info);
	EXPECT_EQ(MZ_OK, ret);
	if (ret != MZ_OK) {
		mz_zip_reader_close(zipReader);
		mz_zip_reader_delete(&zipReader);
		return -4;
	}
	EXPECT_TRUE(file_info != nullptr);
	if (!file_info) {
		mz_zip_reader_close(zipReader);
		mz_zip_reader_delete(&zipReader);
		return -5;
	}
	const int64_t uncompressed_size = file_info->uncompressed_size;
	EXPECT_LE(uncompressed_size, max_filesize) <<
		"Compressed file '" << int_filename << "' is too big.";
	if (uncompressed_size > max_filesize) {
		mz_zip_reader_close(zipReader);
		mz_zip_reader_delete(&zipReader);
		return -6;
	}

	// Open the FST file.
	ret = mz_zip_reader_entry_open(zipReader);
	EXPECT_EQ(MZ_OK, ret);
	if (ret != MZ_OK) {
		mz_zip_reader_close(zipReader);
		mz_zip_reader_delete(&zipReader);
		return -7;
	}

	// Read the FST file.
	// NOTE: zlib and minizip are only guaranteed to be able to
	// read UINT16_MAX (64 KB) at a time, and the updated MiniZip
	// from https://github.com/nmoinvaz/minizip enforces this.
	// TODO: Does MiniZip-NG's native API still have this limitation?
	buf.resize(static_cast<size_t>(uncompressed_size));
	uint8_t *p = buf.data();
	size_t size = buf.size();
	while (size > 0) {
		int to_read = static_cast<int>(size > UINT16_MAX ? UINT16_MAX : size);
		ret = mz_zip_reader_entry_read(zipReader, p, to_read);
		EXPECT_EQ(to_read, ret);
		if (ret != to_read) {
			mz_zip_reader_entry_close(zipReader);
			mz_zip_reader_close(zipReader);
			mz_zip_reader_delete(&zipReader);
			return -8;
		}

		// ret == number of bytes read.
		p += ret;
		size -= ret;
	}

	// Read one more byte to ensure unzReadCurrentFile() returns 0 for EOF.
	// NOTE: MiniZip will zero out tmp if there's no data available.
	uint8_t tmp;
	ret = mz_zip_reader_entry_read(zipReader, &tmp, 1);
	EXPECT_EQ(0, ret);
	if (ret != 0) {
		mz_zip_reader_entry_close(zipReader);
		mz_zip_reader_close(zipReader);
		mz_zip_reader_delete(&zipReader);
		return -9;
	}

	// Close the FST file.
	// An error will occur here if the CRC is incorrect.
	ret = mz_zip_reader_entry_close(zipReader);
	EXPECT_EQ(MZ_OK, ret);

	// Close the Zip file.
	mz_zip_reader_close(zipReader);
	mz_zip_reader_delete(&zipReader);
	// TODO: Change the return type to off64_t?
	return (ret == MZ_OK) ? static_cast<int>(uncompressed_size) : -10;
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

	const IFst::DirEnt *dirent = m_fst->readdir(dirp);
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
			ASSERT_TRUE(false) << "offsetShift is " << static_cast<int>(mode.offsetShift) << "; should be either 0 or 2.";
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
		bool ok_actual = static_cast<bool>(fst_text_actual.getline(line_buf_actual, sizeof(line_buf_actual), '\n'));
		bool ok_expected = static_cast<bool>(fst_text_expected.getline(line_buf_expected, sizeof(line_buf_expected), '\n'));
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
	// NOTE: Cannot use ASSERT_TRUE() here due to the return value.

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
			EXPECT_TRUE(false) << "offsetShift is " << static_cast<int>(offsetShift) << "; should be either 0 or 2.";
			return {};
	}

	mzReader zipReader = mz_zip_reader_create();
	EXPECT_TRUE(zipReader != nullptr) << "Could not create a MiniZip-NG ZIP reader instance!";
	if (!zipReader) {
		return {};
	}
	int ret = mz_zip_reader_open_file(zipReader, zip_filename);
	EXPECT_TRUE(ret == MZ_OK) <<
		"Could not open '" << zip_filename << "', check the test directory!";
	if (ret != MZ_OK) {
		mz_zip_reader_delete(&zipReader);
		return {};
	}

	// MiniZip 2.x (up to 2.2.3) doesn't automatically go to the first file.
	// Hence, we'll need to do that here.
	ret = mz_zip_reader_goto_first_entry(zipReader);
	EXPECT_EQ(0, ret) << "mz_zip_reader_goto_first_entry() failed in '" << zip_filename << "'.";
	if (ret != 0) {
		mz_zip_reader_close(zipReader);
		mz_zip_reader_delete(&zipReader);
		return {};
	}

	// Read the filenames.
	std::vector<GcnFstTest_mode> files;
	do {
		mz_zip_file *file_info;
		ret = mz_zip_reader_entry_get_info(zipReader, &file_info);
		if (ret != MZ_OK) {
			break;
		}

		// Make sure the filename isn't empty.
		EXPECT_GT(file_info->filename_size, 0U) << "A filename in the ZIP file has no name. Skipping...";

		// Make sure the file isn't too big.
		EXPECT_LE(file_info->uncompressed_size, MAX_GCN_FST_BIN_FILESIZE) <<
			"GCN FST file '" << file_info->filename << "' is too big. (maximum size is 1 MB)";

		if (file_info->filename_size > 0 &&
		    file_info->uncompressed_size <= MAX_GCN_FST_BIN_FILESIZE)
		{
			// Add this filename to the list.
			// NOTE: Filename might not be NULL-terminated,
			// so use the explicit length.
			files.emplace_back(string(file_info->filename, file_info->filename_size), offsetShift);
		}

		// Next file.
		ret = mz_zip_reader_goto_next_entry(zipReader);
	} while (ret == MZ_OK);
	mz_zip_reader_close(zipReader);
	mz_zip_reader_delete(&zipReader);

	// Handle "end of file list" as OK.
	if (ret == MZ_END_OF_LIST) {
		ret = MZ_OK;
	}

	EXPECT_EQ(MZ_OK, ret);
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
		[](char c) noexcept -> bool { return !isalnum_ascii(c); }, '_');

	return suffix;
}

INSTANTIATE_TEST_SUITE_P(GameCube, GcnFstTest,
	testing::ValuesIn(GcnFstTest::ReadTestCasesFromDisk(0))
	, GcnFstTest::test_case_suffix_generator);

INSTANTIATE_TEST_SUITE_P(Wii, GcnFstTest,
	testing::ValuesIn(GcnFstTest::ReadTestCasesFromDisk(2))
	, GcnFstTest::test_case_suffix_generator);

} }

#ifdef HAVE_SECCOMP
const unsigned int rp_gtest_syscall_set = 0;
#endif /* HAVE_SECCOMP */

extern "C" int gtest_main(int argc, TCHAR *argv[])
{
	fputs("LibRomData test suite: GcnFst tests.\n\n", stderr);
	fflush(nullptr);

#ifdef _MSC_VER
#  ifdef NDEBUG
#    define DEBUG_SUFFIX ""
#  else
#    define DEBUG_SUFFIX "d"
#  endif
#else
#  define DEBUG_SUFFIX ""
#endif

#ifdef _MSC_VER
	// Delay load verification.
#  ifdef ZLIB_IS_DLL
	// Only if zlib is a DLL.
	if (DelayLoad_test_get_crc_table() != 0) {
		// Delay load failed.
		fputs("*** ERROR: zlib1" DEBUG_SUFFIX ".dll not found. Cannot run tests.", stderr);
		return EXIT_FAILURE;
	}
#  else /* !ZLIB_IS_DLL */
	// zlib isn't in a DLL, but we need to ensure that the
	// CRC table is initialized anyway.
	get_crc_table();
#  endif /* ZLIB_IS_DLL */

#  ifdef MINIZIP_IS_DLL
	// Only if MiniZip is a DLL.
	if (DelayLoad_test_mz_zip_delete() != 0) {
		// Delay load failed.
		fputs("*** ERROR: minizip" DEBUG_SUFFIX ".dll not found. Cannot run tests.", stderr);
		return EXIT_FAILURE;
	}
#  endif /* MINIZIP_IS_DLL */
#endif /* _MSC_VER */

#ifdef _WIN32
	// Check for the fst_data directory and chdir() into it.
	static constexpr const TCHAR *const subdirs[] = {
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
	static constexpr const TCHAR *const subdirs[] = {
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
