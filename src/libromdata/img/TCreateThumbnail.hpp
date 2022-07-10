/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * TCreateThumbnail.hpp: Thumbnail creator template.                       *
 *                                                                         *
 * Copyright (c) 2016-2021 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
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
	RPCT_SOURCE_FILE_CLASS_DISABLED	= 6,	// User configuration has disabled thumbnails for this class.
	RPCT_SOURCE_FILE_BAD_FS		= 7,	// Source file is located on a "bad" file system.
	RPCT_RUNNING_AS_ROOT		= 8,	// Running as root is not supported.
	RPCT_INVALID_IMAGE_SIZE		= 9,	// Invalid image size requested. (e.g. 0 or less)
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
 *
 * NOTE: This is a C function pointer. Keep it as `const char`.
 * Do not change this to `const char8_t`.
 *
 * @param source_file Source file [UTF-8]
 * @param output_file Output file [UTF-8]
 * @param maximum_size Maximum size
 * @return 0 on success; non-zero on error.
 */
typedef int (RP_C_API *PFN_RP_CREATE_THUMBNAIL)(const char *source_file, const char *output_file, int maximum_size);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include "librpbase/RomData.hpp"
#include "librptexture/img/rp_image.hpp"

// C++ includes.
#include <string>

namespace LibRpBase {
	class RomData;
};
namespace LibRpFile {
	class IRpFile;
}

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
		 * @param sBIT		[out,opt] sBIT metadata.
		 * @return Internal image, or null ImgClass on error.
		 */
		ImgClass getInternalImage(const LibRpBase::RomData *romData,
			LibRpBase::RomData::ImageType imageType,
			ImgSize *pOutSize = nullptr,
			LibRpTexture::rp_image::sBIT_t *sBIT = nullptr);

		/**
		 * Get an external image.
		 * @param romData	[in] RomData object.
		 * @param imageType	[in] Image type.
		 * @param req_size	[in] Requested image size.
		 * @param pOutSize	[out,opt] Pointer to ImgSize to store the image's size.
		 * @param sBIT		[out,opt] sBIT metadata.
		 * @return External image, or null ImgClass on error.
		 */
		ImgClass getExternalImage(
			const LibRpBase::RomData *romData, LibRpBase::RomData::ImageType imageType,
			int req_size, ImgSize *pOutSize = nullptr,
			LibRpTexture::rp_image::sBIT_t *sBIT = nullptr);

		/**
		 * getThumbnail() output parameters.
		 */
		struct GetThumbnailOutParams_t {
			ImgClass retImg;			// [out] Returned image.
			ImgSize thumbSize;			// [out] Thumbnail size.
			ImgSize fullSize;			// [out] Full image size.
			LibRpTexture::rp_image::sBIT_t sBIT;	// [out] sBIT metadata.
		};

		/**
		 * Create a thumbnail for the specified ROM file.
		 * @param romData	[in] RomData object
		 * @param reqSize	[in] Requested image size (single dimension; assuming square image)
		 * @param pOutParams	[out] Output parameters (If an error occurs, pOutParams->retImg will be null)
		 * @return 0 on success; non-zero on error.
		 */
		int getThumbnail(const LibRpBase::RomData *romData, int reqSize, GetThumbnailOutParams_t *pOutParams);

		/**
		 * Create a thumbnail for the specified ROM file.
		 * @param file		[in] Open IRpFile object
		 * @param reqSize	[in] Requested image size (single dimension; assuming square image)
		 * @param pOutParams	[out] Output parameters (If an error occurs, pOutParams->retImg will be null)
		 * @return 0 on success; non-zero on error.
		 */
		int getThumbnail(LibRpFile::IRpFile *file, int reqSize, GetThumbnailOutParams_t *pOutParams);

		/**
		 * Create a thumbnail for the specified ROM file.
		 * @param filename	[in] ROM file
		 * @param reqSize	[in] Requested image size (single dimension; assuming square image)
		 * @param pOutParams	[out] Output parameters (If an error occurs, pOutParams->retImg will be null)
		 * @return 0 on success; non-zero on error.
		 */
		int getThumbnail(const char8_t *filename, int reqSize, GetThumbnailOutParams_t *pOutParams);

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
		virtual ImgClass rpImageToImgClass(const LibRpTexture::rp_image *img) const = 0;

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
		 *
		 * This function may be a no-op in cases where ImgClass
		 * is not a pointer, e.g. QImage in Qt frontends.
		 *
		 * @param imgClass ImgClass object.
		 */
		virtual void freeImgClass(ImgClass &imgClass) const = 0;

		enum class ScalingMethod {
			Nearest = 0,
			Bilinear = 1,
		};

		/**
		 * Rescale an ImgClass using the specified scaling method.
		 * @param imgClass ImgClass object.
		 * @param sz New size.
		 * @param method Scaling method.
		 * @return Rescaled ImgClass.
		 */
		virtual ImgClass rescaleImgClass(const ImgClass &imgClass, const ImgSize &sz, ScalingMethod method = ScalingMethod::Nearest) const = 0;

		/**
		 * Get the size of the specified ImgClass.
		 * @param imgClass	[in] ImgClass object.
		 * @param pOutSize	[out] Pointer to ImgSize to store the image size.
		 * @return 0 on success; non-zero on error.
		 */
		virtual int getImgClassSize(const ImgClass &imgClass, ImgSize *pOutSize) const = 0;

		/**
		 * Get the proxy for the specified URL.
		 * @return Proxy, or empty string if no proxy is needed.
		 */
		virtual std::string proxyForUrl(const std::string &url) const = 0;
};

}
#endif /* __cplusplus */

#endif /* __ROMPROPERTIES_LIBROMDATA_IMG_TCREATETHUMBNAIL_HPP__ */
