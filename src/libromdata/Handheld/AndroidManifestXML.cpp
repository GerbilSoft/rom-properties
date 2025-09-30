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
	// - https://github.com/iBotPeaches/platform_frameworks_base/blob/main/libs/androidfw/include/androidfw/ResourceTypes.h
	// - https://apktool.org/wiki/advanced/resources-arsc/
	// TODO: Test on lots of Android packages to find any issues.
	assert(xmlLen > (sizeof(ResXMLTree_header) + sizeof(ResStringPool_header)));
	if (xmlLen <= (sizeof(ResXMLTree_header) + sizeof(ResStringPool_header))) {
		// Not enough data...
		return nullptr;
	}

	const uint8_t *p = pXml;
	const uint8_t *const pEnd = pXml + xmlLen;

	// Verify that the first chunk is RES_XML_TYPE.
	const ResXMLTree_header *const pXmlHdr = reinterpret_cast<const ResXMLTree_header*>(pXml);
	assert(pXmlHdr->header.type == cpu_to_le16(RES_XML_TYPE));
	assert(pXmlHdr->header.headerSize == cpu_to_le16(sizeof(*pXmlHdr)));
	assert(le32_to_cpu(pXmlHdr->header.size) == xmlLen);
	if (pXmlHdr->header.type != cpu_to_le16(RES_XML_TYPE) ||
	    pXmlHdr->header.headerSize != cpu_to_le16(sizeof(*pXmlHdr)) ||
	    le32_to_cpu(pXmlHdr->header.size) != xmlLen)
	{
		// Incorrect XML header.
		return nullptr;
	}
	p += sizeof(ResXMLTree_header);

	// Next chunk should be the string pool.
	const ResStringPool_header *const pStringPoolHdr = reinterpret_cast<const ResStringPool_header*>(pXml + 2*4);
	assert(pStringPoolHdr->header.type == cpu_to_le16(RES_STRING_POOL_TYPE));
	assert(pStringPoolHdr->header.headerSize == cpu_to_le16(sizeof(*pStringPoolHdr)));
	if (pStringPoolHdr->header.type != cpu_to_le16(RES_STRING_POOL_TYPE) ||
	    pStringPoolHdr->header.headerSize != cpu_to_le16(sizeof(*pStringPoolHdr)))
	{
		// Incorrect string pool header.
		return nullptr;
	}
	const uint32_t stringPoolSize = le32_to_cpu(pStringPoolHdr->header.size);
	AndroidResourceReader::StringPoolAccessor xmlStringPool(
		reinterpret_cast<const uint8_t*>(pStringPoolHdr), stringPoolSize);
	p += stringPoolSize;

	// There might be an XML resource map after the string table.
	// Parse chunks until we find the first XML start tag.
	// NOTE: com.android.chrome (140.0.7339.207, 733920733) does *not* start
	// with RES_XML_FIRST_CHUNK_TYPE / RES_XML_START_NAMESPACE_TYPE;
	// it starts with RES_XML_START_ELEMENT_TYPE.
	while (p < pEnd) {
		const ResChunk_header *const pHdr = reinterpret_cast<const ResChunk_header*>(p);
		if (pHdr->type == cpu_to_le16(RES_XML_FIRST_CHUNK_TYPE) ||
		    pHdr->type == cpu_to_le16(RES_XML_START_ELEMENT_TYPE))
		{
			// Found the first XML tag.
 			break;
 		}
		// Skip this chunk.
		p += le32_to_cpu(pHdr->size);
	}
	assert(p < pEnd);
	if (p >= pEnd) {
		// EOF?
		return nullptr;
	}

	// Create a PugiXML document.
	unique_ptr<xml_document> doc(new xml_document);
	// Stack of tags currently being processed.
	stack<xml_node> tags;
	tags.push(*doc);
	xml_node cur_node = *doc;	// current XML node
	int ns_count = 0;

	// Step through the XML tree element tags and attributes
	while (p < pEnd) {
		assert(p + sizeof(ResXMLTree_node) <= pEnd);
		if (p + sizeof(ResXMLTree_node) > pEnd) {
			break;
		}
		const ResXMLTree_node *const node = reinterpret_cast<const ResXMLTree_node*>(p);
		//printf("p == 0x%08X, node size == 0x%04X\n", (unsigned int)(p - pXml), node->header.size);
		assert(node->header.headerSize == cpu_to_le16(sizeof(ResXMLTree_node)));
		if (node->header.headerSize != cpu_to_le16(sizeof(ResXMLTree_node))) {
			break;
		}

		// pNodeData == contents of the node
		const uint8_t *pNodeData = p + sizeof(ResXMLTree_node);

		// Make sure this node doesn't go out of bounds.
		const uint32_t node_size = le32_to_cpu(node->header.size);
		assert(p + node_size <= pEnd);
		if (p + node_size > pEnd) {
			break;
		}

		const uint16_t node_type = le16_to_cpu(node->header.type);
		if (node_type == RES_XML_START_ELEMENT_TYPE) { // XML START TAG
			const ResXMLTree_attrExt *const pAttrExt = reinterpret_cast<const ResXMLTree_attrExt*>(pNodeData);

			// Attributes should start at 0x14 and be 0x14 bytes.
			// (ResXMLTree_attribute struct)
			assert(pAttrExt->attributeStart == cpu_to_le16(0x14));
			assert(pAttrExt->attributeSize  == cpu_to_le16(0x14));

			const unsigned int numbAttrs = le16_to_cpu(pAttrExt->attributeCount);
			pNodeData += sizeof(ResXMLTree_attrExt);

			// Create the tag.
			xml_node xmlTag = cur_node.append_child(xmlStringPool.getString(le32_to_cpu(pAttrExt->name.index)).c_str());
			tags.push(xmlTag);
			cur_node = xmlTag;
			//tr.addSelect(name, null);

			// Look for the Attributes
			const ResXMLTree_attribute *pAttrData = reinterpret_cast<const ResXMLTree_attribute*>(pNodeData);
			for (uint32_t ii = 0; ii < numbAttrs; ii++, pAttrData++) {
				//uint32_t attrNameNsSi = le32_to_cpu(pAttrData->ns.index);	// AttrName Namespace Str Ind, or FFFFFFFF
				int attrNameSi = le32_to_cpu(pAttrData->name.index);		// AttrName String Index
				int attrValueSi = le32_to_cpu(pAttrData->rawValue.index);	// AttrValue Str Ind, or FFFFFFFF

				xml_attribute xmlAttr = xmlTag.append_attribute(xmlStringPool.getString(attrNameSi).c_str());
				if (attrValueSi != -1) {
					// Value is inline
					xmlAttr.set_value(xmlStringPool.getString(attrValueSi).c_str());
				} else {
					// Integer value. Determine how to handle it.
					const uint32_t u32data = le32_to_cpu(pAttrData->typedValue.data);
					switch (pAttrData->typedValue.dataType) {
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
							xmlAttr.set_value(fmt::format(FSTR("@0x{:0>8X}"), u32data).c_str());
							break;

						case Res_value::TYPE_FLOAT: {
							// Single-precision float
							union {
								uint32_t u32;
								float f;
							} val;
							val.u32 = u32data;
							xmlAttr.set_value(fmt::format(FSTR("{:f}"), val.f).c_str());
							break;
						}

						case Res_value::TYPE_INT_DEC:
							xmlAttr.set_value(fmt::format(FSTR("{:d}"), u32data).c_str());
							break;
						case Res_value::TYPE_INT_HEX:
							xmlAttr.set_value(fmt::format(FSTR("0x{:x}"), u32data).c_str());
							break;
						case Res_value::TYPE_INT_BOOLEAN:
							// FIXME: Error if not 0x00000000 or 0xFFFFFFFF?
							xmlAttr.set_value(u32data ? "true" : "false");
							break;

						case Res_value::TYPE_INT_COLOR_ARGB8:
							xmlAttr.set_value(fmt::format(FSTR("#{:0>8x}"), u32data).c_str());
							break;
						case Res_value::TYPE_INT_COLOR_RGB8:
							xmlAttr.set_value(fmt::format(FSTR("#{:0>6x}"), u32data).c_str());
							break;
						case Res_value::TYPE_INT_COLOR_ARGB4:
							xmlAttr.set_value(fmt::format(FSTR("#{:0>4x}"), u32data).c_str());
							break;
						case Res_value::TYPE_INT_COLOR_RGB4:
							xmlAttr.set_value(fmt::format(FSTR("#{:0>3x}"), u32data).c_str());
							break;
					}
				}
				//tr.add(attrName, attrValue);
			}
			// TODO: If no child elements, skip the '\n' and use />.
		} else if (node_type == RES_XML_END_ELEMENT_TYPE) { // XML END TAG
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
			//tr.parent();  // Step back up the NobTree
		} else if (node_type == RES_XML_CDATA_TYPE) { // CDATA tag
			const ResXMLTree_cdataExt *const pCdataExt = reinterpret_cast<const ResXMLTree_cdataExt*>(pNodeData);

			// TODO: Handle typed data?
			const uint32_t u32data = le32_to_cpu(pCdataExt->data.index);
			//cur_node.append_child(pugi::node_cdata).set_value(
			//	xmlStringPool.getString(u32data).c_str());
			cur_node.text().set(xmlStringPool.getString(u32data).c_str());
		} else if (node_type == RES_XML_START_NAMESPACE_TYPE) {
			// Start of namespace (TODO)
			//const ResXMLTree_namespaceExt *const pNamespaceExt = reinterpret_cast<const ResXMLTree_namespaceExt*>(pNodeData);
			// NOTE: We're not handling namespaces, so we're only checking this in case
			// a document has nested namespaces.
			ns_count++;
		} else if (node_type == RES_XML_END_NAMESPACE_TYPE) { // END OF XML DOC TAG
			// End of namespace
			// NOTE: We're not handling namespaces, so we're only checking this in case
			// a document has nested namespaces.
			ns_count--;
			assert(ns_count >= 0);
			if (ns_count < 0) {
				// Too many end-namespace tags...
				break;
			}
		} else {
			assert(!"Unrecognized tag code!");
			return {};
		}

		// Next node.
		p += node_size;
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
			// Seek and/or read error.
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
	// as AndroidManifestXML anyway.
	d->fields.reserve(10);	// Maximum of 10 fields.

	// Get fields from the XML file.
	xml_node manifest_node = d->manifest_xml->child("manifest");
	if (!manifest_node) {
		// No "<manifest>" node???
		return static_cast<int>(d->fields.count());
	}

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
			d->fields.addField_string(C_("AndroidManifestXML", "Title"), label);
		}

		const char *const description = application_node.attribute("description").as_string(nullptr);
		if (description && description[0] != '\0') {
			d->fields.addField_string(C_("AndroidManifestXML", "Description"), description);
		}

		const char *const appCategory = application_node.attribute("appCategory").as_string(nullptr);
		if (appCategory && appCategory[0] != '\0') {
			d->fields.addField_string(C_("AndroidManifestXML", "Category"), appCategory);
		}
	}

	// SDK version
	xml_node uses_sdk = manifest_node.child("uses-sdk");
	if (uses_sdk) {
		const char *const s_minSdkVersion = uses_sdk.attribute("minSdkVersion").as_string(nullptr);
		if (s_minSdkVersion && s_minSdkVersion[0] != '\0') {
			d->fields.addField_string(C_("AndroidManifestXML", "Min. SDK Version"), s_minSdkVersion);
		}

		const char *const s_targetSdkVersion = uses_sdk.attribute("targetSdkVersion").as_string(nullptr);
		if (s_targetSdkVersion && s_targetSdkVersion[0] != '\0') {
			d->fields.addField_string(C_("AndroidManifestXML", "Target SDK Version"), s_targetSdkVersion);
		}
	}

	// Version (and version code)
	const char *const versionName = manifest_node.attribute("versionName").as_string(nullptr);
	if (versionName && versionName[0] != '\0') {
		d->fields.addField_string(C_("AndroidManifestXML", "Version"), versionName);
	}
	const char *const s_versionCode = manifest_node.attribute("versionCode").as_string(nullptr);
	if (s_versionCode && s_versionCode[0] != '\0') {
		d->fields.addField_string(C_("AndroidManifestXML", "Version Code"), s_versionCode);
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
				NOP_C_("AndroidManifestXML|Features", "Feature"),
				NOP_C_("AndroidManifestXML|Features", "Required?"),
			}};
			vector<string> *const v_features_headers = RomFields::strArrayToVector_i18n(
				"AndroidManifestXML|Features", features_headers);

			RomFields::AFLD_PARAMS params(0, rows_visible);
			params.headers = v_features_headers;
			params.data.single = vv_features;
			params.col_attrs.align_data = AFLD_ALIGN2(TXA_D, TXA_C);
			d->fields.addField_listData(C_("AndroidManifestXML", "Features"), &params);
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
 * Load metadata properties.
 * Called by RomData::metaData() if the metadata hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int AndroidManifestXML::loadMetaData(void)
{
	RP_D(AndroidManifestXML);
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
		return static_cast<int>(d->fields.count());
	}

	// AndroidManifest.xml is read in the constructor.
	// NOTE: Resources are not available here, so we can't retrieve string resources.
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
			d->metaData.addMetaData_string(Property::Title, label);
		}

		const char *const description = application_node.attribute("description").as_string(nullptr);
		if (description && description[0] != '\0') {
			d->metaData.addMetaData_string(Property::Description, description);
		}
	}
	// Finished reading the metadata.
	return static_cast<int>(d->metaData.count());
}

/**
 * Does this ROM image have "dangerous" permissions?
 *
 * @return True if the ROM image has "dangerous" permissions; false if not.
 */
bool AndroidManifestXML::hasDangerousPermissions(void) const
{
	RP_D(const AndroidManifestXML);
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
