/***************************************************************************
 * ROM Properties Page shell extension. (libromdata/tests)                 *
 * NintendoSystemIDTest.cpp: Nintendo System ID structs test.              *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Google Test
#include "gtest/gtest.h"
#include "tcharx.h"

#include "librpbyteswap/byteswap_rp.h"
#include "nintendo_system_id.h"

// C includes (C++ namespace)
#include <cstdio>

// libfmt
#include <fmt/core.h>
#include <fmt/format.h>
#define FSTR FMT_STRING

namespace LibRomData { namespace Tests {

class NintendoSystemIDTest : public ::testing::Test
{
	protected:
		NintendoSystemIDTest() = default;
};

/**
 * Test a big-endian title ID.
 */
TEST_F(NintendoSystemIDTest, beTest)
{
	Nintendo_TitleID_BE_t tid;

	// Super Smash Bros. for Nintendo 3DS (U) Update
	tid.id = cpu_to_be64(0x0004000E000EDF00);

	EXPECT_EQ(cpu_to_be32(0x0004000E), tid.hi);
	EXPECT_EQ(cpu_to_be32(0x000EDF00), tid.lo);
	EXPECT_EQ(cpu_to_be16(0x0004), tid.sysID);
	EXPECT_EQ(cpu_to_be16(0x000E), tid.catID);
}

/**
 * Test a little-endian title ID.
 */
TEST_F(NintendoSystemIDTest, leTest)
{
	Nintendo_TitleID_LE_t tid;

	// Super Smash Bros. for Nintendo 3DS (U) Update
	tid.id = cpu_to_le64(0x0004000E000EDF00);

	EXPECT_EQ(cpu_to_le32(0x0004000E), tid.hi);
	EXPECT_EQ(cpu_to_le32(0x000EDF00), tid.lo);
	EXPECT_EQ(cpu_to_le16(0x0004), tid.sysID);
	EXPECT_EQ(cpu_to_le16(0x000E), tid.catID);
}

} }

/**
 * Test suite main function.
 */
extern "C" int gtest_main(int argc, TCHAR *argv[])
{
	fmt::print(stderr, FSTR("LibRomData test suite: NintendoSystemID tests.\n\n"));
	fflush(nullptr);

	// coverity[fun_call_w_exception]: uncaught exceptions cause nonzero exit anyway, so don't warn.
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
