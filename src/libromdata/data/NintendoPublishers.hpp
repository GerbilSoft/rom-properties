/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NintendoPublishers.hpp: Nintendo third-party publishers list.           *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_NINTENDOPUBLISHERS_HPP__
#define __ROMPROPERTIES_LIBROMDATA_NINTENDOPUBLISHERS_HPP__

#include "common.h"

// C includes.
#include <stdint.h>

namespace LibRomData { namespace NintendoPublishers {

/**
 * Look up a company code.
 * @param code Company code.
 * @return Publisher, or nullptr if not found.
 */
const char8_t *lookup(uint16_t code);

/**
 * Look up a company code.
 * @param code Company code.
 * @return Publisher, or nullptr if not found.
 */
const char8_t *lookup(const char *code);

/**
 * Look up a company code.
 * This uses the *old* company code, present in
 * older Game Boy titles.
 * @param code Company code.
 * @return Publisher, or nullptr if not found.
 */
const char8_t *lookup_old(uint8_t code);

/**
 * Look up a company code for FDS titles.
 * This uses the *old* company code format.
 * @param code Company code.
 * @return Publisher, or nullptr if not found.
 */
const char8_t *lookup_fds(uint8_t code);

} }

#endif /* __ROMPROPERTIES_LIBROMDATA_NINTENDOPUBLISHERS_HPP__ */
