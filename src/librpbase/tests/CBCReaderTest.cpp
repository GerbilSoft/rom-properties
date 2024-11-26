/***************************************************************************
 * ROM Properties Page shell extension. (librpbase/tests)                  *
 * CBCReaderTest.cpp: CBCReader class test.                                *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "config.librpbase.h"

// Google Test
#include "gtest/gtest.h"
#include "tcharx.h"

// CBCReader
#include "../disc/CBCReader.hpp"

// librpfile
#include "librpfile/MemFile.hpp"
using LibRpFile::IRpFilePtr;
using LibRpFile::MemFile;

// C includes (C++ namespace)
#include <cstdio>

// C++ includes
#include <array>
#include <sstream>
#include <string>
using std::array;
using std::ostringstream;
using std::string;

namespace LibRpBase { namespace Tests {

enum class CryptoMode {
	Passthru,	// No encryption
#ifdef ENABLE_DECRYPTION
	ECB,
	CBC,
#endif /* ENABLE_DECRYPTION */
};

struct CBCReaderTest_mode
{
	// Block crypto mode
	CryptoMode cryptoMode;

	CBCReaderTest_mode(CryptoMode cryptoMode)
		: cryptoMode(cryptoMode)
	{}
};

/**
 * Formatting function for CBCReaderTest_mode.
 */
inline ::std::ostream& operator<<(::std::ostream& os, const CBCReaderTest_mode& mode)
{
	switch (mode.cryptoMode) {
		case CryptoMode::Passthru:
			return os << "Passthru";
#ifdef ENABLE_DECRYPTION
		case CryptoMode::ECB:
			return os << "ECB";
		case CryptoMode::CBC:
			return os << "CBC";
#endif /* ENABLE_DECRYPTION */
		default:
			assert(!"Invalid CryptoMode");
			return os << "INVALID";
	}
};

class CBCReaderTest : public ::testing::TestWithParam<CBCReaderTest_mode>
{
protected:
	void SetUp(void) final;
	void TearDown(void) final;

public:
	CBCReaderPtr m_cbcReader;
	IRpFilePtr m_memFile;
	
public:
	static const char plaintext[64];

#ifdef ENABLE_DECRYPTION
	static const array<uint8_t, 64> ciphertext_ecb;
	static const array<uint8_t, 64> ciphertext_cbc;

	static const array<uint8_t, 16> aes_key;
	static const array<uint8_t, 16> aes_iv;
#endif /* ENABLE_DECRYPTION */

	/**
	 * Compare two byte arrays.
	 * The byte arrays are converted to hexdumps and then
	 * compared using EXPECT_EQ().
	 * @param expected	[in] Expected data.
	 * @param actual	[in] Actual data.
	 * @param size		[in] Size of both arrays.
	 * @param data_type	[in] Data type.
	 */
	void CompareByteArrays(
		const uint8_t *expected,
		const uint8_t *actual,
		size_t size,
		const char *data_type);

public:
	/** Test case parameters **/

	/**
	 * Test case suffix generator.
	 * @param info Test parameter information.
	 * @return Test case suffix.
	 */
	static string test_case_suffix_generator(const ::testing::TestParamInfo<CBCReaderTest_mode> &info);
};

/**
 * Test case suffix generator.
 * @param info Test parameter information.
 * @return Test case suffix.
 */
string CBCReaderTest::test_case_suffix_generator(const ::testing::TestParamInfo<CBCReaderTest_mode> &info)
{
	switch (info.param.cryptoMode) {
		case CryptoMode::Passthru:
			return "Passthru";
#ifdef ENABLE_DECRYPTION
		case CryptoMode::ECB:
			return "ECB";
		case CryptoMode::CBC:
			return "CBC";
#endif /* ENABLE_DECRYPTION */
		default:
			assert(!"Invalid CryptoMode");
			return "INVALID";
	}
}

const char CBCReaderTest::plaintext[64] =
	"This is a test string. It should be encrypted and decrypted! =P";

#ifdef ENABLE_DECRYPTION
const array<uint8_t, 64> CBCReaderTest::ciphertext_ecb = {{
	0xC7,0xE9,0x48,0x3D,0xF6,0x9F,0x50,0xFA,
	0x4A,0xF5,0x7E,0x62,0x5F,0x48,0xE8,0xC9,
	0x7C,0x01,0x3E,0xE8,0x2A,0x9D,0x25,0x15,
	0x64,0xFA,0x59,0xA6,0xCF,0xBD,0x85,0xBA,
	0x46,0x5F,0x61,0x36,0x09,0x73,0xF3,0x0C,
	0x46,0x7B,0x84,0x60,0x40,0xB2,0xC8,0x20,
	0xCC,0xB2,0xCD,0xA8,0xBE,0xC2,0x6A,0xF3,
	0x7F,0x4A,0x14,0x41,0xC9,0xA3,0x45,0x03
}};

const array<uint8_t, 64> CBCReaderTest::ciphertext_cbc = {{
	0xD4,0x71,0xDF,0xDE,0x04,0xE7,0x0A,0x67,
	0x2B,0xD4,0x82,0x4B,0xD1,0x10,0x71,0x62,
	0xE9,0x09,0x49,0x5D,0x3D,0xAE,0x4C,0xBC,
	0x0C,0x6F,0x3A,0xBE,0x32,0x78,0x39,0xF3,
	0x33,0x07,0x94,0xAF,0xFE,0xF0,0xB4,0xF3,
	0xA5,0x3E,0xFB,0x22,0xA8,0x33,0xFA,0x02,
	0xB8,0x73,0x44,0xF5,0xDC,0x78,0xDA,0x9A,
	0xD4,0xB5,0x8C,0x17,0xEF,0x59,0xB2,0xBF
}};

const array<uint8_t, 16> CBCReaderTest::aes_key = {{
	0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF,
	0xFE,0xDC,0xBA,0x98,0x76,0x54,0x32,0x10,
}};

const array<uint8_t, 16> CBCReaderTest::aes_iv = {{
	0xD9,0x83,0xC2,0xA0,0x1C,0xFA,0x8B,0x88,
	0x3A,0xE3,0xA4,0xBD,0x70,0x1F,0xC1,0x0B
}};
#endif /* ENABLE_DECRYPTION */

/**
 * Compare two byte arrays.
 * The byte arrays are converted to hexdumps and then
 * compared using EXPECT_EQ().
 * @param expected	[in] Expected data.
 * @param actual	[in] Actual data.
 * @param size		[in] Size of both arrays.
 * @param data_type	[in] Data type.
 */
void CBCReaderTest::CompareByteArrays(
	const uint8_t *expected,
	const uint8_t *actual,
	size_t size,
	const char *data_type)
{
	// Output format: (assume ~64 bytes per line)
	// 0000: 01 23 45 67 89 AB CD EF  01 23 45 67 89 AB CD EF
	const size_t bufSize = ((size / 16) + !!(size % 16)) * 64;
	char printf_buf[16];
	string s_expected, s_actual;
	s_expected.reserve(bufSize);
	s_actual.reserve(bufSize);

	const uint8_t *pE = expected, *pA = actual;
	for (size_t i = 0; i < size; i++, pE++, pA++) {
		if (i % 16 == 0) {
			// New line.
			if (i > 0) {
				// Append newlines.
				s_expected += '\n';
				s_actual += '\n';
			}

			snprintf(printf_buf, sizeof(printf_buf), "%04X: ", static_cast<unsigned int>(i));
			s_expected += printf_buf;
			s_actual += printf_buf;
		}

		// Print the byte.
		snprintf(printf_buf, sizeof(printf_buf), "%02X", *pE);
		s_expected += printf_buf;
		snprintf(printf_buf, sizeof(printf_buf), "%02X", *pA);
		s_actual += printf_buf;

		if (i % 16 == 7) {
			s_expected += "  ";
			s_actual += "  ";
		} else if (i % 16  < 15) {
			s_expected += ' ';
			s_actual += ' ';
		}
	}

	// Compare the byte arrays, and
	// print the strings on failure.
	EXPECT_EQ(0, memcmp(expected, actual, size)) <<
		"Expected " << data_type << ":" << '\n' << s_expected << '\n' <<
		"Actual " << data_type << ":" << '\n' << s_actual << '\n';
}

/**
 * SetUp() function.
 * Run before each test.
 */
void CBCReaderTest::SetUp(void)
{
	const CBCReaderTest_mode &mode = GetParam();

	switch (mode.cryptoMode) {
		case CryptoMode::Passthru:
			m_memFile = std::make_shared<MemFile>(plaintext, sizeof(plaintext));
			m_cbcReader = std::make_shared<CBCReader>(m_memFile, 0, sizeof(plaintext), nullptr, nullptr);
			break;

#ifdef ENABLE_DECRYPTION
		case CryptoMode::ECB:
			m_memFile = std::make_shared<MemFile>(ciphertext_ecb.data(), ciphertext_ecb.size());
			m_cbcReader = std::make_shared<CBCReader>(m_memFile, 0, ciphertext_ecb.size(), aes_key.data(), nullptr);
			break;

		case CryptoMode::CBC:
			m_memFile = std::make_shared<MemFile>(ciphertext_cbc.data(), ciphertext_cbc.size());
			m_cbcReader = std::make_shared<CBCReader>(m_memFile, 0, ciphertext_cbc.size(), aes_key.data(), aes_iv.data());
			break;
#endif /* ENABLE_DECRYPTION */

		default:
			assert(!"Invalid CryptoMode");
			m_memFile.reset();
			m_cbcReader.reset();
			ASSERT_TRUE(false) << "Invalid CryptoMode";
	}

	ASSERT_TRUE(m_cbcReader->isOpen());
	ASSERT_EQ(64, m_cbcReader->size());
}

/**
 * TearDown() function.
 * Run after each test.
 */
void CBCReaderTest::TearDown(void)
{
	m_memFile.reset();
	m_cbcReader.reset();
}

#ifdef ENABLE_DECRYPTION
/**
 * Decrypt the full data.
 */
TEST_P(CBCReaderTest, decryptFull)
{
	array<uint8_t, sizeof(plaintext)> decrypted;
	EXPECT_EQ(decrypted.size(), m_cbcReader->read(decrypted.data(), decrypted.size()));
	EXPECT_EQ(decrypted.size(), m_cbcReader->tell());

	// Compare the decrypted data to the known plaintext.
	CompareByteArrays(reinterpret_cast<const uint8_t*>(plaintext),
		decrypted.data(), decrypted.size(), "Full data");
}

/**
 * Decrypt the first AES block.
 */
TEST_P(CBCReaderTest, decryptFirstAESBlock)
{
	array<uint8_t, 16> decrypted;
	EXPECT_EQ(decrypted.size(), m_cbcReader->read(decrypted.data(), 16));
	EXPECT_EQ(decrypted.size(), m_cbcReader->tell());

	// Compare the decrypted data to the known plaintext.
	CompareByteArrays(reinterpret_cast<const uint8_t*>(plaintext),
		decrypted.data(), decrypted.size(), "First AES block");
}

/**
 * Decrypt the last AES block.
 */
TEST_P(CBCReaderTest, decryptLastAESBlock)
{
	array<uint8_t, 16> decrypted;
	EXPECT_EQ(decrypted.size(), m_cbcReader->seekAndRead(sizeof(plaintext) - 16, decrypted.data(), 16));
	EXPECT_EQ(sizeof(plaintext), m_cbcReader->tell());

	// Compare the decrypted data to the known plaintext.
	CompareByteArrays(reinterpret_cast<const uint8_t*>(&plaintext[sizeof(plaintext) - 16]),
		decrypted.data(), decrypted.size(), "Last AES block");
}

/**
 * Decrypt bytes [0x08, 0x17].
 * This is the last 8 bytes of the first AES block,
 * and the first 8 bytes of the second AES block.
 */
TEST_P(CBCReaderTest, decryptSplitAESBlocks0and1)
{
	array<uint8_t, 16> decrypted;
	EXPECT_EQ(decrypted.size(), m_cbcReader->seekAndRead(0x08, decrypted.data(), 16));
	EXPECT_EQ(0x18, m_cbcReader->tell());

	// Compare the decrypted data to the known plaintext.
	CompareByteArrays(reinterpret_cast<const uint8_t*>(&plaintext[0x08]),
		decrypted.data(), decrypted.size(), "Split AES blocks 0 and 1");
}

/**
 * Decrypt bytes [0x38, 0x3F].
 * We'll attempt to read 16 bytes starting at 0x38.
 * We should only get 8 bytes back.
 */
TEST_P(CBCReaderTest, decryptLast8Bytes)
{
	array<uint8_t, 16> decrypted;
	decrypted.fill(0x55);

	EXPECT_EQ(8U, m_cbcReader->seekAndRead(0x38, decrypted.data(), 16));
	EXPECT_EQ(sizeof(plaintext), m_cbcReader->tell());

	// Compare the decrypted data to the known plaintext.
	CompareByteArrays(reinterpret_cast<const uint8_t*>(&plaintext[0x38]),
		decrypted.data(), 8, "Last 8 bytes");
	// Last 8 bytes should be unmodified: 0x55
	for (unsigned int i = 8; i < 16; i++) {
		EXPECT_EQ(0x55, decrypted[i]);
	}
}
#endif /* ENABLE_DECRYPTION */

#ifdef ENABLE_DECRYPTION
INSTANTIATE_TEST_SUITE_P(CBCReaderTest, CBCReaderTest,
	::testing::Values(
		CBCReaderTest_mode(CryptoMode::Passthru),
		CBCReaderTest_mode(CryptoMode::ECB),
		CBCReaderTest_mode(CryptoMode::CBC)
		)
	, CBCReaderTest::test_case_suffix_generator);
#else /* !ENABLE_DECRYPTION */
INSTANTIATE_TEST_SUITE_P(CBCReaderTest, CBCReaderTest,
	::testing::Values(
		CBCReaderTest_mode(CryptoMode::Passthru)
		)
	, CBCReaderTest::test_case_suffix_generator);
#endif /* ENABLE_DECRYPTION */

} }

/**
 * Test suite main function.
 */
extern "C" int gtest_main(int argc, TCHAR *argv[])
{
	fputs("LibRpBase test suite: CBCReader tests.\n\n", stderr);
	fflush(nullptr);

	// coverity[fun_call_w_exception]: uncaught exceptions cause nonzero exit anyway, so don't warn.
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
