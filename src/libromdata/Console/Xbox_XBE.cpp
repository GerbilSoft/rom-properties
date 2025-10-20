/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Xbox_XBE.cpp: Microsoft Xbox executable reader.                         *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "Xbox_XBE.hpp"
#include "xbox_xbe_structs.h"
#include "data/XboxPublishers.hpp"

// Other rom-properties libraries
#include "librpbase/img/RpPng.hpp"
#include "librpfile/SubFile.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;

// librptexture
#include "librptexture/fileformat/XboxXPR.hpp"
using namespace LibRpTexture;

// Other RomData subclasses
#include "Other/EXE.hpp"

// C++ STL classes
using std::array;
using std::ostringstream;
using std::shared_ptr;
using std::string;
using std::unique_ptr;
using std::variant;
using std::vector;

namespace LibRomData {

// Workaround for RP_D() expecting the no-underscore naming convention.
#define Xbox_XBEPrivate Xbox_XBE_Private

class Xbox_XBE_Private final : public RomDataPrivate
{
public:
	explicit Xbox_XBE_Private(const IRpFilePtr &file);

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(Xbox_XBE_Private)

public:
	/** RomDataInfo **/
	static const array<const char*, 1+1> exts;
	static const array<const char*, 1+1> mimeTypes;
	static const RomDataInfo romDataInfo;

public:
	// Region code bitfield names
	static const array<const char*, 4> region_code_bitfield_names;

public:
	// XBE header
	// NOTE: **NOT** byteswapped.
	XBE_Header xbeHeader;

	// XBE certificate
	// NOTE: **NOT** byteswapped.
	XBE_Certificate xbeCertificate;

	// RomData subclasses
	// TODO: Also get the save image? ($$XSIMAGE)
	unique_ptr<EXE> pe_exe;		// PE executable

	// Title image
	// NOTE: May be a PNG image on some discs.
	struct xtImage_t {
		variant<XboxXPRPtr, rp_image_ptr> data;
		bool isInit;

		xtImage_t()
			: isInit(false)
		{ }
	};
	xtImage_t xtImage;

public:
	/**
	 * Find an XBE section header.
	 * @param name		[in] Section header name.
	 * @param pOutHeader	[out] Buffer to store the header. (Byteswapped to host-endian.)
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int findXbeSectionHeader(const char *name, XBE_Section_Header *pOutHeader);

	/**
	 * Initialize the title image object.
	 * NOTE: Check xtImage's union after initializing.
	 * @return 0 if initialized; negative POSIX error code on error.
	 */
	int initXPR0_xtImage(void);

	/**
	 * Initialize the PE executable object.
	 * @return EXE object on success; nullptr on error.
	 */
	const EXE *initEXE(void);

public:
	/**
	 * Get the title ID.
	 * @return Title ID, or empty string on error.
	 */
	string getTitleID(void) const;

	/**
	 * Get the publisher.
	 * @return Publisher.
	 */
	string getPublisher(void) const;

	/**
	 * Get the region code.
	 * Uses XBE_Region_Code_e, but MANUFACTURING is moved to bit 3.
	 * @return Region code
	 */
	unsigned int getRegionCode(void) const;
};

ROMDATA_IMPL(Xbox_XBE)

/** Xbox_XBE_Private **/

/* RomDataInfo */
const array<const char*, 1+1> Xbox_XBE_Private::exts = {{
	".xbe",

	nullptr
}};
const array<const char*, 1+1> Xbox_XBE_Private::mimeTypes = {{
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"application/x-xbox-executable",

	nullptr
}};
const RomDataInfo Xbox_XBE_Private::romDataInfo = {
	"Xbox_XBE", exts.data(), mimeTypes.data()
};

// Region code bitfield names
const array<const char*, 4> Xbox_XBE_Private::region_code_bitfield_names = {{
	NOP_C_("Region", "North America"),
	NOP_C_("Region", "Japan"),
	NOP_C_("Region", "Rest of World"),
	NOP_C_("Region", "Manufacturing"),
}};

Xbox_XBE_Private::Xbox_XBE_Private(const IRpFilePtr &file)
	: super(file, &romDataInfo)
{
	// Clear the XBE structs.
	memset(&xbeHeader, 0, sizeof(xbeHeader));
	memset(&xbeCertificate, 0, sizeof(xbeCertificate));
}

/**
 * Find an XBE section header.
 * @param name		[in] Section header name.
 * @param pOutHeader	[out] Buffer to store the header. (Byteswapped to host-endian.)
 * @return 0 on success; negative POSIX error code on error.
 */
int Xbox_XBE_Private::findXbeSectionHeader(const char *name, XBE_Section_Header *pOutHeader)
{
	if (!file || !file->isOpen()) {
		// File is not open.
		return -EBADF;
	}

	// We're loading the first 64 KB of the executable.
	// Section headers and names are usually there.
	// TODO: Find any exceptions?
	static constexpr size_t XBE_READ_SIZE = 64*1024;

	// Load the section headers
	const uint32_t base_address = le32_to_cpu(xbeHeader.base_address);
	const uint32_t section_headers_address = le32_to_cpu(xbeHeader.section_headers_address);
	if (section_headers_address <= base_address) {
		// Out of range.
		// NOTE: <= - base address would have the magic number.
		return -EIO;
	}

	const uint32_t shdr_address_phys = section_headers_address - base_address;
	if (shdr_address_phys >= XBE_READ_SIZE) {
		// Section headers is not in the first 64 KB.
		return -EIO;
	}

	// Read the XBE header
	unique_ptr<array<uint8_t, XBE_READ_SIZE> > first64KB(new array<uint8_t, XBE_READ_SIZE>);
	size_t size = file->seekAndRead(0, first64KB->data(), first64KB->size());
	if (size != XBE_READ_SIZE) {
		// Seek and/or read error.
		return -EIO;
	}

	// Section count
	unsigned int section_count = le32_to_cpu(xbeHeader.section_count);
	// If this goes over the 64 KB limit, reduce the section count.
	if (shdr_address_phys + (section_count * sizeof(XBE_Section_Header)) > XBE_READ_SIZE) {
		// Out of bounds. Reduce it.
		section_count = (XBE_READ_SIZE - shdr_address_phys) / sizeof(XBE_Section_Header);
	}

	// First section header
	const XBE_Section_Header *pHdr = reinterpret_cast<const XBE_Section_Header*>(
		&(*first64KB)[shdr_address_phys]);
	const XBE_Section_Header *const pHdr_end = pHdr + section_count;

	// Find the specified section.
	// TODO: Cache a "not found" result so we don't have to
	// re-check the section headers again?
	for (; pHdr < pHdr_end; pHdr++) {
		char section_name[16];	// Allow up to 15 chars plus NULL terminator.

		const uint32_t name_address = le32_to_cpu(pHdr->section_name_address);
		if (name_address <= base_address) {
			// Out of range.
			continue;
		}

		// Read the name.
		size = file->seekAndRead(name_address - base_address, section_name, sizeof(section_name));
		if (size != sizeof(section_name)) {
			// Seek and/or read error.
			return -EIO;
		}
		section_name[sizeof(section_name)-1] = '\0';

		if (!strcmp(section_name, name)) {
			// Found it!
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
			// No byteswapping needed. Copy the data directly.
			*pOutHeader = *pHdr;
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
			// Byteswap the data.
			pOutHeader->flags = le32_to_cpu(pHdr->flags);
			pOutHeader->vaddr = le32_to_cpu(pHdr->vaddr);
			pOutHeader->vsize = le32_to_cpu(pHdr->vsize);
			pOutHeader->paddr = le32_to_cpu(pHdr->paddr);
			pOutHeader->psize = le32_to_cpu(pHdr->psize);
			pOutHeader->section_name_address		= le32_to_cpu(pHdr->section_name_address);
			pOutHeader->section_name_refcount		= le32_to_cpu(pHdr->section_name_refcount);
			pOutHeader->head_shared_page_recount_address	= le32_to_cpu(pHdr->head_shared_page_recount_address);
			pOutHeader->tail_shared_page_recount_address	= le32_to_cpu(pHdr->tail_shared_page_recount_address);
			memcpy(pOutHeader->sha1_digest, pHdr->sha1_digest, sizeof(pHdr->sha1_digest));
#endif /* SYS_BYTEORDER == SYS_LIL_ENDIAN */
			return 0;
		}
	}

	// Not found.
	return -ENOENT;
}

/**
 * Initialize the title image object.
 * NOTE: Check xtImage's union after initializing.
 * @return 0 if initialized; negative POSIX error code on error.
 */
int Xbox_XBE_Private::initXPR0_xtImage(void)
{
	if (xtImage.isInit) {
		// Title image is already initialized.
		return 0;
	}

	// Find the $$XTIMAGE section.
	XBE_Section_Header hdr_xtImage;
	int ret = findXbeSectionHeader("$$XTIMAGE", &hdr_xtImage);
	if (ret != 0) {
		// Not found.
		return -ENOENT;
	}

	// Open the XPR0 image.
	// paddr/psize have absolute addresses.
	shared_ptr<SubFile> subFile = std::make_shared<SubFile>(this->file,
		hdr_xtImage.paddr, hdr_xtImage.psize);
	if (!subFile->isOpen()) {
		// Unable to open the XPR0 file.
		return -EIO;
	}

	// $$XTIMAGE is usually an XPR0 image.
	// The Burger King games, wihch have both Xbox and Xbox 360
	// executables, incorrectly use PNG format here.
	uint32_t magic = 0;
	size_t size = subFile->read(&magic, sizeof(magic));
	if (size != sizeof(magic)) {
		// Read error.
		return -EIO;
	}
	subFile->rewind();

	ret = 0;
	if (magic == cpu_to_be32('XPR0')) {
		// XPR0 image
		XboxXPRPtr xpr0 = std::make_shared<XboxXPR>(subFile);
		if (xpr0->isOpen()) {
			// XPR0 image opened.
			xtImage.data = std::move(xpr0);
			xtImage.isInit = true;
		} else {
			// Unable to open the XPR0 image.
			ret = -EIO;
		}
	} else if (magic == cpu_to_be32(0x89504E47U)) {	// '\x89PNG'
		// PNG image
		rp_image_ptr img = RpPng::load(subFile);
		if (img->isValid()) {
			// PNG image opened.
			xtImage.data = std::move(img);
			xtImage.isInit = true;
		} else {
			// Unable to open the PNG image.
			ret = -EIO;
		}
	}
	return ret;
}

/**
 * Initialize the PE executable object.
 * @return EXE object on success; nullptr on error.
 */
const EXE *Xbox_XBE_Private::initEXE(void)
{
	if (pe_exe) {
		// EXE is already initialized.
		return pe_exe.get();
	}

	if (!file || !file->isOpen()) {
		// File is not open.
		return nullptr;
	}

	// EXE is located at (pe_base_address - base_address).
	const off64_t fileSize = file->size();
	const uint32_t exe_address =
		le32_to_cpu(xbeHeader.pe_base_address) -
		le32_to_cpu(xbeHeader.base_address);
	if (exe_address < sizeof(xbeHeader) || exe_address >= fileSize) {
		// Out of range.
		return nullptr;
	}

	// Open the EXE file.
	shared_ptr<SubFile> subFile = std::make_shared<SubFile>(this->file,
		exe_address, fileSize - exe_address);
	if (subFile->isOpen()) {
		EXE *const pe_exe_tmp = new EXE(subFile);
		if (pe_exe_tmp->isOpen()) {
			// EXE opened.
			this->pe_exe.reset(pe_exe_tmp);
		} else {
			// Failed to open the EXE.
			delete pe_exe_tmp;
		}
	}

	// EXE loaded.
	return this->pe_exe.get();
}

/**
 * Get the title ID.
 * @return Title ID, or empty string on error.
 */
string Xbox_XBE_Private::getTitleID(void) const
{
	if (unlikely(xbeCertificate.title_id.u32 == 0)) {
		// No title ID...
		return {};
	}

	// FIXME: Verify behavior on big-endian.
	// TODO: Consolidate implementations into a shared function.
	string tid_str;
	if (isupper_ascii(xbeCertificate.title_id.a)) {
		tid_str += xbeCertificate.title_id.a;
	} else {
		tid_str += fmt::format(FSTR("\\x{:0>2X}"),
			static_cast<uint8_t>(xbeCertificate.title_id.a));
	}
	if (isupper_ascii(xbeCertificate.title_id.b)) {
		tid_str += xbeCertificate.title_id.b;
	} else {
		tid_str += fmt::format(FSTR("\\x{:0>2X}"),
			static_cast<uint8_t>(xbeCertificate.title_id.b));
	}

	// tr: Xbox title ID (32-bit hex, then two letters followed by a 3-digit decimal number)
	return fmt::format(FRUN(C_("Xbox_XBE", "{0:0>8X} ({1:s}-{2:0>3d})")),
		le32_to_cpu(xbeCertificate.title_id.u32),
		tid_str.c_str(),
		le16_to_cpu(xbeCertificate.title_id.u16));
}

/**
 * Get the publisher.
 * @return Publisher.
 */
string Xbox_XBE_Private::getPublisher(void) const
{
	const uint16_t pub_id = (static_cast<unsigned int>(xbeCertificate.title_id.a) << 8) |
	                        (static_cast<unsigned int>(xbeCertificate.title_id.b));
	const char *const publisher = XboxPublishers::lookup(pub_id);
	if (publisher) {
		return publisher;
	}

	// Unknown publisher.
	if (isalnum_ascii(xbeCertificate.title_id.a) &&
	    isalnum_ascii(xbeCertificate.title_id.b))
	{
		// Publisher ID is alphanumeric.
		return fmt::format(FRUN(C_("RomData", "Unknown ({:c}{:c})")),
			xbeCertificate.title_id.a,
			xbeCertificate.title_id.b);
	}

	// Publisher ID is not alphanumeric.
	return fmt::format(FRUN(C_("RomData", "Unknown ({:0>2X} {:0>2X})")),
		static_cast<uint8_t>(xbeCertificate.title_id.a),
		static_cast<uint8_t>(xbeCertificate.title_id.b));
}

/**
 * Get the region code.
 * Uses XBE_Region_Code_e, but MANUFACTURING is moved to bit 3.
 * @return Region code
 */
unsigned int Xbox_XBE_Private::getRegionCode(void) const
{
	const uint32_t region_code_xbx = le32_to_cpu(xbeCertificate.region_code);
	unsigned int region_code = (region_code_xbx & 0x07);

	// Relocate the MANUFACTURING bit to bit 3 to make it easier to
	// handle the region code table.
	if (region_code_xbx & XBE_REGION_CODE_MANUFACTURING) {
		region_code |= (1U << 3);
	}

	return region_code;
}

/** Xbox_XBE **/

/**
 * Read an Xbox XBE file.
 *
 * A ROM image must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the disc image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open XBE file.
 */
Xbox_XBE::Xbox_XBE(const IRpFilePtr &file)
	: super(new Xbox_XBE_Private(file))
{
	// This class handles executables.
	RP_D(Xbox_XBE);
	d->mimeType = "application/x-xbox-executable";	// unofficial, not on fd.o
	d->fileType = FileType::Executable;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the XBE header.
	d->file->rewind();
	size_t size = d->file->read(&d->xbeHeader, sizeof(d->xbeHeader));
	if (size != sizeof(d->xbeHeader)) {
		d->xbeHeader.magic = 0;
		d->file.reset();
		return;
	}

	// Check if this file is supported.
	const DetectInfo info = {
		{0, sizeof(d->xbeHeader), reinterpret_cast<const uint8_t*>(&d->xbeHeader)},
		nullptr,	// ext (not needed for Xbox_XBE)
		0		// szFile (not needed for Xbox_XBE)
	};
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		d->xbeHeader.magic = 0;
		d->file.reset();
		return;
	}

	// Load the certificate.
	const uint32_t base_address = le32_to_cpu(d->xbeHeader.base_address);
	const uint32_t cert_address = le32_to_cpu(d->xbeHeader.cert_address);
	if (cert_address > base_address) {
		size = d->file->seekAndRead(cert_address - base_address,
			&d->xbeCertificate, sizeof(d->xbeCertificate));
		if (size == sizeof(d->xbeCertificate)) {
			// Is PAL?
			d->isPAL = (le32_to_cpu(d->xbeCertificate.region_code) == XBE_REGION_CODE_RESTOFWORLD);
		} else {
			// Unable to load the certificate.
			// Continue anyway.
			d->xbeCertificate.size = 0;
		}
	}
}

/**
 * Close the opened file.
 */
void Xbox_XBE::close(void)
{
	RP_D(Xbox_XBE);

	// NOTE: Don't delete these. They have rp_image objects
	// that may be used by the UI later.
	if (d->pe_exe) {
		d->pe_exe->close();
	}

	if (d->xtImage.isInit) {
		if (std::holds_alternative<XboxXPRPtr>(d->xtImage.data)) {
			// XPR0 image
			std::get<XboxXPRPtr>(d->xtImage.data)->close();
		}
	}

	// Call the superclass function.
	super::close();
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int Xbox_XBE::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(XBE_Header))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// Check for XBE.
	const XBE_Header *const xbeHeader =
		reinterpret_cast<const XBE_Header*>(info->header.pData);
	if (xbeHeader->magic == cpu_to_be32(XBE_MAGIC)) {
		// We have an XBE file.
		return 0;
	}

	// Not supported.
	return -1;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *Xbox_XBE::systemName(unsigned int type) const
{
	RP_D(const Xbox_XBE);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// Xbox 360 has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"Xbox_XBE::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	static const array<const char*, 4> sysNames = {{
		"Microsoft Xbox", "Xbox", "Xbox", nullptr
	}};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t Xbox_XBE::supportedImageTypes(void) const
{
	RP_D(const Xbox_XBE);
	if (!d->xtImage.isInit) {
		const_cast<Xbox_XBE_Private*>(d)->initXPR0_xtImage();
	}

	return (d->xtImage.isInit ? IMGBF_INT_ICON : 0);
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> Xbox_XBE::supportedImageSizes(ImageType imageType) const
{
	ASSERT_supportedImageSizes(imageType);

	if (imageType != IMG_INT_ICON) {
		// Only icons are supported.
		return {};
	}

	RP_D(const Xbox_XBE);
	if (!d->xtImage.isInit) {
		// No images yet. Try loading it.
		const_cast<Xbox_XBE_Private*>(d)->initXPR0_xtImage();
	}
	if (!d->xtImage.isInit) {
		// Still no images.
		return {};
	}

	ImageSizeDef sz_INT_ICON;
	sz_INT_ICON.name = nullptr;
	sz_INT_ICON.index = 0;

	if (std::holds_alternative<XboxXPRPtr>(d->xtImage.data)) {
		// XPR0 image
		const XboxXPR *const xpr0 = std::get<XboxXPRPtr>(d->xtImage.data).get();
		sz_INT_ICON.width = xpr0->width();
		sz_INT_ICON.height = xpr0->height();
	} else {
		// PNG image
		const rp_image *const png = std::get<rp_image_ptr>(d->xtImage.data).get();
		sz_INT_ICON.width = png->width();
		sz_INT_ICON.height = png->height();
	}

	return {sz_INT_ICON};
}

/**
 * Get image processing flags.
 *
 * These specify post-processing operations for images,
 * e.g. applying transparency masks.
 *
 * @param imageType Image type.
 * @return Bitfield of ImageProcessingBF operations to perform.
 */
uint32_t Xbox_XBE::imgpf(ImageType imageType) const
{
	ASSERT_imgpf(imageType);

	if (imageType != IMG_INT_ICON) {
		// Only icons are supported.
		return 0;
	}

	RP_D(const Xbox_XBE);
	if (!d->xtImage.isInit) {
		const_cast<Xbox_XBE_Private*>(d)->initXPR0_xtImage();
	}

	uint32_t ret = 0;
	if (d->xtImage.isInit) {
		// If both dimensions of the texture are 64 or less,
		// specify nearest-neighbor scaling.
		if (std::holds_alternative<XboxXPRPtr>(d->xtImage.data)) {
			// XPR0 image
			const XboxXPR *const xpr0 = std::get<XboxXPRPtr>(d->xtImage.data).get();
			if (xpr0->width() <= 64 && xpr0->height() <= 64) {
				// 64x64 or smaller.
				ret = IMGPF_RESCALE_NEAREST;
			}
		} else {
			// PNG image
			const rp_image *const png = std::get<rp_image_ptr>(d->xtImage.data).get();
			if (png->width() <= 64 && png->height() <= 64) {
				// 64x64 or smaller.
				ret = IMGPF_RESCALE_NEAREST | IMGPF_INTERNAL_PNG_FORMAT;
			} else {
				// Larger than 64x64.
				// We need to set the internal PNG flag regardless.
				ret = IMGPF_INTERNAL_PNG_FORMAT;
			}
		}
	}
	return ret;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int Xbox_XBE::loadFieldData(void)
{
	RP_D(Xbox_XBE);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// XBE file isn't valid.
		return -EIO;
	}

	// Parse the XBE file.
	// NOTE: The magic number is NOT byteswapped in the constructor.
	const XBE_Header *const xbeHeader = &d->xbeHeader;
	const XBE_Certificate *const xbeCertificate = &d->xbeCertificate;
	if (xbeHeader->magic != cpu_to_be32(XBE_MAGIC)) {
		// Invalid magic number.
		return 0;
	}

	// Maximum of 13 fields.
	d->fields.reserve(13);
	d->fields.setTabName(0, "XBE");

	// Game name
	d->fields.addField_string(C_("RomData", "Title"),
		utf16le_to_utf8(xbeCertificate->title_name, ARRAY_SIZE_I(xbeCertificate->title_name)));

	// Original PE filename
	const uint32_t base_address = le32_to_cpu(d->xbeHeader.base_address);
	const uint32_t filenameW_address = le32_to_cpu(d->xbeHeader.debug_filenameW_address);
	const char *const s_filename_title = C_("Xbox_XBE", "PE Filename");
	if (filenameW_address > base_address) {
		char16_t pe_filename_W[260];
		size_t size = d->file->seekAndRead(filenameW_address - base_address,
			pe_filename_W, sizeof(pe_filename_W));
		if (size == sizeof(pe_filename_W)) {
			// Convert to UTF-8.
			pe_filename_W[ARRAY_SIZE(pe_filename_W)-1] = 0;
			const string pe_filename = utf16le_to_utf8(pe_filename_W, -1);
			if (!pe_filename.empty()) {
				d->fields.addField_string(s_filename_title, pe_filename);
			} else {
				d->fields.addField_string(s_filename_title, C_("RomData", "Unknown"));
			}
		}
	} else {
		d->fields.addField_string(s_filename_title, C_("RomData", "Unknown"));
	}

	// Title ID
	const char *const s_titleID_desc = C_("Xbox_XBE", "Title ID");
	const string s_titleID = d->getTitleID();
	if (!s_titleID.empty()) {
		d->fields.addField_string(s_titleID_desc,
			s_titleID, RomFields::STRF_MONOSPACE);
	} else {
		// Title ID is zero.
		d->fields.addField_string(s_titleID_desc,
			fmt::format(FSTR("{:0>8X}"), 0), RomFields::STRF_MONOSPACE);
	}

	// Publisher
	d->fields.addField_string(C_("RomData", "Publisher"), d->getPublisher());

	// Timestamp
	// TODO: time_t is signed, so values greater than 2^31-1 may be negative.
	const char *const s_timestamp_title = C_("Xbox_XBE", "Timestamp");
	const uint32_t timestamp = le32_to_cpu(xbeHeader->timestamp);
	if (timestamp != 0) {
		d->fields.addField_dateTime(s_timestamp_title,
			static_cast<time_t>(timestamp),
			RomFields::RFT_DATETIME_HAS_DATE |
			RomFields::RFT_DATETIME_HAS_TIME);
	} else {
		d->fields.addField_string(s_timestamp_title, C_("Xbox_XBE", "Not set"));
	}

	// Media types
	// NOTE: Using a string instead of a bitfield because very rarely
	// are all of these set, and in most cases, none are.
	// NOTE 2: Some strings are *not* localized.
	// TODO: RFT_LISTDATA?
	static constexpr char media_type_tbl[][12] = {
		// 0
		NOP_C_("Xbox_XBE", "Hard Disk"),
		"XGD1",
		"DVD/CD",
		"CD-ROM",
		// 4
		"DVD-ROM SL",
		"DVD-ROM DL",
		"DVD-RW SL",
		"DVD-RW DL",
		// 8
		NOP_C_("Xbox_XBE", "Dongle"),
		NOP_C_("Xbox_XBE", "Media Board"),
		// TODO: Non-secure HDD
	};

	ostringstream oss;
	unsigned int found = 0;
	uint32_t media_types = le32_to_cpu(xbeCertificate->allowed_media_types);
	for (unsigned int i = 0; i < ARRAY_SIZE(media_type_tbl); i++, media_types >>= 1) {
		if (!(media_types & 1)) {
			continue;
		}

		if (found > 0) {
			if (found % 4 == 0) {
				oss << ",\n";
			} else {
				oss << ", ";
			}
		}
		found++;

		if (i == 0 || i >= 8) {
			// Localized string
			oss << pgettext_expr("Xbox_XBE", media_type_tbl[i]);
		} else {
			// Non-localized string
			oss << media_type_tbl[i];
		}
	}

	d->fields.addField_string(C_("Xbox_XBE", "Media Types"),
		found ? oss.str() : C_("Xbox_XBE", "None"));

	// Initialization flags
	const uint32_t init_flags = le32_to_cpu(xbeHeader->init_flags);
	static const array<const char*, 4> init_flags_tbl = {{
		NOP_C_("Xbox_XBE|InitFlags", "Mount Utility Drive"),
		NOP_C_("Xbox_XBE|InitFlags", "Format Utility Drive"),
		NOP_C_("Xbox_XBE|InitFlags", "Limit RAM to 64 MB"),
		NOP_C_("Xbox_XBE|InitFlags", "Don't Setup HDD"),
	}};
	vector<string> *const v_init_flags = RomFields::strArrayToVector_i18n("Xbox_XBE|InitFlags", init_flags_tbl);
	d->fields.addField_bitfield(C_("Xbox_XBE", "Init Flags"),
		v_init_flags, 2, init_flags);

	// Region code
	const unsigned int region_code = d->getRegionCode();
	vector<string> *const v_region_code = RomFields::strArrayToVector_i18n(
		"Region", d->region_code_bitfield_names);
	d->fields.addField_bitfield(C_("RomData", "Region Code"),
		v_region_code, 3, region_code);

	// TODO: Age ratings, disc number

	// Can we get the EXE section?
	const EXE *const pe_exe = const_cast<Xbox_XBE_Private*>(d)->initEXE();
	if (pe_exe) {
		// Add the fields.
		const RomFields *const exeFields = pe_exe->fields();
		if (exeFields) {
			d->fields.addFields_romFields(exeFields, RomFields::TabOffset_AddTabs);
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
int Xbox_XBE::loadMetaData(void)
{
	RP_D(Xbox_XBE);
	if (!d->metaData.empty()) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// XBE file isn't valid.
		return -EIO;
	}

	const XBE_Certificate *const xbeCertificate = &d->xbeCertificate;
	d->metaData.reserve(4);	// Maximum of 4 metadata properties.

	// Title
	d->metaData.addMetaData_string(Property::Title,
		utf16le_to_utf8(xbeCertificate->title_name, ARRAY_SIZE_I(xbeCertificate->title_name)));

	// Publisher
	d->metaData.addMetaData_string(Property::Publisher, d->getPublisher());

	/** Custom properties! **/

	// Title ID (as Game ID)
	d->metaData.addMetaData_string(Property::GameID, d->getTitleID());

	// Region code
	// For multi-region titles, region will be formatted as: "UJEM"
	// NOTE: Using 'E' for "Rest of World" and 'M' for "Manufacturing".
	// TODO: Special handling for region-free?
	const unsigned int region_code = d->getRegionCode();

	const char *i18n_region = nullptr;
	for (size_t i = 0; i < d->region_code_bitfield_names.size(); i++) {
		if (region_code == (1U << i)) {
			i18n_region = d->region_code_bitfield_names[i];
			break;
		}
	}

	if (i18n_region) {
		d->metaData.addMetaData_string(Property::RegionCode,
			pgettext_expr("Region", i18n_region));
	} else {
		// Multi-region
		static const char all_display_regions[] = "UJEM";
		char s_region_code[] = "----";
		for (size_t i = 0; i < sizeof(s_region_code)-1; i++) {
			if (region_code & (1U << i)) {
				s_region_code[i] = all_display_regions[i];
			}
		}
		d->metaData.addMetaData_string(Property::RegionCode, s_region_code);
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
int Xbox_XBE::loadInternalImage(ImageType imageType, rp_image_const_ptr &pImage)
{
	ASSERT_loadInternalImage(imageType, pImage);

	RP_D(Xbox_XBE);
	if (imageType != IMG_INT_ICON) {
		// Only IMG_INT_ICON is supported by XBE.
		// TODO: -EIO for unsupported imageType?
		pImage.reset();
		return -ENOENT;
	} else if (d->xtImage.isInit) {
		// Image has already been loaded.
		// We'll get the actual image after this.
	} else if (!d->file) {
		// File isn't open.
		pImage.reset();
		return -EBADF;
	} else if (!d->isValid) {
		// Save file isn't valid.
		pImage.reset();
		return -EIO;
	}

	if (!d->xtImage.isInit) {
		d->initXPR0_xtImage();
	}

	if (std::holds_alternative<XboxXPRPtr>(d->xtImage.data)) {
		// XPR0 image
		pImage = std::get<XboxXPRPtr>(d->xtImage.data)->image();
	} else {
		// PNG image
		pImage = std::get<rp_image_ptr>(d->xtImage.data);
	}

	return (pImage) ? 0 : -EIO;
}

} // namespace LibRomData
