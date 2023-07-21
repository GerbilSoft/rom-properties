#pragma once

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

// References:
// 7-Zip Source Code (CPP/7zip/Archive/Wim/WimIn.h)
// https://github.com/libyal/assorted/blob/main/documentation/Windows%20Imaging%20(WIM)%20file%20format.asciidoc

// Version struct, read like MAJOR.MINOR.

#define FLAG_HEADER_RESERVED            0x00000001
#define FLAG_HEADER_COMPRESSION         0x00000002
#define FLAG_HEADER_READONLY            0x00000004
#define FLAG_HEADER_SPANNED             0x00000008
#define FLAG_HEADER_RESOURCE_ONLY       0x00000010
#define FLAG_HEADER_METADATA_ONLY       0x00000020
#define FLAG_HEADER_WRITE_IN_PROGRESS   0x00000040
#define FLAG_HEADER_RP_FIX              0x00000080 // reparse point fixup
#define FLAG_HEADER_COMPRESS_RESERVED   0x00010000
#define FLAG_HEADER_COMPRESS_XPRESS     0x00020000
#define FLAG_HEADER_COMPRESS_LZX        0x00040000


typedef struct 
{
    char unknown;
    uint8_t minor_version;
    uint8_t major_version;
    char unknown2;
} WIM_Version;
ASSERT_STRUCT(WIM_Version, 0x4);

typedef enum {
    Wim113_014 = 0,
    Wim109_112 = 1,
    Wim107_108 = 2,
} WIM_Version_Type;

typedef enum {
    header_reserved = 0x1,
    has_compression = 0x2,
    read_only = 0x4,
    spanned = 0x8,
    resource_only = 0x10,
    metadata_only = 0x20,
    write_in_progress = 0x40,
    rp_fix = 0x80,
} WIM_Flags;

typedef enum {
    compress_reserved = 0x10000,
    compress_xpress = 0x20000,
    compress_lzx = 0x40000,
    compress_lzms = 0x80000,
    compress_xpress2 = 0x200000,
} WIM_Compression_Flags;

typedef struct _WIM_Header_ 
{
    // 0x0
    char header[8]; // MSWIM\0\0\0
    uint32_t header_size;
    WIM_Version version;
    // 0x10
    uint32_t flags;
    uint32_t chunk_size;
    char guid[0x10];
    // 0x28
    uint16_t part_number;
    uint16_t total_parts;
    uint32_t number_of_images;
    // 0x30
    char offset_table_offset[0x18];
    // 0x48
    char xml_offset[0x18];
    // 0x60
    char boot_metadata_offset[0x18];
    // 0x78
    uint32_t bootable_index;
    char integrity_offset[0x18];
    char unused[0x3C];
} WIM_Header;


#ifdef __cplusplus
}
#endif