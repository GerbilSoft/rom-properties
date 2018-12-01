#!/bin/sh

### glib-mkenums script
###
### We can't easily integrate glib-mkenums into the build system, since
### the root GTK+ directory doesn't actually build any targets.
###
### This script will be run whenever enums are updated, and the resulting
### files will be committed to git.
###
### References:
### - https://arosenfeld.wordpress.com/2010/08/11/glib-mkenums/
### - https://github.com/Kurento/kms-cmake-utils/blob/master/CMake/FindGLIB-MKENUMS.cmake
### - https://github.com/Kurento/kms-cmake-utils/blob/master/CMake/GLibHelpers.cmake
### - https://developer.gnome.org/gobject/stable/glib-mkenums.html

includeguard=ROMPROPERTIES_GTK
name=rp-gtk-enums
name_upper=RP_GTK_ENUMS
shortname=RP

set -ev

# Header
glib-mkenums \
	--fhead "#ifndef __${includeguard}_${name_upper}_ENUM_TYPES_H__\n#define __${includeguard}_${name_upper}_ENUM_TYPES_H__\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
	--fprod "\n/* enumerations from \"@filename@\" */\n" \
	--vhead "GType @enum_name@_get_type (void);\n#define ${shortname}_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
	--ftail "\nG_END_DECLS\n\n#endif /* __${includeguard}_${name_upper}_ENUM_TYPES_H__ */" \
	*.hpp > "${name}.h"

# Source
glib-mkenums \
	--fhead "#include \"${name}.h\"" \
	--fprod "\n/* enumerations from \"@filename@\" */\n#include \"@filename@\"" \
	--vhead "GType\n@enum_name@_get_type (void)\n{\n  static volatile gsize g_define_type_id__volatile = 0;\n  if (g_once_init_enter (&g_define_type_id__volatile)) {\n    static const G@Type@Value values[] = {" \
	--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
	--vtail "      { 0, NULL, NULL }\n    };\n    GType g_define_type_id = g_@type@_register_static (\"@EnumName@\", values);\n    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);\n  }\n  return g_define_type_id__volatile;\n}\n" \
	*.hpp > "${name}.c"
