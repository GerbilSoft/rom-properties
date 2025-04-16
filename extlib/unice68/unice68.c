/*
 * @file    unice68.c
 * @brief   program to pack and depack ICE! packed files
 * @author  http://sourceforge.net/users/benjihan
 *
 * Copyright (c) 1998-2016 Benjamin Gerard
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.
 *
 * If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifdef HAVE_CONFIG_H
# include <config.unice68.h>
#endif

#ifndef PACKAGE_NAME
# define PACKAGE_NAME "unice68"
#endif

#ifndef PACKAGE_VERSION
# define PACKAGE_VERSION __DATE__
#endif

#include "unice68.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <assert.h>
#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_IO_H
#include <io.h>
#endif

/* define _O_BINARY */
#ifndef _O_BINARY
# ifdef O_BINARY
#  define _O_BINARY O_BINARY
# else
#  define _O_BINARY 0
# endif
#endif

#ifdef _MSC_VER
# ifndef __cplusplus
#  define inline __inline
# endif
# define UNICE68_CDECL __cdecl
#else
# define UNICE68_CDECL
#endif

static char     * prg;
static int        verbose;
static FILE     * msgout;
static unsigned   memmax = 1<<24;

#ifndef HAVE_BASENAME
# ifndef PATHSEP
#  ifdef UNICE68_WIN32
#   define PATHSEP '\\'
#  else
#   define PATHSEP '/'
#  endif
# endif
static char *basename(char *path)
{
  char * s = strrchr(path, PATHSEP);
  return !s ? path : s+1;
}
#endif

/* Message levels */
enum {
  D = 2,                        /* Message level Debug   */
  V = 1,                        /* Message level verbose */
  N = 0,                        /* Message level normal  */
  E = -1                        /* Message level error   */
};

static inline int myabs(const int v) {
  return v < 0 ? -v : v;
}

static void message_va(const int level, const char *fmt, va_list list)
{
  if (verbose >= level) {
    int save_errno = errno;
    FILE * out =  (level == E) ? stderr : msgout;
    vfprintf(out,fmt,list);
    fflush(out);
    errno = save_errno;
  }
}

static void message(const int level, const char *fmt, ...)
{
  va_list list;
  va_start(list, fmt);
  message_va(level,fmt,list);
  va_end(list);
}

static void error(const char * fmt, ...)
{
  va_list list;
  va_start(list, fmt);
  message(E,"%s: ", prg);
  message_va(E,fmt,list);
  va_end(list);
}

static void syserror(const char * obj)
{
  message(E,"%s: %s -- %s\n", prg, obj, strerror(errno));
}

static int print_usage(void)
{
  int ice_d_version = unice68_unice_version();
  int ice_p_version = unice68_ice_version();

  printf
    (
      "Usage: %s [MODE] [OPTION...] [--] [[<input>] <output>]\n"
      "\n"
      "ICE! depacker %x.%02x\n"
      "       packer %x.%02x\n"
      "\n"
      " `-' as input/output uses respectively stdin/stdout.\n"
      " If output is stdout all messages are diverted to stderr.\n"
      "\n"
      "Modes:\n"
      "\n"
      "  -h --help      Print this message and exit\n"
      "  -V --version   Print version and copyright and exit\n"
      /* "  -c --cat       Output input file as is if not ICE! packed\n" */
      "  -t --test      Test if input is an ICE! packed file\n"
      "  -T --deep-test Test if input is a valid ICE! packed file\n"
      "  -d --depack    Depack mode\n"
      "  -p --pack      Pack mode\n"
      "  -P --pack-old  Force pack with deprecated 'Ice!' identifier\n"
      "  -s --stress    Pack and unpack <input> for testing\n"
      "\n"
      " If no mode is given the default is to pack an unpacked file\n"
      " and to unpack a packed one.\n"
      "\n"
      "Options:\n"
      "\n"
      "  -n --no-limit  Ignore memory sanity check\n"
      "  -v --verbose   Be more verbose (multiple use possible)\n"
      "  -q --quiet     Be less verbose (multiple use possible)\n"
      "\n"
      "Copyright (c) 1998-2016 Benjamin Gerard\n"
      "\n"
      "Visit <" PACKAGE_URL ">\n"
      "Report bugs to <" PACKAGE_BUGREPORT ">\n",
      prg,
      (unsigned int)(ice_d_version>>8), (unsigned int)(ice_d_version&255),
      (unsigned int)(ice_p_version>>8), (unsigned int)(ice_p_version&255)
      );
  return 0;
}

static int print_version(void)
{
  puts(unice68_versionstr());
  puts
    (
      "\n"
      "Copyright (c) 1998-2016 Benjamin Gerard.\n"
      "License GPLv3+ or later <http://gnu.org/licenses/gpl.html>\n"
      "This is free software: you are free to change and redistribute it.\n"
      "There is NO WARRANTY, to the extent permitted by law.\n"
      "\n"
      "Written by Benjamin Gerard"
      );
  return 0;
}

static unsigned hash_buffer(const char * buf, unsigned int len)
{
  const unsigned char * k = (unsigned char *) buf;
  unsigned int h = 0;
  if (len > 0)
    do {
      h += *k++;
      h += h << 10;
      h ^= h >> 6;
    } while (--len);

  return h;
}

#if _O_BINARY

/* Fallback file descriptors if all other methods failed */
#ifndef STDIN_FILENO
# define STDIN_FILENO 0
#endif
#ifndef STDOUT_FILENO
# define STDOUT_FILENO 1
#endif
#ifndef STDERR_FILENO
# define STDERR_FILENO 2
#endif

/* If _fileno is a macro just pretends we have the function. */
#if defined(_fileno) && !defined(HAVE__FILENO)
# define HAVE__FILENO 1
#endif
/* If fileno is a macro just pretends we have the function. */
#if defined(fileno) && !defined(HAVE_FILENO)
# define HAVE_FILENO 1
#endif

/* If _setmode is a macro just pretend we have the function. */
#if defined(_setmode) && !defined(HAVE__SETMODE)
# define HAVE__SETMODE 1
#endif
/* If setmode is a macro just pretend we have the function. */
#if defined(setmode) && !defined(HAVE_SETMODE)
# define HAVE_SETMODE 1
#endif

static int myfileno(FILE * file)
{
#if defined(HAVE_FILENO)
  return fileno(file);
#elif defined(HAVE__FILENO)
  return _fileno(file);
#else
  if (file == stdin)
    return STDIN_FILENO;
  else if (file == stdout)
    return STDOUT_FILENO;
  else if (file == stderr)
    return STDERR_FILENO;
  return -1;
#endif
}

static int set_binary_mode(FILE *file, const char * path)
{
  int fd = myfileno(file);

  if (fd != -1) {
    int res = -1;
#if defined(HAVE__SETMODE)
    res = _setmode(fd,_O_BINARY);
#elif defined(HAVE_SETMODE)
    res = setmode(fd,O_BINARY);
#endif
    if (res != -1) {
      message(D,"%s (%d) set to binary mode\n", path, fd);
      return 0;
    }
  }
  return -1;
}

#else

static int set_binary_mode(FILE *file, const char * path)
{
  ((void)file);
  ((void)path);
  return 0;
}

#endif

static void * myalloc(void * buf, int len, char * name)
{
  void * newbuf = 0;
  const unsigned int al = (1<<10) - 1;

  message(D, "Allocating %d bytes for the %s buffer\n", len, name);
  if (memmax && (unsigned int)len >= memmax) {
    error("cowardly refuse to allocate %u KiB of memory (try `-n')\n",
          (len+al) >> 10);
    free(buf);
  } else {
    newbuf = realloc(buf,len);
    if (!newbuf) {
      syserror(name);
      free(buf);
    }
  }
  return newbuf;
}


static int myread(void * buf, int len, FILE * inp, const char * name)
{
  // rom-properties FIXME: fread() returns size_t, so it can't possibly return a negative value.
  int n = (int)fread(buf, 1, len, inp);
  if (n == -1) {
    syserror(name);
    return -1;
  }
  return n;
}

static int exactread(void * buf, int len, FILE * inp, const char * name)
{
  int n = myread(buf, len, inp, name);
  if (n != -1 && n != len) {
    error("%s -- truncated at %d; expected %d\n", name, n, len);
    n = -1;
  }
  return n;
}

/* return codes */
enum {
  ERR_OK        = 0,                    /* no error */
  ERR_UNDEFINED,                        /* undefined  */
  ERR_CLI,                              /* command line parsing */
  ERR_INPUT,                           /* input file op */
  ERR_OUTPUT,                          /* output file op */

  ERR_UNPACK_NOT_ICE=10,      /* no ICE! header */
  ERR_UNPACK_TOO_SMALL,       /* too snall for being ICE! */
  ERR_UNPACK_SIZE_MISMATCH,   /* file size and header size mismatch */

  ERR_PACKER = 20,
  ERR_PACKER_SIZE_MISMATCH,
  ERR_PACKER_OVERFLOW,
  ERR_PACKER_STRESS,
};

int UNICE68_CDECL main(int argc, char *argv[])
{
  int err = ERR_UNDEFINED;
  int mode = 0, oneop = 0, sens = 0, oldid = 0;

  char * finp = 0, *fout = 0;
  char * ibuffer = 0, * obuffer = 0;
  FILE * inp = 0, * out = 0;
  int    ilen = -1, olen = -1;
  int   csize = 0, dsize = -1;
  int    iextra = 0;

  int i, n, hread;
  char header[12];

  unsigned hash1 = 0;
  int verified = 0;

  msgout = stdout;
  prg = basename(argv[0]);
  if (!prg)
    prg = PACKAGE_NAME;
  argv[0] = prg;
  for (i=n=1; i<argc; ++i) {
    int c;
    char * arg = argv[i];

    if (arg[0] != '-' || !(c = arg[1])) { /* not an option */
      argv[n++] = arg; continue;
    }
    arg += 2;
    if (c == '-') {
      if (!*arg) {              /* `--' breaks parsing */
        ++i; break;
      }
      /* Long options */
      if (!strcmp(arg,"help")) {
        c = 'h';
      } else if (!strcmp(arg,"version")) {
        c = 'V';
      } else if (!strcmp(arg,"verbose")) {
        c = 'v';
      } else if (!strcmp(arg,"quiet")) {
        c = 'q';
        /* } else if (!strcmp(arg,"cat")) { */
        /*   c = 'c'; */
      } else if (!strcmp(arg,"test")) {
        c = 't';
      } else if (!strcmp(arg,"deep-test")) {
        c = 'T';
      } else if (!strcmp(arg,"depack")) {
        c = 'd';
      } else if (!strcmp(arg,"pack")) {
        c = 'p';
      } else if (!strcmp(arg,"pack-old")) {
        c = 'P';
      } else if (!strcmp(arg,"stress")) {
        c = 's';
      } else if (!strcmp(arg,"no-limit")) {
        c = 'n';
      } else {
        error("invalid option `--%s'.\n", arg);
        return ERR_CLI;
      }
      arg += strlen(arg);
    }

    /* short options */
    do {
      switch (c) {
      case 'h': return print_usage();
      case 'V': return print_version();
      case 'v': ++verbose; break;
      case 'q': --verbose; break;
      case 'n': memmax = 0; break;
      case 'd': case 't': case 'T':
        /* case 'c': */
        sens = 'd';
        /* fall-through */
      case 'P':
        oldid = 1;
	/* fall-through */
      case 'p': case 's':
        if (!sens) sens = 'p';
        if (mode != 0) {
          error("only one mode can be specified.\n");
          return ERR_CLI;
        }
        oneop = !!strchr("tTs", c);
        mode = c;
        break;
      default:
        error("invalid option `-%c'.\n", c);
        return ERR_CLI;
      }
      c = *arg++;
    } while (c);

  }
  message(D,"Debug messages activated\n");
  while (i<argc) {
    argv[n++] = argv[i++];
  }
  argc = n;

  if (argc > 2 + !oneop) {
    error("too many argument -- `%s'\n", argv[2 + !oneop]);
    return ERR_CLI;
  }
  if (argc > 1) {
    finp = (argv[1][0] == '-' && !argv[1][1])
      ? 0 : argv[1];
  }
  if (argc > 2) {
    fout = (argv[2][0] == '-' && !argv[2][1])
      ? 0 : argv[2];
  }

  /* Divert all messages to stderr if neccessary */
  if (!oneop && !fout) {
    msgout = stderr;
    message(D,"All messages diverted to stderr\n");
  }

  /***********************************************************************
   * Input
   **********************************************************************/
  err = ERR_INPUT;
  ilen = -1;
  if (!finp) {
    set_binary_mode(inp=stdin, finp="<stdin>");
  } else {
    inp = fopen(finp,"rb");
    if (inp) {
      if (fseek(inp, 0, SEEK_END) != -1) {
        long pos = ftell(inp);
        if (fseek(inp, 0, SEEK_SET) != -1) {
          ilen = pos;
        }
      }
      if (ilen == -1) {
        syserror(finp);
        goto error;
      }
    }
  }
  message(D,"input: %s (%d)\n", finp, ilen);
  if (!inp) {
    syserror(finp);
    goto error;
  }
  err = (int) fread(header, 1, sizeof(header), inp);
  if (err == -1) {
    syserror(finp);
    goto error;
  }
  hread = err;

  if (hread < (int)sizeof(header)) {
    if (sens == 'd') {
      err = ERR_UNPACK_TOO_SMALL;
      error("input is too small, not ice packed.\n");
      goto error;
    }
    mode = sens = 'p';
    message(D,"Assume mode `%c'\n", mode);
  }

  if (sens != 'p') {
    csize = 0;
    dsize = unice68_depacked_size(header, &csize);
    if (dsize == -1) {
      message(D,"Not ice\n");
      if (sens == 'd') {
        err = ERR_UNPACK_NOT_ICE;
        error("input is not ice packed.\n");
        goto error;
      }
      assert(!sens);
      sens = mode = 'p';
      message(D,"Assume mode `%c'\n", mode);
    } else if (!sens) {
      sens = mode = 'd';
      message(D,"Assume mode `%c'\n", mode);
    }

    if (sens == 'd') {
      const int margin = 16;

      if (ilen == -1 || ilen == csize)
        ilen = csize;
      else if ( myabs(ilen-csize) > margin ) {
        err = ERR_UNPACK_SIZE_MISMATCH;
        error("file size (%d) and packed size (%d) do not match.\n",
              ilen, csize);
        goto error;
      } else if (csize > ilen) {
        iextra = csize - ilen;
      }

      if (mode == 't') {
        err = ( dsize == -1 ) ? ERR_UNPACK_NOT_ICE : ERR_OK;
        goto error;
      }
    }
  }

  err = ERR_INPUT;
  if (ilen != -1) {
    if (ibuffer = myalloc(0,ilen+iextra,"input"), !ibuffer)
      goto error;
    memcpy(ibuffer, header, hread);
    if (-1 == exactread(ibuffer+hread, ilen-hread, inp, finp))
      goto error;
  } else {
    int max = 1<<16, m, n;

    ibuffer = myalloc(0, max, "input");
    if (!ibuffer)
      goto error;
    /* copy pre-read header */
    memcpy(ibuffer, header, hread);
    ilen = hread;
    do {
      m = max - ilen;
      if (!m) {
        max <<= 1;
        ibuffer = myalloc(ibuffer, max, "input");
        if (!ibuffer)
          goto error;
        m = max - ilen;
      }
      n = myread(ibuffer+ilen, m, inp, finp);
      message(D,"got %d out of %d\n", n, m);

      if (n == -1)
        goto error;
      ilen += n;
    } while (n > 0 /* && n == m */);
  }
  message(D,"Have read all %d input bytes from `%s' ...\n", ilen, finp);

  err = ERR_OUTPUT;
  olen = (sens == 'd') ? dsize : (ilen + (ilen>>1) + 1000);
  if (obuffer = myalloc(0, olen, "output"), !obuffer)
    goto error;

  /***********************************************************************
   * Process
   **********************************************************************/
  switch (sens) {
  case 'p':
    message(V, "ice packing \"%s\" (%d bytes) ...\n", finp, ilen);
    err = unice68_packer(obuffer, olen, ibuffer, ilen);
    message(D, "packing returns with %d\n", err);
    if (err == -1) {
      err = ERR_PACKER;
      break;
    }
    if (err > olen) {
      error("CRITICAL ! ice packer buffer overflow (%d > %d)\n",
            err , olen);
      exit(ERR_PACKER_OVERFLOW);
    }
    csize = err;
    dsize = unice68_depacked_size(obuffer, &csize);
    if (dsize == -1 || dsize != ilen) {
      if (dsize != -1) dsize = ~dsize;
      error("size inconsistency (%d != %d)\n", dsize, ilen);
      err = ERR_PACKER_SIZE_MISMATCH;
      goto error;
    }
    olen = csize;

    /* Patch ICE magic to fit requested */
    memcpy(obuffer+1,oldid ? "ce" : "CE", 2);

    err = 0;
    if (mode != 's') {
      break;
    } else {
      int tlen;
      char * tbuffer;
      hash1 = hash_buffer(ibuffer,ilen);
      message(D,"input hash: %x\n", hash1);
      memset(ibuffer,0,ilen);
      tbuffer = ibuffer; tlen = ilen;
      ibuffer = obuffer; ilen = olen;
      obuffer = tbuffer; olen = tlen;
    }
    /* fall-through */
  case 'd':
    message(V, "ice depacking \"%s\" (%d bytes) ...\n", finp, ilen);
    err = unice68_depacker(obuffer, olen, ibuffer, ilen);
    message(D, "depacking returns with %d\n", err);
    if (!err && mode =='s') {
      unsigned int hash2 = hash_buffer(obuffer, olen);
      message(D,"depack hash: %x\n", hash2);
      err = - (hash1 != hash2);
      verified = !err;
    }
    if (err) {
      err = ERR_PACKER_STRESS;
      error("stress has failed\n");
    }
    break;

  default:
    assert(!"!!!! internal error !!!! invalid sens");
  }


/***********************************************************************
 * Output
 **********************************************************************/
  if (!oneop && !err) {
    err = ERR_OUTPUT;
    if (!fout) {
      set_binary_mode(out=stdout, fout="<stdout>");
    } else {
      out = fopen(fout,"wb");
    }

    message(D,"output: %s (%d)\n", fout, olen);
    if (!out) {
      syserror(fout);
      goto error;
    }
    // rom-properties FIXME: fread() returns size_t, so it can't possibly return a negative value.
    n = (int)fwrite(obuffer,1,olen,out);
    message(D,"Have written %d bytes to %s\n", n, fout);
    if (n != olen) {
      syserror(fout);
      goto error;
    }
    err = 0;
  }

error:
  if (inp && inp != stdin) {
    fclose(inp);
  }
  if (out) {
    fflush(out);
    if (out != stdout) {
      fclose(out);
    }
  }
  free(ibuffer);
  free(obuffer);

  if (!err) {
    message(N,"ICE! compressed:%d uncompressed:%d ratio:%d%%%s\n",
            csize, dsize, dsize?(csize+50)*100/dsize:-1,
            verified ? " (verified)" : "");
  }

  err &= 127;
  message(D,"exit with code %d\n", err);

  return err;
}
