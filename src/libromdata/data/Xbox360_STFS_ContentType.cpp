/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Xbox360_STFS_ContentType.cpp: Microsoft Xbox 360 STFS Content Type.     *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "Xbox360_STFS_ContentType.hpp"
#include "../Console/xbox360_stfs_structs.h"

namespace LibRomData { namespace Xbox360_STFS_ContentType {

struct ContentTypeEntry {
	uint32_t id;
	const char *contentType;
};

/**
 * Xbox 360 STFS content type list.
 * Reference: https://github.com/Free60Project/wiki/blob/master/STFS.md
 */
static const ContentTypeEntry contentTypeList[] = {
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
	{STFS_CONTENT_TYPE_XNA,			"XNA"},	// NOT localizable!
	{STFS_CONTENT_TYPE_LICENSE_STORE,	NOP_C_("Xbox360_STFS|ContentType", "License Store")},
	{STFS_CONTENT_TYPE_MOVIE,		NOP_C_("Xbox360_STFS|ContentType", "Movie")},
	{STFS_CONTENT_TYPE_TV,			NOP_C_("Xbox360_STFS|ContentType", "TV")},
	{STFS_CONTENT_TYPE_MUSIC_VIDEO,		NOP_C_("Xbox360_STFS|ContentType", "Music Video")},
	{STFS_CONTENT_TYPE_GAME_VIDEO,		NOP_C_("Xbox360_STFS|ContentType", "Game Video")},
	{STFS_CONTENT_TYPE_PODCAST_VIDEO,	NOP_C_("Xbox360_STFS|ContentType", "Podcast Video")},
	{STFS_CONTENT_TYPE_VIRAL_VIDEO,		NOP_C_("Xbox360_STFS|ContentType", "Viral Video")},
	{STFS_CONTENT_TYPE_COMMUNITY_GAME,	NOP_C_("Xbox360_STFS|ContentType", "Community Game")},

	{0, nullptr}
};

/** Public functions **/

/**
 * Look up an STFS content type.
 * @param contentType Content type.
 * @return Content type, or nullptr if not found.
 */
const char *lookup(uint32_t contentType)
{
	// Do a binary search.
	static const ContentTypeEntry *const pContentTypeList_end =
		&contentTypeList[ARRAY_SIZE(contentTypeList)];
	auto pContentType = std::lower_bound(contentTypeList, pContentTypeList_end, contentType,
		[](const ContentTypeEntry &cte, uint32_t contentType) noexcept -> bool {
			return (cte.id < contentType);
		});
	if (pContentType == pContentTypeList_end || pContentType->id != contentType) {
		return nullptr;
	}
	return dpgettext_expr(RP_I18N_DOMAIN, "Xbox360_STFS|ContentType", pContentType->contentType);
}

} }
