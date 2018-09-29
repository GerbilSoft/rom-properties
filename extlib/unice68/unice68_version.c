/*
 * @file    unice68_version.c
 * @brief   Library, packer and depacker version
 * @author  http://sourceforge.net/users/benjihan
 *
 * Copyright (c) 1998-2016 Benjamin Gerard
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.
 *
 * If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#ifndef PACKAGE_NAME
# define PACKAGE_NAME "unice68"
#endif
#ifndef PACKAGE_VERSION
# define PACKAGE_VERSION __DATE__
#endif
#ifndef PACKAGE_VERNUM
# define PACKAGE_VERNUM 0
#endif

#include "unice68_private.h"
#include "unice68.h"

int unice68_unice_version(void)
{
  return 0x240;
}

int unice68_ice_version(void)
{
  return 0x235;
}

const char * unice68_versionstr(void)
{
  return PACKAGE_STRING " ICE! 2.35/2.40" ;
}

int unice68_version(void)
{
  unsigned int v[4] = {0,0,0,0};      /* major, minor, patch, tweak */
  const char * s = unice68_versionstr();
  int i, c;

  /* find 1st <space> ' ' */
  while (c = *s++, (c && c != ' '));
  for (i=0; c && i<4; ++i) {
    /* find 1st digit [0-9] */
    while (c = *s++, (c && c<'0' && c>'9'));
    for (; c >= '0' && c <= '9'; c = *s++)
      v[i] = v[i]*10 + ( c - '0' );
  }

  if (v[0] >= 19700101 && !v[2] && !v[3]) {
    /* Date conversion (after year 2047 32bit int be negative). */
    unsigned int y = v[0]/1000u, m = v[0]/100u%100u, d=v[0]%100u;
    return (y<<21) | (m<<17) | (d<<12) | v[1];
  }

  return 0
    + (v[0] << 28)
    + (v[1] << 20)
    + (v[2] << 12)
    + v[3]
    ;
}
