/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * J2ME.hpp: Java 2 Micro Edition package reader.                          *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "J2ME.hpp"

// MiniZip
#include <zlib.h>
#include "mz_zip.h"
#include "mz_compat.h"

// Other rom-properties libraries
#include "librpbase/img/RpPng.hpp"
#include "librpfile/MemFile.hpp"
using namespace LibRpBase;
using namespace LibRpText;
using namespace LibRpFile;
using namespace LibRpTexture;

// C++ STL classes
using std::array;
using std::map;
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
// unzClose() can safely take nullptr; it won't do anything.
DELAYLOAD_TEST_FUNCTION_IMPL1(unzClose, nullptr);
#  endif /* MINIZIP_IS_DLL */
#endif /* _MSC_VER */

class J2MEPrivate final : public RomDataPrivate
{
public:
	explicit J2MEPrivate(const IRpFilePtr &file);
	~J2MEPrivate() override;

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(J2MEPrivate)

public:
	/** RomDataInfo **/
	static const array<const char*, 2+1> exts;
	static const array<const char*, 2+1> mimeTypes;
	static const RomDataInfo romDataInfo;

public:
	enum class JFileType {
		Unknown	= -1,

		JAR	= 0,	// .jar package
		JAD	= 1,	// .jad file

		Max
	};
	JFileType jfileType;

	// Opened .jar file
	unzFile jarFile;

	// Icon
	rp_image_ptr img_icon;

public:
	enum class manifest_tag_t : uint8_t {
		// MANIFEST.MF and .jad
		Unknown = 0,

		ManifestVersion,
		CreatedBy,
		MicroEdition_Configuration,
		MicroEdition_Profile,
		MIDlet_Name,
		MIDlet_Description,
		MIDlet_Version,
		MIDlet_Vendor,
		MIDlet_Icon,
		MIDlet_Data_Size,
		MIDlet_1,

		// .jad only
		MIDlet_Jar_URL,
		MIDlet_Jar_Size,
		Nokia_MIDlet_Category,
		TC_BookReader_Logging,

		// .jad: File digest tags
		Name,
		MD5_Digest,
		SHA_Digest,	// deprecated alias of SHA1_Digest?
		SHA1_Digest,
		SHA_1_Digest,	// incorrect version found in some .jad files
		SHA_256_Digest,	// probably not found in J2ME .jar files
		Digest_Algorithms,

		Manifest_Tag_Max
	};
	static const array<const char*, static_cast<size_t>(manifest_tag_t::Manifest_Tag_Max)> manifest_tag_names;

	// Map of MANIFEST.MF
	// - Key: manifest_tag_t
	// - Value: std::string
	typedef map<manifest_tag_t, string> map_t;
	map_t m_map;

	// Maximum size for various files.
	static constexpr size_t MANIFEST_MF_FILE_SIZE_MAX = 32768U;
	static constexpr size_t ICON_PNG_FILE_SIZE_MAX = 16384U;

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
	rp::uvector<uint8_t> loadFileFromZip(const char *filename, size_t max_size = ~0ULL);

	/**
	 * Load MANIFEST.MF from this->jarFile.
	 * this->jarFile must have already been opened.
	 * On success, the MANIFEST.MF tags will be loaded into this->m_map.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int loadManifestMF(void);

	/**
	 * Get the icon filename from the "MIDlet-1" tag.
	 * @return Icon filename, or empty string if the tag was not found or malformed.
	 */
	string getIconFilenameFromMIDlet1(void);

	/**
	 * Load the icon.
	 * @return Icon, or nullptr on error.
	 */
	rp_image_const_ptr loadIcon(void);
};

ROMDATA_IMPL(J2ME)

/** J2MEPrivate **/

/* RomDataInfo */
const array<const char*, 2+1> J2MEPrivate::exts = {{
	".jar",
	".jad",

	nullptr
}};
const array<const char*, 2+1> J2MEPrivate::mimeTypes = {{
	// Official MIME types from FreeDesktop.org.
	"application/java-archive",		// .jar

	// Vendor-specific MIME types from FreeDesktop.org.
	"text/vnd.sun.j2me.app-descriptor",	// .jad

	nullptr
}};
const RomDataInfo J2MEPrivate::romDataInfo = {
	"J2ME", exts.data(), mimeTypes.data()
};

// Manifest tag names
const array<const char*, static_cast<size_t>(J2MEPrivate::manifest_tag_t::Manifest_Tag_Max)> J2MEPrivate::manifest_tag_names = {{
	nullptr,

	"Manifest-Version",
	"Created-By",
	"MicroEdition-Configuration",
	"MicroEdition-Profile",
	"MIDlet-Name",
	"MIDlet-Description",
	"MIDlet-Version",
	"MIDlet-Vendor",
	"MIDlet-Icon",
	"MIDlet-Data-Size",
	"MIDlet-1",

	// .jad only
	"MIDlet-Jar-URL",
	"MIDlet-Jar-Size",
	"Nokia-MIDlet-Category",
	"TC-BookReader-Logging",

	// .jad: File digest tags
	"Name",
	"MD5-Digest",
	"SHA-Digest",		// deprecated alias of SHA1_Digest?
	"SHA1-Digest",
	"SHA-1-Digest",		// incorrect version found in some .jad files
	"SHA-256-Digest",	// probably not found in J2ME .jar files
	"Digest-Algorithms",
}};

J2MEPrivate::J2MEPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
	, jfileType(JFileType::Unknown)
	, jarFile(nullptr)
{}

J2MEPrivate::~J2MEPrivate()
{
	if (jarFile) {
		unzClose(jarFile);
	}
}

/**
 * Open a Zip file for reading.
 * @param filename Zip filename.
 * @return Zip file, or nullptr on error.
 */
unzFile J2MEPrivate::openZip(const char *filename)
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
rp::uvector<uint8_t> J2MEPrivate::loadFileFromZip(const char *filename, size_t max_size)
{
	// TODO: This is also used by GcnFstTest. Move to a common utility file?
	int ret = unzLocateFile(jarFile, filename, nullptr);
	if (ret != UNZ_OK) {
		return {};
	}

	// Get file information.
	unz_file_info64 file_info;
	ret = unzGetCurrentFileInfo64(jarFile,
		&file_info, nullptr, 0, nullptr, 0, nullptr, 0);
	if (ret != UNZ_OK || file_info.uncompressed_size >= max_size) {
		// Error getting file information, or the uncompressed size is too big.
		return {};
	}

	ret = unzOpenCurrentFile(jarFile);
	if (ret != UNZ_OK) {
		return {};
	}

	rp::uvector<uint8_t> buf;
	buf.resize(file_info.uncompressed_size);

	// Read the file.
	// NOTE: zlib and minizip are only guaranteed to be able to
	// read UINT16_MAX (64 KB) at a time, and the updated MiniZip
	// from https://github.com/nmoinvaz/minizip enforces this.
	uint8_t *p = buf.data();
	size_t size = file_info.uncompressed_size;
	while (size > 0) {
		int to_read = static_cast<int>(size > UINT16_MAX ? UINT16_MAX : size);
		ret = unzReadCurrentFile(jarFile, p, to_read);
		if (ret != to_read) {
			return {};
		}

		// ret == number of bytes read.
		p += ret;
		size -= ret;
	}

	// Close the file.
	// An error will occur here if the CRC is incorrect.
	ret = unzCloseCurrentFile(jarFile);
	if (ret != UNZ_OK) {
		return {};
	}

	return buf;
}

/**
 * Load MANIFEST.MF from this->jarFile.
 * this->jarFile must have already been opened.
 * On success, the MANIFEST.MF tags will be loaded into this->m_map.
 * @return 0 on success; negative POSIX error code on error.
 */
int J2MEPrivate::loadManifestMF(void)
{
	rp::uvector<uint8_t> manifest_buf;

	switch (jfileType) {
		default:
			assert(!"Unsupported J2ME file type.");
			return -ENOTSUP;

		case JFileType::JAR:
			// The .jar file must have been opened already.
			assert(jarFile != nullptr);
			if (!jarFile) {
				return -EIO;
			}

			// Load MANIFEST.MF.
			manifest_buf = loadFileFromZip("META-INF/MANIFEST.MF", MANIFEST_MF_FILE_SIZE_MAX);
			if (manifest_buf.empty()) {
				// Unable to load MANIFEST.MF.
				return -ENOENT;
			}
			break;

		case JFileType::JAD: {
			// Sanity check: Verify the JAD file size.
			const off64_t filesize = file->size();
			if (filesize <= 0 || filesize > static_cast<off64_t>(J2MEPrivate::MANIFEST_MF_FILE_SIZE_MAX)) {
				// File is too big. (...or too small?)
				return -ENOMEM;
			}

			manifest_buf.resize(static_cast<size_t>(filesize));
			file->rewind();
			size_t size = file->read(manifest_buf.data(), manifest_buf.size());
			if (size != manifest_buf.size()) {
				// Seek and/or read error.
				return -EIO;
			}
			break;
		}
	}

	// Add a NULL byte at the end.
	manifest_buf.push_back(0);

	// Parse the MANIFEST.MF tags.
	// NOTE: May have LF or CRLF line endings.
	m_map.clear();
	char *p = reinterpret_cast<char*>(manifest_buf.data());

	// Check for a UTF-8 byte-order marker.
	static const array<uint8_t, 3> utf8_bom = {{0xEF, 0xBB, 0xBF}};
	if (manifest_buf.size() >= utf8_bom.size() && !memcmp(p, utf8_bom.data(), utf8_bom.size())) {
		// Found the UTF-8 BOM. Skip it.
		// NOTE: We're always interpreting the file contents as UTF-8.
		p += utf8_bom.size();
	}

	char *line_saveptr = nullptr;
	auto last_iter = m_map.end();
	for (char *line = strtok_r(p, "\n", &line_saveptr); line != nullptr;
	     line = strtok_r(nullptr, "\n", &line_saveptr))
	{
		// Check for a multi-line entry.
		if (line[0] == ' ' && line[1] != '\0') {
			// Found a continuation of the previous entry.
			if (last_iter == m_map.end()) {
				// Not a valid iterator...
				continue;
			}

			string &s_value = last_iter->second;
			s_value += &line[1];

			// Remove any trailing CRs and spaces.
			while (!s_value.empty()) {
				const size_t size_minus_one = s_value.size() - 1;
				if (s_value[size_minus_one] != '\r' && s_value[size_minus_one] != ' ') {
					break;
				}
				s_value.resize(size_minus_one);
			}

			// Next line.
			continue;
		}

		char *token_saveptr = nullptr;
		const char *tag_name = strtok_r(line, ":", &token_saveptr);
		if (!tag_name) {
			// No ':'. This line is invalid.
			continue;
		}
		// Check for empty lines.
		if (tag_name[0] == '\0' || tag_name[0] == '\r' || tag_name[0] == '\n') {
			// Found an empty line.
			continue;
		}

		// Determine if this tag is known.
		manifest_tag_t tag = manifest_tag_t::Unknown;
		for (uint8_t i = 1; i < static_cast<uint8_t>(manifest_tag_t::Manifest_Tag_Max); i++) {
			if (!strcmp(manifest_tag_names[i], tag_name)) {
				// Found a match.
				tag = static_cast<manifest_tag_t>(i);
				break;
			}
		}

		switch (tag) {
			default:
				break;

			case manifest_tag_t::Unknown:
				// Unrecognized tag.
				continue;

			case manifest_tag_t::Name:
			case manifest_tag_t::MD5_Digest:
			case manifest_tag_t::SHA_Digest:
			case manifest_tag_t::SHA1_Digest:
			case manifest_tag_t::SHA_256_Digest:
				// Ignoring .jad file digest tags.
				continue;
		}

		const char *value = strtok_r(nullptr, ":", &token_saveptr);
		if (!value) {
			// No value...
			continue;
		}

		// There will most likely be a space between the ':' and the actual value.
		while (*value == ' ') {
			value++;
		}

		if (value[0] == '\r' || value[0] == '\n') {
			// End of line. No value.
			continue;
		}

		// Convert to an std::string so we can emplace it in the map next.
		string s_value(value);

		// Remove any trailing CRs and spaces.
		while (!s_value.empty()) {
			const size_t size_minus_one = s_value.size() - 1;
			if (s_value[size_minus_one] != '\r' && s_value[size_minus_one] != ' ') {
				break;
			}
			s_value.resize(size_minus_one);
		}

		auto status = m_map.emplace(tag, std::move(s_value));
		// FIXME: Some .jar files have duplicate tags in MANIFEST.MF:
		// - Bejeweled.jar
		// - Bejeweled__v600_.jar
		// - Gamester.Smb.In.Demand.v1.00.S30.Java.Retail-BiNPDA.jar
		// - Midtown Madness 3.jar
		// - Space Warrior.jar
		/*if (!status.second) {
			// Failed to emplace the value. (Duplicate tag?)
			m_map.clear();
			return -EIO;
		}*/

		// Needed for multi-line entries.
		last_iter = status.first;
	}

	return (unlikely(m_map.empty()) ? -ENOENT : 0);
}

/**
 * Get the icon filename from the "MIDlet-1" tag.
 * @return Icon filename, or empty string if the tag was not found or malformed.
 */
string J2MEPrivate::getIconFilenameFromMIDlet1(void)
{
	auto iter = m_map.find(manifest_tag_t::MIDlet_1);
	if (iter == m_map.end()) {
		// "MIDlet-1 was not found.
		return {};
	}

	// "MIDlet-1" has three values, separated by commas:
	// - Title
	// - Icon filename
	// - Java package name (maybe?)
	const string &midlet_1 = iter->second;
	size_t comma1 = midlet_1.find(',');
	if (comma1 == string::npos) {
		return {};
	}
	// Skip spaces past the comma, and also leading slsahes.
	for (comma1++; comma1 < midlet_1.size(); comma1++) {
		const char chr = midlet_1[comma1];
		if (chr != ' ' && chr != '/') {
			break;
		}
	}
	if (comma1 >= midlet_1.size()) {
		// Too far.
		return {};
	}

	size_t comma2 = midlet_1.find(',', comma1 + 1);
	if (comma2 == string::npos) {
		return {};
	}

	string s_ret(midlet_1, comma1, comma2 - comma1);

	// Remove trailing spaces.
	while (!s_ret.empty()) {
		const size_t size_minus_one = s_ret.size() - 1;
		if (s_ret[size_minus_one] != ' ') {
			break;
		}
		s_ret.resize(size_minus_one);
	}

	return s_ret;
}

/**
 * Load the icon.
 * @return Icon, or nullptr on error.
 */
rp_image_const_ptr J2MEPrivate::loadIcon(void)
{
	if (img_icon) {
		// Icon has already been loaded.
		return img_icon;
	} else if (!this->isValid || static_cast<int>(this->jfileType) < 0) {
		// Can't load the icon.
		return {};
	}

	// Make sure the .jar file is open.
	if (!jarFile) {
		// Not open...
		return {};
	}

	// PNG data buffer
	rp::uvector<uint8_t> png_buf;

	// Get the icon filename.
	// First, try "MIDlet-Icon".
	auto iter = m_map.find(manifest_tag_t::MIDlet_Icon);
	if (iter != m_map.end()) {
		// NOTE: The icon filename might have a leading slash.
		const char *icon_filename = iter->second.c_str();

		// Remove leading slashes.
		while (*icon_filename == '/') {
			icon_filename++;
		}

		if (*icon_filename == '\0') {
			// No filename.
			return {};
		}

		// Attempt to load the file.
		png_buf = loadFileFromZip(icon_filename, ICON_PNG_FILE_SIZE_MAX);
	}

	if (png_buf.empty()) {
		// "MIDlet-Icon" was not found. Try "MIDlet-1".
		string s_icon_filename = getIconFilenameFromMIDlet1();
		if (s_icon_filename.empty()) {
			// No filename.
			return {};
		}

		// Attempt to load the file.
		png_buf = loadFileFromZip(s_icon_filename.c_str(), ICON_PNG_FILE_SIZE_MAX);
	}

	if (png_buf.empty()) {
		// Unable to load the icon file.
		return {};
	}

	// Create a MemFile and decode the image.
	// TODO: For rpcli, shortcut to extract the PNG directly.
	MemFile *const f_mem = new MemFile(png_buf.data(), png_buf.size());
	this->img_icon = RpPng::load(f_mem);
	delete f_mem;
	return this->img_icon;
}

/** J2ME **/

/**
 * Read a J2ME .jar or .jad file.
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
J2ME::J2ME(const IRpFilePtr &file)
	: super(new J2MEPrivate(file))
{
	RP_D(J2ME);

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

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
		0				// szFile (not needed for J2ME)
	};
	d->jfileType = static_cast<J2MEPrivate::JFileType>(isRomSupported_static(&info));
	d->isValid = (static_cast<int>(d->jfileType) >= 0);

	if (!d->isValid) {
		d->file.reset();
		return;
	}

	switch (d->jfileType) {
		default:
			// Not supported.
			assert(!"Unsupported J2ME file type.");
			d->isValid = false;
			d->file.reset();
			return;

		case J2MEPrivate::JFileType::JAR:
#ifdef _MSC_VER
			// Delay load verification.
			// TODO: Only if linked with /DELAYLOAD?

#  ifdef ZLIB_IS_DLL
			// Only if zlib is a DLL.
			if (DelayLoad_test_get_crc_table() != 0) {
				// Delay load failed.
				// J2ME packages cannot be read without MiniZip.
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
				// J2ME packages cannot be read without MiniZip.
				d->isValid = false;
				d->file.reset();
				return;
			}
#  endif /* MINIZIP_IS_DLL */
#endif /* _MSC_VER */

			// Attempt to open as a .zip file first.
			d->jarFile = d->openZip(file->filename());
			if (!d->jarFile) {
				// Cannot open as a .zip file.
				d->isValid = false;
				d->file.reset();
				return;
			}

			// Load MANIFEST.MF.
			// For .jar files, this requires a Zip file lookup.
			if (d->loadManifestMF() != 0) {
				// Unable to open MANIFEST.MF.
				unzClose(d->jarFile);
				d->jarFile = nullptr;
				d->isValid = false;
				d->file.reset();
				return;
			}

			d->fileType = FileType::ApplicationPackage;	// for .jar
			break;

		case J2MEPrivate::JFileType::JAD:
			// Sanity check: Verify the JAD file size.
			if (file->size() > static_cast<off64_t>(J2MEPrivate::MANIFEST_MF_FILE_SIZE_MAX)) {
				// File is too big.
				d->isValid = false;
				d->file.reset();
				return;
			}

			// Parse the tags.
			// NOTE: This is MANIFEST.MF in a .jar file.
			// In .jad files, it's the whole thing.
			if (d->loadManifestMF() != 0) {
				// Failed to parse the tags.
				d->isValid = false;
				d->file.reset();
				return;
			}

			d->fileType = FileType::MetadataFile;		// for .jad
			break;
	}

	// Check if MANIFEST.MF has the required J2ME tags.
	// Should have either ManifestVersion or MIDlet-1.
	if (d->m_map.find(J2MEPrivate::manifest_tag_t::ManifestVersion) == d->m_map.end() &&
	    d->m_map.find(J2MEPrivate::manifest_tag_t::MIDlet_1) == d->m_map.end())
	{
		// Neither tag was found.
		if (d->jarFile) {
			unzClose(d->jarFile);
			d->jarFile = nullptr;
		}
		d->isValid = false;
		d->file.reset();
		return;
	}

	// All required tags were found.
	d->jfileType = J2MEPrivate::JFileType::JAR;
	d->isValid = ((int)d->jfileType >= 0);

	if (!d->isValid) {
		if (d->jarFile) {
			unzClose(d->jarFile);
			d->jarFile = nullptr;
		}
		d->isValid = false;
		d->file.reset();
		return;
	}

	// MIME type
	if ((int)d->jfileType >= 0) {
		d->mimeType = d->mimeTypes[static_cast<int>(d->jfileType)];
	}
}

/**
 * Close the opened file.
 */
void J2ME::close(void)
{
	RP_D(J2ME);
	if (d->jarFile) {
		unzClose(d->jarFile);
		d->jarFile = nullptr;
	}
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int J2ME::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	if (!info || !info->header.pData) {
		return static_cast<int>(J2MEPrivate::JFileType::Unknown);
	}

	// .jar check: If this is a .zip file, we can try to open it.
	if (info->header.size >= 4) {
		const uint32_t *const pData32 =
			reinterpret_cast<const uint32_t*>(info->header.pData);
		if (pData32[0] == cpu_to_be32(0x504B0304)) {
			// This appears to be a .zip file. (PK\003\004)
			// TODO: Also check for these?:
			// - PK\005\006 (empty)
			// - PK\007\008 (spanned)
			return static_cast<int>(J2MEPrivate::JFileType::JAR);
		}
	}

	// .jad check: It's a text file, so we have to rely on the file extension.
	if (info->ext && !strcasecmp(info->ext, ".jad")) {
		// File has a ".jad" extension.
		return static_cast<int>(J2MEPrivate::JFileType::JAD);
	}

	// Not supported.
	return static_cast<int>(J2MEPrivate::JFileType::Unknown);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @return System name, or nullptr if not supported.
 */
const char *J2ME::systemName(unsigned int type) const
{
	RP_D(const J2ME);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// J2ME has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"J2ME::systemName() array index optimization needs to be updated.");

	static const array<const char*, 4> sysNames = {{
		"Java 2 Micro Edition", "J2ME", "J2ME", nullptr,
	}};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t J2ME::supportedImageTypes(void) const
{
	return IMGBF_INT_ICON;
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> J2ME::supportedImageSizes_static(ImageType imageType)
{
	ASSERT_supportedImageSizes(imageType);

	switch (imageType) {
		case IMG_INT_ICON:
			// TODO: Are there other sizes?
			return {{nullptr, 15, 15, 0}};
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
vector<RomData::ImageSizeDef> J2ME::supportedImageSizes(ImageType imageType) const
{
	ASSERT_supportedImageSizes(imageType);
	RP_D(const J2ME);

	switch (imageType) {
		case IMG_INT_ICON:
			// TODO: Get the actual image size.
			// NOTE: Only .jar files have icons.
			if (d->jfileType == J2MEPrivate::JFileType::JAR) {
				return {{nullptr, 15, 15, 0}};
			}
			break;
		default:
			break;
	}

	// Unsupported image type.
	return {};
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
uint32_t J2ME::imgpf(ImageType imageType) const
{
	ASSERT_imgpf(imageType);
	RP_D(const J2ME);

	uint32_t ret = 0;
	switch (imageType) {
		case IMG_INT_ICON:
			// Use nearest-neighbor scaling when resizing.
			// Image is internally stored in PNG format.
			// NOTE: Only .jar files have icons.
			if (d->jfileType == J2MEPrivate::JFileType::JAR) {
				ret = IMGPF_RESCALE_NEAREST | IMGPF_INTERNAL_PNG_FORMAT;
			}
			break;

		default:
			break;
	}
	return ret;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int J2ME::loadFieldData(void)
{
	RP_D(J2ME);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// ROM image isn't valid.
		return -EIO;
	}

	// Show the raw MANIFEST.MF tag data.
	// TODO: Fancier names and/or leave some out?
	const auto iter_end = d->m_map.end();
	for (auto iter = d->m_map.cbegin(); iter != iter_end; ++iter) {
		assert(static_cast<uint8_t>(iter->first) < J2MEPrivate::manifest_tag_names.size());

		d->fields.addField_string(J2MEPrivate::manifest_tag_names[static_cast<uint8_t>(iter->first)], iter->second);
	}

	return static_cast<int>(d->fields.count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the metadata hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int J2ME::loadMetaData(void)
{
	RP_D(J2ME);
	if (!d->metaData.empty()) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || static_cast<int>(d->jfileType) < 0) {
		// Unknown file type.
		return -EIO;
	}

	// TODO

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
int J2ME::loadInternalImage(ImageType imageType, rp_image_const_ptr &pImage)
{
	ASSERT_loadInternalImage(imageType, pImage);
	RP_D(J2ME);

	// Only .jar files have icons.
	if (d->jfileType != J2MEPrivate::JFileType::JAR) {
		return -ENOENT;
	}

	ROMDATA_loadInternalImage_single(
		IMG_INT_ICON,	// ourImageType
		d->file,	// file
		d->isValid,	// isValid
		d->jfileType,	// romType
		d->img_icon,	// imgCache
		d->loadIcon);	// func
}

} // namespace LibRomData
