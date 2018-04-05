/***************************************************************************
 * ROM Properties Page shell extension. (libromdata/tests)                 *
 * FstPrint.hpp: FST printer.                                              *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
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
