/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * android_apk_structs.h: Android APK data structures.                     *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: Apache-2.0                                     *
 ***************************************************************************/

#pragma once

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

// from ResourceTypes.h
// References:
// - https://github.com/iBotPeaches/platform_frameworks_base/blob/main/libs/androidfw/include/androidfw/ResourceTypes.h
// - https://apktool.org/wiki/advanced/resources-arsc/

/**
 * Header that appears at the front of every data chunk in a resource.
 */
struct ResChunk_header
{
	// Type identifier for this chunk.  The meaning of this value depends
	// on the containing chunk.
	uint16_t type;

	// Size of the chunk header (in bytes).  Adding this value to
	// the address of the chunk allows you to find its associated data
	// (if any).
	uint16_t headerSize;

	// Total size of this chunk (in bytes).  This is the chunkSize plus
	// the size of any data associated with the chunk.  Adding this value
	// to the chunk allows you to completely skip its contents (including
	// any child chunks).  If this value is the same as chunkSize, there is
	// no data associated with the chunk.
	uint32_t size;
};

enum {
	RES_NULL_TYPE                     = 0x0000,
	RES_STRING_POOL_TYPE              = 0x0001,
	RES_TABLE_TYPE                    = 0x0002,
	RES_XML_TYPE                      = 0x0003,

	// Chunk types in RES_XML_TYPE
	RES_XML_FIRST_CHUNK_TYPE          = 0x0100,
	RES_XML_START_NAMESPACE_TYPE      = 0x0100,
	RES_XML_END_NAMESPACE_TYPE        = 0x0101,
	RES_XML_START_ELEMENT_TYPE        = 0x0102,
	RES_XML_END_ELEMENT_TYPE          = 0x0103,
	RES_XML_CDATA_TYPE                = 0x0104,
	RES_XML_LAST_CHUNK_TYPE           = 0x017f,
	// This contains a uint32_t array mapping strings in the string
	// pool back to resource identifiers.  It is optional.
	RES_XML_RESOURCE_MAP_TYPE         = 0x0180,

	// Chunk types in RES_TABLE_TYPE
	RES_TABLE_PACKAGE_TYPE            = 0x0200,
	RES_TABLE_TYPE_TYPE               = 0x0201,
	RES_TABLE_TYPE_SPEC_TYPE          = 0x0202,
	RES_TABLE_LIBRARY_TYPE            = 0x0203,
	RES_TABLE_OVERLAYABLE_TYPE        = 0x0204,
	RES_TABLE_OVERLAYABLE_POLICY_TYPE = 0x0205,
	RES_TABLE_STAGED_ALIAS_TYPE       = 0x0206,
};

/**
 * Macros for building/splitting resource identifiers.
 */
#define Res_VALIDID(resid) (resid != 0)
#define Res_CHECKID(resid) ((resid&0xFFFF0000) != 0)
#define Res_MAKEID(package, type, entry) \
    (((package+1)<<24) | (((type+1)&0xFF)<<16) | (entry&0xFFFF))
#define Res_GETPACKAGE(id) ((id>>24)-1)
#define Res_GETTYPE(id) (((id>>16)&0xFF)-1)
#define Res_GETENTRY(id) (id&0xFFFF)

#define Res_INTERNALID(resid) ((resid&0xFFFF0000) != 0 && (resid&0xFF0000) == 0)
#define Res_MAKEINTERNAL(entry) (0x01000000 | (entry&0xFFFF))
#define Res_MAKEARRAY(entry) (0x02000000 | (entry&0xFFFF))

static const size_t Res_MAXPACKAGE = 255;
static const size_t Res_MAXTYPE = 255;

/**
 * Representation of a value in a resource, supplying type
 * information.
 */
struct Res_value
{
	// Number of bytes in this structure.
	uint16_t size;

	// Always set to 0.
	uint8_t res0;

	// Type of the data value.
	enum : uint8_t {
		// The 'data' is either 0 or 1, specifying this resource is either
		// undefined or empty, respectively.
		TYPE_NULL = 0x00,
		// The 'data' holds a ResTable_ref, a reference to another resource
		// table entry.
		TYPE_REFERENCE = 0x01,
		// The 'data' holds an attribute resource identifier.
		TYPE_ATTRIBUTE = 0x02,
		// The 'data' holds an index into the containing resource table's
		// global value string pool.
		TYPE_STRING = 0x03,
		// The 'data' holds a single-precision floating point number.
		TYPE_FLOAT = 0x04,
		// The 'data' holds a complex number encoding a dimension value,
		// such as "100in".
		TYPE_DIMENSION = 0x05,
		// The 'data' holds a complex number encoding a fraction of a
		// container.
		TYPE_FRACTION = 0x06,
		// The 'data' holds a dynamic ResTable_ref, which needs to be
		// resolved before it can be used like a TYPE_REFERENCE.
		TYPE_DYNAMIC_REFERENCE = 0x07,
		// The 'data' holds an attribute resource identifier, which needs to be resolved
		// before it can be used like a TYPE_ATTRIBUTE.
		TYPE_DYNAMIC_ATTRIBUTE = 0x08,

		// Beginning of integer flavors...
		TYPE_FIRST_INT = 0x10,

		// The 'data' is a raw integer value of the form n..n.
		TYPE_INT_DEC = 0x10,
		// The 'data' is a raw integer value of the form 0xn..n.
		TYPE_INT_HEX = 0x11,
		// The 'data' is either 0 or 1, for input "false" or "true" respectively.
		TYPE_INT_BOOLEAN = 0x12,

		// Beginning of color integer flavors...
		TYPE_FIRST_COLOR_INT = 0x1c,

		// The 'data' is a raw integer value of the form #aarrggbb.
		TYPE_INT_COLOR_ARGB8 = 0x1c,
		// The 'data' is a raw integer value of the form #rrggbb.
		TYPE_INT_COLOR_RGB8 = 0x1d,
		// The 'data' is a raw integer value of the form #argb.
		TYPE_INT_COLOR_ARGB4 = 0x1e,
		// The 'data' is a raw integer value of the form #rgb.
		TYPE_INT_COLOR_RGB4 = 0x1f,

		// ...end of integer flavors.
		TYPE_LAST_COLOR_INT = 0x1f,

		// ...end of integer flavors.
		TYPE_LAST_INT = 0x1f
	};
	uint8_t dataType;

	// Structure of complex data values (TYPE_UNIT and TYPE_FRACTION)
	enum {
		// Where the unit type information is.  This gives us 16 possible
		// types, as defined below.
		COMPLEX_UNIT_SHIFT = 0,
		COMPLEX_UNIT_MASK = 0xf,

		// TYPE_DIMENSION: Value is raw pixels.
		COMPLEX_UNIT_PX = 0,
		// TYPE_DIMENSION: Value is Device Independent Pixels.
		COMPLEX_UNIT_DIP = 1,
		// TYPE_DIMENSION: Value is a Scaled device independent Pixels.
		COMPLEX_UNIT_SP = 2,
		// TYPE_DIMENSION: Value is in points.
		COMPLEX_UNIT_PT = 3,
		// TYPE_DIMENSION: Value is in inches.
		COMPLEX_UNIT_IN = 4,
		// TYPE_DIMENSION: Value is in millimeters.
		COMPLEX_UNIT_MM = 5,

		// TYPE_FRACTION: A basic fraction of the overall size.
		COMPLEX_UNIT_FRACTION = 0,
		// TYPE_FRACTION: A fraction of the parent size.
		COMPLEX_UNIT_FRACTION_PARENT = 1,

		// Where the radix information is, telling where the decimal place
		// appears in the mantissa.  This give us 4 possible fixed point
		// representations as defined below.
		COMPLEX_RADIX_SHIFT = 4,
		COMPLEX_RADIX_MASK = 0x3,

		// The mantissa is an integral number -- i.e., 0xnnnnnn.0
		COMPLEX_RADIX_23p0 = 0,
		// The mantissa magnitude is 16 bits -- i.e, 0xnnnn.nn
		COMPLEX_RADIX_16p7 = 1,
		// The mantissa magnitude is 8 bits -- i.e, 0xnn.nnnn
		COMPLEX_RADIX_8p15 = 2,
		// The mantissa magnitude is 0 bits -- i.e, 0x0.nnnnnn
		COMPLEX_RADIX_0p23 = 3,

		// Where the actual value is.  This gives us 23 bits of
		// precision.  The top bit is the sign.
		COMPLEX_MANTISSA_SHIFT = 8,
		COMPLEX_MANTISSA_MASK = 0xffffff
	};

	// Possible data values for TYPE_NULL.
	enum {
		// The value is not defined.
		DATA_NULL_UNDEFINED = 0,
		// The value is explicitly defined as empty.
		DATA_NULL_EMPTY = 1
	};

	// The data for this item, as interpreted according to dataType.
	typedef uint32_t data_type;
	data_type data;

	//void copyFrom_dtoh(const Res_value& src);
};

/**
 *  This is a reference to a unique entry (a ResTable_entry structure)
 *  in a resource table.  The value is structured as: 0xpptteeee,
 *  where pp is the package index, tt is the type index in that
 *  package, and eeee is the entry index in that type.  The package
 *  and type values start at 1 for the first item, to help catch cases
 *  where they have not been supplied.
 */
struct ResTable_ref
{
	uint32_t ident;
};

/**
 * Reference to a string in a string pool.
 */
struct ResStringPool_ref
{
	// Index into the string pool table (uint32_t-offset from the indices
	// immediately after ResStringPool_header) at which to find the location
	// of the string data in the pool.
	uint32_t index;
};

/**
 * Header for a resource table.  Its data contains a series of
 * additional chunks:
 *   * A ResStringPool_header containing all table values.  This string pool
 *     contains all of the string values in the entire resource table (not
 *     the names of entries or type identifiers however).
 *   * One or more ResTable_package chunks.
 *
 * Specific entries within a resource table can be uniquely identified
 * with a single integer as defined by the ResTable_ref structure.
 */
struct ResTable_header
{
	struct ResChunk_header header;

	// The number of ResTable_package structures.
	uint32_t packageCount;
};

/**
 * A collection of resource data types within a package.  Followed by
 * one or more ResTable_type and ResTable_typeSpec structures containing the
 * entry values for each resource type.
 */
struct ResTable_package
{
	struct ResChunk_header header;

	// If this is a base package, its ID.  Package IDs start
	// at 1 (corresponding to the value of the package bits in a
	// resource identifier).  0 means this is not a base package.
	uint32_t id;

	// Actual name of this package, \0-terminated.
	uint16_t name[128];

	// Offset to a ResStringPool_header defining the resource
	// type symbol table.  If zero, this package is inheriting from
	// another base package (overriding specific values in it).
	uint32_t typeStrings;

	// Last index into typeStrings that is for public use by others.
	uint32_t lastPublicType;

	// Offset to a ResStringPool_header defining the resource
	// key symbol table.  If zero, this package is inheriting from
	// another base package (overriding specific values in it).
	uint32_t keyStrings;

	// Last index into keyStrings that is for public use by others.
	uint32_t lastPublicKey;

	uint32_t typeIdOffset;
};

// The most specific locale can consist of:
//
// - a 3 char language code
// - a 3 char region code prefixed by a 'r'
// - a 4 char script code prefixed by a 's'
// - a 8 char variant code prefixed by a 'v'
//
// each separated by a single char separator, which sums up to a total of 24
// chars, (25 include the string terminator). Numbering system specificator,
// if present, can add up to 14 bytes (-u-nu-xxxxxxxx), giving 39 bytes,
// or 40 bytes to make it 4 bytes aligned.
#define RESTABLE_MAX_LOCALE_LEN 40


// from https://android.googlesource.com/platform/frameworks/native/+/master/include/android/configuration.h
/**
 * Define flags and constants for various subsystem configurations.
 */
enum {
	/** Orientation: not specified. */
	ACONFIGURATION_ORIENTATION_ANY  = 0x0000,
	/**
	 * Orientation: value corresponding to the
	 * <a href="/guide/topics/resources/providing-resources.html#OrientationQualifier">port</a>
	 * resource qualifier.
	 */
	ACONFIGURATION_ORIENTATION_PORT = 0x0001,
	/**
	 * Orientation: value corresponding to the
	 * <a href="/guide/topics/resources/providing-resources.html#OrientationQualifier">land</a>
	 * resource qualifier.
	 */
	ACONFIGURATION_ORIENTATION_LAND = 0x0002,
	/** @deprecated Not currently supported or used. */
	ACONFIGURATION_ORIENTATION_SQUARE = 0x0003,
	/** Touchscreen: not specified. */
	ACONFIGURATION_TOUCHSCREEN_ANY  = 0x0000,
	/**
	 * Touchscreen: value corresponding to the
	 * <a href="/guide/topics/resources/providing-resources.html#TouchscreenQualifier">notouch</a>
	 * resource qualifier.
	 */
	ACONFIGURATION_TOUCHSCREEN_NOTOUCH  = 0x0001,
	/** @deprecated Not currently supported or used. */
	ACONFIGURATION_TOUCHSCREEN_STYLUS  = 0x0002,
	/**
	 * Touchscreen: value corresponding to the
	 * <a href="/guide/topics/resources/providing-resources.html#TouchscreenQualifier">finger</a>
	 * resource qualifier.
	 */
	ACONFIGURATION_TOUCHSCREEN_FINGER  = 0x0003,
	/** Density: default density. */
	ACONFIGURATION_DENSITY_DEFAULT = 0,
	/**
	 * Density: value corresponding to the
	 * <a href="/guide/topics/resources/providing-resources.html#DensityQualifier">ldpi</a>
	 * resource qualifier.
	 */
	ACONFIGURATION_DENSITY_LOW = 120,
	/**
	 * Density: value corresponding to the
	 * <a href="/guide/topics/resources/providing-resources.html#DensityQualifier">mdpi</a>
	 * resource qualifier.
	 */
	ACONFIGURATION_DENSITY_MEDIUM = 160,
	/**
	 * Density: value corresponding to the
	 * <a href="/guide/topics/resources/providing-resources.html#DensityQualifier">tvdpi</a>
	 * resource qualifier.
	 */
	ACONFIGURATION_DENSITY_TV = 213,
	/**
	 * Density: value corresponding to the
	 * <a href="/guide/topics/resources/providing-resources.html#DensityQualifier">hdpi</a>
	 * resource qualifier.
	 */
	ACONFIGURATION_DENSITY_HIGH = 240,
	/**
	 * Density: value corresponding to the
	 * <a href="/guide/topics/resources/providing-resources.html#DensityQualifier">xhdpi</a>
	 * resource qualifier.
	 */
	ACONFIGURATION_DENSITY_XHIGH = 320,
	/**
	 * Density: value corresponding to the
	 * <a href="/guide/topics/resources/providing-resources.html#DensityQualifier">xxhdpi</a>
	 * resource qualifier.
	 */
	ACONFIGURATION_DENSITY_XXHIGH = 480,
	/**
	 * Density: value corresponding to the
	 * <a href="/guide/topics/resources/providing-resources.html#DensityQualifier">xxxhdpi</a>
	 * resource qualifier.
	 */
	ACONFIGURATION_DENSITY_XXXHIGH = 640,
	/** Density: any density. */
	ACONFIGURATION_DENSITY_ANY = 0xfffe,
	/** Density: no density specified. */
	ACONFIGURATION_DENSITY_NONE = 0xffff,
	/** Keyboard: not specified. */
	ACONFIGURATION_KEYBOARD_ANY  = 0x0000,
	/**
	 * Keyboard: value corresponding to the
	 * <a href="/guide/topics/resources/providing-resources.html#ImeQualifier">nokeys</a>
	 * resource qualifier.
	 */
	ACONFIGURATION_KEYBOARD_NOKEYS  = 0x0001,
	/**
	 * Keyboard: value corresponding to the
	 * <a href="/guide/topics/resources/providing-resources.html#ImeQualifier">qwerty</a>
	 * resource qualifier.
	 */
	ACONFIGURATION_KEYBOARD_QWERTY  = 0x0002,
	/**
	 * Keyboard: value corresponding to the
	 * <a href="/guide/topics/resources/providing-resources.html#ImeQualifier">12key</a>
	 * resource qualifier.
	 */
	ACONFIGURATION_KEYBOARD_12KEY  = 0x0003,
	/** Navigation: not specified. */
	ACONFIGURATION_NAVIGATION_ANY  = 0x0000,
	/**
	 * Navigation: value corresponding to the
	 * <a href="@/guide/topics/resources/providing-resources.html#NavigationQualifier">nonav</a>
	 * resource qualifier.
	 */
	ACONFIGURATION_NAVIGATION_NONAV  = 0x0001,
	/**
	 * Navigation: value corresponding to the
	 * <a href="/guide/topics/resources/providing-resources.html#NavigationQualifier">dpad</a>
	 * resource qualifier.
	 */
	ACONFIGURATION_NAVIGATION_DPAD  = 0x0002,
	/**
	 * Navigation: value corresponding to the
	 * <a href="/guide/topics/resources/providing-resources.html#NavigationQualifier">trackball</a>
	 * resource qualifier.
	 */
	ACONFIGURATION_NAVIGATION_TRACKBALL  = 0x0003,
	/**
	 * Navigation: value corresponding to the
	 * <a href="/guide/topics/resources/providing-resources.html#NavigationQualifier">wheel</a>
	 * resource qualifier.
	 */
	ACONFIGURATION_NAVIGATION_WHEEL  = 0x0004,
	/** Keyboard availability: not specified. */
	ACONFIGURATION_KEYSHIDDEN_ANY = 0x0000,
	/**
	 * Keyboard availability: value corresponding to the
	 * <a href="/guide/topics/resources/providing-resources.html#KeyboardAvailQualifier">keysexposed</a>
	 * resource qualifier.
	 */
	ACONFIGURATION_KEYSHIDDEN_NO = 0x0001,
	/**
	 * Keyboard availability: value corresponding to the
	 * <a href="/guide/topics/resources/providing-resources.html#KeyboardAvailQualifier">keyshidden</a>
	 * resource qualifier.
	 */
	ACONFIGURATION_KEYSHIDDEN_YES = 0x0002,
	/**
	 * Keyboard availability: value corresponding to the
	 * <a href="/guide/topics/resources/providing-resources.html#KeyboardAvailQualifier">keyssoft</a>
	 * resource qualifier.
	 */
	ACONFIGURATION_KEYSHIDDEN_SOFT = 0x0003,
	/** Navigation availability: not specified. */
	ACONFIGURATION_NAVHIDDEN_ANY = 0x0000,
	/**
	 * Navigation availability: value corresponding to the
	 * <a href="/guide/topics/resources/providing-resources.html#NavAvailQualifier">navexposed</a>
	 * resource qualifier.
	 */
	ACONFIGURATION_NAVHIDDEN_NO = 0x0001,
	/**
	 * Navigation availability: value corresponding to the
	 * <a href="/guide/topics/resources/providing-resources.html#NavAvailQualifier">navhidden</a>
	 * resource qualifier.
	 */
	ACONFIGURATION_NAVHIDDEN_YES = 0x0002,
	/** Screen size: not specified. */
	ACONFIGURATION_SCREENSIZE_ANY  = 0x00,
	/**
	 * Screen size: value indicating the screen is at least
	 * approximately 320x426 dp units, corresponding to the
	 * <a href="/guide/topics/resources/providing-resources.html#ScreenSizeQualifier">small</a>
	 * resource qualifier.
	 */
	ACONFIGURATION_SCREENSIZE_SMALL = 0x01,
	/**
	 * Screen size: value indicating the screen is at least
	 * approximately 320x470 dp units, corresponding to the
	 * <a href="/guide/topics/resources/providing-resources.html#ScreenSizeQualifier">normal</a>
	 * resource qualifier.
	 */
	ACONFIGURATION_SCREENSIZE_NORMAL = 0x02,
	/**
	 * Screen size: value indicating the screen is at least
	 * approximately 480x640 dp units, corresponding to the
	 * <a href="/guide/topics/resources/providing-resources.html#ScreenSizeQualifier">large</a>
	 * resource qualifier.
	 */
	ACONFIGURATION_SCREENSIZE_LARGE = 0x03,
	/**
	 * Screen size: value indicating the screen is at least
	 * approximately 720x960 dp units, corresponding to the
	 * <a href="/guide/topics/resources/providing-resources.html#ScreenSizeQualifier">xlarge</a>
	 * resource qualifier.
	 */
	ACONFIGURATION_SCREENSIZE_XLARGE = 0x04,
	/** Screen layout: not specified. */
	ACONFIGURATION_SCREENLONG_ANY = 0x00,
	/**
	 * Screen layout: value that corresponds to the
	 * <a href="/guide/topics/resources/providing-resources.html#ScreenAspectQualifier">notlong</a>
	 * resource qualifier.
	 */
	ACONFIGURATION_SCREENLONG_NO = 0x1,
	/**
	 * Screen layout: value that corresponds to the
	 * <a href="/guide/topics/resources/providing-resources.html#ScreenAspectQualifier">long</a>
	 * resource qualifier.
	 */
	ACONFIGURATION_SCREENLONG_YES = 0x2,
	ACONFIGURATION_SCREENROUND_ANY = 0x00,
	ACONFIGURATION_SCREENROUND_NO = 0x1,
	ACONFIGURATION_SCREENROUND_YES = 0x2,
	/** Wide color gamut: not specified. */
	ACONFIGURATION_WIDE_COLOR_GAMUT_ANY = 0x00,
	/**
	 * Wide color gamut: value that corresponds to
	 * <a href="/guide/topics/resources/providing-resources.html#WideColorGamutQualifier">no
	 * nowidecg</a> resource qualifier specified.
	 */
	ACONFIGURATION_WIDE_COLOR_GAMUT_NO = 0x1,
	/**
	 * Wide color gamut: value that corresponds to
	 * <a href="/guide/topics/resources/providing-resources.html#WideColorGamutQualifier">
	 * widecg</a> resource qualifier specified.
	 */
	ACONFIGURATION_WIDE_COLOR_GAMUT_YES = 0x2,
	/** HDR: not specified. */
	ACONFIGURATION_HDR_ANY = 0x00,
	/**
	 * HDR: value that corresponds to
	 * <a href="/guide/topics/resources/providing-resources.html#HDRQualifier">
	 * lowdr</a> resource qualifier specified.
	 */
	ACONFIGURATION_HDR_NO = 0x1,
	/**
	 * HDR: value that corresponds to
	 * <a href="/guide/topics/resources/providing-resources.html#HDRQualifier">
	 * highdr</a> resource qualifier specified.
	 */
	ACONFIGURATION_HDR_YES = 0x2,
	/** UI mode: not specified. */
	ACONFIGURATION_UI_MODE_TYPE_ANY = 0x00,
	/**
	 * UI mode: value that corresponds to
	 * <a href="/guide/topics/resources/providing-resources.html#UiModeQualifier">no
	 * UI mode type</a> resource qualifier specified.
	 */
	ACONFIGURATION_UI_MODE_TYPE_NORMAL = 0x01,
	/**
	 * UI mode: value that corresponds to
	 * <a href="/guide/topics/resources/providing-resources.html#UiModeQualifier">desk</a> resource qualifier specified.
	 */
	ACONFIGURATION_UI_MODE_TYPE_DESK = 0x02,
	/**
	 * UI mode: value that corresponds to
	 * <a href="/guide/topics/resources/providing-resources.html#UiModeQualifier">car</a> resource qualifier specified.
	 */
	ACONFIGURATION_UI_MODE_TYPE_CAR = 0x03,
	/**
	 * UI mode: value that corresponds to
	 * <a href="/guide/topics/resources/providing-resources.html#UiModeQualifier">television</a> resource qualifier specified.
	 */
	ACONFIGURATION_UI_MODE_TYPE_TELEVISION = 0x04,
	/**
	 * UI mode: value that corresponds to
	 * <a href="/guide/topics/resources/providing-resources.html#UiModeQualifier">appliance</a> resource qualifier specified.
	 */
	ACONFIGURATION_UI_MODE_TYPE_APPLIANCE = 0x05,
	/**
	 * UI mode: value that corresponds to
	 * <a href="/guide/topics/resources/providing-resources.html#UiModeQualifier">watch</a> resource qualifier specified.
	 */
	ACONFIGURATION_UI_MODE_TYPE_WATCH = 0x06,
	/**
	 * UI mode: value that corresponds to
	 * <a href="/guide/topics/resources/providing-resources.html#UiModeQualifier">vr</a> resource qualifier specified.
	 */
	ACONFIGURATION_UI_MODE_TYPE_VR_HEADSET = 0x07,
	/** UI night mode: not specified.*/
	ACONFIGURATION_UI_MODE_NIGHT_ANY = 0x00,
	/**
	 * UI night mode: value that corresponds to
	 * <a href="/guide/topics/resources/providing-resources.html#NightQualifier">notnight</a> resource qualifier specified.
	 */
	ACONFIGURATION_UI_MODE_NIGHT_NO = 0x1,
	/**
	 * UI night mode: value that corresponds to
	 * <a href="/guide/topics/resources/providing-resources.html#NightQualifier">night</a> resource qualifier specified.
	 */
	ACONFIGURATION_UI_MODE_NIGHT_YES = 0x2,
	/** Screen width DPI: not specified. */
	ACONFIGURATION_SCREEN_WIDTH_DP_ANY = 0x0000,
	/** Screen height DPI: not specified. */
	ACONFIGURATION_SCREEN_HEIGHT_DP_ANY = 0x0000,
	/** Smallest screen width DPI: not specified.*/
	ACONFIGURATION_SMALLEST_SCREEN_WIDTH_DP_ANY = 0x0000,
	/** Layout direction: not specified. */
	ACONFIGURATION_LAYOUTDIR_ANY  = 0x00,
	/**
	 * Layout direction: value that corresponds to
	 * <a href="/guide/topics/resources/providing-resources.html#LayoutDirectionQualifier">ldltr</a> resource qualifier specified.
	 */
	ACONFIGURATION_LAYOUTDIR_LTR  = 0x01,
	/**
	 * Layout direction: value that corresponds to
	 * <a href="/guide/topics/resources/providing-resources.html#LayoutDirectionQualifier">ldrtl</a> resource qualifier specified.
	 */
	ACONFIGURATION_LAYOUTDIR_RTL  = 0x02,
	/**
	 * Bit mask for
	 * <a href="/guide/topics/resources/providing-resources.html#MccQualifier">mcc</a>
	 * configuration.
	 */
	ACONFIGURATION_MCC = 0x0001,
	/**
	 * Bit mask for
	 * <a href="/guide/topics/resources/providing-resources.html#MccQualifier">mnc</a>
	 * configuration.
	 */
	ACONFIGURATION_MNC = 0x0002,
	/**
	 * Bit mask for
	 * <a href="/guide/topics/resources/providing-resources.html#LocaleQualifier">locale</a>
	 * configuration.
	 */
	ACONFIGURATION_LOCALE = 0x0004,
	/**
	 * Bit mask for
	 * <a href="/guide/topics/resources/providing-resources.html#TouchscreenQualifier">touchscreen</a>
	 * configuration.
	 */
	ACONFIGURATION_TOUCHSCREEN = 0x0008,
	/**
	 * Bit mask for
	 * <a href="/guide/topics/resources/providing-resources.html#ImeQualifier">keyboard</a>
	 * configuration.
	 */
	ACONFIGURATION_KEYBOARD = 0x0010,
	/**
	 * Bit mask for
	 * <a href="/guide/topics/resources/providing-resources.html#KeyboardAvailQualifier">keyboardHidden</a>
	 * configuration.
	 */
	ACONFIGURATION_KEYBOARD_HIDDEN = 0x0020,
	/**
	 * Bit mask for
	 * <a href="/guide/topics/resources/providing-resources.html#NavigationQualifier">navigation</a>
	 * configuration.
	 */
	ACONFIGURATION_NAVIGATION = 0x0040,
	/**
	 * Bit mask for
	 * <a href="/guide/topics/resources/providing-resources.html#OrientationQualifier">orientation</a>
	 * configuration.
	 */
	ACONFIGURATION_ORIENTATION = 0x0080,
	/**
	 * Bit mask for
	 * <a href="/guide/topics/resources/providing-resources.html#DensityQualifier">density</a>
	 * configuration.
	 */
	ACONFIGURATION_DENSITY = 0x0100,
	/**
	 * Bit mask for
	 * <a href="/guide/topics/resources/providing-resources.html#ScreenSizeQualifier">screen size</a>
	 * configuration.
	 */
	ACONFIGURATION_SCREEN_SIZE = 0x0200,
	/**
	 * Bit mask for
	 * <a href="/guide/topics/resources/providing-resources.html#VersionQualifier">platform version</a>
	 * configuration.
	 */
	ACONFIGURATION_VERSION = 0x0400,
	/**
	 * Bit mask for screen layout configuration.
	 */
	ACONFIGURATION_SCREEN_LAYOUT = 0x0800,
	/**
	 * Bit mask for
	 * <a href="/guide/topics/resources/providing-resources.html#UiModeQualifier">ui mode</a>
	 * configuration.
	 */
	ACONFIGURATION_UI_MODE = 0x1000,
	/**
	 * Bit mask for
	 * <a href="/guide/topics/resources/providing-resources.html#SmallestScreenWidthQualifier">smallest screen width</a>
	 * configuration.
	 */
	ACONFIGURATION_SMALLEST_SCREEN_SIZE = 0x2000,
	/**
	 * Bit mask for
	 * <a href="/guide/topics/resources/providing-resources.html#LayoutDirectionQualifier">layout direction</a>
	 * configuration.
	 */
	ACONFIGURATION_LAYOUTDIR = 0x4000,
	ACONFIGURATION_SCREEN_ROUND = 0x8000,
	/**
	 * Bit mask for
	 * <a href="/guide/topics/resources/providing-resources.html#WideColorGamutQualifier">wide color gamut</a>
	 * and <a href="/guide/topics/resources/providing-resources.html#HDRQualifier">HDR</a> configurations.
	 */
	ACONFIGURATION_COLOR_MODE = 0x10000,
	/**
	 * Bit mask for
	 * <a href="/guide/topics/resources/providing-resources.html#GrammaticalInflectionQualifier">grammatical gender</a>
	 * configuration.
	 */
	ACONFIGURATION_GRAMMATICAL_GENDER = 0x20000,
	/**
	 * Constant used to to represent MNC (Mobile Network Code) zero.
	 * 0 cannot be used, since it is used to represent an undefined MNC.
	 */
	ACONFIGURATION_MNC_ZERO = 0xffff,
	/**
	 * <a href="/guide/topics/resources/providing-resources.html#GrammaticalInflectionQualifier">Grammatical gender</a>: not specified.
	 */
	ACONFIGURATION_GRAMMATICAL_GENDER_ANY = 0,
	/**
	 * <a href="/guide/topics/resources/providing-resources.html#GrammaticalInflectionQualifier">Grammatical gender</a>: neuter.
	 */
	ACONFIGURATION_GRAMMATICAL_GENDER_NEUTER = 1,
	/**
	 * <a href="/guide/topics/resources/providing-resources.html#GrammaticalInflectionQualifier">Grammatical gender</a>: feminine.
	 */
	ACONFIGURATION_GRAMMATICAL_GENDER_FEMININE = 2,
	/**
	 * <a href="/guide/topics/resources/providing-resources.html#GrammaticalInflectionQualifier">Grammatical gender</a>: masculine.
	 */
	ACONFIGURATION_GRAMMATICAL_GENDER_MASCULINE = 3,
};

/**
 * Describes a particular resource configuration.
 */
struct ResTable_config
{
	// Number of bytes in this structure.
	uint32_t size;

	union {
		struct {
			// Mobile country code (from SIM).  0 means "any".
			uint16_t mcc;
			// Mobile network code (from SIM).  0 means "any".
			uint16_t mnc;
		};
		uint32_t imsi;
	};

	union {
		struct {
			// This field can take three different forms:
			// - \0\0 means "any".
			//
			// - Two 7 bit ascii values interpreted as ISO-639-1 language
			//   codes ('fr', 'en' etc. etc.). The high bit for both bytes is
			//   zero.
			//
			// - A single 16 bit little endian packed value representing an
			//   ISO-639-2 3 letter language code. This will be of the form:
			//
			//   {1, t, t, t, t, t, s, s, s, s, s, f, f, f, f, f}
			//
			//   bit[0, 4] = first letter of the language code
			//   bit[5, 9] = second letter of the language code
			//   bit[10, 14] = third letter of the language code.
			//   bit[15] = 1 always
			//
			// For backwards compatibility, languages that have unambiguous
			// two letter codes are represented in that format.
			//
			// The layout is always bigendian irrespective of the runtime
			// architecture.
			char language[2];

			// This field can take three different forms:
			// - \0\0 means "any".
			//
			// - Two 7 bit ascii values interpreted as 2 letter region
			//   codes ('US', 'GB' etc.). The high bit for both bytes is zero.
			//
			// - An UN M.49 3 digit region code. For simplicity, these are packed
			//   in the same manner as the language codes, though we should need
			//   only 10 bits to represent them, instead of the 15.
			//
			// The layout is always bigendian irrespective of the runtime
			// architecture.
			char country[2];
		};
		uint32_t locale;
	};

	enum {
		ORIENTATION_ANY  = ACONFIGURATION_ORIENTATION_ANY,
		ORIENTATION_PORT = ACONFIGURATION_ORIENTATION_PORT,
		ORIENTATION_LAND = ACONFIGURATION_ORIENTATION_LAND,
		ORIENTATION_SQUARE = ACONFIGURATION_ORIENTATION_SQUARE,
	};

	enum {
		TOUCHSCREEN_ANY  = ACONFIGURATION_TOUCHSCREEN_ANY,
		TOUCHSCREEN_NOTOUCH  = ACONFIGURATION_TOUCHSCREEN_NOTOUCH,
		TOUCHSCREEN_STYLUS  = ACONFIGURATION_TOUCHSCREEN_STYLUS,
		TOUCHSCREEN_FINGER  = ACONFIGURATION_TOUCHSCREEN_FINGER,
	};

	enum {
		DENSITY_DEFAULT = ACONFIGURATION_DENSITY_DEFAULT,
		DENSITY_LOW = ACONFIGURATION_DENSITY_LOW,
		DENSITY_MEDIUM = ACONFIGURATION_DENSITY_MEDIUM,
		DENSITY_TV = ACONFIGURATION_DENSITY_TV,
		DENSITY_HIGH = ACONFIGURATION_DENSITY_HIGH,
		DENSITY_XHIGH = ACONFIGURATION_DENSITY_XHIGH,
		DENSITY_XXHIGH = ACONFIGURATION_DENSITY_XXHIGH,
		DENSITY_XXXHIGH = ACONFIGURATION_DENSITY_XXXHIGH,
		DENSITY_ANY = ACONFIGURATION_DENSITY_ANY,
		DENSITY_NONE = ACONFIGURATION_DENSITY_NONE
	};

	union {
		struct {
			uint8_t orientation;
			uint8_t touchscreen;
			uint16_t density;
		};
		uint32_t screenType;
	};

	enum {
		KEYBOARD_ANY  = ACONFIGURATION_KEYBOARD_ANY,
		KEYBOARD_NOKEYS  = ACONFIGURATION_KEYBOARD_NOKEYS,
		KEYBOARD_QWERTY  = ACONFIGURATION_KEYBOARD_QWERTY,
		KEYBOARD_12KEY  = ACONFIGURATION_KEYBOARD_12KEY,
	};

	enum {
		NAVIGATION_ANY  = ACONFIGURATION_NAVIGATION_ANY,
		NAVIGATION_NONAV  = ACONFIGURATION_NAVIGATION_NONAV,
		NAVIGATION_DPAD  = ACONFIGURATION_NAVIGATION_DPAD,
		NAVIGATION_TRACKBALL  = ACONFIGURATION_NAVIGATION_TRACKBALL,
		NAVIGATION_WHEEL  = ACONFIGURATION_NAVIGATION_WHEEL,
	};

	enum {
		MASK_KEYSHIDDEN = 0x0003,
		KEYSHIDDEN_ANY = ACONFIGURATION_KEYSHIDDEN_ANY,
		KEYSHIDDEN_NO = ACONFIGURATION_KEYSHIDDEN_NO,
		KEYSHIDDEN_YES = ACONFIGURATION_KEYSHIDDEN_YES,
		KEYSHIDDEN_SOFT = ACONFIGURATION_KEYSHIDDEN_SOFT,
	};

	enum {
		MASK_NAVHIDDEN = 0x000c,
		SHIFT_NAVHIDDEN = 2,
		NAVHIDDEN_ANY = ACONFIGURATION_NAVHIDDEN_ANY << SHIFT_NAVHIDDEN,
		NAVHIDDEN_NO = ACONFIGURATION_NAVHIDDEN_NO << SHIFT_NAVHIDDEN,
		NAVHIDDEN_YES = ACONFIGURATION_NAVHIDDEN_YES << SHIFT_NAVHIDDEN,
	};

	union {
		struct {
			uint8_t keyboard;
			uint8_t navigation;
			uint8_t inputFlags;
			uint8_t inputPad0;
		};
		uint32_t input;
	};

	enum {
		SCREENWIDTH_ANY = 0
	};

	enum {
		SCREENHEIGHT_ANY = 0
	};

	union {
		struct {
			uint16_t screenWidth;
			uint16_t screenHeight;
		};
		uint32_t screenSize;
	};

	enum {
		SDKVERSION_ANY = 0
	};

	enum {
		MINORVERSION_ANY = 0
	};

	union {
		struct {
			uint16_t sdkVersion;
			// For now minorVersion must always be 0!!!  Its meaning
			// is currently undefined.
			uint16_t minorVersion;
		};
		uint32_t version;
	};

	enum {
		// screenLayout bits for screen size class.
		MASK_SCREENSIZE = 0x0f,
		SCREENSIZE_ANY = ACONFIGURATION_SCREENSIZE_ANY,
		SCREENSIZE_SMALL = ACONFIGURATION_SCREENSIZE_SMALL,
		SCREENSIZE_NORMAL = ACONFIGURATION_SCREENSIZE_NORMAL,
		SCREENSIZE_LARGE = ACONFIGURATION_SCREENSIZE_LARGE,
		SCREENSIZE_XLARGE = ACONFIGURATION_SCREENSIZE_XLARGE,

		// screenLayout bits for wide/long screen variation.
		MASK_SCREENLONG = 0x30,
		SHIFT_SCREENLONG = 4,
		SCREENLONG_ANY = ACONFIGURATION_SCREENLONG_ANY << SHIFT_SCREENLONG,
		SCREENLONG_NO = ACONFIGURATION_SCREENLONG_NO << SHIFT_SCREENLONG,
		SCREENLONG_YES = ACONFIGURATION_SCREENLONG_YES << SHIFT_SCREENLONG,

		// screenLayout bits for layout direction.
		MASK_LAYOUTDIR = 0xC0,
		SHIFT_LAYOUTDIR = 6,
		LAYOUTDIR_ANY = ACONFIGURATION_LAYOUTDIR_ANY << SHIFT_LAYOUTDIR,
		LAYOUTDIR_LTR = ACONFIGURATION_LAYOUTDIR_LTR << SHIFT_LAYOUTDIR,
		LAYOUTDIR_RTL = ACONFIGURATION_LAYOUTDIR_RTL << SHIFT_LAYOUTDIR,
	};

	enum {
		// uiMode bits for the mode type.
		MASK_UI_MODE_TYPE = 0x0f,
		UI_MODE_TYPE_ANY = ACONFIGURATION_UI_MODE_TYPE_ANY,
		UI_MODE_TYPE_NORMAL = ACONFIGURATION_UI_MODE_TYPE_NORMAL,
		UI_MODE_TYPE_DESK = ACONFIGURATION_UI_MODE_TYPE_DESK,
		UI_MODE_TYPE_CAR = ACONFIGURATION_UI_MODE_TYPE_CAR,
		UI_MODE_TYPE_TELEVISION = ACONFIGURATION_UI_MODE_TYPE_TELEVISION,
		UI_MODE_TYPE_APPLIANCE = ACONFIGURATION_UI_MODE_TYPE_APPLIANCE,
		UI_MODE_TYPE_WATCH = ACONFIGURATION_UI_MODE_TYPE_WATCH,
		UI_MODE_TYPE_VR_HEADSET = ACONFIGURATION_UI_MODE_TYPE_VR_HEADSET,

		// uiMode bits for the night switch.
		MASK_UI_MODE_NIGHT = 0x30,
		SHIFT_UI_MODE_NIGHT = 4,
		UI_MODE_NIGHT_ANY = ACONFIGURATION_UI_MODE_NIGHT_ANY << SHIFT_UI_MODE_NIGHT,
		UI_MODE_NIGHT_NO = ACONFIGURATION_UI_MODE_NIGHT_NO << SHIFT_UI_MODE_NIGHT,
		UI_MODE_NIGHT_YES = ACONFIGURATION_UI_MODE_NIGHT_YES << SHIFT_UI_MODE_NIGHT,
	};

	union {
		struct {
			uint8_t screenLayout;
			uint8_t uiMode;
			uint16_t smallestScreenWidthDp;
		};
		uint32_t screenConfig;
	};

	union {
		struct {
			uint16_t screenWidthDp;
			uint16_t screenHeightDp;
		};
		uint32_t screenSizeDp;
	};

	// The ISO-15924 short name for the script corresponding to this
	// configuration. (eg. Hant, Latn, etc.). Interpreted in conjunction with
	// the locale field.
	char localeScript[4];

	// A single BCP-47 variant subtag. Will vary in length between 4 and 8
	// chars. Interpreted in conjunction with the locale field.
	char localeVariant[8];

	enum {
		// screenLayout2 bits for round/notround.
		MASK_SCREENROUND = 0x03,
		SCREENROUND_ANY = ACONFIGURATION_SCREENROUND_ANY,
		SCREENROUND_NO = ACONFIGURATION_SCREENROUND_NO,
		SCREENROUND_YES = ACONFIGURATION_SCREENROUND_YES,
	};

	enum {
		// colorMode bits for wide-color gamut/narrow-color gamut.
		MASK_WIDE_COLOR_GAMUT = 0x03,
		WIDE_COLOR_GAMUT_ANY = ACONFIGURATION_WIDE_COLOR_GAMUT_ANY,
		WIDE_COLOR_GAMUT_NO = ACONFIGURATION_WIDE_COLOR_GAMUT_NO,
		WIDE_COLOR_GAMUT_YES = ACONFIGURATION_WIDE_COLOR_GAMUT_YES,

		// colorMode bits for HDR/LDR.
		MASK_HDR = 0x0c,
		SHIFT_COLOR_MODE_HDR = 2,
		HDR_ANY = ACONFIGURATION_HDR_ANY << SHIFT_COLOR_MODE_HDR,
		HDR_NO = ACONFIGURATION_HDR_NO << SHIFT_COLOR_MODE_HDR,
		HDR_YES = ACONFIGURATION_HDR_YES << SHIFT_COLOR_MODE_HDR,
	};

	// An extension of screenConfig.
	union {
		struct {
			uint8_t screenLayout2;      // Contains round/notround qualifier.
			uint8_t colorMode;          // Wide-gamut, HDR, etc.
			uint16_t screenConfigPad2;  // Reserved padding.
		};
		uint32_t screenConfig2;
	};

#if 0
	// If false and localeScript is set, it means that the script of the locale
	// was explicitly provided.
	//
	// If true, it means that localeScript was automatically computed.
	// localeScript may still not be set in this case, which means that we
	// tried but could not compute a script.
	bool localeScriptWasComputed;

	// The value of BCP 47 Unicode extension for key 'nu' (numbering system).
	// Varies in length from 3 to 8 chars. Zero-filled value.
	char localeNumberingSystem[8];

	void copyFromDeviceNoSwap(const ResTable_config& o);

	void copyFromDtoH(const ResTable_config& o);

	void swapHtoD();

	int compare(const ResTable_config& o) const;
	int compareLogical(const ResTable_config& o) const;

	inline bool operator<(const ResTable_config& o) const { return compare(o) < 0; }
#endif

	// Flags indicating a set of config values.  These flag constants must
	// match the corresponding ones in android.content.pm.ActivityInfo and
	// attrs_manifest.xml.
	enum {
		CONFIG_MCC = ACONFIGURATION_MCC,
		CONFIG_MNC = ACONFIGURATION_MNC,
		CONFIG_LOCALE = ACONFIGURATION_LOCALE,
		CONFIG_TOUCHSCREEN = ACONFIGURATION_TOUCHSCREEN,
		CONFIG_KEYBOARD = ACONFIGURATION_KEYBOARD,
		CONFIG_KEYBOARD_HIDDEN = ACONFIGURATION_KEYBOARD_HIDDEN,
		CONFIG_NAVIGATION = ACONFIGURATION_NAVIGATION,
		CONFIG_ORIENTATION = ACONFIGURATION_ORIENTATION,
		CONFIG_DENSITY = ACONFIGURATION_DENSITY,
		CONFIG_SCREEN_SIZE = ACONFIGURATION_SCREEN_SIZE,
		CONFIG_SMALLEST_SCREEN_SIZE = ACONFIGURATION_SMALLEST_SCREEN_SIZE,
		CONFIG_VERSION = ACONFIGURATION_VERSION,
		CONFIG_SCREEN_LAYOUT = ACONFIGURATION_SCREEN_LAYOUT,
		CONFIG_UI_MODE = ACONFIGURATION_UI_MODE,
		CONFIG_LAYOUTDIR = ACONFIGURATION_LAYOUTDIR,
		CONFIG_SCREEN_ROUND = ACONFIGURATION_SCREEN_ROUND,
		CONFIG_COLOR_MODE = ACONFIGURATION_COLOR_MODE,
	};

#if 0
	// Compare two configuration, returning CONFIG_* flags set for each value
	// that is different.
	int diff(const ResTable_config& o) const;

	// Return true if 'this' is more specific than 'o'.
	bool isMoreSpecificThan(const ResTable_config& o) const;

	// Return true if 'this' is a better match than 'o' for the 'requested'
	// configuration.  This assumes that match() has already been used to
	// remove any configurations that don't match the requested configuration
	// at all; if they are not first filtered, non-matching results can be
	// considered better than matching ones.
	// The general rule per attribute: if the request cares about an attribute
	// (it normally does), if the two (this and o) are equal it's a tie.  If
	// they are not equal then one must be generic because only generic and
	// '==requested' will pass the match() call.  So if this is not generic,
	// it wins.  If this IS generic, o wins (return false).
	bool isBetterThan(const ResTable_config& o, const ResTable_config* requested) const;

	// Return true if 'this' can be considered a match for the parameters in
	// 'settings'.
	// Note this is asymetric.  A default piece of data will match every request
	// but a request for the default should not match odd specifics
	// (ie, request with no mcc should not match a particular mcc's data)
	// settings is the requested settings
	bool match(const ResTable_config& settings) const;

	// Get the string representation of the locale component of this
	// Config. The maximum size of this representation will be
	// |RESTABLE_MAX_LOCALE_LEN| (including a terminating '\0').
	//
	// Example: en-US, en-Latn-US, en-POSIX.
	//
	// If canonicalize is set, Tagalog (tl) locales get converted
	// to Filipino (fil).
	void getBcp47Locale(char* out, bool canonicalize=false) const;

	// Append to str the resource-qualifer string representation of the
	// locale component of this Config. If the locale is only country
	// and language, it will look like en-rUS. If it has scripts and
	// variants, it will be a modified bcp47 tag: b+en+Latn+US.
	void appendDirLocale(String8& str) const;

	// Sets the values of language, region, script, variant and numbering
	// system to the well formed BCP 47 locale contained in |in|.
	// The input locale is assumed to be valid and no validation is performed.
	void setBcp47Locale(const char* in);

	inline void clearLocale() {
		locale = 0;
		localeScriptWasComputed = false;
		memset(localeScript, 0, sizeof(localeScript));
		memset(localeVariant, 0, sizeof(localeVariant));
		memset(localeNumberingSystem, 0, sizeof(localeNumberingSystem));
	}

	inline void computeScript() {
		localeDataComputeScript(localeScript, language, country);
	}

	// Get the 2 or 3 letter language code of this configuration. Trailing
	// bytes are set to '\0'.
	size_t unpackLanguage(char language[4]) const;
	// Get the 2 or 3 letter language code of this configuration. Trailing
	// bytes are set to '\0'.
	size_t unpackRegion(char region[4]) const;

	// Sets the language code of this configuration to the first three
	// chars at |language|.
	//
	// If |language| is a 2 letter code, the trailing byte must be '\0' or
	// the BCP-47 separator '-'.
	void packLanguage(const char* language);
	// Sets the region code of this configuration to the first three bytes
	// at |region|. If |region| is a 2 letter code, the trailing byte must be '\0'
	// or the BCP-47 separator '-'.
	void packRegion(const char* region);

	// Returns a positive integer if this config is more specific than |o|
	// with respect to their locales, a negative integer if |o| is more specific
	// and 0 if they're equally specific.
	int isLocaleMoreSpecificThan(const ResTable_config &o) const;

	// Returns an integer representng the imporance score of the configuration locale.
	int getImportanceScoreOfLocale() const;

	// Return true if 'this' is a better locale match than 'o' for the
	// 'requested' configuration. Similar to isBetterThan(), this assumes that
	// match() has already been used to remove any configurations that don't
	// match the requested configuration at all.
	bool isLocaleBetterThan(const ResTable_config& o, const ResTable_config* requested) const;

	String8 toString() const;
#endif
};

/**
 * A specification of the resources defined by a particular type.
 *
 * There should be one of these chunks for each resource type.
 *
 * This structure is followed by an array of integers providing the set of
 * configuration change flags (ResTable_config::CONFIG_*) that have multiple
 * resources for that configuration.  In addition, the high bit is set if that
 * resource has been made public.
 */
struct ResTable_typeSpec
{
	struct ResChunk_header header;

	// The type identifier this chunk is holding.  Type IDs start
	// at 1 (corresponding to the value of the type bits in a
	// resource identifier).  0 is invalid.
	uint8_t id;

	// Must be 0.
	uint8_t res0;
	// Must be 0.
	uint16_t res1;

	// Number of uint32_t entry configuration masks that follow.
	uint32_t entryCount;

	enum : uint32_t {
		// Additional flag indicating an entry is public.
		SPEC_PUBLIC = 0x40000000u,

		// Additional flag indicating the resource id for this resource may change in a future
		// build. If this flag is set, the SPEC_PUBLIC flag is also set since the resource must be
		// public to be exposed as an API to other applications.
		SPEC_STAGED_API = 0x20000000u,
	};
};

/**
 * A collection of resource entries for a particular resource data
 * type.
 *
 * If the flag FLAG_SPARSE is not set in `flags`, then this struct is
 * followed by an array of uint32_t defining the resource
 * values, corresponding to the array of type strings in the
 * ResTable_package::typeStrings string block. Each of these hold an
 * index from entriesStart; a value of NO_ENTRY means that entry is
 * not defined.
 *
 * If the flag FLAG_SPARSE is set in `flags`, then this struct is followed
 * by an array of ResTable_sparseTypeEntry defining only the entries that
 * have values for this type. Each entry is sorted by their entry ID such
 * that a binary search can be performed over the entries. The ID and offset
 * are encoded in a uint32_t. See ResTabe_sparseTypeEntry.
 *
 * There may be multiple of these chunks for a particular resource type,
 * supply different configuration variations for the resource values of
 * that type.
 *
 * It would be nice to have an additional ordered index of entries, so
 * we can do a binary search if trying to find a resource by string name.
 */
struct ResTable_type
{
	struct ResChunk_header header;

	enum {
		NO_ENTRY = 0xFFFFFFFF
	};

	// The type identifier this chunk is holding.  Type IDs start
	// at 1 (corresponding to the value of the type bits in a
	// resource identifier).  0 is invalid.
	uint8_t id;

	enum {
		// If set, the entry is sparse, and encodes both the entry ID and offset into each entry,
		// and a binary search is used to find the key. Only available on platforms >= O.
		// Mark any types that use this with a v26 qualifier to prevent runtime issues on older
		// platforms.
		FLAG_SPARSE = 0x01,
	};
	uint8_t flags;

	// Must be 0.
	uint16_t reserved;

	// Number of uint32_t entry indices that follow.
	uint32_t entryCount;

	// Offset from header where ResTable_entry data starts.
	uint32_t entriesStart;

	// Configuration this collection of entries is designed for. This must always be last.
	ResTable_config config;
};

/**
 * Definition for a pool of strings.  The data of this chunk is an
 * array of uint32_t providing indices into the pool, relative to
 * stringsStart.  At stringsStart are all of the UTF-16 strings
 * concatenated together; each starts with a uint16_t of the string's
 * length and each ends with a 0x0000 terminator.  If a string is >
 * 32767 characters, the high bit of the length is set meaning to take
 * those 15 bits as a high word and it will be followed by another
 * uint16_t containing the low word.
 *
 * If styleCount is not zero, then immediately following the array of
 * uint32_t indices into the string table is another array of indices
 * into a style table starting at stylesStart.  Each entry in the
 * style table is an array of ResStringPool_span structures.
 */
struct ResStringPool_header
{
	struct ResChunk_header header;

	// Number of strings in this pool (number of uint32_t indices that follow
	// in the data).
	uint32_t stringCount;

	// Number of style span arrays in the pool (number of uint32_t indices
	// follow the string indices).
	uint32_t styleCount;

	// Flags.
	enum {
		// If set, the string index is sorted by the string values (based
		// on strcmp16()).
		SORTED_FLAG = 1<<0,

		// String pool is encoded in UTF-8
		UTF8_FLAG = 1<<8
	};
	uint32_t flags;

	// Index from header of the string data.
	uint32_t stringsStart;

	// Index from header of the style data.
	uint32_t stylesStart;
};

/**
 * An entry in a ResTable_type with the flag `FLAG_SPARSE` set.
 */
union ResTable_sparseTypeEntry {
	// Holds the raw uint32_t encoded value. Do not read this.
	uint32_t entry;
	struct {
		// The index of the entry.
		uint16_t idx;

		// The offset from ResTable_type::entriesStart, divided by 4.
		uint16_t offset;
	};
};

struct ResTable_entry
{
	// Number of bytes in this structure.
	uint16_t size;

	enum {
		// If set, this is a complex entry, holding a set of name/value
		// mappings.  It is followed by an array of ResTable_map structures.
		FLAG_COMPLEX = 0x0001,
		// If set, this resource has been declared public, so libraries
		// are allowed to reference it.
		FLAG_PUBLIC = 0x0002,
		// If set, this is a weak resource and may be overriden by strong
		// resources of the same name/type. This is only useful during
		// linking with other resource tables.
		FLAG_WEAK = 0x0004,
	};
	uint16_t flags;

	// Reference into ResTable_package::keyStrings identifying this entry.
	struct ResStringPool_ref key;
};

#ifdef __cplusplus
}
#endif
