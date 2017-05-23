/***************************************************************************
 * ROM Properties Page shell extension.                                    *
 * resource.common.inc.h: Common Win32 resource header.                    *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_RESOURCE_COMMON_INC_H__
#define __ROMPROPERTIES_RESOURCE_COMMON_INC_H__

#ifndef RC_INVOKED
#error This file should only be included in Win32 resource scripts.
#endif

// Windows SDK.
#include <windows.h>

/* program version */
#include "config.version.h"
/* git version */
#include "git.h"

/* Windows-style version number */
#define RP_VERSION_WIN32 RP_VERSION_MAJOR,RP_VERSION_MINOR,RP_VERSION_PATCH,RP_VERSION_DEVEL

/* Windows-style version string */
#ifdef RP_GIT_VERSION
# ifdef RP_GIT_DESCRIBE
#  define Win32_RC_FileVersion RP_VERSION_STRING "\r\n" RP_GIT_VERSION "\r\n" RP_GIT_DESCRIBE
# else /* !RP_GIT_DESCRIBE */
#  define Win32_RC_FileVersion RP_VERSION_STRING "\r\n" RP_GIT_VERSION
# endif /* RP_GIT_DESCRIBE */
#else /* !RP_GIT_VERSION */
# define Win32_RC_FileVersion RP_VERSION_STRING
#endif /* RP_GIT_VERSION */

/* File flags for VERSIONINFO */
#ifdef _DEBUG
# define RP_VS_FF_DEBUG VS_FF_DEBUG
#else
# define RP_VS_FF_DEBUG 0
#endif
#if RP_VERSION_DEVEL != 0
# define RP_VS_FF_PRERELEASE VS_FF_PRERELEASE
#else
# define RP_VS_FF_PRERELEASE 0
#endif

#define RP_VS_FILEFLAGS (RP_VS_FF_DEBUG | RP_VS_FF_PRERELEASE)

#endif /* __ROMPROPERTIES_RESOURCE_COMMON_INC_H__ */
