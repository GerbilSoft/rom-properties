/***************************************************************************
 * ROM Properties Page shell extension. (librpbyteswap/tests)              *
 * ByteswapTest.cpp: Byteswap functions test.                              *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Google Test
#include "gtest/gtest.h"
#include "tcharx.h"

// Byteswap functions.
#include "librpbyteswap/byteswap_rp.h"
#include "aligned_malloc.h"

// C includes (C++ namespace)
#include <cstdio>

// C++ includes
// NOTE: This must be here and *not* in ByteswapTest_data.hpp because
// ByteswapTest_data.hpp is contained within the LibRpByteswap::Tests namespace.
#include <array>
using std::array;

// libfmt
#include "rp-libfmt.h"

namespace LibRpByteswap { namespace Tests {

class ByteswapTest : public ::testing::Test
{
	protected:
		ByteswapTest()
			: align_buf(aligned_uptr<uint8_t>(16, 8))
		{
			// Dummy align_buf initialization to
			// prevent compiler errors.
		}

	public:
		// Test array size
		static constexpr unsigned int TEST_ARRAY_SIZE = 1024U;

		/**
		 * Original test data
		 */
		static const array<uint8_t, TEST_ARRAY_SIZE> bswap_orig;

		/**
		 * 16-bit byteswapped test data
		 */
		static const array<uint8_t, TEST_ARRAY_SIZE> bswap_16b;

		/**
		 * 32-bit byteswapped test data
		 */
		static const array<uint8_t, TEST_ARRAY_SIZE> bswap_32b;

		// Number of iterations for benchmarks.
		static constexpr unsigned int BENCHMARK_ITERATIONS = 100000U;

	public:
		void SetUp(void) final;
		void TearDown(void) final;

	public:
		// Temporary aligned memory buffer.
		static constexpr unsigned int ALIGN_BUF_SIZE = TEST_ARRAY_SIZE * 16U;
		UNIQUE_PTR_ALIGNED(uint8_t) align_buf;
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
	static_assert(bswap_orig.size() == TEST_ARRAY_SIZE, "bswap_orig.size() != TEST_ARRAY_SIZE");
	static_assert(bswap_16b.size() == TEST_ARRAY_SIZE, "bswap_16b.size() != TEST_ARRAY_SIZE");
	static_assert(bswap_32b.size() == TEST_ARRAY_SIZE, "bswap_32b.size() != TEST_ARRAY_SIZE");

	align_buf = aligned_uptr<uint8_t>(16, ALIGN_BUF_SIZE);
	ASSERT_TRUE(align_buf != nullptr);

	uint8_t *ptr = align_buf.get();
	for (unsigned int i = ALIGN_BUF_SIZE / TEST_ARRAY_SIZE; i > 0; i--) {
		memcpy(ptr, bswap_orig.data(), TEST_ARRAY_SIZE);
		ptr += TEST_ARRAY_SIZE;
	}
}

/**
 * TearDown() function.
 * Run after each test.
 */
void ByteswapTest::TearDown(void)
{
	// NOTE: Can't simply reset it to nullptr.
	align_buf = aligned_uptr<uint8_t>(16, 8);
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
	EXPECT_EQ(0x0123456789ABCDEFULL, cpu_to_be64(0x0123456789ABCDEFULL));
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

#define rp_byte_swap_16_array_dispatch(ptr, n) rp_byte_swap_16_array(ptr, n)
#define rp_byte_swap_32_array_dispatch(ptr, n) rp_byte_swap_32_array(ptr, n)

/**
 * Macro for testing a 16-bit byteswap function.
 * @param opt		Byteswap function optimization. (c, mmx, sse2, ssse3; dispatch for the dispatch function)
 * @param unalign	Number of bytes for unaligned testing.
 * @param expr		Expression to check if this optimization can be used. (Use `true` for c.)
 * @param errmsg	Error message to display if the optimization cannot be used.
 */
#define DO_ARRAY_16_TEST(opt, unalign, expr, errmsg) \
TEST_F(ByteswapTest, rp_byte_swap_16_array_##opt##_unalign##unalign##_test) \
{ \
	if (!(expr)) { \
		fputs(errmsg, stderr); \
		return; \
	} \
	rp_byte_swap_16_array_##opt(reinterpret_cast<uint16_t*>(align_buf.get() + (unalign)), ALIGN_BUF_SIZE - (unalign)); \
	const uint8_t *ptr = align_buf.get(); \
	for (unsigned int i = ALIGN_BUF_SIZE / TEST_ARRAY_SIZE; i > 0; i--) { \
		EXPECT_EQ(0, memcmp(ptr + (unalign), bswap_16b.data() + (unalign), TEST_ARRAY_SIZE - (unalign))); \
		ptr += TEST_ARRAY_SIZE; \
	} \
}

/**
 * Macro for benchmarking a 16-bit byteswap function.
 * @param opt		Byteswap function optimization. (c, mmx, sse2, ssse3; dispatch for the dispatch function)
 * @param unalign	Number of bytes for unaligned testing.
 * @param expr		Expression to check if this optimization can be used. (Use `true` for c.)
 * @param errmsg	Error message to display if the optimization cannot be used.
 */
#define DO_ARRAY_16_BENCHMARK(opt, unalign, expr, errmsg) \
TEST_F(ByteswapTest, rp_byte_swap_16_array_##opt##_unalign##unalign##_benchmark) \
{ \
	if (!(expr)) { \
		fputs(errmsg, stderr); \
		return; \
	} \
	for (unsigned int i = BENCHMARK_ITERATIONS; i > 0; i--) { \
		rp_byte_swap_16_array_##opt(reinterpret_cast<uint16_t*>(align_buf.get() + (unalign)), ALIGN_BUF_SIZE - (unalign)); \
	} \
}

/**
 * Macro for testing a 16-bit byteswap function.
 *
 * This version has data that is 16-bit aligned, but not 32-bit aligned,
 * and the block has an odd number of WORDs at the end.
 *
 * @param opt		Byteswap function optimization. (c, mmx, sse2, ssse3; dispatch for the dispatch function)
 * @param unalign	Number of bytes for unaligned testing.
 * @param expr		Expression to check if this optimization can be used. (Use `true` for c.)
 * @param errmsg	Error message to display if the optimization cannot be used.
 */
#define DO_ARRAY_16_unDWORD_TEST(opt, expr, errmsg) \
TEST_F(ByteswapTest, rp_byte_swap_16_array_##opt##_unDWORD_test) \
{ \
	if (!(expr)) { \
		fputs(errmsg, stderr); \
		return; \
	} \
	rp_byte_swap_16_array_##opt(reinterpret_cast<uint16_t*>(&align_buf.get()[2]), ALIGN_BUF_SIZE-6); \
	const uint8_t *ptr = &align_buf.get()[2]; \
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
 * @param unalign	Number of bytes for unaligned testing.
 * @param expr		Expression to check if this optimization can be used. (Use `true` for c.)
 * @param errmsg	Error message to display if the optimization cannot be used.
 */
#define DO_ARRAY_16_unDWORD_BENCHMARK(opt, expr, errmsg) \
TEST_F(ByteswapTest, rp_byte_swap_16_array_##opt##_unDWORD_benchmark) \
{ \
	if (!(expr)) { \
		fputs(errmsg, stderr); \
		return; \
	} \
	for (unsigned int i = BENCHMARK_ITERATIONS; i > 0; i--) { \
		rp_byte_swap_16_array_##opt(reinterpret_cast<uint16_t*>(&align_buf.get()[2]), ALIGN_BUF_SIZE-6); \
	} \
}

/**
 * Macro for testing a 32-bit byteswap function.
 * @param opt		Byteswap function optimization. (c, mmx, sse2, ssse3; dispatch for the dispatch function)
 * @param unalign	Number of bytes for unaligned testing.
 * @param expr		Expression to check if this optimization can be used. (Use `true` for c.)
 * @param errmsg	Error message to display if the optimization cannot be used.
 */
#define DO_ARRAY_32_TEST(opt, unalign, expr, errmsg) \
TEST_F(ByteswapTest, rp_byte_swap_32_array_##opt##_unalign##unalign##_test) \
{ \
	if (!(expr)) { \
		fputs(errmsg, stderr); \
		return; \
	} \
	rp_byte_swap_32_array_##opt(reinterpret_cast<uint32_t*>(align_buf.get() + (unalign)), ALIGN_BUF_SIZE - (unalign)); \
	const uint8_t *ptr = align_buf.get(); \
	for (unsigned int i = ALIGN_BUF_SIZE / TEST_ARRAY_SIZE; i > 0; i--) { \
		EXPECT_EQ(0, memcmp(ptr + (unalign), bswap_32b.data() + (unalign), TEST_ARRAY_SIZE - (unalign))); \
		ptr += TEST_ARRAY_SIZE; \
	} \
}

/**
 * Macro for benchmarking a 32-bit byteswap function.
 * @param opt		Byteswap function optimization. (c, mmx, sse2, ssse3; dispatch for the dispatch function)
 * @param unalign	Number of bytes for unaligned testing.
 * @param expr		Expression to check if this optimization can be used. (Use `true` for c.)
 * @param errmsg	Error message to display if the optimization cannot be used.
 */
#define DO_ARRAY_32_BENCHMARK(opt, unalign, expr, errmsg) \
TEST_F(ByteswapTest, rp_byte_swap_32_array_##opt##_unalign##unalign##_benchmark) \
{ \
	if (!(expr)) { \
		fputs(errmsg, stderr); \
		return; \
	} \
	for (unsigned int i = BENCHMARK_ITERATIONS; i > 0; i--) { \
		rp_byte_swap_32_array_##opt(reinterpret_cast<uint32_t*>(align_buf.get() + (unalign)), ALIGN_BUF_SIZE - (unalign)); \
	} \
}

/**
 * Macro for testing a 32-bit byteswap function.
 *
 * This version has data that is 32-bit aligned, but not 64-bit aligned,
 * and the block has an odd number of DWORDs at the end.
 *
 * @param opt		Byteswap function optimization. (c, mmx, sse2, ssse3; dispatch for the dispatch function)
 * @param unalign	Number of bytes for unaligned testing.
 * @param expr		Expression to check if this optimization can be used. (Use `true` for c.)
 * @param errmsg	Error message to display if the optimization cannot be used.
 */
#define DO_ARRAY_32_unQWORD_TEST(opt, expr, errmsg) \
TEST_F(ByteswapTest, rp_byte_swap_32_array_##opt##_unQWORD_test) \
{ \
	if (!(expr)) { \
		fputs(errmsg, stderr); \
		return; \
	} \
	rp_byte_swap_32_array_##opt(reinterpret_cast<uint32_t*>(&align_buf.get()[4]), ALIGN_BUF_SIZE-8); \
	const uint8_t *ptr = &align_buf.get()[4]; \
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
 * @param unalign	Number of bytes for unaligned testing.
 * @param expr		Expression to check if this optimization can be used. (Use `true` for c.)
 * @param errmsg	Error message to display if the optimization cannot be used.
 */
#define DO_ARRAY_32_unQWORD_BENCHMARK(opt, expr, errmsg) \
TEST_F(ByteswapTest, rp_byte_swap_32_array_##opt##_unQWORD_benchmark) \
{ \
	if (!(expr)) { \
		fputs(errmsg, stderr); \
		return; \
	} \
	for (unsigned int i = BENCHMARK_ITERATIONS; i > 0; i--) { \
		rp_byte_swap_32_array_##opt(reinterpret_cast<uint32_t*>(&align_buf.get()[4]), ALIGN_BUF_SIZE-8); \
	} \
}

// Standard tests
DO_ARRAY_16_TEST		(c, 0, true, "")
DO_ARRAY_16_BENCHMARK		(c, 0, true, "")
DO_ARRAY_16_unDWORD_TEST	(c, true, "")
DO_ARRAY_16_unDWORD_BENCHMARK	(c, true, "")
DO_ARRAY_32_TEST		(c, 0, true, "")
DO_ARRAY_32_BENCHMARK		(c, 0, true, "")
DO_ARRAY_32_unQWORD_TEST	(c, true, "")
DO_ARRAY_32_unQWORD_BENCHMARK	(c, true, "")

DO_ARRAY_16_TEST		(c, 2, true, "")
DO_ARRAY_16_BENCHMARK		(c, 2, true, "")
DO_ARRAY_32_TEST		(c, 4, true, "")
DO_ARRAY_32_BENCHMARK		(c, 4, true, "")

#ifdef BYTESWAP_HAS_MMX
// MMX-optimized tests
DO_ARRAY_16_TEST		(mmx, 0, RP_CPU_x86_HasMMX(), "*** MMX is not supported on this CPU. Skipping test.\n")
DO_ARRAY_16_BENCHMARK		(mmx, 0 ,RP_CPU_x86_HasMMX(), "*** MMX is not supported on this CPU. Skipping test.\n")
DO_ARRAY_16_unDWORD_TEST	(mmx, RP_CPU_x86_HasMMX(), "*** MMX is not supported on this CPU. Skipping test.\n")
DO_ARRAY_16_unDWORD_BENCHMARK	(mmx, RP_CPU_x86_HasMMX(), "*** MMX is not supported on this CPU. Skipping test.\n")
DO_ARRAY_32_TEST		(mmx, 0, RP_CPU_x86_HasMMX(), "*** MMX is not supported on this CPU. Skipping test.\n")
DO_ARRAY_32_BENCHMARK		(mmx, 0, RP_CPU_x86_HasMMX(), "*** MMX is not supported on this CPU. Skipping test.\n")
DO_ARRAY_32_unQWORD_TEST	(mmx, RP_CPU_x86_HasMMX(), "*** MMX is not supported on this CPU. Skipping test.\n")
DO_ARRAY_32_unQWORD_BENCHMARK	(mmx, RP_CPU_x86_HasMMX(), "*** MMX is not supported on this CPU. Skipping test.\n")

DO_ARRAY_16_TEST		(mmx, 2, RP_CPU_x86_HasMMX(), "*** MMX is not supported on this CPU. Skipping test.\n")
DO_ARRAY_16_BENCHMARK		(mmx, 2 ,RP_CPU_x86_HasMMX(), "*** MMX is not supported on this CPU. Skipping test.\n")
DO_ARRAY_32_TEST		(mmx, 4, RP_CPU_x86_HasMMX(), "*** MMX is not supported on this CPU. Skipping test.\n")
DO_ARRAY_32_BENCHMARK		(mmx, 4, RP_CPU_x86_HasMMX(), "*** MMX is not supported on this CPU. Skipping test.\n")
#endif /* BYTESWAP_HAS_MMX */

#ifdef BYTESWAP_HAS_SSE2
// SSE2-optimized tests
DO_ARRAY_16_TEST		(sse2, 0, RP_CPU_x86_HasSSE2(), "*** SSE2 is not supported on this CPU. Skipping test.\n")
DO_ARRAY_16_BENCHMARK		(sse2, 0, RP_CPU_x86_HasSSE2(), "*** SSE2 is not supported on this CPU. Skipping test.\n")
DO_ARRAY_16_unDWORD_TEST	(sse2, RP_CPU_x86_HasSSE2(), "*** SSE2 is not supported on this CPU. Skipping test.\n")
DO_ARRAY_16_unDWORD_BENCHMARK	(sse2, RP_CPU_x86_HasSSE2(), "*** SSE2 is not supported on this CPU. Skipping test.\n")
DO_ARRAY_32_TEST		(sse2, 0, RP_CPU_x86_HasSSE2(), "*** SSE2 is not supported on this CPU. Skipping test.\n")
DO_ARRAY_32_BENCHMARK		(sse2, 0, RP_CPU_x86_HasSSE2(), "*** SSE2 is not supported on this CPU. Skipping test.\n")
DO_ARRAY_32_unQWORD_TEST	(sse2, RP_CPU_x86_HasSSE2(), "*** SSE2 is not supported on this CPU. Skipping test.\n")
DO_ARRAY_32_unQWORD_BENCHMARK	(sse2, RP_CPU_x86_HasSSE2(), "*** SSE2 is not supported on this CPU. Skipping test.\n")

DO_ARRAY_16_TEST		(sse2, 2, RP_CPU_x86_HasSSE2(), "*** SSE2 is not supported on this CPU. Skipping test.\n")
DO_ARRAY_16_BENCHMARK		(sse2, 2, RP_CPU_x86_HasSSE2(), "*** SSE2 is not supported on this CPU. Skipping test.\n")
DO_ARRAY_32_TEST		(sse2, 4, RP_CPU_x86_HasSSE2(), "*** SSE2 is not supported on this CPU. Skipping test.\n")
DO_ARRAY_32_BENCHMARK		(sse2, 4, RP_CPU_x86_HasSSE2(), "*** SSE2 is not supported on this CPU. Skipping test.\n")
#endif /* BYTESWAP_HAS_SSE2 */

#ifdef BYTESWAP_HAS_SSSE3
// SSSE3-optimized tests
DO_ARRAY_16_TEST		(ssse3, 0, RP_CPU_x86_HasSSSE3(), "*** SSSE3 is not supported on this CPU. Skipping test.\n")
DO_ARRAY_16_BENCHMARK		(ssse3, 0, RP_CPU_x86_HasSSSE3(), "*** SSSE3 is not supported on this CPU. Skipping test.\n")
DO_ARRAY_16_unDWORD_TEST	(ssse3, RP_CPU_x86_HasSSSE3(), "*** SSSE3 is not supported on this CPU. Skipping test.\n")
DO_ARRAY_16_unDWORD_BENCHMARK	(ssse3, RP_CPU_x86_HasSSSE3(), "*** SSSE3 is not supported on this CPU. Skipping test.\n")
DO_ARRAY_32_TEST		(ssse3, 0, RP_CPU_x86_HasSSSE3(), "*** SSSE3 is not supported on this CPU. Skipping test.\n")
DO_ARRAY_32_BENCHMARK		(ssse3, 0, RP_CPU_x86_HasSSSE3(), "*** SSSE3 is not supported on this CPU. Skipping test.\n")
DO_ARRAY_32_unQWORD_TEST	(ssse3, RP_CPU_x86_HasSSSE3(), "*** SSSE3 is not supported on this CPU. Skipping test.\n")
DO_ARRAY_32_unQWORD_BENCHMARK	(ssse3, RP_CPU_x86_HasSSSE3(), "*** SSSE3 is not supported on this CPU. Skipping test.\n")

DO_ARRAY_16_TEST		(ssse3, 2, RP_CPU_x86_HasSSSE3(), "*** SSSE3 is not supported on this CPU. Skipping test.\n")
DO_ARRAY_16_BENCHMARK		(ssse3, 2, RP_CPU_x86_HasSSSE3(), "*** SSSE3 is not supported on this CPU. Skipping test.\n")
DO_ARRAY_32_TEST		(ssse3, 4, RP_CPU_x86_HasSSSE3(), "*** SSSE3 is not supported on this CPU. Skipping test.\n")
DO_ARRAY_32_BENCHMARK		(ssse3, 4, RP_CPU_x86_HasSSSE3(), "*** SSSE3 is not supported on this CPU. Skipping test.\n")
#endif /* BYTESWAP_HAS_SSSE3 */

#ifdef BYTESWAP_HAS_NEON
// ARM64-optimized tests
DO_ARRAY_16_TEST		(neon, 0, 1 /* TODO */, "*** NEON is not supported on this CPU. Skipping test.\n")
DO_ARRAY_16_BENCHMARK		(neon, 0, 1 /* TODO */, "*** NEON is not supported on this CPU. Skipping test.\n")
DO_ARRAY_16_unDWORD_TEST	(neon, 1 /* TODO */, "*** NEON is not supported on this CPU. Skipping test.\n")
DO_ARRAY_16_unDWORD_BENCHMARK	(neon, 1 /* TODO */, "*** NEON is not supported on this CPU. Skipping test.\n")
DO_ARRAY_32_TEST		(neon, 0, 1 /* TODO */, "*** NEON is not supported on this CPU. Skipping test.\n")
DO_ARRAY_32_BENCHMARK		(neon, 0, 1 /* TODO */, "*** NEON is not supported on this CPU. Skipping test.\n")
DO_ARRAY_32_unQWORD_TEST	(neon, 1 /* TODO */, "*** NEON is not supported on this CPU. Skipping test.\n")
DO_ARRAY_32_unQWORD_BENCHMARK	(neon, 1 /* TODO */, "*** NEON is not supported on this CPU. Skipping test.\n")

DO_ARRAY_16_TEST		(neon, 2, 1 /* TODO */, "*** NEON is not supported on this CPU. Skipping test.\n")
DO_ARRAY_16_BENCHMARK		(neon, 2, 1 /* TODO */, "*** NEON is not supported on this CPU. Skipping test.\n")
DO_ARRAY_32_TEST		(neon, 4, 1 /* TODO */, "*** NEON is not supported on this CPU. Skipping test.\n")
DO_ARRAY_32_BENCHMARK		(neon, 4, 1 /* TODO */, "*** NEON is not supported on this CPU. Skipping test.\n")
#endif /* BYTESWAP_HAS_NEON */

// NOTE: Add more instruction sets to the #ifdef if other optimizations are added.
#if defined(BYTESWAP_HAS_MMX) || defined(BYTESWAP_HAS_SSE2) || defined(BYTESWAP_HAS_SSSE3) || defined(BYTESWAP_HAS_NEON)
// Dispatch functions
DO_ARRAY_16_TEST		(dispatch, 0, true, "")
DO_ARRAY_16_BENCHMARK		(dispatch, 0, true, "")
DO_ARRAY_16_unDWORD_TEST	(dispatch, true, "")
DO_ARRAY_16_unDWORD_BENCHMARK	(dispatch, true, "")
DO_ARRAY_32_TEST		(dispatch, 0, true, "")
DO_ARRAY_32_BENCHMARK		(dispatch, 0, true, "")
DO_ARRAY_32_unQWORD_TEST	(dispatch, true, "")
DO_ARRAY_32_unQWORD_BENCHMARK	(dispatch, true, "")

DO_ARRAY_16_TEST		(dispatch, 2, true, "")
DO_ARRAY_16_BENCHMARK		(dispatch, 2, true, "")
DO_ARRAY_32_TEST		(dispatch, 4, true, "")
DO_ARRAY_32_BENCHMARK		(dispatch, 4, true, "")
#endif /* BYTESWAP_HAS_MMX || BYTESWAP_HAS_SSE2 || BYTESWAP_HAS_SSSE3 || BYTESWAP_HAS_NEON */

} }

/**
 * Test suite main function.
 */
extern "C" int gtest_main(int argc, TCHAR *argv[])
{
	fputs("LibRpByteswap test suite: Byteswap tests.\n\n", stderr);
	fmt::print(stderr, FSTR("Benchmark iterations: {:d}\n"),
		LibRpByteswap::Tests::ByteswapTest::BENCHMARK_ITERATIONS);
	fflush(nullptr);

	// coverity[fun_call_w_exception]: uncaught exceptions cause nonzero exit anyway, so don't warn.
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
