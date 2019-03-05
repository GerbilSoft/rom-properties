/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Xbox360_STFS_ContentType.cpp: Microsoft Xbox 360 STFS Content Type.     *
 *                                                                         *
* Copyright (c) 2016-2019 by David Korth.                                 *
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

#include "Xbox360_STFS_ContentType.hpp"
#include "../Console/xbox360_stfs_structs.h"

// libi18n
#include "libi18n/i18n.h"

// C includes.
#include <stdlib.h>

namespace LibRomData {

class Xbox360_STFS_ContentTypePrivate {
	private:
		// Static class.
		Xbox360_STFS_ContentTypePrivate();
		~Xbox360_STFS_ContentTypePrivate();
		RP_DISABLE_COPY(Xbox360_STFS_ContentTypePrivate)

	public:
		/**
		 * Xbox 360 STFS content type list.
		 * Reference: https://github.com/Free60Project/wiki/blob/master/STFS.md
		 */
		struct ContentTypeEntry {
			uint32_t id;
			const char *contentType;
		};
		static const ContentTypeEntry contentTypeList[];

		/**
		 * Comparison function for bsearch().
		 * @param a
		 * @param b
		 * @return
		 */
		static int RP_C_API compar(const void *a, const void *b);

};

/**
 * Sega third-party publisher list.
 * Reference: http://segaretro.org/Third-party_T-series_codes
 */
const Xbox360_STFS_ContentTypePrivate::ContentTypeEntry Xbox360_STFS_ContentTypePrivate::contentTypeList[] = {
	{STFS_CONTENT_TYPE_SAVED_GAME,		NOP_C_("Xbox360_STFS|ContentType", "Saved Game")},
	{STFS_CONTENT_TYPE_MARKETPLACE_CONTENT,	NOP_C_("Xbox360_STFS|ContentType", "Marketplace Content")},
	{STFS_CONTENT_TYPE_PUBLISHER,		NOP_C_("Xbox360_STFS|ContentType", "Publisher")},
	{STFS_CONTENT_TYPE_XBOX_360_TITLE,	NOP_C_("Xbox360_STFS|ContentType", "Xbox 360 Game")},
	{STFS_CONTENT_TYPE_IPTV_PAUSE_BUFFER,	NOP_C_("Xbox360_STFS|ContentType", "IPTV Pause Buffer")},
	{STFS_CONTENT_TYPE_INSTALLED_GAME,	NOP_C_("Xbox360_STFS|ContentType", "Installed Game")},
	{STFS_CONTENT_TYPE_XBOX_ORIGINAL_GAME,	NOP_C_("Xbox360_STFS|ContentType", "Original Xbox Game")},
	{STFS_CONTENT_TYPE_AVATAR_ITEM,		NOP_C_("Xbox360_STFS|ContentType", "Avatar Item")},
	{STFS_CONTENT_TYPE_PROFILE,		NOP_C_("Xbox360_STFS|ContentType", "Profile")},
	{STFS_CONTENT_TYPE_GAMER_PICTURE,	NOP_C_("Xbox360_STFS|ContentType", "Gamer Picture")},
	{STFS_CONTENT_TYPE_THEME,		NOP_C_("Xbox360_STFS|ContentType", "Theme")},
	{STFS_CONTENT_TYPE_CACHE_FILE,		NOP_C_("Xbox360_STFS|ContentType", "Cache File")},
	{STFS_CONTENT_TYPE_STORAGE_DOWNLOAD,	NOP_C_("Xbox360_STFS|ContentType", "Storage Download")},
	// TODO: Verify.
	{STFS_CONTENT_TYPE_XBOX_SAVED_GAME,	NOP_C_("Xbox360_STFS|ContentType", "Original Xbox Saved Game")},
	// TODO: Verify.
	{STFS_CONTENT_TYPE_XBOX_DOWNLOAD,	NOP_C_("Xbox360_STFS|ContentType", "Original Xbox Download")},
	{STFS_CONTENT_TYPE_GAME_DEMO,		NOP_C_("Xbox360_STFS|ContentType", "Game Demo")},
	{STFS_CONTENT_TYPE_VIDEO,		NOP_C_("Xbox360_STFS|ContentType", "Video")},
	{STFS_CONTENT_TYPE_GAME_TITLE,		NOP_C_("Xbox360_STFS|ContentType", "Game")},
	{STFS_CONTENT_TYPE_INSTALLER,		NOP_C_("Xbox360_STFS|ContentType", "Installer")},
	{STFS_CONTENT_TYPE_GAME_TRAILER,	NOP_C_("Xbox360_STFS|ContentType", "Game Trailer")},
	{STFS_CONTENT_TYPE_ARCADE_TITLE,	NOP_C_("Xbox360_STFS|ContentType", "Arcade Game")},
	{STFS_CONTENT_TYPE_XNA,			NOP_C_("Xbox360_STFS|ContentType", "XNA")},
	{STFS_CONTENT_TYPE_LICENSE_STORE,	NOP_C_("Xbox360_STFS|ContentType", "License Store")},
	{STFS_CONTENT_TYPE_MOVIE,		NOP_C_("Xbox360_STFS|ContentType", "Movie")},
	{STFS_CONTENT_TYPE_TV,			NOP_C_("Xbox360_STFS|ContentType", "TV")},
	{STFS_CONTENT_TYPE_MUSIC_VIDEO,		NOP_C_("Xbox360_STFS|ContentType", "Music Video")},
	{STFS_CONTENT_TYPE_PODCAST_VIDEO,	NOP_C_("Xbox360_STFS|ContentType", "Podcast Video")},
	{STFS_CONTENT_TYPE_VIRAL_VIDEO,		NOP_C_("Xbox360_STFS|ContentType", "Viral Video")},
	{STFS_CONTENT_TYPE_GAME_VIDEO,		NOP_C_("Xbox360_STFS|ContentType", "Game Video")},
	{STFS_CONTENT_TYPE_COMMUNITY_GAME,	NOP_C_("Xbox360_STFS|ContentType", "Community Game")},

	{0, nullptr}
};

/**
 * Comparison function for bsearch().
 * @param a
 * @param b
 * @return
 */
int RP_C_API Xbox360_STFS_ContentTypePrivate::compar(const void *a, const void *b)
{
	uint32_t id1 = static_cast<const ContentTypeEntry*>(a)->id;
	uint32_t id2 = static_cast<const ContentTypeEntry*>(b)->id;
	if (id1 < id2) return -1;
	if (id1 > id2) return 1;
	return 0;
}

/**
 * Look up an STFS content type.
 * @param contentType Content type.
 * @return Content type, or nullptr if not found.
 */
const char *Xbox360_STFS_ContentType::lookup(uint32_t contentType)
{
	// Do a binary search.
	const Xbox360_STFS_ContentTypePrivate::ContentTypeEntry key = {contentType, nullptr};
	const Xbox360_STFS_ContentTypePrivate::ContentTypeEntry *res =
		static_cast<const Xbox360_STFS_ContentTypePrivate::ContentTypeEntry*>(bsearch(&key,
			Xbox360_STFS_ContentTypePrivate::contentTypeList,
			ARRAY_SIZE(Xbox360_STFS_ContentTypePrivate::contentTypeList)-1,
			sizeof(Xbox360_STFS_ContentTypePrivate::ContentTypeEntry),
			Xbox360_STFS_ContentTypePrivate::compar));
	return (res
		? dpgettext_expr(RP_I18N_DOMAIN, "Xbox360_STFS|ContentType", res->contentType)
		: nullptr);
}

}
