/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiUPackage.cpp: Wii U NUS Package reader.                              *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"

#include "WiiUPackage.hpp"
#include "WiiUPackage_p.hpp"
#include "data/WiiUData.hpp"
#include "GameCubeRegions.hpp"

// for extURLs_int()
#include "WiiU.hpp"

// TGA FileFormat
#include "librptexture/fileformat/TGA.hpp"

// Other rom-properties libraries
#include "librpbase/disc/PartitionFile.hpp"
#include "librpbase/SystemRegion.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;
using namespace LibRpTexture;

#ifdef _WIN32
#  include "librptext/wchar.hpp"
#else /* _WIN32 */
#  define U82T_c(str) (str)
#endif /* _WIN32 */

// C++ STL classes
using std::array;
using std::string;
using std::tstring;
using std::unique_ptr;
using std::vector;

namespace LibRomData {

ROMDATA_IMPL(WiiUPackage)

/** WiiUPackagePrivate **/

/* RomDataInfo */
const array<const char*, 0+1> WiiUPackagePrivate::exts = {{
	// No file extensions; NUS packages are directories.
	nullptr
}};
const array<const char*, 1+1> WiiUPackagePrivate::mimeTypes = {{
	// NUS packages are directories.
	"inode/directory",

	nullptr
}};
const RomDataInfo WiiUPackagePrivate::romDataInfo = {
	"WiiUPackage", exts.data(), mimeTypes.data()
};

WiiUPackagePrivate::WiiUPackagePrivate(const char *path)
	: super({}, &romDataInfo)
	, packageType(PackageType::Unknown)
{
	if (path && path[0] != '\0') {
#ifdef _WIN32
		// Windows: Storing the path as UTF-16 internally.
		this->path.assign(U82T_c(path));
#else /* !_WIN32 */
		this->path.assign(path);
#endif /* _WIN32 */
	}

#ifdef ENABLE_DECRYPTION
	// Clear the title key.
	memset(title_key, 0, sizeof(title_key));
#endif /* ENABLE_DECRYPTION */
}

#if defined(_WIN32) && defined(_UNICODE)
WiiUPackagePrivate::WiiUPackagePrivate(const wchar_t *path)
	: super({}, &romDataInfo)
	, packageType(PackageType::Unknown)
{
	if (path && path[0] != L'\0') {
		this->path.assign(path);
	}

#ifdef ENABLE_DECRYPTION
	// Clear the title key.
	memset(title_key, 0, sizeof(title_key));
#endif /* ENABLE_DECRYPTION */
}
#endif /* _WIN32 && _UNICODE */

/**
 * Clear everything.
 */
void WiiUPackagePrivate::reset(void)
{
	path.clear();

	ticket.reset();
	tmd.reset();
	fst.reset();
}

/**
 * Open a content file.
 * @param idx Content index (TMD index)
 * @return Content file, or nullptr on error.
 */
IDiscReaderPtr WiiUPackagePrivate::openContentFile(unsigned int idx)
{
	assert(packageType == PackageType::NUS);
	assert(idx < contentsReaders.size());
	if (packageType != PackageType::NUS ||
	    idx >= contentsReaders.size())
	{
		return {};
	}

	if (contentsReaders[idx]) {
		// Content is already open.
		return contentsReaders[idx];
	}

#ifdef ENABLE_DECRYPTION
	// Attempt to open the content.
	const WUP_Content_Entry &entry = contentsTable[idx];
	tstring s_path(this->path);
	s_path += DIR_SEP_CHR;
	const size_t orig_path_size = s_path.size();

	// Try with lowercase hex first.
	const uint32_t content_id = be32_to_cpu(entry.content_id);
	s_path += fmt::format(FSTR(_T("{:0>8x}.app")), content_id);

	IRpFilePtr subfile = std::make_shared<RpFile>(s_path.c_str(), RpFile::FM_OPEN_READ);
	if (!subfile->isOpen()) {
		// Try with uppercase hex.
		s_path.resize(orig_path_size);
		s_path += fmt::format(FSTR(_T("{:0>8X}.app")), content_id);

		subfile = std::make_shared<RpFile>(s_path.c_str(), RpFile::FM_OPEN_READ);
		if (!subfile->isOpen()) {
			// Unable to open the content file.
			// TODO: Error code?
			return {};
		}
	}

	// Create a disc reader.
	// TODO: Bitfield constants for 'type'?
	IDiscReaderPtr discReader;
	if (entry.type & cpu_to_be16(0x0002)) {
		// Content is H3-hashed.
		// NOTE: No IV is needed here.
		discReader = std::make_shared<WiiUH3Reader>(subfile, title_key, sizeof(title_key));
	} else {
		// Content is not H3-hashed.

		// IV is the 2-byte content index, followed by zeroes.
		array<uint8_t, 16> iv;
		iv.fill(0);
		memcpy(iv.data(), &entry.index, 2);

		discReader = std::make_shared<CBCReader>(subfile, 0, subfile->size(), title_key, iv.data());
	}
	if (!discReader->isOpen()) {
		// Unable to open the CBC reader.
		return {};
	}

	// Disc reader is open.
	contentsReaders[idx] = discReader;
	return discReader;
#else /* !ENABLE_DECRYPTION */
	// Unencrypted NUS packages are NOT supported right now.
	return {};
#endif /* ENABLE_DECRYPTION */
}

/**
 * Open a file from the contents using the FST.
 * @param filename Filename
 * @return IRpFile, or nullptr on error.
 */
IRpFilePtr WiiUPackagePrivate::open(const char *filename)
{
	if (!filename || filename[0] == '\0') {
		return {};
	}

	if (packageType == PackageType::Extracted) {
		// Extracted package format. Open the file directly.
		// TODO: Change slashes to backslashes on Windows?
		tstring ts_full_filename(path);
		ts_full_filename += DIR_SEP_CHR;

		// Remove leading slashes, if present.
		while (*filename == _T('/')) {
			filename++;
		}
		if (*filename == '\0') {
			// Oops, no filename...
			return {};
		}

#ifdef _WIN32
		const size_t old_sz = ts_full_filename.size();
#endif /* _WIN32 */
		ts_full_filename += U82T_c(filename);
#ifdef _WIN32
		// Replace all slashes with backslashes.
		const auto start_iter = ts_full_filename.begin() + old_sz;
		std::transform(start_iter, ts_full_filename.end(), start_iter, [](TCHAR c) {
			return (c == '/') ? DIR_SEP_CHR : c;
		});
#endif /* _WIN32 */

		return std::make_shared<RpFile>(ts_full_filename.c_str(), RpFile::FM_OPEN_READ);
	}

	assert(fst != nullptr);
	if (!fst) {
		// No FST.
		return {};
	}

	// Get the FST entry.
	IFst::DirEnt dirent;
	int ret = fst->find_file(filename, &dirent);
	if (ret != 0) {
		// File not found?
		return {};
	}

	// Make sure the required content file is open.
	IDiscReaderPtr contentFile = openContentFile(dirent.ptnum);
	if (!contentFile) {
		// Unable to open this content file.
		return {};
	}

	// Create a PartitionFile.
	return std::make_shared<PartitionFile>(contentFile, dirent.offset, dirent.size);
}

/**
 * Load the icon.
 * @return Icon, or nullptr on error.
 */
rp_image_const_ptr WiiUPackagePrivate::loadIcon(void)
{
	if (img_icon) {
		// Icon has already been loaded.
		return img_icon;
	} else if (!this->isValid) {
		// Can't load the icon.
		return {};
	}

	// Verify that this is a Wii U package. (TMD format must be v1 or higher.)
	if (tmd && tmd->tmdFormatVersion() < 1) {
		// Not a Wii U package.
		// TODO: loadInternalImage() should return ENOENT.
		return {};
	}

	// Icon is "/meta/iconTex.tga".
	IRpFilePtr f_icon = this->open("/meta/iconTex.tga");
	if (!f_icon) {
		// Icon not found?
		return {};
	}

	// Attempt to open the icon as TGA.
	TGA tga(f_icon);
	if (!tga.isValid()) {
		// Not a valid TGA file.
		return {};
	}

	// Get the icon.
	img_icon = tga.image();
	return img_icon;
}

/** WiiUPackage **/

/**
 * Read a Wii U NUS package.
 *
 * NOTE: Wii U NUS packages are directories. This constructor
 * only accepts IRpFilePtr, so it isn't usable.
 *
 * A ROM image must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the ROM image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM image.
 */
WiiUPackage::WiiUPackage(const IRpFilePtr &file)
	: super(new WiiUPackagePrivate((const TCHAR*)nullptr))
{
	// Not supported!
	RP_UNUSED(file);
}

/**
 * Read a Wii U NUS package.
 *
 * NOTE: Wii U NUS packages are directories. This constructor
 * takes a local directory path.
 *
 * NOTE: Check isValid() to determine if the directory is supported by this class.
 *
 * @param path Local directory path (UTF-8)
 */
WiiUPackage::WiiUPackage(const char *path)
	: super(new WiiUPackagePrivate(path))
{
	init();
}

#if defined(_WIN32) && defined(_UNICODE)
/**
 * Read a Wii U NUS package.
 *
 * NOTE: Wii U NUS packages are directories. This constructor
 * takes a local directory path.
 *
 * NOTE: Check isValid() to determine if the directory is supported by this class.
 *
 * @param path Local directory path (UTF-8)
 */
WiiUPackage::WiiUPackage(const wchar_t *path)
	: super(new WiiUPackagePrivate(path))
{
	init();
}
#endif /* _WIN32 && _UNICODE */

/**
 * Internal initialization function for the two constructors.
 */
void WiiUPackage::init(void)
{
	RP_D(WiiUPackage);
	d->mimeType = "inode/directory";
	d->fileType = FileType::ApplicationPackage;

	if (d->path.empty()) {
		// No path specified...
		d->reset();
		return;
	}

	// Check if this path is supported.
	d->packageType = static_cast<WiiUPackagePrivate::PackageType>(isDirSupported_static(d->path));

	d->isValid = ((int)d->packageType >= 0);
	if (!d->isValid) {
		d->reset();
		return;
	}

	// Open the ticket.
	// NOTE: May not be present in extracted packages.
	WiiTicket *ticket = nullptr;
	int ticketFormatVersion = -1;

	tstring s_path(d->path);
	s_path += DIR_SEP_CHR;
	const size_t orig_path_size = s_path.size();
	if (d->packageType == WiiUPackagePrivate::PackageType::Extracted) {
		s_path += _T("code");
		s_path += DIR_SEP_CHR;
	}
	s_path += _T("title.tik");
	IRpFilePtr subfile = std::make_shared<RpFile>(s_path, RpFile::FM_OPEN_READ);
	if (subfile->isOpen()) {
		ticket = new WiiTicket(subfile);
		if (ticket->isValid()) {
			// Check the ticket version.
			// Wii U titles are generally v1.
			// vWii titles are v0.
			ticketFormatVersion = ticket->ticketFormatVersion();
			if (ticketFormatVersion != 0 && ticketFormatVersion != 1) {
				// Not v0 or v1.
				delete ticket;
				ticket = nullptr;
			}
		} else {
			delete ticket;
			ticket = nullptr;
		}
	}
	subfile.reset();
	if (!ticket && d->packageType == WiiUPackagePrivate::PackageType::NUS) {
		// Unable to load the ticket.
		d->reset();
		d->isValid = false;
		return;
	}
	d->ticket.reset(ticket);

	// Open the TMD.
	// NOTE: May not be present in extracted packages.
	WiiTMD *tmd = nullptr;
	int tmdFormatVersion = -1;

	s_path.resize(orig_path_size);
	if (d->packageType == WiiUPackagePrivate::PackageType::Extracted) {
		s_path += _T("code");
		s_path += DIR_SEP_CHR;
	}
	s_path += _T("title.tmd");
	subfile = std::make_shared<RpFile>(s_path, RpFile::FM_OPEN_READ);
	if (subfile->isOpen()) {
		tmd = new WiiTMD(subfile);
		if (tmd->isValid()) {
			// Check the TMD version.
			// Wii U titles are generally v1.
			// vWii titles are v0.
			tmdFormatVersion = tmd->tmdFormatVersion();
			if (tmdFormatVersion != 0 && tmdFormatVersion != 1) {
				// Not v0 or v1.
				delete tmd;
				tmd = nullptr;
			}
		} else {
			delete tmd;
			tmd = nullptr;
		}
	}
	subfile.reset();
	if (!tmd && d->packageType == WiiUPackagePrivate::PackageType::NUS) {
		// Unable to load the TMD.
		d->reset();
		d->isValid = false;
		return;
	}
	d->tmd.reset(tmd);

	if (d->packageType != WiiUPackagePrivate::PackageType::NUS) {
		// Only NUS format needs decryption.
		// Extracted format is already decrypted.
		return;
	}

	// NOTE: From this point on, if an error occurs, we won't reset fields.
	// This will allow Ticket and TMD to be displayed, even if we can't
	// decrypt anything else.

#if ENABLE_DECRYPTION
	// Decrypt the title key.
	int ret = ticket->decryptTitleKey(d->title_key, sizeof(d->title_key));
	if (ret != 0) {
		// Failed to decrypt the title key.
		// TODO: verifyResult
		return;
	}
#endif /* ENABLE_DECRYPTION */

	if (tmdFormatVersion < 1) {
		// This is a vWii title. No V1 contents table.
		// There's also usually no useful icon.
		// TODO: Do what WiiWAD does?
		return;
	}

	// Read the contents table for group 0.
	// TODO: Multiple groups?
	d->contentsTable = tmd->contentsTableV1(0);
	if (d->contentsTable.empty()) {
		// No contents?
		return;
	}

	// Initialize the contentsReaders vector based on the number of contents.
	d->contentsReaders.resize(d->contentsTable.size());

	// Find and load the FST.
	// NOTE: tmd->bootIndex() byteswaps to host-endian. Swap it back for comparisons.
	// NOTE: Many dev system titles do NOT have the FST as the bootable content,
	// but it *is* always index 0. Hence, search for index 0 instead of the boot_index.
	IDiscReaderPtr fstReader;
	unsigned int i = 0;
	for (const auto &p : d->contentsTable) {
		if (p.index == 0) {
			// Found it!
			fstReader = d->openContentFile(i);
			break;
		}
		i++;
	}
	if (!fstReader) {
		// Could not open the FST.
		return;
	}

	// Need to load the entire FST, which will be memcpy()'d by WiiUFst.
	// TODO: Eliminate a copy.
	off64_t fst_size64 = fstReader->size();
	if (fst_size64 <= 0 || fst_size64 > 1048576U) {
		// FST is empty and/or too big?
		return;
	}
	const size_t fst_size = static_cast<size_t>(fst_size64);
	unique_ptr<uint8_t[]> fst_buf(new uint8_t[fst_size]);
	size_t size = fstReader->read(fst_buf.get(), fst_size);
	if (size != static_cast<size_t>(fst_size)) {
		// Read error.
		return;
	}

	// Create the WiiUFst.
	WiiUFst *const fst = new WiiUFst(fst_buf.get(), static_cast<uint32_t>(fst_size));
	if (!fst->isOpen()) {
		// FST is invalid?
		// NOTE: boot1 does not have an FST.
		delete fst;
		return;
	}
	d->fst.reset(fst);

	// FST loaded.
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int WiiUPackage::isRomSupported_static(const DetectInfo *info)
{
	// Files are not supported.
	RP_UNUSED(info);
	return -1;
}

/**
 * Is a directory supported by this class?
 * @param path Directory to check
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int WiiUPackage::isDirSupported_static(const char *path)
{
	// Check for an NUS package.
	static const array<const char*, 3> NUS_package_filenames = {{
		"title.tik",	// Ticket
		"title.tmd",	// TMD
		"title.cert",	// Certificate chain
	}};

	if (RomDataPrivate::T_isDirSupported_allFiles_static(path, NUS_package_filenames)) {
		return static_cast<int>(WiiUPackagePrivate::PackageType::NUS);
	}

	// Check for an extracted title.
	// NOTE: Ticket, TMD, and certificate chain might not be present.
	static const array<const char*, 3> extracted_package_filenames = {{
		"code/app.xml",
		"code/cos.xml",
		"meta/meta.xml",
	}};

	if (RomDataPrivate::T_isDirSupported_allFiles_static(path, extracted_package_filenames)) {
		return static_cast<int>(WiiUPackagePrivate::PackageType::Extracted);
	}

	// Not supported.
	return static_cast<int>(WiiUPackagePrivate::PackageType::Unknown);
}

#if defined(_WIN32) && defined(_UNICODE)
/**
 * Is a directory supported by this class?
 * @param path Directory to check
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int WiiUPackage::isDirSupported_static(const wchar_t *path)
{
	// Check for an NUS package.
	static const array<const wchar_t*, 3> NUS_package_filenames = {{
		L"title.tik",	// Ticket
		L"title.tmd",	// TMD
		L"title.cert",	// Certificate chain
	}};

	if (RomDataPrivate::T_isDirSupported_allFiles_static(path, NUS_package_filenames)) {
		return static_cast<int>(WiiUPackagePrivate::PackageType::NUS);
	}

	// Check for an extracted title.
	// NOTE: Ticket, TMD, and certificate chain might not be present.
	static const array<const wchar_t*, 3> extracted_package_filenames = {{
		L"code/app.xml",
		L"code/cos.xml",
		L"meta/meta.xml",
	}};

	if (RomDataPrivate::T_isDirSupported_allFiles_static(path, extracted_package_filenames)) {
		return static_cast<int>(WiiUPackagePrivate::PackageType::Extracted);
	}

	// Not supported.
	return static_cast<int>(WiiUPackagePrivate::PackageType::Unknown);
}
#endif /* defined(_WIN32) && defined(_UNICODE) */

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *WiiUPackage::systemName(unsigned int type) const
{
	RP_D(const WiiUPackage);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// WiiUPackage has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"WiiUPackage::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	static const array<const char*, 4> sysNames = {{
		"Nintendo Wii U", "Wii U", "Wii U", nullptr
	}};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t WiiUPackage::supportedImageTypes_static(void)
{
#ifdef ENABLE_XML

#ifdef HAVE_JPEG
	return IMGBF_INT_ICON |
	       IMGBF_EXT_MEDIA |
	       IMGBF_EXT_COVER | IMGBF_EXT_COVER_3D |
	       IMGBF_EXT_COVER_FULL;
#else /* !HAVE_JPEG */
	return IMGBF_INT_ICON |
	       IMGBF_EXT_MEDIA | IMGBF_EXT_COVER_3D;
#endif /* HAVE_JPEG */

#else /* !ENABLE_XML */
	return IMGBF_INT_ICON;
#endif /* ENABLE_XML */
}

/**
 * Get a bitfield of image types this object can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t WiiUPackage::supportedImageTypes(void) const
{
	RP_D(const WiiUPackage);
	uint32_t ret = 0;
	if (d->tmd && d->tmd->tmdFormatVersion() >= 1) {
		// Wii U packages have an icon.
		ret = IMGBF_INT_ICON;
	}

#ifdef ENABLE_XML

#ifdef HAVE_JPEG
	ret |= IMGBF_EXT_MEDIA |
	       IMGBF_EXT_COVER | IMGBF_EXT_COVER_3D |
	       IMGBF_EXT_COVER_FULL;
#else /* !HAVE_JPEG */
	ret |= IMGBF_INT_ICON |
	       IMGBF_EXT_MEDIA | IMGBF_EXT_COVER_3D;
#endif /* HAVE_JPEG */

#endif /* ENABLE_XML */

	return ret;
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> WiiUPackage::supportedImageSizes_static(ImageType imageType)
{
	ASSERT_supportedImageSizes(imageType);

	switch (imageType) {
		case IMG_INT_ICON: {
			// Wii U icons are usually 128x128.
			return {{nullptr, 128, 128, 0}};
		}

#ifdef ENABLE_XML
		case IMG_EXT_MEDIA:
			return {
				{nullptr, 160, 160, 0},
				{"M", 500, 500, 1},
			};
#ifdef HAVE_JPEG
		case IMG_EXT_COVER:
			return {
				{nullptr, 160, 224, 0},
				{"M", 350, 500, 1},
				{"HQ", 768, 1080, 2},
			};
#endif /* HAVE_JPEG */
		case IMG_EXT_COVER_3D:
			return {{nullptr, 176, 248, 0}};
#ifdef HAVE_JPEG
		case IMG_EXT_COVER_FULL:
			return {
				{nullptr, 340, 224, 0},
				{"M", 752, 500, 1},
				{"HQ", 1632, 1080, 2},
			};
#endif /* HAVE_JPEG */
#endif /* ENABLE_XML */
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
vector<RomData::ImageSizeDef> WiiUPackage::supportedImageSizes(ImageType imageType) const
{
	ASSERT_supportedImageSizes(imageType);

	if (imageType == IMG_INT_ICON) {
		// IMG_INT_ICON requires a Wii U (v1) TMD.
		RP_D(const WiiUPackage);
		if (d->tmd && d->tmd->tmdFormatVersion() >= 1) {
			// Wii U packages have an icon.
			return {{nullptr, 128, 128, 0}};;
		} else {
			// Not a Wii U (v1) TMD.
			return {};
		}
	}

	// Other image types don't depend on the TMD.
	return supportedImageSizes_static(imageType);
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int WiiUPackage::loadFieldData(void)
{
	RP_D(WiiUPackage);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (d->path.empty()) {
		// No directory...
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown ROM image type.
		return -EIO;
	}

	d->fields.reserve(10);	// Maximum of 10 fields.

	// TODO: Show a decryption key warning and/or "no XMLs".
	d->fields.setTabName(0, "Wii U");

	bool canLoadXMLs = false;
	if (d->packageType == WiiUPackagePrivate::PackageType::NUS) {
		// Check if the decryption keys were loaded.
		const KeyManager::VerifyResult verifyResult = d->ticket->verifyResult();
		if (verifyResult == KeyManager::VerifyResult::OK) {
			// We can decrypt the XMLs if the FST was loaded.
			canLoadXMLs = (d->fst != nullptr);
		} else {
			// No decryption keys, so we can't decrypt the XMLs.
			// Show a warning.
			// NOTE: We can still show ticket/TMD fields.
			canLoadXMLs = false;
			const char *err = KeyManager::verifyResultToString(verifyResult);
			if (!err) {
				err = C_("RomData", "Unknown error. (THIS IS A BUG!)");
			}
			d->fields.addField_string(C_("RomData", "Warning"), err,
				RomFields::STRF_WARNING);
		}
	} else if (d->packageType == WiiUPackagePrivate::PackageType::Extracted) {
		// XML can always be loaded in extracted packages.
		canLoadXMLs = true;
	}

#ifdef ENABLE_XML
	// Parse the Wii U System XMLs.
	// NOTE: Only if the FST was loaded, or reading an extracted package.
	if (canLoadXMLs) {
		int ret = d->addFields_System_XMLs();
		if (ret != 0) {
			d->fields.addField_string(C_("RomData", "Warning"),
				C_("RomData", "XML parsing failed."),
				RomFields::STRF_WARNING);
		}
	}
#else /* !ENABLE_XML */
	d->fields.addField_string(C_("RomData", "Warning"),
		C_("RomData", "XML parsing is disabled in this build."),
		RomFields::STRF_WARNING);
#endif /* ENABLE_XML */

	// Add the ticket and/or TMD fields.
	// NOTE: If the XMLs aren't found, we'll need to reuse tab 0.
	if (d->ticket) {
		const RomFields *const ticket_fields = d->ticket->fields();
		assert(ticket_fields != nullptr);
		if (ticket_fields) {
			// TODO: Localize this?
			if (d->fields.count() == 0) {
				d->fields.setTabName(0, "Ticket");
			} else {
				d->fields.addTab("Ticket");
			}
			d->fields.addFields_romFields(ticket_fields, -1);
		}
	}
	if (d->tmd) {
		const RomFields *const tmd_fields = d->tmd->fields();
		assert(tmd_fields != nullptr);
		if (tmd_fields) {
			if (d->fields.count() == 0) {
				d->fields.setTabName(0, "TMD");
			} else {
				d->fields.addTab("TMD");
			}
			d->fields.addFields_romFields(tmd_fields, -1);
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
int WiiUPackage::loadMetaData(void)
{
	RP_D(WiiUPackage);
	if (!d->metaData.empty()) {
		// Metadata *has* been loaded...
		return 0;
	} else if (d->path.empty()) {
		// No directory...
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown ROM image type.
		return -EIO;
	}

#ifdef ENABLE_XML
	// Check if the decryption keys were loaded.
	const KeyManager::VerifyResult verifyResult = d->ticket->verifyResult();
	if (verifyResult == KeyManager::VerifyResult::OK) {
		// Decryption keys were loaded. We can add XML fields.
		// Parse the Wii U System XMLs.
		d->metaData.reserve(2);	// Maximum of 2 metadata properties.
		d->addMetaData_System_XMLs();
	}
#endif /* ENABLE_XML */

	// No ticket/TMD metadata; the only thing it sets is the
	// "Title" property, which is the Title ID.

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
int WiiUPackage::loadInternalImage(ImageType imageType, rp_image_const_ptr &pImage)
{
	ASSERT_loadInternalImage(imageType, pImage);
	RP_D(WiiUPackage);
	ROMDATA_loadInternalImage_single(
		IMG_INT_ICON,	// ourImageType
		d->path.c_str(),// file (NOTE: Using d->path because we don't have a "file")
		d->isValid,	// isValid
		0,		// romType
		d->img_icon,	// imgCache
		d->loadIcon);	// func
}

/**
 * Get a list of URLs for an external image type.
 *
 * A thumbnail size may be requested from the shell.
 * If the subclass supports multiple sizes, it should
 * try to get the size that most closely matches the
 * requested size.
 *
 * @param imageType	[in]     Image type
 * @param extURLs	[out]    Output vector
 * @param size		[in,opt] Requested image size. This may be a requested
 *                               thumbnail size in pixels, or an ImageSizeType
 *                               enum value.
 * @return 0 on success; negative POSIX error code on error.
 */
int WiiUPackage::extURLs(ImageType imageType, vector<ExtURL> &extURLs, int size) const
{
	extURLs.clear();

	RP_D(const WiiUPackage);
	if (!d->isValid) {
		// Package isn't valid.
		return -EIO;
	}

#ifdef ENABLE_XML
	if (d->tmd && d->tmd->tmdFormatVersion() >= 1) {
		// This is a Wii U (v1) TMD. We can get Wii U XML files.

		// Get the game ID and application type from the system XML.
		// Format: "WUP-X-ABCD"
		uint32_t applType = 0;
		const string productCode = const_cast<WiiUPackagePrivate*>(d)->getProductCodeAndApplType_xml(&applType);
		if (productCode.empty() || productCode.size() != 10 || productCode.compare(0, 4, "WUP-", 4) != 0 || productCode[5] != '-') {
			// Invalid product code.
			// TODO: Check 'X'?
			return -ENOENT;
		} else if (applType != 0x80000000) {
			// Not a game.
			return -ENOENT;
		}

		const char *const id4 = &productCode[6];

		return WiiU::extURLs_int(id4, imageType, extURLs, size);
	}

	// TODO: Wii-style external images?
	return -ENOENT;
#else /* !ENABLE_XML */
	// Cannot check the system XML without XML support.
	return -ENOTSUP;
#endif /* ENABLE_XML */
}

} // namespace LibRomData
