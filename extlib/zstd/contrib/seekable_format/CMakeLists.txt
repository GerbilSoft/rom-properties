# zstd seekable_format
PROJECT(zstd_seekable_format LANGUAGES C)

ADD_LIBRARY(zstd_seekable_format STATIC
	zstd_seekable.h
	zstdseek_compress.c
	zstdseek_decompress.c
	)
TARGET_INCLUDE_DIRECTORIES(zstd_seekable_format
	PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	PRIVATE	../../lib/common
	)

TARGET_LINK_LIBRARIES(zstd_seekable_format ${ZSTD_LIBRARY})
