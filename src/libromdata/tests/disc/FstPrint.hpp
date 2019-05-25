/***************************************************************************
 * ROM Properties Page shell extension. (libromdata/tests)                 *
 * FstPrint.hpp: FST printer.                                              *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// C++ includes.
#include <ostream>

namespace LibRpBase {
	class IFst;
}

namespace LibRomData {

/**
 * Print an FST to an ostream.
 * @param fst	[in] FST to print.
 * @param os	[in,out] ostream.
 *
 * @return 0 on success; negative POSIX error code on error.
 */
int fstPrint(LibRpBase::IFst *fst, std::ostream &os);

};
