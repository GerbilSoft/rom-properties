/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiUPackage.cpp: Wii U NUS Package reader.                              *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "WiiUPackage.hpp"
#include "WiiUPackage_p.hpp"

// TGA FileFormat
#include "librptexture/fileformat/TGA.hpp"

// Other rom-properties libraries
#include "librpbase/disc/PartitionFile.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;
using namespace LibRpTexture;

// C++ STL classes
using std::string;
using std::unique_ptr;
using std::vector;

namespace LibRomData {

ROMDATA_IMPL(WiiUPackage)
ROMDATA_IMPL_IMG(WiiUPackage)

/** WiiUPackagePrivate **/

/* RomDataInfo */
const char *const WiiUPackagePrivate::exts[] = {
	// No file extensions; NUS packages are directories.
	nullptr
};
const char *const WiiUPackagePrivate::mimeTypes[] = {
	// NUS packages are directories.
	"inode/directory",

	nullptr
};
const RomDataInfo WiiUPackagePrivate::romDataInfo = {
	"WiiUPackage", exts, mimeTypes
};

WiiUPackagePrivate::WiiUPackagePrivate(const char *path)
	: super({}, &romDataInfo)
	, ticket(nullptr)
	, tmd(nullptr)
	, fst(nullptr)
{
	if (path && path[0] != '\0') {
		this->path = strdup(path);
	} else {
		this->path = nullptr;
	}

#ifdef ENABLE_DECRYPTION
	// Clear the title key.
	memset(title_key, 0, sizeof(title_key));
#endif /* ENABLE_DECRYPTION */
}

WiiUPackagePrivate::~WiiUPackagePrivate()
{
	free(path);
	delete ticket;
	delete tmd;
	delete fst;
}

/**
 * Clear everything.
 */
void WiiUPackagePrivate::reset(void)
{
	free(path);
	delete ticket;
	delete tmd;
	delete fst;

	path = nullptr;
	ticket = nullptr;
	tmd = nullptr;
	path = nullptr;
}

/**
 * Open a content file.
 * @param idx Content index (TMD index)
 * @return Content file, or nullptr on error.
 */
IDiscReaderPtr WiiUPackagePrivate::openContentFile(unsigned int idx)
{
	assert(idx < contentsReaders.size());
	if (idx >= contentsReaders.size())
		return {};

	if (contentsReaders[idx]) {
		// Content is already open.
		return contentsReaders[idx];
	}

#ifdef ENABLE_DECRYPTION
	// Attempt to open the content.
	const WUP_Content_Entry &entry = contentsTable[idx];
	string s_path(this->path);
	s_path += DIR_SEP_CHR;

	// Try with lowercase hex first.
	const uint32_t content_id = be32_to_cpu(entry.content_id);
	char fnbuf[16];
	snprintf(fnbuf, sizeof(fnbuf), "%08x.app", content_id);
	s_path += fnbuf;

	IRpFilePtr subfile = std::make_shared<RpFile>(s_path.c_str(), RpFile::FM_OPEN_READ);
	if (!subfile->isOpen()) {
		// Try with uppercase hex.
		snprintf(fnbuf, sizeof(fnbuf), "%08X.app", content_id);
		s_path.resize(s_path.size()-12);
		s_path += fnbuf;

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
		uint8_t iv[16];
		memcpy(iv, &entry.index, 2);
		memset(&iv[2], 0, 14);

		discReader = std::make_shared<CBCReader>(subfile, 0, subfile->size(), title_key, iv);
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
	// NOTE: PartitionFile does not take a shared_ptr<> because it's
	// also created from within an IPartition object, so it can't
	// get its own shared_ptr<>.
	// The IDiscReaderPtr above must remain valid while this PartitionFilePtr is valid.
	return std::make_shared<PartitionFile>(contentFile.get(), dirent.offset, dirent.size);
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
	} else if (!this->isValid || !this->fst) {
		// Can't load the icon.
		return {};
	}

	// Icon is "/meta/iconTex.tga".
	IRpFilePtr f_icon = this->open("/meta/iconTex.tga");
	if (!f_icon) {
		// Icon not found?
		return {};
	}

	// Attempt to open the icon as TGA.
	unique_ptr<TGA> tga(new TGA(f_icon));
	if (!tga->isValid()) {
		// Not a valid TGA file.
		return {};
	}

	// Get the icon.
	img_icon = tga->image();
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
	: super(new WiiUPackagePrivate(nullptr))
{
	// Not supported!
	RP_UNUSED(file);
	return;
}

/**
 * Read a Wii U NUS package.
 *
 * NOTE: Wii U NUS packages are directories. This constructor
 * takes a local directory path.
 *
 * A ROM image must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the ROM image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param path Local directory path (UTF-8)
 */
WiiUPackage::WiiUPackage(const char *path)
	: super(new WiiUPackagePrivate(path))
{
	RP_D(WiiUPackage);
	d->fileType = FileType::ApplicationPackage;

	if (!d->path) {
		// No path specified...
		d->reset();
		return;
	}

	// Check if this path is supported.
	d->isValid = (isDirSupported_static(path) >= 0);

	if (!d->isValid) {
		d->reset();
		return;
	}

	// Open the ticket.
	WiiTicket *ticket = nullptr;

	string s_path(d->path);
	s_path += DIR_SEP_CHR;
	s_path += "title.tik";
	IRpFilePtr subfile = std::make_shared<RpFile>(s_path, RpFile::FM_OPEN_READ);
	if (subfile->isOpen()) {
		ticket = new WiiTicket(subfile);
		if (ticket->isValid()) {
			// Make sure the ticket is v1.
			if (ticket->ticketFormatVersion() != 1) {
				// Not v1.
				delete ticket;
				ticket = nullptr;
			}
		} else {
			delete ticket;
			ticket = nullptr;
		}
	}
	subfile.reset();
	if (!ticket) {
		// Unable to load the ticket.
		d->reset();
		d->isValid = false;
		return;
	}
	d->ticket = ticket;

	// Open the TMD.
	WiiTMD *tmd = nullptr;

	s_path.resize(s_path.size()-9);
	s_path += "title.tmd";
	subfile = std::make_shared<RpFile>(s_path, RpFile::FM_OPEN_READ);
	if (subfile->isOpen()) {
		tmd = new WiiTMD(subfile);
		if (tmd->isValid()) {
			// Make sure the TMD is v1.
			if (tmd->tmdFormatVersion() != 1) {
				// Not v1.
				delete tmd;
				tmd = nullptr;
			}
		} else {
			delete tmd;
			tmd = nullptr;
		}
	}
	subfile.reset();
	if (!tmd) {
		// Unable to load the TMD.
		d->reset();
		d->isValid = false;
		return;
	}
	d->tmd = tmd;

	// NOTE: From this point on, if an error occurs, we won't reset fields.
	// This will allow Ticket and TMD to be displayed, even if we can't
	// decrypt anything else.

#if ENABLE_DECRYPTION
	// Decrypt the title key.
	int ret = d->ticket->decryptTitleKey(d->title_key, sizeof(d->title_key));
	if (ret != 0) {
		// Failed to decrypt the title key.
		// TODO: verifyResult
		return;
	}
#endif /* ENABLE_DECRYPTION */

	// Read the contents table for group 0.
	// TODO: Multiple groups?
	d->contentsTable = tmd->contentsTableV1(0);
	if (d->contentsTable.empty()) {
		// No contents?
		return;
	}

	// Initialize the contentsReaders vector based on the number of contents.
	d->contentsReaders.resize(d->contentsTable.size());

	// Find and load the FST. (It has the "bootable" flag, and is usually the first content.)
	// NOTE: tmd->bootIndex() byteswaps to host-endian. Swap it back for comparisons.
	IDiscReaderPtr fstReader;
	const uint16_t boot_index = cpu_to_be16(tmd->bootIndex());
	unsigned int i = 0;
	for (const auto &p : d->contentsTable) {
		if (p.index == boot_index) {
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
	off64_t fst_size = fstReader->size();
	if (fst_size <= 0 || fst_size > 1048576U) {
		// FST is empty and/or too big?
		return;
	}
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
		return;
	}
	d->fst = fst;

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
 * @param path Directory to check.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int WiiUPackage::isDirSupported_static(const char *path)
{
	assert(path != nullptr);
	assert(path[0] != '\0');
	if (!path || path[0] == '\0') {
		// No path specified.
		return -1;
	}

	string s_path(path);
	s_path += DIR_SEP_CHR;

	/// Check for the ticket, TMD, and certificate chain files.

	s_path += "title.tik";
	if (FileSystem::access(s_path.c_str(), R_OK) != 0) {
		// No ticket.
		return -1;
	}

	s_path.resize(s_path.size()-4);
	s_path += ".tmd";
	if (FileSystem::access(s_path.c_str(), R_OK) != 0) {
		// No TMD.
		return -1;
	}

	s_path.resize(s_path.size()-4);
	s_path += ".cert";
	if (FileSystem::access(s_path.c_str(), R_OK) != 0) {
		// No certificate chain.
		return -1;
	}

	// This appears to be a Wii U NUS package.
	return 0;
}

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
	static const char *const sysNames[4] = {
		"Nintendo Wii U", "Wii U", "Wii U", nullptr
	};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t WiiUPackage::supportedImageTypes_static(void)
{
	return IMGBF_INT_ICON;;
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
		case IMG_INT_ICON:
			// Wii U icons are usually 128x128.
			return {{nullptr, 128, 128, 0}};
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
int WiiUPackage::loadFieldData(void)
{
	RP_D(WiiUPackage);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->path) {
		// No directory...
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown ROM image type.
		return -EIO;
	}

#ifdef ENABLE_XML
	d->fields.reserve(10);	// Maximum of 10 fields.

	// Check if the decryption keys were loaded.
	const KeyManager::VerifyResult verifyResult = d->ticket->verifyResult();
	if (verifyResult == KeyManager::VerifyResult::OK) {
		// Decryption keys were loaded. We can add XML fields.
		// Parse the Wii U System XMLs.
		d->addFields_System_XMLs();
	} else {
		// Decryption keys were NOT loaded. Show a warning.
		// NOTE: We can still show ticket/TMD fields.
		const char *err = KeyManager::verifyResultToString(verifyResult);
		if (!err) {
			err = C_("RomData", "Unknown error. (THIS IS A BUG!)");
		}
		d->fields.addField_string(C_("RomData", "Warning"), err,
			RomFields::STRF_WARNING);
	}
#endif /* ENABLE_XML */

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
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->path) {
		// No directory...
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown ROM image type.
		return -EIO;
	}

	// Create the metadata object.
	d->metaData = new RomMetaData();
	d->metaData->reserve(2);	// Maximum of 2 metadata properties.

#ifdef ENABLE_XML
	// Check if the decryption keys were loaded.
	const KeyManager::VerifyResult verifyResult = d->ticket->verifyResult();
	if (verifyResult == KeyManager::VerifyResult::OK) {
		// Decryption keys were loaded. We can add XML fields.
		// Parse the Wii U System XMLs.
		d->addMetaData_System_XMLs();
	}
#endif /* ENABLE_XML */

	// Finished reading the metadata.
	return static_cast<int>(d->metaData->count());
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
		d->path,	// file (NOTE: Using d->path because we don't have a "file")
		d->isValid,	// isValid
		0,		// romType
		d->img_icon,	// imgCache
		d->loadIcon);	// func
}

}
