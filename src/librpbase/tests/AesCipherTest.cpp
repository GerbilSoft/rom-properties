/***************************************************************************
 * ROM Properties Page shell extension. (librpbase/tests)                  *
 * AesCipherTest.cpp: AesCipher class test.                                *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Google Test
#include "gtest_init.hpp"

// AesCipher
#include "../crypto/IAesCipher.hpp"
#include "../crypto/AesCipherFactory.hpp"

// C includes (C++ namespace)
#include <cstdio>

// C++ includes
#include <array>
#include <sstream>
#include <string>
#include <vector>
using std::array;
using std::string;
using std::vector;

// libfmt
#include "rp-libfmt.h"

namespace LibRpBase { namespace Tests {

typedef IAesCipher* (*pfnCreateIAesCipher_t)(void);

struct AesCipherTest_mode
{
	// Cipher class.
	pfnCreateIAesCipher_t pfnCreateIAesCipher;
	bool isRequired;	// True if this test is required to pass.

	// Cipher settings.
	IAesCipher::ChainingMode chainingMode;
	size_t key_len;

	// Cipher text.
	const uint8_t *cipherText;	// Cipher text.
	size_t cipherText_len;		// Cipher text length, in bytes.

	AesCipherTest_mode(
		pfnCreateIAesCipher_t pfnCreateIAesCipher,
		bool isRequired,
		IAesCipher::ChainingMode chainingMode,
		size_t key_len,
		const uint8_t *cipherText, size_t cipherText_len)
		: pfnCreateIAesCipher(pfnCreateIAesCipher)
		, isRequired(isRequired)
		, chainingMode(chainingMode)
		, key_len(key_len)
		, cipherText(cipherText), cipherText_len(cipherText_len)
	{ }
};

/**
 * Formatting function for AesCipherTest_mode.
 */
inline ::std::ostream& operator<<(::std::ostream& os, const AesCipherTest_mode& mode)
{
	const char *cm_str;
	switch (mode.chainingMode) {
		case IAesCipher::ChainingMode::ECB:
			cm_str = "ECB";
			break;
		case IAesCipher::ChainingMode::CBC:
			cm_str = "CBC";
			break;
		case IAesCipher::ChainingMode::CTR:
			cm_str = "CTR";
			break;
		default:
			cm_str = "UNK";
			break;
	}

	return os << "AES-" << (mode.key_len * 8) << "-" << cm_str;
};

class AesCipherTest : public ::testing::TestWithParam<AesCipherTest_mode>
{
	protected:
		AesCipherTest()
			: m_cipher(nullptr) { }

		void SetUp(void) final;
		void TearDown(void) final;

	public:
		IAesCipher *m_cipher;

		// AES-256 encryption key.
		// AES-128 and AES-192 use the first
		// 16 and 24 bytes of this key.
		static const array<uint8_t, 32> aes_key;

		// IV for AES-CBC.
		static const array<uint8_t, 16> aes_iv;

		// Test string.
		static const char test_string[64];

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
		/** Test case parameters. **/

		/**
		 * Test case suffix generator.
		 * @param info Test parameter information.
		 * @return Test case suffix.
		 */
		static string test_case_suffix_generator(const ::testing::TestParamInfo<AesCipherTest_mode> &info);
};

// AES-256 encryption key.
// AES-128 and AES-192 use the first
// 16 and 24 bytes of this key.
const array<uint8_t, 32> AesCipherTest::aes_key = {{
	0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF,
	0xFE,0xDC,0xBA,0x98,0x76,0x54,0x32,0x10,
	0x10,0x32,0x54,0x76,0x98,0xBA,0xDC,0xFE,
	0xEF,0xCD,0xAB,0x89,0x67,0x45,0x23,0x01
}};

// IV for AES-CBC.
const array<uint8_t, 16> AesCipherTest::aes_iv = {{
	0xD9,0x83,0xC2,0xA0,0x1C,0xFA,0x8B,0x88,
	0x3A,0xE3,0xA4,0xBD,0x70,0x1F,0xC1,0x0B
}};

// Test string.
const char AesCipherTest::test_string[64] =
	"This is a test string. It should be encrypted and decrypted! =P";

/**
 * Compare two byte arrays.
 * The byte arrays are converted to hexdumps and then
 * compared using EXPECT_EQ().
 * @param expected	[in] Expected data.
 * @param actual	[in] Actual data.
 * @param size		[in] Size of both arrays.
 * @param data_type	[in] Data type.
 */
void AesCipherTest::CompareByteArrays(
	const uint8_t *expected,
	const uint8_t *actual,
	size_t size,
	const char *data_type)
{
	// Output format: (assume ~64 bytes per line)
	// 0000: 01 23 45 67 89 AB CD EF  01 23 45 67 89 AB CD EF
	const size_t bufSize = ((size / 16) + !!(size % 16)) * 64;
	string s_expected, s_actual;
	s_expected.reserve(bufSize);
	s_actual.reserve(bufSize);

	string s_tmp;
	s_tmp.reserve(14);

	const uint8_t *pE = expected, *pA = actual;
	for (size_t i = 0; i < size; i++, pE++, pA++) {
		if (i % 16 == 0) {
			// New line.
			if (i > 0) {
				// Append newlines.
				s_expected += '\n';
				s_actual += '\n';
			}

			s_tmp = fmt::format(FSTR("{:0>4X}:  "), static_cast<unsigned int>(i));
			s_expected += s_tmp;
			s_actual += s_tmp;
		}

		// Print the byte.
		s_expected += fmt::format(FSTR("{:0>2X}"), *pE);
		s_actual   += fmt::format(FSTR("{:0>2X}"), *pA);

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
void AesCipherTest::SetUp(void)
{
	const AesCipherTest_mode &mode = GetParam();
	m_cipher = mode.pfnCreateIAesCipher();
	ASSERT_TRUE(m_cipher != nullptr);

	static pfnCreateIAesCipher_t pfnLastCreateIAesCipher = nullptr;
	if (pfnLastCreateIAesCipher != mode.pfnCreateIAesCipher) {
		// Print the AesCipher implementation name.
		const char *name = m_cipher->name();
		ASSERT_TRUE(name != nullptr);
		fmt::print(FSTR("AesCipher implementation: {:s}\n"), name);
		pfnLastCreateIAesCipher = mode.pfnCreateIAesCipher;

		if (!mode.isRequired && !m_cipher->isInit()) {
			fmt::print(FSTR("This implementation is not supported on this system; skipping tests.\n"));
		}
	}

	if (mode.isRequired) {
		ASSERT_TRUE(m_cipher->isInit());
	}
}

/**
 * TearDown() function.
 * Run after each test.
 */
void AesCipherTest::TearDown(void)
{
	delete m_cipher;
	m_cipher = nullptr;
}

/**
 * Run an AesCipher decryption test.
 * setIV() is called first, then the data is decrypted.
 *
 * This version sets the key before the chaining mode.
 */
TEST_P(AesCipherTest, decryptTest_setIV_keyThenChaining)
{
	const AesCipherTest_mode &mode = GetParam();
	ASSERT_TRUE(mode.key_len == 16 || mode.key_len == 24 || mode.key_len == 32);

	if (!mode.isRequired && !m_cipher->isInit()) {
		return;
	}

	// Set the cipher settings.
	EXPECT_EQ(0, m_cipher->setKey(aes_key.data(), mode.key_len));
	EXPECT_EQ(0, m_cipher->setChainingMode(mode.chainingMode));

	switch (mode.chainingMode) {
		case IAesCipher::ChainingMode::CBC:
		case IAesCipher::ChainingMode::CTR:
			// CBC requires an initialization vector.
			// CTR requires an initial counter value.
			EXPECT_EQ(0, m_cipher->setIV(aes_iv.data(), aes_iv.size()));
			break;

		case IAesCipher::ChainingMode::ECB:
		default:
			// ECB doesn't use an initialization vector.
			// setIV() should fail.
			EXPECT_NE(0, m_cipher->setIV(aes_iv.data(), aes_iv.size()));
			break;
	}

	// Decrypt the data.
	vector<uint8_t> buf(mode.cipherText, mode.cipherText + mode.cipherText_len);
	EXPECT_EQ(buf.size(), m_cipher->decrypt(buf.data(), buf.size()));

	// Compare the buffer to the known plaintext.
	CompareByteArrays(reinterpret_cast<const uint8_t*>(test_string),
		buf.data(), buf.size(), "plaintext data");
}

/**
 * Run an AesCipher decryption test.
 * setIV() is called first, then the data is decrypted.
 *
 * This version sets the chaining mode before the key.
 */
TEST_P(AesCipherTest, decryptTest_setIV_chainingThenKey)
{
	const AesCipherTest_mode &mode = GetParam();
	ASSERT_TRUE(mode.key_len == 16 || mode.key_len == 24 || mode.key_len == 32);

	if (!mode.isRequired && !m_cipher->isInit()) {
		return;
	}

	// Set the cipher settings.
	EXPECT_EQ(0, m_cipher->setChainingMode(mode.chainingMode));
	EXPECT_EQ(0, m_cipher->setKey(aes_key.data(), mode.key_len));

	switch (mode.chainingMode) {
		case IAesCipher::ChainingMode::CBC:
		case IAesCipher::ChainingMode::CTR:
			// CBC requires an initialization vector.
			// CTR requires an initial counter value.
			EXPECT_EQ(0, m_cipher->setIV(aes_iv.data(), aes_iv.size()));
			break;

		case IAesCipher::ChainingMode::ECB:
		default:
			// ECB doesn't use an initialization vector.
			// setIV() should fail.
			EXPECT_NE(0, m_cipher->setIV(aes_iv.data(), aes_iv.size()));
			break;
	}

	// Decrypt the data.
	vector<uint8_t> buf(mode.cipherText, mode.cipherText + mode.cipherText_len);
	EXPECT_EQ(buf.size(), m_cipher->decrypt(buf.data(), buf.size()));

	// Compare the buffer to the known plaintext.
	CompareByteArrays(reinterpret_cast<const uint8_t*>(test_string),
		buf.data(), buf.size(), "plaintext data");
}

/**
 * Run an AesCipher decryption test.
 * setIV() is called first, then the data is decrypted.
 *
 * This version is similar to chainingThenKey, but it decrypts
 * one 16-byte block at a time in order to test IV chaining
 * when using CBC and CTR.
 */
TEST_P(AesCipherTest, decryptTest_setIV_blockAtATime)
{
	const AesCipherTest_mode &mode = GetParam();
	ASSERT_TRUE(mode.key_len == 16 || mode.key_len == 24 || mode.key_len == 32);

	if (!mode.isRequired && !m_cipher->isInit()) {
		return;
	}

	// Set the cipher settings.
	EXPECT_EQ(0, m_cipher->setChainingMode(mode.chainingMode));
	EXPECT_EQ(0, m_cipher->setKey(aes_key.data(), mode.key_len));

	switch (mode.chainingMode) {
		case IAesCipher::ChainingMode::CBC:
		case IAesCipher::ChainingMode::CTR:
			// CBC requires an initialization vector.
			// CTR requires an initial counter value.
			EXPECT_EQ(0, m_cipher->setIV(aes_iv.data(), aes_iv.size()));
			break;

		case IAesCipher::ChainingMode::ECB:
		default:
			// ECB doesn't use an initialization vector.
			// setIV() should fail.
			EXPECT_NE(0, m_cipher->setIV(aes_iv.data(), aes_iv.size()));
			break;
	}

	// Decrypt one 16-byte block at a time.
	vector<uint8_t> buf(mode.cipherText, mode.cipherText + mode.cipherText_len);
	for (size_t i = 0; i < buf.size(); i += 16U) {
		EXPECT_EQ(16U, m_cipher->decrypt(&buf[i], 16U));
	}

	// Compare the buffer to the known plaintext.
	CompareByteArrays(reinterpret_cast<const uint8_t*>(test_string),
		buf.data(), buf.size(), "plaintext data");
}

/**
 * Run an AesCipher decryption test.
 * The four-parameter decrypt() function is used first,
 * which sets the IV. (ECB is not tested here.)
 *
 * This version sets the key before the chaining mode.
 */
TEST_P(AesCipherTest, decryptTest_fourParam_keyThenChaining)
{
	const AesCipherTest_mode &mode = GetParam();
	ASSERT_TRUE(mode.key_len == 16 || mode.key_len == 24 || mode.key_len == 32);

	if (!mode.isRequired && !m_cipher->isInit()) {
		return;
	}

	// Set the cipher settings.
	EXPECT_EQ(0, m_cipher->setKey(aes_key.data(), mode.key_len));
	EXPECT_EQ(0, m_cipher->setChainingMode(mode.chainingMode));

	switch (mode.chainingMode) {
		case IAesCipher::ChainingMode::CBC:
		case IAesCipher::ChainingMode::CTR:
			break;

		case IAesCipher::ChainingMode::ECB:
		default:
			// Not supported here.
			return;
	}

	// Decrypt the data.
	vector<uint8_t> buf(mode.cipherText, mode.cipherText + mode.cipherText_len);
	EXPECT_EQ(buf.size(), m_cipher->decrypt(buf.data(), buf.size(), aes_iv.data(), aes_iv.size()));

	// Compare the buffer to the known plaintext.
	CompareByteArrays(reinterpret_cast<const uint8_t*>(test_string),
		buf.data(), buf.size(), "plaintext data");
}

/**
 * Run an AesCipher decryption test.
 * The four-parameter decrypt() function is used first,
 * which sets the IV. (ECB is not tested here.)
 *
 * This version sets the chaining mode before the key.
 */
TEST_P(AesCipherTest, decryptTest_fourParam_chainingThenKey)
{
	const AesCipherTest_mode &mode = GetParam();
	ASSERT_TRUE(mode.key_len == 16 || mode.key_len == 24 || mode.key_len == 32);

	if (!mode.isRequired && !m_cipher->isInit()) {
		return;
	}

	// Set the cipher settings.
	EXPECT_EQ(0, m_cipher->setChainingMode(mode.chainingMode));
	EXPECT_EQ(0, m_cipher->setKey(aes_key.data(), mode.key_len));

	switch (mode.chainingMode) {
		case IAesCipher::ChainingMode::CBC:
		case IAesCipher::ChainingMode::CTR:
			break;

		case IAesCipher::ChainingMode::ECB:
		default:
			// Not supported here.
			return;
	}

	// Decrypt the data.
	vector<uint8_t> buf(mode.cipherText, mode.cipherText + mode.cipherText_len);
	EXPECT_EQ(buf.size(), m_cipher->decrypt(buf.data(), buf.size(), aes_iv.data(), aes_iv.size()));

	// Compare the buffer to the known plaintext.
	CompareByteArrays(reinterpret_cast<const uint8_t*>(test_string),
		buf.data(), buf.size(), "plaintext data");
}

/**
 * Run an AesCipher decryption test.
 * The four-parameter decrypt() function is used first,
 * which sets the IV. (ECB is not tested here.)
 *
 * This version is similar to chainingThenKey, but it decrypts
 * one 16-byte block at a time in order to test IV chaining
 * when using CBC and CTR.
 */
TEST_P(AesCipherTest, decryptTest_fourParam_blockAtATime)
{
	const AesCipherTest_mode &mode = GetParam();
	ASSERT_TRUE(mode.key_len == 16 || mode.key_len == 24 || mode.key_len == 32);

	if (!mode.isRequired && !m_cipher->isInit()) {
		return;
	}

	// Set the cipher settings.
	EXPECT_EQ(0, m_cipher->setChainingMode(mode.chainingMode));
	EXPECT_EQ(0, m_cipher->setKey(aes_key.data(), mode.key_len));

	switch (mode.chainingMode) {
		case IAesCipher::ChainingMode::CBC:
		case IAesCipher::ChainingMode::CTR:
			break;

		case IAesCipher::ChainingMode::ECB:
		default:
			// Not supported here.
			return;
	}

	// Decrypt one 16-byte block at a time.
	vector<uint8_t> buf(mode.cipherText, mode.cipherText + mode.cipherText_len);
	EXPECT_EQ(16U, m_cipher->decrypt(&buf[0], 16, aes_iv.data(), aes_iv.size()));
	for (size_t i = 16U; i < buf.size(); i += 16U) {
		EXPECT_EQ(16U, m_cipher->decrypt(&buf[i], 16U));
	}

	// Compare the buffer to the known plaintext.
	CompareByteArrays(reinterpret_cast<const uint8_t*>(test_string),
		buf.data(), buf.size(), "plaintext data");
}

/** Decryption tests. **/

/**
 * Test case suffix generator.
 * @param info Test parameter information.
 * @return Test case suffix.
 */
string AesCipherTest::test_case_suffix_generator(const ::testing::TestParamInfo<AesCipherTest_mode> &info)
{
	const char *cm_str;
	switch (info.param.chainingMode) {
		case IAesCipher::ChainingMode::ECB:
			cm_str = "ECB";
			break;
		case IAesCipher::ChainingMode::CBC:
			cm_str = "CBC";
			break;
		case IAesCipher::ChainingMode::CTR:
			cm_str = "CTR";
			break;
		default:
			cm_str = "UNK";
			break;
	}

	return fmt::format(FSTR("AES_{:d}_{:s}"), (info.param.key_len * 8), cm_str);
}

static constexpr array<uint8_t, 64> aes128ecb_ciphertext = {{
	0xC7,0xE9,0x48,0x3D,0xF6,0x9F,0x50,0xFA,
	0x4A,0xF5,0x7E,0x62,0x5F,0x48,0xE8,0xC9,
	0x7C,0x01,0x3E,0xE8,0x2A,0x9D,0x25,0x15,
	0x64,0xFA,0x59,0xA6,0xCF,0xBD,0x85,0xBA,
	0x46,0x5F,0x61,0x36,0x09,0x73,0xF3,0x0C,
	0x46,0x7B,0x84,0x60,0x40,0xB2,0xC8,0x20,
	0xCC,0xB2,0xCD,0xA8,0xBE,0xC2,0x6A,0xF3,
	0x7F,0x4A,0x14,0x41,0xC9,0xA3,0x45,0x03
}};

static constexpr array<uint8_t, 64> aes192ecb_ciphertext = {{
	0xEC,0x90,0x1B,0x32,0x20,0xC2,0xD0,0x78,
	0xA0,0x43,0xA6,0xE5,0x13,0xE1,0xF6,0x6C,
	0xE6,0x25,0x4A,0x4D,0x8C,0xF1,0x02,0xE8,
	0x63,0x40,0xFF,0x94,0x00,0x62,0x7B,0x4E,
	0xEF,0x73,0x76,0xD5,0x44,0xE5,0x96,0x94,
	0x26,0x78,0xF5,0x6D,0x96,0x20,0x6B,0xB1,
	0x78,0xC9,0x23,0x04,0xA0,0x03,0x77,0xC6,
	0xC2,0x69,0x8E,0xE5,0xDE,0xBB,0x73,0x27
}};

static constexpr array<uint8_t, 64> aes256ecb_ciphertext = {{
	0xF0,0x70,0x5F,0xFC,0x15,0x55,0x5A,0x7E,
	0x7C,0xAF,0xDA,0x82,0x12,0x6A,0x69,0x5E,
	0x20,0x55,0xD1,0x8E,0xC3,0x53,0xD1,0xF7,
	0xB3,0xC0,0xC5,0xFD,0x17,0x2E,0x39,0x30,
	0x4A,0x4A,0x68,0x84,0x6F,0xF0,0xE9,0xB2,
	0x0D,0x1C,0xE8,0xD0,0xF7,0x8B,0x22,0xEF,
	0x70,0xFA,0x81,0x71,0x5D,0x6B,0x9A,0x40,
	0x81,0xFC,0xB9,0xF5,0xBB,0x4F,0x3D,0x7C
}};

static constexpr array<uint8_t, 64> aes128cbc_ciphertext = {{
	0xD4,0x71,0xDF,0xDE,0x04,0xE7,0x0A,0x67,
	0x2B,0xD4,0x82,0x4B,0xD1,0x10,0x71,0x62,
	0xE9,0x09,0x49,0x5D,0x3D,0xAE,0x4C,0xBC,
	0x0C,0x6F,0x3A,0xBE,0x32,0x78,0x39,0xF3,
	0x33,0x07,0x94,0xAF,0xFE,0xF0,0xB4,0xF3,
	0xA5,0x3E,0xFB,0x22,0xA8,0x33,0xFA,0x02,
	0xB8,0x73,0x44,0xF5,0xDC,0x78,0xDA,0x9A,
	0xD4,0xB5,0x8C,0x17,0xEF,0x59,0xB2,0xBF
}};

static constexpr array<uint8_t, 64> aes192cbc_ciphertext = {{
	0x41,0x28,0x37,0x74,0x5B,0x88,0x08,0xDA,
	0xCC,0xC4,0x14,0xF0,0x2F,0x8D,0xF4,0x6A,
	0xBE,0xE6,0xF0,0xB7,0xE1,0x9E,0xCB,0x00,
	0x7A,0x86,0xC0,0x76,0xF0,0xA7,0x10,0x62,
	0xE4,0x5C,0x04,0xBA,0xD6,0x52,0xA8,0x32,
	0x15,0x93,0x50,0xD3,0x56,0x25,0xBB,0x92,
	0xA8,0xA0,0x64,0x26,0xA6,0xE3,0x68,0x00,
	0xBD,0x99,0x47,0x4B,0x83,0xC3,0xAD,0xF4
}};

static constexpr array<uint8_t, 64> aes256cbc_ciphertext = {{
	0x70,0x96,0xEB,0xE1,0x4B,0xC3,0xCA,0xD4,
	0xF3,0x85,0x55,0x42,0xF6,0x98,0xB9,0x19,
	0x14,0xB9,0x61,0xA3,0xF5,0xB5,0x3D,0x44,
	0x74,0xA5,0x14,0x0C,0x44,0x07,0xF6,0x78,
	0x5F,0x36,0x5A,0x3C,0xDD,0x75,0xD4,0x90,
	0x7B,0x20,0xFE,0x7F,0x6B,0x25,0x69,0xCD,
	0xAD,0x72,0xBA,0x39,0x5E,0x19,0xF2,0xBF,
	0xCE,0x35,0xAF,0x78,0x8A,0x0B,0x38,0xDB
}};

static constexpr array<uint8_t, 64> aes128ctr_ciphertext = {{
	0xAC,0x52,0x86,0x43,0x5A,0x3D,0x8E,0x0A,
	0xB0,0x9E,0xEE,0x90,0x27,0x3A,0xDA,0x81,
	0xE9,0xC0,0x88,0x78,0x4F,0x81,0xE2,0xFD,
	0x14,0x11,0x24,0xB1,0x61,0xA5,0x79,0x78,
	0xC1,0xCC,0xB9,0x5B,0xD1,0x5B,0x3D,0xBB,
	0x3D,0x25,0x20,0x55,0x95,0x98,0xBE,0x24,
	0x09,0x79,0xAD,0xB0,0xEA,0x99,0x6C,0x98,
	0x83,0x19,0xA7,0xAB,0xC4,0x2E,0x3C,0x08
}};

static constexpr array<uint8_t, 64> aes192ctr_ciphertext = {{
	0x25,0x8C,0xF0,0x21,0x59,0x35,0xAF,0xB6,
	0xD4,0x99,0xF5,0x11,0x29,0xEF,0xAF,0x8E,
	0x6C,0x8D,0x9F,0xD5,0x76,0xBF,0x1F,0xB0,
	0x10,0x10,0x14,0x6D,0x3B,0xBE,0x39,0x50,
	0x1F,0x17,0xF6,0x73,0xF0,0x92,0xE3,0xDB,
	0xE2,0x7F,0xED,0xB1,0xDA,0xE1,0x47,0xC3,
	0xC8,0x83,0xA8,0x36,0xA4,0x58,0x0A,0x03,
	0x92,0x70,0x03,0x5C,0x42,0x68,0x44,0x06
}};

static constexpr array<uint8_t, 64> aes256ctr_ciphertext = {{
	0x35,0x3B,0xD6,0xA5,0xD2,0x18,0xC7,0x27,
	0x84,0xCD,0x91,0x33,0xAC,0x05,0xF5,0x33,
	0xD0,0x1E,0x31,0x71,0xF5,0x3E,0x22,0x92,
	0x06,0x36,0x76,0x1D,0x8B,0x07,0x5C,0x29,
	0x0E,0x2D,0x12,0xD8,0xD0,0x98,0x00,0x45,
	0xFD,0x5B,0xB2,0xC1,0x7D,0x92,0xC0,0xF4,
	0xB0,0x7E,0x8E,0x53,0x11,0xCB,0x9D,0xB1,
	0xBA,0x23,0xD4,0x70,0x25,0x74,0xDB,0x8F
}};

#define AesDecryptTestSet(klass, isRequired) \
static IAesCipher *createIAesCipher_##klass(void) { \
	return AesCipherFactory::create(AesCipherFactory::Implementation::klass); \
} \
INSTANTIATE_TEST_SUITE_P(AesDecryptTest_##klass, AesCipherTest, \
	::testing::Values( \
		AesCipherTest_mode(createIAesCipher_##klass, (isRequired), \
			IAesCipher::ChainingMode::ECB, 16, \
			aes128ecb_ciphertext.data(), \
			aes128ecb_ciphertext.size()), \
		AesCipherTest_mode(createIAesCipher_##klass, (isRequired), \
			IAesCipher::ChainingMode::ECB, 24, \
			aes192ecb_ciphertext.data(), \
			aes192ecb_ciphertext.size()), \
		AesCipherTest_mode(createIAesCipher_##klass, (isRequired), \
			IAesCipher::ChainingMode::ECB, 32, \
			aes256ecb_ciphertext.data(), \
			aes256ecb_ciphertext.size()), \
		\
		AesCipherTest_mode(createIAesCipher_##klass, (isRequired), \
			IAesCipher::ChainingMode::CBC, 16, \
			aes128cbc_ciphertext.data(), \
			aes128cbc_ciphertext.size()), \
		AesCipherTest_mode(createIAesCipher_##klass, (isRequired), \
			IAesCipher::ChainingMode::CBC, 24, \
			aes192cbc_ciphertext.data(), \
			aes192cbc_ciphertext.size()), \
		AesCipherTest_mode(createIAesCipher_##klass, (isRequired), \
			IAesCipher::ChainingMode::CBC, 32, \
			aes256cbc_ciphertext.data(), \
			aes256cbc_ciphertext.size()), \
		\
		AesCipherTest_mode(createIAesCipher_##klass, (isRequired), \
			IAesCipher::ChainingMode::CTR, 16, \
			aes128ctr_ciphertext.data(), \
			aes128ctr_ciphertext.size()), \
		AesCipherTest_mode(createIAesCipher_##klass, (isRequired), \
			IAesCipher::ChainingMode::CTR, 24, \
			aes192ctr_ciphertext.data(), \
			aes192ctr_ciphertext.size()), \
		AesCipherTest_mode(createIAesCipher_##klass, (isRequired), \
			IAesCipher::ChainingMode::CTR, 32, \
			aes256ctr_ciphertext.data(), \
			aes256ctr_ciphertext.size()) \
		) \
	\
	, AesCipherTest::test_case_suffix_generator);

#ifdef _WIN32
AesDecryptTestSet(CAPI, true)
AesDecryptTestSet(CAPI_NG, false)
#endif /* _WIN32 */
#ifdef HAVE_NETTLE
AesDecryptTestSet(Nettle, true)
#endif /* HAVE_NETTLE */

} }

#ifdef HAVE_SECCOMP
const unsigned int rp_gtest_syscall_set = 0;
#endif /* HAVE_SECCOMP */

/**
 * Test suite main function.
 */
extern "C" int gtest_main(int argc, TCHAR *argv[])
{
	fmt::print(stderr, FSTR("LibRpBase test suite: Crypto tests.\n\n"));
	fflush(nullptr);

	// coverity[fun_call_w_exception]: uncaught exceptions cause nonzero exit anyway, so don't warn.
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
