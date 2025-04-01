/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * EXE_manifest.cpp: DOS/Windows executable reader. (PE manifest reader)   *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "EXE_p.hpp"

#ifndef ENABLE_XML
#error Cannot compile EXE_manifest.cpp without XML support.
#endif

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;

// PugiXML
#include <pugixml.hpp>
using namespace pugi;

// C++ STL classes
using std::array;
using std::string;
using std::unique_ptr;
using std::vector;

namespace LibRomData {

#if defined(_MSC_VER) && defined(XML_IS_DLL)
/**
 * Check if PugiXML can be delay-loaded.
 * @return 0 on success; negative POSIX error code on error.
 */
extern int DelayLoad_test_PugiXML(void);
#endif /* defined(_MSC_VER) && defined(XML_IS_DLL) */

/** PugiXML macros **/

#define FIRST_CHILD_ELEMENT_NS(var, parent_elem, child_elem_name, namespace) \
	xml_node var = parent_elem.child(child_elem_name); \
	if (!var) { \
		var = parent_elem.child(namespace ":" child_elem_name); \
	} \
do { } while (0)

#define FIRST_CHILD_ELEMENT(var, parent_elem, child_elem_name) \
	FIRST_CHILD_ELEMENT_NS(var, parent_elem, child_elem_name, "asmv3")

#define ADD_ATTR(elem, attr_name, desc) do { \
	xml_attribute attr = elem.attribute(attr_name); \
	if (attr) { \
		fields.addField_string((desc), attr.value()); \
	} \
} while (0)

#define ADD_TEXT(parent_elem, child_elem_name, desc) do { \
	FIRST_CHILD_ELEMENT(child_elem, parent_elem, child_elem_name); \
	if (child_elem) { \
		xml_text text = child_elem.text(); \
		if (text) { \
			fields.addField_string((desc), text.get()); \
		} \
	} \
} while (0)

/**
 * Load the Win32 manifest resource.
 *
 * The XML is loaded and parsed using the specified
 * PugiXML document.
 *
 * NOTE: DelayLoad must be checked by the caller, since it's
 * passing an xml_document reference to this function.
 *
 * @param doc		[in/out] XML document
 * @param ppResName	[out,opt] Pointer to receive the loaded resource name. (statically-allocated string)
 * @return 0 on success; negative POSIX error code on error.
 */
int EXEPrivate::loadWin32ManifestResource(xml_document &doc, const char **ppResName) const
{
	// Make sure the resource directory is loaded.
	int ret = const_cast<EXEPrivate*>(this)->loadPEResourceTypes();
	if (ret != 0) {
		// Unable to load the resource directory.
		return ret;
	}

	// Manifest resource IDs
	struct resource_id_t {
		uint16_t id;
		const char *name;
	};
	static const array<resource_id_t, 4> resource_id_tbl = {{
		{CREATEPROCESS_MANIFEST_RESOURCE_ID, "CreateProcess"},
		{ISOLATIONAWARE_MANIFEST_RESOURCE_ID, "Isolation-Aware"},
		{ISOLATIONAWARE_NOSTATICIMPORT_MANIFEST_RESOURCE_ID, "Isolation-Aware, No Static Import"},

		// Windows XP's explorer.exe uses resource ID 123.
		// Reference: https://docs.microsoft.com/en-us/windows/desktop/Controls/cookbook-overview
		{XP_VISUAL_STYLE_MANIFEST_RESOURCE_ID, "Visual Style"},
	}};

	// Search for a PE manifest resource.
	IRpFilePtr f_manifest;
	const char *res_name = nullptr;
	for (const resource_id_t &id_idx : resource_id_tbl) {
		f_manifest = rsrcReader->open(RT_MANIFEST, id_idx.id, -1);
		if (f_manifest) {
			res_name = id_idx.name;
			break;
		}
	}

	if (!f_manifest) {
		// No manifest resource.
		return -ENOENT;
	}

	// PugiXML memory allocation functions
	allocation_function xml_alloc = get_memory_allocation_function();
	deallocation_function xml_dealloc = get_memory_deallocation_function();

	// Read the entire resource into memory.
	// Assuming a limit of 64 KB for manifests.
	const size_t xml_size = static_cast<size_t>(f_manifest->size());
	if (xml_size > 65536) {
		// Manifest is too big.
		// (Or, it's negative, and wraps around due to unsigned.)
		return -ENOMEM;
	}
	char *const xml_data = static_cast<char*>(xml_alloc(xml_size));
	size_t size = f_manifest->read(xml_data, xml_size);
	if (size != xml_size) {
		// Read error.
		xml_dealloc(xml_data);
		int err = f_manifest->lastError();
		if (err == 0) {
			err = EIO;
		}
		return -err;
	}
	f_manifest.reset();

	// Parse the XML.
	doc.reset();
	xml_parse_result result = doc.load_buffer_inplace_own(xml_data, xml_size, parse_default, encoding_utf8);
	if (!result) {
		// Error parsing the manifest XML.
		// TODO: Better error code.
		doc.reset();
		return -EIO;
	}

	// Root element must be assembly.
	xml_node const assembly = doc.child("assembly");
	if (!assembly) {
		// No assembly element.
		// TODO: Better error code.
		doc.reset();
		return -EIO;
	}

	// Verify assembly attributes.
	// NOTE: We're not verifying xmlns due to excessive complexity:
	// - asmv3: may be specified as part of the element name.
	// - It may also be an xmlns attribute.
	// Instead, we'll simply check for both non-prefixed and prefixed
	// element names.
	xml_attribute xmlns = assembly.attribute("xmlns");
	xml_attribute manifestVersion = assembly.attribute("manifestVersion");
	if (!xmlns || !manifestVersion ||
	    strcmp(xmlns.value(), "urn:schemas-microsoft-com:asm.v1") != 0 ||
	    strcmp(manifestVersion.value(), "1.0") != 0)
	{
		// Incorrect assembly attributes.
		// TODO: Better error code.
		doc.reset();
		return -EIO;
	}

	// XML document loaded.
	if (ppResName) {
		*ppResName = res_name;
	}
	return 0;
}

/**
 * Add fields from the Win32 manifest resource.
 * @return 0 on success; negative POSIX error code on error.
 */
int EXEPrivate::addFields_PE_Manifest(void)
{
#if defined(_MSC_VER) && defined(XML_IS_DLL)
	// Delay load verification.
	int ret_dl = DelayLoad_test_PugiXML();
	if (ret_dl != 0) {
		// Delay load failed.
		return ret_dl;
	}
#endif /* defined(_MSC_VER) && defined(XML_IS_DLL) */

	const char *pResName = nullptr;
	xml_document doc;
	int ret = loadWin32ManifestResource(doc, &pResName);
	if (ret != 0) {
		return ret;
	}

	// Add the manifest fields.
	fields.addTab(C_("EXE", "Manifest"));

	// Manifest ID
	fields.addField_string(C_("EXE|Manifest", "Manifest ID"),
		(pResName ? pResName : C_("RomData", "Unknown")));

	// Assembly element
	xml_node const assembly = doc.child("assembly");
	if (!assembly) {
		// No assembly element.
		// TODO: Better error code.
		doc.reset();
		return -EIO;
	}

	// Assembly identity
	FIRST_CHILD_ELEMENT(assemblyIdentity, assembly, "assemblyIdentity");
	if (assemblyIdentity) {
		ADD_ATTR(assemblyIdentity, "type", C_("EXE|Manifest", "Type"));
		ADD_ATTR(assemblyIdentity, "name", C_("EXE|Manifest", "Name"));
		ADD_ATTR(assemblyIdentity, "language", C_("EXE|Manifest", "Language"));
		ADD_ATTR(assemblyIdentity, "version", C_("EXE|Manifest", "Version"));
		// TODO: Replace "*" with "Any"?
		ADD_ATTR(assemblyIdentity, "processorArchitecture", C_("EXE|Manifest", "CPU Arch"));
		ADD_ATTR(assemblyIdentity, "publicKeyToken", C_("EXE|Manifest", "publicKeyToken"));
	}

	// Description
	ADD_TEXT(assembly, "description", C_("EXE|Manifest", "Description"));

	// Trust info
	// TODO: Fine-grained permissions?
	// Reference: https://docs.microsoft.com/en-us/visualstudio/deployment/trustinfo-element-clickonce-application
	FIRST_CHILD_ELEMENT_NS(trustInfo, assembly, "trustInfo", "asmv2");
	if (trustInfo) {
		FIRST_CHILD_ELEMENT_NS(security, trustInfo, "security", "asmv2");
		if (security) {
			FIRST_CHILD_ELEMENT_NS(requestedPrivileges, security, "requestedPrivileges", "asmv2");
			if (requestedPrivileges) {
				FIRST_CHILD_ELEMENT_NS(requestedExecutionLevel, requestedPrivileges, "requestedExecutionLevel", "asmv2");
				if (requestedExecutionLevel) {
					ADD_ATTR(requestedExecutionLevel, "level", C_("EXE|Manifest", "Execution Level"));
					ADD_ATTR(requestedExecutionLevel, "uiAccess", C_("EXE|Manifest", "UI Access"));
				}
			}
		}
	}

	// windowsSettings bitfield
	// Reference: https://docs.microsoft.com/en-us/windows/win32/sbscs/manifest-file-schema
	// TODO: Ordering.
	typedef enum {
		Setting_autoElevate				= (1U << 0),
		Setting_disableTheming				= (1U << 1),
		Setting_disableWindowFiltering			= (1U << 2),
		Setting_highResolutionScrollingAware		= (1U << 3),
		Setting_magicFutureSetting			= (1U << 4),
		Setting_printerDriverIsolation			= (1U << 5),
		Setting_ultraHighResolutionScrollingAware	= (1U << 6),
	} WindowsSettings_t;

	static const array<const char*, 7> WindowsSettings_names = {{
		NOP_C_("EXE|Manifest|WinSettings", "Auto Elevate"),
		NOP_C_("EXE|Manifest|WinSettings", "Disable Theming"),
		NOP_C_("EXE|Manifest|WinSettings", "Disable Window Filter"),
		NOP_C_("EXE|Manifest|WinSettings", "High-Res Scroll"),
		NOP_C_("EXE|Manifest|WinSettings", "Magic Future Setting"),
		NOP_C_("EXE|Manifest|WinSettings", "Printer Driver Isolation"),
		NOP_C_("EXE|Manifest|WinSettings", "Ultra High-Res Scroll"),
	}};

	// Windows settings
	// NOTE: application and windowsSettings may be
	// prefixed with asmv3.
	#define ADD_SETTING(settings, parent_elem, setting_name) do { \
		FIRST_CHILD_ELEMENT(child_elem, parent_elem, #setting_name); \
		if (child_elem) { \
			xml_text text = child_elem.text(); \
			if (text && !strcasecmp(text.get(), "true")) { \
				settings |= (Setting_##setting_name); \
			} \
		} \
	} while (0)

	FIRST_CHILD_ELEMENT(application, assembly, "application");
	if (application) {
		FIRST_CHILD_ELEMENT(windowsSettings, application, "windowsSettings");
		if (windowsSettings) {
			// Use a bitfield for most Windows settings.
			// DPI awareness is weird and will be handled differently.
			uint32_t settings = 0;
			ADD_SETTING(settings, windowsSettings, autoElevate);
			ADD_SETTING(settings, windowsSettings, disableTheming);
			ADD_SETTING(settings, windowsSettings, disableWindowFiltering);
			ADD_SETTING(settings, windowsSettings, highResolutionScrollingAware);
			ADD_SETTING(settings, windowsSettings, magicFutureSetting);
			ADD_SETTING(settings, windowsSettings, printerDriverIsolation);
			ADD_SETTING(settings, windowsSettings, ultraHighResolutionScrollingAware);

			// Show the bitfield.
			vector<string> *const v_WindowsSettings_names = RomFields::strArrayToVector_i18n(
				"EXE|Manifest|WinSettings", WindowsSettings_names);
			fields.addField_bitfield(C_("EXE|Manifest", "Settings"),
				v_WindowsSettings_names, 2, settings);

			// dpiAware
			// TODO: Test 10/1607 and improve descriptions.
			// TODO: Decode strings to more useful values.
			// Reference: https://docs.microsoft.com/en-us/windows/win32/sbscs/application-manifests
			ADD_TEXT(windowsSettings, "dpiAware", C_("EXE|Manifest", "DPI Aware"));
			// dpiAwareness (Win10/1607)
			ADD_TEXT(windowsSettings, "dpiAwareness", C_("EXE|Manifest", "DPI Awareness"));

			// activeCodePage (Win10/1903)
			ADD_TEXT(windowsSettings, "activeCodePage", C_("EXE|Manifest", "Active Code Page"));
		}
	}

	// Operating system compatibility.
	// References:
	// - https://docs.microsoft.com/en-us/windows/win32/sbscs/application-manifests
	// - https://docs.microsoft.com/en-us/windows/win32/sysinfo/targeting-your-application-at-windows-8-1
	typedef enum {
		OS_WinVista		= (1U << 0),
		OS_Win7			= (1U << 1),
		OS_Win8			= (1U << 2),
		OS_Win81		= (1U << 3),
		OS_Win10		= (1U << 4),

		// Not specifically OS-compatibility, but
		// present in the same section.
		OS_LongPathAware	= (1U << 5),
	} OS_Compatibility_t;

	// NOTE: OS names aren't translatable, but "Long Path Aware" is.
	static const array<const char*, 6> OS_Compatibility_names = {{
		"Windows Vista",
		"Windows 7",
		"Windows 8",
		"Windows 8.1",
		"Windows 10",
		NOP_C_("EXE|Manifest|OSCompatibility", "Long Path Aware"),
	}};

	FIRST_CHILD_ELEMENT_NS(compatibility, assembly, "compatibility", "asmv1");
	if (compatibility) {
		FIRST_CHILD_ELEMENT_NS(application, compatibility, "application", "asmv1");
		if (application) {
			// Use a bitfield for these settings.
			uint32_t compat = 0;

			// Go through all "supportedOS" elements.
			// TODO: Check for asmv1 prefixes?
			for (xml_node supportedOS = application.child("supportedOS");
			     supportedOS; supportedOS = supportedOS.next_sibling())
			{
				const char *const Id = supportedOS.attribute("Id").value();
				if (!Id || Id[0] == 0) {
					continue;
				}

				// Check for supported OSes.
				if (!strcasecmp(Id, "{e2011457-1546-43c5-a5fe-008deee3d3f0}")) {
					compat |= OS_WinVista;
				} else if (!strcasecmp(Id, "{35138b9a-5d96-4fbd-8e2d-a2440225f93a}")) {
					compat |= OS_Win7;
				} else if (!strcasecmp(Id, "{4a2f28e3-53b9-4441-ba9c-d69d4a4a6e38}")) {
					compat |= OS_Win8;
				} else if (!strcasecmp(Id, "{1f676c76-80e1-4239-95bb-83d0f6d0da78}")) {
					compat |= OS_Win81;
				} else if (!strcasecmp(Id, "{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}")) {
					// NOTE: Also used for Windows 11.
					// Reference: https://stackoverflow.com/questions/68240304/whats-the-supportedos-guid-for-windows-11
					compat |= OS_Win10;
				}
			}

			// Check for long path awareness.
			xml_node const longPathAware = application.child("longPathAware");
			if (longPathAware) {
				xml_text text = longPathAware.text();
				if (text && !strcasecmp(text.get(), "true")) {
					// Long path aware.
					compat |= OS_LongPathAware;
				}
			}

			// Show the bitfield.
			vector<string> *const v_OS_Compatibility_names = RomFields::strArrayToVector_i18n(
				"EXE|Manifest|OSCompatibility", OS_Compatibility_names);
			fields.addField_bitfield(C_("EXE|Manifest", "Compatibility"),
				v_OS_Compatibility_names, 2, compat);
		}
	}

	// Manifest read successfully.
	return 0;
}

/**
 * Is the requestedExecutionLevel set to requireAdministrator?
 * @return True if set; false if not or unable to determine.
 */
bool EXEPrivate::doesExeRequireAdministrator(void) const
{
#if defined(_MSC_VER) && defined(XML_IS_DLL)
	// Delay load verification.
	int ret_dl = DelayLoad_test_PugiXML();
	if (ret_dl != 0) {
		// Delay load failed.
		return false;
	}
#endif /* defined(_MSC_VER) && defined(XML_IS_DLL) */

	xml_document doc;
	if (loadWin32ManifestResource(doc) != 0) {
		// No Win32 manifest resource.
		return false;
	}

	// Assembly element.
	xml_node const assembly = doc.child("assembly");
	if (!assembly) {
		// No assembly element.
		return false;
	}

	// Trust info.
	FIRST_CHILD_ELEMENT_NS(trustInfo, assembly, "trustInfo", "asmv2");
	if (!trustInfo)
		return false;
	FIRST_CHILD_ELEMENT_NS(security, trustInfo, "security", "asmv2");
	if (!security)
		return false;
	FIRST_CHILD_ELEMENT_NS(requestedPrivileges, security, "requestedPrivileges", "asmv2");
	if (!requestedPrivileges)
		return false;
	FIRST_CHILD_ELEMENT_NS(requestedExecutionLevel, requestedPrivileges, "requestedExecutionLevel", "asmv2");
	if (!requestedExecutionLevel)
		return false;

	xml_attribute attr = requestedExecutionLevel.attribute("level");
	if (!attr) {
		return false;
	}
	return (!strcasecmp(attr.value(), "requireAdministrator"));
}

}
