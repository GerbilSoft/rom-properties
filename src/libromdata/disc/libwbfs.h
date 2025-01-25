// Stripped down version of libwbfs.h.
// Source: https://github.com/davebaol/d2x-cios/blob/master/source/cios-lib/libwbfs/libwbfs.h

#ifndef LIBWBFS_H
#define LIBWBFS_H

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable:4200)
#endif

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

typedef uint32_t be32_t;
typedef uint16_t be16_t;

#pragma pack(1)
typedef struct RP_PACKED wbfs_head {
        be32_t	magic;
        // parameters copied in the partition for easy dumping, and bug reports
        be32_t	n_hd_sec;	// total number of hd_sec in this partition
        uint8_t	hd_sec_sz_s;	// sector size in this partition
        uint8_t	wbfs_sec_sz_s;	// size of a wbfs sec
        uint8_t	padding3[2];
        uint8_t	disc_table[0];	// size depends on hd sector size
} wbfs_head_t;
#pragma pack()

typedef struct wbfs_disc_info {
        uint8_t	disc_header_copy[0x100];
        be16_t	wlba_table[0];
} wbfs_disc_info_t;

//  WBFS first wbfs_sector structure:
//
//  -----------
// | wbfs_head |  (hd_sec_sz)
//  -----------
// |	       |
// | disc_info |
// |	       |
//  -----------
// |	       |
// | disc_info |
// |	       |
//  -----------
// |	       |
// | ...       |
// |	       |
//  -----------
// |	       |
// | disc_info |
// |	       |
//  -----------
// |	       |
// |freeblk_tbl|
// |	       |
//  -----------
//

typedef struct wbfs_s {
	wbfs_head_t *head;

        /* hdsectors, the size of the sector provided by the hosting hard drive */
        uint32_t hd_sec_sz;
        uint8_t  hd_sec_sz_s;	// the power of two of the last number
        uint32_t n_hd_sec;	// the number of hd sector in the wbfs partition

        /* standard wii sector (0x8000 bytes) */
        uint32_t wii_sec_sz; 
        uint8_t  wii_sec_sz_s;
        uint32_t n_wii_sec;
        uint32_t n_wii_sec_per_disc;
        
        /* The size of a wbfs sector */
        uint32_t wbfs_sec_sz;
        uint32_t wbfs_sec_sz_s; 
        uint16_t n_wbfs_sec;		// this must fit in 16 bit!
        uint16_t n_wbfs_sec_per_disc;	// size of the lookup table

        uint16_t max_disc;
        uint32_t freeblks_lba;
        uint32_t *freeblks;		// free blocks table (unused here?)
        uint16_t disc_info_sz;

	uint32_t n_disc_open;
} wbfs_t;

typedef struct wbfs_disc_s
{
        wbfs_t *p;
        wbfs_disc_info_t  *header;	  // pointer to wii header
        int i;		  		  // disc index in the wbfs header (disc_table)
} wbfs_disc_t;

#define WBFS_MAGIC (('W'<<24)|('B'<<16)|('F'<<8)|('S'))

#ifdef __cplusplus
   }
#endif /* __cplusplus */

#ifdef _MSC_VER
# pragma warning(pop)
#endif

#endif
