/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GLTypeStrings.hpp: OpenGL string tables.                                *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#include "GLTypeStrings.hpp"
#include "../ktx_structs.h"

// C includes.
#include <stdlib.h>

namespace LibRomData {

class GLTypeStringsPrivate
{
	private:
		// Static class.
		GLTypeStringsPrivate();
		~GLTypeStringsPrivate();
		RP_DISABLE_COPY(GLTypeStringsPrivate)

	public:
		// String tables.
		// NOTE: Must be sorted by id.
		// NOTE: Leaving the "GL_" prefix off of the strings.
		#define STRTBL_ENTRY(x) {GL_##x, #x}
		struct StrTbl {
			unsigned int id;
			const char *str;
		};

		/**
		* Comparison function for bsearch().
		* @param a
		* @param b
		* @return
		*/
		static int RP_C_API compar(const void *a, const void *b);

		/**
		 * glType
		 *
		 * See the OpenGL 4.4 specification, table 8.2:
		 * - https://www.khronos.org/registry/OpenGL/specs/gl/glspec44.core.pdf#nameddest=table-8.2
		 */
		static const StrTbl glType_tbl[];

		/**
		 * glFormat
		 *
		 * See the OpenGL 4.4 specification, table 8.3:
		 * - https://www.khronos.org/registry/OpenGL/specs/gl/glspec44.core.pdf#nameddest=table-8.3
		 */
		static const StrTbl glFormat_tbl[];

		/**
		 * glInternalFormat
		 *
		 * For uncompressed textures, see the OpenGL 4.4 specification, tables 8.12 and 8.13:
		 * - https://www.khronos.org/registry/OpenGL/specs/gl/glspec44.core.pdf#nameddest=table-8.14
		 *
		 * For compressed textures, see the OpenGL 4.4 specification, table 8.14:
		 * - https://www.khronos.org/registry/OpenGL/specs/gl/glspec44.core.pdf#nameddest=table-8.14
		 */
		static const StrTbl glInternalFormat_tbl[];

		/**
		 * glBaseInternalFormat
		 *
		 * See the OpenGL 4.4 specification, table 8.11:
		 * - https://www.khronos.org/registry/OpenGL/specs/gl/glspec44.core.pdf#nameddest=table-8.11
		 */
		static const StrTbl glBaseInternalFormat_tbl[];
};

/** GLTypeStringsPrivate **/

/**
 * Comparison function for bsearch().
 * @param a
 * @param b
 * @return
 */
int RP_C_API GLTypeStringsPrivate::compar(const void *a, const void *b)
{
	unsigned int id1 = static_cast<const StrTbl*>(a)->id;
	unsigned int id2 = static_cast<const StrTbl*>(b)->id;
	if (id1 < id2) return -1;
	if (id1 > id2) return 1;
	return 0;
}

/**
 * glType
 *
 * See the OpenGL 4.4 specification, table 8.2:
 * - https://www.khronos.org/registry/OpenGL/specs/gl/glspec44.core.pdf#nameddest=table-8.2
 */
const GLTypeStringsPrivate::StrTbl GLTypeStringsPrivate::glType_tbl[] = {
	STRTBL_ENTRY(BYTE),
	STRTBL_ENTRY(UNSIGNED_BYTE),
	STRTBL_ENTRY(SHORT),
	STRTBL_ENTRY(UNSIGNED_SHORT),
	STRTBL_ENTRY(INT),
	STRTBL_ENTRY(UNSIGNED_INT),
	STRTBL_ENTRY(FLOAT),
	STRTBL_ENTRY(HALF_FLOAT),
	STRTBL_ENTRY(UNSIGNED_BYTE_3_3_2),
	STRTBL_ENTRY(UNSIGNED_SHORT_4_4_4_4),
	STRTBL_ENTRY(UNSIGNED_SHORT_5_5_5_1),
	STRTBL_ENTRY(UNSIGNED_INT_8_8_8_8),
	STRTBL_ENTRY(UNSIGNED_INT_10_10_10_2),
	STRTBL_ENTRY(UNSIGNED_BYTE_2_3_3_REV),
	STRTBL_ENTRY(UNSIGNED_SHORT_5_6_5),
	STRTBL_ENTRY(UNSIGNED_SHORT_5_6_5_REV),
	STRTBL_ENTRY(UNSIGNED_SHORT_4_4_4_4_REV),
	STRTBL_ENTRY(UNSIGNED_SHORT_1_5_5_5_REV),
	STRTBL_ENTRY(UNSIGNED_INT_8_8_8_8_REV),
	STRTBL_ENTRY(UNSIGNED_INT_2_10_10_10_REV),
	STRTBL_ENTRY(UNSIGNED_INT_24_8),
	STRTBL_ENTRY(UNSIGNED_INT_10F_11F_11F_REV),
	STRTBL_ENTRY(UNSIGNED_INT_5_9_9_9_REV),
	STRTBL_ENTRY(FLOAT_32_UNSIGNED_INT_24_8_REV),

	{0, nullptr}
};

/**
 * glFormat
 *
 * See the OpenGL 4.4 specification, table 8.3:
 * - https://www.khronos.org/registry/OpenGL/specs/gl/glspec44.core.pdf#nameddest=table-8.3
 */
const GLTypeStringsPrivate::StrTbl GLTypeStringsPrivate::glFormat_tbl[] = {
	STRTBL_ENTRY(STENCIL_INDEX),
	STRTBL_ENTRY(DEPTH_COMPONENT),
	STRTBL_ENTRY(RED),
	STRTBL_ENTRY(GREEN),
	STRTBL_ENTRY(BLUE),
	STRTBL_ENTRY(RGB),
	STRTBL_ENTRY(RGBA),
	STRTBL_ENTRY(BGR),
	STRTBL_ENTRY(BGRA),
	STRTBL_ENTRY(RG),
	STRTBL_ENTRY(RG_INTEGER),
	STRTBL_ENTRY(DEPTH_STENCIL),
	STRTBL_ENTRY(RED_INTEGER),
	STRTBL_ENTRY(GREEN_INTEGER),
	STRTBL_ENTRY(BLUE_INTEGER),
	STRTBL_ENTRY(RGB_INTEGER),
	STRTBL_ENTRY(RGBA_INTEGER),
	STRTBL_ENTRY(BGR_INTEGER),
	STRTBL_ENTRY(BGRA_INTEGER),

	{0, nullptr}
};

/**
 * glInternalFormat
 *
 * For uncompressed textures, see the OpenGL 4.4 specification, tables 8.12 and 8.13:
 * - https://www.khronos.org/registry/OpenGL/specs/gl/glspec44.core.pdf#nameddest=table-8.14
 *
 * For compressed textures, see the OpenGL 4.4 specification, table 8.14:
 * - https://www.khronos.org/registry/OpenGL/specs/gl/glspec44.core.pdf#nameddest=table-8.14
 */
const GLTypeStringsPrivate::StrTbl GLTypeStringsPrivate::glInternalFormat_tbl[] = {
	STRTBL_ENTRY(R3_G3_B2),
	STRTBL_ENTRY(RGB4),
	STRTBL_ENTRY(RGB5),
	STRTBL_ENTRY(RGB8),
	STRTBL_ENTRY(RGB10),
	STRTBL_ENTRY(RGB12),
	STRTBL_ENTRY(RGB16),
	STRTBL_ENTRY(RGBA2),
	STRTBL_ENTRY(RGBA4),
	STRTBL_ENTRY(RGB5_A1),
	STRTBL_ENTRY(RGBA8),
	STRTBL_ENTRY(RGB10_A2),
	STRTBL_ENTRY(RGBA12),
	STRTBL_ENTRY(RGBA16),
	STRTBL_ENTRY(DEPTH_COMPONENT16),
	STRTBL_ENTRY(DEPTH_COMPONENT24),
	STRTBL_ENTRY(DEPTH_COMPONENT32),
	STRTBL_ENTRY(COMPRESSED_RED),
	STRTBL_ENTRY(COMPRESSED_RG),
	STRTBL_ENTRY(R8),
	STRTBL_ENTRY(R16),
	STRTBL_ENTRY(RG8),
	STRTBL_ENTRY(RG16),
	STRTBL_ENTRY(R16F),
	STRTBL_ENTRY(R32F),
	STRTBL_ENTRY(RG16F),
	STRTBL_ENTRY(RG32F),
	STRTBL_ENTRY(R8I),
	STRTBL_ENTRY(R8UI),
	STRTBL_ENTRY(R16I),
	STRTBL_ENTRY(R16UI),
	STRTBL_ENTRY(R32I),
	STRTBL_ENTRY(R32UI),
	STRTBL_ENTRY(RG8I),
	STRTBL_ENTRY(RG8UI),
	STRTBL_ENTRY(RG16I),
	STRTBL_ENTRY(RG16UI),
	STRTBL_ENTRY(RG32I),
	STRTBL_ENTRY(RG32UI),
	STRTBL_ENTRY(COMPRESSED_RGB),
	STRTBL_ENTRY(COMPRESSED_RGBA),
	STRTBL_ENTRY(RGBA32F),
	STRTBL_ENTRY(RGB32F),
	STRTBL_ENTRY(RGBA16F),
	STRTBL_ENTRY(RGB16F),
	STRTBL_ENTRY(DEPTH24_STENCIL8),
	STRTBL_ENTRY(R11F_G11F_B10F),
	STRTBL_ENTRY(RGB9_E5),
	STRTBL_ENTRY(SRGB8),
	STRTBL_ENTRY(SRGB8_ALPHA8),
	STRTBL_ENTRY(COMPRESSED_SRGB),
	STRTBL_ENTRY(COMPRESSED_SRGB_ALPHA),
	STRTBL_ENTRY(DEPTH_COMPONENT32F),
	STRTBL_ENTRY(DEPTH32F_STENCIL8),
	STRTBL_ENTRY(STENCIL_INDEX1),
	STRTBL_ENTRY(STENCIL_INDEX4),
	STRTBL_ENTRY(STENCIL_INDEX8),
	STRTBL_ENTRY(STENCIL_INDEX16),
	STRTBL_ENTRY(RGB565),
	STRTBL_ENTRY(ETC1_RGB8_OES),	// ETC1
	STRTBL_ENTRY(RGBA32UI),
	STRTBL_ENTRY(RGB32UI),
	STRTBL_ENTRY(RGBA16UI),
	STRTBL_ENTRY(RGB16UI),
	STRTBL_ENTRY(RGBA8UI),
	STRTBL_ENTRY(RGB8UI),
	STRTBL_ENTRY(RGBA32I),
	STRTBL_ENTRY(RGB32I),
	STRTBL_ENTRY(RGBA16I),
	STRTBL_ENTRY(RGB16I),
	STRTBL_ENTRY(RGBA8I),
	STRTBL_ENTRY(RGB8I),
	STRTBL_ENTRY(COMPRESSED_RED_RGTC1),
	STRTBL_ENTRY(COMPRESSED_SIGNED_RED_RGTC1),
	STRTBL_ENTRY(COMPRESSED_RG_RGTC2),
	STRTBL_ENTRY(COMPRESSED_SIGNED_RG_RGTC2),
	STRTBL_ENTRY(COMPRESSED_RGBA_BPTC_UNORM),
	STRTBL_ENTRY(COMPRESSED_SRGB_ALPHA_BPTC_UNORM),
	STRTBL_ENTRY(COMPRESSED_RGB_BPTC_SIGNED_FLOAT),
	STRTBL_ENTRY(COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT),
	STRTBL_ENTRY(R8_SNORM),
	STRTBL_ENTRY(RG8_SNORM),
	STRTBL_ENTRY(RGB8_SNORM),
	STRTBL_ENTRY(RGBA8_SNORM),
	STRTBL_ENTRY(R16_SNORM),
	STRTBL_ENTRY(RG16_SNORM),
	STRTBL_ENTRY(RGB16_SNORM),
	STRTBL_ENTRY(RGBA16_SNORM),
	STRTBL_ENTRY(RGB10_A2UI),
	STRTBL_ENTRY(COMPRESSED_R11_EAC),
	STRTBL_ENTRY(COMPRESSED_SIGNED_R11_EAC),
	STRTBL_ENTRY(COMPRESSED_RG11_EAC),
	STRTBL_ENTRY(COMPRESSED_SIGNED_RG11_EAC),
	STRTBL_ENTRY(COMPRESSED_RGB8_ETC2),
	STRTBL_ENTRY(COMPRESSED_SRGB8_ETC2),
	STRTBL_ENTRY(COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2),
	STRTBL_ENTRY(COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2),
	STRTBL_ENTRY(COMPRESSED_RGBA8_ETC2_EAC),
	STRTBL_ENTRY(COMPRESSED_SRGB8_ALPHA8_ETC2_EAC),

	// GL_KHR_texture_compression_astc_hdr
	// GL_KHR_texture_compression_astc_ldr
	STRTBL_ENTRY(COMPRESSED_RGBA_ASTC_4x4_KHR),
	STRTBL_ENTRY(COMPRESSED_RGBA_ASTC_5x4_KHR),
	STRTBL_ENTRY(COMPRESSED_RGBA_ASTC_5x5_KHR),
	STRTBL_ENTRY(COMPRESSED_RGBA_ASTC_6x5_KHR),
	STRTBL_ENTRY(COMPRESSED_RGBA_ASTC_6x6_KHR),
	STRTBL_ENTRY(COMPRESSED_RGBA_ASTC_8x5_KHR),
	STRTBL_ENTRY(COMPRESSED_RGBA_ASTC_8x6_KHR),
	STRTBL_ENTRY(COMPRESSED_RGBA_ASTC_8x8_KHR),
	STRTBL_ENTRY(COMPRESSED_RGBA_ASTC_10x5_KHR),
	STRTBL_ENTRY(COMPRESSED_RGBA_ASTC_10x6_KHR),
	STRTBL_ENTRY(COMPRESSED_RGBA_ASTC_10x8_KHR),
	STRTBL_ENTRY(COMPRESSED_RGBA_ASTC_10x10_KHR),
	STRTBL_ENTRY(COMPRESSED_RGBA_ASTC_12x10_KHR),
	STRTBL_ENTRY(COMPRESSED_RGBA_ASTC_12x12_KHR),
	STRTBL_ENTRY(COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR),
	STRTBL_ENTRY(COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR),
	STRTBL_ENTRY(COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR),
	STRTBL_ENTRY(COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR),
	STRTBL_ENTRY(COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR),
	STRTBL_ENTRY(COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR),
	STRTBL_ENTRY(COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR),
	STRTBL_ENTRY(COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR),
	STRTBL_ENTRY(COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR),
	STRTBL_ENTRY(COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR),
	STRTBL_ENTRY(COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR),
	STRTBL_ENTRY(COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR),
	STRTBL_ENTRY(COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR),
	STRTBL_ENTRY(COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR),

	{0, nullptr}
};

/**
 * glBaseInternalFormat
 *
 * See the OpenGL 4.4 specification, table 8.11:
 * - https://www.khronos.org/registry/OpenGL/specs/gl/glspec44.core.pdf#nameddest=table-8.11
 */
const GLTypeStringsPrivate::StrTbl GLTypeStringsPrivate::glBaseInternalFormat_tbl[] = {
	STRTBL_ENTRY(STENCIL_INDEX),
	STRTBL_ENTRY(DEPTH_COMPONENT),
	STRTBL_ENTRY(RED),
	STRTBL_ENTRY(RGB),
	STRTBL_ENTRY(RGBA),
	STRTBL_ENTRY(RG),
	STRTBL_ENTRY(DEPTH_STENCIL),

	{0, nullptr}
};

/** GLTypeStrings **/

/**
 * Look up an OpenGL glType string.
 * @param glType	[in] glType
 * @return String, or nullptr if not found.
 */
const char *GLTypeStrings::lookup_glType(unsigned int glType)
{
	// Do a binary search.
	const GLTypeStringsPrivate::StrTbl key = {glType, nullptr};
	const GLTypeStringsPrivate::StrTbl *res =
		static_cast<const GLTypeStringsPrivate::StrTbl*>(bsearch(&key,
			GLTypeStringsPrivate::glType_tbl,
			ARRAY_SIZE(GLTypeStringsPrivate::glType_tbl)-1,
			sizeof(GLTypeStringsPrivate::StrTbl),
			GLTypeStringsPrivate::compar));

	return (res ? res->str : 0);
}

/**
 * Look up an OpenGL glFormat string.
 * @param glFormat	[in] glFormat
 * @return String, or nullptr if not found.
 */
const char *GLTypeStrings::lookup_glFormat(unsigned int glFormat)
{
	// Do a binary search.
	const GLTypeStringsPrivate::StrTbl key = {glFormat, nullptr};
	const GLTypeStringsPrivate::StrTbl *res =
		static_cast<const GLTypeStringsPrivate::StrTbl*>(bsearch(&key,
			GLTypeStringsPrivate::glFormat_tbl,
			ARRAY_SIZE(GLTypeStringsPrivate::glFormat_tbl)-1,
			sizeof(GLTypeStringsPrivate::StrTbl),
			GLTypeStringsPrivate::compar));

	return (res ? res->str : 0);
}

/**
 * Look up an OpenGL glInternalFormat string.
 * @param glInternalFormat	[in] glInternalFormat
 * @return String, or nullptr if not found.
 */
const char *GLTypeStrings::lookup_glInternalFormat(unsigned int glInternalFormat)
{
	// Do a binary search.
	const GLTypeStringsPrivate::StrTbl key = {glInternalFormat, nullptr};
	const GLTypeStringsPrivate::StrTbl *res =
		static_cast<const GLTypeStringsPrivate::StrTbl*>(bsearch(&key,
			GLTypeStringsPrivate::glInternalFormat_tbl,
			ARRAY_SIZE(GLTypeStringsPrivate::glInternalFormat_tbl)-1,
			sizeof(GLTypeStringsPrivate::StrTbl),
			GLTypeStringsPrivate::compar));

	return (res ? res->str : 0);
}

/**
 * Look up an OpenGL glBaseInternalFormat string.
 * @param glBaseInternalFormat	[in] glBaseInternalFormat
 * @return String, or nullptr if not found.
 */
const char *GLTypeStrings::lookup_glBaseInternalFormat(unsigned int glBaseInternalFormat)
{
	// Do a binary search.
	const GLTypeStringsPrivate::StrTbl key = {glBaseInternalFormat, nullptr};
	const GLTypeStringsPrivate::StrTbl *res =
		static_cast<const GLTypeStringsPrivate::StrTbl*>(bsearch(&key,
			GLTypeStringsPrivate::glBaseInternalFormat_tbl,
			ARRAY_SIZE(GLTypeStringsPrivate::glBaseInternalFormat_tbl)-1,
			sizeof(GLTypeStringsPrivate::StrTbl),
			GLTypeStringsPrivate::compar));

	return (res ? res->str : 0);
}

}
