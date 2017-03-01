/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * TCreateThumbnail.hpp: Thumbnail creator template.                       *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_IMG_TCREATETHUMBNAIL_HPP__
#define __ROMPROPERTIES_LIBROMDATA_IMG_TCREATETHUMBNAIL_HPP__

/**
 * NOTE: TCreateThumbnail.cpp MUST be #included by a file in
 * the UI frontend. Otherwise, the instantiated template won't
 * be compiled correctly.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * TCreateThumbnail::create() errors.
 */
typedef enum {
	RPCT_SUCCESS			= 0,
	RPCT_DLL_ERROR			= 1,	// Cannot load the shared library.
	RPCT_SOURCE_FILE_ERROR		= 2,	// Cannot open the source file.
	RPCT_SOURCE_FILE_NOT_SUPPORTED	= 3,	// Source file isn't supported.
	RPCT_SOURCE_FILE_NO_IMAGE	= 4,	// Source file has no image.
	RPCT_OUTPUT_FILE_FAILED		= 5,	// Failed to save the output file.
} RpCreateThumbnailError;

/**
 * Thumbnail nearest-neighbor upscaling policy.
 * TODO: Make this configurable.
 */
typedef enum {
	RESIZE_UP_NONE,	// No resizing.

	// Only resize images that are less than or equal to half the
	// requested thumbnail size. This is a compromise to allow
	// small icons like Nintendo DS icons to be enlarged while
	// larger but not-quite 256px images like GameTDB disc scans'
	// (160px) will remain as-is.
	RESIZE_UP_HALF,

	// Resize all images that are smaller than the requested
	// thumbnail size.
	RESIZE_UP_ALL,
} ResizeNearestUpPolicy;

/**
 * rp_create_thumbnail() function pointer.
 * Used for wrapper programs that don't link to libromdata directly.
 * @param source_file Source file. (UTF-8)
 * @param output_file Output file. (UTF-8)
 * @param maximum_size Maximum size.
 * @return 0 on success; non-zero on error.
 */
typedef int (*PFN_RP_CREATE_THUMBNAIL)(const char *source_file, const char *output_file, int maximum_size);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include "../RomData.hpp"

namespace LibRomData {

template<typename ImgClass>
class TCreateThumbnail
{
	public:
		TCreateThumbnail();
		virtual ~TCreateThumbnail();
	private:
		RP_DISABLE_COPY(TCreateThumbnail)

	public:
		/**
		 * Image size struct.
		 */
		struct ImgSize {
			int width;
			int height;
		};

		/**
		 * Get an internal image.
		 * @param romData	[in] RomData object.
		 * @param imageType	[in] Image type.
		 * @param pOutSize	[out,opt] Pointer to ImgSize to store the image's size.
		 * @return Internal image, or null ImgClass on error.
		 */
		ImgClass getInternalImage(const RomData *romData,
			RomData::ImageType imageType,
			ImgSize *pOutSize);

		/**
		 * Get an external image.
		 * @param romData	[in] RomData object.
		 * @param imageType	[in] Image type.
		 * @param req_size	[in] Requested image size.
		 * @param pOutSize	[out,opt] Pointer to ImgSize to store the image's size.
		 * @return External image, or null ImgClass on error.
		 */
		ImgClass getExternalImage(
			const RomData *romData, RomData::ImageType imageType,
			int req_size, ImgSize *pOutSize);

		/**
		 * Create a thumbnail for the specified ROM file.
		 * @param romData	[in] RomData object.
		 * @param req_size	[in] Requested image size.
		 * @param ret_img	[out] Return image.
		 * @return 0 on success; non-zero on error.
		 */
		int getThumbnail(const RomData *romData, int req_size, ImgClass &ret_img);

		/**
		 * Create a thumbnail for the specified ROM file.
		 * @param file		[in] Open IRpFile object.
		 * @param req_size	[in] Requested image size.
		 * @param ret_img	[out] Return image.
		 * @return 0 on success; non-zero on error.
		 */
		int getThumbnail(IRpFile *file, int req_size, ImgClass &ret_img);

		/**
		 * Create a thumbnail for the specified ROM file.
		 * @param filename	[in] ROM file.
		 * @param req_size	[in] Requested image size.
		 * @param ret_img	[out] Return image.
		 * @return 0 on success; non-zero on error.
		 */
		int getThumbnail(const rp_char *filename, int req_size, ImgClass &ret_img);

	protected:
		/**
		 * Rescale a size while maintaining the aspect ratio.
		 * Based on Qt 4.8's QSize::scale().
		 * @param rs_size	[in,out] Original size, which will be rescaled.
		 * @param tgt_size	[in] Target size.
		 */
		static inline void rescale_aspect(ImgSize &rs_size, const ImgSize &tgt_size);

	protected:
		/** Pure virtual functions. **/

		/**
		 * Wrapper function to convert rp_image* to ImgClass.
		 * @param img rp_image
		 * @return ImgClass
		 */
		virtual ImgClass rpImageToImgClass(const rp_image *img) const = 0;

		/**
		 * Wrapper function to check if an ImgClass is valid.
		 * @param imgClass ImgClass
		 * @return True if valid; false if not.
		 */
		virtual bool isImgClassValid(const ImgClass &imgClass) const = 0;

		/**
		 * Wrapper function to get a "null" ImgClass.
		 * @return "Null" ImgClass.
		 */
		virtual ImgClass getNullImgClass(void) const = 0;

		/**
		 * Free an ImgClass object.
		 * This may be no-op for e.g. QImage.
		 * @param imgClass ImgClass object.
		 */
		virtual void freeImgClass(ImgClass &imgClass) const = 0;

		/**
		 * Rescale an ImgClass using nearest-neighbor scaling.
		 * @param imgClass ImgClass object.
		 * @param sz New size.
		 * @return Rescaled ImgClass.
		 */
		virtual ImgClass rescaleImgClass(const ImgClass &imgClass, const ImgSize &sz) const = 0;

		/**
		 * Get the proxy for the specified URL.
		 * @return Proxy, or empty string if no proxy is needed.
		 */
		virtual rp_string proxyForUrl(const rp_string &url) const = 0;
};

}
#endif /* __cplusplus */

#endif /* __ROMPROPERTIES_LIBROMDATA_IMG_TCREATETHUMBNAIL_HPP__ */
