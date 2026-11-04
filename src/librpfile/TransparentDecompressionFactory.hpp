/*****************************************************************************
 * ROM Properties Page shell extension. (librpfile)                          *
 * TransparentDecompressionFactory.hpp: Check for transparent decompression. *
 *                                                                           *
 * Copyright (c) 2026 by David Korth.                                        *
 * SPDX-License-Identifier: GPL-2.0-or-later                                 *
 *****************************************************************************/

#pragma once

#include "IRpFile.hpp"

namespace LibRpFile { namespace TransparentDecompressionFactory {

/**
 * Check if a file is compressed using a supported compression scheme.
 * If it is, open a transparent decompression handler IRpFile subclass
 * and return it.
 * @param file IRpFile to check
 * @return Transparent decompression handler IRpFile, or nullptr if not compressed.
 */
RP_LIBROMDATA_PUBLIC
IRpFilePtr create(const IRpFilePtr &file);

} } // namespace LibRpFile::TransparentDecompressionFactory
