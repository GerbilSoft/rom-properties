# zstd seekable_format
# Custom CMakeLists.txt file for rom-properties.
PROJECT(zstd_seekable_format LANGUAGES C)

# Set CMake hidden visibility options.
SET(CMAKE_C_VISIBILITY_PRESET "hidden")
SET(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

ADD_LIBRARY(zstd_seekable_format STATIC
	zstd_seekable.h
	zstdseek_compress.c
	zstdseek_decompress.c
	../../lib/common/xxhash.c
	../../lib/common/xxhash.h
	)
TARGET_INCLUDE_DIRECTORIES(zstd_seekable_format
	PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	PRIVATE	../../lib/common
	)

TARGET_LINK_LIBRARIES(zstd_seekable_format ${ZSTD_LIBRARY})
