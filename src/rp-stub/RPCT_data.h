/** RPCT error code strings (generated from RPCT_data.txt) **/
#pragma once

#include <stdint.h>

static const char RPCT_error_code_strtbl[] =
	"\x00" "RPCT_SUCCESS" "\x00" "RPCT_ERROR_CANNOT_LOAD_DLL" "\x00" "C"
	"annot load the shared library." "\x00" "RPCT_ERROR_CANNOT_OPEN_S"
	"OURCE_FILE" "\x00" "Cannot open the source file." "\x00" "RPCT_E"
	"RROR_SOURCE_FILE_NOT_SUPPORTED" "\x00" "Source file is not suppo"
	"rted." "\x00" "RPCT_ERROR_SOURCE_FILE_NO_IMAGE" "\x00" "Source f"
	"ile has no image." "\x00" "RPCT_ERROR_OUTPUT_FILE_FAILED" "\x00" "F"
	"ailed to save the output file." "\x00" "RPCT_ERROR_SOURCE_FILE_C"
	"LASS_DISABLED" "\x00" "User configuration has disabled thumbnail"
	"s for this class." "\x00" "RPCT_ERROR_SOURCE_FILE_BAD_FS" "\x00" "S"
	"ource file is located on a \"bad\" file system." "\x00" "RPCT_ER"
	"ROR_RUNNING_AS_ROOT" "\x00" "Running as root is not supported." "\x00"
	"RPCT_ERROR_INVALID_IMAGE_SIZE" "\x00" "Invalid image size reques"
	"ted. (e.g. 0 or less)" "\x00" "RPCT_ERROR_INVALID_FLAGS" "\x00" "I"
	"nvalid flag values." "\x00" "RPCT_ERROR_DIRECTORY_THUMBNAILING_D"
	"ISABLED" "\x00" "Directory thumbnailing is disabled." "\x00" "RP"
	"CT_ERROR_INVALID_EXT_IMAGE_TYPE" "\x00" "Invalid image type pass"
	"ed to getExternalImage()." "\x00" "RPCT_ERROR_SOURCE_FILE_HAS_NO"
	"_EXTURLS" "\x00" "Source file has no external image URLs." "\x00"
	"RPCT_ERROR_CANNOT_GET_CACHE_FILENAME" "\x00" "LibCacheCommon::ge"
	"tCacheFilename() failed." "\x00" "RPCT_ERROR_CACHE_NEGATIVE_CACH"
	"E_UNEXPIRED" "\x00" "File has a negative cache entry that has no"
	"t expired yet." "\x00" "RPCT_ERROR_CACHE_CANNOT_DELETE_NEGATIVE_"
	"CACHE_ENTRY" "\x00" "Could not delete an expired negative cache "
	"entry." "\x00" "RPCT_ERROR_CACHE_CANNOT_READ_CACHED_FILE" "\x00" "C"
	"annot read a file in the cache." "\x00" "RPCT_ERROR_CACHE_UNKNOW"
	"N_CACHE_ENTRY_ERROR" "\x00" "Unknown cache entry error..." "\x00"
	"RPCT_ERROR_EXECRPDOWNLOAD_NOT_IMPLEMENTED" "\x00" "execRpDownloa"
	"d() is not implemented on this system." "\x00" "RPCT_ERROR_RP_DO"
	"WNLOAD_MISSING" "\x00" "`rp-download` executable is missing." "\x00"
	"RPCT_ERROR_RP_DOWNLOAD_FAILED" "\x00" "`rp-download` executable "
	"failed to start for some reason." "\x00" "RPCT_ERROR_RP_DOWNLOAD"
	"_HUNG" "\x00" "`rp-download` never completed..." "\x00" "RPCT_ER"
	"ROR_EXT_FILE_NOT_FOUND" "\x00" "External URL returned 404." "\x00"
	"RPCT_ERROR_EXT_OTHER_ERROR" "\x00" "External URL returned a diff"
	"erent error." "\x00" "RPCT_ERROR_EXT_FILE_NOT_IN_CACHE" "\x00" "E"
	"xternal URL is not in the local cache, and a cache-only search w"
	"as requested." "\x00" "RPCT_ERROR_INVALID_INT_IMAGE_TYPE" "\x00" "I"
	"nvalid image type passed to getInternalImage()." "\x00" "RPCT_ER"
	"ROR_INT_IMAGE_NOT_FOUND" "\x00" "The specified internal image wa"
	"s not found." "\x00" "RPCT_ERROR_INTERNAL_ERROR" "\x00" "Interna"
	"l error!" "\x00";

typedef struct _RPCT_Err_String_t {
	uint16_t name_idx;
	uint16_t desc_idx;
} RPCT_Err_String_t;

static const RPCT_Err_String_t RPCT_Err_String_offtbl[] = {
	/* Error code 0 */
	{1, 0},
	{14, 41},
	{73, 108},
	{137, 174},
	{204, 236},
	{262, 292},
	{324, 362},
	{421, 451},
	{498, 525},
	{559, 589},

	/* Error code 10 */
	{636, 661},
	{682, 725},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},

	/* Error code 20 */
	{761, 795},
	{844, 882},
	{922, 959},
	{1002, 1044},
	{1102, 1154},
	{1204, 1245},
	{1278, 1321},
	{0, 0},
	{0, 0},
	{0, 0},

	/* Error code 30 */
	{1350, 1392},
	{1444, 1475},
	{1512, 1542},
	{1600, 1628},
	{1661, 1691},
	{1718, 1745},
	{1786, 1819},
	{0, 0},
	{0, 0},
	{0, 0},

	/* Error code 40 */
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},

	/* Error code 50 */
	{1898, 1932},
	{1981, 2012},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},

	/* Error code 60 */
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},

	/* Error code 70 */
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},

	/* Error code 80 */
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},

	/* Error code 90 */
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},

	/* Error code 100 */
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},

	/* Error code 110 */
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},

	/* Error code 120 */
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{2056, 2082},
};
