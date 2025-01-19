# uniwidth: Unicode character width subset of gnulib.
PROJECT(uniwidth LANGUAGES C)

# rom-properties: Hide symbols by default, since we don't want them
# leaking from the static library to the plugins.
CMAKE_POLICY(SET CMP0063 NEW)
SET(CMAKE_C_VISIBILITY_PRESET "hidden")
SET(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

# Sources
SET(uniwidth_SRCS width.c)
SET(uniwidth_H bitmap.h cjk.h unitypes.h uniwidth.h width0.h width2.h)

######################
# Build the library. #
######################

ADD_LIBRARY(uniwidth STATIC ${uniwidth_SRCS})
TARGET_INCLUDE_DIRECTORIES(uniwidth
	INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
	)
