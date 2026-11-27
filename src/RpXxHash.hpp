/***************************************************************************
 * ROM Properties Page shell extension.                                    *
 * RpXxHash.hpp: std::hash<> implementations using XxHash.                 *
 * Anything using this file must link in libxxhash!                        *
 *                                                                         *
 * Copyright (c) 2026 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "config.xxhash.h"

// for if_constexpr
#include "compiler-attrs.h"

// C++ STL classes
#include <string>

// libxxhash
#include "xxhash.h"

// Using XXH32 on 32-bit and XXH64 on 64-bit.
// NOTE: If libxxhash is unavailable, these will fall back to implementations of std::hash<>.

namespace RpXxHash {

#ifdef HAVE_XXHASH

static constexpr uint32_t RP_XXH32_seed = 0x0115D8CF;
static constexpr uint64_t RP_XXH64_seed = 0x1991062319921124;

template<typename T>
struct BasicStringHash
{
	size_t operator()(const std::basic_string<T> &str) const noexcept
	{
		if_constexpr (sizeof(size_t) > 4) {
			return XXH64(str.data(), str.size() * sizeof(T), RP_XXH32_seed);
		} else {
			return XXH32(str.data(), str.size() * sizeof(T), RP_XXH64_seed);
		}
	}
};

using StringHash = BasicStringHash<char>;
using WStringHash = BasicStringHash<wchar_t>;
using u16StringHash = BasicStringHash<char16_t>;
using u32StringHash = BasicStringHash<char32_t>;

#else /* !HAVE_XXHASH */

using StringHash = std::hash<std::string>;
using WStringHash = std::hash<std::wstring>;
using u16StringHash = std::hash<std::u16string>;
using u32StringHash = std::hash<std::u32string>;

#endif /* HAVE_XXHASH */

} // namespace RpXxHash
