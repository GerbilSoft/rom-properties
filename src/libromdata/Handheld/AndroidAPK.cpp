/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * AndroidAPK.cpp: Android APK package reader.                             *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "config.librpbase.h"

#include "AndroidAPK.hpp"
#include "RomData_p.hpp"

#include "AndroidCommon.hpp"
#include "android_apk_structs.h"
#include "../disc/AndroidResourceReader.hpp"

// MiniZip-NG
#include <zlib.h>
#include "mz.h"
#include "mz_zip.h"
#include "mz_strm.h"	// FIXME: Should be included by mz_zip_rw.h...
#include "mz_zip_rw.h"
#include "../file/mz_stream_IRpFile.hpp"
using namespace LibRomData::mz_stream_IRpFile;

// MiniZip-NG's native interface uses `void*` for handles.
// We'll typedef it to mzStream and mzReader.
typedef void *mzStream;
typedef void *mzReader;

// Other rom-properties libraries
#include "librpbase/img/RpImageLoader.hpp"
#include "librpfile/FileSystem.hpp"
#include "librpfile/MemFile.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;
using namespace LibRpTexture;

// PugiXML
// NOTE: This file is only compiled if ENABLE_XML is defined.
#include <pugixml.hpp>
using namespace pugi;

// AndroidManifestXML parser
#include "AndroidManifestXML.hpp"

// C++ STL classes
#include <limits>
using std::array;
using std::string;
using std::unique_ptr;
using std::vector;

// Uninitialized vector class
#include "uvector.h"

#ifdef _MSC_VER
// MSVC: Exception handling for /DELAYLOAD.
#  include "libwin32common/DelayLoadHelper.h"
#endif /* _MSC_VER */

namespace LibRomData {

#ifdef _MSC_VER
// DelayLoad test implementations
#  ifdef ZLIB_IS_DLL
DELAYLOAD_TEST_FUNCTION_IMPL0(get_crc_table);
#  endif /* ZLIB_IS_DLL */
#  ifdef MINIZIP_IS_DLL
// mz_zip_delete() can safely take nullptr; it won't do anything.
DELAYLOAD_TEST_FUNCTION_IMPL1(mz_zip_delete, nullptr);
#  endif /* MINIZIP_IS_DLL */
#  ifdef XML_IS_DLL
/**
 * Check if PugiXML can be delay-loaded.
 * NOTE: Implemented in EXE_delayload.cpp.
 * @return 0 on success; negative POSIX error code on error.
 */
extern int DelayLoad_test_PugiXML(void);
#  endif /* XML_IS_DLL */
#endif /* _MSC_VER */

class AndroidAPKPrivate final : public RomDataPrivate
{
public:
	explicit AndroidAPKPrivate(const IRpFilePtr &file, mzStream apkStream, mzReader apkReader);
	~AndroidAPKPrivate() override;

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(AndroidAPKPrivate)

public:
	/** RomDataInfo **/
	static const array<const char*, 1+1> exts;
	static const array<const char*, 1+1> mimeTypes;
	static const RomDataInfo romDataInfo;

public:
	// Opened .apk file
	mzStream apkStream;	// mz_stream_IRpFile
	mzReader apkReader;	// opened with mz_zip_reader_open()

	// Icon
	rp_image_ptr img_icon;

public:
	/**
	 * Load a file from the opened .jar file.
	 * @param filename Filename to load
	 * @param max_size Maximum size
	 * @return rp::uvector with the file data, or empty vector on error
	 */
	rp::uvector<uint8_t> loadFileFromZip(const char *filename, off64_t max_size = std::numeric_limits<off64_t>::max());

	/**
	 * Load AndroidManifest.xml from this->apkReader.
	 * this->apkFile must have already been opened.
	 * TODO: Store it in a PugiXML document, but need to check delay-load...
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int loadAndroidManifestXml(void);

public:
	// Maximum size for various files.
	static constexpr size_t AndroidManifest_xml_FILE_SIZE_MAX = (256U * 1024U);
	static constexpr size_t resources_arsc_FILE_SIZE_MAX = (4096U * 1024U);
	static constexpr size_t ICON_PNG_FILE_SIZE_MAX = (1024U * 1024U);

	// AndroidManifest.xml document
	// NOTE: Using a pointer to prevent delay-load issues.
	unique_ptr<xml_document> manifest_xml;

	// Android resource buffer
	// NOTE: Must be maintained while arscReader is still valid!
	rp::uvector<uint8_t> resources_arsc_buf;

	// Android resource reader
	unique_ptr<AndroidResourceReader> arscReader;

	/**
	 * Add string field data.
	 *
	 * If the string is in the format "@0x12345678", it will be loaded from
	 * resource.arsc if available, with RFT_STRING_MULTI.
	 *
	 * @param name Field name
	 * @param str String
	 * @param flags Formatting flags
	 * @return Field index, or -1 on error.
	 */
	int addField_string_i18n(const char *name, const char *str, unsigned int flags = 0);

	/**
	 * Add string field data.
	 *
	 * If the string is in the format "@0x12345678", it will be loaded from
	 * resource.arsc if available, with RFT_STRING_MULTI.
	 *
	 * @param name Field name
	 * @param str String
	 * @param flags Formatting flags
	 * @return Field index, or -1 on error.
	 */
	inline int addField_string_i18n(const char *name, const std::string &str, unsigned int flags = 0)
	{
		return addField_string_i18n(name, str.c_str(), flags);
	}

public:
	/**
	 * Load the icon.
	 * @return Icon, or nullptr on error.
	 */
	rp_image_const_ptr loadIcon(void);
};

ROMDATA_IMPL(AndroidAPK)

/** AndroidAPKPrivate **/

/* RomDataInfo */
const array<const char*, 1+1> AndroidAPKPrivate::exts = {{
	".apk",

	nullptr
}};
const array<const char*, 1+1> AndroidAPKPrivate::mimeTypes = {{
	// Vendor-specific MIME types from FreeDesktop.org.
	"application/vnd.android.package-archive",	// .jad

	nullptr
}};
const RomDataInfo AndroidAPKPrivate::romDataInfo = {
	"AndroidAPK", exts.data(), mimeTypes.data()
};

AndroidAPKPrivate::AndroidAPKPrivate(const IRpFilePtr &file, mzStream apkStream, mzReader apkReader)
	: super(file, &romDataInfo)
	, apkStream(apkStream)
	, apkReader(apkReader)
{}

AndroidAPKPrivate::~AndroidAPKPrivate()
{
	if (apkReader) {
		mz_zip_reader_close(apkReader);
		mz_zip_reader_delete(&apkReader);
	}
	if (apkStream) {
		mz_stream_IRpFile_close(apkStream);
		mz_stream_IRpFile_delete(&apkStream);
	}
}

/**
 * Load a file from the opened .jar file.
 * @param filename Filename to load
 * @param max_size Maximum size
 * @return rp::uvector with the file data, or empty vector on error
 */
rp::uvector<uint8_t> AndroidAPKPrivate::loadFileFromZip(const char *filename, off64_t max_size)
{
	// TODO: This is also used by GcnFstTest. Move to a common utility file?
	// NOTE: Using case-insensitive lookups for compatibility. Needs testing!
	int ret = mz_zip_reader_locate_entry(apkReader, filename, true);
	if (ret != MZ_OK) {
		return {};
	}

	// Get file information.
	mz_zip_file *file_info;
	ret = mz_zip_reader_entry_get_info(apkReader, &file_info);
	if (ret != MZ_OK) {
		// Error getting the file information.
		return {};
	}
	const off64_t uncompressed_size = file_info->uncompressed_size;
	if (uncompressed_size >= max_size) {
		// The uncompressed size is too big.
		return {};
	}

	ret = mz_zip_reader_entry_open(apkReader);
	if (ret != MZ_OK) {
		return {};
	}

	size_t size = static_cast<size_t>(uncompressed_size);
	rp::uvector<uint8_t> buf;
	buf.resize(size);

	// Read the file.
	// NOTE: zlib and minizip are only guaranteed to be able to
	// read UINT16_MAX (64 KB) at a time, and the updated MiniZip
	// from https://github.com/nmoinvaz/minizip enforces this.
	// TODO: Does MiniZip-NG's native API still have this limitation?
	uint8_t *p = buf.data();
	while (size > 0) {
		int to_read = static_cast<int>(size > UINT16_MAX ? UINT16_MAX : size);
		ret = mz_zip_reader_entry_read(apkReader, p, to_read);
		if (ret != to_read) {
			mz_zip_reader_entry_close(apkReader);
			return {};
		}

		// ret == number of bytes read.
		p += ret;
		size -= ret;
	}

	// Close the file.
	// An error will occur here if the CRC is incorrect.
	ret = mz_zip_reader_entry_close(apkReader);
	if (ret != MZ_OK) {
		return {};
	}

	return buf;
}

/**
 * Load AndroidManifest.xml from this->apkReader.
 * this->apkReader must have already been opened.
 * TODO: Store it in a PugiXML document, but need to check delay-load...
 * @return 0 on success; negative POSIX error code on error.
 */
int AndroidAPKPrivate::loadAndroidManifestXml(void)
{
	if (manifest_xml) {
		// AndroidManifest.xml is already loaded.
		return 0;
	}

	// The .apk file must have been opened already.
	assert(apkReader != nullptr);
	if (!apkReader) {
		return -EIO;
	}

	// Load AndroidManifest.xml.
	// TODO: May need to load resources too.
	rp::uvector<uint8_t> AndroidManifest_xml_buf = loadFileFromZip(
		"AndroidManifest.xml", AndroidManifest_xml_FILE_SIZE_MAX);
	if (AndroidManifest_xml_buf.empty()) {
		// Unable to load AndroidManifest.xml.
		return -ENOENT;
	}

	MemFilePtr memFile(new MemFile(AndroidManifest_xml_buf.data(), AndroidManifest_xml_buf.size()));
	memFile->setFilename("AndroidManifest.xml");
	AndroidManifestXML *const pManifestXML = new AndroidManifestXML(memFile);
	manifest_xml.reset(pManifestXML->takeXmlDocument());
	delete pManifestXML;

	if (!manifest_xml || manifest_xml->empty()) {
		// No document and/or it's empty?
		manifest_xml.reset();
		return -EIO;
	}

	// Load resources.arsc.
	// NOTE: We have to load the full file due to .zip limitations.
	// TODO: Figure out the best "max size".
	rp::uvector<uint8_t> resources_arsc_buf = loadFileFromZip(
		"resources.arsc", resources_arsc_FILE_SIZE_MAX);
	if (!resources_arsc_buf.empty()) {
		arscReader.reset(new AndroidResourceReader(resources_arsc_buf.data(), resources_arsc_buf.size()));
		if (!arscReader->isValid()) {
			// Not valid...
			arscReader.reset();
			resources_arsc_buf.clear();
			resources_arsc_buf.shrink_to_fit();
		}
	}

	return 0;
}

/**
 * Add string field data.
 *
 * If the string is in the format "@0x12345678", it will be loaded from
 * resource.arsc if available, with RFT_STRING_MULTI.
 *
 * @param name Field name
 * @param str String
 * @param flags Formatting flags
 * @return Field index, or -1 on error.
 */
int AndroidAPKPrivate::addField_string_i18n(const char *name, const char *str, unsigned int flags)
{
	// Do we have an AndroidResourceReader available?
	if (arscReader) {
		// Add the field using the AndroidResourceReader.
		return arscReader->addField_string_i18n(&fields, name, str, flags);
	}

	// No resources. Add the field directly.
	return fields.addField_string(name, str, flags);
}

/**
 * Load the icon.
 * @return Icon, or nullptr on error.
 */
rp_image_const_ptr AndroidAPKPrivate::loadIcon(void)
{
	if (img_icon) {
		// Icon has already been loaded.
		return img_icon;
	} else if (!this->isValid) {
		// Can't load the icon.
		return {};
	}

	// Make sure the .apk file is open.
	assert(apkReader != nullptr);
	if (!apkReader) {
		// Not open...
		return {};
	}

	// Get the icon filename from the AndroidManfiest.xml file.
	xml_node manifest_node = manifest_xml->child("manifest");
	if (!manifest_node) {
		// No "<manifest>" node???
		return {};
	}
	xml_node application_node = manifest_node.child("application");
	if (!application_node) {
		return {};
	}
	const char *icon_filename = application_node.attribute("icon").as_string(nullptr);
	if (!icon_filename || icon_filename[0] == '\0') {
		return {};
	}

	// TODO: Lower density on request?
	const uint32_t resource_id = AndroidResourceReader::parseResourceID(icon_filename);
	string resIcon;
	if (resource_id != 0 && arscReader) {
		// Icon filename has a resource ID.
		// Find the icon with the highest density.
		resIcon = arscReader->findIconHighestDensity(resource_id);
	}
	if (!resIcon.empty()) {
		icon_filename = resIcon.c_str();
	} else if (!icon_filename) {
		// Unable to load the icon filename...
		return {};
	}

	// Attempt to load the file.
	rp::uvector<uint8_t> icon_buf = loadFileFromZip(icon_filename, ICON_PNG_FILE_SIZE_MAX);
	if (icon_buf.size() < 8) {
		// Unable to load the icon file.
		return {};
	}

	// Check for an Adaptive Icon.
	// The icon file will be a binary XML instead of a PNG image.
	const uint32_t *const pData32 = reinterpret_cast<const uint32_t*>(icon_buf.data());
	if (pData32[0] == cpu_to_be32(ANDROID_BINARY_XML_MAGIC)) {
		// Not supported yet due to not supporting SVG...
		return {};
#if 0
		// Decode the XML.
		MemFilePtr memFile(new MemFile(icon_buf.data(), icon_buf.size()));
		memFile->setFilename(icon_filename);
		AndroidManifestXML *const pIconXML = new AndroidManifestXML(memFile);
		unique_ptr<xml_document> icon_xml(pIconXML->takeXmlDocument());
		delete pIconXML;
		icon_filename = nullptr;

		xml_node adaptive_icon_node = icon_xml->child("adaptive-icon");
		if (!adaptive_icon_node) {
			// No "<adaptive-icon>" node???
			return {};
		}

		// Get the foreground drawable if available.
		// Otherwise, get the background drawable if available.
		// TODO: Properly composite foreground onto background?
		// May need to write a quick and dirty composite function...
		xml_node foreground_node = adaptive_icon_node.child("foreground");
		if (foreground_node) {
			const char *const drawable = foreground_node.attribute("drawable").as_string(nullptr);
			if (drawable && drawable[0] != '\0') {
				resIcon = arscReader->getStringFromResource(drawable);
				icon_filename = resIcon.c_str();
			}
		} else {
			xml_node background_node = adaptive_icon_node.child("background");
			if (background_node) {
				const char *const drawable = background_node.attribute("drawable").as_string(nullptr);
				if (drawable && drawable[0] != '\0') {
					resIcon = arscReader->getStringFromResource(drawable);
					icon_filename = resIcon.c_str();
				}
			}
		}

		if (!icon_filename) {
			// No drawables...
			return {};
		}

		// Load the drawable.
		icon_buf = loadFileFromZip(icon_filename, ICON_PNG_FILE_SIZE_MAX);
		if (icon_buf.size() < 8) {
			// Unable to load the icon file.
			return {};
		}
#endif
	}

	// Create a MemFile and decode the image.
	// TODO: For rpcli, shortcut to extract the PNG directly?
	MemFile f_mem(icon_buf.data(), icon_buf.size());
	this->img_icon = RpImageLoader::load(&f_mem);
	return this->img_icon;
}

/** AndroidAPK **/

/**
 * Read an AndroidAPK .apk file.
 *
 * A ROM file must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the ROM.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM file
 * @param stream MiniZip-NG mz_stream (this object takes ownership)
 * @param reader MiniZip-NG mz_zip_reader (this object takes ownership)
 */
AndroidAPK::AndroidAPK(const IRpFilePtr &file, mzStream stream, mzReader reader)
	: super(new AndroidAPKPrivate(file, stream, reader))
{
	// This class handles application packages.
	RP_D(AndroidAPK);
	d->mimeType = "application/vnd.android.package-archive";	// vendor-specific
	d->fileType = FileType::ApplicationPackage;

	if (!d->file) {
		// Could not ref() the file handle.
		// NOTE: Delay-Load handling is *not* needed here because
		// if apkReader/apkStream are not nullptr, MiniZip was already used.
		if (d->apkReader) {
			mz_zip_reader_close(d->apkReader);
			mz_zip_reader_delete(&d->apkReader);
			d->apkReader = nullptr;
		}
		if (d->apkStream) {
			mz_stream_IRpFile_close(d->apkStream);
			mz_stream_IRpFile_delete(&d->apkStream);
			d->apkStream = nullptr;
		}
		return;
	}

#ifdef _MSC_VER
	// Delay load verification.
	// TODO: zlib/minizip checks are only needed if mz == nullptr?
#  ifdef ZLIB_IS_DLL
	// Only if zlib is a DLL.
	if (DelayLoad_test_get_crc_table() != 0) {
		// Delay load failed.
		// Android packages cannot be read without MiniZip.
		d->isValid = false;
		d->file.reset();
		return;
	}
#  else /* !ZLIB_IS_DLL */
	// zlib isn't in a DLL, but we need to ensure that the
	// CRC table is initialized anyway.
	get_crc_table();
#  endif /* ZLIB_IS_DLL */

#  ifdef MINIZIP_IS_DLL
	// Only if MiniZip is a DLL.
	if (DelayLoad_test_mz_zip_delete() != 0) {
		// Delay load failed.
		// Android packages cannot be read without MiniZip.
		d->isValid = false;
		d->file.reset();
		return;
	}
#  endif /* MINIZIP_IS_DLL */

#  ifdef XML_IS_DLL
	int ret_dl = DelayLoad_test_PugiXML();
	if (ret_dl != 0) {
		// Delay load failed.
		d->isValid = false;
		if (d->apkReader) {
			mz_zip_reader_close(d->apkReader);
			mz_zip_reader_delete(&d->apkReader);
			d->apkReader = nullptr;
		}
		if (d->apkStream) {
			mz_stream_IRpFile_close(d->apkStream);
			mz_stream_IRpFile_delete(&d->apkStream);
			d->apkStream = nullptr;
		}
		d->file.reset();
		return;
	}
#  endif /* XML_IS_DLL */
#endif /* _MSC_VER */

	// Seek to the beginning of the file.
	d->file->rewind();

	// Read the file header. (at least 32 bytes)
	uint8_t header[32];
	size_t size = d->file->read(header, sizeof(header));
	if (size < 32) {
		if (d->apkReader) {
			mz_zip_reader_close(d->apkReader);
			mz_zip_reader_delete(&d->apkReader);
			d->apkReader = nullptr;
		}
		if (d->apkStream) {
			mz_stream_IRpFile_close(d->apkStream);
			mz_stream_IRpFile_delete(&d->apkStream);
			d->apkStream = nullptr;
		}
		d->file.reset();
		return;
	}

	// Check if this file is supported.
	const char *const filename = file->filename();
	const DetectInfo info = {
		{0, sizeof(header), header},
		FileSystem::file_ext(filename),	// ext
		0				// szFile (not needed for AndroidAPK)
	};
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		if (d->apkReader) {
			mz_zip_reader_close(d->apkReader);
			mz_zip_reader_delete(&d->apkReader);
			d->apkReader = nullptr;
		}
		if (d->apkStream) {
			mz_stream_IRpFile_close(d->apkStream);
			mz_stream_IRpFile_delete(&d->apkStream);
			d->apkStream = nullptr;
		}
		d->file.reset();
		return;
	}

	// Attempt to open as a .zip file. (only if apkStream and apkReader are nullptr)
	// FIXME: Both must be either set or nullptr. Check for a bad condition!
	if (!d->apkStream && !d->apkReader) {
		d->apkStream = mz_stream_IRpFile_create();
		if (!d->apkStream) {
			d->file.reset();
			return;
		}

		int ret = mz_stream_IRpFile_open(d->apkStream, d->file, MZ_OPEN_MODE_READ | MZ_OPEN_MODE_EXISTING);
		if (ret != MZ_OK) {
			mz_stream_IRpFile_delete(&d->apkStream);
			d->apkStream = nullptr;
			d->file.reset();
			return;
		}

		d->apkReader = mz_zip_reader_create();
		if (!d->apkReader) {
			mz_stream_IRpFile_close(d->apkStream);
			mz_stream_IRpFile_delete(&d->apkStream);
			d->apkStream = nullptr;
			d->file.reset();
			return;
		}

		ret = mz_zip_reader_open(d->apkReader, d->apkStream);
		if (ret != MZ_OK) {
			mz_zip_reader_delete(&d->apkStream);
			d->apkReader = nullptr;
			mz_stream_IRpFile_close(d->apkStream);
			mz_stream_IRpFile_delete(&d->apkStream);
			d->apkStream = nullptr;
			d->file.reset();
			return;
		}
	}

	// Attempt to load AndroidManifest.xml.
	if (d->loadAndroidManifestXml() != 0) {
		// Failed to load AndroidManifest.xml.
		d->isValid = false;
		if (d->apkReader) {
			mz_zip_reader_close(d->apkReader);
			mz_zip_reader_delete(&d->apkReader);
			d->apkReader = nullptr;
		}
		if (d->apkStream) {
			mz_stream_IRpFile_close(d->apkStream);
			mz_stream_IRpFile_delete(&d->apkStream);
			d->apkStream = nullptr;
		}
		d->file.reset();
		return;
	}
}

/**
 * Close the opened file.
 */
void AndroidAPK::close(void)
{
	RP_D(AndroidAPK);
	if (d->apkReader) {
		mz_zip_reader_close(d->apkReader);
		mz_zip_reader_delete(&d->apkReader);
		d->apkReader = nullptr;
	}
	if (d->apkStream) {
		mz_stream_IRpFile_close(d->apkStream);
		mz_stream_IRpFile_delete(&d->apkStream);
		d->apkStream = nullptr;
	}
	d->file.reset();
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int AndroidAPK::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	if (!info || !info->header.pData) {
		return -1;
	}

	// .apk check: If this is a .zip file, we can try to open it.
	if (info->header.size >= 4) {
		const uint32_t *const pData32 =
			reinterpret_cast<const uint32_t*>(info->header.pData);
		if (pData32[0] == cpu_to_be32(0x504B0304)) {
			// This appears to be a .zip file. (PK\003\004)
			// TODO: Also check for these?:
			// - PK\005\006 (empty)
			// - PK\007\008 (spanned)
			return 0;
		}
	}

	// Not supported.
	return -1;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @return System name, or nullptr if not supported.
 */
const char *AndroidAPK::systemName(unsigned int type) const
{
	RP_D(const AndroidAPK);
	if (!d->isValid || !isSystemNameTypeValid(type)) {
		return nullptr;
	}

	// AndroidAPK has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"AndroidAPK::systemName() array index optimization needs to be updated.");

	static const array<const char*, 4> sysNames = {{
		"Google Android", "Android", "Android", nullptr,
	}};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t AndroidAPK::supportedImageTypes(void) const
{
	return IMGBF_INT_ICON;
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> AndroidAPK::supportedImageSizes_static(ImageType imageType)
{
	ASSERT_supportedImageSizes(imageType);

	switch (imageType) {
		case IMG_INT_ICON:
			// TODO: Get the actual image size.
			return {{nullptr, 64, 64, 0}};
		default:
			break;
	}

	// Unsupported image type.
	return {};
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> AndroidAPK::supportedImageSizes(ImageType imageType) const
{
	ASSERT_supportedImageSizes(imageType);
	//RP_D(const AndroidAPK);

	switch (imageType) {
		case IMG_INT_ICON:
			// TODO: Get the actual image size.
			return {{nullptr, 64, 64, 0}};
		default:
			break;
	}

	// Unsupported image type.
	return {};
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int AndroidAPK::loadFieldData(void)
{
	RP_D(AndroidAPK);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || !d->manifest_xml) {
		// APK isn't valid, and/or AndroidManifest.xml could not be loaded.
		return -EIO;
	}

	// Get fields from the XML file.
	AndroidCommon::loadFieldData(d->fields, *d->manifest_xml, d->arscReader.get());

	// Finished reading the field data.
	return d->fields.count();
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the metadata hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int AndroidAPK::loadMetaData(void)
{
	RP_D(AndroidAPK);
	if (!d->metaData.empty()) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || !d->manifest_xml) {
		// APK isn't valid, and/or AndroidManifest.xml could not be loaded.
		return -EIO;
	}

	// Get metadata from the XML file.
	AndroidCommon::loadMetaData(d->metaData, *d->manifest_xml, d->arscReader.get());

	// Finished reading the metadata.
	return d->metaData.count();
}

/**
 * Load an internal image.
 * Called by RomData::image().
 * @param imageType	[in] Image type to load.
 * @param pImage	[out] Reference to rp_image_const_ptr to store the image in.
 * @return 0 on success; negative POSIX error code on error.
 */
int AndroidAPK::loadInternalImage(ImageType imageType, rp_image_const_ptr &pImage)
{
	ASSERT_loadInternalImage(imageType, pImage);
	RP_D(AndroidAPK);

	ROMDATA_loadInternalImage_single(
		IMG_INT_ICON,	// ourImageType
		d->file,	// file
		d->isValid,	// isValid
		0,		// romType
		d->img_icon,	// imgCache
		d->loadIcon);	// func
}

/**
 * Does this ROM image have "dangerous" permissions?
 *
 * @return True if the ROM image has "dangerous" permissions; false if not.
 */
bool AndroidAPK::hasDangerousPermissions(void) const
{
	RP_D(const AndroidAPK);
	if (!d->isValid || !d->manifest_xml) {
		// APK isn't valid, and/or AndroidManifest.xml could not be loaded.
		return false;
	}

	return AndroidCommon::hasDangerousPermissions(*d->manifest_xml);
}

} // namespace LibRomData
