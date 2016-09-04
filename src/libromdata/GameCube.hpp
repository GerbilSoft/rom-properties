/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GameCube.hpp: Nintendo GameCube and Wii disc image reader.              *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_GAMECUBE_HPP__
#define __ROMPROPERTIES_LIBROMDATA_GAMECUBE_HPP__

#include <stdint.h>
#include <string>
#include "TextFuncs.hpp"

#include "RomData.hpp"

namespace LibRomData {

class GameCubePrivate;
class GameCube : public RomData
{
	public:
		// TODO: Some abstraction to read the file directory
		// using a wrapper around FILE*, QFile, etc.
		// For now, just check the header.

		/**
		 * Read a Nintendo GameCube or Wii disc image.
		 *
		 * A disc image must be opened by the caller. The file handle
		 * will be dup()'d and must be kept open in order to load
		 * data from the disc image.
		 *
		 * To close the file, either delete this object or call close().
		 *
		 * NOTE: Check isValid() to determine if this is a valid ROM.
		 *
		 * @param file Open disc image.
		 */
		GameCube(IRpFile *file);
		virtual ~GameCube();

	private:
		GameCube(const GameCube &);
		GameCube &operator=(const GameCube &);

	private:
		friend class GameCubePrivate;
		GameCubePrivate *const d;

	public:
		/** ROM detection functions. **/

		/**
		 * Is a ROM image supported by this class?
		 * @param info DetectInfo containing ROM detection information.
		 * @return Class-specific system ID (>= 0) if supported; -1 if not.
		 */
		static int isRomSupported_static(const DetectInfo *info);

		/**
		 * Is a ROM image supported by this object?
		 * @param info DetectInfo containing ROM detection information.
		 * @return Class-specific system ID (>= 0) if supported; -1 if not.
		 */
		virtual int isRomSupported(const DetectInfo *info) const override;

		/**
		 * Get the name of the system the loaded ROM is designed for.
		 * @return System name, or nullptr if not supported.
		 */
		virtual const rp_char *systemName(void) const override;

	public:
		/**
		 * Get a list of all supported file extensions.
		 * This is to be used for file type registration;
		 * subclasses don't explicitly check the extension.
		 *
		 * NOTE: The extensions include the leading dot,
		 * e.g. ".bin" instead of "bin".
		 *
		 * NOTE 2: The strings in the std::vector should *not*
		 * be freed by the caller.
		 *
		 * @return List of all supported file extensions.
		 */
		static std::vector<const rp_char*> supportedFileExtensions_static(void);

		/**
		 * Get a list of all supported file extensions.
		 * This is to be used for file type registration;
		 * subclasses don't explicitly check the extension.
		 *
		 * NOTE: The extensions include the leading dot,
		 * e.g. ".bin" instead of "bin".
		 *
		 * NOTE 2: The strings in the std::vector should *not*
		 * be freed by the caller.
		 *
		 * @return List of all supported file extensions.
		 */
		virtual std::vector<const rp_char*> supportedFileExtensions(void) const override;

		/**
		 * Get a bitfield of image types this class can retrieve.
		 * @return Bitfield of supported image types. (ImageTypesBF)
		 */
		virtual uint32_t supportedImageTypes(void) const override;

	protected:
		/**
		 * Load field data.
		 * Called by RomData::fields() if the field data hasn't been loaded yet.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		virtual int loadFieldData(void) override;

		/**
		 * Load URLs for an external media type.
		 * Called by RomData::extURL() if the URLs haven't been loaded yet.
		 * @param imageType Image type to load.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		virtual int loadURLs(ImageType imageType) override;
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_GAMECUBE_HPP__ */
