/*****************************************************************************
 * ROM Properties Page shell extension. (libromdata)                         *
 * pngcheck.hpp: pngcheck library functions                                  *
 *                                                                           *
 * Modifications for rom-properties Copyright (c) 2016 by David Korth.       *
 *                                                                           *
 * Copyright 1995-2007 by Alexander Lehmann <lehmann@usa.net>,               *
 *                        Andreas Dilger <adilger@enel.ucalgary.ca>,         *
 *                        Glenn Randers-Pehrson <randeg@alum.rpi.edu>,       *
 *                        Greg Roelofs <newt@pobox.com>,                     *
 *                        John Bowler <jbowler@acm.org>,                     *
 *                        Tom Lane <tgl@sss.pgh.pa.us>                       *
 *                                                                           *
 * Permission to use, copy, modify, and distribute this software and its     *
 * documentation for any purpose and without fee is hereby granted, provided *
 * that the above copyright notice appear in all copies and that both that   *
 * copyright notice and this permission notice appear in supporting          *
 * documentation.  This software is provided "as is" without express or      *
 * implied warranty.                                                         *
 *                                                                           *
 *****************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_IMG_PNGCHECK_PNGCHECK_HPP__
#define __ROMPROPERTIES_LIBROMDATA_IMG_PNGCHECK_PNGCHECK_HPP__

namespace LibRomData {

class IRpFile;

int pngcheck(IRpFile *fp, int searching);

}

#endif /* __ROMPROPERTIES_LIBROMDATA_IMG_PNGCHECK_PNGCHECK_HPP__ */
