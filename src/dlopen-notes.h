/***************************************************************************
 * ROM Properties Page shell extension.                                    *
 * dlopen-notes.h: dlopen() notes helper.                                  *
 *                                                                         *
 * SPDX-License-Identifier: CC0-1.0                                        *
 ***************************************************************************/

// Reference: https://github.com/systemd/package-notes/blob/main/test/notes.c

#pragma once

#ifdef __ELF__

#include <stdint.h>

#define XCONCATENATE(x, y) x ## y
#define CONCATENATE(x, y) XCONCATENATE(x, y)
#define UNIQ_T(x, uniq) CONCATENATE(__unique_prefix_, CONCATENATE(x, uniq))
#define UNIQ __COUNTER__

#define ELF_NOTE_DLOPEN_VENDOR "FDO"
#define ELF_NOTE_DLOPEN_TYPE 0x407c0c0a

#define _ELF_NOTE_DLOPEN(module_, variable_name)                        \
	__attribute__((used, section(".note.dlopen"))) _Alignas(sizeof(uint32_t)) static const struct { \
		struct {                                                \
			uint32_t n_namesz, n_descsz, n_type;            \
		} nhdr;                                                 \
		char name[sizeof(ELF_NOTE_DLOPEN_VENDOR)];              \
		_Alignas(sizeof(uint32_t)) char dlopen_module[sizeof(module_)]; \
	} variable_name = {                                             \
		.nhdr = {                                               \
			.n_namesz = sizeof(ELF_NOTE_DLOPEN_VENDOR),     \
			.n_descsz = sizeof(module_),                     \
			.n_type   = ELF_NOTE_DLOPEN_TYPE,               \
		},                                                      \
		.name = ELF_NOTE_DLOPEN_VENDOR,                         \
		.dlopen_module = module_,                               \
	}

#define _SONAME_ARRAY1(a) "[\"" a "\"]"
#define _SONAME_ARRAY2(a, b) "[\"" a "\",\"" b "\"]"
#define _SONAME_ARRAY3(a, b, c) "[\"" a "\",\"" b "\",\"" c "\"]"
#define _SONAME_ARRAY4(a, b, c, d) "[\"" a "\",\"" b "\",\"" c "\"",\"" d "\"]"
#define _SONAME_ARRAY5(a, b, c, d, e) "[\"" a "\",\"" b "\",\"" c "\"",\"" d "\",\"" e "\"]"
#define _SONAME_ARRAY_GET(_1,_2,_3,_4,_5,NAME,...) NAME
#define _SONAME_ARRAY(...) _SONAME_ARRAY_GET(__VA_ARGS__, _SONAME_ARRAY5, _SONAME_ARRAY4, _SONAME_ARRAY3, _SONAME_ARRAY2, _SONAME_ARRAY1)(__VA_ARGS__)

#define ELF_NOTE_DLOPEN_VA(feature, description, priority, ...) \
	_ELF_NOTE_DLOPEN("[{\"feature\":\"" feature "\",\"description\":\"" description "\",\"priority\":\"" priority "\",\"soname\":" _SONAME_ARRAY(__VA_ARGS__) "}]", UNIQ_T(s, UNIQ))


#define _ELF_NOTE_DLOPEN_INT(feature, description, priority, module_) \
        "{\"feature\":\"" feature "\",\"description\":\"" description "\",\"priority\":\"" priority "\",\"soname\":[\"" module_ "\"]}"

#else /* !__ELF__ */

// Non-ELF platform. Dummy out the macros.
#define _ELF_NOTE_DLOPEN(module_, variable_name)
#define ELF_NOTE_DLOPEN_VA(feature, description, priority, ...)
#define _ELF_NOTE_DLOPEN_INT(feature, description, priority, module_)

#endif /* __ELF__ */

#define ELF_NOTE_DLOPEN(var, feature0, description0, priority0, module0) \
	_ELF_NOTE_DLOPEN("[" _ELF_NOTE_DLOPEN_INT(feature0, description0, priority0, module0) "]", var)

#define ELF_NOTE_DLOPEN2(var, feature0, description0, priority0, module0, feature1, description1, priority1, module1) \
	_ELF_NOTE_DLOPEN("[" _ELF_NOTE_DLOPEN_INT(feature0, description0, priority0, module0) "," \
	                     _ELF_NOTE_DLOPEN_INT(feature1, description1, priority1, module1) "]", var)

#define ELF_NOTE_DLOPEN3(var, feature0, description0, priority0, module0, feature1, description1, priority1, module1, feature2, description2, priority2, module2) \
	_ELF_NOTE_DLOPEN("[" _ELF_NOTE_DLOPEN_INT(feature0, description0, priority0, module0) "," \
	                     _ELF_NOTE_DLOPEN_INT(feature1, description1, priority1, module1) "," \
	                     _ELF_NOTE_DLOPEN_INT(feature2, description2, priority2, module2) "]", var)

#define ELF_NOTE_DLOPEN4(var, feature0, description0, priority0, module0, feature1, description1, priority1, module1, feature2, description2, priority2, module2, feature3, description3, priority3, module3) \
	_ELF_NOTE_DLOPEN("[" _ELF_NOTE_DLOPEN_INT(feature0, description0, priority0, module0) "," \
	                     _ELF_NOTE_DLOPEN_INT(feature1, description1, priority1, module1) "," \
	                     _ELF_NOTE_DLOPEN_INT(feature2, description2, priority2, module2) "," \
	                     _ELF_NOTE_DLOPEN_INT(feature3, description3, priority3, module3) "]", var)

#define ELF_NOTE_DLOPEN5(var, feature0, description0, priority0, module0, feature1, description1, priority1, module1, feature2, description2, priority2, module2, feature3, description3, priority3, module3, feature4, description4, priority4, module4) \
	_ELF_NOTE_DLOPEN("[" _ELF_NOTE_DLOPEN_INT(feature0, description0, priority0, module0) "," \
	                     _ELF_NOTE_DLOPEN_INT(feature1, description1, priority1, module1) "," \
	                     _ELF_NOTE_DLOPEN_INT(feature2, description2, priority2, module2) "," \
	                     _ELF_NOTE_DLOPEN_INT(feature3, description3, priority3, module3) "," \
	                     _ELF_NOTE_DLOPEN_INT(feature4, description4, priority4, module4) "]", var)

#define ELF_NOTE_DLOPEN6(var, feature0, description0, priority0, module0, feature1, description1, priority1, module1, feature2, description2, priority2, module2, feature3, description3, priority3, module3, feature4, description4, priority4, module4, feature5, description5, priority5, module5) \
	_ELF_NOTE_DLOPEN("[" _ELF_NOTE_DLOPEN_INT(feature0, description0, priority0, module0) "," \
	                     _ELF_NOTE_DLOPEN_INT(feature1, description1, priority1, module1) "," \
	                     _ELF_NOTE_DLOPEN_INT(feature2, description2, priority2, module2) "," \
	                     _ELF_NOTE_DLOPEN_INT(feature3, description3, priority3, module3) "," \
	                     _ELF_NOTE_DLOPEN_INT(feature4, description4, priority4, module4) "," \
	                     _ELF_NOTE_DLOPEN_INT(feature5, description5, priority5, module5) "]", var)
