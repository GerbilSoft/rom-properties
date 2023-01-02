/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * SegaPublishers.hpp: Sega third-party publishers list.                   *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"

namespace LibRomData { namespace SegaPublishers {

/**
 * Look up a company code.
 * @param code Company code.
 * @return Publisher, or nullptr if not found.
 */
const char *lookup(unsigned int code);

} }
