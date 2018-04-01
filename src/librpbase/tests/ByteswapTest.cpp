/***************************************************************************
 * ROM Properties Page shell extension. (librpbase/tests)                  *
 * ByteswapTest.cpp: Byteswap functions test.                              *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

// Google Test
#include "gtest/gtest.h"

// Byteswap functions.
#include "librpbase/byteswap.h"
#include "librpbase/aligned_malloc.h"

// C includes. (C++ namespace)
#include <cstdio>

// C++ includes.
#include <memory>
using std::unique_ptr;

namespace LibRomData { namespace Tests {

class ByteswapTest : public ::testing::Test
{
	protected:
		ByteswapTest()
			: align_buf(nullptr)
		{ }

	public:
		// Test array size.
		static const unsigned int TEST_ARRAY_SIZE = 1024;

		/**
		 * Original test data.
		 */
		static const uint8_t bswap_orig[TEST_ARRAY_SIZE];

		/**
		 * 16-bit byteswapped test data.
		 */
		static const uint8_t bswap_16b[TEST_ARRAY_SIZE];

		/**
		 * 32-bit byteswapped test data.
		 */
		static const uint8_t bswap_32b[TEST_ARRAY_SIZE];

		// Number of iterations for benchmarks.
		static const unsigned int BENCHMARK_ITERATIONS = 100000;

	public:
		void SetUp(void) final;
		void TearDown(void) final;

	public:
		// Temporary aligned memory buffer.
		// Automatically freed in teardown().
		static const unsigned int ALIGN_BUF_SIZE = TEST_ARRAY_SIZE * 16;
		uint8_t *align_buf;
};

// Test data is in ByteswapTest_data.hpp.
#include "ByteswapTest_data.hpp"

/**
 * SetUp() function.
 * Run before each test.
 */
void ByteswapTest::SetUp(void)
{
	static_assert(ALIGN_BUF_SIZE >= TEST_ARRAY_SIZE, "ALIGN_BUF_SIZE is too small.");
	static_assert(ALIGN_BUF_SIZE % TEST_ARRAY_SIZE == 0, "ALIGN_BUF_SIZE is not a multiple of TEST_ARRAY_SIZE.");

	align_buf = static_cast<uint8_t*>(aligned_malloc(16, ALIGN_BUF_SIZE));
	ASSERT_TRUE(align_buf != nullptr);

	uint8_t *ptr = align_buf;
	for (unsigned int i = ALIGN_BUF_SIZE / TEST_ARRAY_SIZE; i > 0; i--) {
		memcpy(ptr, bswap_orig, TEST_ARRAY_SIZE);
		ptr += TEST_ARRAY_SIZE;
	}
}

/**
 * TearDown() function.
 * Run after each test.
 */
void ByteswapTest::TearDown(void)
{
	aligned_free(align_buf);
	align_buf = nullptr;
}

/**
 * Test the individual byteswapping macros.
 */
TEST_F(ByteswapTest, macroTest)
{
	EXPECT_EQ(0x2301U, __swab16(0x0123U));
	EXPECT_EQ(0x67452301U, __swab32(0x01234567U));
	EXPECT_EQ(0xEFCDAB8967452301ULL, __swab64(0x0123456789ABCDEFULL));
}

/**
 * Test the host-endian byteswapping macros.
 */
TEST_F(ByteswapTest, hostEndianMacroTest)
{
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	EXPECT_EQ(0x0123U, le16_to_cpu(0x0123U));
	EXPECT_EQ(0x01234567U, le32_to_cpu(0x01234567U));
	EXPECT_EQ(0x0123456789ABCDEFULL, le64_to_cpu(0x0123456789ABCDEFULL));
	EXPECT_EQ(0x0123U, cpu_to_le16(0x0123U));
	EXPECT_EQ(0x01234567U, cpu_to_le32(0x01234567U));
	EXPECT_EQ(0x0123456789ABCDEFULL, cpu_to_le64(0x0123456789ABCDEFULL));
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
	EXPECT_EQ(0x0123U, be16_to_cpu(0x0123U));
	EXPECT_EQ(0x01234567U, be32_to_cpu(0x01234567U));
	EXPECT_EQ(0x0123456789ABCDEFULL, be64_to_cpu(0x0123456789ABCDEFULL));
	EXPECT_EQ(0x0123U, cpu_to_be16(0x0123U));
	EXPECT_EQ(0x01234567U, cpu_to_be32(0x01234567U));
	EXPECT_EQ(0x0123456789ABCDEFULL, cpu_to_le64(0x0123456789ABCDEFULL));
#endif
}

/**
 * Test the non-host-endian byteswapping macros.
 */
TEST_F(ByteswapTest, nonHostEndianMacroTest)
{
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	EXPECT_EQ(0x2301U, be16_to_cpu(0x0123U));
	EXPECT_EQ(0x67452301U, be32_to_cpu(0x01234567U));
	EXPECT_EQ(0xEFCDAB8967452301ULL, be64_to_cpu(0x0123456789ABCDEFULL));
	EXPECT_EQ(0x2301U, cpu_to_be16(0x0123U));
	EXPECT_EQ(0x67452301U, cpu_to_be32(0x01234567U));
	EXPECT_EQ(0xEFCDAB8967452301ULL, cpu_to_be64(0x0123456789ABCDEFULL));
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
	EXPECT_EQ(0x2301U, le16_to_cpu(0x0123U));
	EXPECT_EQ(0x67452301U, le32_to_cpu(0x01234567U));
	EXPECT_EQ(0xEFCDAB8967452301ULL, le64_to_cpu(0x0123456789ABCDEFULL));
	EXPECT_EQ(0x2301U, cpu_to_le16(0x0123U));
	EXPECT_EQ(0x67452301U, cpu_to_le32(0x01234567U));
	EXPECT_EQ(0xEFCDAB8967452301ULL, cpu_to_le64(0x0123456789ABCDEFULL));
#endif
}

#define __byte_swap_16_array_dispatch(ptr, n) __byte_swap_16_array(ptr, n)
#define __byte_swap_32_array_dispatch(ptr, n) __byte_swap_32_array(ptr, n)

/**
 * Macro for testing a 16-bit byteswap function.
 * @param opt		Byteswap function optimization. (c, mmx, sse2, ssse3; dispatch for the dispatch function)
 * @param expr		Expression to check if this optimization can be used. (Use `true` for c.)
 * @param errmsg	Error message to display if the optimization cannot be used.
 */
#define DO_ARRAY_16_TEST(opt, expr, errmsg) \
TEST_F(ByteswapTest, __byte_swap_16_array_##opt##_test) \
{ \
	if (!(expr)) { \
		fputs(errmsg, stderr); \
		return; \
	} \
	__byte_swap_16_array_##opt(reinterpret_cast<uint16_t*>(align_buf), ALIGN_BUF_SIZE); \
	uint8_t *ptr = align_buf; \
	for (unsigned int i = ALIGN_BUF_SIZE / TEST_ARRAY_SIZE; i > 0; i--) { \
		EXPECT_EQ(0, memcmp(ptr, bswap_16b, TEST_ARRAY_SIZE)); \
		ptr += TEST_ARRAY_SIZE; \
	} \
}

/**
 * Macro for benchmarking a 16-bit byteswap function.
 * @param opt		Byteswap function optimization. (c, mmx, sse2, ssse3; dispatch for the dispatch function)
 * @param expr		Expression to check if this optimization can be used. (Use `true` for c.)
 * @param errmsg	Error message to display if the optimization cannot be used.
 */
#define DO_ARRAY_16_BENCHMARK(opt, expr, errmsg) \
TEST_F(ByteswapTest, __byte_swap_16_array_##opt##_benchmark) \
{ \
	if (!(expr)) { \
		fputs(errmsg, stderr); \
		return; \
	} \
	for (unsigned int i = BENCHMARK_ITERATIONS; i > 0; i--) { \
		__byte_swap_16_array_##opt(reinterpret_cast<uint16_t*>(align_buf), ALIGN_BUF_SIZE); \
	} \
}

/**
 * Macro for testing a 16-bit byteswap function.
 *
 * This version has data that is 16-bit aligned, but not 32-bit aligned,
 * and the block has an odd number of WORDs at the end.
 *
 * @param opt		Byteswap function optimization. (c, mmx, sse2, ssse3; dispatch for the dispatch function)
 * @param expr		Expression to check if this optimization can be used. (Use `true` for c.)
 * @param errmsg	Error message to display if the optimization cannot be used.
 */
#define DO_ARRAY_16_unDWORD_TEST(opt, expr, errmsg) \
TEST_F(ByteswapTest, __byte_swap_16_array_unDWORD_##opt##_test) \
{ \
	if (!(expr)) { \
		fputs(errmsg, stderr); \
		return; \
	} \
	__byte_swap_16_array_##opt(reinterpret_cast<uint16_t*>(&align_buf[2]), ALIGN_BUF_SIZE-6); \
	uint8_t *ptr = &align_buf[2]; \
	for (unsigned int i = ALIGN_BUF_SIZE / TEST_ARRAY_SIZE; i > 6; i--) { \
		EXPECT_EQ(0, memcmp(ptr, &bswap_16b[2], TEST_ARRAY_SIZE-6)); \
		ptr += TEST_ARRAY_SIZE; \
	} \
}

/**
 * Macro for benchmarking a 16-bit byteswap function.
 *
 * This version has data that is 16-bit aligned, but not 32-bit aligned,
 * and the block has an odd number of WORDs at the end.
 *
 * @param opt		Byteswap function optimization. (c, mmx, sse2, ssse3; dispatch for the dispatch function)
 * @param expr		Expression to check if this optimization can be used. (Use `true` for c.)
 * @param errmsg	Error message to display if the optimization cannot be used.
 */
#define DO_ARRAY_16_unDWORD_BENCHMARK(opt, expr, errmsg) \
TEST_F(ByteswapTest, __byte_swap_16_array_unDWORD_##opt##_benchmark) \
{ \
	if (!(expr)) { \
		fputs(errmsg, stderr); \
		return; \
	} \
	for (unsigned int i = BENCHMARK_ITERATIONS; i > 0; i--) { \
		__byte_swap_16_array_##opt(reinterpret_cast<uint16_t*>(&align_buf[2]), ALIGN_BUF_SIZE-6); \
	} \
}

/**
 * Macro for testing a 32-bit byteswap function.
 * @param opt		Byteswap function optimization. (c, mmx, sse2, ssse3; dispatch for the dispatch function)
 * @param expr		Expression to check if this optimization can be used. (Use `true` for c.)
 * @param errmsg	Error message to display if the optimization cannot be used.
 */
#define DO_ARRAY_32_TEST(opt, expr, errmsg) \
TEST_F(ByteswapTest, __byte_swap_32_array_##opt##_test) \
{ \
	if (!(expr)) { \
		fputs(errmsg, stderr); \
		return; \
	} \
	__byte_swap_32_array_##opt(reinterpret_cast<uint32_t*>(align_buf), ALIGN_BUF_SIZE); \
	uint8_t *ptr = align_buf; \
	for (unsigned int i = ALIGN_BUF_SIZE / TEST_ARRAY_SIZE; i > 0; i--) { \
		EXPECT_EQ(0, memcmp(ptr, bswap_32b, TEST_ARRAY_SIZE)); \
		ptr += TEST_ARRAY_SIZE; \
	} \
}

/**
 * Macro for benchmarking a 32-bit byteswap function.
 * @param opt		Byteswap function optimization. (c, mmx, sse2, ssse3; dispatch for the dispatch function)
 * @param expr		Expression to check if this optimization can be used. (Use `true` for c.)
 * @param errmsg	Error message to display if the optimization cannot be used.
 */
#define DO_ARRAY_32_BENCHMARK(opt, expr, errmsg) \
TEST_F(ByteswapTest, __byte_swap_32_array_##opt##_benchmark) \
{ \
	if (!(expr)) { \
		fputs(errmsg, stderr); \
		return; \
	} \
	for (unsigned int i = BENCHMARK_ITERATIONS; i > 0; i--) { \
		__byte_swap_32_array_##opt(reinterpret_cast<uint32_t*>(align_buf), ALIGN_BUF_SIZE); \
	} \
}

/**
 * Macro for testing a 32-bit byteswap function.
 *
 * This version has data that is 32-bit aligned, but not 64-bit aligned,
 * and the block has an odd number of DWORDs at the end.
 *
 * @param opt		Byteswap function optimization. (c, mmx, sse2, ssse3; dispatch for the dispatch function)
 * @param expr		Expression to check if this optimization can be used. (Use `true` for c.)
 * @param errmsg	Error message to display if the optimization cannot be used.
 */
#define DO_ARRAY_32_unQWORD_TEST(opt, expr, errmsg) \
TEST_F(ByteswapTest, __byte_swap_32_array_unQWORD_##opt##_test) \
{ \
	if (!(expr)) { \
		fputs(errmsg, stderr); \
		return; \
	} \
	__byte_swap_32_array_##opt(reinterpret_cast<uint32_t*>(&align_buf[4]), ALIGN_BUF_SIZE-8); \
	uint8_t *ptr = &align_buf[4]; \
	for (unsigned int i = ALIGN_BUF_SIZE / TEST_ARRAY_SIZE; i > 8; i--) { \
		EXPECT_EQ(0, memcmp(ptr, &bswap_32b[4], TEST_ARRAY_SIZE-8)); \
		ptr += TEST_ARRAY_SIZE; \
	} \
}

/**
 * Macro for benchmarking a 32-bit byteswap function.
 *
 * This version has data that is 32-bit aligned, but not 64-bit aligned,
 * and the block has an odd number of DWORDs at the end.
 *
 * @param opt		Byteswap function optimization. (c, mmx, sse2, ssse3; dispatch for the dispatch function)
 * @param expr		Expression to check if this optimization can be used. (Use `true` for c.)
 * @param errmsg	Error message to display if the optimization cannot be used.
 */
#define DO_ARRAY_32_unQWORD_BENCHMARK(opt, expr, errmsg) \
TEST_F(ByteswapTest, __byte_swap_32_array_unQWORD_##opt##_benchmark) \
{ \
	if (!(expr)) { \
		fputs(errmsg, stderr); \
		return; \
	} \
	for (unsigned int i = BENCHMARK_ITERATIONS; i > 0; i--) { \
		__byte_swap_32_array_##opt(reinterpret_cast<uint32_t*>(&align_buf[4]), ALIGN_BUF_SIZE-8); \
	} \
}

// Standard tests.
DO_ARRAY_16_TEST		(c, true, "")
DO_ARRAY_16_BENCHMARK		(c, true, "")
DO_ARRAY_16_unDWORD_TEST	(c, true, "")
DO_ARRAY_16_unDWORD_BENCHMARK	(c, true, "")
DO_ARRAY_32_TEST		(c, true, "")
DO_ARRAY_32_BENCHMARK		(c, true, "")
DO_ARRAY_32_unQWORD_TEST	(c, true, "")
DO_ARRAY_32_unQWORD_BENCHMARK	(c, true, "")

#ifdef BYTESWAP_HAS_MMX
// MMX-optimized tests.
DO_ARRAY_16_TEST		(mmx, RP_CPU_HasMMX(), "*** MMX is not supported on this CPU. Skipping test.\n")
DO_ARRAY_16_BENCHMARK		(mmx, RP_CPU_HasMMX(), "*** MMX is not supported on this CPU. Skipping test.\n")
DO_ARRAY_16_unDWORD_TEST	(mmx, RP_CPU_HasMMX(), "*** MMX is not supported on this CPU. Skipping test.\n")
DO_ARRAY_16_unDWORD_BENCHMARK	(mmx, RP_CPU_HasMMX(), "*** MMX is not supported on this CPU. Skipping test.\n")
DO_ARRAY_32_TEST		(mmx, RP_CPU_HasMMX(), "*** MMX is not supported on this CPU. Skipping test.\n")
DO_ARRAY_32_BENCHMARK		(mmx, RP_CPU_HasMMX(), "*** MMX is not supported on this CPU. Skipping test.\n")
DO_ARRAY_32_unQWORD_TEST	(mmx, RP_CPU_HasMMX(), "*** MMX is not supported on this CPU. Skipping test.\n")
DO_ARRAY_32_unQWORD_BENCHMARK	(mmx, RP_CPU_HasMMX(), "*** MMX is not supported on this CPU. Skipping test.\n")
#endif /* BYTESWAP_HAS_MMX */

#ifdef BYTESWAP_HAS_SSE2
// SSE2-optimized tests.
DO_ARRAY_16_TEST		(sse2, RP_CPU_HasSSE2(), "*** SSE2 is not supported on this CPU. Skipping test.\n")
DO_ARRAY_16_BENCHMARK		(sse2, RP_CPU_HasSSE2(), "*** SSE2 is not supported on this CPU. Skipping test.\n")
DO_ARRAY_16_unDWORD_TEST	(sse2, RP_CPU_HasSSE2(), "*** SSE2 is not supported on this CPU. Skipping test.\n")
DO_ARRAY_16_unDWORD_BENCHMARK	(sse2, RP_CPU_HasSSE2(), "*** SSE2 is not supported on this CPU. Skipping test.\n")
DO_ARRAY_32_TEST		(sse2, RP_CPU_HasSSE2(), "*** SSE2 is not supported on this CPU. Skipping test.\n")
DO_ARRAY_32_BENCHMARK		(sse2, RP_CPU_HasSSE2(), "*** SSE2 is not supported on this CPU. Skipping test.\n")
DO_ARRAY_32_unQWORD_TEST	(sse2, RP_CPU_HasSSE2(), "*** SSE2 is not supported on this CPU. Skipping test.\n")
DO_ARRAY_32_unQWORD_BENCHMARK	(sse2, RP_CPU_HasSSE2(), "*** SSE2 is not supported on this CPU. Skipping test.\n")
#endif /* BYTESWAP_HAS_SSE2 */

#ifdef BYTESWAP_HAS_SSSE3
// SSSE3-optimized tests.
DO_ARRAY_16_TEST		(ssse3, RP_CPU_HasSSSE3(), "*** SSSE3 is not supported on this CPU. Skipping test.\n")
DO_ARRAY_16_BENCHMARK		(ssse3, RP_CPU_HasSSSE3(), "*** SSSE3 is not supported on this CPU. Skipping test.\n")
DO_ARRAY_16_unDWORD_TEST	(ssse3, RP_CPU_HasSSSE3(), "*** SSSE3 is not supported on this CPU. Skipping test.\n")
DO_ARRAY_16_unDWORD_BENCHMARK	(ssse3, RP_CPU_HasSSSE3(), "*** SSSE3 is not supported on this CPU. Skipping test.\n")
DO_ARRAY_32_TEST		(ssse3, RP_CPU_HasSSSE3(), "*** SSSE3 is not supported on this CPU. Skipping test.\n")
DO_ARRAY_32_BENCHMARK		(ssse3, RP_CPU_HasSSSE3(), "*** SSSE3 is not supported on this CPU. Skipping test.\n")
DO_ARRAY_32_unQWORD_TEST	(ssse3, RP_CPU_HasSSSE3(), "*** SSSE3 is not supported on this CPU. Skipping test.\n")
DO_ARRAY_32_unQWORD_BENCHMARK	(ssse3, RP_CPU_HasSSSE3(), "*** SSSE3 is not supported on this CPU. Skipping test.\n")
#endif /* BYTESWAP_HAS_SSSE3 */

// NOTE: Add more instruction sets to the #ifdef if other optimizations are added.
#if defined(BYTESWAP_HAS_MMX) || defined(BYTESWAP_HAS_SSE2) || defined(BYTESWAP_HAS_SSSE3)
// Dispatch functions.
DO_ARRAY_16_TEST		(dispatch, true, "")
DO_ARRAY_16_BENCHMARK		(dispatch, true, "")
DO_ARRAY_16_unDWORD_TEST	(dispatch, true, "")
DO_ARRAY_16_unDWORD_BENCHMARK	(dispatch, true, "")
DO_ARRAY_32_TEST		(dispatch, true, "")
DO_ARRAY_32_BENCHMARK		(dispatch, true, "")
DO_ARRAY_32_unQWORD_TEST	(dispatch, true, "")
DO_ARRAY_32_unQWORD_BENCHMARK	(dispatch, true, "")
#endif /* BYTESWAP_HAS_MMX || BYTESWAP_HAS_SSE2 || BYTESWAP_HAS_SSSE3 */

} }

/**
 * Test suite main function.
 */
extern "C" int gtest_main(int argc, char *argv[])
{
	fprintf(stderr, "LibRomData test suite: Byteswap tests.\n\n");
	fprintf(stderr, "Benchmark iterations: %u\n", LibRomData::Tests::ByteswapTest::BENCHMARK_ITERATIONS);
	fflush(nullptr);

	// coverity[fun_call_w_exception]: uncaught exceptions cause nonzero exit anyway, so don't warn.
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
