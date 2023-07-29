/***************************************************************************
 * ROM Properties Page shell extension.                                    *
 * ctypex.h: ctype functions with unsigned char casting.                   *
 *                                                                         *
 * Copyright (c) 2018-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#ifdef __cplusplus
#  include <cctype>
#else
#  include <ctype.h>
#endif

#define ISALNUM(c) isalnum((unsigned char)(c))
#define ISALPHA(c) isalpha((unsigned char)(c))
#define ISCNTRL(c) iscntrl((unsigned char)(c))
#define ISDIGIT(c) isdigit((unsigned char)(c))
#define ISGRAPH(c) isgraph((unsigned char)(c))
#define ISLOWER(c) islower((unsigned char)(c))
#define ISPRINT(c) isprint((unsigned char)(c))
#define ISPUNCT(c) ispunct((unsigned char)(c))
#define ISSPACE(c) isspace((unsigned char)(c))
#define ISUPPER(c) isupper((unsigned char)(c))
#define ISXDIGIT(c) isxdigit((unsigned char)(c))

#define ISASCII(c) isascii((unsigned char)(c))
#define ISBLANK(c) isblank((unsigned char)(c))

#define TOUPPER(c) toupper((unsigned char)(c))
#define TOLOWER(c) tolower((unsigned char)(c))
