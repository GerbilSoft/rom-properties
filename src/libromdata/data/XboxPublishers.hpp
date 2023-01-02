/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * XboxPublishers.hpp: Xbox third-party publishers list.                   *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <stdint.h>

namespace LibRomData { namespace XboxPublishers {

/**
 * Look up a company code.
 * @param code Company code.
 * @return Publisher, or nullptr if not found.
 */
const char *lookup(uint16_t code);

/**
 * Look up a company code.
 * @param code Company code.
 * @return Publisher, or nullptr if not found.
 */
const char *lookup(const char *code);

} }
