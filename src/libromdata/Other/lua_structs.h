/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * lua_structs.h: Lua data structures.                                     *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * Copyright (c) 2016-2022 by Egor.                                        *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

/* NOTE: this file is unused, but I left it here for future reference. */

#pragma once

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LUA_MAGIC "\033Lua"
#define LUA_TAIL "\x19\x93\r\n\x1a\n"

/* Actual header sizes:
 * 2.4: 11
 * 2.5: 14
 * 3.1: 7+Number
 * 3.2: 6+Number
 * 4.0: 13+Number
 * 5.0: 14+Number
 * 5.1: 12
 * 5.2: 18
 * 5.3: 17+Integer+Number
 * 5.4: 15+Integer+Number
 * 5.5: 16+int+Instruction+Integer+Number (the biggest one)
 */
#define LUA_HEADERSIZE (16+8+8+8+8)

/**
 * Lua binary chunk header.
 *
 * References:
 * - lundump.{c,h} from various Lua versions.
 */
typedef struct _Lua_Header {
	char magic[4]; /* '\033Lua' */
	uint8_t version; /* 0x50 = 5.0, 0x51 = 5.1, etc */
} Lua_Header;
ASSERT_STRUCT(Lua_Header, 5);

/**
 * Lua 2.3 binary chunk header.
 */
typedef struct _Lua2_3_Header {
	Lua_Header header;
	/* followed by a test number, which is 0x1234 cast to word */
	/* followed by a test number, which is 0.123456789e-23 cast to float */
} Lua2_3_Header;
ASSERT_STRUCT(Lua2_3_Header, 5);

/**
 * Lua 2.5 binary chunk header.
 */
typedef struct _Lua2_5_Header {
	Lua_Header header;
	uint8_t word_size; // hardcoded to 2
	uint8_t float_size; // hardcoded to 4
	uint8_t ptr_size;
	/* followed by a test number, which is 0x1234 cast to word */
	/* followed by a test number, which is 0.123456789e-23 cast to float */
} Lua2_5_Header;
ASSERT_STRUCT(Lua2_5_Header, 8);

/**
 * Lua 3.1 binary chunk header.
 */
typedef struct _Lua3_1_Header {
	Lua_Header header;
	uint8_t Number_type; /* 'l' long BE 32, 'f' float BE 32, 'd' double BE 64, '?' native */
	uint8_t Number_size; /* this is the size of the *native* number type */
	/* followed by a test number, which is 3.14159265358979323846E8 cast to lua_Number */
} Lua3_1_Header;
ASSERT_STRUCT(Lua3_1_Header, 7);

/**
 * Lua 3.2 binary chunk header.
 */
typedef struct _Lua3_2_Header {
	Lua_Header header;
	uint8_t Number_size; /* if 0, the numbers are stored as strings, and the test number doesn't exist */
	/* followed by a test number, which is 3.14159265358979323846E8 cast to lua_Number */
} Lua3_2_Header;
ASSERT_STRUCT(Lua3_2_Header, 6);

/**
 * Lua 4.0 binary chunk header.
 */
typedef struct _Lua4_0_Header {
	Lua_Header header;
	uint8_t endianness; /* 0 = BE, 1 = LE */
	uint8_t int_size;
	uint8_t size_t_size;
	uint8_t Instruction_size;
	uint8_t INSTRUCTION_bits;
	uint8_t OP_bits;
	uint8_t B_bits;
	uint8_t Number_size;
	/* followed by a test number, which is 3.14159265358979323846E8 cast to lua_Number */
} Lua4_0_Header;
ASSERT_STRUCT(Lua4_0_Header, 13);

/**
 * Lua 5.0 binary chunk header.
 */
typedef struct _Lua5_0_Header {
	Lua_Header header;
	uint8_t endianness; /* 0 = BE, 1 = LE */
	uint8_t int_size;
	uint8_t size_t_size;
	uint8_t Instruction_size;
	uint8_t OP_bits;
	uint8_t A_bits;
	uint8_t B_bits;
	uint8_t C_bits;
	uint8_t Number_size;
	/* followed by a test number, which is 3.14159265358979323846E7 cast to lua_Number */
} Lua5_0_Header;
ASSERT_STRUCT(Lua5_0_Header, 14);

/**
 * Lua 5.1 binary chunk header.
 */
typedef struct _Lua5_1_Header {
	Lua_Header header;
	uint8_t format; /* 0 = official format */
	uint8_t endianness; /* 0 = BE, 1 = LE */
	uint8_t int_size;
	uint8_t size_t_size;
	uint8_t Instruction_size;
	uint8_t Number_size;
	uint8_t is_integral;
} Lua5_1_Header;
ASSERT_STRUCT(Lua5_1_Header, 12);

/**
 * Lua 5.2 binary chunk header.
 */
typedef struct _Lua5_2_Header {
	Lua_Header header;
	uint8_t format; /* 0 = official format */
	uint8_t endianness; /* 0 = BE, 1 = LE */
	uint8_t int_size;
	uint8_t size_t_size;
	uint8_t Instruction_size;
	uint8_t Number_size;
	uint8_t is_integral;
	char tail[6]; /* LUA_TAIL */
} Lua5_2_Header;
ASSERT_STRUCT(Lua5_2_Header, 18);

/**
 * Lua 5.3 binary chunk header.
 */
typedef struct _Lua5_3_Header {
	Lua_Header header;
	uint8_t format; /* 0 = official format */
	char tail[6]; /* LUA_TAIL */
	uint8_t int_size;
	uint8_t size_t_size;
	uint8_t Instruction_size;
	uint8_t Integer_size;
	uint8_t Number_size;
	/* followed by test integer 0x5678 */
	/* followed by test number 370.5 */
} Lua5_3_Header;
ASSERT_STRUCT(Lua5_3_Header, 17);

/**
 * Lua 5.4 binary chunk header.
 */
typedef struct _Lua5_4_Header {
	Lua_Header header;
	uint8_t format; /* 0 = official format */
	char tail[6]; /* LUA_TAIL */
	uint8_t Instruction_size;
	uint8_t Integer_size;
	uint8_t Number_size;
	/* followed by test integer 0x5678 */
	/* followed by test number 370.5 */
} Lua5_4_Header;
ASSERT_STRUCT(Lua5_4_Header, 15);

/**
 * Lua 5.5 binary chunk header.
 */
typedef struct _Lua5_5_Header {
	Lua_Header header;
	uint8_t format; /* 0 = official format */
	char tail[6]; /* LUA_TAIL */
	/* uint8_t int_size;
	 * followed by test int -0x5678
	 * uint8_t Instruction_size;
	 * followed by test instruction 0x12345678
	 * uint8_t Integer_size;
	 * followed by test integer -0x5678
	 * uint8_t Number_size;
	 * followed by test number -370.5
	 */
} Lua5_5_Header;
ASSERT_STRUCT(Lua5_5_Header, 12);

#ifdef __cplusplus
}
#endif
