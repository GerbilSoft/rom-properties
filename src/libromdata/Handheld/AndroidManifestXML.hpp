/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * AndroidManifestXML.hpp: Android Manifest XML reader.                    *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#ifndef ENABLE_XML
#  error XML must be enabled in order to use AndroidManifestXML
#endif /* ENABLE_XML */

#include <pugixml.hpp>

namespace LibRomData {

ROMDATA_DECL_BEGIN(AndroidManifestXML)
ROMDATA_DECL_METADATA()
ROMDATA_DECL_DANGEROUS()

public:
	/**
	 * Get the decompressed XML document, as parsed by PugiXML.
	 * @return xml_document, or nullptr if it wasn't loaded.
	 */
	const pugi::xml_document *getXmlDocument(void) const;

	/**
	 * Take ownership of the decompressed XML document, as parsed by PugiXML.
	 * @return xml_document, or nullptr if it wasn't loaded.
	 */
	pugi::xml_document *takeXmlDocument(void);

ROMDATA_DECL_END()

}
