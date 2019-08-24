/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ValveVTF.hpp: Valve VTF texture reader.                                 *
 *                                                                         *
 * Copyright (c) 2017-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPTEXTURE_FILEFORMAT_VALVEVTF_HPP__
#define __ROMPROPERTIES_LIBRPTEXTURE_FILEFORMAT_VALVEVTF_HPP__

#include "FileFormat.hpp"

namespace LibRpBase {
	class IRpFile;
}

namespace LibRpTexture {

class ValveVTFPrivate;
class ValveVTF : public FileFormat
{
	public:
		explicit ValveVTF(LibRpBase::IRpFile *file);

	private:
		typedef FileFormat super;
		RP_DISABLE_COPY(ValveVTF)
	protected:
		friend class ValveVTFPrivate;

	public:
		/** Propety accessors **/

		/**
		 * Get the texture format name.
		 * @return Texture format name, or nullptr on error.
		 */
		const char *textureFormatName(void) const final;

		/**
		 * Get the pixel format, e.g. "RGB888" or "DXT1".
		 * @return Pixel format, or nullptr if unavailable.
		 */
		const char *pixelFormat(void) const final;

		/**
		 * Get the mipmap count.
		 * @return Number of mipmaps. (0 if none; -1 if format doesn't support mipmaps)
		 */
		int mipmapCount(void) const final;

#ifdef ENABLE_LIBRPBASE_ROMFIELDS
	public:
		/**
		 * Get property fields for rom-properties.
		 * @param fields RomFields object to which fields should be added.
		 * @return Number of fields added, or 0 on error.
		 */
		int getFields(LibRpBase::RomFields *fields) const final;
#endif /* ENABLE_LIBRPBASE_ROMFIELDS */

	public:
		/** Image accessors **/

		/**
		 * Get the image.
		 * For textures with mipmaps, this is the largest mipmap.
		 * The image is owned by this object.
		 * @return Image, or nullptr on error.
		 */
		const rp_image *image(void) const final;

		/**
		 * Get the image for the specified mipmap.
		 * Mipmap 0 is the largest image.
		 * @param num Mipmap number.
		 * @return Image, or nullptr on error.
		 */
		const rp_image *mipmap(int num) const final;
};

}

#endif /* __ROMPROPERTIES_LIBRPTEXTURE_FILEFORMAT_VALVEVTF_HPP__ */
