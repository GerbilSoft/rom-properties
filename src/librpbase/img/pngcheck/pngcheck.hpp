/*****************************************************************************
 * ROM Properties Page shell extension. (librpbase)                          *
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

#ifndef __ROMPROPERTIES_LIBRPBASE_IMG_PNGCHECK_PNGCHECK_HPP__
#define __ROMPROPERTIES_LIBRPBASE_IMG_PNGCHECK_PNGCHECK_HPP__

namespace LibRpBase {

class IRpFile;

enum {
  kOK = 0,
  kWarning,           /* could be an error in some circumstances but not all */
  kCommandLineError,  /* pilot error */
  kMinorError,        /* minor spec errors (e.g., out-of-range values) */
  kMajorError,        /* file corruption, invalid chunk length/layout, etc. */
  kCriticalError      /* unexpected EOF or other file(system) error */
};

/**
 * Check a PNG file for errors.
 * @param fp PNG file.
 * @return kOK on success; other value on error.
 */
int pngcheck(IRpFile *fp);

}

#endif /* __ROMPROPERTIES_LIBRPBASE_IMG_PNGCHECK_PNGCHECK_HPP__ */
