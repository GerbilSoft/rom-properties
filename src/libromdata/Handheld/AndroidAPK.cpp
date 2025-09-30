/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * AndroidAPK.cpp: Android APK package reader.                             *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"

#include "config.librpbase.h"
#include "AndroidAPK.hpp"
#include "android_apk_structs.h"
#include "../disc/AndroidResourceReader.hpp"

// MiniZip
#include <zlib.h>
#include "mz_zip.h"
#include "compat/ioapi.h"
#include "compat/unzip.h"

// Other rom-properties libraries
#include "librpbase/img/RpImageLoader.hpp"
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
#include <stack>
using std::array;
using std::stack;
using std::string;
using std::unique_ptr;
using std::unordered_map;
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
// unzClose() can safely take nullptr; it won't do anything.
DELAYLOAD_TEST_FUNCTION_IMPL1(unzClose, nullptr);
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
	explicit AndroidAPKPrivate(const IRpFilePtr &file);
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
	unzFile apkFile;

	// Icon
	rp_image_ptr img_icon;

public:
	/**
	 * Open a Zip file for reading.
	 * @param filename Zip filename.
	 * @return Zip file, or nullptr on error.
	 */
	static unzFile openZip(const char *filename);

	/**
	 * Load a file from the opened .jar file.
	 * @param filename Filename to load
	 * @param max_size Maximum size
	 * @return rp::uvector with the file data, or empty vector on error
	 */
	rp::uvector<uint8_t> loadFileFromZip(const char *filename, size_t max_size = std::numeric_limits<size_t>::max());

	/**
	 * Load AndroidManifest.xml from this->apkFile.
	 * this->apkFile must have already been opened.
	 * TODO: Store it in a PugiXML document, but need to check delay-load...
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int loadAndroidManifestXml(void);

public:
	// Maximum size for various files.
	static constexpr size_t AndroidManifest_xml_FILE_SIZE_MAX = (256U * 1024U);
	static constexpr size_t resources_arsc_FILE_SIZE_MAX = (4096U * 1024U);
	static constexpr size_t ICON_PNG_FILE_SIZE_MAX = (1024U * 1024U);;

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

AndroidAPKPrivate::AndroidAPKPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
	, apkFile(nullptr)
{ }

AndroidAPKPrivate::~AndroidAPKPrivate()
{
	if (apkFile) {
		unzClose(apkFile);
	}
}

/**
 * Open a Zip file for reading.
 * @param filename Zip filename.
 * @return Zip file, or nullptr on error.
 */
unzFile AndroidAPKPrivate::openZip(const char *filename)
{
#ifdef _WIN32
	// NOTE: MiniZip-NG 3.0.2's compatibility functions
	// take UTF-8 on Windows, not UTF-16.
	zlib_filefunc64_def ffunc;
	fill_win32_filefunc64(&ffunc);
	return unzOpen2_64(filename, &ffunc);
#else /* !_WIN32 */
	return unzOpen(filename);
#endif /* _WIN32 */
}

/**
 * Load a file from the opened .jar file.
 * @param filename Filename to load
 * @param max_size Maximum size
 * @return rp::uvector with the file data, or empty vector on error
 */
rp::uvector<uint8_t> AndroidAPKPrivate::loadFileFromZip(const char *filename, size_t max_size)
{
	// TODO: This is also used by GcnFstTest. Move to a common utility file?
	int ret = unzLocateFile(apkFile, filename, nullptr);
	if (ret != UNZ_OK) {
		return {};
	}

	// Get file information.
	unz_file_info64 file_info;
	ret = unzGetCurrentFileInfo64(apkFile,
		&file_info, nullptr, 0, nullptr, 0, nullptr, 0);
	if (ret != UNZ_OK || file_info.uncompressed_size >= max_size) {
		// Error getting file information, or the uncompressed size is too big.
		return {};
	}

	ret = unzOpenCurrentFile(apkFile);
	if (ret != UNZ_OK) {
		return {};
	}

	size_t size = static_cast<size_t>(file_info.uncompressed_size);
	rp::uvector<uint8_t> buf;
	buf.resize(size);

	// Read the file.
	// NOTE: zlib and minizip are only guaranteed to be able to
	// read UINT16_MAX (64 KB) at a time, and the updated MiniZip
	// from https://github.com/nmoinvaz/minizip enforces this.
	uint8_t *p = buf.data();
	while (size > 0) {
		int to_read = static_cast<int>(size > UINT16_MAX ? UINT16_MAX : size);
		ret = unzReadCurrentFile(apkFile, p, to_read);
		if (ret != to_read) {
			return {};
		}

		// ret == number of bytes read.
		p += ret;
		size -= ret;
	}

	// Close the file.
	// An error will occur here if the CRC is incorrect.
	ret = unzCloseCurrentFile(apkFile);
	if (ret != UNZ_OK) {
		return {};
	}

	return buf;
}

/**
 * Load AndroidManifest.xml from this->apkFile.
 * this->apkFile must have already been opened.
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
	assert(apkFile != nullptr);
	if (!apkFile) {
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
	AndroidManifestXML *pManifestXML = new AndroidManifestXML(memFile);
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
	if (!apkFile) {
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
		// TODO: Handle adaptive icons.
		return {};
	}

	// Create a MemFile and decode the image.
	// TODO: For rpcli, shortcut to extract the PNG directly?
	MemFile f_mem(icon_buf.data(), icon_buf.size());
	this->img_icon = RpImageLoader::load(&f_mem);
	return this->img_icon;
}

/** AndroidAPK **/

/**
 * Read a AndroidAPK .jar or .jad file.
 *
 * A ROM file must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the ROM.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM file.
 */
AndroidAPK::AndroidAPK(const IRpFilePtr &file)
	: super(new AndroidAPKPrivate(file))
{
	// This class handles application packages.
	RP_D(AndroidAPK);
	d->mimeType = "application/vnd.android.package-archive";	// vendor-specific
	d->fileType = FileType::ApplicationPackage;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

#ifdef _MSC_VER
	// Delay load verification.
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
	if (DelayLoad_test_unzClose() != 0) {
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
		d->file.reset();
		return;
	}

	// Attempt to open as a .zip file first.
	// TODO: Custom MiniZip functions to use IRpFile so we can use IStream?
	d->apkFile = d->openZip(file->filename());
	if (!d->apkFile) {
		// Cannot open as a .zip file.
		d->isValid = false;
		d->file.reset();
		return;
	}

	// Attempt to load AndroidManifest.xml.
	if (d->loadAndroidManifestXml() != 0) {
		// Failed to load AndroidManifest.xml.
		d->isValid = false;
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
	if (d->apkFile) {
		unzClose(d->apkFile);
		d->apkFile = nullptr;
	}
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
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

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
	xml_node manifest_node = d->manifest_xml->child("manifest");
	if (!manifest_node) {
		// No "<manifest>" node???
		return static_cast<int>(d->fields.count());
	}

	d->fields.reserve(10);	// Maximum of 10 fields.

	// Package name is in the manifest tag.
	// <application name=""> is something else.
	const char *const package_name = manifest_node.attribute("package").as_string(nullptr);
	if (package_name && package_name[0] != '\0') {
		d->fields.addField_string(C_("AndroidManifestXML", "Package Name"), package_name);
	}

	// Application information
	xml_node application_node = manifest_node.child("application");
	if (application_node) {
		const char *const label = application_node.attribute("label").as_string(nullptr);
		if (label && label[0] != '\0') {
			d->addField_string_i18n(C_("AndroidAPK", "Title"), label);
		}

		const char *const description = application_node.attribute("description").as_string(nullptr);
		if (description && description[0] != '\0') {
			d->addField_string_i18n(C_("AndroidAPK", "Description"), description);
		}

		const char *const appCategory = application_node.attribute("appCategory").as_string(nullptr);
		if (appCategory && appCategory[0] != '\0') {
			d->fields.addField_string(C_("AndroidAPK", "Category"), appCategory);
		}
	}

	// SDK version
	xml_node uses_sdk = manifest_node.child("uses-sdk");
	if (uses_sdk) {
		const char *const s_minSdkVersion = uses_sdk.attribute("minSdkVersion").as_string(nullptr);
		if (s_minSdkVersion && s_minSdkVersion[0] != '\0') {
			d->fields.addField_string(C_("AndroidAPK", "Min. SDK Version"), s_minSdkVersion);
		}

		const char *const s_targetSdkVersion = uses_sdk.attribute("targetSdkVersion").as_string(nullptr);
		if (s_targetSdkVersion && s_targetSdkVersion[0] != '\0') {
			d->fields.addField_string(C_("AndroidAPK", "Target SDK Version"), s_targetSdkVersion);
		}
	}

	// Version (and version code)
	const char *const versionName = manifest_node.attribute("versionName").as_string(nullptr);
	if (versionName && versionName[0] != '\0') {
		d->fields.addField_string(C_("AndroidAPK", "Version"), versionName);
	}
	const char *const s_versionCode = manifest_node.attribute("versionCode").as_string(nullptr);
	if (s_versionCode && s_versionCode[0] != '\0') {
		d->fields.addField_string(C_("AndroidAPK", "Version Code"), s_versionCode);
	}

	// Copied from Nintendo3DS. (TODO: Centralize it?)
#ifdef _WIN32
	// Windows: 6 visible rows per RFT_LISTDATA.
	static constexpr int rows_visible = 6;
#else /* !_WIN32 */
	// Linux: 4 visible rows per RFT_LISTDATA.
	static constexpr int rows_visible = 4;
#endif /* _WIN32 */

	// Features
	// TODO: Normalize/localize feature names?
	// FIXME: Get strings from resources?
	xml_node feature_node = manifest_node.child("uses-feature");
	if (feature_node) {
		auto *const vv_features = new RomFields::ListData_t;
		do {
			vector<string> v_feature;

			const char *const feature = feature_node.attribute("name").as_string(nullptr);
			if (feature && feature[0] != '\0') {
				v_feature.push_back(feature);
			} else {
				// Check if glEsVersion is set.
				uint32_t glEsVersion = feature_node.attribute("glEsVersion").as_uint();
				if (glEsVersion != 0) {
					v_feature.push_back(fmt::format(FSTR("OpenGL ES {:d}.{:d}"),
						glEsVersion >> 16, glEsVersion & 0xFFFF));
				} else {
					const char *const s_glEsVersion = feature_node.attribute("glEsVersion").as_string(nullptr);
					if (s_glEsVersion && s_glEsVersion[0] != '\0') {
						v_feature.push_back(s_glEsVersion);
					} else {
						v_feature.push_back(string());
					}
				}
			}

			const char *required = feature_node.attribute("required").as_string(nullptr);
			if (required && required[0] != '\0') {
				v_feature.push_back(required);
			} else {
				// Default value is true.
				v_feature.push_back("true");
			}

			vv_features->push_back(std::move(v_feature));

			// Next feature
			feature_node = feature_node.next_sibling("uses-feature");
		} while (feature_node);

		if (!vv_features->empty()) {
			static const array<const char*, 2> features_headers = {{
				NOP_C_("AndroidAPK|Features", "Feature"),
				NOP_C_("AndroidAPK|Features", "Required?"),
			}};
			vector<string> *const v_features_headers = RomFields::strArrayToVector_i18n(
				"AndroidAPK|Features", features_headers);

			RomFields::AFLD_PARAMS params(0, rows_visible);
			params.headers = v_features_headers;
			params.data.single = vv_features;
			params.col_attrs.align_data = AFLD_ALIGN2(TXA_D, TXA_C);
			d->fields.addField_listData(C_("AndroidAPK", "Features"), &params);
		} else {
			delete vv_features;
		}
	}

	// Permissions
	// TODO: Normalize/localize permission names?
	// TODO: maxSdkVersion?
	// TODO: Also handle "uses-permission-sdk-23"?
	xml_node permission_node = manifest_node.child("uses-permission");
	if (permission_node) {
		auto *const vv_permissions = new RomFields::ListData_t;
		do {
			const char *const permission = permission_node.attribute("name").as_string(nullptr);
			if (permission && permission[0] != '\0') {
				vector<string> v_permission;
				v_permission.push_back(permission);
				vv_permissions->push_back(std::move(v_permission));
			}

			// Next permission
			permission_node = permission_node.next_sibling("uses-permission");
		} while (permission_node);

		if (!vv_permissions->empty()) {
			RomFields::AFLD_PARAMS params(0, rows_visible);
			params.headers = nullptr;
			params.data.single = vv_permissions;
			d->fields.addField_listData(C_("AndroidAPK", "Permissions"), &params);
		} else {
			delete vv_permissions;
		}
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
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

	// Get fields from the XML file.
	xml_node manifest_node = d->manifest_xml->child("manifest");
	if (!manifest_node) {
		// No "<manifest>" node???
		return static_cast<int>(d->metaData.count());
	}

	// AndroidManifest.xml is read in the constructor.
	AndroidResourceReader *const arscReader = d->arscReader.get();
	d->metaData.reserve(3);	// Maximum of 3 metadata properties.

	// NOTE: Only retrieving a single language.
	// TODO: Get the system language code and use it as def_lc?

	// Package name is in the manifest tag. (as Title ID)
	// <application name=""> is something else.
	const char *const package_name = manifest_node.attribute("package").as_string(nullptr);
	if (package_name && package_name[0] != '\0') {
		d->metaData.addMetaData_string(Property::TitleID, package_name);
	}

	// Application information
	xml_node application_node = manifest_node.child("application");
	if (application_node) {
		const char *const label = application_node.attribute("label").as_string(nullptr);
		if (label && label[0] != '\0') {
			if (arscReader) {
				d->metaData.addMetaData_string(Property::Title, arscReader->getStringFromResource(label));
			} else {
				d->metaData.addMetaData_string(Property::Title, label);
			}
		}

		const char *const description = application_node.attribute("description").as_string(nullptr);
		if (description && description[0] != '\0') {
			if (arscReader) {
				d->metaData.addMetaData_string(Property::Description, arscReader->getStringFromResource(description));
			} else {
				d->metaData.addMetaData_string(Property::Description, description);
			}
		}
	}
	// Finished reading the metadata.
	return static_cast<int>(d->metaData.count());
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

	xml_node manifest_node = d->manifest_xml->child("manifest");
	if (!manifest_node) {
		// No "<manifest>" node???
		return false;
	}

	// Dangerous permissions
	static const array<const char*, 2> dangerousPermissions = {{
		"android.permission.ACCESS_SUPERUSER",
		"android.permission.BIND_DEVICE_ADMIN",
	}};

	// Permissions
	// TODO: Normalize/localize permission names?
	// TODO: maxSdkVersion?
	// TODO: Also handle "uses-permission-sdk-23"?
	xml_node permission_node = manifest_node.child("uses-permission");
	if (!permission_node) {
		// No permissions?
		return false;
	}

	// Search for dangerous permissions.
	bool bDangerous = false;
	do {
		const char *const permission = permission_node.attribute("name").as_string(nullptr);
		if (permission && permission[0] != '\0') {
			// Doing a linear search.
			// TODO: Use std::lower_bound() instead?
			auto iter = std::find_if(dangerousPermissions.cbegin(), dangerousPermissions.cend(),
				[permission](const char *dperm) {
					return (!strcmp(dperm, permission));
				});
			if (iter != dangerousPermissions.cend()) {
				// Found a dangerous permission.
				bDangerous = true;
				break;
			}
		}

		// Next permission
		permission_node = permission_node.next_sibling("uses-permission");
	} while (permission_node);

	return bDangerous;
}

} // namespace LibRomData
