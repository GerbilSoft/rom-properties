/*
 * pngcheck:  Authenticate the structure of a PNG file and dump info about
 *            it if desired.
 *
 * This program checks the PNG signature bytes (with tests for various forms
 * of text-mode corruption), chunks (CRCs, dependencies, out-of-range values),
 * and compressed image data (IDAT zlib stream).  In addition, it optionally
 * dumps the contents of PNG, JNG and MNG image streams in more-or-less human-
 * readable form.
 *
 *        NOTE:  this program is currently NOT EBCDIC-compatible!
 *               (as of July 2007)
 *
 * Maintainer:  Greg Roelofs <newt@pobox.com>
 * ChangeLog:   see CHANGELOG file
 */

/*============================================================================
 *
 *   Copyright 1995-2007 by Alexander Lehmann <lehmann@usa.net>,
 *                          Andreas Dilger <adilger@enel.ucalgary.ca>,
 *                          Glenn Randers-Pehrson <randeg@alum.rpi.edu>,
 *                          Greg Roelofs <newt@pobox.com>,
 *                          John Bowler <jbowler@acm.org>,
 *                          Tom Lane <tgl@sss.pgh.pa.us>
 *
 *   Permission to use, copy, modify, and distribute this software and its
 *   documentation for any purpose and without fee is hereby granted, provided
 *   that the above copyright notice appear in all copies and that both that
 *   copyright notice and this permission notice appear in supporting
 *   documentation.  This software is provided "as is" without express or
 *   implied warranty.
 *
 *===========================================================================*/

#define VERSION "2.3.0 of 7 July 2007"

/*
 * NOTE:  current MNG support is informational; error-checking is MINIMAL!
 *
 *
 * Currently supported chunks, in order of appearance in pngcheck() function:
 *
 *   IHDR JHDR MHDR				// PNG/JNG/MNG header chunks
 *
 *   PLTE IDAT IEND				// critical PNG chunks
 *
 *   bKGD cHRM fRAc gAMA gIFg gIFt gIFx hIST	// ancillary PNG chunks
 *   iCCP iTXt oFFs pCAL pHYs sBIT sCAL sPLT
 *   sRGB tEXt zTXt tIME tRNS
 *
 *   cmOD cmPP cpIp mkBF mkBS mkBT mkTS pcLb	// known private PNG chunks
 *   prVW spAL					// [msOG = ??]
 *
 *   JDAT JSEP  				// critical JNG chunks
 *
 *   DHDR FRAM SAVE SEEK nEED DEFI BACK MOVE	// MNG chunks
 *   CLON SHOW CLIP LOOP ENDL PROM fPRI eXPI
 *   BASI IPNG PPLT PAST TERM DISC pHYg DROP
 *   DBYK ORDR MAGN MEND
 *
 * Known unregistered, "public" chunks (i.e., invalid and now flagged as such):
 *
 *   pRVW nULL tXMP
 *
 *
 * GRR to do:
 *   - normalize error levels (mainly usage of kMinorError vs. kMajorError)
 *   - fix tEXt chunk:  small buffers or lots of text => truncation
 *       (see pngcheck-1.99.4-test.c.dif)
 *   - fix iCCP, sPLT chunks:  small buffers or large chunks => truncation?
 *   - update existing MNG support to version 1.0 (already done?)
 *   - add JNG restrictions to bKGD
 *   - allow top-level ancillary PNGs in MNG (i.e., subsequent ones may be NULL)
 *   * add MNG profile report based on actual chunks found
 *   - split out each chunk's code into XXXX() function (e.g., IDAT(), tRNS())
 *   * with USE_ZLIB, print zTXt and compressed iTXt chunks if -t option
 *       (break out zlib decoder into separate function and reuse)
 *       (also iCCP?)
 *   - DOS/Win32 wildcard support beyond emx+gcc, MSVC (Borland wildargs.obj?)
 *   - EBCDIC support (minimal?)
 *   - go back and make sure validation checks not dependent on verbosity level
 *
 *
 * GRR NOTE:  The MNG "top level" concept is not explicitly defined anywhere
 * in the MNG 1.0 spec, but it refers to "global" chunks/values and apparently
 * means before any embedded PNG or JNG images appear (i.e., before any IHDR
 * or JHDR chunks are encountered).  The top_level variable is set accordingly.
 */

/*
 * Compilation example (GNU C, command line; replace "/zlibpath" appropriately):
 *
 *    without zlib:
 *       gcc -O -o pngcheck pngcheck.c
 *    with zlib support (recommended):
 *       gcc -O -DUSE_ZLIB -I/zlibpath -o pngcheck pngcheck.c -L/zlibpath -lz
 *    or (static zlib):
 *       gcc -O -DUSE_ZLIB -I/zlibpath -o pngcheck pngcheck.c /zlibpath/libz.a
 *
 * Windows compilation example (MSVC, command line, assuming VCVARS32.BAT or
 * whatever has been run):
 *
 *    without zlib:
 *       cl -nologo -O -W3 -DWIN32 -c pngcheck.c
 *       link -nologo pngcheck.obj setargv.obj
 *    with zlib support (note that Win32 zlib is compiled as a DLL by default):
 *       cl -nologo -O -W3 -DWIN32 -DUSE_ZLIB -I/zlibpath -c pngcheck.c
 *       link -nologo pngcheck.obj setargv.obj \zlibpath\zlib.lib
 *       [copy pngcheck.exe and zlib.dll to installation directory]
 *
 * "setargv.obj" is included with MSVC and will be found if the batch file has
 * been run.  Either Borland or Watcom (both?) may use "wildargs.obj" instead.
 * Both object files serve the same purpose:  they expand wildcard arguments
 * into a list of files on the local file system, just as Unix shells do by
 * default ("globbing").  Note that mingw32 + gcc (Unix-like compilation
 * environment for Windows) apparently expands wildcards on its own, so no
 * special object files are necessary for it.  emx + gcc for OS/2 (and possibly
 * rsxnt + gcc for Windows NT) has a special _wildcard() function call, which
 * is already included at the top of main() below.
 *
 * zlib info:		http://www.zlib.net/
 * PNG/MNG/JNG info:	http://www.libpng.org/pub/png/
 *			http://www.libpng.org/pub/mng/  and
 *                      ftp://ftp.simplesystems.org/pub/libpng/mng/
 * pngcheck sources:	http://www.libpng.org/pub/png/apps/pngcheck.html
 */

/**
 * rom-properties: Modified to be usable as a library for IRpFile*
 * instead of as a standalone program.
 */
#include "pngcheck.hpp"
#include "../../file/IRpFile.hpp"
using LibRpBase::IRpFile;

#include <memory>
using std::unique_ptr;

// C includes.
#include <stdlib.h>

// C includes. (C++ namespace)
#include <cstdio>
#include <cctype>
#include <cstring>

// rom-properties: Uncomment this to enable printf() output.
//#define RP_PRINTF_ENABLED 1
// rom-properties: Dummy define for data used by main() and usage().
// DO NOT ENABLE THIS!
//#define RP_MAIN_USAGE_ENABLED 1

#ifdef __riscos
   /* not sure if this will work (fragile!), but relatively clean... */
   struct stat { long st_size; };
#  define stat(f,s)   _swix(8 /*OS_File*/, 3 | 1<<27, 17, f, s.st_size)
#  define isatty(fd)  (!__iob[fd].__file)
#else
#  include <fcntl.h>
#  if defined(__MWERKS__) && defined(macintosh)  /* pxm for CodeWarrior */
#    include <types.h>
#    include <stat.h>
#  elif defined(applec) || defined(THINK_C)  /* via Mark Fleming; not tested */
#    include <Types.h>
#    include <ToolUtils.h>
#  else
#    include <sys/types.h>
#    include <sys/stat.h>
#  endif
#endif

#if defined(unix) || (defined(__MWERKS__) && defined(macintosh))  /* pxm */
#  include <unistd.h>	/* isatty() */
#endif
#ifdef WIN32
#  include <io.h>
#endif

#ifdef USE_ZLIB
#  include <zlib.h>
#endif

namespace LibRpBase {

typedef unsigned char  uch;
typedef unsigned short ush;
typedef unsigned long  ulg;

/* printbuf state variables */
typedef struct printbuf_state {
#ifdef RP_PRINTF_ENABLED
  int cr;
  int lf;
  int nul;
  int control;
  int esc;
#else
  // Need to define at least one field.
  uint8_t dummy;
#endif /* RP_PRINTF_ENABLED */
} printbuf_state;

#if 0 /* rom-properties */
/* int  main (int argc, char *argv[]); */
void usage (FILE *fpMsg);
#endif /* rom-properties */

/* rom-properties: Disable printf(), since we don't want any console output. */
#ifndef RP_PRINTF_ENABLED
#define printf(fmt, ...) do { } while (0)
#endif

#ifndef USE_ZLIB
static void make_crc_table (void);
static ulg  update_crc (ulg crc, uch *buf, int len);
#endif
#if 0 /* rom-properties */
static ulg  getlong (FILE *fp, const char *fname, const char *where);
#endif /* rom-properties */
static void putlong (FILE *fpOut, ulg ul);
#if 0 /* rom-properties */
static void init_printbuf_state (printbuf_state *prbuf);
static void print_buffer (printbuf_state *prbuf, uch *buffer, int size, int indent);
static void report_printbuf (printbuf_state *prbuf, const char *fname, const char *chunkid);
#endif /* rom-properties */
static int  keywordlen (uch *buffer, int maxsize);
#ifdef RP_PRINTF_ENABLED
static const char *getmonth (int m);
static int  ratio (ulg uc, ulg c);
#endif /* RP_PRINTF_ENABLED */
static ulg  gcf (ulg a, ulg b);
#if 0 /* rom-properties */
// TODO: Wrapper function for compatibility?
int  pngcheck (FILE *fp, const char *_fname, int searching, FILE *fpOut);
#endif /* rom-properties */
int  pngcheck (IRpFile *fp);
#if 0 /* rom-properties */
static int  pnginfile (FILE *fp, const char *fname, int ipng, int extracting);
static void pngsearch (FILE *fp, const char *fname, int extracting);
static int  check_magic (uch *magic, const char *fname, int which);
static int  check_chunk_name (const char *chunk_name, const char *fname);
static int  check_keyword (uch *buffer, int maxsize, int *pKeylen,
                    const char *keyword_name, const char *chunkid, const char *fname);
static int  check_text (uch *buffer, int maxsize, const char *chunkid, const char *fname);
static int  check_ascii_float (uch *buffer, int len, const char *chunkid, const char *fname);
#endif

#define BS 32000 /* size of read block for CRC calculation (and zlib) */

/* Mark's macros to extract big-endian short and long ints: */
#define SH(p) ((ush)(uch)((p)[1]) | ((ush)(uch)((p)[0]) << 8))
#define LG(p) ((ulg)(SH((p)+2)) | ((ulg)(SH(p)) << 16))

/* for check_magic(): */
#define DO_PNG  0
#define DO_MNG  1
#define DO_JNG  2

#ifdef RP_PRINTF_ENABLED
/* GRR 20070704:  borrowed from GRR from/mailx hack */
#define COLOR_NORMAL        "\033[0m"
#define COLOR_RED_BOLD      "\033[01;31m"
#define COLOR_RED           "\033[40;31m"
#define COLOR_GREEN_BOLD    "\033[01;32m"
#define COLOR_GREEN         "\033[40;32m"
#define COLOR_YELLOW_BOLD   "\033[01;33m"
#define COLOR_YELLOW        "\033[40;33m"	/* chunk names */
#define COLOR_BLUE_BOLD     "\033[01;34m"
#define COLOR_BLUE          "\033[40;34m"
#define COLOR_MAGENTA_BOLD  "\033[01;35m"
#define COLOR_MAGENTA       "\033[40;35m"
#define COLOR_CYAN_BOLD     "\033[01;36m"
#define COLOR_CYAN          "\033[40;36m"
#define COLOR_WHITE_BOLD    "\033[01;37m"	/* filenames, filter seps */
#define COLOR_WHITE         "\033[40;37m"
#endif /* RP_PRINTF_ENABLED */

#define isASCIIalpha(x)     (ascii_alpha_table[x] & 0x1)

#define ANCILLARY(chunkID)  ((chunkID)[0] & 0x20)
#define PRIVATE(chunkID)    ((chunkID)[1] & 0x20)
#define RESERVED(chunkID)   ((chunkID)[2] & 0x20)
#define SAFECOPY(chunkID)   ((chunkID)[3] & 0x20)
#define CRITICAL(chunkID)   (!ANCILLARY(chunkID))
#define PUBLIC(chunkID)     (!PRIVATE(chunkID))

#define set_err(x)  global_error = ((global_error < (x))? (x) : global_error)
#define is_err(x)   (global_error > (x) || (!force && global_error == (x)))
#define no_err(x)   (global_error < (x) || (force && global_error == (x)))

class CPngCheck {
IRpFile *fp;
const char *fname;

/* Command-line flag variables */
int verbose;		/* print chunk info */
int quiet;		/* print only error messages */
int printtext;		/* print tEXt chunks */
int printpal;		/* print PLTE/tRNS/hIST/sPLT contents */
int color;		/* print with ANSI colors to spice things up */
int sevenbit;		/* escape characters >=160 */
int force;		/* continue even if an error occurs (CRC error, etc) */
int check_windowbits;	/* more stringent zlib stream-checking */
int suppress_warnings;	/* don't fuss about ambiguous stuff */
int search;		/* hunt for PNGs in the file... */
int extract;		/* ...and extract them to arbitrary file names. */
int png;		/* it's a PNG */
int mng;		/* it's a MNG instead of a PNG (won't work in pipe) */
int jng;		/* it's a JNG */

int global_error ; 	/* the current error status */
uch buffer[BS];

#ifdef USE_ZLIB
   int first_idat;	/* flag:  is this the first IDAT chunk? */
   int zlib_error;	/* reset in IHDR section; used for IDAT */
   int check_zlib;	/* validate zlib stream (just IDATs for now) */
   unsigned zlib_windowbits;
   uch outbuf[BS];
   z_stream zstrm;
#ifdef RP_PRINTF_ENABLED
   const char **pass_color;
   const char *color_off;
#endif /* RP_PRINTF_ENABLED */
#endif

ulg getlong(const char *where);

#ifdef RP_PRINTF_ENABLED
void init_printbuf_state (printbuf_state *prbuf);
void print_buffer (printbuf_state *prbuf, uch *buffer, int size, int indent);
void report_printbuf (printbuf_state *prbuf, const char *chunkid);
#else /* !RP_PRINTF_ENABLED */
static inline void init_printbuf_state (printbuf_state *prbuf) {
	((void)prbuf);
}
static inline void print_buffer (printbuf_state *prbuf, uch *buffer, int size, int indent) {
	((void)prbuf);
	((void)buffer);
	((void)size);
	((void)indent);
}
static inline void report_printbuf (printbuf_state *prbuf, const char *chunkid) {
	((void)prbuf);
	((void)chunkid);
}
#endif /* RP_PRINTF_ENABLED */

int check_magic (uch *magic, int which);
int check_chunk_name (const char *chunk_name);
int check_keyword (uch *buffer, int maxsize, int *pKeylen,
                   const char *keyword_name, const char *chunkid);
int check_text (uch *buffer, int maxsize, const char *chunkid);
int check_ascii_float (uch *buffer, int len, const char *chunkid);

public:
	explicit CPngCheck(IRpFile *fp)
		: fp(fp)
		, fname(nullptr)
		, verbose(0)
		, quiet(1)
		, printtext(0)
		, printpal(0)
		, color(0)
		, sevenbit(0)
		, force(0)
		, check_windowbits(1)
		, suppress_warnings(0)
		, search(0)
		, extract(0)
		, png(0)
		, mng(0)
		, jng(0)
		, global_error(kOK)
#ifdef USE_ZLIB
		, first_idat(1)
		, zlib_error(0)
		, check_zlib(1)
		, zlib_windowbits(15)
#ifdef RP_PRINTF_ENABLED
		, pass_color(nullptr)
		, color_off(nullptr)
#endif /* RP_PRINTF_ENABLED */
#endif /* USE_ZLIB */
	{
#ifdef USE_ZLIB
		// Clear arrays and structs.
		memset(outbuf, 0, sizeof(outbuf));
		memset(&zstrm, 0, sizeof(zstrm));
#endif /* USE_ZLIB */
#if defined(USE_ZLIB) && defined(RP_PRINTF_ENABLED)
		// Can't use the one declared later, so we'll
		// copy it here.
		static const char *const pass_color_disabled[] = {
			"", "", "", "", "", "", "", ""
		};
		pass_color = pass_color_disabled;
		color_off = pass_color_disabled[0];
#endif /* USE_ZLIB && RP_PRINTF_ENABLED */
	}

	int pngcheck(void);
};

/* what the PNG, MNG and JNG magic numbers should be */
static const uch good_PNG_magic[8] = {137, 80, 78, 71, 13, 10, 26, 10};
static const uch good_MNG_magic[8] = {138, 77, 78, 71, 13, 10, 26, 10};
static const uch good_JNG_magic[8] = {139, 74, 78, 71, 13, 10, 26, 10};

/* GRR FIXME:  could merge all three of these into single table (bit fields) */

/* GRR 20061203:  for "isalpha()" that works even on EBCDIC machines */
static const uch ascii_alpha_table[256] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,
  0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 
};

/* GRR 20070707:  list of forbidden characters in various keywords */
static const uch latin1_keyword_forbidden[256] = {
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 
};

/* GRR 20070707:  list of discouraged (control) characters in tEXt/zTXt text */
static const uch latin1_text_discouraged[256] = {
  1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 
};

static const char inv[] = "INVALID";

#ifndef USE_ZLIB
   static ulg crc_table[256];           /* table of CRCs of all 8-bit messages */
   static int crc_table_computed = 0;   /* flag:  has the table been computed? */
#endif

/* PNG stuff */

static const char *const png_type[] = {		/* IHDR, tRNS, BASI, summary */
  "grayscale",
  "INVALID",   /* can't use inv as initializer */
  "RGB",
  "palette",   /* was "colormap" */
  "grayscale+alpha",
  "INVALID",
  "RGB+alpha"
};

#ifdef RP_PRINTF_ENABLED
static const char *const deflate_type[] = {		/* IDAT */
  "superfast",
  "fast",
  "default",
  "maximum"
};

#ifdef USE_ZLIB
static const char *const zlib_error_type[] = {		/* IDAT */
  "filesystem error",
  "stream error",
  "data error",
  "memory error",
  "buffering error",
  "version error"
};

#ifdef RP_MAIN_USAGE_ENABLED
static const char *const pass_color_enabled[] = {	/* IDAT */
  COLOR_NORMAL,		/* color_off */
  COLOR_WHITE,		/* using 1-based indexing */
  COLOR_BLUE,
  COLOR_GREEN,
  COLOR_YELLOW,
  COLOR_RED,
  COLOR_CYAN,
  COLOR_MAGENTA
};

static const char *const pass_color_disabled[] = {	/* IDAT */
  "", "", "", "", "", "", "", ""
};
#endif /* RP_MAIN_USAGE_ENABLED */
#endif /* USE_ZLIB */

static const char *const eqn_type[] = {			/* pCAL */
  "physical_value = p0 + p1 * original_sample / (x1-x0)",
  "physical_value = p0 + p1 * exp(p2 * original_sample / (x1-x0))",
  "physical_value = p0 + p1 * pow(p2, (original_sample / (x1-x0)))",
  "physical_value = p0 + p1 * sinh(p2 * (original_sample - p3) / (x1-x0))"
};
#endif /* RP_PRINTF_ENABLED */

static const int eqn_params[] = { 2, 3, 3, 4 };		/* pCAL */

#ifdef RP_PRINTF_ENABLED
static const char *const rendering_intent[] = {		/* sRGB */
  "perceptual",
  "relative colorimetric",
  "saturation-preserving",
  "absolute colorimetric"
};


/* JNG stuff */

static const char *const jng_type[] = {			/* JHDR, summary */
  "grayscale",
  "YCbCr",
  "grayscale+alpha",
  "YCbCr+alpha"
};


/* MNG stuff */

static const char *const delta_type[] = {		/* DHDR */
  "full image replacement",
  "block pixel addition",
  "block alpha addition",
  "block pixel replacement",
  "block alpha replacement",
  "no change"
};

static const char *const termination_condition[] = {	/* LOOP */
  "deterministic",
  "decoder discretion",
  "user discretion",
  "external signal"
};

static const char *const termination_action[] = {	/* TERM */
  "show last frame indefinitely",
  "cease displaying anything",
  "show first frame after TERM",
  "repeat sequence between TERM and MEND"
};

static const char *const framing_mode[] = {		/* FRAM */
  "no change in framing mode",
  "no background layer; interframe delay before each image displayed",
  "no background layer; interframe delay before each FRAM chunk",
  "interframe delay and background layer before each image displayed",
  "interframe delay and background layer after each FRAM chunk"
};

static const char *const change_interframe_delay[] = {	/* FRAM */
  "no change in interframe delay",
  "change interframe delay for next subframe",
  "change interframe delay and make default"
};

static const char *const change_timeout_and_termination[] = {	/* FRAM */
  "no change in timeout and termination",
  "deterministic change in timeout and termination for next subframe",
  "deterministic change in timeout and termination; make default",
  "decoder-discretion change in timeout and termination for next subframe",
  "decoder-discretion change in timeout and termination; make default",
  "user-discretion change in timeout and termination for next subframe",
  "user-discretion change in timeout and termination; make default",
  "change in timeout and termination for next subframe via signal",
  "change in timeout and termination via signal; make default"
};

static const char *const change_subframe_clipping_boundaries[] = {	/* FRAM */
  "no change in subframe clipping boundaries",
  "change frame clipping boundaries for next subframe",
  "change frame clipping boundaries and make default"
};

static const char *const change_sync_id_list[] = {	/* FRAM */
  "no change in sync ID list",
  "change sync ID list for next subframe:",
  "change sync ID list and make default:"
};

static const char *const clone_type[] = {		/* CLON */
  "full",
  "partial",
  "renumber"
};

static const char *const do_not_show[] = {		/* DEFI, CLON */
  "potentially visible",
  "do not show",
  "same visibility as parent"
};

static const char *const show_mode[] = {		/* SHOW */
  "make objects potentially visible and display",
  "make objects invisible",
  "display potentially visible objects",
  "make objects potentially visible but do not display",
  "toggle potentially visible and invisible objects; display visible ones",
  "toggle potentially visible and invisible objects but do not display any",
  "make next object potentially visible and display; make rest invisible",
  "make next object potentially visible but do not display; make rest invisible"
};

static const char *const entry_type[] = {		/* SAVE */
  "segment with full info",
  "segment",
  "subframe",
  "exported image"
};

static const char *const pplt_delta_type[] = {		/* PPLT */
  "replacement RGB samples",
  "delta RGB samples",
  "replacement alpha samples",
  "delta alpha samples",
  "replacement RGBA samples",
  "delta RGBA samples"
};

static const char *const composition_mode[] = {		/* PAST */
  "composite over",
  "replace",
  "composite under"
};

static const char *const orientation[] = {		/* PAST */
  "same as source image",
  "flipped left-right then up-down",
  "flipped left-right",
  "flipped up-down",
  "tiled with source image"
};

static const char *const order_type[] = {		/* ORDR */
  "anywhere",
  "after IDAT and/or JDAT or JDAA",
  "before IDAT and/or JDAT or JDAA",
  "before IDAT but not before PLTE",
  "before IDAT but not after PLTE"
};

static const char *const magnification_method[] = {	/* MAGN */
  "no magnification",
  "pixel replication of all samples",
  "linear interpolation of all samples",
  "replication of all samples from nearest pixel",
  "linear interpolation of color, nearest-pixel replication of alpha",
  "linear interpolation of alpha, nearest-pixel replication of color"
};

#ifdef RP_MAIN_USAGE_ENABLED
static const char brief_error_color[] = COLOR_RED_BOLD "ERROR" COLOR_NORMAL;
static const char brief_error_plain[] = "ERROR";
#endif /* RP_MAIN_USAGE_ENABLED */
static const char brief_warn_color[] = COLOR_YELLOW_BOLD "WARN" COLOR_NORMAL;
static const char brief_warn_plain[] = "WARN";
static const char brief_OK_color[] = COLOR_GREEN_BOLD "OK" COLOR_NORMAL;
static const char brief_OK_plain[] = "OK";

#ifdef RP_MAIN_USAGE_ENABLED
static const char errors_color[] = COLOR_RED_BOLD "ERRORS DETECTED" COLOR_NORMAL;
static const char errors_plain[] = "ERRORS DETECTED";
#endif /* RP_MAIN_USAGE_ENABLED */
static const char warnings_color[] = COLOR_YELLOW_BOLD "WARNINGS DETECTED" COLOR_NORMAL;
static const char warnings_plain[] = "WARNINGS DETECTED";
static const char no_err_color[] = COLOR_GREEN_BOLD "No errors detected" COLOR_NORMAL;
static const char no_err_plain[] = "No errors detected";
#endif /* RP_PRINTF_ENABLED */



#if 0 /* rom-properties */
int main(int argc, char *argv[])
{
  FILE *fp;
  int i = 1;
  int err = kOK;
  int num_files = 0;
  int num_errors = 0;
  int num_warnings = 0;
  const char *brief_error     = color? brief_error_color : brief_error_plain;
  const char *errors_detected = color? errors_color      : errors_plain;

#ifdef __EMX__
  _wildcard(&argc, &argv);   /* Unix-like globbing for OS/2 and DOS */
#endif

  while (argc > 1 && argv[1][0] == '-') {
    switch (argv[1][i]) {
      case '\0':
        --argc;
        ++argv;
        i = 1;
        break;
      case '7':
        printtext = 1;
        sevenbit = 1;
        ++i;
        break;
      case 'c':
        color = 1;
        ++i;
        break;
      case 'f':
        force = 1;
        ++i;
        break;
      case 'h':
        usage(stdout);
        return err;
      case 'p':
        printpal = 1;
        ++i;
        break;
      case 'q':
        verbose = 0;
        quiet = 1;
        ++i;
        break;
      case 's':
        search = 1;
        ++i;
        break;
      case 't':
        printtext = 1;
        ++i;
        break;
      case 'v':
        ++verbose;  /* verbose == 2 means decode IDATs and print filter info */
        quiet = 0;  /* verbose == 4 means print pixel values, too */
        ++i;
        break;
      case 'w':
        check_windowbits = 0;
        ++i;
        break;
      case 'x':
        search = extract = 1;
        ++i;
        break;
      default:
        fprintf(stderr, "error: unknown option %c\n\n", argv[1][i]);
        usage(stderr);
        return kCommandLineError;
    }
  }

  if (color) {
    brief_error = brief_error_color;
    errors_detected = errors_color;
#ifdef USE_ZLIB
    pass_color = pass_color_enabled;
    color_off = pass_color_enabled[0];
#endif
  } else {
    brief_error = brief_error_plain;
    errors_detected = errors_plain;
#ifdef USE_ZLIB
    pass_color = pass_color_disabled;
    color_off = pass_color_disabled[0];
#endif
  }

  if (argc == 1) {
    if (isatty(0)) { /* if stdin not redirected, give the user help */
      usage(stdout);
    } else {
      const char fname[] = "stdin";

      if (search)
        pngsearch(stdin, fname, extract);  /* currently returns void */
      else
        err = pngcheck(stdin, fname, 0, NULL);
      ++num_files;
      if (err == kWarning)
        ++num_warnings;
      else if (err > kWarning) {
        ++num_errors;
        if (verbose)
          printf("%s in %s\n", errors_detected, fname);
        else
          printf("%s: %s%s%s\n", brief_error,
            color? COLOR_YELLOW:"", fname, color? COLOR_NORMAL:"");
      }
    }
  } else {
#ifdef USE_ZLIB
    /* make sure we're using the zlib version we were compiled to use */
    if (zlib_version[0] != ZLIB_VERSION[0]) {
      printf("zlib error:  incompatible version (expected %s,"
        " using %s):  skipping zlib check\n\n", ZLIB_VERSION, zlib_version);
      check_zlib = 0;
      if (verbose > 1)
        verbose = 1;
    } else if (strcmp(zlib_version, ZLIB_VERSION) != 0) {
      printf("zlib warning:  different version (expected %s,"
        " using %s)\n\n", ZLIB_VERSION, zlib_version);
    }
#endif /* USE_ZLIB */

    /* main loop over files listed on command line */
    for (i = 1; i < argc; ++i) {
      const char *const fname = argv[i];

      err = kOK;
      if (strcmp(fname, "-") == 0) {
        fname = "stdin";
        fp = stdin;
      } else if ((fp = fopen(fname, "rb")) == NULL) {
        perror(fname);
        err = kCriticalError;
      }

      if (err == kOK) {
        if (search)
          pngsearch(fp, fname, extract);
        else
          err = pngcheck(fp, fname, 0, NULL);
        if (fp != stdin)
          fclose(fp);
      }

      ++num_files;
      if (err == kWarning)
        ++num_warnings;
      else if (err > kWarning) {
        ++num_errors;
        if (verbose)
          printf("%s in %s\n", errors_detected, fname);
        else
          printf("%s: %s%s%s\n", brief_error,
            color? COLOR_YELLOW:"", fname, color? COLOR_NORMAL:"");
      }
    }
  }

  if (num_errors > 0)
    err = (num_errors > 127)? 127 : (num_errors < 2)? 2 : num_errors;
  else if (num_warnings > 0)
    err = 1;
  if (!quiet && num_files > 1) {
      printf("\n");
      if (num_errors > 0)
        printf("Errors were detected in %d of the %d files tested.\n",
          num_errors, num_files);
      if (num_warnings > 0)
        printf("Warnings were detected in %d of the %d files tested.\n",
          num_warnings, num_files);
      if (num_errors + num_warnings < num_files)
        printf("No errors were detected in %d of the %d files tested.\n",
          num_files - (num_errors + num_warnings), num_files);
  }

  return err;
}



/* GRR 20061203 */
static void usage(FILE *fpMsg)
{
  fprintf(fpMsg, "PNGcheck, version %s,\n", VERSION);
  fprintf(fpMsg, "   by Alexander Lehmann, Andreas Dilger and Greg Roelofs.\n");
#ifdef USE_ZLIB
  fprintf(fpMsg, "   Compiled with zlib %s; using zlib %s.\n",
    ZLIB_VERSION, zlib_version);
#endif

  fprintf(fpMsg, "\n"
    "Test PNG, JNG or MNG image files for corruption, and print size/type info."
    "\n\n"
    "Usage:  pngcheck [-7cfpqtv] file.{png|jng|mng} [file2.{png|jng|mng} [...]]\n"
    "   or:  ... | pngcheck [-7cfpqstvx]\n"
    "   or:  pngcheck [-7cfpqstvx] file-containing-PNGs...\n"
    "\n"
    "Options:\n"
    "   -7  print contents of tEXt chunks, escape chars >=128 (for 7-bit terminals)\n"
    "   -c  colorize output (for ANSI terminals)\n"
    "   -f  force continuation even after major errors\n"
    "   -p  print contents of PLTE, tRNS, hIST, sPLT and PPLT (can be used with -q)\n"
    "   -q  test quietly (output only errors)\n"
    "   -s  search for PNGs within another file\n"
    "   -t  print contents of tEXt chunks (can be used with -q)\n"
    "   -v  test verbosely (print most chunk data)\n"
#ifdef USE_ZLIB
    "   -vv test very verbosely (decode & print line filters)\n"
    "   -w  suppress windowBits test (more-stringent compression check)\n"
#endif
    "   -x  search for PNGs within another file and extract them when found\n"
    "\n"
    "Note:  MNG support is more informational than conformance-oriented.\n"
  );

  fflush(fpMsg);
}
#endif /* rom-properties */



#ifdef USE_ZLIB

#  define CRCCOMPL(c) c
#  define CRCINIT (0)
#  define update_crc crc32

#else /* !USE_ZLIB */

   /* use these instead of ~crc and -1, since that doesn't work on machines
    * that have 64-bit longs */
#  define CRCCOMPL(c) ((c)^0xffffffff)
#  define CRCINIT (CRCCOMPL(0))

/* make the table for a fast crc */
static void make_crc_table(void)
{
  int n;

  for (n = 0; n < 256; ++n) {
    ulg c;
    int k;

    c = (ulg)n;
    for (k = 0; k < 8; ++k)
      c = c & 1 ? 0xedb88320L ^ (c >> 1):c >> 1;

    crc_table[n] = c;
  }
  crc_table_computed = 1;
}



/* update a running crc with the bytes buf[0..len-1]--the crc should be
   initialized to all 1's, and the transmitted value is the 1's complement
   of the final running crc. */

static ulg update_crc(ulg crc, uch *buf, int len)
{
  ulg c = crc;
  uch *p = buf;
  int n = len;

  if (!crc_table_computed) {
    make_crc_table();
  }

  if (n > 0) do {
    c = crc_table[(c ^ (*p++)) & 0xff] ^ (c >> 8);
  } while (--n);
  return c;
}

#endif /* ?USE_ZLIB */



ulg CPngCheck::getlong(const char *where)
{
  ulg res = 0;
  int j;

  for (j = 0; j < 4; ++j) {
    int c;

    if ((c = fp->getc()) == EOF) {
      printf("%s  EOF while reading %s\n", verbose? ":":fname, where);
      set_err(kCriticalError);
      return 0;
    }
    res <<= 8;
    res |= c & 0xff;
  }

  return res;
}



/* output a long when copying an embedded PNG out of a file. */
static void putlong(FILE *fpOut, ulg ul)
{
  putc(ul >> 24, fpOut);
  putc(ul >> 16, fpOut);
  putc(ul >> 8, fpOut);
  putc(ul, fpOut);
}



#ifdef RP_PRINTF_ENABLED
/* print out "size" characters in buffer, taking care not to print control
   chars other than whitespace, since this may open ways of attack by so-
   called ANSI-bombs */

void CPngCheck::init_printbuf_state(printbuf_state *prbuf)
{
  prbuf->cr = 0;
  prbuf->lf = 0;
  prbuf->nul = 0;
  prbuf->control = 0;
  prbuf->esc = 0;
}



/* GRR EBCDIC WARNING */
void CPngCheck::print_buffer(printbuf_state *prbuf, uch *buf, int size, int indent)
{
  if (indent)
    printf("    ");
  while (size--) {
    uch c;

    c = *buf++;

    if ((c < ' ' && c != '\t' && c != '\n') ||
        (sevenbit? c > 127 : (c >= 127 && c < 160)))
      printf("\\%02X", c);
/*
    else if (c == '\\')
      printf("\\\\");
 */
    else
      putchar(c);

    if (c < 32 || (c >= 127 && c < 160)) {
      if (c == '\n') {
        prbuf->lf = 1;
        if (indent && size > 0)
          printf("    ");
      } else if (c == '\r')
        prbuf->cr = 1;
      else if (c == '\0')
        prbuf->nul = 1;
      else
        prbuf->control = 1;
      if (c == 27)
        prbuf->esc = 1;
    }
  }
}



void CPngCheck::report_printbuf(printbuf_state *prbuf, const char *chunkid)
{
  if (prbuf->cr) {
    if (prbuf->lf) {
      printf("%s  %s chunk contains both CR and LF as line terminators\n",
             verbose? "":fname, chunkid);
      set_err(kMinorError);
    } else {
      printf("%s  %s chunk contains only CR as line terminator\n",
             verbose? "":fname, chunkid);
      set_err(kMinorError);
    }
  }
  if (prbuf->nul) {
    printf("%s  %s chunk contains null bytes\n", verbose? "":fname, chunkid);
    set_err(kMinorError);
  }
  if (prbuf->control) {
    printf("%s  %s chunk contains one or more control characters%s\n",
           verbose? "":fname, chunkid, prbuf->esc? " including Escape":"");
    set_err(kMinorError);
  }
}
#endif /* RP_PRINTF_ENABLED */



static int keywordlen(uch *buf, int maxsize)
{
  int j = 0;

  while (j < maxsize && buf[j])
    ++j;

  return j;
}



#ifdef RP_PRINTF_ENABLED
static const char *getmonth(int m)
{
  static const char month[][4] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };

  return (m < 1 || m > 12)? inv : month[m-1];
}



static int ratio(ulg uc, ulg c)   /* GRR 19970621:  swiped from UnZip 5.31 list.c */
{
    ulg denom;

    if (uc == 0)
        return 0;
    if (uc > 2000000L) {    /* risk signed overflow if multiply numerator */
        denom = uc / 1000L;
        return ((uc >= c) ?
            (int) ((uc-c + (denom>>1)) / denom) :
          -((int) ((c-uc + (denom>>1)) / denom)));
    } else {             /* ^^^^^^^^ rounding */
        denom = uc;
        return ((uc >= c) ?
            (int) ((1000L*(uc-c) + (denom>>1)) / denom) :
          -((int) ((1000L*(c-uc) + (denom>>1)) / denom)));
    }                            /* ^^^^^^^^ rounding */
}
#endif /* RP_PRINTF_ENABLED */



/* GRR 20050724:  Implementation of Euclidean Algorithm to find greatest
 *                common factor of two positive integers.
 *                (see http://mathworld.wolfram.com/EuclideanAlgorithm.html)
 */
static ulg gcf(ulg a, ulg b)
{
    ulg r;

    if (b == 0)
      return (a == 0)? 1 : a;

    while ((r = a - (a/b)*b) != 0) {
      a = b;
      b = r;
    }

    return b;
}



#if 0 /* rom-properties */
// TODO: Wrapper function for compatibility?
int pngcheck(FILE *fp, const char *fname, int searching, FILE *fpOut)
{
}
#endif /* rom-properties **/



int pngcheck(IRpFile *fp)
{
	unique_ptr<CPngCheck> cpngcheck(new CPngCheck(fp));
	return cpngcheck->pngcheck();
}



int CPngCheck::pngcheck(void)
{
  const char *const fname = nullptr;	// dummy
  FILE *const fpOut = nullptr;		// dummy
  const int searching = 0;		// dummy

  int i, j;
  long sz;  /* FIXME:  should be ulg (not using negative values as flags...) */
  uch magic[8];
  char chunkid[5] = {'\0', '\0', '\0', '\0', '\0'};
  const char *and_str = "";
  int toread;
  int c;
  int have_IHDR = 0, have_IEND = 0;
  int have_MHDR = 0, have_MEND = 0;
  int have_DHDR = 0, have_PLTE = 0;
  int have_JHDR = 0, have_JSEP = 0, need_JSEP = 0;
  int have_IDAT = 0, have_JDAT = 0, last_is_IDAT = 0, last_is_JDAT = 0;
  int have_bKGD = 0, have_cHRM = 0, have_gAMA = 0, have_hIST = 0, have_iCCP = 0;
  int have_oFFs = 0, have_pCAL = 0, have_pHYs = 0, have_sBIT = 0, have_sCAL = 0;
  int have_sRGB = 0, have_sTER = 0, have_tIME = 0, have_tRNS = 0;
  int have_SAVE = 0, have_TERM = 0, have_MAGN = 0, have_pHYg = 0;
  int top_level = 1;
  ulg zhead = 1;   /* 0x10000 indicates both zlib header bytes read */
  ulg crc, filecrc;
  ulg layers = 0L, frames = 0L;
  long num_chunks = 0L;
  long w = 0L, h = 0L;
  long mng_width = 0L, mng_height = 0L;
  int vlc = -1, lc = -1;
  int bitdepth = 0, sampledepth = 0, ityp = 1, jtyp = 0, lace = 0, nplte = 0;
  int jbitd = 0, alphadepth = 0;
#ifdef RP_PRINTF_ENABLED
  int did_stat = 0;
#endif /* RP_PRINTF_ENABLED */
  printbuf_state prbuf_state;
#ifdef RP_PRINTF_ENABLED
  struct stat statbuf;
  int first_file = 1;
  const char *const brief_warn  = color? brief_warn_color  : brief_warn_plain;
  const char *const brief_OK    = color? brief_OK_color    : brief_OK_plain;
  const char *const warnings_detected  = color? warnings_color : warnings_plain;
  const char *const no_errors_detected = color? no_err_color   : no_err_plain;
#endif /* RP_PRINTF_ENABLED */

  // FIXME: Need to use the stack or TLS in order to
  // make this function thread-safe.
  global_error = kOK;

#if RP_PRINTF_ENABLED
  if (verbose || printtext || printpal) {
    printf("%sFile: %s%s%s", first_file? "":"\n", color? COLOR_WHITE_BOLD:"",
      fname, color? COLOR_NORMAL:"");

    if (searching) {
      printf("\n");
    } else {
      stat(fname, &statbuf);   /* know file exists => know stat() successful */
      did_stat = 1;
      /* typecast long since off_t may be 64-bit (e.g., IRIX): */
      printf(" (%ld bytes)\n", (long)statbuf.st_size);
    }
  }

  first_file = 0;
#endif /* RP_PRINTF_ENABLED */
  png = mng = jng = 0;

  if (!searching) {
    int check = 0;

    if (fp->read(magic, 8)!=8) {
      printf("%s  cannot read PNG or MNG signature\n", verbose? "":fname);
      set_err(kCriticalError);
      return global_error;
    }

    if (magic[0]==0 && magic[1]>0 && magic[1]<=64 && magic[2]!=0) {
      if (!quiet)
        printf("%s  (trying to skip MacBinary header)\n", verbose? "":fname);

      if (fp->read(buffer, 120) != 120 || fp->read(magic, 8) != 8) {
        printf("    cannot read past MacBinary header\n");
        set_err(kCriticalError);
      } else if ((check = check_magic(magic, DO_PNG)) == 0) {
        png = 1;
        if (!quiet)
          printf("    this PNG seems to be contained in a MacBinary file\n");
      } else if ((check = check_magic(magic, DO_MNG)) == 0) {
        mng = 1;
        if (!quiet)
          printf("    this MNG seems to be contained in a MacBinary file\n");
      } else if ((check = check_magic(magic, DO_JNG)) == 0) {
        jng = 1;
        if (!quiet)
          printf("    this JNG seems to be contained in a MacBinary file\n");
      } else {
        if (check == 2)
          printf("    this is neither a PNG nor JNG image nor a MNG stream\n");
        set_err(kCriticalError);
      }
    } else if ((check = check_magic(magic, DO_PNG)) == 0) {
      png = 1;
    } else if (check == 1) {   /* bytes 2-4 == "PNG" but others are bad */
      set_err(kCriticalError);
    } else if (check == 2) {   /* not "PNG"; see if it's MNG or JNG instead */
      if ((check = check_magic(magic, DO_MNG)) == 0)
        mng = 1;        /* yup */
      else if (check == 2 && (check = check_magic(magic, DO_JNG)) == 0)
        jng = 1;        /* yup */
      else {
        set_err(kCriticalError);
        if (check == 2)
          printf("%s  this is neither a PNG or JNG image nor a MNG stream\n",
            verbose? "":fname);
      }
    }

    if (is_err(kMinorError))
      return global_error;
  }

  /*-------------------- BEGINNING OF IMMENSE WHILE-LOOP --------------------*/

#ifdef USE_ZLIB
  // This used to be at the beginning of the zlib code,
  // but we can't use 'static' because this function
  // needs to be reentrant and thread-safe.
  // NOTE: uch *p must be initialized; otherwise, MSVC 2015
  // release builds fail due to potential use of an uninitialized
  // variable. (/sdl option)
  uch *p = nullptr;   /* always points to next filter byte */
  int cur_y = 0, cur_pass = 0, cur_xoff = 0, cur_yoff = 0, cur_xskip = 0, cur_yskip = 0;
  long cur_width = 0, cur_linebytes = 0;
  long numfilt = 0, numfilt_this_block = 0, numfilt_total = 0, numfilt_pass[7] = {0, 0, 0, 0, 0, 0, 0};
  uch *eod = nullptr;
  int err=Z_OK;
#endif

  while ((c = fp->getc()) != EOF) {
    fp->ungetc(c);

    if (((png || jng) && have_IEND) || (mng && have_MEND)) {
      if (searching) /* start looking again in the next file */
        return global_error;

      if (!quiet)
        printf("%s  additional data after %cEND chunk\n", verbose? "":fname,
          mng? 'M':'I');

      set_err(kMinorError);
      if (!force)
        return global_error;
    }

    sz = getlong("chunk length");

    if (is_err(kMajorError))  /* FIXME:  return only if !force? */
      return global_error;

    if (sz < 0 || sz > 0x7fffffff) {   /* FIXME:  convert to ulg, lose "< 0" */
      printf("%s  invalid chunk length (too large)\n", verbose? ":":fname);
      set_err(kMajorError);
      /* if (!force) */  /* code not yet vetted for negative sz */
        return global_error;
    }

    if (fp->read(chunkid, 4) != 4) {
      printf("%s  EOF while reading chunk type\n", verbose? ":":fname);
      set_err(kCriticalError);
      return global_error;
    }

    /* GRR:  add 4-character EBCDIC conversion here (chunkid) */

    chunkid[4] = '\0';
    ++num_chunks;

    if (check_chunk_name(chunkid) != 0) {
      set_err(kMajorError);
      if (!force)
        return global_error;
    }

    if (verbose)
      printf("  chunk %s%s%s at offset 0x%05lx, length %ld",
        color? COLOR_YELLOW:"", chunkid, color? COLOR_NORMAL:"",
        fp->tell()-4, sz);

    if (is_err(kMajorError))
      return global_error;

    crc = update_crc(CRCINIT, (uch *)chunkid, 4);

    if ((png && !have_IHDR && strcmp(chunkid,"IHDR")!=0) ||
        (mng && !have_MHDR && strcmp(chunkid,"MHDR")!=0) ||
        (jng && !have_JHDR && strcmp(chunkid,"JHDR")!=0))
    {
      printf("%s  first chunk must be %cHDR\n",
        verbose? ":":fname, png? 'I' : (mng? 'M':'J'));
      set_err(kMinorError);
      if (!force)
        return global_error;
    }

    toread = (sz > BS)? BS:sz;

    if (fp->read(buffer, (size_t)toread) != (size_t)toread) {
      printf("%s  EOF while reading %s%sdata\n",
        verbose? ":":fname, verbose? "":chunkid, verbose? "":" ");
      set_err(kCriticalError);
      return global_error;
    }

    crc = update_crc(crc, (uch *)buffer, toread);

    /*================================*
     * PNG, JNG and MNG header chunks *
     *================================*/

    /*------*
     | IHDR |
     *------*/
    if (strcmp(chunkid, "IHDR") == 0) {
      if (png && have_IHDR) {
        printf("%s  multiple IHDR not allowed\n", verbose? ":":fname);
        set_err(kMinorError);
      } else if (sz != 13) {
        printf("%s  invalid %slength\n",
          verbose? ":":fname, verbose? "":"IHDR ");
        set_err(kMajorError);
      }
      if (no_err(kMinorError)) {
        int compr, filt;

        w = LG(buffer);
        h = LG(buffer+4);
        if (w <= 0 || h <= 0 || w > 2147483647 || h > 2147483647) {
          printf("%s  invalid %simage dimensions (%ldx%ld)\n",
            verbose? ":":fname, verbose? "":"IHDR ", w, h);
          set_err(kMinorError);
        }
        bitdepth = sampledepth = (uch)buffer[8];
        ityp = (uch)buffer[9];
        if (ityp == 1 || ityp == 5 || ityp > (int)(sizeof(png_type)/sizeof(char*))) {
          printf("%s  invalid %simage type (%d)\n",
            verbose? ":":fname, verbose? "":"IHDR ", ityp);
          ityp = 1; /* avoid out of range array index */
          set_err(kMinorError);
        }
        switch (sampledepth) {
          case 1:
          case 2:
          case 4:
            if (ityp == 2 || ityp == 4 || ityp == 6) { /* RGB or GA or RGBA */
              printf("%s  invalid %ssample depth (%d) for %s image\n",
                verbose? ":":fname, verbose? "":"IHDR ", sampledepth,
                png_type[ityp]);
              set_err(kMinorError);
            }
            break;
          case 8:
            break;
          case 16:
            if (ityp == 3) { /* palette */
              printf("%s  invalid %ssample depth (%d) for %s image\n",
                verbose? ":":fname, verbose? "":"IHDR ", sampledepth,
                png_type[ityp]);
              set_err(kMinorError);
            }
            break;
          default:
            printf("%s  invalid %ssample depth (%d)\n",
              verbose? ":":fname, verbose? "":"IHDR ", sampledepth);
            set_err(kMinorError);
            break;
        }
        compr = (uch)buffer[10];
        if (compr > 127) {
          printf("%s  private (invalid?) %scompression method (%d) "
            "(warning)\n", verbose? ":":fname, verbose? "":"IHDR ", compr);
          set_err(kWarning);
        } else if (compr > 0) {
          printf("%s  invalid %scompression method (%d)\n",
            verbose? ":":fname, verbose? "":"IHDR ", compr);
          set_err(kMinorError);
        }
        filt = (uch)buffer[11];
        if (filt > 127) {
          printf("%s  private (invalid?) %sfilter method (%d) "
            "(warning)\n", verbose? ":":fname, verbose? "":"IHDR ", filt);
          set_err(kWarning);
        } else if (filt > 0 && !(mng && (ityp == 2 || ityp == 6) && filt == 64))
        {
          printf("%s  invalid %sfilter method (%d)\n",
            verbose? ":":fname, verbose? "":"IHDR ", filt);
          set_err(kMinorError);
        }
        lace = (uch)buffer[12];
        if (lace > 127) {
          printf("%s  private (invalid?) %sinterlace method (%d) "
            "(warning)\n", verbose? ":":fname, verbose? "":"IHDR ", lace);
          set_err(kWarning);
        } else if (lace > 1) {
          printf("%s  invalid %sinterlace method (%d)\n",
            verbose? ":":fname, verbose? "":"IHDR ", lace);
          set_err(kMinorError);
        }
        switch (ityp) {
          case 2:
            bitdepth = sampledepth * 3;   /* RGB */
            break;
          case 4:
            bitdepth = sampledepth * 2;   /* gray+alpha */
            break;
          case 6:
            bitdepth = sampledepth * 4;   /* RGBA */
            break;
        }
        if (verbose && no_err(kMinorError)) {
          printf("\n    %ld x %ld image, %d-bit %s, %sinterlaced\n", w, h,
            bitdepth, png_type[ityp], lace? "":"non-");
        }
      }
      have_IHDR = 1;
      if (mng)
        top_level = 0;
      last_is_IDAT = last_is_JDAT = 0;
#ifdef USE_ZLIB
      first_idat = 1;  /* flag:  next IDAT will be the first in this subimage */
      zlib_error = 0;  /* flag:  no zlib errors yet in this file */
      /* GRR 20000304:  data dump not yet compatible with interlaced images: */
      if (lace && verbose > 3)  /* (FIXME eventually...or move to pngcrunch) */
        verbose = 2;
#endif

    /*------*
     | JHDR |
     *------*/
    } else if (strcmp(chunkid, "JHDR") == 0) {
      if (png) {
        printf("%s  JHDR not defined in PNG\n", verbose? ":":fname);
        set_err(kMinorError);
      } else if (jng && have_JHDR) {
        printf("%s  multiple JHDR not allowed\n", verbose? ":":fname);
        set_err(kMinorError);
      } else if (sz != 16) {
        printf("%s  invalid %slength\n",
          verbose? ":":fname, verbose? "":"JHDR ");
        set_err(kMajorError);
      }
      if (no_err(kMinorError)) {
        w = LG(buffer);
        h = LG(buffer+4);
        if (w == 0 || h == 0) {
          printf("%s  invalid %simage dimensions (%ldx%ld)\n",
            verbose? ":":fname, verbose? "":"JHDR ", w, h);
          set_err(kMinorError);
        }
        jtyp = (uch)buffer[8];
        if (jtyp != 8 && jtyp != 10 && jtyp != 12 && jtyp != 14) {
          printf("%s  invalid %scolor type\n",
            verbose? ":":fname, verbose? "":"JHDR ");
          set_err(kMinorError);
        } else {
          jtyp = (jtyp >> 1) - 4;   /* now 0,1,2,3:  index into jng_type[] */
          bitdepth = (uch)buffer[9];
          if (bitdepth != 8 && bitdepth != 12 && bitdepth != 20) {
            printf("%s  invalid %sbit depth (%d)\n",
              verbose? ":":fname, verbose? "":"JHDR ", bitdepth);
            set_err(kMinorError);
          } else if (buffer[10] != 8) {
            printf("%s  invalid %sJPEG compression method (%d)\n",
              verbose? ":":fname, verbose? "":"JHDR ", buffer[10]);
            set_err(kMinorError);
          } else if (buffer[13] != 0) {
            printf("%s  invalid %salpha-channel compression method (%d)\n",
              verbose? ":":fname, verbose? "":"JHDR ", buffer[13]);
            set_err(kMinorError);
          } else if (buffer[14] != 0) {
            printf("%s  invalid %salpha-channel filter method (%d)\n",
              verbose? ":":fname, verbose? "":"JHDR ", buffer[14]);
            set_err(kMinorError);
          } else if (buffer[15] != 0) {
            printf("%s  invalid %salpha-channel interlace method (%d)\n",
              verbose? ":":fname, verbose? "":"JHDR ", buffer[15]);
            set_err(kMinorError);
          } else if ((lace = (uch)buffer[11]) != 0 && lace != 8) {
            printf("%s  invalid %sJPEG interlace method (%d)\n",
              verbose? ":":fname, verbose? "":"JHDR ", lace);
            set_err(kMinorError);
          } else {
            int a;

            if (bitdepth == 20) {
              need_JSEP = 1;
              jbitd = 8;
              and_str = "and 12-bit ";
            } else
              jbitd = bitdepth;
            a = alphadepth = buffer[12];
            if ((a != 1 && a != 2 && a != 4 && a != 8 && a != 16 &&
                       jtyp > 1) || (a != 0 && jtyp < 2))
            {
              printf("%s  invalid %salpha-channel bit depth (%d) for %s image\n"
                , verbose? ":":fname, verbose? "":"JHDR ", alphadepth,
                jng_type[jtyp]);
              set_err(kMinorError);
            } else if (verbose && no_err(kMinorError)) {
              if (jtyp < 2)
                printf("\n    %ld x %ld image, %d-bit %s%s%s\n",
                  w, h, jbitd, and_str, jng_type[jtyp], lace? ", progressive":"");
              else
                printf("\n    %ld x %ld image, %d-bit %s%s + %d-bit alpha%s\n",
                  w, h, jbitd, and_str, jng_type[jtyp-2], alphadepth,
                  lace? ", progressive":"");
            }
          }
        }
      }
      have_JHDR = 1;
      if (mng)
        top_level = 0;
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | MHDR |
     *------*/
    } else if (strcmp(chunkid, "MHDR") == 0) {
      if (png || jng) {
        printf("%s  MHDR not defined in %cNG\n", verbose? ":":fname,
          png? 'P':'J');
        set_err(kMinorError);
      } else
      if (have_MHDR) {
        printf("%s  multiple MHDR not allowed\n", verbose? ":":fname);
        set_err(kMinorError);
      } else if (sz != 28) {
        printf("%s  invalid %slength\n",
               verbose? ":":fname, verbose? "":"MHDR ");
        set_err(kMajorError);
      }
      if (no_err(kMinorError)) {
        ulg tps, playtime, profile;
        int validtrans = 0;

        mng_width  = w = LG(buffer);
        mng_height = h = LG(buffer+4);
        tps = LG(buffer+8);
        layers = LG(buffer+12);
        frames = LG(buffer+16);
        playtime = LG(buffer+20);
        profile = LG(buffer+24);
        if (verbose) {
          printf("\n    %lu x %lu frame size, ", w, h);
          if (tps)
            printf("%lu tick%s per second, ", tps, (tps == 1L)? "" : "s");
          else
            printf("infinite tick length, ");
          if (layers && layers < 0x7ffffffL)
            printf("%lu layer%s,\n", layers, (layers == 1L)? "" : "s");
          else
            printf("%s layer count,\n", layers? "infinite" : "unspecified");
          if (frames && frames < 0x7ffffffL)
            printf("    %lu frame%s, ", frames, (frames == 1L)? "" : "s");
          else
            printf("    %s frame count, ", frames? "infinite" : "unspecified");
          if (playtime && playtime < 0x7ffffffL) {
            printf("%lu-tick play time ", playtime);
            if (tps)
              printf("(%lu seconds), ", (playtime + (tps >> 1)) / tps);
            else
              printf(", ");
          } else
            printf("%s play time, ", playtime? "infinite" : "unspecified");
        }
        if (profile & 0x0001) {
          int bits = 0;

          vlc = lc = 1;
          if (verbose)
            printf("valid profile:\n      ");
          if (profile & 0x0002) {
            if (verbose)
              printf("simple MNG features");
            ++bits;
            vlc = 0;
          }
          if (profile & 0x0004) {
            if (verbose)
              printf("%scomplex MNG features", bits? ", " : "");
            ++bits;
            vlc = 0;
            lc = 0;
          }
          if (profile & 0x0008) {
            if (verbose)
              printf("%scritical transparency", bits? ", " : "");
            ++bits;
          }
          if (profile & 0x0010) {
            if (verbose)
              printf("%s%sJNG", bits? ", " : "", (bits == 3)? "\n      " : "");
            ++bits;
            vlc = 0;
            lc = 0;
          }
          if (profile & 0x0020) {
            if (verbose)
              printf("%s%sdelta-PNG", bits? ", " : "",
                (bits == 3)? "\n      " : "");
            ++bits;
            vlc = 0;
            lc = 0;
          }
          if (profile & 0x0040) {
            if (verbose)
              printf("%s%svalid transparency info", bits? ", " : "",
                (bits > 0 && (bits % 3) == 0)? "\n      " : "");
            ++bits;
            validtrans = 1;
          }
          if (/* validtrans && */ profile & 0x0080) {
            if (verbose)
              printf("%s%smay have bkgd transparency", bits? ", " : "",
                (bits > 0 && (bits % 3) == 0)? "\n      " : "");
            ++bits;
          }
          if (/* validtrans && */ profile & 0x0100) {
/*
FIXME: also check bit 3 (0x0008); if not set, this one is meaningless
 */
            if (verbose)
              printf("%s%smay have semi-transparency", bits? ", " : "",
                (bits > 0 && (bits % 3) == 0)? "\n      " : "");
            ++bits;
          }
          if (/* validtrans && */ profile & 0x0200) {
            if (verbose)
              printf("%s%sobject buffers must be stored", bits? ", " : "",
                (bits > 0 && (bits % 3) == 0)? "\n      " : "");
            ++bits;
            vlc = 0;
            lc = 0;
          }
          if (profile & 0xfc00) {
/*
FIXME:  error/strong warning
 */
            if (verbose)
              printf("%s%sreserved bits are set", bits? ", " : "",
                (bits > 0 && (bits % 3) == 0)? "\n      " : "");
            ++bits;
            vlc = 0;
            lc = 0;
          }
          if (profile & 0x7fff0000) {
            if (verbose)
              printf("%s%sprivate/experimental bits are set", bits? ", " : "",
                (bits > 0 && (bits % 3) == 0)? "\n      " : "");
            ++bits;
            vlc = 0;
            lc = 0;
          }
/*
FIXME: make sure bit 31 (0x80000000) is 0
 */
          if (verbose) {
            if (vlc)
              printf("%s (MNG-VLC)", bits? "":"no feature bits specified");
            else if (lc)
              printf(" (MNG-LC)");
            printf("\n");
          }
        } else {
          vlc = lc = -1;
          if (verbose)
            printf("%s\n    simplicity profile\n",
              profile? "invalid" : "unspecified");
        }
      }
      have_MHDR = 1;
      last_is_IDAT = last_is_JDAT = 0;

    /*================================================*
     * PNG chunks (with the exception of IHDR, above) *
     *================================================*/

    /*------*
     | PLTE |
     *------*/
    } else if (strcmp(chunkid, "PLTE") == 0) {
      if (jng) {
        printf("%s  PLTE not defined in JNG\n", verbose? ":":fname);
        set_err(kMinorError);
      } else if (png && have_PLTE) {
        printf("%s  multiple PLTE not allowed\n", verbose? ":":fname);
        set_err(kMinorError);
      } else if (png && ityp != 3 && ityp != 2 && ityp != 6) {
        printf("%s  PLTE not allowed in %s image\n", verbose? ":":fname,
          png_type[ityp]);
        set_err(kMinorError);
      } else if (png && have_IDAT) {
        printf("%s  %smust precede IDAT\n",
               verbose? ":":fname, verbose? "":"PLTE ");
        set_err(kMinorError);
      } else if (png && have_bKGD) {
        printf("%s  %smust precede bKGD\n",
               verbose? ":":fname, verbose? "":"PLTE ");
        set_err(kMinorError);
      } else if ((!(mng && have_PLTE) && sz < 3) || sz > 768 || sz % 3 != 0) {
        printf("%s  invalid number of %sentries (%g)\n",
               verbose? ":":fname, verbose? "":"PLTE ", (double)sz / 3);
        set_err(kMinorError); /* was kMajorError, but should be able to cont. */
      } else {
        nplte = sz / 3;
        if (!(mng && have_PLTE) && ((bitdepth == 1 && nplte > 2) ||
            (bitdepth == 2 && nplte > 4) || (bitdepth == 4 && nplte > 16)))
        {
          printf("%s  invalid number of %sentries (%d) for %d-bit image\n",
                 verbose? ":":fname, verbose? "":"PLTE ", nplte, bitdepth);
          set_err(kMinorError);
        }
      }
/*
      else if (printpal && !verbose)
        printf("\n");
 */
      if (no_err(kMinorError)) {
        if (ityp == 1)   /* for MNG and tRNS */
          ityp = 3;
        if (verbose || (printpal && !quiet)) {
          if (!verbose && printpal && !quiet)
            printf("  PLTE chunk");
          printf(": %d palette entr%s\n", nplte, nplte == 1? "y":"ies");
        }
        if (printpal) {
          const char *spc;

          if (nplte < 10)
            spc = "  ";
          else if (nplte < 100)
            spc = "   ";
          else
            spc = "    ";
          for (i = j = 0; i < nplte; ++i, j += 3)
            printf("%s%3d:  (%3d,%3d,%3d) = (0x%02x,0x%02x,0x%02x)\n", spc, i,
                   buffer[j], buffer[j + 1], buffer[j + 2],
                   buffer[j], buffer[j + 1], buffer[j + 2]);
        }
      }
      have_PLTE = 1;
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | IDAT |
     *------*/
    } else if (strcmp(chunkid, "IDAT") == 0) {
      /* GRR FIXME:  need to check for consecutive IDATs within MNG segments */
      if (have_IDAT && !last_is_IDAT) {
        if (mng) {  /* reset things (SEMI-HACK; check for segments instead!) */
          have_IDAT = 0;
#ifdef USE_ZLIB
          zlib_error = 0;
          zlib_windowbits = 15;
#endif
          zhead = 1;
          if (verbose)
            printf("\n");
        } else {
          printf("%s  IDAT chunks must be consecutive\n", verbose? ":":fname);
          set_err(kMajorError);
        }
      } else if (png && ityp == 3 && !have_PLTE) {
        printf("%s  %smust follow PLTE in %s image\n",
          verbose? ":":fname, verbose? "":"IDAT ", png_type[ityp]);
        set_err(kMajorError);
      } else if (verbose)
        printf("\n");

      if (!no_err(kMinorError) && !force)
        return global_error;

      /* We just want to check that we have read at least the minimum (10)
       * IDAT bytes possible, but avoid any overflow for short ints.  We
       * must also take into account that 0-length IDAT chunks are legal.
       */
      if (have_IDAT <= 0)
        have_IDAT = (sz > 0)? sz : -1;  /* -1 as marker for IDAT(s), no data */
      else if (have_IDAT < 10)
        have_IDAT += (sz > 10)? 10 : sz;  /* FIXME? could cap at 10 always */

      /* Dump the zlib header from the first two bytes. */
      if (zhead < 0x10000 && sz > 0) {
        zhead = (zhead << 8) + buffer[0];
        if (sz > 1 && zhead < 0x10000)
          zhead = (zhead << 8) + buffer[1];
        if (zhead >= 0x10000) {
          /* formerly print_zlibheader(zhead & 0xffff); */
          /* See the code in zlib deflate.c that writes out the header when
             s->status is INIT_STATE.  In fact this code is based on the zlib
             specification in RFC 1950 (ftp://ds.internic.net/rfc/rfc1950.txt),
             with the implicit assumption that the zlib header *is* written (it
             always should be inside a valid PNG file).  The variable names are
             taken, verbatim, from the RFC. */
          unsigned int CINFO = (zhead & 0xf000) >> 12;

#ifdef USE_ZLIB
          if (check_windowbits)   /* check for libpng 1.2.6 windowBits bug */
            zlib_windowbits = CINFO + 8;
#endif
          if (verbose) {
            unsigned int CM = (zhead & 0xf00) >> 8;
            unsigned int FDICT = (zhead & 0x20) >> 5;
            unsigned int FLEVEL = (zhead & 0xc0) >> 6;

            printf("    zlib: ");
            if ((zhead & 0xffff) % 31) {
              printf("compression header fails checksum\n");
              set_err(kMajorError);
            } else if (CM == 8) {
              if (CINFO > 1) {
                printf("deflated, %dK window, %s compression%s\n",
                  (1 << (CINFO-2)), deflate_type[FLEVEL],
                  FDICT? ", preset dictionary":"");
              } else {
                printf("deflated, %d-byte window, %s compression%s\n",
                  (1 << (CINFO+8)), deflate_type[FLEVEL],
                  FDICT? ", preset dictionary":"");
              }
            } else {
              printf("non-deflate compression method (%d)\n", CM);
              set_err(kMajorError);
            }
          }

        }
      }

#ifdef USE_ZLIB
      if (check_zlib && !zlib_error) {
        // Variables moved outside of the 'while' loop.
        // We can't use 'static' here because that's not
        // reentrant or thread-safe.
        zstrm.next_in = buffer;
        zstrm.avail_in = toread;

// FIXME!  when inflate error and force, need to skip over rest of IDAT
        /* initialize zlib and bit/byte/line variables if not already done */
        if (first_idat) {
          zstrm.next_out = p = outbuf;
          zstrm.avail_out = BS;
          zstrm.zalloc = (alloc_func)Z_NULL;
          zstrm.zfree = (free_func)Z_NULL;
          zstrm.opaque = (voidpf)Z_NULL;
          if ((err = inflateInit2(&zstrm, zlib_windowbits)) != Z_OK) {
            printf("\n    zlib: oops! can't initialize (error = %d)\n", err);
            zlib_error = 1;
            /* actually, fatal error for subsequent PNGs, too; could (should?)
               return here... */
          }
          cur_y = 0;
          cur_pass = 1;     /* interlace pass:  1 through 7 */
          cur_xoff = cur_yoff = 0;
          cur_xskip = cur_yskip = lace? 8 : 1;
          cur_width = (w - cur_xoff + cur_xskip - 1) / cur_xskip; /* round up */
          cur_linebytes = ((cur_width*bitdepth + 7) >> 3) + 1; /* round, fltr */
          numfilt = 0L;
          first_idat = 0;
          if (lace) {   /* loop through passes to calculate total filters */
            int passm1, yskip=0, yoff=0, xoff=0;

            if (verbose)  /* GRR FIXME? could move this calc outside USE_ZLIB */
              printf("    rows per pass%s: ",
                (lace > 1)? " (assuming Adam7-like interlacing)":"");
            for (passm1 = 0;  passm1 < 7;  ++passm1) {
              switch (passm1) {  /* (see table below for full summary) */
                case 0:  yskip = 8; yoff = 0; xoff = 0; break;
                case 1:  yskip = 8; yoff = 0; xoff = 4; break;
                case 2:  yskip = 8; yoff = 4; xoff = 0; break;
                case 3:  yskip = 4; yoff = 0; xoff = 2; break;
                case 4:  yskip = 4; yoff = 2; xoff = 0; break;
                case 5:  yskip = 2; yoff = 0; xoff = 1; break;
                case 6:  yskip = 2; yoff = 1; xoff = 0; break;
              }
              /* effective height is reduced if odd pass:  subtract yoff (but
               * if effective width of pass is 0 => no rows and no filters) */
              numfilt_pass[passm1] =
                (w <= xoff)? 0 : (h - yoff + yskip - 1) / yskip;
              if (verbose) {
                /* colors here are handy as a key if row-filters are being
                 * printed, but otherwise they're a bit too busy */
                printf("%s%s%ld%s", passm1? ", ":"",
                  (verbose > 1)? pass_color[passm1+1]:"",
                  numfilt_pass[passm1], (verbose > 1)? color_off:"");
              }
              if (passm1 > 0)  /* now make it cumulative */
                numfilt_pass[passm1] += numfilt_pass[passm1 - 1];
            }
            if (verbose)
              printf("\n");
          } else {
            numfilt_pass[0] = h;   /* if non-interlaced */
            numfilt_pass[1] = numfilt_pass[2] = numfilt_pass[3] = h;
            numfilt_pass[4] = numfilt_pass[5] = numfilt_pass[6] = h;
          }
          numfilt_total = numfilt_pass[6];
        }

        if (verbose > 1) {
          printf("    row filters (0 none, 1 sub, 2 up, 3 avg, "
            "4 paeth)%s:\n     %s", verbose > 3? " and data" : "",
            pass_color[cur_pass]);
        }
        numfilt_this_block = 0L;

        while (err != Z_STREAM_END && zstrm.avail_in > 0) {
          /* know zstrm.avail_out > 0:  get some image/filter data */
          err = inflate(&zstrm, Z_SYNC_FLUSH);
          if (err != Z_OK && err != Z_STREAM_END) {
            printf("%s  zlib: inflate error = %d (%s)\n",
              verbose > 1? "\n  " : (verbose == 1? "  ":fname), err,
              (-err < 1 || -err > 6)? "unknown":zlib_error_type[-err-1]);
            zlib_error = 1;		/* fatal error only for this PNG */
            break;			/* kill inner loop */
          }

          /* now have uncompressed, filtered image data in outbuf */
          eod = outbuf + BS - zstrm.avail_out;
          while (p < eod) {

            if (cur_linebytes) {	/* GRP 20000727:  bugfix */
              int filttype = p[0];
              if (filttype > 127) {
                if (lace > 1)
                  break;  /* assume it's due to unknown interlace method */
                if (numfilt_this_block == 0) {
                  /* warn only on first one per block; don't break */
                  printf("%s  private (invalid?) %srow-filter type (%d) "
                    "(warning)\n", verbose? "\n  ":fname, verbose? "":"IDAT ",
                    filttype);
                  set_err(kWarning);
                }
              } else if (filttype > 4) {
                if (lace <= 1) {
                  printf("%s  invalid %srow-filter type (%d)\n",
                    verbose? "  ":fname, verbose? "":"IDAT ", filttype);
                  set_err(kMinorError);
                } /* else assume it's due to unknown interlace method */
                break;
              }
              if (verbose > 3) {	/* GRR 20000304 */
                printf(" [%1d]", filttype);
                fflush(stdout);
                ++numfilt;
                for (i = 1;  i < cur_linebytes;  ++i, ++p) {
                  printf(" %d", (int)p[1]);
                  fflush(stdout);
                }
                ++p;
                printf("\n     ");
                fflush(stdout);
              } else {
                if (verbose > 1) {
                  printf(" %1d", filttype);
                  if (++numfilt_this_block % 25 == 0)
                    printf("\n     ");
                }
                ++numfilt;
                if (lace && verbose > 1) {
                  int passm1, cur_pass_delta=0;

                  for (passm1 = 0;  passm1 < 6;  ++passm1) {  /* omit pass 7 */
                    if (numfilt == numfilt_pass[passm1]) {
                      ++cur_pass_delta;
                      if (color) {
                        printf("%s %s|", COLOR_NORMAL, COLOR_WHITE_BOLD);
                      } else {
                        printf(" |");
                      }
                      if (++numfilt_this_block % 25 == 0) /* pretend | is one */
                        printf("%s\n     %s", color_off,
                          pass_color[cur_pass + cur_pass_delta]);
                    }
                  }
                  if (numfilt_this_block % 25)  /* else already did this */
                    printf("%s%s", color_off,
                      pass_color[cur_pass + cur_pass_delta]);
                }
                p += cur_linebytes;
              }
            }
            cur_y += cur_yskip;

            if (lace) {
              while (cur_y >= h) {	/* may loop if very short image */
                /*
                    pass  xskip yskip  xoff yoff
                      1     8     8      0    0
                      2     8     8      4    0
                      3     4     8      0    4
                      4     4     4      2    0
                      5     2     4      0    2
                      6     2     2      1    0
                      7     1     2      0    1
                 */
                ++cur_pass;
                if (cur_pass & 1) {	/* beginning an odd pass */
                  cur_yoff = cur_xoff;
                  cur_xoff = 0;
                  cur_xskip >>= 1;
                } else {		/* beginning an even pass */
                  if (cur_pass == 2)
                    cur_xoff = 4;
                  else {
                    cur_xoff = cur_yoff >> 1;
                    cur_yskip >>= 1;
                  }
                  cur_yoff = 0;
                }
                cur_y = cur_yoff;
                /* effective width is reduced if even pass: subtract cur_xoff */
                cur_width = (w - cur_xoff + cur_xskip - 1) / cur_xskip;
                cur_linebytes = ((cur_width*bitdepth + 7) >> 3) + 1;
                if (cur_linebytes == 1)	/* just the filter byte?  no can do */
                    cur_linebytes = 0;	/* GRP 20000727:  added fix */
              }
            } else if (cur_y >= h) {
              if (verbose > 3) {	/* GRR 20000304:  bad code */
                printf(" %td bytes remaining in buffer before inflateEnd()",
                  eod-p);		// ptrdiff_t
                printf("\n     ");
                fflush(stdout);
                i = inflateEnd(&zstrm);	/* we're all done */
                if (i == Z_OK || i == Z_STREAM_ERROR)
                  printf(" inflateEnd() returns %s\n     ",
                    i == Z_OK? "Z_OK" : "Z_STREAM_ERROR");
                else
                  printf(" inflateEnd() returns %d\n     ", i);
                fflush(stdout);
              } else
                inflateEnd(&zstrm);	/* we're all done */
              zlib_error = -1;		/* kill outermost loop (over chunks) */
              err = Z_STREAM_END;	/* kill middle loop */
              break;			/* kill innermost loop */
            }
          }
          p -= (eod - outbuf);		/* wrap p back into outbuf region */
          zstrm.next_out = outbuf;
          zstrm.avail_out = BS;

          /* get more input (waiting until buffer empties is not necessary best
           * zlib strategy, but simpler than shifting leftover data around) */
          if (zstrm.avail_in == 0 && sz > toread) {
            int data_read;

            sz -= toread;
            toread = (sz > BS)? BS:sz;
            if ((data_read = (int)fp->read(buffer, toread)) != toread) {
              printf("\nEOF while reading %s data\n", chunkid);
              set_err(kCriticalError);
              return global_error;
            }
            crc = update_crc(crc, buffer, toread);
            zstrm.next_in = buffer;
            zstrm.avail_in = toread;
          }
        }
        if (verbose > 1 && no_err(kMinorError))
          printf("%s (%ld out of %ld)\n", color_off, numfilt, numfilt_total);
      }
      if (zlib_error > 0)  /* our flag, not zlib's (-1 means normal exit) */
        set_err(kMajorError);
#endif /* USE_ZLIB */
      last_is_IDAT = 1;
      last_is_JDAT = 0;

    /*------*
     | IEND |
     *------*/
    } else if (strcmp(chunkid, "IEND") == 0) {
      if (!mng && have_IEND) {
        printf("%s  multiple IEND not allowed\n", verbose? ":":fname);
        set_err(kMinorError);
      } else if (sz != 0) {
        printf("%s  invalid %slength\n",
          verbose? ":":fname, verbose? "":"IEND ");
        set_err(kMinorError);
      } else if (jng && need_JSEP && !have_JSEP) {
        printf("%s  missing JSEP in 20-bit JNG\n", verbose? ":":fname);
        set_err(kMinorError);
      } else if (jng && have_JDAT <= 0) {
        printf("%s  no JDAT chunks\n", verbose? ":":fname);
        set_err(kMajorError);
/*
 *    FIXME:  what's minimum valid JPEG/JFIF length?
 *    } else if (jng && have_JDAT < 10) {
 *      printf("%s  not enough JDAT data\n", verbose? ":":fname);
 *      set_err(kMajorError);
 */
      } else if (png && have_IDAT <= 0) {
        printf("%s  no IDAT chunks\n", verbose? ":":fname);
        set_err(kMajorError);
      } else if (png && have_IDAT < 10) {
        printf("%s  not enough IDAT data\n", verbose? ":":fname);
        set_err(kMajorError);
      } else if (verbose) {
        printf("\n");
      }
      have_IEND = 1;
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | bKGD |
     *------*/
    } else if (strcmp(chunkid, "bKGD") == 0) {
      if (!mng && have_bKGD) {
        printf("%s  multiple bKGD not allowed\n", verbose? ":":fname);
        set_err(kMinorError);
      } else if (!mng && (have_IDAT || have_JDAT)) {
        printf("%s  %smust precede %cDAT\n",
               verbose? ":":fname, verbose? "":"bKGD ", have_IDAT? 'I':'J');
        set_err(kMinorError);
      }
      switch (ityp) {
        case 0:
        case 4:
          if (sz != 2) {
            printf("%s  invalid %slength\n",
              verbose? ":":fname, verbose? "":"bKGD ");
            set_err(kMajorError);
          }
          if (verbose && no_err(kMinorError)) {
            printf("\n    gray = 0x%04x\n", SH(buffer));
          }
          break;
        case 1: /* MNG top-level chunk (default values):  "as if 16-bit RGBA" */
        case 2:
        case 6:
          if (sz != 6) {
            printf("%s  invalid %slength\n",
              verbose? ":":fname, verbose? "":"bKGD ");
            set_err(kMajorError);
          }
          if (verbose && no_err(kMinorError)) {
            printf("\n    red = 0x%04x, green = 0x%04x, blue = 0x%04x\n",
              SH(buffer), SH(buffer+2), SH(buffer+4));
          }
          break;
        case 3:
          if (sz != 1) {
            printf("%s  invalid %slength\n",
              verbose? ":":fname, verbose? "":"bKGD ");
            set_err(kMajorError);
          } else if (buffer[0] >= nplte) {
            printf("%s  %sindex (%u) falls outside PLTE (%u)\n",
              verbose? ":":fname, verbose? "":"bKGD ", buffer[0], nplte);
            set_err(kMajorError);
          }
          if (verbose && no_err(kMinorError)) {
            printf("\n    index = %u\n", buffer[0]);
          }
          break;
      }
      have_bKGD = 1;
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | cHRM |
     *------*/
    } else if (strcmp(chunkid, "cHRM") == 0) {
      if (!mng && have_cHRM) {
        printf("%s  multiple cHRM not allowed\n", verbose? ":":fname);
        set_err(kMinorError);
      } else if (!mng && have_PLTE) {
        printf("%s  %smust precede PLTE\n",
               verbose? ":":fname, verbose? "":"cHRM ");
        set_err(kMinorError);
      } else if (!mng && (have_IDAT || have_JDAT)) {
        printf("%s  %smust precede %cDAT\n",
               verbose? ":":fname, verbose? "":"cHRM ", have_IDAT? 'I':'J');
        set_err(kMinorError);
      } else if (sz != 32) {
        printf("%s  invalid %slength\n",
               verbose? ":":fname, verbose? "":"cHRM ");
        set_err(kMajorError);
      }
      if (no_err(kMinorError)) {
        double wx, wy, rx, ry, gx, gy, bx, by;

        wx = (double)LG(buffer)/100000;
        wy = (double)LG(buffer+4)/100000;
        rx = (double)LG(buffer+8)/100000;
        ry = (double)LG(buffer+12)/100000;
        gx = (double)LG(buffer+16)/100000;
        gy = (double)LG(buffer+20)/100000;
        bx = (double)LG(buffer+24)/100000;
        by = (double)LG(buffer+28)/100000;

        if (wx < 0 || wx > 0.8 || wy < 0 || wy > 0.8 || wx + wy > 1.0) {
          printf("%s  invalid %swhite point %0g %0g\n",
                 verbose? ":":fname, verbose? "":"cHRM ", wx, wy);
          set_err(kMinorError);
        } else if (rx < 0 || rx > 0.8 || ry < 0 || ry > 0.8 || rx + ry > 1.0) {
          printf("%s  invalid %sred point %0g %0g\n",
                 verbose? ":":fname, verbose? "":"cHRM ", rx, ry);
          set_err(kMinorError);
        } else if (gx < 0 || gx > 0.8 || gy < 0 || gy > 0.8 || gx + gy > 1.0) {
          printf("%s  invalid %sgreen point %0g %0g\n",
                 verbose? ":":fname, verbose? "":"cHRM ", gx, gy);
          set_err(kMinorError);
        } else if (bx < 0 || bx > 0.8 || by < 0 || by > 0.8 || bx + by > 1.0) {
          printf("%s  invalid %sblue point %0g %0g\n",
                 verbose? ":":fname, verbose? "":"cHRM ", bx, by);
          set_err(kMinorError);
        }
        else if (verbose) {
          printf("\n");
        }

        if (verbose && no_err(kMinorError)) {
          printf("    White x = %0g y = %0g,  Red x = %0g y = %0g\n",
                 wx, wy, rx, ry);
          printf("    Green x = %0g y = %0g,  Blue x = %0g y = %0g\n",
                 gx, gy, bx, by);
        }
      }
      have_cHRM = 1;
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | fRAc |
     *------*/
    } else if (strcmp(chunkid, "fRAc") == 0) {
      if (verbose)
        printf("\n    undefined fractal parameters (ancillary, safe to copy)\n"
          "    [contact Tim Wegner, twegner@phoenix.net, for specification]\n");
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | gAMA |
     *------*/
    } else if (strcmp(chunkid, "gAMA") == 0) {
      if (!mng && have_gAMA) {
        printf("%s  multiple gAMA not allowed\n", verbose? ":":fname);
        set_err(kMinorError);
      } else if (!mng && (have_IDAT || have_JDAT)) {
        printf("%s  %smust precede %cDAT\n",
               verbose? ":":fname, verbose? "":"gAMA ", have_IDAT? 'I':'J');
        set_err(kMinorError);
      } else if (!mng && have_PLTE) {
        printf("%s  %smust precede PLTE\n",
               verbose? ":":fname, verbose? "":"gAMA ");
        set_err(kMinorError);
      } else if (sz != 4) {
        printf("%s  invalid %slength\n",
               verbose? ":":fname, verbose? "":"gAMA ");
        set_err(kMajorError);
      } else if (LG(buffer) == 0) {
        printf("%s  invalid %svalue (0.0000)\n",
               verbose? ":":fname, verbose? "":"gAMA ");
        set_err(kMinorError);
      }
      if (verbose && no_err(kMinorError)) {
        printf(": %#0.5g\n", (double)LG(buffer)/100000);
      }
      have_gAMA = 1;
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | gIFg |
     *------*/
    } else if (strcmp(chunkid, "gIFg") == 0) {
      if (jng) {
        printf("%s  gIFg not defined in JNG\n", verbose? ":":fname);
        set_err(kMinorError);
      } else if (sz != 4) {
        printf("%s  invalid %slength\n",
          verbose? ":":fname, verbose? "":"gIFg ");
        set_err(kMajorError);
      }
      if (verbose && no_err(kMinorError)) {
        double dtime = .01 * SH(buffer+2);

        printf("\n    disposal method = %d, user input flag = %d, display time = %lf seconds\n",
          buffer[0], buffer[1], dtime);
      }
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | gIFt |
     *------*/
    } else if (strcmp(chunkid, "gIFt") == 0) {
      printf("%s  %sDEPRECATED CHUNK\n",
        verbose? ":":fname, verbose? "":"gIFt ");
      set_err(kMinorError);
      if (jng) {
        printf("%s  gIFt not defined in JNG\n", verbose? ":":fname);
        set_err(kMinorError);
      } else if (sz < 24) {
        printf("%s  invalid %slength\n",
          verbose? ":":fname, verbose? "":"gIFt ");
        set_err(kMajorError);
      }
      if (verbose && no_err(kMinorError)) {
        printf("    %ldx%ld-pixel text grid at (%ld,%ld) pixels from upper "
          "left\n", LG(buffer+8), LG(buffer+12), LG(buffer), LG(buffer+4));
        printf("    character cell = %dx%d pixels\n", buffer[16], buffer[17]);
        printf("    foreground color = 0x%02d%02d%02d, background color = "
          "0x%02d%02d%02d\n", buffer[18], buffer[19], buffer[20], buffer[21],
          buffer[22], buffer[23]);
        printf("    %ld bytes of text data\n", sz-24);
        /* GRR:  print text according to grid size/cell size? */
      }
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | gIFx |
     *------*/
    } else if (strcmp(chunkid, "gIFx") == 0) {
      if (jng) {
        printf("%s  gIFx not defined in JNG\n", verbose? ":":fname);
        set_err(kMinorError);
      } else if (sz < 11) {
        printf("%s  invalid %slength\n",
          verbose? ":":fname, verbose? "":"gIFx ");
        set_err(kMajorError);
      }
      if (verbose && no_err(kMinorError)) {
        printf(
          "\n    application ID = %.*s, authentication code = 0x%02x%02x%02x\n",
          8, buffer, buffer[8], buffer[9], buffer[10]);
        printf("    %ld bytes of application data\n", sz-11);
      }
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | hIST |
     *------*/
    } else if (strcmp(chunkid, "hIST") == 0) {
      if (jng) {
        printf("%s  hIST not defined in JNG\n", verbose? ":":fname);
        set_err(kMinorError);
      } else if (png && have_hIST) {
        printf("%s  multiple hIST not allowed\n", verbose? ":":fname);
        set_err(kMinorError);
      } else if (!have_PLTE) {
        printf("%s  %smust follow PLTE\n",
          verbose? ":":fname, verbose? "":"hIST ");
        set_err(kMinorError);
      } else if (png && have_IDAT) {
        printf("%s  %smust precede IDAT\n",
          verbose? ":":fname, verbose? "":"hIST ");
        set_err(kMinorError);
      } else if (sz != nplte * 2) {
        printf("%s  invalid number of %sentries (%g)\n",
          verbose? ":":fname, verbose? "":"hIST ", (double)sz / 2);
        set_err(kMajorError);
      }
      if ((verbose || (printpal && !quiet)) && no_err(kMinorError)) {
        if (!verbose && printpal && !quiet)
          printf("  hIST chunk");
        printf(": %ld histogram entr%s\n", sz / 2, sz/2 == 1? "y":"ies");
      }
      if (printpal && no_err(kMinorError)) {
        const char *spc;

        if (sz < 10)
          spc = "  ";
        else if (sz < 100)
          spc = "   ";
        else
          spc = "    ";
        for (i = j = 0;  j < sz;  ++i, j += 2)
          printf("%s%3d:  %5u\n", spc, i, SH(buffer+j));
      }
      have_hIST = 1;
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | iCCP |
     *------*/
    } else if (strcmp(chunkid, "iCCP") == 0) {
      int name_len;

      if (!mng && have_iCCP) {
        printf("%s  multiple iCCP not allowed\n", verbose? ":":fname);
        set_err(kMinorError);
      } else if (!mng && have_sRGB) {
        printf("%s  %snot allowed with sRGB\n",
               verbose? ":":fname, verbose? "":"iCCP ");
        set_err(kMinorError);
      } else if (!mng && have_PLTE) {
        printf("%s  %smust precede PLTE\n",
               verbose? ":":fname, verbose? "":"iCCP ");
        set_err(kMinorError);
      } else if (!mng && (have_IDAT || have_JDAT)) {
        printf("%s  %smust precede %cDAT\n",
               verbose? ":":fname, verbose? "":"iCCP ", have_IDAT? 'I':'J');
        set_err(kMinorError);
      } else if (check_keyword(buffer, toread, &name_len, "profile name", chunkid)) {
        set_err(kMinorError);
      } else {
        int remainder = toread - name_len - 3;
        uch compr = buffer[name_len+1];

        if (remainder < 0) {
          printf("%s  invalid %slength\n",  /* or input buffer too small */
            verbose? ":":fname, verbose? "":"iCCP ");
          set_err(kMajorError);
        } else if (buffer[name_len] != 0) {
          printf("%s  missing NULL after %sprofile name\n",
            verbose? ":":fname, verbose? "":"iCCP ");
          set_err(kMajorError);
        } else if (compr > 0 && compr < 128) {
          printf("%s  invalid %scompression method (%d)\n",
            verbose? ":":fname, verbose? "":"iCCP ", compr);
          set_err(kMinorError);
        } else if (compr >= 128) {
          set_err(kWarning);
        }
        if (verbose && no_err(kMinorError)) {
          printf("\n    profile name = ");
          init_printbuf_state(&prbuf_state);
          print_buffer(&prbuf_state, buffer, name_len, 0);
          report_printbuf(&prbuf_state, chunkid);
          printf("%scompression method = %d (%s)%scompressed profile = "
            "%ld bytes\n", (name_len > 24)? "\n    ":", ", compr,
            (compr == 0)? "deflate":"private: warning",
            (name_len > 24)? ", ":"\n    ", sz-name_len-2);
                                      /* FIXME: should use remainder instead? */
        }
      }
      have_iCCP = 1;
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | iTXt |
     *------*/
    } else if (strcmp(chunkid, "iTXt") == 0) {
      int keylen;

      if (check_keyword(buffer, toread, &keylen, "keyword", chunkid))
        set_err(kMinorError);
      else {
        int compressed = 0, compr = 0, taglen = 0;

        init_printbuf_state(&prbuf_state);
        if (verbose) {
          printf(", keyword: ");
        }
        if (verbose || printtext) {
          print_buffer(&prbuf_state, buffer, keylen, 0);
        }
        if (verbose)
          printf("\n");
        else if (printtext)
          printf(":\n");
        /* FIXME:  need some size checks here? */
        compressed = buffer[keylen+1];
        if (compressed < 0 || compressed > 1) {
          printf("%s  invalid %scompression flag (%d)\n",
            verbose? ":":fname, verbose? "":"iTXt ", compressed);
          set_err(kMinorError);
        } else if ((compr = (uch)buffer[keylen+2]) > 127) {
          printf("%s  private (invalid?) %scompression method (%d) "
            "(warning)\n", verbose? ":":fname, verbose? "":"iTXt ", compr);
          set_err(kWarning);
        } else if (compr > 0) {
          printf("%s  invalid %scompression method (%d)\n",
            verbose? ":":fname, verbose? "":"iTXt ", compr);
          set_err(kMinorError);
        }
        if (no_err(kMinorError)) {
          taglen = keywordlen(buffer+keylen+3, toread-keylen-3);
          if (verbose) {
            if (taglen > 0) {
              printf("    %scompressed, language tag = ", compressed? "":"un");
              print_buffer(&prbuf_state, buffer+keylen+3, taglen, 0);
            } else {
              printf("    %scompressed, no language tag",
                compressed? "":"un");
            }
            if (buffer[keylen+3+taglen+1] == 0)
              printf("\n    no translated keyword, %ld bytes of UTF-8 text\n",
                sz - (keylen+3+taglen+1));
            else
              printf("\n    %ld bytes of translated keyword and UTF-8 text\n",
                sz - (keylen+3+taglen));
          } else if (printtext) {
            if (buffer[keylen+3+taglen+1] == 0)
              printf("    (no translated keyword, %ld bytes of UTF-8 text)\n",
                sz - (keylen+3+taglen+1));
            else
              printf("    (%ld bytes of translated keyword and UTF-8 text)\n",
                sz - (keylen+3+taglen));
          }
        }
        report_printbuf(&prbuf_state, chunkid);   /* print CR/LF & NULLs info */
      }
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | oFFs |
     *------*/
    } else if (strcmp(chunkid, "oFFs") == 0) {
      if (!mng && have_oFFs) {
        printf("%s  multiple oFFs not allowed\n", verbose? ":":fname);
        set_err(kMinorError);
      } else if (!mng && (have_IDAT || have_JDAT)) {
        printf("%s  %smust precede %cDAT\n",
               verbose? ":":fname, verbose? "":"oFFs ", have_IDAT? 'I':'J');
        set_err(kMinorError);
      } else if (sz != 9) {
        printf("%s  invalid %slength\n",
               verbose? ":":fname, verbose? "":"oFFs ");
        set_err(kMinorError);
      } else if (buffer[8] > 1) {
        printf("%s  invalid %sunit specifier (%u)\n",
          verbose? ":":fname, verbose? "":"oFFs ", buffer[8]);
        set_err(kMinorError);
      }
      if (verbose && no_err(kMinorError)) {
        printf(": %ldx%ld %s offset\n", LG(buffer), LG(buffer+4),
               (buffer[8] == 0)? "pixels":"micrometers");
      }
      have_oFFs = 1;
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | pCAL |
     *------*/
    } else if (strcmp(chunkid, "pCAL") == 0) {
      if (jng) {
        printf("%s  pCAL not defined in JNG\n", verbose? ":":fname);
        set_err(kMinorError);
      } else if (png && have_pCAL) {
        printf("%s  multiple pCAL not allowed\n", verbose? ":":fname);
        set_err(kMinorError);
      } else if (png && have_IDAT) {
        printf("%s  %smust precede IDAT\n",
          verbose? ":":fname, verbose? "":"pCAL ");
        set_err(kMinorError);
      }
      if (no_err(kMinorError)) {
        int name_len;

        if (check_keyword(buffer, toread, &name_len, "calibration name", chunkid))
          set_err(kMinorError);
        else if (sz < name_len + 15) {
          printf("%s  invalid %slength\n",
            verbose? ":":fname, verbose? "":"pCAL ");
          set_err(kMajorError);
        } else {
          long x0 = LG(buffer+name_len+1);	/* already checked sz */
          long x1 = LG(buffer+name_len+5);
          int eqn_num = buffer[name_len+9];
          int num_params = buffer[name_len+10];

          if (eqn_num < 0 || eqn_num > 3) {
            printf("%s  invalid %s equation type (%d)\n",
              verbose? ":":fname, verbose? "":chunkid, eqn_num);
            set_err(kMinorError);
          } else if (num_params != eqn_params[eqn_num]) {
            printf(
              "%s  invalid number of parameters (%d) for %s equation type %d\n",
              verbose? ":":fname, num_params, verbose? "":chunkid, eqn_num);
            set_err(kMinorError);
          } else if (verbose) {
            int remainder = 0;
            uch *pbuf;

            printf(": equation type %d\n", eqn_num);
            printf("    %s\n", eqn_type[eqn_num]);
            printf("    calibration name = ");
            init_printbuf_state(&prbuf_state);
            print_buffer(&prbuf_state, buffer, name_len, 0);
            report_printbuf(&prbuf_state, chunkid);
            if (toread != sz) {
              printf(
                "\n    pngcheck INTERNAL LOGIC ERROR:  toread (%d) != sz (%ld)",
                toread, sz);
            } else
              remainder = toread - name_len - 11;
            pbuf = buffer + name_len + 11;
            if (*pbuf == 0)
              printf("\n    no physical_value unit name\n");
            else {
              int unit_len = keywordlen(pbuf, remainder);

              printf("\n    physical_value unit name = ");
              init_printbuf_state(&prbuf_state);
              print_buffer(&prbuf_state, pbuf, unit_len, 0);
              report_printbuf(&prbuf_state, chunkid);
              printf("\n");
              pbuf += unit_len;
              remainder -= unit_len;
            }
            printf("    x0 = %ld\n", x0);
            printf("    x1 = %ld\n", x1);
            for (i = 0;  i < num_params;  ++i) {
              int len;

              if (remainder < 2) {
                printf("%s  invalid %slength\n",
                  verbose? ":":fname, verbose? "":"pCAL ");
                set_err(kMajorError);
                break;
              }
              if (*pbuf != 0) {
                printf("%s  %smissing NULL separator\n",
                  verbose? ":":fname, verbose? "":"pCAL ");
                set_err(kMinorError);
                break;
              }
              ++pbuf;
              --remainder;
              len = keywordlen(pbuf, remainder);
              printf("    p%d = ", i);
              init_printbuf_state(&prbuf_state);
              print_buffer(&prbuf_state, pbuf, len, 0);
              report_printbuf(&prbuf_state, chunkid);
              printf("\n");
              pbuf += len;
              remainder -= len;
            }
          }
        }
      }
      have_pCAL = 1;
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | pHYs |
     *------*/
    } else if (strcmp(chunkid, "pHYs") == 0) {
      if (!mng && have_pHYs) {
        printf("%s  multiple pHYs not allowed\n", verbose? ":":fname);
        set_err(kMinorError);
      } else if (!mng && (have_IDAT || have_JDAT)) {
        printf("%s  %smust precede %cDAT\n",
               verbose? ":":fname, verbose? "":"pHYs ", have_IDAT? 'I':'J');
        set_err(kMinorError);
      } else if (sz != 9) {
        printf("%s  invalid %slength\n",
               verbose? ":":fname, verbose? "":"pHYs ");
        set_err(kMajorError);
      } else if (buffer[8] > 1) {
        printf("%s  invalid %sunit specifier (%u)\n",
          verbose? ":":fname, verbose? "":"pHYs ", buffer[8]);
        set_err(kMinorError);
      }
      if (verbose && no_err(kMinorError)) {
        ulg xres = LG(buffer);
        ulg yres = LG(buffer+4);
        unsigned units = buffer[8];

        printf(": %lux%lu pixels/%s", xres, yres, units? "meter":"unit");
        if (units && xres == yres)
          printf(" (%lu dpi)", (ulg)(xres*0.0254 + 0.5));
        else if (!units) {
          ulg gcf_xres_yres = gcf(xres, yres);

          printf(" (%lu:%lu)", xres/gcf_xres_yres, yres/gcf_xres_yres);
        }
        printf("\n");
      }
      have_pHYs = 1;
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | sBIT |
     *------*/
    } else if (strcmp(chunkid, "sBIT") == 0) {
      int maxbits = (ityp == 3)? 8 : sampledepth;

      if (jng) {
        printf("%s  sBIT not defined in JNG\n", verbose? ":":fname);
        set_err(kMinorError);
      } else if (png && have_sBIT) {
        printf("%s  multiple sBIT not allowed\n", verbose? ":" : fname);
        set_err(kMinorError);
      } else if (!mng && have_PLTE) {
        printf("%s  %smust precede PLTE\n",
          verbose? ":" : fname, verbose? "" : "sBIT ");
        set_err(kMinorError);
      } else if (png && have_IDAT) {
        printf("%s  %smust precede IDAT\n",
          verbose? ":" : fname, verbose? "" : "sBIT ");
        set_err(kMinorError);
      }
      switch (ityp) {
        case 0:
          if (sz != 1) {
            printf("%s  invalid %slength\n",
              verbose? ":" : fname, verbose? "" : "sBIT ");
            set_err(kMajorError);
          } else if (buffer[0] == 0 || buffer[0] > maxbits) {
            printf("%s  %d %sgrey bits invalid for %d-bit/sample image\n",
              verbose? ":" : fname, buffer[0], verbose? "" : "sBIT ", maxbits);
            set_err(kMinorError);
          } else if (verbose && no_err(kMinorError)) {
            printf("\n    gray = %u = 0x%02x\n", buffer[0], buffer[0]);
          }
          break;
        case 2:
        case 3:
          if (sz != 3) {
            printf("%s  invalid %slength\n",
              verbose? ":":fname, verbose? "":"sBIT ");
            set_err(kMajorError);
          } else if (buffer[0] == 0 || buffer[0] > maxbits) {
            printf("%s  %d %sred bits invalid for %d-bit/sample image\n",
              verbose? ":":fname, buffer[0], verbose? "":"sBIT ", maxbits);
            set_err(kMinorError);
          } else if (buffer[1] == 0 || buffer[1] > maxbits) {
            printf("%s  %d %sgreen bits invalid for %d-bit/sample image\n",
              verbose? ":":fname, buffer[1], verbose? "":"sBIT ", maxbits);
            set_err(kMinorError);
          } else if (buffer[2] == 0 || buffer[2] > maxbits) {
            printf("%s  %d %sblue bits invalid for %d-bit/sample image\n",
              verbose? ":":fname, buffer[2], verbose? "":"sBIT ", maxbits);
            set_err(kMinorError);
          } else if (verbose && no_err(kMinorError)) {
            printf("\n    red = %u = 0x%02x, green = %u = 0x%02x, "
              "blue = %u = 0x%02x\n", buffer[0], buffer[0],
              buffer[1], buffer[1], buffer[2], buffer[2]);
          }
          break;
        case 4:
          if (sz != 2) {
            printf("%s  invalid %slength\n",
              verbose? ":":fname, verbose? "":"sBIT ");
            set_err(kMajorError);
          } else if (buffer[0] == 0 || buffer[0] > maxbits) {
            printf("%s  %d %sgrey bits invalid for %d-bit/sample image\n",
              verbose? ":":fname, buffer[0], verbose? "":"sBIT ", maxbits);
            set_err(kMajorError);
          } else if (buffer[1] == 0 || buffer[1] > maxbits) {
            printf("%s  %d %salpha bits invalid for %d-bit/sample image\n",
              verbose? ":":fname, buffer[1], verbose? "":"sBIT ", maxbits);
            set_err(kMajorError);
          } else if (verbose && no_err(kMinorError)) {
            printf("\n    gray = %u = 0x%02x, alpha = %u = 0x%02x\n",
              buffer[0], buffer[0], buffer[1], buffer[1]);
          }
          break;
        case 6:
          if (sz != 4) {
            printf("%s  invalid %slength\n",
              verbose? ":":fname, verbose? "":"sBIT ");
            set_err(kMajorError);
          } else if (buffer[0] == 0 || buffer[0] > maxbits) {
            printf("%s  %d %sred bits invalid for %d-bit/sample image\n",
              verbose? ":":fname, buffer[0], verbose? "":"sBIT ", maxbits);
            set_err(kMinorError);
          } else if (buffer[1] == 0 || buffer[1] > maxbits) {
            printf("%s  %d %sgreen bits invalid for %d-bit/sample image\n",
              verbose? ":":fname, buffer[1], verbose? "":"sBIT ", maxbits);
            set_err(kMinorError);
          } else if (buffer[2] == 0 || buffer[2] > maxbits) {
            printf("%s  %d %sblue bits invalid for %d-bit/sample image\n",
              verbose? ":":fname, buffer[2], verbose? "":"sBIT ", maxbits);
            set_err(kMinorError);
          } else if (buffer[3] == 0 || buffer[3] > maxbits) {
            printf("%s  %d %salpha bits invalid for %d-bit/sample image\n",
              verbose? ":":fname, buffer[3], verbose? "":"sBIT ", maxbits);
            set_err(kMinorError);
          } else if (verbose && no_err(kMinorError)) {
            printf("\n    red = %u = 0x%02x, green = %u = 0x%02x, "
              "blue = %u = 0x%02x, alpha = %u = 0x%02x\n", buffer[0], buffer[0],
              buffer[1], buffer[1], buffer[2], buffer[2], buffer[3], buffer[3]);
          }
          break;
      }
      have_sBIT = 1;
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | sCAL |
     *------*/
    } else if (strcmp(chunkid, "sCAL") == 0) {
      int unittype = buffer[0];
      uch *pPixwidth = buffer+1, *pPixheight=NULL;

      if (!mng && have_sCAL) {
        printf("%s  multiple sCAL not allowed\n", verbose? ":":fname);
        set_err(kMinorError);
      } else if (!mng && (have_IDAT || have_JDAT)) {
        printf("%s  %smust precede %cDAT\n",
               verbose? ":":fname, verbose? "":"sCAL ", have_IDAT? 'I':'J');
        set_err(kMinorError);
      } else if (sz < 4) {
        printf("%s  invalid %slength\n",
               verbose? ":":fname, verbose? "":"sCAL ");
        set_err(kMinorError);
      } else if (unittype < 1 || unittype > 2) {
        printf("%s  invalid %sunit specifier (%d)\n",
               verbose? ":":fname, verbose? "":"sCAL ", unittype);
        set_err(kMinorError);
      } else {
        uch *qq;
        for (qq = pPixwidth;  qq < buffer+sz;  ++qq) {
          if (*qq == 0)
            break;
        }
        if (qq == buffer+sz) {
          printf("%s  missing %snull separator\n",
                 verbose? ":":fname, verbose? "":"sCAL ");
          set_err(kMinorError);
        } else {
          pPixheight = qq + 1;
          if (pPixheight == buffer+sz || *pPixheight == 0) {
            printf("%s  missing %spixel height\n",
                   verbose? ":":fname, verbose? "":"sCAL ");
            set_err(kMinorError);
          }
        }
        if (no_err(kMinorError)) {
          for (qq = pPixheight;  qq < buffer+sz;  ++qq) {
            if (*qq == 0)
              break;
          }
          if (qq != buffer+sz) {
            printf("%s  extra %snull separator (warning)\n",
                   verbose? ":":fname, verbose? "":"sCAL ");
            set_err(kWarning);
          }
          if (*pPixwidth == '-' || *pPixheight == '-') {
            printf("%s  invalid negative %svalue(s)\n",
                   verbose? ":":fname, verbose? "":"sCAL ");
            set_err(kMinorError);
          } else if (check_ascii_float(pPixwidth, (int)(pPixheight-pPixwidth-1), chunkid) ||
                     check_ascii_float(pPixheight, (int)(buffer+sz-pPixheight), chunkid))
          {
            set_err(kMinorError);
          }
        }
      }
      if (verbose && no_err(kMinorError)) {
        if (sz >= BS)
          sz = BS-1;
        buffer[sz] = '\0';
        printf(": image size %s x %s %s\n", pPixwidth, pPixheight,
               (unittype == 1)? "meters":"radians");
      }
      have_sCAL = 1;
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | sPLT |
     *------*/
    } else if (strcmp(chunkid, "sPLT") == 0) {
      int name_len;

      if (jng) {
        printf("%s  sPLT not defined in JNG\n", verbose? ":":fname);
        set_err(kMinorError);
      } else if (png && have_IDAT) {
        printf("%s  %smust precede IDAT\n",
          verbose? ":":fname, verbose? "":"sPLT ");
        set_err(kMinorError);
      } else if (check_keyword(buffer, toread, &name_len, "palette name", chunkid)) {
        set_err(kMinorError);
      } else {
        uch bps = buffer[name_len+1];
        int remainder = toread - name_len - 2;
        int bytes = (bps >> 3);
        int entry_sz = 4*bytes + 2;
        int nsplt = remainder / entry_sz;

        if (remainder < 0) {
          printf("%s  invalid %slength\n",  /* or input buffer too small */
            verbose? ":":fname, verbose? "":"sPLT ");
          set_err(kMajorError);
        } else if (buffer[name_len] != 0) {
          printf("%s  missing NULL after %spalette name\n",
            verbose? ":":fname, verbose? "":"sPLT ");
          set_err(kMinorError);
        } else if (bps != 8 && bps != 16) {
          printf("%s  invalid %ssample depth (%u bits)\n",
            verbose? ":":fname, verbose? "":"sPLT ", bps);
          set_err(kMinorError);
        } else if (remainder % entry_sz != 0) {
          printf("%s  invalid number of %sentries (%g)\n",
            verbose? ":":fname, verbose? "":"sPLT ",
            (double)remainder / entry_sz);
          set_err(kMajorError);
        } else if (verbose || (printpal && !quiet)) {
          if (!verbose && printpal && !quiet)
            printf("  sPLT chunk");
          printf(": %d palette/histogram entr%s\n", nsplt,
            nsplt == 1? "y":"ies");
          printf("    sample depth = %u bits, palette name = ", bps);
          init_printbuf_state(&prbuf_state);
          print_buffer(&prbuf_state, buffer, name_len, 0);
          report_printbuf(&prbuf_state, chunkid);
          printf("\n");
        }
        if (printpal && no_err(kMinorError)) {
          const char *spc;
          int i, j = name_len+2;

          if (nsplt < 10)
            spc = "  ";
          else if (nsplt < 100)
            spc = "   ";
          else if (nsplt < 1000)
            spc = "    ";
          else if (nsplt < 10000)
            spc = "     ";
          else
            spc = "      ";
          /* GRR:  could check for (required) non-increasing freq order */
          /* GRR:  could also check for all zero freqs:  undefined hist */
          if (bytes == 1) {
            for (i = 0;  i < nsplt;  ++i, j += 6)
              printf("%s%3d:  (%3u,%3u,%3u,%3u) = "
                "(0x%02x,0x%02x,0x%02x,0x%02x)  freq = %u\n", spc, i,
                buffer[j], buffer[j+1], buffer[j+2], buffer[j+3],
                buffer[j], buffer[j+1], buffer[j+2], buffer[j+3],
                SH(buffer+j+4));
          } else {
            for (i = 0;  i < nsplt;  ++i, j += 10)
              printf("%s%5d:  (%5u,%5u,%5u,%5u) = (%04x,%04x,%04x,%04x)  "
                "freq = %u\n", spc, i, SH(buffer+j), SH(buffer+j+2),
                SH(buffer+j+4), SH(buffer+j+6), SH(buffer+j), SH(buffer+j+2),
                SH(buffer+j+4), SH(buffer+j+6), SH(buffer+j+8));
          }
        }
      }
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | sRGB |
     *------*/
    } else if (strcmp(chunkid, "sRGB") == 0) {
      if (!mng && have_sRGB) {
        printf("%s  multiple sRGB not allowed\n", verbose? ":":fname);
        set_err(kMinorError);
      } else if (!mng && have_iCCP) {
        printf("%s  %snot allowed with iCCP\n",
               verbose? ":":fname, verbose? "":"sRGB ");
        set_err(kMinorError);
      } else if (!mng && have_PLTE) {
        printf("%s  %smust precede PLTE\n",
               verbose? ":":fname, verbose? "":"sRGB ");
        set_err(kMinorError);
      } else if (!mng && (have_IDAT || have_JDAT)) {
        printf("%s  %smust precede %cDAT\n",
               verbose? ":":fname, verbose? "":"sRGB ", have_IDAT? 'I':'J');
        set_err(kMinorError);
      } else if (sz != 1) {
        printf("%s  invalid %slength\n",
               verbose? ":":fname, verbose? "":"sRGB ");
        set_err(kMinorError);
      } else if (buffer[0] > 3) {
        printf("%s  %sinvalid rendering intent\n",
               verbose? ":":fname, verbose? "":"sRGB ");
        set_err(kMinorError);
      }
      if (verbose && no_err(kMinorError)) {
        printf("\n    rendering intent = %s\n", rendering_intent[buffer[0]]);
      }
      have_sRGB = 1;
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | sTER |
     *------*/
    } else if (strcmp(chunkid, "sTER") == 0) {
      if (!mng && have_sTER) {
        printf("%s  multiple sTER not allowed\n", verbose? ":":fname);
        set_err(kMinorError);
      } else if (!mng && (have_IDAT || have_JDAT)) {
        printf("%s  %smust precede %cDAT\n",
               verbose? ":":fname, verbose? "":"sTER ", have_IDAT? 'I':'J');
        set_err(kMinorError);
      } else if (sz != 1) {
        printf("%s  invalid %slength\n",
               verbose? ":":fname, verbose? "":"sTER ");
        set_err(kMinorError);
      } else if (buffer[0] > 1) {
        printf("%s  invalid %slayout mode\n",
               verbose? ":":fname, verbose? "":"sTER ");
        set_err(kMinorError);
      }
      if (verbose && no_err(kMinorError)) {
        printf("\n    stereo subimage layout = %s\n",
               buffer[0]? "divergent (parallel)":"cross-eyed");
      }
      have_sTER = 1;
      last_is_IDAT = last_is_JDAT = 0;

    /*------*  *------*
     | tEXt |  | zTXt |
     *------*  *------*/
    } else if (strcmp(chunkid, "tEXt") == 0 || strcmp(chunkid, "zTXt") == 0) {
      int ztxt = (chunkid[0] == 'z');
      int keylen;

      if (check_keyword(buffer, toread, &keylen, "keyword", chunkid))
        set_err(kMinorError);
      else if (ztxt) {
        int compr = (uch)buffer[keylen+1];
        if (compr > 127) {
          printf("%s  private (possibly invalid) %scompression method (%d) "
            "(warning)\n", verbose? ":":fname, verbose? "":"zTXt ", compr);
          set_err(kWarning);
        } else if (compr > 0) {
          printf("%s  invalid %scompression method (%d)\n",
            verbose? ":":fname, verbose? "":"zTXt ", compr);
          set_err(kMinorError);
        }
/*
FIXME: add support for checking zlib header bytes of zTXt (and iTXt, iCCP, etc.)
 */
      }
      else if (check_text(buffer + keylen + 1, toread - keylen - 1, chunkid)) {
        set_err(kMinorError);
      }
      if (no_err(kMinorError)) {
        init_printbuf_state(&prbuf_state);
        if (verbose || printtext) {
          if (verbose)
            printf(", keyword: ");
          print_buffer(&prbuf_state, buffer, keylen, 0);
        }
        if (printtext) {
          printf(verbose? "\n" : ":\n");
          if (strcmp(chunkid, "tEXt") == 0)
            print_buffer(&prbuf_state, buffer + keylen + 1,
              toread - keylen - 1, 1);
          else {
            printf("%s(compressed %s text)", verbose? "    " : "", chunkid);
/*
FIXME: add support for decompressing/printing zTXt
 */
          }

          /* For the sake of simplifying this program, we will not print
           * the contents of a tEXt chunk whose size is larger than the
           * buffer size (currently 32K).  People should use zTXt for
           * such large amounts of text, anyway!  Note that this does not
           * mean that the tEXt/zTXt contents will be lost if extracting.
           */
          printf("\n");
        } else if (verbose) {
          printf("\n");
        }
        report_printbuf(&prbuf_state, chunkid);
      }
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | tIME |
     *------*/
    } else if (strcmp(chunkid, "tIME") == 0) {
      if (!mng && have_tIME) {
        printf("%s  multiple tIME not allowed\n", verbose? ":":fname);
        set_err(kMinorError);
      } else if (sz != 7) {
        printf("%s  invalid %slength\n",
          verbose? ":":fname, verbose? "":"tIME ");
        set_err(kMinorError);
      } else {
        int yr = SH(buffer);
        int mo = buffer[2];
        int dy = buffer[3];
        int hh = buffer[4];
        int mm = buffer[5];
        int ss = buffer[6];

        if (yr < 1995) {
          /* conversion to PNG format counts as modification... */
          /* FIXME:  also test for future dates? (may allow current year + 1) */
          printf("%s  invalid %syear (%d)\n",
            verbose? ":":fname, verbose? "":"tIME ", yr);
          set_err(kMinorError);
        } else if (mo < 1 || mo > 12) {
          printf("%s  invalid %smonth (%d)\n",
            verbose? ":":fname, verbose? "":"tIME ", mo);
          set_err(kMinorError);
        } else if (dy < 1 || dy > 31) {
          /* FIXME:  also validate day given specified month? */
          printf("%s  invalid %sday (%d)\n",
            verbose? ":":fname, verbose? "":"tIME ", dy);
          set_err(kMinorError);
        } else if (hh < 0 || hh > 23) {
          printf("%s  invalid %shour (%d)\n",
            verbose? ":":fname, verbose? "":"tIME ", hh);
          set_err(kMinorError);
        } else if (mm < 0 || mm > 59) {
          printf("%s  invalid %sminute (%d)\n",
            verbose? ":":fname, verbose? "":"tIME ", mm);
          set_err(kMinorError);
        } else if (ss < 0 || ss > 60) {
          printf("%s  invalid %ssecond (%d)\n",
            verbose? ":":fname, verbose? "":"tIME ", ss);
          set_err(kMinorError);
        }
        /* print the date in RFC 1123 format, rather than stored order */
        /* FIXME:  change to ISO-whatever format, i.e., yyyy-mm-dd hh:mm:ss? */
        if (verbose && no_err(kMinorError)) {
          printf(": %2d %s %4d %02d:%02d:%02d UTC\n", dy, getmonth(mo), yr,
            hh, mm, ss);
        }
      }
      have_tIME = 1;
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | tRNS |
     *------*/
    } else if (strcmp(chunkid, "tRNS") == 0) {
      if (jng) {
        printf("%s  tRNS not defined in JNG\n", verbose? ":":fname);
        set_err(kMinorError);
      } else if (png && have_tRNS) {
        printf("%s  multiple tRNS not allowed\n", verbose? ":":fname);
        set_err(kMinorError);
      } else if (ityp == 3 && !have_PLTE) {
        printf("%s  %smust follow PLTE\n",
          verbose? ":":fname, verbose? "":"tRNS ");
        set_err(kMinorError);
      } else if (png && have_IDAT) {
        printf("%s  %smust precede IDAT\n",
          verbose? ":":fname, verbose? "":"tRNS ");
        set_err(kMinorError);
      }
      if (no_err(kMinorError)) {
        switch (ityp) {
          case 0:
            if (sz != 2) {
              printf("%s  invalid %slength for %s image\n",
                verbose? ":":fname, verbose? "":"tRNS ", png_type[ityp]);
              set_err(kMajorError);
            } else if (verbose && no_err(kMinorError)) {
              printf("\n    gray = 0x%04x\n", SH(buffer));
            }
            break;
          case 2:
            if (sz != 6) {
              printf("%s  invalid %slength for %s image\n",
                verbose? ":":fname, verbose? "":"tRNS ", png_type[ityp]);
              set_err(kMajorError);
            } else if (verbose && no_err(kMinorError)) {
              printf("\n    red = 0x%04x, green = 0x%04x, blue = 0x%04x\n",
                     SH(buffer), SH(buffer+2), SH(buffer+4));
            }
            break;
          case 3:
            if (sz > nplte) {
              printf("%s  invalid %slength for %s image\n",
                verbose? ":":fname, verbose? "":"tRNS ", png_type[ityp]);
              set_err(kMajorError);
            } else if ((verbose || (printpal && !quiet)) && no_err(kMinorError))
            {
              if (!verbose && printpal && !quiet)
                printf("  tRNS chunk");
              printf(": %ld transparency entr%s\n", sz, sz == 1? "y":"ies");
            }
            if (printpal && no_err(kMinorError)) {
              const char *spc;

              if (sz < 10)
                spc = "  ";
              else if (sz < 100)
                spc = "   ";
              else
                spc = "    ";
              for (i = 0;  i < sz;  ++i)
                printf("%s%3d:  %3d = 0x%02x\n", spc, i, buffer[i], buffer[i]);
            }
            break;
          default:
            printf("%s  %snot allowed in %s image\n",
                   verbose? ":":fname, verbose? "":"tRNS ", png_type[ityp]);
            set_err(kMinorError);
            break;
        }
      }
      have_tRNS = 1;
      last_is_IDAT = last_is_JDAT = 0;

    /*===========================================*/
    /* identifiable private chunks; guts unknown */

    /*------*
     | cmOD |
     *------*/
    } else if (strcmp(chunkid, "cmOD") == 0) {
      if (verbose)
        printf("\n    "
          "Microsoft Picture It private, ancillary, unsafe-to-copy chunk\n");
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | cmPP |   (guessing MS)
     *------*/
    } else if (strcmp(chunkid, "cmPP") == 0) {
      if (verbose)
        printf("\n    "
          "Microsoft Picture It(?) private, ancillary, unsafe-to-copy chunk\n");
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | cpIp |
     *------*/
    } else if (strcmp(chunkid, "cpIp") == 0) {
      if (verbose)
        printf("\n    "
          "Microsoft Picture It private, ancillary, safe-to-copy chunk\n");
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | mkBF |
     *------*/
    } else if (strcmp(chunkid, "mkBF") == 0) {
      if (verbose)
        printf("\n    "
          "Macromedia Fireworks private, ancillary, unsafe-to-copy chunk\n");
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | mkBS |
     *------*/
    } else if (strcmp(chunkid, "mkBS") == 0) {
      if (verbose)
        printf("\n    "
          "Macromedia Fireworks private, ancillary, unsafe-to-copy chunk\n");
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | mkBT |
     *------*/
    } else if (strcmp(chunkid, "mkBT") == 0) {
      if (verbose)
        printf("\n    "
          "Macromedia Fireworks private, ancillary, unsafe-to-copy chunk\n");
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | mkTS |
     *------*/
    } else if (strcmp(chunkid, "mkTS") == 0) {
      if (verbose)
        printf("\n    "
          "Macromedia Fireworks(?) private, ancillary, unsafe-to-copy chunk\n");
      last_is_IDAT = last_is_JDAT = 0;

    /* msOG - Microsoft?  Macromedia? */

    /*------*
     | pcLb |
     *------*/
    } else if (strcmp(chunkid, "pcLb") == 0) {
      if (verbose)
        printf("\n    "
          "Piclab(?) private, ancillary, safe-to-copy chunk\n");
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | prVW |
     *------*/
    } else if (strcmp(chunkid, "prVW") == 0) {
      if (verbose)
        printf("\n    Macromedia Fireworks preview chunk"
          " (private, ancillary, unsafe to copy)\n");
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | spAL |  intermediate sPLT test version (still had gamma field)
     *------*/
    } else if (strcmp(chunkid, "spAL") == 0) {
      /* png-group/documents/history/png-proposed-sPLT-19961015.html */
      if (verbose)
        printf("\n    preliminary/test version of sPLT "
          "(private, ancillary, unsafe to copy)\n");
      last_is_IDAT = last_is_JDAT = 0;

    /*================================================*
     * JNG chunks (with the exception of JHDR, above) *
     *================================================*/

    /*------*
     | JDAT |
     *------*/
    } else if (strcmp(chunkid, "JDAT") == 0) {
      if (png) {
        printf("%s  JDAT not defined in PNG\n", verbose? ":":fname);
        set_err(kMinorError);
      } else if (have_JDAT && !(last_is_JDAT || last_is_IDAT)) {
        /* GRR:  need to check for consecutive IDATs within MNG segments */
        if (mng) {  /* reset things (FIXME:  SEMI-HACK--check for segments!) */
          have_JDAT = 0;
          if (verbose)
            printf("\n");
        } else {
          printf(
            "%s  JDAT chunks must be consecutive or interleaved with IDATs\n",
            verbose? ":":fname);
          set_err(kMajorError);
          if (!force)
            return global_error;
        }
      } else if (verbose)
        printf("\n");
      have_JDAT = 1;
      last_is_IDAT = 0;
      last_is_JDAT = 1;   /* also true if last was JSEP (see below) */

    /*------*
     | JSEP |
     *------*/
    } else if (strcmp(chunkid, "JSEP") == 0) {
      if (png) {
        printf("%s  JSEP not defined in PNG\n", verbose? ":":fname);
        set_err(kMinorError);
      } else if (jng && bitdepth != 20) {
        printf("%s  JSEP allowed only if 8-bit and 12-bit JDATs present\n",
          verbose? ":":fname);
        set_err(kMinorError);
      } else if (jng && have_JSEP) {
        printf("%s  multiple JSEP not allowed\n", verbose? ":":fname);
        set_err(kMinorError);
      } else if (jng && !(last_is_JDAT || last_is_IDAT)) {
        printf("%s  JSEP must appear between JDAT or IDAT chunks\n",
          verbose? ":":fname);
        set_err(kMinorError);
      } else if (sz != 0) {
        printf("%s  invalid %slength\n",
          verbose? ":":fname, verbose? "":"JSEP ");
        set_err(kMinorError);
      } else if (verbose) {
        printf("\n");
      }
      have_JSEP = 1;
      last_is_IDAT = 0;
      last_is_JDAT = 1;   /* effectively... (GRR HACK) */

    /*===============================================================*
     * MNG chunks (with the exception of MHDR and JNG chunks, above) *
     *===============================================================*/

    /*------*
     | DHDR | 		DELTA-PNG
     *------*/
    } else if (strcmp(chunkid, "DHDR") == 0) {
      if (png || jng) {
        printf("%s  DHDR not defined in %cNG\n", verbose? ":":fname,
          png? 'P':'J');
        set_err(kMinorError);
      } else if (sz != 4 && sz != 12 && sz != 20) {
        printf("%s  invalid %slength\n",
          verbose? ":":fname, verbose? "":"DHDR ");
        set_err(kMajorError);
      }
      if (verbose && no_err(kMinorError)) {
        uch dtype = buffer[3];

        printf("\n    object ID = %u, image type = %s, delta type = %s\n",
          SH(buffer), buffer[2]? "PNG":"unspecified",
          (dtype < sizeof(delta_type)/sizeof(const char *))?
          delta_type[dtype] : inv);
        if (sz > 4) {
          if (dtype == 5) {
            printf("%s  invalid %slength for delta type %d\n",
              verbose? ":":fname, verbose? "":"DHDR ", dtype);
            set_err(kMinorError);
          } else {
            printf("    block width = %lu, block height = %lu\n", LG(buffer+4),
              LG(buffer+8));
            if (sz > 12) {
              if (dtype == 0 || dtype == 5) {
                printf("%s  invalid %slength for delta type %d\n",
                  verbose? ":":fname, verbose? "":"DHDR ", dtype);
                set_err(kMinorError);
              } else
                printf("    x offset = %lu, y offset = %lu\n", LG(buffer+12),
                  LG(buffer+16));
            }
          }
        }
      }
      have_DHDR = 1;
      last_is_IDAT = last_is_JDAT = 0;
#ifdef USE_ZLIB
      first_idat = 1;  /* flag:  next IDAT will be the first in this subimage */
      zlib_error = 0;  /* flag:  no zlib errors yet in this file */
      /* GRR 20000304:  data dump not yet compatible with interlaced images: */
      if (lace && verbose > 3)  /* (FIXME eventually...or move to pngcrunch) */
        verbose = 2;
#endif

    /*------*
     | FRAM |
     *------*/
    } else if (strcmp(chunkid, "FRAM") == 0) {
      if (png || jng) {
        printf("%s  FRAM not defined in %cNG\n", verbose? ":":fname,
          png? 'P':'J');
        set_err(kMinorError);
      } else if (sz == 0 && verbose) {
        printf(":  empty\n");
      } else if (verbose) {
        uch fmode = buffer[0];

        printf(": mode %d\n    %s\n", fmode,
          (fmode < sizeof(framing_mode)/sizeof(const char *))?
          framing_mode[fmode] : inv);
        if (sz > 1) {
          uch *p = buffer+1;
          int bytes_left, found_null=0;

          if (*p) {
            printf("    frame name = ");
            do {
              if (*p)
                putchar(*p);	/* GRR EBCDIC WARNING */
              else {
                putchar('\n');
                ++p;
                break;
              }
            } while (++p < buffer + sz);
          } else {
            ++p;  /* skip over null */
            ++found_null;
          }
          bytes_left = (int)(sz - (p-buffer));   /* FIXME:  is sz big enough? */
          if (bytes_left == 0 && found_null) {
            printf("    invalid trailing NULL byte\n");
            set_err(kMinorError);
          } else if (bytes_left < 4) {
            printf("    invalid length\n");
            set_err(kMajorError);
          } else {
            uch cid = *p++;	/* change_interframe_delay */
            uch ctt = *p++;	/* change_timeout_and_termination */
            uch cscb = *p++;	/* change_subframe_clipping_boundaries */
            uch csil = *p++;	/* change_sync_id_list */

            if (cid > 2 || ctt > 8 || cscb > 2 || csil > 2) {
              printf("    invalid change flags\n");
              set_err(kMinorError);
            } else {
              bytes_left -= 4;
              printf("    %s\n", change_interframe_delay[cid]);
              /* GRR:  need real error-checking here: */
              if (cid && bytes_left >= 4) {
                ulg delay = LG(p);

                printf("      new delay = %lu tick%s\n", delay, (delay == 1L)?
                  "" : "s");
                p += 4;
                bytes_left -= 4;
              }
              printf("    %s\n", change_timeout_and_termination[ctt]);
              /* GRR:  need real error-checking here: */
              if (ctt && bytes_left >= 4) {
                ulg val = LG(p);

                if (val == 0x7fffffffL)
                  printf("      new timeout = infinite\n");
                else
                  printf("      new timeout = %lu tick%s\n", val, (val == 1L)?
                    "" : "s");
                p += 4;
                bytes_left -= 4;
              }
              printf("    %s\n", change_subframe_clipping_boundaries[cscb]);
              /* GRR:  need real error-checking here: */
              if (cscb && bytes_left >= 17) {
                printf("      new frame clipping boundaries (%s):\n", (*p++)?
                  "differences from previous values":"absolute pixel values");
                printf(
                  "        left = %ld, right = %ld, top = %ld, bottom = %ld\n",
                  LG(p), LG(p+4), LG(p+8), LG(p+12));
                p += 16;
                bytes_left -= 17;
              }
              printf("    %s\n", change_sync_id_list[csil]);
              if (csil) {
                if (bytes_left) {
                  while (bytes_left >= 4) {
                    printf("      %lu\n", LG(p));
                    p += 4;
                    bytes_left -= 4;
                  }
                } else
                  printf("      [empty list]\n");
              }
            }
          }
/*
          if (p < buffer + sz)
            printf("    (bytes left = %d)\n", sz - (p-buffer));
          else
            printf("    (no bytes left)\n");
 */
        }
      }
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | SAVE |
     *------*/
    } else if (strcmp(chunkid, "SAVE") == 0) {
      if (png || jng) {
        printf("%s  SAVE not defined in %cNG\n", verbose? ":":fname,
          png? 'P':'J');
        set_err(kMinorError);
      } else if (have_SAVE) {
        printf("%s  multiple SAVE not allowed\n", verbose? ":":fname);
        set_err(kMinorError);
      } else if (sz > 0 && verbose) {
        uch offsize = buffer[0];

        if (offsize != 4 && offsize != 8) {
          printf("%s  invalid %soffset size (%u bytes)\n",
            verbose? ":":fname, verbose? "":"SAVE ", (unsigned)offsize);
          set_err(kMinorError);
        } else if (sz > 1) {
          uch *p = buffer+1;
          int bytes_left = sz-1;

          printf("\n    offset size = %u bytes\n", (unsigned)offsize);
          while (bytes_left > 0) {
            uch type = *p;

            if ((type == 0 && bytes_left < 5+2*offsize) ||
                (type == 1 && bytes_left < 1+offsize)) {
              printf("%s  invalid %slength\n",
                verbose? ":":fname, verbose? "":"SAVE ");
              set_err(kMinorError);
              break;
            }
            printf("    entry type = %s", (type <
              sizeof(entry_type)/sizeof(const char *))? entry_type[type] : inv);
            ++p;
            if (type <= 1) {
              ulg first4 = LG(p);

              printf(", offset = ");
              if ((offsize == 4 && first4 == 0L) ||
                  (offsize == 8 && first4 == 0L && LG(p+4) == 0L))
                printf("unknown\n");
              else if (offsize == 4)
                printf("0x%08lx\n", first4);
              else
                printf("0x%08lx%08lx\n", first4, LG(p+4));  /* big-endian */
              p += offsize;
              if (type == 0) {
                printf("    nominal start time = 0x%08lx", LG(p));
                if (offsize == 8)
                  printf("%08lx", LG(p+4));
                p += offsize;
                printf(", nominal layer number = %lu,\n", LG(p));
                p += 4;
                printf("    nominal frame number = %lu\n", LG(p));
                p += 4;
              }
            } else
              printf("\n");
            bytes_left = (int)(sz - (p-buffer));   /* FIXME:  is sz big enough? */
            // name must match that in corresponding SEEK/FRAM/eXPI chunk, or
            // be omitted if unnamed segment (not checked!)
            if (bytes_left) {
              int have_name = 0;
              if (*p) {
                have_name = 1;
                printf("    name = ");
              }
              do {
                if (*p)
                  putchar(*p);          /* GRR EBCDIC WARNING */
                else {
                  ++p;  // skip over separator byte (but omitted for final one)
                  break;
                }
              } while (++p < buffer + sz);
              if (have_name)
                printf("\n");
              bytes_left = (int)(sz - (p-buffer));   /* FIXME:  is sz big enough? */
            }
          } /* end while (bytes_left > 0) */
        }
      } else if (verbose) {
        printf("\n");
      }
      have_SAVE = 1;
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | SEEK |
     *------*/
    } else if (strcmp(chunkid, "SEEK") == 0) {
      if (png || jng) {
        printf("%s  SEEK not defined in %cNG\n", verbose? ":":fname,
          png? 'P':'J');
        set_err(kMinorError);
      } else if (!have_SAVE) {
        printf("%s  %snot allowed without preceding SAVE chunk\n",
          verbose? ":":fname, verbose? "":"SEEK ");
        set_err(kMinorError);
      } else if (verbose) {
        printf("\n");
        if (sz > 0) {
          init_printbuf_state(&prbuf_state);
          print_buffer(&prbuf_state, buffer, sz, 1);
          report_printbuf(&prbuf_state, chunkid);
          printf("\n");
        }
      }
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | nEED |
     *------*/
    } else if (strcmp(chunkid, "nEED") == 0) {
      if (png || jng) {
        printf("%s  nEED not defined in %cNG\n", verbose? ":":fname,
          png? 'P':'J');
        set_err(kMinorError);
      } else if (sz > 0 && verbose) {
        uch *p = buffer;
        uch *lastbreak = buffer;

        if (sz < 32)
          printf(":  ");
        else
          printf("\n    ");
        do {
          if (*p)
            putchar(*p);	/* GRR EBCDIC WARNING */
          else if (p - lastbreak > 40)
            printf("\n    ");
          else {
            putchar(';');
            putchar(' ');
          }
        } while (++p < buffer + sz);
        printf("\n");
      }
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | DEFI |
     *------*/
    } else if (strcmp(chunkid, "DEFI") == 0) {
      if (png || jng) {
        printf("%s  DEFI not defined in %cNG\n", verbose? ":":fname,
          png? 'P':'J');
        set_err(kMinorError);
      } else if (sz != 2 && sz != 3 && sz != 4 && sz != 12 && sz != 28) {
        printf("%s  invalid %slength\n",
          verbose? ":":fname, verbose? "":"DEFI ");
        set_err(kMajorError);
      }
#ifdef RP_PRINTF_ENABLED
      if (verbose && no_err(kMinorError)) {
        const char *noshow = do_not_show[0];
        uch concrete = 0;
        long x = 0L;
        long y = 0L;

        if (sz > 2) {
          if (buffer[2] == 1)
            noshow = do_not_show[1];
          else if (buffer[2] > 1)
            noshow = inv;
        }
        if (sz > 3)
          concrete = buffer[3];
        if (sz > 4) {
          x = LG(buffer+4);
          y = LG(buffer+8);
        }
        printf("\n    object ID = %u, %s, %s, x = %ld, y = %ld\n", SH(buffer),
          noshow, concrete? "concrete":"abstract", x, y);
        if (sz > 12) {
          printf(
            "    clipping:  left = %ld, right = %ld, top = %ld, bottom = %ld\n",
            LG(buffer+12), LG(buffer+16), LG(buffer+20), LG(buffer+24));
        }
      }
#endif /* RP_PRINTF_ENABLED */
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | BACK |
     *------*/
    } else if (strcmp(chunkid, "BACK") == 0) {
      if (png || jng) {
        printf("%s  BACK not defined in %cNG\n", verbose? ":":fname,
          png? 'P':'J');
        set_err(kMinorError);
      } else if (sz < 6 || sz == 8 || sz > 10) {
        printf("%s  invalid %slength\n",
          verbose? ":":fname, verbose? "":"BACK ");
        set_err(kMajorError);
      }
      if (verbose && no_err(kMinorError)) {
        printf("\n    red = 0x%04x, green = 0x%04x, blue = 0x%04x (%s)\n",
          SH(buffer), SH(buffer+2), SH(buffer+4), (sz > 6 && (buffer[6] & 1))?
          "mandatory":"advisory");
        if (sz >= 9) {
          printf("    background image ID = %u (%s, %stile)\n",
            SH(buffer+7), (buffer[6] & 1)? "mandatory":"advisory",
            (sz > 9 && (buffer[9] & 1))? "":"do not ");
        }
      }
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | MOVE |
     *------*/
    } else if (strcmp(chunkid, "MOVE") == 0) {
      if (png || jng) {
        printf("%s  MOVE not defined in %cNG\n", verbose? ":":fname,
          png? 'P':'J');
        set_err(kMinorError);
      } else if (sz != 13) {
        printf("%s  invalid %slength\n",
          verbose? ":":fname, verbose? "":"MOVE ");
        set_err(kMajorError);
      }
      if (verbose && no_err(kMinorError)) {
        printf("\n    first object ID = %u, last object ID = %u\n",
          SH(buffer), SH(buffer+2));
        if (buffer[4])
          printf(
            "    relative change in position:  delta-x = %ld, delta-y = %ld\n",
            LG(buffer+5), LG(buffer+9));
        else
          printf("    new position:  x = %ld, y = %ld\n",
            LG(buffer+5), LG(buffer+9));
      }
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | CLON |
     *------*/
    } else if (strcmp(chunkid, "CLON") == 0) {
      if (png || jng) {
        printf("%s  CLON not defined in %cNG\n", verbose? ":":fname,
          png? 'P':'J');
        set_err(kMinorError);
      } else if (sz != 4 && sz != 5 && sz != 6 && sz != 7 && sz != 16) {
        printf("%s  invalid %slength\n",
          verbose? ":":fname, verbose? "":"CLON ");
        set_err(kMajorError);
      }
      if (verbose && no_err(kMinorError)) {
        uch ct = 0;	/* full clone */
        uch dns = 2;	/* same as parent's */
        uch cf = 0;	/* same as parent's */
        uch ldt = 1;	/* delta from parent */
        long x = 0L;
        long y = 0L;

        if (sz > 4)
          ct = buffer[4];
        if (sz > 5)
          dns = buffer[5];
        if (sz > 6)
          cf = buffer[6];
        if (sz > 7) {
          ldt = buffer[7];
          x = LG(buffer+8);
          y = LG(buffer+12);
        }
        printf("\n    parent object ID = %u, clone object ID = %u\n",
          SH(buffer), SH(buffer+2));
        printf("    clone type = %s, %s, %s\n",
          (ct < sizeof(clone_type)/sizeof(const char *))? clone_type[ct] : inv,
          (dns < sizeof(do_not_show)/sizeof(const char *))? do_not_show[dns] : inv,
          cf? "same concreteness as parent":"abstract");
        if (ldt)
          printf("    difference from parent's position:  delta-x = %ld,"
            " delta-y = %ld\n", x, y);
        else
          printf("    absolute position:  x = %ld, y = %ld\n", x, y);
      }
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | SHOW |
     *------*/
    } else if (strcmp(chunkid, "SHOW") == 0) {
      if (png || jng) {
        printf("%s  SHOW not defined in %cNG\n", verbose? ":":fname,
          png? 'P':'J');
        set_err(kMinorError);
      } else if (sz != 0 && sz != 2 && sz != 4 && sz != 5) {
        printf("%s  invalid %slength\n",
          verbose? ":":fname, verbose? "":"SHOW ");
        set_err(kMajorError);
      }
      if (verbose && no_err(kMinorError)) {
        ush first = 0;
        ush last = 65535;
        uch smode = 2;

        if (sz > 0) {
          first = last = SH(buffer);
          smode = 0;
        }
        if (sz > 2)
          last = SH(buffer+2);
        if (sz > 4)
          smode = buffer[4];
        printf("\n    first object = %u, last object = %u\n", first, last);
        printf("    %s\n",
          (smode < sizeof(show_mode)/sizeof(const char *))? show_mode[smode] : inv);
      }
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | CLIP |
     *------*/
    } else if (strcmp(chunkid, "CLIP") == 0) {
      if (png || jng) {
        printf("%s  CLIP not defined in %cNG\n", verbose? ":":fname,
          png? 'P':'J');
        set_err(kMinorError);
      } else if (sz != 21) {
        printf("%s  invalid %slength\n",
          verbose? ":":fname, verbose? "":"CLIP ");
        set_err(kMajorError);
      }
      if (verbose && no_err(kMinorError)) {
        printf(
          "\n    first object = %u, last object = %u; %s clip boundaries:\n",
          SH(buffer), SH(buffer+2), buffer[4]? "relative change in":"absolute");
        printf("      left = %ld, right = %ld, top = %ld, bottom = %ld\n",
          LG(buffer+5), LG(buffer+9), LG(buffer+13), LG(buffer+17));
      }
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | LOOP |
     *------*/
    } else if (strcmp(chunkid, "LOOP") == 0) {
      if (png || jng) {
        printf("%s  LOOP not defined in %cNG\n", verbose? ":":fname,
          png? 'P':'J');
        set_err(kMinorError);
      } else if (sz < 5 || (sz > 6 && ((sz-6) % 4) != 0)) {
        printf("%s  invalid %slength\n",
          verbose? ":":fname, verbose? "":"LOOP ");
        set_err(kMajorError);
      }
      if (verbose && no_err(kMinorError)) {
        printf(":  nest level = %u\n    count = %lu, termination = %s\n",
          (unsigned)(buffer[0]), LG(buffer+1), sz == 5?
          termination_condition[0] : termination_condition[buffer[5] & 0x3]);
          /* GRR:  not checking for valid buffer[1] values */
        if (sz > 6) {
          printf("    iteration min = %lu", LG(buffer+6));
          if (sz > 10) {
            printf(", max = %lu", LG(buffer+10));
            if (sz > 14) {
              long i, count = (sz-14) >> 2;

              printf(", signal number%s = %lu", (count > 1)? "s" : "",
                LG(buffer+14));
              for (i = 1;  i < count;  ++i)
                printf(", %lu", LG(buffer+14+(i<<2)));
            }
          }
          printf("\n");
        }
      }
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | ENDL |
     *------*/
    } else if (strcmp(chunkid, "ENDL") == 0) {
      if (png || jng) {
        printf("%s  ENDL not defined in %cNG\n", verbose? ":":fname,
          png? 'P':'J');
        set_err(kMinorError);
      } else if (sz != 1) {
        printf("%s  invalid %slength\n",
          verbose? ":":fname, verbose? "":"ENDL ");
        set_err(kMajorError);
      }
      if (verbose && no_err(kMinorError))
        printf(":  nest level = %u\n", (unsigned)(buffer[0]));
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | PROM |
     *------*/
    } else if (strcmp(chunkid, "PROM") == 0) {
      if (png || jng) {
        printf("%s  PROM not defined in %cNG\n", verbose? ":":fname,
          png? 'P':'J');
        set_err(kMinorError);
      } else if (sz != 3) {
        printf("%s  invalid %slength\n",
          verbose? ":":fname, verbose? "":"PROM ");
        set_err(kMajorError);
      }
      if (verbose && no_err(kMinorError)) {
        const char *ctype;

        switch (buffer[0]) {
          case 2:
            ctype = "gray+alpha";
            break;
          case 4:
            ctype = "RGB";
            break;
          case 6:
            ctype = "RGBA";
            break;
          default:
            ctype = inv;
            set_err(kMinorError);
            break;
        }
        printf("\n    new color type = %s, new bit depth = %u\n",
          ctype, (unsigned)(buffer[1]));
          /* GRR:  not checking for valid buffer[1] values */
        printf("    fill method (if bit depth increased) = %s\n",
          buffer[2]? "zero fill" : "left bit replication");
          /* GRR:  not checking for valid buffer[2] values */
      }
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | fPRI |
     *------*/
    } else if (strcmp(chunkid, "fPRI") == 0) {
      if (png || jng) {
        printf("%s  fPRI not defined in %cNG\n", verbose? ":":fname,
          png? 'P':'J');
        set_err(kMinorError);
      } else if (sz != 2) {
        printf("%s  invalid %slength\n",
          verbose? ":":fname, verbose? "":"fPRI ");
        set_err(kMajorError);
      }
      if (verbose && no_err(kMinorError))
        printf(":  %spriority = %u\n", buffer[0]? "delta " : "",
          (unsigned)(buffer[1]));
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | eXPI |
     *------*/
    } else if (strcmp(chunkid, "eXPI") == 0) {
      if (png || jng) {
        printf("%s  eXPI not defined in %cNG\n", verbose? ":":fname,
          png? 'P':'J');
        set_err(kMinorError);
      } else if (sz <= 2) {
        printf("%s  invalid %slength\n",
          verbose? ":":fname, verbose? "":"eXPI ");
        set_err(kMajorError);
      }
      if (verbose && no_err(kMinorError)) {
        printf("\n    snapshot ID = %u, snapshot name = %.*s\n", SH(buffer),
          (int)(sz-2), buffer+2);	/* GRR EBCDIC WARNING */
      }
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | BASI |
     *------*/
    } else if (strcmp(chunkid, "BASI") == 0) {
      if (png || jng) {
        printf("%s  BASI not defined in %cNG\n", verbose? ":":fname,
          png? 'P':'J');
        set_err(kMinorError);
      } else if (sz != 13 && sz != 19 && sz != 22) {
        printf("%s  invalid %slength\n",
          verbose? ":":fname, verbose? "":"BASI ");
        set_err(kMajorError);
      }
      if (no_err(kMinorError)) {
        w = LG(buffer);
        h = LG(buffer+4);
        if (w == 0 || h == 0) {
          printf("%s  invalid %simage dimensions (%ldx%ld)\n",
            verbose? ":":fname, verbose? "":"BASI ", w, h);
          set_err(kMinorError);
        }
        bitdepth = (uch)buffer[8];
        ityp = (uch)buffer[9];
        if (ityp > (int)(sizeof(png_type)/sizeof(char*))) {
          ityp = 1; /* avoid out of range array index */
        }
        switch (bitdepth) {
          case 1:
          case 2:
          case 4:
            if (ityp == 2 || ityp == 4 || ityp == 6) {  /* RGB or GA or RGBA */
              printf("%s  invalid %sbit depth (%d) for %s image\n",
                verbose? ":":fname, verbose? "":"BASI ", bitdepth,
                png_type[ityp]);
              set_err(kMinorError);
            }
            break;
          case 8:
            break;
          case 16:
            if (ityp == 3) { /* palette */
              printf("%s  invalid %sbit depth (%d) for %s image\n",
                verbose? ":":fname, verbose? "":"BASI ", bitdepth,
                png_type[ityp]);
              set_err(kMinorError);
            }
            break;
          default:
            printf("%s  invalid %sbit depth (%d)\n",
              verbose? ":":fname, verbose? "":"BASI ", bitdepth);
            set_err(kMinorError);
            break;
        }
        lace = (uch)buffer[12];
        switch (ityp) {
          case 2:
            bitdepth *= 3;   /* RGB */
            break;
          case 4:
            bitdepth *= 2;   /* gray+alpha */
            break;
          case 6:
            bitdepth *= 4;   /* RGBA */
            break;
        }
        if (verbose && no_err(kMinorError)) {
          printf("\n    %ld x %ld image, %d-bit %s, %sinterlaced\n", w, h,
            bitdepth, (ityp > 6)? png_type[1]:png_type[ityp], lace? "":"non-");
        }
        if (sz > 13) {
          ush red, green, blue;
          long alpha = -1;
          int viewable = -1;

          red = SH(buffer+13);
          green = SH(buffer+15);
          blue = SH(buffer+17);
          if (sz > 19) {
            alpha = (long)SH(buffer+19);
            if (sz > 21)
                viewable = buffer[21];
          }
          if (verbose && no_err(kMinorError)) {
            if (ityp == 0)
              printf("    gray = 0x%04x", red);
            else
              printf("    red = 0x%04x, green = 0x%04x, blue = 0x%04x",
                red, green, blue);
            if (alpha >= 0) {
              printf(", alpha = 0x%04lx", alpha);
              if (viewable >= 0)
                printf(", %sviewable", viewable? "" : "not ");
            }
            printf("\n");
          }
        }
      }
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | IPNG |    (empty stand-in for IHDR)
     *------*/
    } else if (strcmp(chunkid, "IPNG") == 0) {
      if (png || jng) {
        printf("%s  IPNG not defined in %cNG\n", verbose? ":":fname,
          png? 'P':'J');
        set_err(kMinorError);
      } else if (sz != 0) {
        printf("%s  invalid %slength\n",
          verbose? ":":fname, verbose? "":"IPNG ");
        set_err(kMinorError);
      } else if (verbose) {
        printf("\n");
      }
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | PPLT |
     *------*/
    } else if (strcmp(chunkid, "PPLT") == 0) {
      if (png || jng) {
        printf("%s  PPLT not defined in %cNG\n", verbose? ":":fname,
          png? 'P':'J');
        set_err(kMinorError);
      } else if (sz < 4) {
        printf("%s  invalid %slength\n",
          verbose? ":":fname, verbose? "":"PPLT ");
        set_err(kMinorError);
      } else {
        const char *plus;
        uch dtype = buffer[0];
        uch first_idx = buffer[1];
        uch last_idx = buffer[2];
        uch *buf = buffer+3;
        int bytes_left = sz-3;
        int samples, npplt = 0, nblks = 0;

        if (!verbose && printpal && !quiet)
          printf("  PPLT chunk");
        if (verbose)
          printf(": %s\n", (dtype < sizeof(pplt_delta_type)/sizeof(const char *))?
            pplt_delta_type[dtype] : inv);
        plus = (dtype & 1)? "+" : "";
        if (dtype < 2)
          samples = 3;
        else if (dtype < 4)
          samples = 1;
        else
          samples = 4;
        while (bytes_left > 0) {
          bytes_left -= samples*(last_idx - first_idx + 1);
          if (bytes_left < 0)
            break;
          ++nblks;
          for (i = first_idx;  i <= last_idx;  ++i, buf += samples) {
            ++npplt;
            if (printpal) {
              if (samples == 4)
                printf("    %3d:  %s(%3d,%3d,%3d,%3d) = "
                  "%s(0x%02x,0x%02x,0x%02x,0x%02x)\n", i,
                  plus, buf[0], buf[1], buf[2], buf[3],
                  plus, buf[0], buf[1], buf[2], buf[3]);
              else if (samples == 3)
                printf("    %3d:  %s(%3d,%3d,%3d) = %s(0x%02x,0x%02x,0x%02x)\n",
                  i, plus, buf[0], buf[1], buf[2],
                  plus, buf[0], buf[1], buf[2]);
              else
                printf("    %3d:  %s(%3d) = %s(0x%02x)\n", i,
                  plus, *buf, plus, *buf);
            }
          }
          if (bytes_left > 2) {
            first_idx = buf[0];
            last_idx = buf[1];
            buf += 2;
            bytes_left -= 2;
          } else if (bytes_left)
            break;
        }
        if (bytes_left) {
          printf("%s  invalid %slength (too %s bytes)\n",
            verbose? ":" : fname, verbose? "" : "PPLT ",
            (bytes_left < 0)? "few" : "many");
          set_err(kMinorError);
        }
        if (verbose && no_err(kMinorError))
          printf("    %d %s palette entr%s in %d block%s\n",
            npplt, (dtype & 1)? "delta" : "replacement", npplt== 1? "y":"ies",
            nblks, nblks== 1? "":"s");
      }
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | PAST |
     *------*/
    } else if (strcmp(chunkid, "PAST") == 0) {
      if (png || jng) {
        printf("%s  PAST not defined in %cNG\n", verbose? ":":fname,
          png? 'P':'J');
        set_err(kMinorError);
      } else if (sz < 41 || ((sz-11) % 30) != 0) {
        printf("%s  invalid %slength\n",
          verbose? ":":fname, verbose? "":"PAST ");
        set_err(kMajorError);
      } else if (buffer[2] > 2) {
        printf("%s  invalid %starget delta type (%u)\n",
          verbose? ":":fname, verbose? "":"PAST ", buffer[2]);
        set_err(kMinorError);
      }
      if (no_err(kMinorError)) {
        ush dest_id = SH(buffer);
        uch target_dtype = buffer[2];
        long x = LG(buffer+3);
        long y = LG(buffer+7);
        uch *buf = buffer+11;
        int bytes_left = sz-11;

        if (verbose)
          printf("\n    destination ID = %u, target = {%ld,%ld}%s\n", dest_id,
            x, y, target_dtype == 1? " (delta from previous PAST, same ID)" :
            (target_dtype == 2? " (delta from previous PAST)" : ""));

        /* now loop over remaining groups of 30 bytes */
        while (bytes_left > 0) {
          ush src_id        = SH(buf);
          uch comp_mode     = buf[2];
          uch orient        = buf[3];
          uch offset_origin = buf[4];
          long xoff         = LG(buf+5);
          long yoff         = LG(buf+9);
          uch bdry_origin   = buf[13];
          long left_clip    = LG(buf+14);
          long right_clip   = LG(buf+18);
          long top_clip     = LG(buf+22);
          long bott_clip    = LG(buf+26);

          if (src_id == 0) {
            printf("%s  invalid %ssource ID\n",
              verbose? ":":fname, verbose? "":"PAST ");
            set_err(kMinorError);
          } else if (comp_mode > 2) {
            printf("%s  invalid %scomposition mode (%u)\n",
              verbose? ":":fname, verbose? "":"PAST ", comp_mode);
            set_err(kMinorError);
          } else if (orient > 8 || (orient & 1)) {
            printf("%s  invalid %sorientation (%u)\n",
              verbose? ":":fname, verbose? "":"PAST ", orient);
            set_err(kMinorError);
          } else if (offset_origin > 1) {
            printf("%s  invalid %soffset origin (%u)\n",
              verbose? ":":fname, verbose? "":"PAST ", offset_origin);
            set_err(kMinorError);
          } else if (bdry_origin > 1) {
            printf("%s  invalid %sboundary origin (%u)\n",
              verbose? ":":fname, verbose? "":"PAST ", bdry_origin);
            set_err(kMinorError);
          }
          if (!no_err(kMinorError))
            break;

          if (verbose) {
              printf("    source ID = %u:  composition mode = %s,\n",
                src_id, composition_mode[comp_mode]);
              printf("      orientation = %s,\n", orientation[orient >> 1]);
              printf("      offset = {%ld,%ld} measured from {%ld,%ld} in "
                "destination image,\n", xoff, yoff,
                offset_origin? x:0, offset_origin? y:0);
              printf("      clipping box = {%ld,%ld} to {%ld,%ld} measured "
                "from {%ld,%ld}\n", left_clip, top_clip, right_clip, bott_clip,
                bdry_origin? x:0, bdry_origin? y:0);
          }
          buf += 30;
          bytes_left -= 30;
        }
      }
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | TERM |
     *------*/
    } else if (strcmp(chunkid, "TERM") == 0) {
      if (png || jng) {
        printf("%s  TERM not defined in %cNG\n", verbose? ":":fname,
          png? 'P':'J');
        set_err(kMinorError);
      } else if (have_TERM) {
        printf("%s  multiple TERM not allowed\n", verbose? ":":fname);
        set_err(kMinorError);
      } else if ((sz != 1 && sz != 10) ||
                 (sz == 1 && buffer[0] == 3) ||
                 (sz == 10 && buffer[0] != 3))
      {
        printf("%s  invalid %slength\n",
          verbose? ":":fname, verbose? "":"TERM ");
        set_err(kMajorError);
      } else if (buffer[0] > 3) {
        printf("%s  %sinvalid termination action\n",
               verbose? ":":fname, verbose? "":"TERM ");
        set_err(kMinorError);
      } else if (buffer[0] == 3 && buffer[1] > 2) {
        printf("%s  %sinvalid termination action-after-iterations\n",
               verbose? ":":fname, verbose? "":"TERM ");
        set_err(kMinorError);
      }
      if (verbose && no_err(kMinorError)) {
        printf("\n    action = %s\n", termination_action[buffer[0] /* & 3 */]);
        if (sz >= 10) {
          ulg val = LG(buffer+2);

          printf("    action after iterations = %s\n",
            termination_action[buffer[1]]);
          printf("    inter-iteration delay = %lu tick%s, max iterations = ",
            val, (val == 1)? "":"s");
          val = LG(buffer+6);
          if (val == 0x7fffffff)
            printf("infinite\n");
          else
            printf("%lu\n", val);
        }
      }
      have_TERM = 1;
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | DISC |
     *------*/
    } else if (strcmp(chunkid, "DISC") == 0) {
      if (png || jng) {
        printf("%s  DISC not defined in %cNG\n", verbose? ":":fname,
          png? 'P':'J');
        set_err(kMinorError);
      } else if (sz & 1) {
        printf("%s  invalid %slength\n",
          verbose? ":":fname, verbose? "":"DISC ");
        set_err(kMajorError);
      }
      if (verbose && no_err(kMinorError)) {
        if (sz == 0) {
          printf("\n    discard all nonzero objects%s\n",
            have_SAVE? " except those before SAVE":"");
        } else {
          uch *buf = buffer;
          int bytes_left = sz;

          printf(": %ld objects\n", sz >> 1);
          while (bytes_left > 0) {
            printf("    discard ID = %u\n", SH(buf));
            buf += 2;
            bytes_left -= 2;
          }
        }
      }
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | pHYg |
     *------*/
    } else if (strcmp(chunkid, "pHYg") == 0) {
      if (png || jng) {
        printf("%s  pHYg not defined in %cNG\n", verbose? ":":fname,
          png? 'P':'J');
        set_err(kMinorError);
      } else if (!top_level) {
        printf("%s  %smust appear at MNG top level\n",
          verbose? ":":fname, verbose? "":"pHYg ");
        set_err(kMinorError);
      } else if (sz != 9 && sz != 0) {
        printf("%s  invalid %slength\n",
          verbose? ":":fname, verbose? "":"pHYg ");
        set_err(kMajorError);
      } else if (sz && buffer[8] > 1) {
        printf("%s  invalid %sunit specifier (%u)\n",
          verbose? ":":fname, verbose? "":"pHYg ", buffer[8]);
        set_err(kMinorError);
      }
      if (verbose && no_err(kMinorError)) {
        if (sz == 0)
          printf("\n    %s\n",
            have_pHYg? "nullifies previous pHYg values":"(no effect)");
        else {
          ulg xres = LG(buffer);
          ulg yres = LG(buffer+4);
          unsigned units = buffer[8];

          printf(": %lux%lu pixels/%s", xres, yres, units? "meter":"unit");
          if (units && xres == yres)
            printf(" (%lu dpi)", (ulg)(xres*0.0254 + 0.5));
          else if (!units) {
            ulg gcf_xres_yres = gcf(xres, yres);

            printf(" (%lu:%lu)", xres/gcf_xres_yres, yres/gcf_xres_yres);
          }
          printf("\n");
        }
      }
      have_pHYg = 1;
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | DROP |
     *------*/
    } else if (strcmp(chunkid, "DROP") == 0) {
      if (png || jng) {
        printf("%s  DROP not defined in %cNG\n", verbose? ":":fname,
          png? 'P':'J');
        set_err(kMinorError);
      } else if (sz & 0x3) {
        printf("%s  invalid %slength\n",
          verbose? ":":fname, verbose? "":"DROP ");
        set_err(kMajorError);
      }
      if (no_err(kMinorError)) {
        uch *buf = buffer;
        int bytes_left = sz;
        int num_names = 0;

        while (bytes_left > 0) {
          if (check_chunk_name((const char *)buf) != 0) {
            printf("%s  invalid chunk name to be dropped\n",
              verbose? ":":fname);
            set_err(kMinorError);
            break;
          }
          if (verbose)
            printf("%s%.*s", (num_names%12)? " ":"\n    ", 4, buf);
          ++num_names;
          buf += 4;
          bytes_left -= 4;
        }
        if (verbose)
          printf("\n");
      }
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | DBYK |
     *------*/
    /* NOTE:  the spec's "keyword at beginning" and "null-terminated" restric-
     *  tions limit the (known) chunk types that can be dropped to iCCP, pCAL,
     *  iTXt, tEXt, and zTXt--and the three text chunks are irrelevant in
     *  any case.  Other chunks with keyword-like fields that do NOT qualify
     *  include FRAM, SAVE, SEEK, and eXPI. */
    } else if (strcmp(chunkid, "DBYK") == 0) {
      if (png || jng) {
        printf("%s  DBYK not defined in %cNG\n", verbose? ":":fname,
          png? 'P':'J');
        set_err(kMinorError);
      } else if (sz < 6) {
        printf("%s  invalid %slength\n",
          verbose? ":":fname, verbose? "":"DBYK ");
        set_err(kMajorError);
      } else if (buffer[4] > 1) {
        printf("%s  invalid %spolarity (%u)\n",
          verbose? ":":fname, verbose? "":"DBYK ", buffer[4]);
        set_err(kMinorError);
      } else if (check_chunk_name((const char *)buffer) != 0) {
        printf("%s  invalid chunk name to be dropped\n",
          verbose? ":":fname);
        set_err(kMinorError);
      }
      if (no_err(kMinorError)) {
        uch *buf = buffer + 5;
        int bytes_left = sz - 5;
        int first = 1;
        int space_left = 75;

        if (verbose) {
          printf("\n    %.*s: drop %s", 4, buffer, buffer[4]? "all but":"only");
          space_left -= buffer[4]? 18:15;   /* e.g., "cHNK: drop all but" */
        }
        while (bytes_left > 0) {
          const char *sep;
          int keylen;

          if (check_keyword(buf, bytes_left, &keylen, "keyword", chunkid))
            set_err(kMinorError);
          else if (keylen < bytes_left && buf[keylen] != 0) {
            /* realistically, this can never happen (due to keywordlen())... */
            printf("%s  unterminated %skeyword\n",
              verbose? ":":fname, verbose? "":"DBYK ");
            set_err(kMinorError);
          }
          if (!no_err(kMinorError))
            break;
          if (verbose) {
            /* account for trailing comma (along with space and two quotes)
             * even though we're not yet printing it, because we'll need to
             * add it *on the same line* if there are any more keywords: */
            if (keylen+4 < space_left) {
              sep = first? "":",";
              space_left -= keylen+4;
            } else {
              sep = ",\n     ";  /* indent two extra spaces */
              space_left = 75 - 2 - (keylen+4);
            }
            printf("%s \"%.*s\"", sep, keylen, buf);
          }
          first = 0;
          buf += keylen+1;        /* no NULL separator for last keyword... */
          bytes_left -= keylen+1; /* ...but then bytes_left will be < 0:  NP */
        }
        if (verbose)
          printf("\n");
      }
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | ORDR |
     *------*/
    } else if (strcmp(chunkid, "ORDR") == 0) {
      if (png || jng) {
        printf("%s  ORDR not defined in %cNG\n", verbose? ":":fname,
          png? 'P':'J');
        set_err(kMinorError);
      } else if (sz % 5) {
        printf("%s  invalid %slength\n",
          verbose? ":":fname, verbose? "":"ORDR ");
        set_err(kMajorError);
      }
      if (no_err(kMinorError)) {
        uch *buf = buffer;
        int bytes_left = sz;

        if (verbose)
          printf("\n");
        while (bytes_left > 0) {
          if (check_chunk_name((const char *)buf) != 0) {
            printf("%s  %slisted chunk name is invalid\n",
              verbose? ":":fname, verbose? "":"ORDR: ");
            set_err(kMinorError);
          } else if (!(buf[0] & 0x20)) {
            printf("%s  %scritical chunk (%.*s) not allowed\n",
              verbose? ":":fname, verbose? "":"ORDR: ", 4, buf);
            set_err(kMinorError);
          } else if (buf[4] > 4) {
            printf("%s  invalid %sordering value\n",
              verbose? ":":fname, verbose? "":"ORDR ");
            set_err(kMinorError);
          }
          if (!no_err(kMinorError))
            break;
          if (verbose)
            printf("    %.*s: %s\n", 4, buf, order_type[buf[4]]);
          buf += 5;
          bytes_left -= 5;
        }
      }
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | MAGN |
     *------*/
    } else if (strcmp(chunkid, "MAGN") == 0) {
      if (png || jng) {
        printf("%s  MAGN not defined in %cNG\n", verbose? ":":fname,
          png? 'P':'J');
        set_err(kMinorError);
      } else if ((sz <= 4 && (sz & 1)) ||
                 (sz >= 5 && sz <= 17 && !(sz & 1)) ||
                 sz > 18)
      {
        printf("%s  invalid %slength\n",
          verbose? ":":fname, verbose? "":"MAGN ");
        set_err(kMajorError);
      }
      if (no_err(kMinorError)) {
        if (sz == 0) {
          if (verbose)
            printf("\n    %s\n",
              have_MAGN? "nullifies previous MAGN values":"(no effect)");
        } else {
          ush first = SH(buffer);
          ush last = first;
          uch xmeth = 0;	/* no X magnification */
          ush mx = 1;
          ush my = mx;
          ush ml = mx;
          ush mr = mx;
          ush mt = my;
          ush mb = my;
          uch ymeth = xmeth;

          if (sz > 2)
            last = SH(buffer+2);
          if (sz > 4)
            xmeth = buffer[4];
          if (xmeth) {
            if (sz > 5)
              mx = SH(buffer+5);
            if (sz > 9)
              ml = SH(buffer+9);
            if (sz > 11)
              mr = SH(buffer+11);
          }
          if (sz > 17)
            ymeth = buffer[17];
          if (ymeth) {
            if (sz > 7)
              my = SH(buffer+7);
            if (sz > 13)
              mt = SH(buffer+13);
            if (sz > 15)
              mb = SH(buffer+15);
          }
          if (xmeth > 5 || ymeth > 5) {
            printf("%s  invalid %smagnification method(s)\n",
              verbose? ":":fname, verbose? "":"MAGN ");
            set_err(kMinorError);
          }
          if (verbose && no_err(kMinorError)) {
            printf("\n    magnified object ID");
            if (first == last)
              printf(" = %u\n", first);
            else
              printf("s = %u to %u\n", first, last);
            if (xmeth == ymeth)
              printf("    method = %s\n", magnification_method[xmeth]);
            else
              printf("    X method = %s\n    Y method = %s\n",
                magnification_method[xmeth], magnification_method[ymeth]);
            printf("    X mag = %u, left mag = %u, right mag = %u\n",
              mx, ml, mr);
            printf("    Y mag = %u, top mag = %u, bottom mag = %u\n",
              my, mt, mb);
          }
        }
      }
      have_MAGN = 1;
      last_is_IDAT = last_is_JDAT = 0;

    /*------*
     | MEND |
     *------*/
    } else if (strcmp(chunkid, "MEND") == 0) {
      if (png || jng) {
        printf("%s  MEND not defined in %cNG\n", verbose? ":":fname,
          png? 'P':'J');
        set_err(kMinorError);
      } else if (have_MEND) {
        printf("%s  multiple MEND not allowed\n", verbose? ":":fname);
        set_err(kMinorError);
      } else if (sz != 0) {
        printf("%s  invalid %slength\n",
               verbose? ":":fname, verbose? "":"MEND ");
        set_err(kMinorError);
      } else if (verbose) {
        printf("\n");
      }
      have_MEND = 1;
      last_is_IDAT = last_is_JDAT = 0;

    /*===============*
     * unknown chunk *
     *===============*/

    } else {
      if (CRITICAL(chunkid) && SAFECOPY(chunkid)) {
        /* a critical, safe-to-copy chunk is an error */
        printf("%s  illegal critical, safe-to-copy chunk%s%s\n",
               verbose? ":":fname, verbose? "":" ", verbose? "":chunkid);
        set_err(kMajorError);
      } else if (RESERVED(chunkid)) {
        /* a chunk with the reserved bit set is an error (or spec updated) */
        printf("%s  illegal reserved-bit-set chunk%s%s\n",
               verbose? ":":fname, verbose? "":" ", verbose? "":chunkid);
        set_err(kMajorError);
      } else if (PUBLIC(chunkid)) {
        /* GRR 20050725:  all registered (public) PNG/MNG/JNG chunks are now
         *  known to pngcheck, so any unknown public ones are invalid (or have
         *  been proposed and approved since the last release of pngcheck) */
        printf("%s  illegal (unless recently approved) unknown, public "
               "chunk%s%s\n", verbose? ":":fname, verbose? "":" ",
               verbose? "":chunkid);
        set_err(kMajorError);
      } else if (/* !PUBLIC(chunkid) && */ CRITICAL(chunkid) &&
                 !suppress_warnings)
      {
        /* GRR 20060617:  as Chris Nokleberg noted, "private, critical chunks
         *  should not be used in publicly available software or files" (PNG
         *  spec) */
        printf("%s  private, critical chunk%s%s (warning)\n",
               verbose? ":":fname, verbose? "":" ", verbose? "":chunkid);
        set_err(kWarning);  /* not an error if used only internally */
      } else if (verbose) {
        printf("\n    unknown %s, %s, %s%ssafe-to-copy chunk\n",
               PRIVATE(chunkid)   ? "private":"public",
               ANCILLARY(chunkid) ? "ancillary":"critical",
               RESERVED(chunkid)  ? "reserved-bit-set, ":"",
               SAFECOPY(chunkid)  ? "":"un");
      }
      last_is_IDAT = last_is_JDAT = 0;
    }

    /*=======================================================================*/

    if (no_err(kMinorError)) {
      if (fpOut != NULL) {
        putlong(fpOut, sz);
        (void)fwrite(chunkid, 1, 4, fpOut);
        (void)fwrite(buffer, 1, toread, fpOut);
      }

      while (sz > toread) {
        int data_read;
        sz -= toread;
        toread = (sz > BS)? BS:sz;

        data_read = (int)fp->read(buffer, toread);
        if (fpOut != NULL)
          (void)fwrite(buffer, 1, data_read, fpOut);

        if (data_read != toread) {
          printf("%s  EOF while reading %s%sdata\n",
                 verbose? ":":fname, verbose? "":chunkid, verbose? "":" ");
          set_err(kCriticalError);
          return global_error;
        }

        crc = update_crc(crc, (uch *)buffer, toread);
      }

      filecrc = getlong("CRC value");

      if (is_err(kMajorError))
        return global_error;

      if (filecrc != CRCCOMPL(crc)) {
        printf("%s  CRC error in chunk %s (computed %08lx, expected %08lx)\n",
               verbose? "":fname, chunkid, CRCCOMPL(crc), filecrc);
        set_err(kMinorError);
      }

      if (no_err(kMinorError) && fpOut != NULL)
        putlong(fpOut, CRCCOMPL(crc));

    } else if (force) {
      /* force may result in set_err(kMajorError) or more upstream, and failing
       * to read CRC bytes here guarantees immediate downstream error when
       * attempting to read length bytes and chunk type/name bytes */
      filecrc = getlong("CRC value");
    }

    if (global_error > kWarning && !force)
      return global_error;
  }

  /*----------------------- END OF IMMENSE WHILE-LOOP -----------------------*/

  if (no_err(kMinorError)) {
    if (((png || jng) && !have_IEND) || (mng && !have_MEND)) {
      printf("%s  file doesn't end with a%sEND chunk\n", verbose? "":fname,
        mng? " M":"n I");
      set_err(kMinorError);
    }
  }

  if (global_error > kWarning)
    return global_error;

  /* GRR 19970621: print compression ratio based on file size vs. byte-packed
   *   raw data size.  Arguably it might be fairer to compare against the size
   *   of the unadorned, compressed data, but since PNG is a package deal...
   * GRR 19990619: disabled for MNG, at least until we figure out a reasonable
   *   way to calculate the ratio; also switched to MNG-relevant stats. */

#ifdef RP_PRINTF_ENABLED
  /* if (global_error == 0) */ {   /* GRR 20061202:  always print a summary */
    if (mng) {
      if (verbose) {  /* already printed MHDR/IHDR/JHDR info */
        printf("%s in %s (%ld chunks).\n",
          global_error? warnings_detected : no_errors_detected, fname,
          num_chunks);
      } else if (!quiet) {
        printf("%s: %s%s%s (%ldx%ld, %ld chunks",
          global_error? brief_warn : brief_OK,
          color? COLOR_YELLOW:"", fname, color? COLOR_NORMAL:"",
          mng_width, mng_height, num_chunks);
        if (vlc == 1)
          printf(", VLC");
        else if (lc == 1)
          printf(", LC");
        if (layers && layers < 0x7ffffffL)
          printf(", %lu layer%s", layers, (layers == 1L)? "" : "s");
        if (frames && frames < 0x7ffffffL)
          printf(", %lu frame%s", frames, (frames == 1L)? "" : "s");
        printf(").\n");
      }

    } else if (jng) {
      const char *sgn = "";
      int cfactor;
      ulg ucsize;

      if (!did_stat) {
        stat(fname, &statbuf);   /* already know file exists; don't check rc */
      }

      /* uncompressed size (bytes), compressed size => returns 10*ratio (%) */
      if (bitdepth == 20)
        ucsize = h*(w + ((w*12+7)>>3));
      else
        ucsize = h*((w*bitdepth+7)>>3);
      if (alphadepth > 0)
        ucsize += h*((w*alphadepth+7)>>3);
      if ((cfactor = ratio(ucsize, statbuf.st_size)) < 0)
      {
        sgn = "-";
        cfactor = -cfactor;
      }

      if (verbose) {  /* already printed JHDR info */
        printf("%s in %s (%ld chunks, %s%d.%d%% compression).\n",
          global_error? warnings_detected : no_errors_detected, fname,
          num_chunks, sgn, cfactor/10, cfactor%10);
      } else if (!quiet) {
        if (jtyp < 2)
          printf("%s: %s%s%s (%ldx%ld, %d-bit %s%s%s, %s%d.%d%%).\n",
            global_error? brief_warn : brief_OK,
            color? COLOR_YELLOW:"", fname, color? COLOR_NORMAL:"",
            w, h, jbitd, and_str, jng_type[jtyp],
            lace? ", progressive":"", sgn, cfactor/10, cfactor%10);
        else
          printf("%s: %s%s%s (%ldx%ld, %d-bit %s%s + %d-bit alpha%s, %s%d.%d%%)"
            ".\n", global_error? brief_warn : brief_OK,
            color? COLOR_YELLOW:"", fname, color? COLOR_NORMAL:"",
            w, h, jbitd, and_str, jng_type[jtyp-2],
            alphadepth, lace? ", progressive":"", sgn, cfactor/10, cfactor%10);
      }

    } else {
      const char *sgn = "";
      int cfactor;

      if (!did_stat)
        stat(fname, &statbuf);   /* already know file exists */

      /* uncompressed size (bytes), compressed size => returns 10*ratio (%) */
      if ((cfactor = ratio((ulg)(h*((w*bitdepth+7)>>3)), statbuf.st_size)) < 0)
      {
        sgn = "-";
        cfactor = -cfactor;
      }

      if (verbose) {  /* already printed IHDR/JHDR info */
        printf("%s in %s (%ld chunks, %s%d.%d%% compression).\n",
          global_error? warnings_detected : no_errors_detected, fname,
          num_chunks, sgn, cfactor/10, cfactor%10);
      } else if (!quiet) {
        printf("%s: %s%s%s (%ldx%ld, %d-bit %s%s, %sinterlaced, %s%d.%d%%).\n",
          global_error? brief_warn : brief_OK,
          color? COLOR_YELLOW:"", fname, color? COLOR_NORMAL:"",
          w, h, bitdepth, (ityp > 6)? png_type[1] : png_type[ityp],
          (ityp == 3 && have_tRNS)? "+trns" : "",
          lace? "" : "non-", sgn, cfactor/10, cfactor%10);
      }
    }

  }
#endif /* rom-properties */

  return global_error;

} /* end function pngcheck() */



#if 0 /* rom-properties */
static int pnginfile(FILE *fp, const char *fname, int ipng, int extracting)
{
  char name[1024], *szdot;
  int err = kOK;
  FILE *fpOut = NULL;

#if 1
  strncpy(name, fname, 1024-20);
  name[1024-20] = 0;
  szdot = strrchr(name, '.');
  if (szdot == NULL)
    szdot = name + strlen(name);
  sprintf(szdot, "-%d", ipng);
#else
  /* Use this if filename length is restricted. */
  sprintf(name, "PNG%d", ipng);
  szdot = name;
#endif

  if (extracting) {
    szdot += strlen(szdot);
    strcpy(szdot, ".png");
    fpOut = fopen(name, "wb");
    if (fpOut == NULL) {
      perror(name);
      fprintf(stderr, "%s: could not write output (ignored)\n", name);
    } else if (verbose) {
      printf("%s: contains %s PNG %d\n", name, fname, ipng);
    }
    (void)fwrite(good_PNG_magic, 8, 1, fpOut);
    *szdot = 0;
  }

  err = pngcheck(fp, name, 1, fpOut);

  if (fpOut != NULL) {
    if (ferror(fpOut) != 0 || fclose(fpOut) != 0) {
      perror(name); /* will only show most recent error */
      fprintf(stderr, "%s: error on output (ignored)\n", name);
    }
  }

  return err;
}



static void pngsearch(FILE *fp, const char *fname, int extracting)
{
  /* Go through the file looking for a PNG magic number; if one is
     found, check the data to see if it is a PNG and validate the
     contents.  Useful when something puts a PNG in something else. */
  int ch;
  int ipng = 0;

  if (verbose)
    printf("Scanning: %s\n", fname);
  else if (extracting)
    printf("Extracting PNGs from %s\n", fname);

  /* This works because the leading 137 code is not repeated in the
     magic, so ANSI C says we will break out of the comparison as
     soon as the partial match fails, and we can start a new test.
   */
  do {
    ch = getc(fp);
    while (ch == good_PNG_magic[0]) {
      if ((ch = getc(fp)) == good_PNG_magic[1] &&
          (ch = getc(fp)) == good_PNG_magic[2] &&
          (ch = getc(fp)) == good_PNG_magic[3] &&
          (ch = getc(fp)) == good_PNG_magic[4] &&
          (ch = getc(fp)) == good_PNG_magic[5] &&
          (ch = getc(fp)) == good_PNG_magic[6] &&
          (ch = getc(fp)) == good_PNG_magic[7])
      {
        /* just after a PNG header */
        /* int error = */ pnginfile(fp, fname, ++ipng, extracting);
      }
    }
  } while (ch != EOF);
}
#endif /* rom-properties */



/*  PNG_subs
 *
 *  Utility routines for PNG encoders and decoders
 *  by Glenn Randers-Pehrson
 *
 */

/* check_magic()
 *
 * Check the magic numbers in 8-byte buffer at the beginning of
 * a (possible) PNG or MNG or JNG file.
 *
 * by Alexander Lehmann, Glenn Randers-Pehrson and Greg Roelofs
 *
 * This is free software; you can redistribute it and/or modify it
 * without any restrictions.
 *
 */
int CPngCheck::check_magic(uch *magic, int which)
{
  int i;
  const uch *good_magic = (which == 0)? good_PNG_magic :
                          ((which == 1)? good_MNG_magic : good_JNG_magic);

  for (i = 1; i < 3; ++i)
  {
    if (magic[i] != good_magic[i]) {
      return 2;
    }
  }

  if (magic[0] != good_magic[0] ||
      magic[4] != good_magic[4] || magic[5] != good_magic[5] ||
      magic[6] != good_magic[6] || magic[7] != good_magic[7]) {

    if (!verbose) {
      printf("%s:  CORRUPTED by text conversion\n", fname);
      return 1;
    }

    printf("  File is CORRUPTED.  It seems to have suffered ");

    /* This coding derived from Alexander Lehmann's checkpng code   */
    if (strncmp((const char *)&magic[4], "\012\032", 2) == 0)
      printf("DOS->Unix");
    else if (strncmp((const char *)&magic[4], "\015\032", 2) == 0)
      printf("DOS->Mac");
    else if (strncmp((const char *)&magic[4], "\015\015\032", 3) == 0)
      printf("Unix->Mac");
    else if (strncmp((const char *)&magic[4], "\012\012\032", 3) == 0)
      printf("Mac->Unix");
    else if (strncmp((const char *)&magic[4], "\012\012", 2) == 0)
      printf("DOS->Unix");
    else if (strncmp((const char *)&magic[4], "\015\015\012\032", 4) == 0)
      printf("Unix->DOS");
    else if (strncmp((const char *)&magic[4], "\015\012\032\015", 4) == 0)
      printf("Unix->DOS");
    else if (strncmp((const char *)&magic[4], "\015\012\012", 3) == 0)
      printf("DOS EOF");
    else if (strncmp((const char *)&magic[4], "\015\012\032\012", 4) != 0)
      printf("EOL");
    else
      printf("an unknown");

    printf(" conversion.\n");

    if (magic[0] == 9)
      printf("  It was probably transmitted through a 7-bit channel.\n");
    else if (magic[0] != good_magic[0])
      printf("  It was probably transmitted in text mode.\n");

    return 1;
  }

  return 0;
}



/* GRR 20061203:  now EBCDIC-safe */
int CPngCheck::check_chunk_name(const char *chunk_name)
{
  if (isASCIIalpha((int)chunk_name[0]) && isASCIIalpha((int)chunk_name[1]) &&
      isASCIIalpha((int)chunk_name[2]) && isASCIIalpha((int)chunk_name[3]))
    return 0;

  printf("%s%s  invalid chunk name \"%.*s\" (%02x %02x %02x %02x)\n",
    verbose? "":fname, verbose? "":":", 4, chunk_name,
    chunk_name[0], chunk_name[1], chunk_name[2], chunk_name[3]);
  set_err(kMajorError);  /* usually means we've "jumped the tracks": bail! */
  return 1;
}



/* GRR 20050724 */
/* caller must do set_err(kMinorError) based on return value (0 == OK) */
/* keyword_name is "keyword" for most chunks, but it can instead be "name" or
 * "identifier" or whatever makes sense for the chunk in question */
int CPngCheck::check_keyword(uch *buffer, int maxsize, int *pKeylen,
                  const char *keyword_name, const char *chunkid)
{
  int j, prev_space = 0;
  int keylen = keywordlen(buffer, maxsize);

  if (pKeylen)
    *pKeylen = keylen;

  if (keylen == 0) {
    printf("%s  zero length %s%s%s\n",
      verbose? ":":fname, verbose? "":chunkid, verbose? "":" ", keyword_name);
    return 1;
  }

  if (keylen > 79) {
    printf("%s  %s %s is longer than 79 characters\n",
      verbose? ":":fname, verbose? "":chunkid, keyword_name);
    return 2;
  }

  if (buffer[0] == ' ') {
    printf("%s  %s %s has leading space(s)\n",
      verbose? ":":fname, verbose? "":chunkid, keyword_name);
    return 3;
  }

  if (buffer[keylen - 1] == ' ') {
    printf("%s  %s %s has trailing space(s)\n",
      verbose? ":":fname, verbose? "":chunkid, keyword_name);
    return 4;
  }

  for (j = 0; j < keylen; ++j) {
    if (buffer[j] == ' ') {
      if (prev_space) {
        printf("%s  %s %s has consecutive spaces\n",
          verbose? ":":fname, verbose? "":chunkid, keyword_name);
        return 5;
      }
      prev_space = 1;
    } else {
      prev_space = 0;
    }
  }

  for (j = 0; j < keylen; ++j) {
    if (latin1_keyword_forbidden[buffer[j]]) {   /* [0,31] || [127,160] */
      printf("%s  %s %s has control character(s) (%u)\n",
        verbose? ":":fname, verbose? "":chunkid, keyword_name, buffer[j]);
      return 6;
    }
  }

  return 0;
}



/* GRR 20070707 */
/* caller must do set_err(kMinorError) based on return value (0 == OK) */
int CPngCheck::check_text(uch *buffer, int maxsize, const char *chunkid)
{
  int j, ctrlwarn = verbose? 1 : 0;  /* print message once, only if verbose */

  for (j = 0; j < maxsize; ++j) {
    if (buffer[j] == 0) {
      printf("%s  %s text contains NULL character(s)\n",
        verbose? ":":fname, verbose? "":chunkid);
      return 1;
    } else if (ctrlwarn && latin1_text_discouraged[buffer[j]]) {
      printf(":   text has control character(s) (%u) (discouraged)\n",
        buffer[j]);
      ctrlwarn = 0;
    }
  }

  return 0;
}



/* GRR 20061203 (used only for sCAL) */
/* caller must do set_err(kMinorError) based on return value (0 == OK) */
int CPngCheck::check_ascii_float(uch *buffer, int len, const char *chunkid)
{
  uch *qq = buffer, *bufEnd = buffer + len;
  int have_sign = 0, have_integer = 0, have_dot = 0, have_fraction = 0;
  int have_E = 0, have_Esign = 0, have_exponent = 0, in_digits = 0;
  int have_nonzero = 0;
  int rc = 0;

  for (qq = buffer;  qq < bufEnd && !rc;  ++qq) {
    switch (*qq) {
      case '+':
      case '-':
        if (qq == buffer) {
          have_sign = 1;
          in_digits = 0;
        } else if (have_E && !have_Esign) {
          have_Esign = 1;
          in_digits = 0;
        } else {
          printf("%s  invalid sign character%s%s (buf[%td])\n",
            verbose? ":":fname, verbose? "":" in ", verbose? "":chunkid,
            qq-buffer);		// ptrdiff_t
          rc = 1;
        }
        break;

      case '.':
        if (!have_dot && !have_E) {
          have_dot = 1;
          in_digits = 0;
        } else {
          printf("%s  invalid decimal point%s%s (buf[%td])\n",
            verbose? ":":fname, verbose? "":" in ", verbose? "":chunkid,
            qq-buffer);		// ptrdiff_t
          rc = 2;
        }
        break;

      case 'e':
      case 'E':
        if (have_integer || have_fraction) {
          have_E = 1;
          in_digits = 0;
        } else {
          printf("%s  invalid exponent before mantissa%s%s (buf[%td])\n",
            verbose? ":":fname, verbose? "":" in ", verbose? "":chunkid,
            qq-buffer);		// ptrdiff_t
          rc = 3;
        }
        break;

      default:
        if (*qq < '0' || *qq > '9') {
          printf("%s  invalid character ('%c' = 0x%02x)%s%s\n",
            verbose? ":":fname, *qq, *qq,
            verbose? "":" in ", verbose? "":chunkid);
          rc = 4;
        } else if (in_digits) {
          /* still in digits:  do nothing except check for non-zero digits */
          if (!have_exponent && *qq != '0')
            have_nonzero = 1;
        } else if (!have_integer && !have_dot) {
          have_integer = 1;
          in_digits = 1;
          if (*qq != '0')
            have_nonzero = 1;
        } else if (have_dot && !have_fraction) {
          have_fraction = 1;
          in_digits = 1;
          if (*qq != '0')
            have_nonzero = 1;
        } else if (have_E && !have_exponent) {
          have_exponent = 1;
          in_digits = 1;
        } else {
          /* is this case possible? */
          printf("%s  invalid digits%s%s (buf[%td])\n",
            verbose? ":":fname, verbose? "":" in ", verbose? "":chunkid,
            qq-buffer);		// ptrdiff_t
          rc = 5;
        }
        break;
    }
  }

  /* must have either integer part or fractional part; all else is optional */
  if (rc == 0 && !have_integer && !have_fraction) {
    printf("%s  missing mantissa%s%s\n",
      verbose? ":":fname, verbose? "":" in ", verbose? "":chunkid);
    rc = 6;
  }

  /* non-exponent part must be non-zero (=> must have seen a non-zero digit) */
  if (rc == 0 && !have_nonzero) {
    if (verbose)
      printf(":  invalid zero value(s)\n");
    else
      printf("%s  invalid zero %s value(s)\n", fname, chunkid);
    rc = 7;
  }

  return rc;
}

}
