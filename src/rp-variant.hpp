/*****************************************************************************
 * ROM Properties Page shell extension.                                      *
 * rp-variant.h: std::variant wrapper header.                                *
 *                                                                           *
 * Copyright (c) 2025-2026 by David Korth.                                   *
 * SPDX-License-Identifier: GPL-2.0-or-later                                 *
 *****************************************************************************/

#pragma once

#ifdef __cplusplus

#include "config.libc.h"

#ifdef HAVE_STD_VARIANT
#  include <variant>
#else /* !HAVE_STD_VARIANT */
// std::variant<> is not available on this system.
// Use mpark variant instead.
#  include "mpark/variant.hpp"
namespace std {
	using mpark::variant;
	using mpark::holds_alternative;
	using mpark::get;
	using mpark::monostate;
}
#endif /* HAVE_STD_VARIANT */

#endif /* __cplusplus */
