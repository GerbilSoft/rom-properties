/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * IsDriveLetter.hpp: Determine if a character is a valid drive letter.    *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

template<typename CharType>
static inline constexpr bool T_IsDriveLetter(CharType letter)
{
	return (letter >= (CharType)'A') && (letter <= (CharType)'Z');
}

static inline constexpr bool IsDriveLetterA(char letter)
{
	return (letter >= 'A') && (letter <= 'Z');
}

static inline constexpr bool IsDriveLetterW(wchar_t letter)
{
	return (letter >= L'A') && (letter <= L'Z');
}

#ifdef _UNICODE
#  define IsDriveLetter(x) IsDriveLetterW(x)
#else /* !_UNICODE */
#  define IsDriveLetter(x) IsDriveLetterA(x)
#endif /* _UNICODE */
