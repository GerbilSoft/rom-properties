/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * AndroidCommon.cpp: Android common functions.                            *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"

#include "AndroidCommon.hpp"
#include "../disc/AndroidResourceReader.hpp"

// Other rom-properties libraries
using namespace LibRpBase;

// PugiXML
// NOTE: This file is only compiled if ENABLE_XML is defined.
#include <pugixml.hpp>
using namespace pugi;

// C++ STL classes
using std::array;
using std::string;
using std::vector;

namespace LibRomData { namespace AndroidCommon {

#define addField_string_i18n(name, str, flags) do { \
	if (arscReader) { \
		arscReader->addField_string_i18n(&(fields), (name), (str), (flags)); \
	} else { \
		fields.addField_string((name), (str), (flags)); \
	} \
} while (0)

/**
 * Load field data.
 * @param fields	[out] RomFields
 * @param manifest_xml	[in] AndroidManifest.xml
 * @param arscReader	[in,opt] AndroidResourceReader, if available.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int loadFieldData(LibRpBase::RomFields &fields, const pugi::xml_document &manifest_xml, const AndroidResourceReader *arscReader)
{
	// Get fields from the XML file.
	xml_node manifest_node = manifest_xml.child("manifest");
	if (!manifest_node) {
		// No "<manifest>" node???
		return 0;
	}

	const int fieldCount_initial = fields.count();
	fields.reserve(fieldCount_initial + 10);	// Maximum of 10 fields.

	// Package name is in the manifest tag.
	// <application name=""> is something else.
	const char *const package_name = manifest_node.attribute("package").as_string(nullptr);
	if (package_name && package_name[0] != '\0') {
		fields.addField_string(C_("AndroidManifestXML", "Package Name"), package_name);
	}

	// Application information
	xml_node application_node = manifest_node.child("application");
	if (application_node) {
		const char *const label = application_node.attribute("label").as_string(nullptr);
		if (label && label[0] != '\0') {
			addField_string_i18n(C_("AndroidAPK", "Title"), label, 0);
		}

		const char *const description = application_node.attribute("description").as_string(nullptr);
		if (description && description[0] != '\0') {
			addField_string_i18n(C_("AndroidAPK", "Description"), description, 0);
		}

		const char *const appCategory = application_node.attribute("appCategory").as_string(nullptr);
		if (appCategory && appCategory[0] != '\0') {
			fields.addField_string(C_("AndroidAPK", "Category"), appCategory);
		}
	}

	// SDK version
	xml_node uses_sdk = manifest_node.child("uses-sdk");
	if (uses_sdk) {
		const char *const s_minSdkVersion = uses_sdk.attribute("minSdkVersion").as_string(nullptr);
		if (s_minSdkVersion && s_minSdkVersion[0] != '\0') {
			fields.addField_string(C_("AndroidAPK", "Min. SDK Version"), s_minSdkVersion);
		}

		const char *const s_targetSdkVersion = uses_sdk.attribute("targetSdkVersion").as_string(nullptr);
		if (s_targetSdkVersion && s_targetSdkVersion[0] != '\0') {
			fields.addField_string(C_("AndroidAPK", "Target SDK Version"), s_targetSdkVersion);
		}
	}

	// Version (and version code)
	const char *const versionName = manifest_node.attribute("versionName").as_string(nullptr);
	if (versionName && versionName[0] != '\0') {
		fields.addField_string(C_("AndroidAPK", "Version"), versionName);
	}
	const char *const s_versionCode = manifest_node.attribute("versionCode").as_string(nullptr);
	if (s_versionCode && s_versionCode[0] != '\0') {
		fields.addField_string(C_("AndroidAPK", "Version Code"), s_versionCode);
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
			fields.addField_listData(C_("AndroidAPK", "Features"), &params);
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
			fields.addField_listData(C_("AndroidAPK", "Permissions"), &params);
		} else {
			delete vv_permissions;
		}
	}

	// Finished reading the field data.
	return static_cast<int>(fields.count()) - fieldCount_initial;
}

#define addMetaData_string_i18n(name, str, flags) do { \
	if (arscReader) { \
		metaData.addMetaData_string(Property::Title, arscReader->getStringFromResource(str), (flags)); \
	} else { \
		metaData.addMetaData_string(Property::Title, (str), (flags)); \
	} \
} while (0)

/**
 * Load metadata properties.
 * @param metaData	[out] RomMetaData
 * @param manifest_xml	[in] AndroidManifest.xml
 * @param arscReader	[in,opt] AndroidResourceReader, if available.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int loadMetaData(LibRpBase::RomMetaData &metaData, const pugi::xml_document &manifest_xml, const AndroidResourceReader *arscReader)
{
	// Get metadata from the XML file.
	xml_node manifest_node = manifest_xml.child("manifest");
	if (!manifest_node) {
		// No "<manifest>" node???
		return 0;
	}

	// AndroidManifest.xml is read in the constructor.
	const int metaDataCount_initial = metaData.count();
	metaData.reserve(metaDataCount_initial + 3);	// Maximum of 3 metadata properties.

	// NOTE: Only retrieving a single language.
	// TODO: Get the system language code and use it as def_lc?

	// Package name is in the manifest tag. (as Title ID)
	// <application name=""> is something else.
	const char *const package_name = manifest_node.attribute("package").as_string(nullptr);
	if (package_name && package_name[0] != '\0') {
		metaData.addMetaData_string(Property::TitleID, package_name);
	}

	// Application information
	xml_node application_node = manifest_node.child("application");
	if (application_node) {
		const char *const label = application_node.attribute("label").as_string(nullptr);
		if (label && label[0] != '\0') {
			addMetaData_string_i18n(Property::Title, label, 0);
		}

		const char *const description = application_node.attribute("description").as_string(nullptr);
		if (description && description[0] != '\0') {
			addMetaData_string_i18n(Property::Description, description, 0);
		}
	}
	// Finished reading the metadata.
	return static_cast<int>(metaData.count()) - metaDataCount_initial;
}

/**
 * Does the Android manifest have "dangerous" permissions?
 *
 * @param manifest_xml	[in] AndroidManifest.xml
 * @return True if the Android manifest has "dangerous" permissions; false if not.
 */
bool hasDangerousPermissions(const pugi::xml_document &manifest_xml)
{
	xml_node manifest_node = manifest_xml.child("manifest");
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

} }
