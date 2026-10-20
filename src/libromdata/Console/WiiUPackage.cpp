/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiUPackage.cpp: Wii U NUS Package reader.                              *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "WiiUPackage.hpp"
#include "WiiUPackage_p.hpp"
#include "data/WiiUData.hpp"
#include "GameCubeRegions.hpp"

// for extURLs_int()
#include "WiiU.hpp"

// TGA FileFormat
#include "librptexture/fileformat/TGA.hpp"

#ifdef HAVE_ZARCHIVE
// for .wua files
#  include "file/RpFile_ZArchiveReader.hpp"
#endif /* HAVE_ZARCHIVE */

// Other rom-properties libraries
#include "librpbase/disc/PartitionFile.hpp"
#include "librpbase/SystemRegion.hpp"
#include "librpfile/FileSystem.hpp"
#include "librpfile/RpFile.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;
using namespace LibRpTexture;

#ifdef _WIN32
#  include "librptext/wchar.hpp"
#else /* _WIN32 */
#  define U82T_c(str) (str)
#  define U82T_s(str) (str)
#  define T2U8(str) (str)
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
const array<const char*, WiiUPackagePrivate::ZARCHIVE_EXT_COUNT+0+1> WiiUPackagePrivate::exts = {{
	// NUS packages are directories.

#ifdef HAVE_ZARCHIVE
	// WUA archives are files.
	".wua",
#endif /* HAVE_ZARCHIVE */

	nullptr
}};
const array<const char*, WiiUPackagePrivate::ZARCHIVE_MIME_COUNT+1+1> WiiUPackagePrivate::mimeTypes = {{
	// NUS packages are directories.
	"inode/directory",

#ifdef HAVE_ZARCHIVE
	// WUA archives are also supported.
	"application/x-wii-u-wua",
#endif /* HAVE_ZARCHIVE */

	nullptr
}};
const RomDataInfo WiiUPackagePrivate::romDataInfo = {
	"WiiUPackage", exts.data(), mimeTypes.data()
};

WiiUPackagePrivate::WiiUPackagePrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
	, packageType(PackageType::Unknown)
{
#ifdef HAVE_ZARCHIVE
	this->data = wua_t();
#endif /* HAVE_ZARCHIVE */
}

WiiUPackagePrivate::WiiUPackagePrivate(const char *path)
	: super({}, &romDataInfo)
	, packageType(PackageType::Unknown)
{
	if (path && path[0] != '\0') {
#ifdef _WIN32
		// Windows: Storing the path as UTF-16 internally.
		this->data = nus_t(U82T_c(path));
#else /* !_WIN32 */
		this->data = nus_t(path);
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
		this->data = nus_t(path);
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
	this->data = std::monostate();

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
	IDiscReaderPtr discReader;

	assert(packageType == PackageType::NUS);
	assert(std::holds_alternative<nus_t>(this->data));
	assert(idx < contentsReaders.size());
	if (packageType != PackageType::NUS ||
	    !std::holds_alternative<nus_t>(this->data) ||
	    idx >= contentsReaders.size())
	{
		return discReader;
	}

	if (contentsReaders[idx]) {
		// Content is already open.
		// NOTE: Assigning to `discReader` for named-return-value optimization.
		discReader = contentsReaders[idx];
		return discReader;
	}

#ifdef ENABLE_DECRYPTION
	const nus_t &nus = std::get<nus_t>(this->data);

	// Attempt to open the content.
	const WUP_Content_Entry &entry = contentsTable[idx];
	tstring ts_path(nus.path);
	ts_path += DIR_SEP_CHR;
	const size_t orig_path_size = ts_path.size();

	// Try with lowercase hex first.
	const uint32_t content_id = be32_to_cpu(entry.content_id);
	ts_path += fmt::format(FSTR(_T("{:0>8x}.app")), content_id);

	IRpFilePtr subfile = std::make_shared<RpFile>(ts_path.c_str(), RpFile::FM_OPEN_READ);
	if (!subfile->isOpen()) {
		// Try with uppercase hex.
		ts_path.resize(orig_path_size);
		ts_path += fmt::format(FSTR(_T("{:0>8X}.app")), content_id);

		subfile = std::make_shared<RpFile>(ts_path.c_str(), RpFile::FM_OPEN_READ);
		if (!subfile->isOpen()) {
			// Unable to open the content file.
			// TODO: Error code?
			return discReader;
		}
	}

	// Create a disc reader.
	// TODO: Bitfield constants for 'type'?
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
		discReader.reset();
		return discReader;
	}

	// Disc reader is open.
	contentsReaders[idx] = discReader;
#endif

	// NOTE: Unencrypted NUS packages are NOT supported right now.
	return discReader;
}

/**
 * Create a host system filename for an Extracted packgae.
 * @param filename Filename within the package
 * @return Host system filename
 */
std::tstring WiiUPackagePrivate::host_system_filename(const char *filename) const
{
	tstring ts_full_filename;
	assert(packageType == PackageType::Extracted);
	assert(std::holds_alternative<nus_t>(this->data));
	if (packageType != PackageType::Extracted ||
	    !std::holds_alternative<nus_t>(this->data))
	{
		return ts_full_filename;
	}

	const nus_t &nus = std::get<nus_t>(this->data);

	// Extracted package format. Open the file directly.
	ts_full_filename.assign(nus.path);
	ts_full_filename += DIR_SEP_CHR;

	// Remove leading slashes, if present.
	while (*filename == _T('/')) {
		filename++;
	}
	if (*filename == '\0') {
		// Oops, no filename...
		ts_full_filename.clear();
		return ts_full_filename;
	}

#ifdef _WIN32
	const size_t old_sz = ts_full_filename.size();
#endif /* _WIN32 */
	ts_full_filename += U82T_c(filename);
#ifdef _WIN32
	// Replace all slashes with backslashes.
	const auto start_iter = ts_full_filename.begin() + old_sz;
	std::transform(start_iter, ts_full_filename.end(), start_iter, [](TCHAR c) noexcept -> bool {
		return (c == '/') ? DIR_SEP_CHR : c;
	});
#endif /* _WIN32 */

	return ts_full_filename;
}

/**
 * Open a file from the contents using the FST.
 * @param filename Filename
 * @return IRpFile, or nullptr on error.
 */
IRpFilePtr WiiUPackagePrivate::open(const char *filename)
{
	IRpFilePtr fp;
	if (!filename || filename[0] == '\0') {
		return fp;
	}

	switch (packageType) {
		default:
			assert(!"Unsupported Wii U package type.");
			return fp;

		case PackageType::NUS: {
			if (!fst) {
				// No FST.
				return fp;
			}

			// Get the FST entry.
			IFst::DirEnt dirent;
			int ret = fst->find_file(filename, &dirent);
			if (ret != 0) {
				// File not found?
				return fp;
			}

			// Make sure the required content file is open.
			IDiscReaderPtr contentFile = openContentFile(dirent.ptnum);
			if (!contentFile) {
				// Unable to open this content file.
				return fp;
			}

			// Create a PartitionFile.
			// NOTE: Assigning to `fp` for named-return-value optimization.
			fp = std::make_shared<PartitionFile>(contentFile, dirent.offset, dirent.size);
			return fp;
		}

		case PackageType::Extracted: {
			// Extracted package format. Open the file directly.
			tstring ts_full_filename = host_system_filename(filename);
			if (ts_full_filename.empty()) {
				return fp;
			}

			// NOTE: Assigning to `fp` for named-return-value optimization.
			fp = std::make_shared<RpFile>(ts_full_filename.c_str(), RpFile::FM_OPEN_READ);
			return fp;
		}

#ifdef HAVE_ZARCHIVE
		case PackageType::WUA: {
			// .wua archive. Open the file using ZArchiveReader.
			assert(std::holds_alternative<wua_t>(this->data));
			if (!std::holds_alternative<wua_t>(this->data)) {
				return fp;
			}
			const wua_t &wua = std::get<wua_t>(this->data);
			assert(wua.zarchive != nullptr);
			if (!wua.zarchive) {
				return fp;
			}

			fp = std::make_shared<RpFile_ZArchiveReader>(wua.zarchive, wua.package_name.c_str(), filename);
			return fp;
		}
#endif /* HAVE_ZARCHIVE */
	}
}

/**
 * Does the specified file exist in the package?
 * @param filename Filename
 * @return True if the file exists; false if not.
 */
bool WiiUPackagePrivate::file_exists(const char *filename) const
{
	if (!filename || filename[0] == '\0') {
		return false;
	}

	switch (packageType) {
		default:
			assert(!"Unsupported Wii U package type.");
			return false;

		case PackageType::NUS: {
			if (!fst) {
				// No FST.
				return false;
			}

			// Get the FST entry.
			IFst::DirEnt dirent;
			return (fst->find_file(filename, &dirent) == 0);
		}

		case PackageType::Extracted: {
			// Extracted package format. Open the file directly.
			tstring ts_full_filename = host_system_filename(filename);
			if (ts_full_filename.empty()) {
				return false;
			}

			// Does the file exist?
			return (FileSystem::access(ts_full_filename.c_str(), R_OK) == 0);
		}

#ifdef HAVE_ZARCHIVE
		case PackageType::WUA: {
			// .wua archive. Check if we can get a valid handle from ZArchive.
			assert(std::holds_alternative<wua_t>(this->data));
			if (!std::holds_alternative<wua_t>(this->data)) {
				return false;
			}
			const wua_t &wua = std::get<wua_t>(this->data);
			assert(wua.zarchive != nullptr);
			if (!wua.zarchive) {
				return false;
			}

			// Construct the full filename.
			string full_filename = "/";
			full_filename += wua.package_name;
			if (filename[0] != '/') {
				full_filename += '/';
			}
			full_filename += filename;

			ZArchiveNodeHandle handle = wua.zarchive->LookUp(full_filename, true, false);
			return (handle != ZARCHIVE_INVALID_NODE && wua.zarchive->IsFile(handle));
		}
#endif /* HAVE_ZARCHIVE */
	}
}

/**
 * Load the icon.
 * @return Icon, or nullptr on error.
 */
rp_image_const_ptr WiiUPackagePrivate::loadIcon(void)
{
	rp_image_const_ptr icon;

	if (img_icon) {
		// Icon has already been loaded.
		// NOTE: Assigning to `icon` for named-return-value optimization.
		icon = img_icon;
		return icon;
	} else if (!this->isValid) {
		// Can't load the icon.
		return icon;
	}

	// Verify that this is a Wii U package. (TMD format must be v1 or higher.)
	if (tmd && tmd->tmdFormatVersion() < 1) {
		// Not a Wii U package.
		// TODO: loadInternalImage() should return ENOENT.
		return icon;
	}

	// Icon is "/meta/iconTex.tga".
	IRpFilePtr f_icon = this->open("/meta/iconTex.tga");
	if (!f_icon) {
		// Icon not found?
		return icon;
	}

	// Attempt to open the icon as TGA.
	TGA tga(f_icon);
	if (!tga.isValid()) {
		// Not a valid TGA file.
		return icon;
	}

	// Get the icon.
	icon = tga.image();
	this->img_icon = icon;
	return icon;
}

/** WiiUPackage **/

/**
 * Read a WUA (Wii U Archive) file.
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
	: super(new WiiUPackagePrivate(file))
{
#ifdef HAVE_ZARCHIVE
	init();
#else /* !HAVE_ZARCHIVE */
	// Can't open .wua on this system...
#endif /* HAVE_ZARCHIVE */
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
	d->fileType = FileType::Unknown;

	using nus_t = WiiUPackagePrivate::nus_t;
	using wua_t = WiiUPackagePrivate::wua_t;

	if (std::holds_alternative<nus_t>(d->data)) {
		const nus_t &nus = std::get<nus_t>(d->data);
		if (nus.path.empty()) {
			// No path...
			d->reset();
			return;
		}
	} else if (!d->file) {
		// Not NUS, and no file specified...
		d->reset();
		return;
	}

	tstring ts_path;

	// Check if this path is supported.
	if (d->file) {
#ifdef HAVE_ZARCHIVE
		// Attempt to open as .wua.
		if (!std::holds_alternative<wua_t>(d->data)) {
			d->reset();
			return;
		}
		wua_t &wua = std::get<wua_t>(d->data);

		// Create an std::streambuf wrapper around the IRpFile.
		// The streambuf will be used for an std::istream.
		wua.streambuf = new streambuf_IRpFile(d->file);
		wua.zarchive.reset(ZArchiveReader::OpenFromStream(unique_ptr<std::istream>(new std::istream(wua.streambuf))));
		if (!wua.zarchive) {
			d->reset();
			return;
		}

		// Get the first package name that matches 00050000*. (game package)
		// TODO: Allow the user to select other packages?
		ZArchiveNodeHandle handle_root = wua.zarchive->LookUp("/", false, true);
		if (handle_root == ZARCHIVE_INVALID_NODE || !wua.zarchive->IsDirectory(handle_root)) {
			// Unable to get a handle to the root directory...
			d->reset();
			return;
		}

		// Read the root directory.
		uint32_t root_count = wua.zarchive->GetDirEntryCount(handle_root);
		assert(root_count < 65536);
		if (root_count >= 65536) {
			// Too many entries...
			return;
		}

		ZArchiveReader::DirEntry dirEntry;
		for (uint32_t i = 0; i < root_count; i++) {
			bool bRet = wua.zarchive->GetDirEntry(handle_root, i, dirEntry);
			if (!bRet) {
				continue;
			}
			if (!dirEntry.isDirectory || dirEntry.name.size() < 8) {
				continue;
			}

			// Check if it's a game entry.
			if (strncmp(dirEntry.name.data(), "00050000", 8) == 0) {
				// It's a game entry. We'll use it.
				wua.package_name = "/" + string(dirEntry.name);
				break;
			}
		}
		if (wua.package_name.empty()) {
			// No game packages...
			// TODO: Use the first package name?
			d->reset();
			return;
		}

		d->packageType = WiiUPackagePrivate::PackageType::WUA;
		ts_path = _T("/");
		ts_path += U82T_s(wua.package_name);
		ts_path += _T('/');
#else /* !HAVE_ZARCHIVE */
		// Can't open .wua on this system...
		d->reset();
		return;
#endif /* HAVE_ZARCHIVE */
	} else {
		if (!std::holds_alternative<nus_t>(d->data)) {
			d->reset();
			return;
		}
		const nus_t &nus = std::get<nus_t>(d->data);
		d->packageType = static_cast<WiiUPackagePrivate::PackageType>(isDirSupported_static(nus.path));
		if ((int)d->packageType < 0) {
			d->reset();
			return;
		}
		ts_path = nus.path;
		ts_path += DIR_SEP_CHR;
	}

	d->isValid = ((int)d->packageType >= 0);
	if (!d->isValid) {
		d->reset();
		return;
	}

	// Open the ticket.
	// NOTE: May not be present in extracted packages.
	unique_ptr<WiiTicket> ticket;
	int ticketFormatVersion = -1;

	// TODO: Skip this for .wua?
	// TODO: Use '/' instead of DIR_SEP_CHR for .wua?
	const size_t orig_path_size = ts_path.size();

	if (d->packageType != WiiUPackagePrivate::PackageType::NUS) {
		ts_path += _T("code");
		ts_path += DIR_SEP_CHR;
	}
	ts_path += _T("title.tik");

	IRpFilePtr subfile;
#ifdef HAVE_ZARCHIVE
	if (d->packageType == WiiUPackagePrivate::PackageType::WUA) {
		const wua_t &wua = std::get<wua_t>(d->data);
		subfile = std::make_shared<RpFile_ZArchiveReader>(wua.zarchive, wua.package_name.c_str(), T2U8(ts_path).c_str());
	} else
#endif /* HAVE_ZARCHIVE */
	{
		subfile = std::make_shared<RpFile>(ts_path, RpFile::FM_OPEN_READ);
	}

	if (subfile->isOpen()) {
		ticket.reset(new WiiTicket(subfile));
		if (ticket->isValid()) {
			// Check the ticket version.
			// Wii U titles are generally v1.
			// vWii titles are v0.
			ticketFormatVersion = ticket->ticketFormatVersion();
			if (ticketFormatVersion != 0 && ticketFormatVersion != 1) {
				// Not v0 or v1.
				ticket.reset();
			}
		} else {
			ticket.reset();
		}
	}
	subfile.reset();
	if (!ticket && d->packageType == WiiUPackagePrivate::PackageType::NUS) {
		// Unable to load the ticket.
		d->reset();
		d->isValid = false;
		return;
	}
	d->ticket = std::move(ticket);

	// Open the TMD.
	// NOTE: May not be present in extracted packages.
	unique_ptr<WiiTMD> tmd;
	int tmdFormatVersion = -1;

	ts_path.resize(orig_path_size);
	if (d->packageType != WiiUPackagePrivate::PackageType::NUS) {
		ts_path += _T("code");
		ts_path += DIR_SEP_CHR;
	}
	ts_path += _T("title.tmd");

#ifdef HAVE_ZARCHIVE
	if (d->packageType == WiiUPackagePrivate::PackageType::WUA) {
		const wua_t &wua = std::get<wua_t>(d->data);
		subfile = std::make_shared<RpFile_ZArchiveReader>(wua.zarchive, wua.package_name.c_str(), T2U8(ts_path).c_str());
	} else
#endif /* HAVE_ZARCHIVE */
	{
		subfile = std::make_shared<RpFile>(ts_path, RpFile::FM_OPEN_READ);
	}
	if (subfile->isOpen()) {
		tmd.reset(new WiiTMD(subfile));
		if (tmd->isValid()) {
			// Check the TMD version.
			// Wii U titles are generally v1.
			// vWii titles are v0.
			tmdFormatVersion = tmd->tmdFormatVersion();
			if (tmdFormatVersion != 0 && tmdFormatVersion != 1) {
				// Not v0 or v1.
				tmd.reset();
			}
		} else {
			tmd.reset();
		}
	}
	subfile.reset();
	if (!tmd && d->packageType == WiiUPackagePrivate::PackageType::NUS) {
		// Unable to load the TMD.
		d->reset();
		d->isValid = false;
		return;
	}
	d->tmd = std::move(tmd);

	if (d->file) {
		// Using an archive file; probably ZArchive.
		d->mimeType = "application/x-wii-u-wua";
		d->fileType = FileType::ArchiveFile;
	} else {
		// Using a directory.
		d->mimeType = "inode/directory";
		d->fileType = FileType::ApplicationPackage;
	}

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
	int ret = d->ticket->decryptTitleKey(d->title_key, sizeof(d->title_key));
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
	d->contentsTable = d->tmd->contentsTableV1(0);
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
	unique_ptr<WiiUFst> fst(new WiiUFst(fst_buf.get(), static_cast<uint32_t>(fst_size)));
	if (!fst->isOpen()) {
		// FST is invalid?
		// NOTE: boot1 does not have an FST.
		return;
	}
	d->fst = std::move(fst);

	// FST loaded.
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int WiiUPackage::isRomSupported_static(const DetectInfo *info)
{
#ifdef HAVE_ZARCHIVE
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(uint32_t) ||
	    !info->ext || info->ext[0] == '\0')
	{
		// Either no detection information was specified,
		// or the header is too small.
		return static_cast<int>(WiiUPackagePrivate::PackageType::Unknown);
	}

	// Check for a .wua file.
	// File extension must be .wua, and it must start with the zstd magic number.
	if (strcasecmp(info->ext, ".wua") != 0) {
		// Not a .wua file.
		return static_cast<int>(WiiUPackagePrivate::PackageType::Unknown);
	}

	const uint32_t *const pData32 = reinterpret_cast<const uint32_t*>(info->header.pData);
	if (pData32[0] != cpu_to_be32(0x28B52FFD)) {
		// Not the ZSTD magic number.
		return static_cast<int>(WiiUPackagePrivate::PackageType::Unknown);
	}

	// May be a .wua file.
	return static_cast<int>(WiiUPackagePrivate::PackageType::WUA);
#else /* !HAVE_ZARCHIVE */
	// Can't open .wua on this system...
	RP_UNUSED(info);
	return static_cast<int>(WiiUPackagePrivate::PackageType::Unknown);
#endif /* HAVE_ZARCHIVE */
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
	if (!d->isValid || !isSystemNameTypeValid(type)) {
		return nullptr;
	}

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

	// NOTE: Extracted packages might not have a TMD.
	if (d->tmd && d->tmd->tmdFormatVersion() >= 1) {
		// Wii U packages have an icon.
		ret = IMGBF_INT_ICON;
	} else if (d->file_exists("/meta/iconTex.tga")) {
		// iconTex.tga exists.
		ret = IMGBF_INT_ICON;
	}

#ifdef ENABLE_XML

#ifdef HAVE_JPEG
	ret |= IMGBF_EXT_MEDIA |
	       IMGBF_EXT_COVER | IMGBF_EXT_COVER_3D |
	       IMGBF_EXT_COVER_FULL;
#else /* !HAVE_JPEG */
	ret |= IMGBF_EXT_MEDIA | IMGBF_EXT_COVER_3D;
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
			return {{nullptr, 128, 128, 0}};
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
	} else if (std::holds_alternative<std::monostate>(d->data)) {
		// No directory or file...
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown ROM image type.
		return -EIO;
	}

	d->fields.reserve(10);	// Maximum of 10 fields.

	// TODO: Show a decryption key warning and/or "no XMLs".
	d->fields.setTabName(0, "Wii U");

	bool canLoadXMLs = false;
	switch (d->packageType) {
		default:
			assert(!"Unsupported Wii U package type.");
			return d->fields.count();

		case WiiUPackagePrivate::PackageType::NUS: {
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
			break;
		}

		case WiiUPackagePrivate::PackageType::Extracted:
#ifdef HAVE_ZARCHIVE
		case WiiUPackagePrivate::PackageType::WUA:
#endif /* HAVE_ZARCHIVE */
			// XML can always be loaded in extracted packages and WUA archives.
			canLoadXMLs = true;
			break;
	}

#ifdef ENABLE_XML
	// Parse the Wii U System XMLs.
	// NOTE: Only if the FST was loaded, or reading an extracted package or WUA archive.
	if (canLoadXMLs) {
		const bool hasTicket = (bool)d->ticket;
		int ret = d->addFields_System_XMLs(!hasTicket);
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
	return d->fields.count();
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
	} else if (std::holds_alternative<std::monostate>(d->data)) {
		// No directory or file...
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown ROM image type.
		return -EIO;
	}

	d->metaData.reserve(6);	// Maximum of 6 metadata properties.

	// NOTE: Adding custom properties from the ticket first, since it
	// sets the Title to the Title ID. This will be overwritten with
	// the actual Title if decryption is available and the XMLs are usable.
	if (d->ticket) {
		d->metaData.addMetaData_metaData(d->ticket->metaData());
	}

#ifdef ENABLE_XML
	// Check if the decryption keys were loaded.
	// NOTE: If not in Extracted or .wua format, requires a valid ticket.
	if (d->packageType != WiiUPackagePrivate::PackageType::NUS) {
		// Extracted or .wua format. We can always add the XMLs.
		// Parse the Wii U System XMLs.
		const bool hasTicket = (bool)d->ticket;
		d->addMetaData_System_XMLs(!hasTicket);
	} else {
		// For NUS format, we need a valid ticket in order to decrypt the contents.
		const KeyManager::VerifyResult verifyResult = d->ticket->verifyResult();
		if (verifyResult == KeyManager::VerifyResult::OK) {
			// Decryption keys were loaded. We can add XML fields.
			// Parse the Wii U System XMLs.
			d->addMetaData_System_XMLs();
		}
	}
#endif /* ENABLE_XML */

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
int WiiUPackage::loadInternalImage(ImageType imageType, rp_image_const_ptr &pImage)
{
	ASSERT_loadInternalImage(imageType, pImage);
	RP_D(WiiUPackage);
	ROMDATA_loadInternalImage_single(
		IMG_INT_ICON,	// ourImageType
		!std::holds_alternative<std::monostate>(d->data),	// file (NOTE: Using d->data because we might not have a "file")
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
