/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * AndroidManifestXML.hpp: Android Manifest XML reader.                    *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"

#include "config.librpbase.h"
#include "AndroidManifestXML.hpp"
#include "android_apk_structs.h"

// parseResourceID() from AndroidResourceReader
#include "../disc/AndroidResourceReader.hpp"

// Other rom-properties libraries
#include "librpfile/MemFile.hpp"
using namespace LibRpBase;
using namespace LibRpText;
using namespace LibRpFile;

// PugiXML
// NOTE: This file is only compiled if ENABLE_XML is defined.
using namespace pugi;

// C++ STL classes
#include <limits>
#include <stack>
using std::array;
using std::stack;
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
#  ifdef XML_IS_DLL
/**
 * Check if PugiXML can be delay-loaded.
 * NOTE: Implemented in EXE_delayload.cpp.
 * @return 0 on success; negative POSIX error code on error.
 */
extern int DelayLoad_test_PugiXML(void);
#  endif /* XML_IS_DLL */
#endif /* _MSC_VER */

class AndroidManifestXMLPrivate final : public RomDataPrivate
{
public:
	explicit AndroidManifestXMLPrivate(const IRpFilePtr &file);

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(AndroidManifestXMLPrivate)

public:
	/** RomDataInfo **/
	static const array<const char*, 1+1> exts;
	static const array<const char*, 1+1> mimeTypes;
	static const RomDataInfo romDataInfo;

public:
	/**
	 * Read a little-endian 32-bit word from the data.
	 * @param pData Data.
	 * @param dataLen Length of data. (32-bit maximum)
	 * @param offset Offset to read from.
	 * @return Little-endian 32-bit word.
	 */
	static inline uint32_t LEW(const uint8_t *pData, uint32_t dataLen, size_t offset)
	{
		assert(offset + 3 < dataLen);
		if (offset + 3 >= dataLen) {
			// Out of bounds.
			// TODO: Fail.
			return 0;
		}

		// TODO: Optimize with a 32-bit read?
		return (pData[offset+3] << 24) |
		       (pData[offset+2] << 16) |
		       (pData[offset+1] <<  8) |
		        pData[offset+0];
	}

	/**
	 * Return the string stored in StringTable format at offset strOff.
	 * This offset points to the 16-bit string length, which is followed
	 * by that number of 16-bit (Unicode) characters.
	 */
	static string compXmlStringAt(const uint8_t *pXml, uint32_t xmlLen, uint32_t strOff);

	/**
	 * Compose an XML string.
	 * @param pXml Compressed XML data.
	 * @param xmlLen Length of pXml. (32-bit maximum)
	 * @param sitOff StringIndexTable offset.
	 * @param stOff StringTable offset.
	 * @param strInd String index.
	 * @return XML string.
	 */
	static string compXmlString(const uint8_t *pXml, uint32_t xmlLen, uint32_t sitOff, uint32_t stOff, int strInd);

	/**
	 * Decompress Android binary XML.
	 * Strings that are referencing resources will be printed as "@0x12345678".
	 * NOTE: Need to return a pointer to work around delay-load issues.
	 * @param pXml		[in] Android binary XML data
	 * @param xmlLen	[in] Size of XML data
	 * @return Pointer to a PugiXML document, or nullptr on error.
	 */
	ATTR_ACCESS_SIZE(read_only, 1, 2)
	static xml_document *decompressAndroidBinaryXml(const uint8_t *pXml, size_t xmlLen);

public:
	// Maximum size for various files.
	static constexpr size_t AndroidManifest_xml_FILE_SIZE_MAX = (256U * 1024U);

	// Binary XML tags
	static constexpr uint32_t startDocTag	= 0x00100000 | RES_XML_START_NAMESPACE_TYPE;
	static constexpr uint32_t endDocTag	= 0x00100000 | RES_XML_END_NAMESPACE_TYPE;
	static constexpr uint32_t startTag	= 0x00100000 | RES_XML_START_ELEMENT_TYPE;
	static constexpr uint32_t endTag	= 0x00100000 | RES_XML_END_ELEMENT_TYPE;
	static constexpr uint32_t cdataTag	= 0x00100000 | RES_XML_CDATA_TYPE;

	// AndroidManifest.xml document
	// NOTE: Using a pointer to prevent delay-load issues.
	unique_ptr<xml_document> manifest_xml;
};

ROMDATA_IMPL(AndroidManifestXML)

/** AndroidManifestXMLPrivate **/

/* RomDataInfo */
const array<const char*, 1+1> AndroidManifestXMLPrivate::exts = {{
	".xml",		// FIXME: Too broad?

	nullptr
}};
const array<const char*, 1+1> AndroidManifestXMLPrivate::mimeTypes = {{
	// Vendor-specific MIME types from FreeDesktop.org.
	"application/xml",	// FIXME: Too broad?

	nullptr
}};
const RomDataInfo AndroidManifestXMLPrivate::romDataInfo = {
	"AndroidManifestXML", exts.data(), mimeTypes.data()
};

AndroidManifestXMLPrivate::AndroidManifestXMLPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
{ }

/**
 * Return the string stored in StringTable format at offset strOff.
 * This offset points to the 16-bit string length, which is followed
 * by that number of 16-bit (Unicode) characters.
 */
string AndroidManifestXMLPrivate::compXmlStringAt(const uint8_t *pXml, uint32_t xmlLen, uint32_t strOff)
{
	assert(strOff + 1 < xmlLen);
	if (strOff + 1 >= xmlLen) {
		// Out of bounds.
		return string();
	}

	const uint16_t len = (pXml[strOff+1] << 8) |
			      pXml[strOff+0];
	assert(strOff + 2 + len < xmlLen);
	if (strOff + 2 + len >= xmlLen) {
		// Out of bounds.
		return string();
	}

	// Convert from UTF-16LE to UTF-8.
	// TODO: Verify that it's always UTF-16LE.
	return utf16le_to_utf8(reinterpret_cast<const char16_t*>(&pXml[strOff + 2]), len);
} // end of compXmlStringAt

/**
 * Compose an XML string.
 * @param pXml Compressed XML data.
 * @param xmlLen Length of pXml. (32-bit maximum)
 * @param sitOff StringIndexTable offset.
 * @param stOff StringTable offset.
 * @param strInd String index.
 * @return XML string.
 */
string AndroidManifestXMLPrivate::compXmlString(const uint8_t *pXml, uint32_t xmlLen, uint32_t sitOff, uint32_t stOff, int strInd)
{
	assert(strInd >= 0);
	if (strInd < 0) {
		// Invalid string index.
		return string();
	}

	const uint32_t addr = sitOff + (strInd * 4);
	assert(addr < xmlLen);
	if (addr >= xmlLen) {
		// Out of bounds.
		return string();
	}

	uint32_t strOff = stOff + LEW(pXml, xmlLen, addr);
	return compXmlStringAt(pXml, xmlLen, strOff);
}

static inline uint16_t read_u16(const uint8_t *&p)
{
	uint16_t v = p[0] | (p[1] << 8);
	p += sizeof(v);
	return v;
}

static inline uint32_t read_u32(const uint8_t *&p)
{
	uint32_t v = p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
	p += sizeof(v);
	return v;
}

/**
 * Decompress Android binary XML.
 * Strings that are referencing resources will be printed as "@0x12345678".
 * NOTE: Need to return a pointer to work around delay-load issues.
 * @param pXml		[in] Android binary XML data
 * @param xmlLen	[in] Size of XML data
 * @return Pointer to a PugiXML document, or nullptr on error.
 */
xml_document *AndroidManifestXMLPrivate::decompressAndroidBinaryXml(const uint8_t *pXml, size_t xmlLen)
{
	// Reference:
	// - https://stackoverflow.com/questions/2097813/how-to-parse-the-androidmanifest-xml-file-inside-an-apk-package
	// - https://stackoverflow.com/a/4761689
	// TODO: Test on lots of Android packages to find any issues.
	// TODO: Split into a separate class to convert Android binary XML to text XML?
	// TODO: Instead of creating text, parse directly into a PugiXML document?

	// Compressed XML file/bytes starts with 24x bytes of data,
	// 9 32 bit words in little endian order (LSB first):
	//   0th word is 03 00 08 00
	//   3rd word SEEMS TO BE:  Offset at then of StringTable
	//   4th word is: Number of strings in string table
	// WARNING: Sometime I indiscriminently display or refer to word in 
	//   little endian storage format, or in integer format (ie MSB first).	
	const uint32_t numbStrings = LEW(pXml, xmlLen, 4*4);

	// StringIndexTable starts at offset 24x, an array of 32 bit LE offsets
	// of the length/string data in the StringTable.
	static const uint32_t sitOff = 0x24;  // Offset of start of StringIndexTable

	// StringTable, each string is represented with a 16 bit little endian 
	// character count, followed by that number of 16 bit (LE) (Unicode) chars.
	const uint32_t stOff = sitOff + numbStrings*4;  // StringTable follows StrIndexTable

	// XMLTags, The XML tag tree starts after some unknown content after the
	// StringTable.  There is some unknown data after the StringTable, scan
	// forward from this point to the flag for the start of an XML start tag.
	uint32_t xmlTagOff = LEW(pXml, xmlLen, 3*4);  // Start from the offset in the 3rd word.
	// Scan forward until we find the bytes: 0x02011000(x00100102 in normal int)
	for (unsigned int ii = xmlTagOff; ii < xmlLen-4; ii += 4) {
		if (LEW(pXml, xmlLen, ii) == startTag) { 
			xmlTagOff = ii;
			break;
		}
	} // end of hack, scanning for start of first start tag

	// XML tags and attributes:
	// Every XML start and end tag consists of 6 32 bit words:
	//   0th word: 02011000 for startTag and 03011000 for endTag 
	//   1st word: a flag?, like 38000000
	//   2nd word: Line of where this tag appeared in the original source file
	//   3rd word: FFFFFFFF ??
	//   4th word: StringIndex of NameSpace name, or FFFFFFFF for default NS
	//   5th word: StringIndex of Element Name
	//   (Note: 01011000 in 0th word means end of XML document, endDocTag)

	// Start tags (not end tags) contain 3 more words:
	//   6th word: 14001400 meaning?? 
	//   7th word: Number of Attributes that follow this tag(follow word 8th)
	//   8th word: 00000000 meaning??

	// Attributes consist of 5 words: 
	//   0th word: StringIndex of Attribute Name's Namespace, or FFFFFFFF
	//   1st word: StringIndex of Attribute Name
	//   2nd word: StringIndex of Attribute Value, or FFFFFFF if ResourceId used
	//   3rd word: Flags?
	//   4th word: str ind of attr value again, or ResourceId of value

	// TMP, dump string table to tr for debugging
	//tr.addSelect("strings", null);
	//for (int ii=0; ii<numbStrings; ii++) {
	//  // Length of string starts at StringTable plus offset in StrIndTable
	//  String str = compXmlString(xml, sitOff, stOff, ii);
	//  tr.add(String.valueOf(ii), str);
	//}
	//tr.parent();

	// Create a PugiXML document.
	unique_ptr<xml_document> doc(new xml_document);
	// Stack of tags currently being processed.
	stack<xml_node> tags;
	tags.push(*doc);
	xml_node cur_node = *doc;	// current XML node
	int ns_count = 1;		// finished processing when this reaches 0

	// Step through the XML tree element tags and attributes
	uint32_t off = xmlTagOff;
	while (off < xmlLen) {
		uint32_t tag0 = LEW(pXml, xmlLen, off);
		//uint32_t tag1 = LEW(pXml, xmlLen, off+1*4);
		//uint32_t lineNo = LEW(pXml, xmlLen, off+2*4);
		//uint32_t tag3 = LEW(pXml, xmlLen, off+3*4);
		//uint32_t nameNsSi = LEW(pXml, xmlLen, off+4*4);
		int nameSi = LEW(pXml, xmlLen, off+5*4);

		if (tag0 == startTag) { // XML START TAG
			// TODO: Verify some of the tags?
			//uint32_t tag6 = LEW(pXml, xmlLen, off+6*4);  // Expected to be 14001400
			uint32_t numbAttrs = LEW(pXml, xmlLen, off+7*4);  // Number of Attributes to follow
			//uint32_t tag8 = LEW(pXml, xmlLen, off+8*4);  // Expected to be 00000000
			off += 9*4;  // Skip over 6+3 words of startTag data

			// Create the tag.
			xml_node xmlTag = cur_node.append_child(compXmlString(pXml, xmlLen, sitOff, stOff, nameSi).c_str());
			tags.push(xmlTag);
			cur_node = xmlTag;
			//tr.addSelect(name, null);

			// Look for the Attributes
			for (uint32_t ii = 0; ii < numbAttrs; ii++) {
				//uint32_t attrNameNsSi = LEW(pXml, xmlLen, off);  // AttrName Namespace Str Ind, or FFFFFFFF
				int attrNameSi = LEW(pXml, xmlLen, off+1*4);  // AttrName String Index
				int attrValueSi = LEW(pXml, xmlLen, off+2*4); // AttrValue Str Ind, or FFFFFFFF
				const Res_value *const value = reinterpret_cast<const Res_value*>(&pXml[off+3*4]);
				off += 5*4;  // Skip over the 5 words of an attribute

				xml_attribute xmlAttr = xmlTag.append_attribute(compXmlString(pXml, xmlLen, sitOff, stOff, attrNameSi).c_str());
				if (attrValueSi != -1) {
					// Value is inline
					xmlAttr.set_value(compXmlString(pXml, xmlLen, sitOff, stOff, attrValueSi).c_str());
				} else {
					// Integer value. Determine how to handle it.
					switch (value->dataType) {
						case Res_value::TYPE_NULL:
							// 0 == undefined; 1 == empty
							// TODO: Handle undefined better.
							break;

						case Res_value::TYPE_REFERENCE:
						case Res_value::TYPE_ATTRIBUTE:
						case Res_value::TYPE_STRING:	// TODO?
						case Res_value::TYPE_DIMENSION:
						case Res_value::TYPE_FRACTION:
						case Res_value::TYPE_DYNAMIC_REFERENCE:
						case Res_value::TYPE_DYNAMIC_ATTRIBUTE:
						default:
							// Resource identifier
							// FIXME: Most of these types aren't handled properly...
							xmlAttr.set_value(fmt::format(FSTR("@0x{:0>8X}"), value->data).c_str());
							break;

						case Res_value::TYPE_FLOAT: {
							// Single-precision float
							union {
								uint32_t u32;
								float f;
							} val;
							val.u32 = value->data;
							xmlAttr.set_value(fmt::format(FSTR("{:f}"), val.f).c_str());
							break;
						}

						case Res_value::TYPE_INT_DEC:
							xmlAttr.set_value(fmt::format(FSTR("{:d}"), value->data).c_str());
							break;
						case Res_value::TYPE_INT_HEX:
							xmlAttr.set_value(fmt::format(FSTR("0x{:x}"), value->data).c_str());
							break;
						case Res_value::TYPE_INT_BOOLEAN:
							// FIXME: Error if not 0x00000000 or 0xFFFFFFFF?
							xmlAttr.set_value(value->data ? "true" : "false");
							break;

						case Res_value::TYPE_INT_COLOR_ARGB8:
							xmlAttr.set_value(fmt::format(FSTR("#{:0>8x}"), value->data).c_str());
							break;
						case Res_value::TYPE_INT_COLOR_RGB8:
							xmlAttr.set_value(fmt::format(FSTR("#{:0>6x}"), value->data).c_str());
							break;
						case Res_value::TYPE_INT_COLOR_ARGB4:
							xmlAttr.set_value(fmt::format(FSTR("#{:0>4x}"), value->data).c_str());
							break;
						case Res_value::TYPE_INT_COLOR_RGB4:
							xmlAttr.set_value(fmt::format(FSTR("#{:0>3x}"), value->data).c_str());
							break;
					}
				}
				//tr.add(attrName, attrValue);
			}
			// TODO: If no child elements, skip the '\n' and use />.
		} else if (tag0 == endTag) { // XML END TAG
			// End of the current tag.
			assert(tags.size() >= 2);
			if (tags.size() < 2) {
				// Less than 2 tags on the stack.
				// The first tag is the document, so this means there is a
				// stray end tag somewhere...
				return nullptr;
			}
			tags.pop();
			cur_node = tags.top();
			off += 6*4;  // Skip over 6 words of endTag data
			//tr.parent();  // Step back up the NobTree
		} else if (tag0 == cdataTag) { // CDATA tag
			// Reference: https://github.com/ytsutano/axmldec/blob/master/lib/jitana/util/axml_parser.cpp
			// CDATA has five DWORDs:
			// - 0: line number
			// - 1: comment
			// - 2: text (string ID)
			// - 3: unknown
			// - 4: unknown
			// NOTE: axmldec prints the CDATA as regular text, not a CDATA node.
			uint32_t tag4 = LEW(pXml, xmlLen, off+4*4);
			//cur_node.append_child(pugi::node_cdata).set_value(
			//	compXmlString(pXml, xmlLen, sitOff, stOff, tag4).c_str());
			cur_node.text().set(
				compXmlString(pXml, xmlLen, sitOff, stOff, tag4).c_str());
			off += 7*4;  // Skip over 5+2 words of cdataTag data
		} else if (tag0 == startDocTag) {
			// Start of namespace
			// NOTE: We're not handling namespaces, so we're only checking this in case
			// a document has nested namespaces.
			//uint32_t tag4 = LEW(pXml, xmlLen, off+4*4);	// namespace tag
			ns_count++;
			off += 6*4;	// Skip over 4+2 words of startDocTag data
		} else if (tag0 == endDocTag) { // END OF XML DOC TAG
			// End of namespace
			// NOTE: We're not handling namespaces, so we're only checking this in case
			// a document has nested namespaces.
			ns_count--;
			if (ns_count == 0) {
				break;
			}
			off += 6*4;	// Skip over 4+2 words of endDocTag data
		} else {
			assert(!"Unrecognized tag code!");
			return {};
		}
	} // end of while loop scanning tags and attributes of XML tree

	assert(tags.size() == 1);
	if (tags.size() != 1) {
		// The tag stack is incorrect.
		// We should only have one tag left: the root document node.
		return nullptr;
	}

	assert(ns_count == 0);
	if (ns_count != 0) {
		// The namespace count is incorrect.
		// We should have 0 namespaces remaining.
		return nullptr;
	}

	// XML document decompressed.
	return doc.release();
}

/** AndroidManifestXML **/

/**
 * Read a AndroidManifestXML file.
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
AndroidManifestXML::AndroidManifestXML(const IRpFilePtr &file)
	: super(new AndroidManifestXMLPrivate(file))
{
	// This class handles application packages.
	RP_D(AndroidManifestXML);
	d->mimeType = "application/xml";	// vendor-specific
	d->fileType = FileType::MetadataFile;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

#ifdef _MSC_VER
	// Delay load verification.
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

	// Read the file header. (at least 8 bytes)
	uint8_t header[8];
	size_t size = d->file->read(header, sizeof(header));
	if (size < 8) {
		d->file.reset();
		return;
	}

	// Check if this file is supported.
	const char *const filename = file->filename();
	const DetectInfo info = {
		{0, sizeof(header), header},
		FileSystem::file_ext(filename),	// ext
		0				// szFile (not needed for AndroidManifestXML)
	};
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		d->file.reset();
		return;
	}

	// If this is a MemFile, access the buffer directly.
	// Otherwise, load the file into memory.
	MemFile *const memFile = dynamic_cast<MemFile*>(d->file.get());
	if (memFile) {
		d->manifest_xml.reset(d->decompressAndroidBinaryXml(
			static_cast<const uint8_t*>(memFile->buffer()),
			static_cast<size_t>(memFile->size())));
	} else {
		const off64_t fileSize_o64 = d->file->size();
		if (static_cast<size_t>(fileSize_o64) > d->AndroidManifest_xml_FILE_SIZE_MAX) {
			// Too big...
			d->file.reset();
			return;
		}
		const size_t fileSize = static_cast<size_t>(fileSize_o64);

		rp::uvector<uint8_t> AndroidManifest_xml_buf;
		AndroidManifest_xml_buf.resize(fileSize);
		size_t size = d->file->seekAndRead(0, AndroidManifest_xml_buf.data(), fileSize);
		if (size != fileSize) {
			// Seel and/or read error.
			d->file.reset();
			return;
		}

		d->manifest_xml.reset(d->decompressAndroidBinaryXml(AndroidManifest_xml_buf.data(), fileSize));
	}
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int AndroidManifestXML::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->ext || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < 4)
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// File extension must be ".xml".
	if (strcasecmp(info->ext, ".xml") != 0) {
		// Not a .xml file.
		return -1;
	}

	// Check the binary XML "magic".
	const uint32_t *const pData32 = reinterpret_cast<const uint32_t*>(info->header.pData);
	if (pData32[0] != cpu_to_be32(ANDROID_BINARY_XML_MAGIC)) {
		// Incorrect magic.
		return -1;
	}

	// This appears to be an Android Binary XML file..
	return 0;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @return System name, or nullptr if not supported.
 */
const char *AndroidManifestXML::systemName(unsigned int type) const
{
	RP_D(const AndroidManifestXML);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// AndroidManifestXML has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"AndroidManifestXML::systemName() array index optimization needs to be updated.");

	static const array<const char*, 4> sysNames = {{
		"Google Android", "Android", "Android", nullptr,
	}};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int AndroidManifestXML::loadFieldData(void)
{
	RP_D(AndroidManifestXML);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || !d->manifest_xml) {
		// APK isn't valid, and/or AndroidManifestXML.xml could not be loaded.
		return -EIO;
	}

	// NOTE: We can't get resources here, but we'll get the same fields
	// as AndroidAPK anyway.
	d->fields.reserve(10);	// Maximum of 10 fields.

	// Get fields from the XML file.
	xml_node manifest_node = d->manifest_xml->child("manifest");
	if (!manifest_node) {
		// No "<manifest>" node???
		return static_cast<int>(d->fields.count());
	}

	// Application information
	xml_node application_node = manifest_node.child("application");
	if (application_node) {
		const char *const label = application_node.attribute("label").as_string(nullptr);
		if (label && label[0] != '\0') {
			d->fields.addField_string(C_("AndroidManifestXML", "Title"), label);
		}

		const char *const name = application_node.attribute("name").as_string(nullptr);
		if (name && name[0] != '\0') {
			d->fields.addField_string(C_("AndroidManifestXML", "Package Name"), name);
		}

		const char *const description = application_node.attribute("description").as_string(nullptr);
		if (description && description[0] != '\0') {
			d->fields.addField_string(C_("AndroidManifestXML", "Description"), description);
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
			d->fields.addField_listData(C_("AndroidManifestXML", "Permissions"), &params);
		} else {
			delete vv_permissions;
		}
	}

	return static_cast<int>(d->fields.count());
}

/**
 * Get the decompressed XML document, as parsed by PugiXML.
 * @return xml_document, or nullptr if it wasn't loaded.
 */
const xml_document *AndroidManifestXML::getXmlDocument(void) const
{
	RP_D(const AndroidManifestXML);
	return	d->manifest_xml.get();
}

/**
 * Take ownership of the decompressed XML document, as parsed by PugiXML.
 * @return xml_document, or nullptr if it wasn't loaded.
 */
pugi::xml_document *AndroidManifestXML::takeXmlDocument(void)
{
	RP_D(AndroidManifestXML);
	return d->manifest_xml.release();
}

} // namespace LibRomData
