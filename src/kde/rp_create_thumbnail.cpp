/*****************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                           *
 * rp_create_thumbnail.cpp: Thumbnail creation function export for rp-stub.  *
 *                                                                           *
 * Copyright (c) 2016-2024 by David Korth.                                   *
 * SPDX-License-Identifier: GPL-2.0-or-later                                 *
 *****************************************************************************/

#include "stdafx.h"
#include "check-uid.hpp"

#include "plugins/RomThumbCreator_p.hpp"
#include "RpQUrl.hpp"

#include "RpQImageBackend.hpp"

// Other rom-properties libraries
#include "libromdata/RomDataFactory.hpp"
#include "librpfile/FileSystem.hpp"
using LibRpText::rp_sprintf;
using namespace LibRpFile;
using namespace LibRomData;

// C++ STL classes
using std::string;

/**
 * Thumbnail creator function for wrapper programs. (v2)
 * @param source_file Source file (UTF-8)
 * @param output_file Output file (UTF-8)
 * @param maximum_size Maximum size
 * @param flags Flags (see RpCreateThumbnailFlags)
 * @return 0 on success; non-zero on error.
 */
extern "C"
Q_DECL_EXPORT int RP_C_API rp_create_thumbnail2(const char *source_file, const char *output_file, int maximum_size, unsigned int flags)
{
	// NOTE: TCreateThumbnail() has wrappers for opening the
	// ROM file and getting RomData*, but we're doing it here
	// in order to return better error codes.
	CHECK_UID_RET(RPCT_ERROR_RUNNING_AS_ROOT);

	// Validate flags.
	if ((flags & ~RPCT_FLAG_VALID_MASK) != 0) {
		return RPCT_ERROR_INVALID_FLAGS;
	}

	// Register RpQImageBackend.
	// TODO: Static initializer somewhere?
	rp_image::setBackendCreatorFn(RpQImageBackend::creator_fn);

	// TODO: Check enableThumbnailOnNetworkFS
	RomDataPtr romData;

	// Check if this is a directory.
	const QUrl localUrl = localizeQUrl(QUrl(QString::fromUtf8(source_file)));
	const string s_local_filename = localUrl.toLocalFile().toUtf8().constData();
	if (unlikely(!s_local_filename.empty() && FileSystem::is_directory(s_local_filename.c_str()))) {
		// Directory: Call RomDataFactory::create() with the filename.
		romData = RomDataFactory::create(s_local_filename.c_str());
	} else {
		// File: Open the file and call RomDataFactory::create() with the opened file.

		// Attempt to open the ROM file.
		const IRpFilePtr file(openQUrl(localUrl, true));
		if (!file) {
			// Could not open the file.
			return RPCT_ERROR_CANNOT_OPEN_SOURCE_FILE;
		}

		// Get the appropriate RomData class for this ROM.
		// RomData class *must* support at least one image type.
		romData = RomDataFactory::create(file, RomDataFactory::RDA_HAS_THUMBNAIL);
	}

	if (!romData) {
		// ROM is not supported.
		return RPCT_ERROR_SOURCE_FILE_NOT_SUPPORTED;
	}

	// Create the thumbnail.
	RomThumbCreatorPrivate *const d = new RomThumbCreatorPrivate();
	RomThumbCreatorPrivate::GetThumbnailOutParams_t outParams;
	int ret = d->getThumbnail(romData, maximum_size, &outParams);
	delete d;

	if (ret != 0 || outParams.retImg.isNull()) {
		// No image.
		return RPCT_ERROR_SOURCE_FILE_NO_IMAGE;
	}

	// Save the image using RpPngWriter.
	const int height = outParams.retImg.height();

	/** tEXt chunks. **/
	// NOTE: These are written before IHDR in order to put the
	// tEXt chunks before the IDAT chunk.

	// NOTE: QString::toStdString() uses toAscii() in Qt4.
	// Hence, we'll use toUtf8() manually.

	// Get values for the XDG thumbnail cache text chunks.
	// KDE uses this order: Software, MTime, Mimetype, Size, URI
	RpPngWriter::kv_vector kv;

	// Determine the image format.
	rp_image::Format format;
	switch (outParams.retImg.format()) {
		case QImage::Format_Indexed8:
			format = rp_image::Format::CI8;
			break;
		case QImage::Format_ARGB32:
			format = rp_image::Format::ARGB32;
			break;
		default:
			// Unsupported...
			assert(!"Unsupported QImage image format.");
			return RPCT_ERROR_OUTPUT_FILE_FAILED;
	}

	unique_ptr<RpPngWriter> pngWriter(new RpPngWriter(output_file,
		outParams.retImg.width(), height, format));
	if (!pngWriter->isOpen()) {
		// Could not open the PNG writer.
		return RPCT_ERROR_OUTPUT_FILE_FAILED;
	}

	const bool doXDG = !(flags & RPCT_FLAG_NO_XDG_THUMBNAIL_METADATA);
	kv.reserve(doXDG ? 5 : 1);

	// Software
	kv.emplace_back("Software", "ROM Properties Page shell extension (" RP_KDE_UPPER ")");

	if (doXDG) {
		// Local filename
		QString qs_source_filename;
		if (localUrl.scheme().isEmpty() || localUrl.isLocalFile()) {
			qs_source_filename = localUrl.toLocalFile();
		}

		// FIXME: Local files only. Figure out how to handle this for remote.
		if (!qs_source_filename.isEmpty()) {
			const QFileInfo fi_src(qs_source_filename);

			// Modification time
			const int64_t mtime = fi_src.lastModified().toMSecsSinceEpoch() / 1000;
			if (mtime > 0) {
				kv.emplace_back("Thumb::MTime", rp_sprintf("%" PRId64, mtime));
			}

			// File size
			const off64_t szFile = fi_src.size();
			if (szFile > 0) {
				kv.emplace_back("Thumb::Size", rp_sprintf("%" PRId64, szFile));
			}
		}

		// MIME type
		const char *const mimeType = romData->mimeType();
		if (mimeType) {
			kv.emplace_back("Thumb::Mimetype", mimeType);
		}

		// Original image dimensions
		if (outParams.fullSize.width > 0 && outParams.fullSize.height > 0) {
			char imgdim_str[16];
			snprintf(imgdim_str, sizeof(imgdim_str), "%d", outParams.fullSize.width);
			kv.emplace_back("Thumb::Image::Width", imgdim_str);
			snprintf(imgdim_str, sizeof(imgdim_str), "%d", outParams.fullSize.height);
			kv.emplace_back("Thumb::Image::Height", imgdim_str);
		}

		// URI
		// NOTE: KDE desktops don't urlencode spaces or non-ASCII characters.
		// GTK+ desktops *do* urlencode spaces and non-ASCII characters.
		// FIXME: Do we want to store the local URI or the original URI?
		kv.emplace_back("Thumb::URI", localUrl.toEncoded().constData());
	}

	// Write the tEXt chunks.
	pngWriter->write_tEXt(kv);

	/** IHDR **/

	// CI8 palette.
	// This will be an empty vector if the image isn't CI8.
	// RpPngWriter will ignore the palette arguments in that case.
	const QVector<QRgb> colorTable = outParams.retImg.colorTable();

	// If sBIT wasn't found, all fields will be 0.
	// RpPngWriter will ignore sBIT in this case.
	int pwRet = pngWriter->write_IHDR(&outParams.sBIT,
		colorTable.constData(), colorTable.size());
	if (pwRet != 0) {
		// Error writing IHDR.
		// TODO: Unlink the PNG image.
		return RPCT_ERROR_OUTPUT_FILE_FAILED;
	}

	/** IDAT chunk **/

	// Initialize the row pointers.
	unique_ptr<const uint8_t*[]> row_pointers(new const uint8_t*[height]);
	const uint8_t *bits = outParams.retImg.bits();
	const int bytesPerLine = outParams.retImg.bytesPerLine();
	for (int y = 0; y < height; y++, bits += bytesPerLine) {
		row_pointers[y] = bits;
	}

	// Write the IDAT section.
	pwRet = pngWriter->write_IDAT(row_pointers.get());
	if (pwRet != 0) {
		// Error writing IDAT.
		// TODO: Unlink the PNG image.
		ret = RPCT_ERROR_OUTPUT_FILE_FAILED;
	}

	return ret;
}

/**
 * Thumbnail creator function for wrapper programs. (v1)
 * @param source_file Source file (UTF-8)
 * @param output_file Output file (UTF-8)
 * @param maximum_size Maximum size
 * @param flags Flags (see RpCreateThumbnailFlags)
 * @return 0 on success; non-zero on error.
 */
extern "C"
Q_DECL_EXPORT int RP_C_API rp_create_thumbnail(const char *source_file, const char *output_file, int maximum_size)
{
	// Wrapper function that calls rp_create_thumbnail2()
	// with flags == 0.
	return rp_create_thumbnail2(source_file, output_file, maximum_size, 0);
}
