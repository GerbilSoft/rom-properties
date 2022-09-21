/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * EXENEEntries.cpp: EXE NE Entry ordinal data                             *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * Copyright (c) 2022 by Egor.                                             *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "EXENEEntries.hpp"
#include "Other/exe_structs.h"

namespace LibRomData { namespace EXENEEntries {

struct OrdinalName {
	uint16_t ordinal;
	const char *name;
};

struct OrdinalNameTable {
	const char *modname;
	const OrdinalName *table;
	size_t count;
};

// see EXENEEntries_info.md for more info on how this file was generated.
#include "EXENEEntries_data.h"

/**
 * Look up an ordinal.
 * @param modname The module name
 * @param ordinal The ordinal
 * @return Name for the ordinal, or nullptr if not found.
 */
const char *lookup_ordinal(const std::string &modname, uint16_t ordinal)
{
	auto it = std::lower_bound(entries, entries+ARRAY_SIZE(entries), modname,
		[](const OrdinalNameTable &lhs, const std::string &rhs) -> bool {
			return strcasecmp(lhs.modname, rhs.c_str()) < 0;
		});
	if (it == entries+ARRAY_SIZE(entries) || strcmp(it->modname, modname.c_str()) != 0)
		return nullptr;

	auto it2 = std::lower_bound(it->table, it->table+it->count, ordinal,
		[](const OrdinalName &lhs, uint16_t rhs) -> bool {
			return lhs.ordinal < rhs;
		});
	if (it2 == it->table+it->count || it2->ordinal != ordinal)
		return nullptr;
	return it2->name;
}

} }
