/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * curl-mini.h: Subset of definitions from libcurl's headers.              *
 *                                                                         *
 * Copyright (C) Daniel Stenberg, <daniel@haxx.se>, et al.                 *
 * SPDX-License-Identifier: curl                                           *
 ***************************************************************************/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Definitions are current as of cURL v8.15.0 [2025/07/16]

/** curl/curl.h **/

/* Compile-time deprecation macros. */
#if (defined(__GNUC__) &&                                              \
  ((__GNUC__ > 12) || ((__GNUC__ == 12) && (__GNUC_MINOR__ >= 1))) ||  \
  (defined(__clang__) && __clang_major__ >= 3) ||                      \
  defined(__IAR_SYSTEMS_ICC__)) &&                                     \
  !defined(__INTEL_COMPILER) &&                                        \
  !defined(CURL_DISABLE_DEPRECATION) && !defined(BUILDING_LIBCURL)
#define CURL_DEPRECATED(version, message)                       \
  __attribute__((deprecated("since " # version ". " message)))
#ifdef __IAR_SYSTEMS_ICC__
#define CURL_IGNORE_DEPRECATION(statements) \
      _Pragma("diag_suppress=Pe1444") \
      statements \
      _Pragma("diag_default=Pe1444")
#else
#define CURL_IGNORE_DEPRECATION(statements) \
      _Pragma("GCC diagnostic push") \
      _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"") \
      statements \
      _Pragma("GCC diagnostic pop")
#endif
#else
#define CURL_DEPRECATED(version, message)
#define CURL_IGNORE_DEPRECATION(statements)     statements
#endif

typedef void CURL;
typedef void CURLSH;

#ifdef _WIN32
#  define CURL_EXTERN __declspec(dllimport)
#else /* !_WIN32 */
#  define CURL_EXTERN
#endif /* _WIN32 */

/* All possible error codes from all sorts of curl functions. Future versions
   may return other values, stay prepared.

   Always add new return codes last. Never *EVER* remove any. The return
   codes must remain the same!
 */

typedef enum {
  CURLE_OK = 0,
  CURLE_UNSUPPORTED_PROTOCOL,    /* 1 */
  CURLE_FAILED_INIT,             /* 2 */
  CURLE_URL_MALFORMAT,           /* 3 */
  CURLE_NOT_BUILT_IN,            /* 4 - [was obsoleted in August 2007 for
                                    7.17.0, reused in April 2011 for 7.21.5] */
  CURLE_COULDNT_RESOLVE_PROXY,   /* 5 */
  CURLE_COULDNT_RESOLVE_HOST,    /* 6 */
  CURLE_COULDNT_CONNECT,         /* 7 */
  CURLE_WEIRD_SERVER_REPLY,      /* 8 */
  CURLE_REMOTE_ACCESS_DENIED,    /* 9 a service was denied by the server
                                    due to lack of access - when login fails
                                    this is not returned. */
  CURLE_FTP_ACCEPT_FAILED,       /* 10 - [was obsoleted in April 2006 for
                                    7.15.4, reused in Dec 2011 for 7.24.0]*/
  CURLE_FTP_WEIRD_PASS_REPLY,    /* 11 */
  CURLE_FTP_ACCEPT_TIMEOUT,      /* 12 - timeout occurred accepting server
                                    [was obsoleted in August 2007 for 7.17.0,
                                    reused in Dec 2011 for 7.24.0]*/
  CURLE_FTP_WEIRD_PASV_REPLY,    /* 13 */
  CURLE_FTP_WEIRD_227_FORMAT,    /* 14 */
  CURLE_FTP_CANT_GET_HOST,       /* 15 */
  CURLE_HTTP2,                   /* 16 - A problem in the http2 framing layer.
                                    [was obsoleted in August 2007 for 7.17.0,
                                    reused in July 2014 for 7.38.0] */
  CURLE_FTP_COULDNT_SET_TYPE,    /* 17 */
  CURLE_PARTIAL_FILE,            /* 18 */
  CURLE_FTP_COULDNT_RETR_FILE,   /* 19 */
  CURLE_OBSOLETE20,              /* 20 - NOT USED */
  CURLE_QUOTE_ERROR,             /* 21 - quote command failure */
  CURLE_HTTP_RETURNED_ERROR,     /* 22 */
  CURLE_WRITE_ERROR,             /* 23 */
  CURLE_OBSOLETE24,              /* 24 - NOT USED */
  CURLE_UPLOAD_FAILED,           /* 25 - failed upload "command" */
  CURLE_READ_ERROR,              /* 26 - could not open/read from file */
  CURLE_OUT_OF_MEMORY,           /* 27 */
  CURLE_OPERATION_TIMEDOUT,      /* 28 - the timeout time was reached */
  CURLE_OBSOLETE29,              /* 29 - NOT USED */
  CURLE_FTP_PORT_FAILED,         /* 30 - FTP PORT operation failed */
  CURLE_FTP_COULDNT_USE_REST,    /* 31 - the REST command failed */
  CURLE_OBSOLETE32,              /* 32 - NOT USED */
  CURLE_RANGE_ERROR,             /* 33 - RANGE "command" did not work */
  CURLE_OBSOLETE34,              /* 34 */
  CURLE_SSL_CONNECT_ERROR,       /* 35 - wrong when connecting with SSL */
  CURLE_BAD_DOWNLOAD_RESUME,     /* 36 - could not resume download */
  CURLE_FILE_COULDNT_READ_FILE,  /* 37 */
  CURLE_LDAP_CANNOT_BIND,        /* 38 */
  CURLE_LDAP_SEARCH_FAILED,      /* 39 */
  CURLE_OBSOLETE40,              /* 40 - NOT USED */
  CURLE_OBSOLETE41,              /* 41 - NOT USED starting with 7.53.0 */
  CURLE_ABORTED_BY_CALLBACK,     /* 42 */
  CURLE_BAD_FUNCTION_ARGUMENT,   /* 43 */
  CURLE_OBSOLETE44,              /* 44 - NOT USED */
  CURLE_INTERFACE_FAILED,        /* 45 - CURLOPT_INTERFACE failed */
  CURLE_OBSOLETE46,              /* 46 - NOT USED */
  CURLE_TOO_MANY_REDIRECTS,      /* 47 - catch endless re-direct loops */
  CURLE_UNKNOWN_OPTION,          /* 48 - User specified an unknown option */
  CURLE_SETOPT_OPTION_SYNTAX,    /* 49 - Malformed setopt option */
  CURLE_OBSOLETE50,              /* 50 - NOT USED */
  CURLE_OBSOLETE51,              /* 51 - NOT USED */
  CURLE_GOT_NOTHING,             /* 52 - when this is a specific error */
  CURLE_SSL_ENGINE_NOTFOUND,     /* 53 - SSL crypto engine not found */
  CURLE_SSL_ENGINE_SETFAILED,    /* 54 - can not set SSL crypto engine as
                                    default */
  CURLE_SEND_ERROR,              /* 55 - failed sending network data */
  CURLE_RECV_ERROR,              /* 56 - failure in receiving network data */
  CURLE_OBSOLETE57,              /* 57 - NOT IN USE */
  CURLE_SSL_CERTPROBLEM,         /* 58 - problem with the local certificate */
  CURLE_SSL_CIPHER,              /* 59 - could not use specified cipher */
  CURLE_PEER_FAILED_VERIFICATION, /* 60 - peer's certificate or fingerprint
                                     was not verified fine */
  CURLE_BAD_CONTENT_ENCODING,    /* 61 - Unrecognized/bad encoding */
  CURLE_OBSOLETE62,              /* 62 - NOT IN USE since 7.82.0 */
  CURLE_FILESIZE_EXCEEDED,       /* 63 - Maximum file size exceeded */
  CURLE_USE_SSL_FAILED,          /* 64 - Requested FTP SSL level failed */
  CURLE_SEND_FAIL_REWIND,        /* 65 - Sending the data requires a rewind
                                    that failed */
  CURLE_SSL_ENGINE_INITFAILED,   /* 66 - failed to initialise ENGINE */
  CURLE_LOGIN_DENIED,            /* 67 - user, password or similar was not
                                    accepted and we failed to login */
  CURLE_TFTP_NOTFOUND,           /* 68 - file not found on server */
  CURLE_TFTP_PERM,               /* 69 - permission problem on server */
  CURLE_REMOTE_DISK_FULL,        /* 70 - out of disk space on server */
  CURLE_TFTP_ILLEGAL,            /* 71 - Illegal TFTP operation */
  CURLE_TFTP_UNKNOWNID,          /* 72 - Unknown transfer ID */
  CURLE_REMOTE_FILE_EXISTS,      /* 73 - File already exists */
  CURLE_TFTP_NOSUCHUSER,         /* 74 - No such user */
  CURLE_OBSOLETE75,              /* 75 - NOT IN USE since 7.82.0 */
  CURLE_OBSOLETE76,              /* 76 - NOT IN USE since 7.82.0 */
  CURLE_SSL_CACERT_BADFILE,      /* 77 - could not load CACERT file, missing
                                    or wrong format */
  CURLE_REMOTE_FILE_NOT_FOUND,   /* 78 - remote file not found */
  CURLE_SSH,                     /* 79 - error from the SSH layer, somewhat
                                    generic so the error message will be of
                                    interest when this has happened */

  CURLE_SSL_SHUTDOWN_FAILED,     /* 80 - Failed to shut down the SSL
                                    connection */
  CURLE_AGAIN,                   /* 81 - socket is not ready for send/recv,
                                    wait till it is ready and try again (Added
                                    in 7.18.2) */
  CURLE_SSL_CRL_BADFILE,         /* 82 - could not load CRL file, missing or
                                    wrong format (Added in 7.19.0) */
  CURLE_SSL_ISSUER_ERROR,        /* 83 - Issuer check failed.  (Added in
                                    7.19.0) */
  CURLE_FTP_PRET_FAILED,         /* 84 - a PRET command failed */
  CURLE_RTSP_CSEQ_ERROR,         /* 85 - mismatch of RTSP CSeq numbers */
  CURLE_RTSP_SESSION_ERROR,      /* 86 - mismatch of RTSP Session Ids */
  CURLE_FTP_BAD_FILE_LIST,       /* 87 - unable to parse FTP file list */
  CURLE_CHUNK_FAILED,            /* 88 - chunk callback reported error */
  CURLE_NO_CONNECTION_AVAILABLE, /* 89 - No connection available, the
                                    session will be queued */
  CURLE_SSL_PINNEDPUBKEYNOTMATCH, /* 90 - specified pinned public key did not
                                     match */
  CURLE_SSL_INVALIDCERTSTATUS,   /* 91 - invalid certificate status */
  CURLE_HTTP2_STREAM,            /* 92 - stream error in HTTP/2 framing layer
                                    */
  CURLE_RECURSIVE_API_CALL,      /* 93 - an api function was called from
                                    inside a callback */
  CURLE_AUTH_ERROR,              /* 94 - an authentication function returned an
                                    error */
  CURLE_HTTP3,                   /* 95 - An HTTP/3 layer problem */
  CURLE_QUIC_CONNECT_ERROR,      /* 96 - QUIC connection error */
  CURLE_PROXY,                   /* 97 - proxy handshake error */
  CURLE_SSL_CLIENTCERT,          /* 98 - client-side certificate required */
  CURLE_UNRECOVERABLE_POLL,      /* 99 - poll/select returned fatal error */
  CURLE_TOO_LARGE,               /* 100 - a value/data met its maximum */
  CURLE_ECH_REQUIRED,            /* 101 - ECH tried but failed */
  CURL_LAST /* never use! */
} CURLcode;

/* long may be 32 or 64 bits, but we should never depend on anything else
   but 32 */
#define CURLOPTTYPE_LONG          0
#define CURLOPTTYPE_OBJECTPOINT   10000
#define CURLOPTTYPE_FUNCTIONPOINT 20000
#define CURLOPTTYPE_OFF_T         30000
#define CURLOPTTYPE_BLOB          40000

/* *STRINGPOINT is an alias for OBJECTPOINT to allow tools to extract the
   string options from the header file */


#define CURLOPT(na,t,nu) na = t + nu
#define CURLOPTDEPRECATED(na,t,nu,v,m) na CURL_DEPRECATED(v,m) = t + nu

/* CURLOPT aliases that make no runtime difference */

/* 'char *' argument to a string with a trailing zero */
#define CURLOPTTYPE_STRINGPOINT CURLOPTTYPE_OBJECTPOINT

/* 'struct curl_slist *' argument */
#define CURLOPTTYPE_SLISTPOINT  CURLOPTTYPE_OBJECTPOINT

/* 'void *' argument passed untouched to callback */
#define CURLOPTTYPE_CBPOINT     CURLOPTTYPE_OBJECTPOINT

/* 'long' argument with a set of values/bitmask */
#define CURLOPTTYPE_VALUES      CURLOPTTYPE_LONG

/*
 * All CURLOPT_* values.
 */

typedef enum {
  /* This is the FILE * or void * the regular output should be written to. */
  CURLOPT(CURLOPT_WRITEDATA, CURLOPTTYPE_CBPOINT, 1),

  /* The full URL to get/put */
  CURLOPT(CURLOPT_URL, CURLOPTTYPE_STRINGPOINT, 2),

  /* Port number to connect to, if other than default. */
  CURLOPT(CURLOPT_PORT, CURLOPTTYPE_LONG, 3),

  /* Name of proxy to use. */
  CURLOPT(CURLOPT_PROXY, CURLOPTTYPE_STRINGPOINT, 4),

  /* "user:password;options" to use when fetching. */
  CURLOPT(CURLOPT_USERPWD, CURLOPTTYPE_STRINGPOINT, 5),

  /* "user:password" to use with proxy. */
  CURLOPT(CURLOPT_PROXYUSERPWD, CURLOPTTYPE_STRINGPOINT, 6),

  /* Range to get, specified as an ASCII string. */
  CURLOPT(CURLOPT_RANGE, CURLOPTTYPE_STRINGPOINT, 7),

  /* not used */

  /* Specified file stream to upload from (use as input): */
  CURLOPT(CURLOPT_READDATA, CURLOPTTYPE_CBPOINT, 9),

  /* Buffer to receive error messages in, must be at least CURL_ERROR_SIZE
   * bytes big. */
  CURLOPT(CURLOPT_ERRORBUFFER, CURLOPTTYPE_OBJECTPOINT, 10),

  /* Function that will be called to store the output (instead of fwrite). The
   * parameters will use fwrite() syntax, make sure to follow them. */
  CURLOPT(CURLOPT_WRITEFUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 11),

  /* Function that will be called to read the input (instead of fread). The
   * parameters will use fread() syntax, make sure to follow them. */
  CURLOPT(CURLOPT_READFUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 12),

  /* Time-out the read operation after this amount of seconds */
  CURLOPT(CURLOPT_TIMEOUT, CURLOPTTYPE_LONG, 13),

  /* If CURLOPT_READDATA is used, this can be used to inform libcurl about
   * how large the file being sent really is. That allows better error
   * checking and better verifies that the upload was successful. -1 means
   * unknown size.
   *
   * For large file support, there is also a _LARGE version of the key
   * which takes an off_t type, allowing platforms with larger off_t
   * sizes to handle larger files. See below for INFILESIZE_LARGE.
   */
  CURLOPT(CURLOPT_INFILESIZE, CURLOPTTYPE_LONG, 14),

  /* POST static input fields. */
  CURLOPT(CURLOPT_POSTFIELDS, CURLOPTTYPE_OBJECTPOINT, 15),

  /* Set the referrer page (needed by some CGIs) */
  CURLOPT(CURLOPT_REFERER, CURLOPTTYPE_STRINGPOINT, 16),

  /* Set the FTP PORT string (interface name, named or numerical IP address)
     Use i.e '-' to use default address. */
  CURLOPT(CURLOPT_FTPPORT, CURLOPTTYPE_STRINGPOINT, 17),

  /* Set the User-Agent string (examined by some CGIs) */
  CURLOPT(CURLOPT_USERAGENT, CURLOPTTYPE_STRINGPOINT, 18),

  /* If the download receives less than "low speed limit" bytes/second
   * during "low speed time" seconds, the operations is aborted.
   * You could i.e if you have a pretty high speed connection, abort if
   * it is less than 2000 bytes/sec during 20 seconds.
   */

  /* Set the "low speed limit" */
  CURLOPT(CURLOPT_LOW_SPEED_LIMIT, CURLOPTTYPE_LONG, 19),

  /* Set the "low speed time" */
  CURLOPT(CURLOPT_LOW_SPEED_TIME, CURLOPTTYPE_LONG, 20),

  /* Set the continuation offset.
   *
   * Note there is also a _LARGE version of this key which uses
   * off_t types, allowing for large file offsets on platforms which
   * use larger-than-32-bit off_t's. Look below for RESUME_FROM_LARGE.
   */
  CURLOPT(CURLOPT_RESUME_FROM, CURLOPTTYPE_LONG, 21),

  /* Set cookie in request: */
  CURLOPT(CURLOPT_COOKIE, CURLOPTTYPE_STRINGPOINT, 22),

  /* This points to a linked list of headers, struct curl_slist kind. This
     list is also used for RTSP (in spite of its name) */
  CURLOPT(CURLOPT_HTTPHEADER, CURLOPTTYPE_SLISTPOINT, 23),

  /* This points to a linked list of post entries, struct curl_httppost */
  CURLOPTDEPRECATED(CURLOPT_HTTPPOST, CURLOPTTYPE_OBJECTPOINT, 24,
                    7.56.0, "Use CURLOPT_MIMEPOST"),

  /* name of the file keeping your private SSL-certificate */
  CURLOPT(CURLOPT_SSLCERT, CURLOPTTYPE_STRINGPOINT, 25),

  /* password for the SSL or SSH private key */
  CURLOPT(CURLOPT_KEYPASSWD, CURLOPTTYPE_STRINGPOINT, 26),

  /* send TYPE parameter? */
  CURLOPT(CURLOPT_CRLF, CURLOPTTYPE_LONG, 27),

  /* send linked-list of QUOTE commands */
  CURLOPT(CURLOPT_QUOTE, CURLOPTTYPE_SLISTPOINT, 28),

  /* send FILE * or void * to store headers to, if you use a callback it
     is simply passed to the callback unmodified */
  CURLOPT(CURLOPT_HEADERDATA, CURLOPTTYPE_CBPOINT, 29),

  /* point to a file to read the initial cookies from, also enables
     "cookie awareness" */
  CURLOPT(CURLOPT_COOKIEFILE, CURLOPTTYPE_STRINGPOINT, 31),

  /* What version to specifically try to use.
     See CURL_SSLVERSION defines below. */
  CURLOPT(CURLOPT_SSLVERSION, CURLOPTTYPE_VALUES, 32),

  /* What kind of HTTP time condition to use, see defines */
  CURLOPT(CURLOPT_TIMECONDITION, CURLOPTTYPE_VALUES, 33),

  /* Time to use with the above condition. Specified in number of seconds
     since 1 Jan 1970 */
  CURLOPT(CURLOPT_TIMEVALUE, CURLOPTTYPE_LONG, 34),

  /* 35 = OBSOLETE */

  /* Custom request, for customizing the get command like
     HTTP: DELETE, TRACE and others
     FTP: to use a different list command
     */
  CURLOPT(CURLOPT_CUSTOMREQUEST, CURLOPTTYPE_STRINGPOINT, 36),

  /* FILE handle to use instead of stderr */
  CURLOPT(CURLOPT_STDERR, CURLOPTTYPE_OBJECTPOINT, 37),

  /* 38 is not used */

  /* send linked-list of post-transfer QUOTE commands */
  CURLOPT(CURLOPT_POSTQUOTE, CURLOPTTYPE_SLISTPOINT, 39),

  /* 40 is not used */

  /* talk a lot */
  CURLOPT(CURLOPT_VERBOSE, CURLOPTTYPE_LONG, 41),

  /* throw the header out too */
  CURLOPT(CURLOPT_HEADER, CURLOPTTYPE_LONG, 42),

  /* shut off the progress meter */
  CURLOPT(CURLOPT_NOPROGRESS, CURLOPTTYPE_LONG, 43),

  /* use HEAD to get http document */
  CURLOPT(CURLOPT_NOBODY, CURLOPTTYPE_LONG, 44),

  /* no output on http error codes >= 400 */
  CURLOPT(CURLOPT_FAILONERROR, CURLOPTTYPE_LONG, 45),

  /* this is an upload */
  CURLOPT(CURLOPT_UPLOAD, CURLOPTTYPE_LONG, 46),

  /* HTTP POST method */
  CURLOPT(CURLOPT_POST, CURLOPTTYPE_LONG, 47),

  /* bare names when listing directories */
  CURLOPT(CURLOPT_DIRLISTONLY, CURLOPTTYPE_LONG, 48),

  /* Append instead of overwrite on upload! */
  CURLOPT(CURLOPT_APPEND, CURLOPTTYPE_LONG, 50),

  /* Specify whether to read the user+password from the .netrc or the URL.
   * This must be one of the CURL_NETRC_* enums below. */
  CURLOPT(CURLOPT_NETRC, CURLOPTTYPE_VALUES, 51),

  /* use Location: Luke! */
  CURLOPT(CURLOPT_FOLLOWLOCATION, CURLOPTTYPE_LONG, 52),

   /* transfer data in text/ASCII format */
  CURLOPT(CURLOPT_TRANSFERTEXT, CURLOPTTYPE_LONG, 53),

  /* HTTP PUT */
  CURLOPTDEPRECATED(CURLOPT_PUT, CURLOPTTYPE_LONG, 54,
                    7.12.1, "Use CURLOPT_UPLOAD"),

  /* 55 = OBSOLETE */

  /* DEPRECATED
   * Function that will be called instead of the internal progress display
   * function. This function should be defined as the curl_progress_callback
   * prototype defines. */
  CURLOPTDEPRECATED(CURLOPT_PROGRESSFUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 56,
                    7.32.0, "Use CURLOPT_XFERINFOFUNCTION"),

  /* Data passed to the CURLOPT_PROGRESSFUNCTION and CURLOPT_XFERINFOFUNCTION
     callbacks */
  CURLOPT(CURLOPT_XFERINFODATA, CURLOPTTYPE_CBPOINT, 57),
#define CURLOPT_PROGRESSDATA CURLOPT_XFERINFODATA

  /* We want the referrer field set automatically when following locations */
  CURLOPT(CURLOPT_AUTOREFERER, CURLOPTTYPE_LONG, 58),

  /* Port of the proxy, can be set in the proxy string as well with:
     "[host]:[port]" */
  CURLOPT(CURLOPT_PROXYPORT, CURLOPTTYPE_LONG, 59),

  /* size of the POST input data, if strlen() is not good to use */
  CURLOPT(CURLOPT_POSTFIELDSIZE, CURLOPTTYPE_LONG, 60),

  /* tunnel non-http operations through an HTTP proxy */
  CURLOPT(CURLOPT_HTTPPROXYTUNNEL, CURLOPTTYPE_LONG, 61),

  /* Set the interface string to use as outgoing network interface */
  CURLOPT(CURLOPT_INTERFACE, CURLOPTTYPE_STRINGPOINT, 62),

  /* Set the krb4/5 security level, this also enables krb4/5 awareness. This
   * is a string, 'clear', 'safe', 'confidential' or 'private'. If the string
   * is set but does not match one of these, 'private' will be used.  */
  CURLOPT(CURLOPT_KRBLEVEL, CURLOPTTYPE_STRINGPOINT, 63),

  /* Set if we should verify the peer in ssl handshake, set 1 to verify. */
  CURLOPT(CURLOPT_SSL_VERIFYPEER, CURLOPTTYPE_LONG, 64),

  /* The CApath or CAfile used to validate the peer certificate
     this option is used only if SSL_VERIFYPEER is true */
  CURLOPT(CURLOPT_CAINFO, CURLOPTTYPE_STRINGPOINT, 65),

  /* 66 = OBSOLETE */
  /* 67 = OBSOLETE */

  /* Maximum number of http redirects to follow */
  CURLOPT(CURLOPT_MAXREDIRS, CURLOPTTYPE_LONG, 68),

  /* Pass a long set to 1 to get the date of the requested document (if
     possible)! Pass a zero to shut it off. */
  CURLOPT(CURLOPT_FILETIME, CURLOPTTYPE_LONG, 69),

  /* This points to a linked list of telnet options */
  CURLOPT(CURLOPT_TELNETOPTIONS, CURLOPTTYPE_SLISTPOINT, 70),

  /* Max amount of cached alive connections */
  CURLOPT(CURLOPT_MAXCONNECTS, CURLOPTTYPE_LONG, 71),

  /* 72 = OBSOLETE */
  /* 73 = OBSOLETE */

  /* Set to explicitly use a new connection for the upcoming transfer.
     Do not use this unless you are absolutely sure of this, as it makes the
     operation slower and is less friendly for the network. */
  CURLOPT(CURLOPT_FRESH_CONNECT, CURLOPTTYPE_LONG, 74),

  /* Set to explicitly forbid the upcoming transfer's connection to be reused
     when done. Do not use this unless you are absolutely sure of this, as it
     makes the operation slower and is less friendly for the network. */
  CURLOPT(CURLOPT_FORBID_REUSE, CURLOPTTYPE_LONG, 75),

  /* Set to a filename that contains random data for libcurl to use to
     seed the random engine when doing SSL connects. */
  CURLOPTDEPRECATED(CURLOPT_RANDOM_FILE, CURLOPTTYPE_STRINGPOINT, 76,
                    7.84.0, "Serves no purpose anymore"),

  /* Set to the Entropy Gathering Daemon socket pathname */
  CURLOPTDEPRECATED(CURLOPT_EGDSOCKET, CURLOPTTYPE_STRINGPOINT, 77,
                    7.84.0, "Serves no purpose anymore"),

  /* Time-out connect operations after this amount of seconds, if connects are
     OK within this time, then fine... This only aborts the connect phase. */
  CURLOPT(CURLOPT_CONNECTTIMEOUT, CURLOPTTYPE_LONG, 78),

  /* Function that will be called to store headers (instead of fwrite). The
   * parameters will use fwrite() syntax, make sure to follow them. */
  CURLOPT(CURLOPT_HEADERFUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 79),

  /* Set this to force the HTTP request to get back to GET. Only really usable
     if POST, PUT or a custom request have been used first.
   */
  CURLOPT(CURLOPT_HTTPGET, CURLOPTTYPE_LONG, 80),

  /* Set if we should verify the Common name from the peer certificate in ssl
   * handshake, set 1 to check existence, 2 to ensure that it matches the
   * provided hostname. */
  CURLOPT(CURLOPT_SSL_VERIFYHOST, CURLOPTTYPE_LONG, 81),

  /* Specify which filename to write all known cookies in after completed
     operation. Set filename to "-" (dash) to make it go to stdout. */
  CURLOPT(CURLOPT_COOKIEJAR, CURLOPTTYPE_STRINGPOINT, 82),

  /* Specify which TLS 1.2 (1.1, 1.0) ciphers to use */
  CURLOPT(CURLOPT_SSL_CIPHER_LIST, CURLOPTTYPE_STRINGPOINT, 83),

  /* Specify which HTTP version to use! This must be set to one of the
     CURL_HTTP_VERSION* enums set below. */
  CURLOPT(CURLOPT_HTTP_VERSION, CURLOPTTYPE_VALUES, 84),

  /* Specifically switch on or off the FTP engine's use of the EPSV command. By
     default, that one will always be attempted before the more traditional
     PASV command. */
  CURLOPT(CURLOPT_FTP_USE_EPSV, CURLOPTTYPE_LONG, 85),

  /* type of the file keeping your SSL-certificate ("DER", "PEM", "ENG") */
  CURLOPT(CURLOPT_SSLCERTTYPE, CURLOPTTYPE_STRINGPOINT, 86),

  /* name of the file keeping your private SSL-key */
  CURLOPT(CURLOPT_SSLKEY, CURLOPTTYPE_STRINGPOINT, 87),

  /* type of the file keeping your private SSL-key ("DER", "PEM", "ENG") */
  CURLOPT(CURLOPT_SSLKEYTYPE, CURLOPTTYPE_STRINGPOINT, 88),

  /* crypto engine for the SSL-sub system */
  CURLOPT(CURLOPT_SSLENGINE, CURLOPTTYPE_STRINGPOINT, 89),

  /* set the crypto engine for the SSL-sub system as default
     the param has no meaning...
   */
  CURLOPT(CURLOPT_SSLENGINE_DEFAULT, CURLOPTTYPE_LONG, 90),

  /* Non-zero value means to use the global dns cache */
  /* DEPRECATED, do not use! */
  CURLOPTDEPRECATED(CURLOPT_DNS_USE_GLOBAL_CACHE, CURLOPTTYPE_LONG, 91,
                    7.11.1, "Use CURLOPT_SHARE"),

  /* DNS cache timeout */
  CURLOPT(CURLOPT_DNS_CACHE_TIMEOUT, CURLOPTTYPE_LONG, 92),

  /* send linked-list of pre-transfer QUOTE commands */
  CURLOPT(CURLOPT_PREQUOTE, CURLOPTTYPE_SLISTPOINT, 93),

  /* set the debug function */
  CURLOPT(CURLOPT_DEBUGFUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 94),

  /* set the data for the debug function */
  CURLOPT(CURLOPT_DEBUGDATA, CURLOPTTYPE_CBPOINT, 95),

  /* mark this as start of a cookie session */
  CURLOPT(CURLOPT_COOKIESESSION, CURLOPTTYPE_LONG, 96),

  /* The CApath directory used to validate the peer certificate
     this option is used only if SSL_VERIFYPEER is true */
  CURLOPT(CURLOPT_CAPATH, CURLOPTTYPE_STRINGPOINT, 97),

  /* Instruct libcurl to use a smaller receive buffer */
  CURLOPT(CURLOPT_BUFFERSIZE, CURLOPTTYPE_LONG, 98),

  /* Instruct libcurl to not use any signal/alarm handlers, even when using
     timeouts. This option is useful for multi-threaded applications.
     See libcurl-the-guide for more background information. */
  CURLOPT(CURLOPT_NOSIGNAL, CURLOPTTYPE_LONG, 99),

  /* Provide a CURLShare for mutexing non-ts data */
  CURLOPT(CURLOPT_SHARE, CURLOPTTYPE_OBJECTPOINT, 100),

  /* indicates type of proxy. accepted values are CURLPROXY_HTTP (default),
     CURLPROXY_HTTPS, CURLPROXY_SOCKS4, CURLPROXY_SOCKS4A and
     CURLPROXY_SOCKS5. */
  CURLOPT(CURLOPT_PROXYTYPE, CURLOPTTYPE_VALUES, 101),

  /* Set the Accept-Encoding string. Use this to tell a server you would like
     the response to be compressed. Before 7.21.6, this was known as
     CURLOPT_ENCODING */
  CURLOPT(CURLOPT_ACCEPT_ENCODING, CURLOPTTYPE_STRINGPOINT, 102),

  /* Set pointer to private data */
  CURLOPT(CURLOPT_PRIVATE, CURLOPTTYPE_OBJECTPOINT, 103),

  /* Set aliases for HTTP 200 in the HTTP Response header */
  CURLOPT(CURLOPT_HTTP200ALIASES, CURLOPTTYPE_SLISTPOINT, 104),

  /* Continue to send authentication (user+password) when following locations,
     even when hostname changed. This can potentially send off the name
     and password to whatever host the server decides. */
  CURLOPT(CURLOPT_UNRESTRICTED_AUTH, CURLOPTTYPE_LONG, 105),

  /* Specifically switch on or off the FTP engine's use of the EPRT command (
     it also disables the LPRT attempt). By default, those ones will always be
     attempted before the good old traditional PORT command. */
  CURLOPT(CURLOPT_FTP_USE_EPRT, CURLOPTTYPE_LONG, 106),

  /* Set this to a bitmask value to enable the particular authentications
     methods you like. Use this in combination with CURLOPT_USERPWD.
     Note that setting multiple bits may cause extra network round-trips. */
  CURLOPT(CURLOPT_HTTPAUTH, CURLOPTTYPE_VALUES, 107),

  /* Set the ssl context callback function, currently only for OpenSSL or
     wolfSSL ssl_ctx, or mbedTLS mbedtls_ssl_config in the second argument.
     The function must match the curl_ssl_ctx_callback prototype. */
  CURLOPT(CURLOPT_SSL_CTX_FUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 108),

  /* Set the userdata for the ssl context callback function's third
     argument */
  CURLOPT(CURLOPT_SSL_CTX_DATA, CURLOPTTYPE_CBPOINT, 109),

  /* FTP Option that causes missing dirs to be created on the remote server.
     In 7.19.4 we introduced the convenience enums for this option using the
     CURLFTP_CREATE_DIR prefix.
  */
  CURLOPT(CURLOPT_FTP_CREATE_MISSING_DIRS, CURLOPTTYPE_LONG, 110),

  /* Set this to a bitmask value to enable the particular authentications
     methods you like. Use this in combination with CURLOPT_PROXYUSERPWD.
     Note that setting multiple bits may cause extra network round-trips. */
  CURLOPT(CURLOPT_PROXYAUTH, CURLOPTTYPE_VALUES, 111),

  /* Option that changes the timeout, in seconds, associated with getting a
     response. This is different from transfer timeout time and essentially
     places a demand on the server to acknowledge commands in a timely
     manner. For FTP, SMTP, IMAP and POP3. */
  CURLOPT(CURLOPT_SERVER_RESPONSE_TIMEOUT, CURLOPTTYPE_LONG, 112),

  /* Set this option to one of the CURL_IPRESOLVE_* defines (see below) to
     tell libcurl to use those IP versions only. This only has effect on
     systems with support for more than one, i.e IPv4 _and_ IPv6. */
  CURLOPT(CURLOPT_IPRESOLVE, CURLOPTTYPE_VALUES, 113),

  /* Set this option to limit the size of a file that will be downloaded from
     an HTTP or FTP server.

     Note there is also _LARGE version which adds large file support for
     platforms which have larger off_t sizes. See MAXFILESIZE_LARGE below. */
  CURLOPT(CURLOPT_MAXFILESIZE, CURLOPTTYPE_LONG, 114),

  /* See the comment for INFILESIZE above, but in short, specifies
   * the size of the file being uploaded.  -1 means unknown.
   */
  CURLOPT(CURLOPT_INFILESIZE_LARGE, CURLOPTTYPE_OFF_T, 115),

  /* Sets the continuation offset. There is also a CURLOPTTYPE_LONG version
   * of this; look above for RESUME_FROM.
   */
  CURLOPT(CURLOPT_RESUME_FROM_LARGE, CURLOPTTYPE_OFF_T, 116),

  /* Sets the maximum size of data that will be downloaded from
   * an HTTP or FTP server. See MAXFILESIZE above for the LONG version.
   */
  CURLOPT(CURLOPT_MAXFILESIZE_LARGE, CURLOPTTYPE_OFF_T, 117),

  /* Set this option to the filename of your .netrc file you want libcurl
     to parse (using the CURLOPT_NETRC option). If not set, libcurl will do
     a poor attempt to find the user's home directory and check for a .netrc
     file in there. */
  CURLOPT(CURLOPT_NETRC_FILE, CURLOPTTYPE_STRINGPOINT, 118),

  /* Enable SSL/TLS for FTP, pick one of:
     CURLUSESSL_TRY     - try using SSL, proceed anyway otherwise
     CURLUSESSL_CONTROL - SSL for the control connection or fail
     CURLUSESSL_ALL     - SSL for all communication or fail
  */
  CURLOPT(CURLOPT_USE_SSL, CURLOPTTYPE_VALUES, 119),

  /* The _LARGE version of the standard POSTFIELDSIZE option */
  CURLOPT(CURLOPT_POSTFIELDSIZE_LARGE, CURLOPTTYPE_OFF_T, 120),

  /* Enable/disable the TCP Nagle algorithm */
  CURLOPT(CURLOPT_TCP_NODELAY, CURLOPTTYPE_LONG, 121),

  /* 122 OBSOLETE, used in 7.12.3. Gone in 7.13.0 */
  /* 123 OBSOLETE. Gone in 7.16.0 */
  /* 124 OBSOLETE, used in 7.12.3. Gone in 7.13.0 */
  /* 125 OBSOLETE, used in 7.12.3. Gone in 7.13.0 */
  /* 126 OBSOLETE, used in 7.12.3. Gone in 7.13.0 */
  /* 127 OBSOLETE. Gone in 7.16.0 */
  /* 128 OBSOLETE. Gone in 7.16.0 */

  /* When FTP over SSL/TLS is selected (with CURLOPT_USE_SSL), this option
     can be used to change libcurl's default action which is to first try
     "AUTH SSL" and then "AUTH TLS" in this order, and proceed when a OK
     response has been received.

     Available parameters are:
     CURLFTPAUTH_DEFAULT - let libcurl decide
     CURLFTPAUTH_SSL     - try "AUTH SSL" first, then TLS
     CURLFTPAUTH_TLS     - try "AUTH TLS" first, then SSL
  */
  CURLOPT(CURLOPT_FTPSSLAUTH, CURLOPTTYPE_VALUES, 129),

  CURLOPTDEPRECATED(CURLOPT_IOCTLFUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 130,
                    7.18.0, "Use CURLOPT_SEEKFUNCTION"),
  CURLOPTDEPRECATED(CURLOPT_IOCTLDATA, CURLOPTTYPE_CBPOINT, 131,
                    7.18.0, "Use CURLOPT_SEEKDATA"),

  /* 132 OBSOLETE. Gone in 7.16.0 */
  /* 133 OBSOLETE. Gone in 7.16.0 */

  /* null-terminated string for pass on to the FTP server when asked for
     "account" info */
  CURLOPT(CURLOPT_FTP_ACCOUNT, CURLOPTTYPE_STRINGPOINT, 134),

  /* feed cookie into cookie engine */
  CURLOPT(CURLOPT_COOKIELIST, CURLOPTTYPE_STRINGPOINT, 135),

  /* ignore Content-Length */
  CURLOPT(CURLOPT_IGNORE_CONTENT_LENGTH, CURLOPTTYPE_LONG, 136),

  /* Set to non-zero to skip the IP address received in a 227 PASV FTP server
     response. Typically used for FTP-SSL purposes but is not restricted to
     that. libcurl will then instead use the same IP address it used for the
     control connection. */
  CURLOPT(CURLOPT_FTP_SKIP_PASV_IP, CURLOPTTYPE_LONG, 137),

  /* Select "file method" to use when doing FTP, see the curl_ftpmethod
     above. */
  CURLOPT(CURLOPT_FTP_FILEMETHOD, CURLOPTTYPE_VALUES, 138),

  /* Local port number to bind the socket to */
  CURLOPT(CURLOPT_LOCALPORT, CURLOPTTYPE_LONG, 139),

  /* Number of ports to try, including the first one set with LOCALPORT.
     Thus, setting it to 1 will make no additional attempts but the first.
  */
  CURLOPT(CURLOPT_LOCALPORTRANGE, CURLOPTTYPE_LONG, 140),

  /* no transfer, set up connection and let application use the socket by
     extracting it with CURLINFO_LASTSOCKET */
  CURLOPT(CURLOPT_CONNECT_ONLY, CURLOPTTYPE_LONG, 141),

  /* Function that will be called to convert from the
     network encoding (instead of using the iconv calls in libcurl) */
  CURLOPTDEPRECATED(CURLOPT_CONV_FROM_NETWORK_FUNCTION,
                    CURLOPTTYPE_FUNCTIONPOINT, 142,
                    7.82.0, "Serves no purpose anymore"),

  /* Function that will be called to convert to the
     network encoding (instead of using the iconv calls in libcurl) */
  CURLOPTDEPRECATED(CURLOPT_CONV_TO_NETWORK_FUNCTION,
                    CURLOPTTYPE_FUNCTIONPOINT, 143,
                    7.82.0, "Serves no purpose anymore"),

  /* Function that will be called to convert from UTF8
     (instead of using the iconv calls in libcurl)
     Note that this is used only for SSL certificate processing */
  CURLOPTDEPRECATED(CURLOPT_CONV_FROM_UTF8_FUNCTION,
                    CURLOPTTYPE_FUNCTIONPOINT, 144,
                    7.82.0, "Serves no purpose anymore"),

  /* if the connection proceeds too quickly then need to slow it down */
  /* limit-rate: maximum number of bytes per second to send or receive */
  CURLOPT(CURLOPT_MAX_SEND_SPEED_LARGE, CURLOPTTYPE_OFF_T, 145),
  CURLOPT(CURLOPT_MAX_RECV_SPEED_LARGE, CURLOPTTYPE_OFF_T, 146),

  /* Pointer to command string to send if USER/PASS fails. */
  CURLOPT(CURLOPT_FTP_ALTERNATIVE_TO_USER, CURLOPTTYPE_STRINGPOINT, 147),

  /* callback function for setting socket options */
  CURLOPT(CURLOPT_SOCKOPTFUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 148),
  CURLOPT(CURLOPT_SOCKOPTDATA, CURLOPTTYPE_CBPOINT, 149),

  /* set to 0 to disable session ID reuse for this transfer, default is
     enabled (== 1) */
  CURLOPT(CURLOPT_SSL_SESSIONID_CACHE, CURLOPTTYPE_LONG, 150),

  /* allowed SSH authentication methods */
  CURLOPT(CURLOPT_SSH_AUTH_TYPES, CURLOPTTYPE_VALUES, 151),

  /* Used by scp/sftp to do public/private key authentication */
  CURLOPT(CURLOPT_SSH_PUBLIC_KEYFILE, CURLOPTTYPE_STRINGPOINT, 152),
  CURLOPT(CURLOPT_SSH_PRIVATE_KEYFILE, CURLOPTTYPE_STRINGPOINT, 153),

  /* Send CCC (Clear Command Channel) after authentication */
  CURLOPT(CURLOPT_FTP_SSL_CCC, CURLOPTTYPE_LONG, 154),

  /* Same as TIMEOUT and CONNECTTIMEOUT, but with ms resolution */
  CURLOPT(CURLOPT_TIMEOUT_MS, CURLOPTTYPE_LONG, 155),
  CURLOPT(CURLOPT_CONNECTTIMEOUT_MS, CURLOPTTYPE_LONG, 156),

  /* set to zero to disable the libcurl's decoding and thus pass the raw body
     data to the application even when it is encoded/compressed */
  CURLOPT(CURLOPT_HTTP_TRANSFER_DECODING, CURLOPTTYPE_LONG, 157),
  CURLOPT(CURLOPT_HTTP_CONTENT_DECODING, CURLOPTTYPE_LONG, 158),

  /* Permission used when creating new files and directories on the remote
     server for protocols that support it, SFTP/SCP/FILE */
  CURLOPT(CURLOPT_NEW_FILE_PERMS, CURLOPTTYPE_LONG, 159),
  CURLOPT(CURLOPT_NEW_DIRECTORY_PERMS, CURLOPTTYPE_LONG, 160),

  /* Set the behavior of POST when redirecting. Values must be set to one
     of CURL_REDIR* defines below. This used to be called CURLOPT_POST301 */
  CURLOPT(CURLOPT_POSTREDIR, CURLOPTTYPE_VALUES, 161),

  /* used by scp/sftp to verify the host's public key */
  CURLOPT(CURLOPT_SSH_HOST_PUBLIC_KEY_MD5, CURLOPTTYPE_STRINGPOINT, 162),

  /* Callback function for opening socket (instead of socket(2)). Optionally,
     callback is able change the address or refuse to connect returning
     CURL_SOCKET_BAD. The callback should have type
     curl_opensocket_callback */
  CURLOPT(CURLOPT_OPENSOCKETFUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 163),
  CURLOPT(CURLOPT_OPENSOCKETDATA, CURLOPTTYPE_CBPOINT, 164),

  /* POST volatile input fields. */
  CURLOPT(CURLOPT_COPYPOSTFIELDS, CURLOPTTYPE_OBJECTPOINT, 165),

  /* set transfer mode (;type=<a|i>) when doing FTP via an HTTP proxy */
  CURLOPT(CURLOPT_PROXY_TRANSFER_MODE, CURLOPTTYPE_LONG, 166),

  /* Callback function for seeking in the input stream */
  CURLOPT(CURLOPT_SEEKFUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 167),
  CURLOPT(CURLOPT_SEEKDATA, CURLOPTTYPE_CBPOINT, 168),

  /* CRL file */
  CURLOPT(CURLOPT_CRLFILE, CURLOPTTYPE_STRINGPOINT, 169),

  /* Issuer certificate */
  CURLOPT(CURLOPT_ISSUERCERT, CURLOPTTYPE_STRINGPOINT, 170),

  /* (IPv6) Address scope */
  CURLOPT(CURLOPT_ADDRESS_SCOPE, CURLOPTTYPE_LONG, 171),

  /* Collect certificate chain info and allow it to get retrievable with
     CURLINFO_CERTINFO after the transfer is complete. */
  CURLOPT(CURLOPT_CERTINFO, CURLOPTTYPE_LONG, 172),

  /* "name" and "pwd" to use when fetching. */
  CURLOPT(CURLOPT_USERNAME, CURLOPTTYPE_STRINGPOINT, 173),
  CURLOPT(CURLOPT_PASSWORD, CURLOPTTYPE_STRINGPOINT, 174),

    /* "name" and "pwd" to use with Proxy when fetching. */
  CURLOPT(CURLOPT_PROXYUSERNAME, CURLOPTTYPE_STRINGPOINT, 175),
  CURLOPT(CURLOPT_PROXYPASSWORD, CURLOPTTYPE_STRINGPOINT, 176),

  /* Comma separated list of hostnames defining no-proxy zones. These should
     match both hostnames directly, and hostnames within a domain. For
     example, local.com will match local.com and www.local.com, but NOT
     notlocal.com or www.notlocal.com. For compatibility with other
     implementations of this, .local.com will be considered to be the same as
     local.com. A single * is the only valid wildcard, and effectively
     disables the use of proxy. */
  CURLOPT(CURLOPT_NOPROXY, CURLOPTTYPE_STRINGPOINT, 177),

  /* block size for TFTP transfers */
  CURLOPT(CURLOPT_TFTP_BLKSIZE, CURLOPTTYPE_LONG, 178),

  /* Socks Service */
  /* DEPRECATED, do not use! */
  CURLOPTDEPRECATED(CURLOPT_SOCKS5_GSSAPI_SERVICE,
                    CURLOPTTYPE_STRINGPOINT, 179,
                    7.49.0, "Use CURLOPT_PROXY_SERVICE_NAME"),

  /* Socks Service */
  CURLOPT(CURLOPT_SOCKS5_GSSAPI_NEC, CURLOPTTYPE_LONG, 180),

  /* set the bitmask for the protocols that are allowed to be used for the
     transfer, which thus helps the app which takes URLs from users or other
     external inputs and want to restrict what protocol(s) to deal
     with. Defaults to CURLPROTO_ALL. */
  CURLOPTDEPRECATED(CURLOPT_PROTOCOLS, CURLOPTTYPE_LONG, 181,
                    7.85.0, "Use CURLOPT_PROTOCOLS_STR"),

  /* set the bitmask for the protocols that libcurl is allowed to follow to,
     as a subset of the CURLOPT_PROTOCOLS ones. That means the protocol needs
     to be set in both bitmasks to be allowed to get redirected to. */
  CURLOPTDEPRECATED(CURLOPT_REDIR_PROTOCOLS, CURLOPTTYPE_LONG, 182,
                    7.85.0, "Use CURLOPT_REDIR_PROTOCOLS_STR"),

  /* set the SSH knownhost filename to use */
  CURLOPT(CURLOPT_SSH_KNOWNHOSTS, CURLOPTTYPE_STRINGPOINT, 183),

  /* set the SSH host key callback, must point to a curl_sshkeycallback
     function */
  CURLOPT(CURLOPT_SSH_KEYFUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 184),

  /* set the SSH host key callback custom pointer */
  CURLOPT(CURLOPT_SSH_KEYDATA, CURLOPTTYPE_CBPOINT, 185),

  /* set the SMTP mail originator */
  CURLOPT(CURLOPT_MAIL_FROM, CURLOPTTYPE_STRINGPOINT, 186),

  /* set the list of SMTP mail receiver(s) */
  CURLOPT(CURLOPT_MAIL_RCPT, CURLOPTTYPE_SLISTPOINT, 187),

  /* FTP: send PRET before PASV */
  CURLOPT(CURLOPT_FTP_USE_PRET, CURLOPTTYPE_LONG, 188),

  /* RTSP request method (OPTIONS, SETUP, PLAY, etc...) */
  CURLOPT(CURLOPT_RTSP_REQUEST, CURLOPTTYPE_VALUES, 189),

  /* The RTSP session identifier */
  CURLOPT(CURLOPT_RTSP_SESSION_ID, CURLOPTTYPE_STRINGPOINT, 190),

  /* The RTSP stream URI */
  CURLOPT(CURLOPT_RTSP_STREAM_URI, CURLOPTTYPE_STRINGPOINT, 191),

  /* The Transport: header to use in RTSP requests */
  CURLOPT(CURLOPT_RTSP_TRANSPORT, CURLOPTTYPE_STRINGPOINT, 192),

  /* Manually initialize the client RTSP CSeq for this handle */
  CURLOPT(CURLOPT_RTSP_CLIENT_CSEQ, CURLOPTTYPE_LONG, 193),

  /* Manually initialize the server RTSP CSeq for this handle */
  CURLOPT(CURLOPT_RTSP_SERVER_CSEQ, CURLOPTTYPE_LONG, 194),

  /* The stream to pass to INTERLEAVEFUNCTION. */
  CURLOPT(CURLOPT_INTERLEAVEDATA, CURLOPTTYPE_CBPOINT, 195),

  /* Let the application define a custom write method for RTP data */
  CURLOPT(CURLOPT_INTERLEAVEFUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 196),

  /* Turn on wildcard matching */
  CURLOPT(CURLOPT_WILDCARDMATCH, CURLOPTTYPE_LONG, 197),

  /* Directory matching callback called before downloading of an
     individual file (chunk) started */
  CURLOPT(CURLOPT_CHUNK_BGN_FUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 198),

  /* Directory matching callback called after the file (chunk)
     was downloaded, or skipped */
  CURLOPT(CURLOPT_CHUNK_END_FUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 199),

  /* Change match (fnmatch-like) callback for wildcard matching */
  CURLOPT(CURLOPT_FNMATCH_FUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 200),

  /* Let the application define custom chunk data pointer */
  CURLOPT(CURLOPT_CHUNK_DATA, CURLOPTTYPE_CBPOINT, 201),

  /* FNMATCH_FUNCTION user pointer */
  CURLOPT(CURLOPT_FNMATCH_DATA, CURLOPTTYPE_CBPOINT, 202),

  /* send linked-list of name:port:address sets */
  CURLOPT(CURLOPT_RESOLVE, CURLOPTTYPE_SLISTPOINT, 203),

  /* Set a username for authenticated TLS */
  CURLOPT(CURLOPT_TLSAUTH_USERNAME, CURLOPTTYPE_STRINGPOINT, 204),

  /* Set a password for authenticated TLS */
  CURLOPT(CURLOPT_TLSAUTH_PASSWORD, CURLOPTTYPE_STRINGPOINT, 205),

  /* Set authentication type for authenticated TLS */
  CURLOPT(CURLOPT_TLSAUTH_TYPE, CURLOPTTYPE_STRINGPOINT, 206),

  /* Set to 1 to enable the "TE:" header in HTTP requests to ask for
     compressed transfer-encoded responses. Set to 0 to disable the use of TE:
     in outgoing requests. The current default is 0, but it might change in a
     future libcurl release.

     libcurl will ask for the compressed methods it knows of, and if that
     is not any, it will not ask for transfer-encoding at all even if this
     option is set to 1.

  */
  CURLOPT(CURLOPT_TRANSFER_ENCODING, CURLOPTTYPE_LONG, 207),

  /* Callback function for closing socket (instead of close(2)). The callback
     should have type curl_closesocket_callback */
  CURLOPT(CURLOPT_CLOSESOCKETFUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 208),
  CURLOPT(CURLOPT_CLOSESOCKETDATA, CURLOPTTYPE_CBPOINT, 209),

  /* allow GSSAPI credential delegation */
  CURLOPT(CURLOPT_GSSAPI_DELEGATION, CURLOPTTYPE_VALUES, 210),

  /* Set the name servers to use for DNS resolution.
   * Only supported by the c-ares DNS backend */
  CURLOPT(CURLOPT_DNS_SERVERS, CURLOPTTYPE_STRINGPOINT, 211),

  /* Time-out accept operations (currently for FTP only) after this amount
     of milliseconds. */
  CURLOPT(CURLOPT_ACCEPTTIMEOUT_MS, CURLOPTTYPE_LONG, 212),

  /* Set TCP keepalive */
  CURLOPT(CURLOPT_TCP_KEEPALIVE, CURLOPTTYPE_LONG, 213),

  /* non-universal keepalive knobs (Linux, AIX, HP-UX, more) */
  CURLOPT(CURLOPT_TCP_KEEPIDLE, CURLOPTTYPE_LONG, 214),
  CURLOPT(CURLOPT_TCP_KEEPINTVL, CURLOPTTYPE_LONG, 215),

  /* Enable/disable specific SSL features with a bitmask, see CURLSSLOPT_* */
  CURLOPT(CURLOPT_SSL_OPTIONS, CURLOPTTYPE_VALUES, 216),

  /* Set the SMTP auth originator */
  CURLOPT(CURLOPT_MAIL_AUTH, CURLOPTTYPE_STRINGPOINT, 217),

  /* Enable/disable SASL initial response */
  CURLOPT(CURLOPT_SASL_IR, CURLOPTTYPE_LONG, 218),

  /* Function that will be called instead of the internal progress display
   * function. This function should be defined as the curl_xferinfo_callback
   * prototype defines. (Deprecates CURLOPT_PROGRESSFUNCTION) */
  CURLOPT(CURLOPT_XFERINFOFUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 219),

  /* The XOAUTH2 bearer token */
  CURLOPT(CURLOPT_XOAUTH2_BEARER, CURLOPTTYPE_STRINGPOINT, 220),

  /* Set the interface string to use as outgoing network
   * interface for DNS requests.
   * Only supported by the c-ares DNS backend */
  CURLOPT(CURLOPT_DNS_INTERFACE, CURLOPTTYPE_STRINGPOINT, 221),

  /* Set the local IPv4 address to use for outgoing DNS requests.
   * Only supported by the c-ares DNS backend */
  CURLOPT(CURLOPT_DNS_LOCAL_IP4, CURLOPTTYPE_STRINGPOINT, 222),

  /* Set the local IPv6 address to use for outgoing DNS requests.
   * Only supported by the c-ares DNS backend */
  CURLOPT(CURLOPT_DNS_LOCAL_IP6, CURLOPTTYPE_STRINGPOINT, 223),

  /* Set authentication options directly */
  CURLOPT(CURLOPT_LOGIN_OPTIONS, CURLOPTTYPE_STRINGPOINT, 224),

  /* Enable/disable TLS NPN extension (http2 over ssl might fail without) */
  CURLOPTDEPRECATED(CURLOPT_SSL_ENABLE_NPN, CURLOPTTYPE_LONG, 225,
                    7.86.0, "Has no function"),

  /* Enable/disable TLS ALPN extension (http2 over ssl might fail without) */
  CURLOPT(CURLOPT_SSL_ENABLE_ALPN, CURLOPTTYPE_LONG, 226),

  /* Time to wait for a response to an HTTP request containing an
   * Expect: 100-continue header before sending the data anyway. */
  CURLOPT(CURLOPT_EXPECT_100_TIMEOUT_MS, CURLOPTTYPE_LONG, 227),

  /* This points to a linked list of headers used for proxy requests only,
     struct curl_slist kind */
  CURLOPT(CURLOPT_PROXYHEADER, CURLOPTTYPE_SLISTPOINT, 228),

  /* Pass in a bitmask of "header options" */
  CURLOPT(CURLOPT_HEADEROPT, CURLOPTTYPE_VALUES, 229),

  /* The public key in DER form used to validate the peer public key
     this option is used only if SSL_VERIFYPEER is true */
  CURLOPT(CURLOPT_PINNEDPUBLICKEY, CURLOPTTYPE_STRINGPOINT, 230),

  /* Path to Unix domain socket */
  CURLOPT(CURLOPT_UNIX_SOCKET_PATH, CURLOPTTYPE_STRINGPOINT, 231),

  /* Set if we should verify the certificate status. */
  CURLOPT(CURLOPT_SSL_VERIFYSTATUS, CURLOPTTYPE_LONG, 232),

  /* Set if we should enable TLS false start. */
  CURLOPTDEPRECATED(CURLOPT_SSL_FALSESTART, CURLOPTTYPE_LONG, 233,
                    8.15.0, "Has no function"),

  /* Do not squash dot-dot sequences */
  CURLOPT(CURLOPT_PATH_AS_IS, CURLOPTTYPE_LONG, 234),

  /* Proxy Service Name */
  CURLOPT(CURLOPT_PROXY_SERVICE_NAME, CURLOPTTYPE_STRINGPOINT, 235),

  /* Service Name */
  CURLOPT(CURLOPT_SERVICE_NAME, CURLOPTTYPE_STRINGPOINT, 236),

  /* Wait/do not wait for pipe/mutex to clarify */
  CURLOPT(CURLOPT_PIPEWAIT, CURLOPTTYPE_LONG, 237),

  /* Set the protocol used when curl is given a URL without a protocol */
  CURLOPT(CURLOPT_DEFAULT_PROTOCOL, CURLOPTTYPE_STRINGPOINT, 238),

  /* Set stream weight, 1 - 256 (default is 16) */
  CURLOPT(CURLOPT_STREAM_WEIGHT, CURLOPTTYPE_LONG, 239),

  /* Set stream dependency on another curl handle */
  CURLOPT(CURLOPT_STREAM_DEPENDS, CURLOPTTYPE_OBJECTPOINT, 240),

  /* Set E-xclusive stream dependency on another curl handle */
  CURLOPT(CURLOPT_STREAM_DEPENDS_E, CURLOPTTYPE_OBJECTPOINT, 241),

  /* Do not send any tftp option requests to the server */
  CURLOPT(CURLOPT_TFTP_NO_OPTIONS, CURLOPTTYPE_LONG, 242),

  /* Linked-list of host:port:connect-to-host:connect-to-port,
     overrides the URL's host:port (only for the network layer) */
  CURLOPT(CURLOPT_CONNECT_TO, CURLOPTTYPE_SLISTPOINT, 243),

  /* Set TCP Fast Open */
  CURLOPT(CURLOPT_TCP_FASTOPEN, CURLOPTTYPE_LONG, 244),

  /* Continue to send data if the server responds early with an
   * HTTP status code >= 300 */
  CURLOPT(CURLOPT_KEEP_SENDING_ON_ERROR, CURLOPTTYPE_LONG, 245),

  /* The CApath or CAfile used to validate the proxy certificate
     this option is used only if PROXY_SSL_VERIFYPEER is true */
  CURLOPT(CURLOPT_PROXY_CAINFO, CURLOPTTYPE_STRINGPOINT, 246),

  /* The CApath directory used to validate the proxy certificate
     this option is used only if PROXY_SSL_VERIFYPEER is true */
  CURLOPT(CURLOPT_PROXY_CAPATH, CURLOPTTYPE_STRINGPOINT, 247),

  /* Set if we should verify the proxy in ssl handshake,
     set 1 to verify. */
  CURLOPT(CURLOPT_PROXY_SSL_VERIFYPEER, CURLOPTTYPE_LONG, 248),

  /* Set if we should verify the Common name from the proxy certificate in ssl
   * handshake, set 1 to check existence, 2 to ensure that it matches
   * the provided hostname. */
  CURLOPT(CURLOPT_PROXY_SSL_VERIFYHOST, CURLOPTTYPE_LONG, 249),

  /* What version to specifically try to use for proxy.
     See CURL_SSLVERSION defines below. */
  CURLOPT(CURLOPT_PROXY_SSLVERSION, CURLOPTTYPE_VALUES, 250),

  /* Set a username for authenticated TLS for proxy */
  CURLOPT(CURLOPT_PROXY_TLSAUTH_USERNAME, CURLOPTTYPE_STRINGPOINT, 251),

  /* Set a password for authenticated TLS for proxy */
  CURLOPT(CURLOPT_PROXY_TLSAUTH_PASSWORD, CURLOPTTYPE_STRINGPOINT, 252),

  /* Set authentication type for authenticated TLS for proxy */
  CURLOPT(CURLOPT_PROXY_TLSAUTH_TYPE, CURLOPTTYPE_STRINGPOINT, 253),

  /* name of the file keeping your private SSL-certificate for proxy */
  CURLOPT(CURLOPT_PROXY_SSLCERT, CURLOPTTYPE_STRINGPOINT, 254),

  /* type of the file keeping your SSL-certificate ("DER", "PEM", "ENG") for
     proxy */
  CURLOPT(CURLOPT_PROXY_SSLCERTTYPE, CURLOPTTYPE_STRINGPOINT, 255),

  /* name of the file keeping your private SSL-key for proxy */
  CURLOPT(CURLOPT_PROXY_SSLKEY, CURLOPTTYPE_STRINGPOINT, 256),

  /* type of the file keeping your private SSL-key ("DER", "PEM", "ENG") for
     proxy */
  CURLOPT(CURLOPT_PROXY_SSLKEYTYPE, CURLOPTTYPE_STRINGPOINT, 257),

  /* password for the SSL private key for proxy */
  CURLOPT(CURLOPT_PROXY_KEYPASSWD, CURLOPTTYPE_STRINGPOINT, 258),

  /* Specify which TLS 1.2 (1.1, 1.0) ciphers to use for proxy */
  CURLOPT(CURLOPT_PROXY_SSL_CIPHER_LIST, CURLOPTTYPE_STRINGPOINT, 259),

  /* CRL file for proxy */
  CURLOPT(CURLOPT_PROXY_CRLFILE, CURLOPTTYPE_STRINGPOINT, 260),

  /* Enable/disable specific SSL features with a bitmask for proxy, see
     CURLSSLOPT_* */
  CURLOPT(CURLOPT_PROXY_SSL_OPTIONS, CURLOPTTYPE_LONG, 261),

  /* Name of pre proxy to use. */
  CURLOPT(CURLOPT_PRE_PROXY, CURLOPTTYPE_STRINGPOINT, 262),

  /* The public key in DER form used to validate the proxy public key
     this option is used only if PROXY_SSL_VERIFYPEER is true */
  CURLOPT(CURLOPT_PROXY_PINNEDPUBLICKEY, CURLOPTTYPE_STRINGPOINT, 263),

  /* Path to an abstract Unix domain socket */
  CURLOPT(CURLOPT_ABSTRACT_UNIX_SOCKET, CURLOPTTYPE_STRINGPOINT, 264),

  /* Suppress proxy CONNECT response headers from user callbacks */
  CURLOPT(CURLOPT_SUPPRESS_CONNECT_HEADERS, CURLOPTTYPE_LONG, 265),

  /* The request target, instead of extracted from the URL */
  CURLOPT(CURLOPT_REQUEST_TARGET, CURLOPTTYPE_STRINGPOINT, 266),

  /* bitmask of allowed auth methods for connections to SOCKS5 proxies */
  CURLOPT(CURLOPT_SOCKS5_AUTH, CURLOPTTYPE_LONG, 267),

  /* Enable/disable SSH compression */
  CURLOPT(CURLOPT_SSH_COMPRESSION, CURLOPTTYPE_LONG, 268),

  /* Post MIME data. */
  CURLOPT(CURLOPT_MIMEPOST, CURLOPTTYPE_OBJECTPOINT, 269),

  /* Time to use with the CURLOPT_TIMECONDITION. Specified in number of
     seconds since 1 Jan 1970. */
  CURLOPT(CURLOPT_TIMEVALUE_LARGE, CURLOPTTYPE_OFF_T, 270),

  /* Head start in milliseconds to give happy eyeballs. */
  CURLOPT(CURLOPT_HAPPY_EYEBALLS_TIMEOUT_MS, CURLOPTTYPE_LONG, 271),

  /* Function that will be called before a resolver request is made */
  CURLOPT(CURLOPT_RESOLVER_START_FUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 272),

  /* User data to pass to the resolver start callback. */
  CURLOPT(CURLOPT_RESOLVER_START_DATA, CURLOPTTYPE_CBPOINT, 273),

  /* send HAProxy PROXY protocol header? */
  CURLOPT(CURLOPT_HAPROXYPROTOCOL, CURLOPTTYPE_LONG, 274),

  /* shuffle addresses before use when DNS returns multiple */
  CURLOPT(CURLOPT_DNS_SHUFFLE_ADDRESSES, CURLOPTTYPE_LONG, 275),

  /* Specify which TLS 1.3 ciphers suites to use */
  CURLOPT(CURLOPT_TLS13_CIPHERS, CURLOPTTYPE_STRINGPOINT, 276),
  CURLOPT(CURLOPT_PROXY_TLS13_CIPHERS, CURLOPTTYPE_STRINGPOINT, 277),

  /* Disallow specifying username/login in URL. */
  CURLOPT(CURLOPT_DISALLOW_USERNAME_IN_URL, CURLOPTTYPE_LONG, 278),

  /* DNS-over-HTTPS URL */
  CURLOPT(CURLOPT_DOH_URL, CURLOPTTYPE_STRINGPOINT, 279),

  /* Preferred buffer size to use for uploads */
  CURLOPT(CURLOPT_UPLOAD_BUFFERSIZE, CURLOPTTYPE_LONG, 280),

  /* Time in ms between connection upkeep calls for long-lived connections. */
  CURLOPT(CURLOPT_UPKEEP_INTERVAL_MS, CURLOPTTYPE_LONG, 281),

  /* Specify URL using CURL URL API. */
  CURLOPT(CURLOPT_CURLU, CURLOPTTYPE_OBJECTPOINT, 282),

  /* add trailing data just after no more data is available */
  CURLOPT(CURLOPT_TRAILERFUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 283),

  /* pointer to be passed to HTTP_TRAILER_FUNCTION */
  CURLOPT(CURLOPT_TRAILERDATA, CURLOPTTYPE_CBPOINT, 284),

  /* set this to 1L to allow HTTP/0.9 responses or 0L to disallow */
  CURLOPT(CURLOPT_HTTP09_ALLOWED, CURLOPTTYPE_LONG, 285),

  /* alt-svc control bitmask */
  CURLOPT(CURLOPT_ALTSVC_CTRL, CURLOPTTYPE_LONG, 286),

  /* alt-svc cache filename to possibly read from/write to */
  CURLOPT(CURLOPT_ALTSVC, CURLOPTTYPE_STRINGPOINT, 287),

  /* maximum age (idle time) of a connection to consider it for reuse
   * (in seconds) */
  CURLOPT(CURLOPT_MAXAGE_CONN, CURLOPTTYPE_LONG, 288),

  /* SASL authorization identity */
  CURLOPT(CURLOPT_SASL_AUTHZID, CURLOPTTYPE_STRINGPOINT, 289),

  /* allow RCPT TO command to fail for some recipients */
  CURLOPT(CURLOPT_MAIL_RCPT_ALLOWFAILS, CURLOPTTYPE_LONG, 290),

  /* the private SSL-certificate as a "blob" */
  CURLOPT(CURLOPT_SSLCERT_BLOB, CURLOPTTYPE_BLOB, 291),
  CURLOPT(CURLOPT_SSLKEY_BLOB, CURLOPTTYPE_BLOB, 292),
  CURLOPT(CURLOPT_PROXY_SSLCERT_BLOB, CURLOPTTYPE_BLOB, 293),
  CURLOPT(CURLOPT_PROXY_SSLKEY_BLOB, CURLOPTTYPE_BLOB, 294),
  CURLOPT(CURLOPT_ISSUERCERT_BLOB, CURLOPTTYPE_BLOB, 295),

  /* Issuer certificate for proxy */
  CURLOPT(CURLOPT_PROXY_ISSUERCERT, CURLOPTTYPE_STRINGPOINT, 296),
  CURLOPT(CURLOPT_PROXY_ISSUERCERT_BLOB, CURLOPTTYPE_BLOB, 297),

  /* the EC curves requested by the TLS client (RFC 8422, 5.1);
   * OpenSSL support via 'set_groups'/'set_curves':
   * https://docs.openssl.org/master/man3/SSL_CTX_set1_curves/
   */
  CURLOPT(CURLOPT_SSL_EC_CURVES, CURLOPTTYPE_STRINGPOINT, 298),

  /* HSTS bitmask */
  CURLOPT(CURLOPT_HSTS_CTRL, CURLOPTTYPE_LONG, 299),
  /* HSTS filename */
  CURLOPT(CURLOPT_HSTS, CURLOPTTYPE_STRINGPOINT, 300),

  /* HSTS read callback */
  CURLOPT(CURLOPT_HSTSREADFUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 301),
  CURLOPT(CURLOPT_HSTSREADDATA, CURLOPTTYPE_CBPOINT, 302),

  /* HSTS write callback */
  CURLOPT(CURLOPT_HSTSWRITEFUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 303),
  CURLOPT(CURLOPT_HSTSWRITEDATA, CURLOPTTYPE_CBPOINT, 304),

  /* Parameters for V4 signature */
  CURLOPT(CURLOPT_AWS_SIGV4, CURLOPTTYPE_STRINGPOINT, 305),

  /* Same as CURLOPT_SSL_VERIFYPEER but for DoH (DNS-over-HTTPS) servers. */
  CURLOPT(CURLOPT_DOH_SSL_VERIFYPEER, CURLOPTTYPE_LONG, 306),

  /* Same as CURLOPT_SSL_VERIFYHOST but for DoH (DNS-over-HTTPS) servers. */
  CURLOPT(CURLOPT_DOH_SSL_VERIFYHOST, CURLOPTTYPE_LONG, 307),

  /* Same as CURLOPT_SSL_VERIFYSTATUS but for DoH (DNS-over-HTTPS) servers. */
  CURLOPT(CURLOPT_DOH_SSL_VERIFYSTATUS, CURLOPTTYPE_LONG, 308),

  /* The CA certificates as "blob" used to validate the peer certificate
     this option is used only if SSL_VERIFYPEER is true */
  CURLOPT(CURLOPT_CAINFO_BLOB, CURLOPTTYPE_BLOB, 309),

  /* The CA certificates as "blob" used to validate the proxy certificate
     this option is used only if PROXY_SSL_VERIFYPEER is true */
  CURLOPT(CURLOPT_PROXY_CAINFO_BLOB, CURLOPTTYPE_BLOB, 310),

  /* used by scp/sftp to verify the host's public key */
  CURLOPT(CURLOPT_SSH_HOST_PUBLIC_KEY_SHA256, CURLOPTTYPE_STRINGPOINT, 311),

  /* Function that will be called immediately before the initial request
     is made on a connection (after any protocol negotiation step).  */
  CURLOPT(CURLOPT_PREREQFUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 312),

  /* Data passed to the CURLOPT_PREREQFUNCTION callback */
  CURLOPT(CURLOPT_PREREQDATA, CURLOPTTYPE_CBPOINT, 313),

  /* maximum age (since creation) of a connection to consider it for reuse
   * (in seconds) */
  CURLOPT(CURLOPT_MAXLIFETIME_CONN, CURLOPTTYPE_LONG, 314),

  /* Set MIME option flags. */
  CURLOPT(CURLOPT_MIME_OPTIONS, CURLOPTTYPE_LONG, 315),

  /* set the SSH host key callback, must point to a curl_sshkeycallback
     function */
  CURLOPT(CURLOPT_SSH_HOSTKEYFUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 316),

  /* set the SSH host key callback custom pointer */
  CURLOPT(CURLOPT_SSH_HOSTKEYDATA, CURLOPTTYPE_CBPOINT, 317),

  /* specify which protocols that are allowed to be used for the transfer,
     which thus helps the app which takes URLs from users or other external
     inputs and want to restrict what protocol(s) to deal with. Defaults to
     all built-in protocols. */
  CURLOPT(CURLOPT_PROTOCOLS_STR, CURLOPTTYPE_STRINGPOINT, 318),

  /* specify which protocols that libcurl is allowed to follow directs to */
  CURLOPT(CURLOPT_REDIR_PROTOCOLS_STR, CURLOPTTYPE_STRINGPOINT, 319),

  /* WebSockets options */
  CURLOPT(CURLOPT_WS_OPTIONS, CURLOPTTYPE_LONG, 320),

  /* CA cache timeout */
  CURLOPT(CURLOPT_CA_CACHE_TIMEOUT, CURLOPTTYPE_LONG, 321),

  /* Can leak things, gonna exit() soon */
  CURLOPT(CURLOPT_QUICK_EXIT, CURLOPTTYPE_LONG, 322),

  /* set a specific client IP for HAProxy PROXY protocol header? */
  CURLOPT(CURLOPT_HAPROXY_CLIENT_IP, CURLOPTTYPE_STRINGPOINT, 323),

  /* millisecond version */
  CURLOPT(CURLOPT_SERVER_RESPONSE_TIMEOUT_MS, CURLOPTTYPE_LONG, 324),

  /* set ECH configuration */
  CURLOPT(CURLOPT_ECH, CURLOPTTYPE_STRINGPOINT, 325),

  /* maximum number of keepalive probes (Linux, *BSD, macOS, etc.) */
  CURLOPT(CURLOPT_TCP_KEEPCNT, CURLOPTTYPE_LONG, 326),

  CURLOPT(CURLOPT_UPLOAD_FLAGS, CURLOPTTYPE_LONG, 327),

  /* set TLS supported signature algorithms */
  CURLOPT(CURLOPT_SSL_SIGNATURE_ALGORITHMS, CURLOPTTYPE_STRINGPOINT, 328),

  CURLOPT_LASTENTRY /* the last unused */
} CURLoption;

#define CURL_TIMECOND_NONE         0L
#define CURL_TIMECOND_IFMODSINCE   1L
#define CURL_TIMECOND_IFUNMODSINCE 2L
#define CURL_TIMECOND_LASTMOD      3L

typedef enum {
  /* we set a single member here, just to make sure we still provide
     the enum typedef, but the values to use are defined above with L
     suffixes */
  CURL_TIMECOND_LAST = 4
} curl_TimeCond;

/*
 * NAME curl_global_init()
 *
 * DESCRIPTION
 *
 * curl_global_init() should be invoked exactly once for each application that
 * uses libcurl and before any call of other libcurl functions.

 * This function is thread-safe if CURL_VERSION_THREADSAFE is set in the
 * curl_version_info_data.features flag (fetch by curl_version_info()).

 */
CURL_EXTERN CURLcode curl_global_init(long flags);

#define CURL_GLOBAL_SSL (1<<0) /* no purpose since 7.57.0 */
#define CURL_GLOBAL_WIN32 (1<<1)
#define CURL_GLOBAL_ALL (CURL_GLOBAL_SSL|CURL_GLOBAL_WIN32)
#define CURL_GLOBAL_NOTHING 0
#define CURL_GLOBAL_DEFAULT CURL_GLOBAL_ALL
#define CURL_GLOBAL_ACK_EINTR (1<<2)

struct curl_slist;

/*
 * NAME curl_slist_append()
 *
 * DESCRIPTION
 *
 * Appends a string to a linked list. If no list exists, it will be created
 * first. Returns the new list, after appending.
 */
CURL_EXTERN struct curl_slist *curl_slist_append(struct curl_slist *list,
                                                 const char *data);

/*
 * NAME curl_slist_free_all()
 *
 * DESCRIPTION
 *
 * free a previously built curl_slist.
 */
CURL_EXTERN void curl_slist_free_all(struct curl_slist *list);

/*
 * NAME curl_getdate()
 *
 * DESCRIPTION
 *
 * Returns the time, in seconds since 1 Jan 1970 of the time string given in
 * the first argument. The time argument in the second parameter is unused
 * and should be set to NULL.
 */
CURL_EXTERN time_t curl_getdate(const char *p, const time_t *unused);

#define CURLINFO_STRING   0x100000
#define CURLINFO_LONG     0x200000
#define CURLINFO_DOUBLE   0x300000
#define CURLINFO_SLIST    0x400000
#define CURLINFO_PTR      0x400000 /* same as SLIST */
#define CURLINFO_SOCKET   0x500000
#define CURLINFO_OFF_T    0x600000
#define CURLINFO_MASK     0x0fffff
#define CURLINFO_TYPEMASK 0xf00000

typedef enum {
  CURLINFO_NONE, /* first, never use this */
  CURLINFO_EFFECTIVE_URL    = CURLINFO_STRING + 1,
  CURLINFO_RESPONSE_CODE    = CURLINFO_LONG   + 2,
  CURLINFO_TOTAL_TIME       = CURLINFO_DOUBLE + 3,
  CURLINFO_NAMELOOKUP_TIME  = CURLINFO_DOUBLE + 4,
  CURLINFO_CONNECT_TIME     = CURLINFO_DOUBLE + 5,
  CURLINFO_PRETRANSFER_TIME = CURLINFO_DOUBLE + 6,
  CURLINFO_SIZE_UPLOAD CURL_DEPRECATED(7.55.0, "Use CURLINFO_SIZE_UPLOAD_T")
                            = CURLINFO_DOUBLE + 7,
  CURLINFO_SIZE_UPLOAD_T    = CURLINFO_OFF_T  + 7,
  CURLINFO_SIZE_DOWNLOAD
                       CURL_DEPRECATED(7.55.0, "Use CURLINFO_SIZE_DOWNLOAD_T")
                            = CURLINFO_DOUBLE + 8,
  CURLINFO_SIZE_DOWNLOAD_T  = CURLINFO_OFF_T  + 8,
  CURLINFO_SPEED_DOWNLOAD
                       CURL_DEPRECATED(7.55.0, "Use CURLINFO_SPEED_DOWNLOAD_T")
                            = CURLINFO_DOUBLE + 9,
  CURLINFO_SPEED_DOWNLOAD_T = CURLINFO_OFF_T  + 9,
  CURLINFO_SPEED_UPLOAD
                       CURL_DEPRECATED(7.55.0, "Use CURLINFO_SPEED_UPLOAD_T")
                            = CURLINFO_DOUBLE + 10,
  CURLINFO_SPEED_UPLOAD_T   = CURLINFO_OFF_T  + 10,
  CURLINFO_HEADER_SIZE      = CURLINFO_LONG   + 11,
  CURLINFO_REQUEST_SIZE     = CURLINFO_LONG   + 12,
  CURLINFO_SSL_VERIFYRESULT = CURLINFO_LONG   + 13,
  CURLINFO_FILETIME         = CURLINFO_LONG   + 14,
  CURLINFO_FILETIME_T       = CURLINFO_OFF_T  + 14,
  CURLINFO_CONTENT_LENGTH_DOWNLOAD
                       CURL_DEPRECATED(7.55.0,
                                      "Use CURLINFO_CONTENT_LENGTH_DOWNLOAD_T")
                            = CURLINFO_DOUBLE + 15,
  CURLINFO_CONTENT_LENGTH_DOWNLOAD_T = CURLINFO_OFF_T  + 15,
  CURLINFO_CONTENT_LENGTH_UPLOAD
                       CURL_DEPRECATED(7.55.0,
                                       "Use CURLINFO_CONTENT_LENGTH_UPLOAD_T")
                            = CURLINFO_DOUBLE + 16,
  CURLINFO_CONTENT_LENGTH_UPLOAD_T   = CURLINFO_OFF_T  + 16,
  CURLINFO_STARTTRANSFER_TIME = CURLINFO_DOUBLE + 17,
  CURLINFO_CONTENT_TYPE     = CURLINFO_STRING + 18,
  CURLINFO_REDIRECT_TIME    = CURLINFO_DOUBLE + 19,
  CURLINFO_REDIRECT_COUNT   = CURLINFO_LONG   + 20,
  CURLINFO_PRIVATE          = CURLINFO_STRING + 21,
  CURLINFO_HTTP_CONNECTCODE = CURLINFO_LONG   + 22,
  CURLINFO_HTTPAUTH_AVAIL   = CURLINFO_LONG   + 23,
  CURLINFO_PROXYAUTH_AVAIL  = CURLINFO_LONG   + 24,
  CURLINFO_OS_ERRNO         = CURLINFO_LONG   + 25,
  CURLINFO_NUM_CONNECTS     = CURLINFO_LONG   + 26,
  CURLINFO_SSL_ENGINES      = CURLINFO_SLIST  + 27,
  CURLINFO_COOKIELIST       = CURLINFO_SLIST  + 28,
  CURLINFO_LASTSOCKET  CURL_DEPRECATED(7.45.0, "Use CURLINFO_ACTIVESOCKET")
                            = CURLINFO_LONG   + 29,
  CURLINFO_FTP_ENTRY_PATH   = CURLINFO_STRING + 30,
  CURLINFO_REDIRECT_URL     = CURLINFO_STRING + 31,
  CURLINFO_PRIMARY_IP       = CURLINFO_STRING + 32,
  CURLINFO_APPCONNECT_TIME  = CURLINFO_DOUBLE + 33,
  CURLINFO_CERTINFO         = CURLINFO_PTR    + 34,
  CURLINFO_CONDITION_UNMET  = CURLINFO_LONG   + 35,
  CURLINFO_RTSP_SESSION_ID  = CURLINFO_STRING + 36,
  CURLINFO_RTSP_CLIENT_CSEQ = CURLINFO_LONG   + 37,
  CURLINFO_RTSP_SERVER_CSEQ = CURLINFO_LONG   + 38,
  CURLINFO_RTSP_CSEQ_RECV   = CURLINFO_LONG   + 39,
  CURLINFO_PRIMARY_PORT     = CURLINFO_LONG   + 40,
  CURLINFO_LOCAL_IP         = CURLINFO_STRING + 41,
  CURLINFO_LOCAL_PORT       = CURLINFO_LONG   + 42,
  CURLINFO_TLS_SESSION CURL_DEPRECATED(7.48.0, "Use CURLINFO_TLS_SSL_PTR")
                            = CURLINFO_PTR    + 43,
  CURLINFO_ACTIVESOCKET     = CURLINFO_SOCKET + 44,
  CURLINFO_TLS_SSL_PTR      = CURLINFO_PTR    + 45,
  CURLINFO_HTTP_VERSION     = CURLINFO_LONG   + 46,
  CURLINFO_PROXY_SSL_VERIFYRESULT = CURLINFO_LONG + 47,
  CURLINFO_PROTOCOL    CURL_DEPRECATED(7.85.0, "Use CURLINFO_SCHEME")
                            = CURLINFO_LONG   + 48,
  CURLINFO_SCHEME           = CURLINFO_STRING + 49,
  CURLINFO_TOTAL_TIME_T     = CURLINFO_OFF_T + 50,
  CURLINFO_NAMELOOKUP_TIME_T = CURLINFO_OFF_T + 51,
  CURLINFO_CONNECT_TIME_T   = CURLINFO_OFF_T + 52,
  CURLINFO_PRETRANSFER_TIME_T = CURLINFO_OFF_T + 53,
  CURLINFO_STARTTRANSFER_TIME_T = CURLINFO_OFF_T + 54,
  CURLINFO_REDIRECT_TIME_T  = CURLINFO_OFF_T + 55,
  CURLINFO_APPCONNECT_TIME_T = CURLINFO_OFF_T + 56,
  CURLINFO_RETRY_AFTER      = CURLINFO_OFF_T + 57,
  CURLINFO_EFFECTIVE_METHOD = CURLINFO_STRING + 58,
  CURLINFO_PROXY_ERROR      = CURLINFO_LONG + 59,
  CURLINFO_REFERER          = CURLINFO_STRING + 60,
  CURLINFO_CAINFO           = CURLINFO_STRING + 61,
  CURLINFO_CAPATH           = CURLINFO_STRING + 62,
  CURLINFO_XFER_ID          = CURLINFO_OFF_T + 63,
  CURLINFO_CONN_ID          = CURLINFO_OFF_T + 64,
  CURLINFO_QUEUE_TIME_T     = CURLINFO_OFF_T + 65,
  CURLINFO_USED_PROXY       = CURLINFO_LONG + 66,
  CURLINFO_POSTTRANSFER_TIME_T = CURLINFO_OFF_T + 67,
  CURLINFO_EARLYDATA_SENT_T = CURLINFO_OFF_T + 68,
  CURLINFO_HTTPAUTH_USED    = CURLINFO_LONG + 69,
  CURLINFO_PROXYAUTH_USED   = CURLINFO_LONG + 70,
  CURLINFO_LASTONE          = 70
} CURLINFO;

/* CURLINFO_RESPONSE_CODE is the new name for the option previously known as
   CURLINFO_HTTP_CODE */
#define CURLINFO_HTTP_CODE CURLINFO_RESPONSE_CODE

/** curl/easy.h **/

CURL_EXTERN CURL *curl_easy_init(void);
CURL_EXTERN CURLcode curl_easy_setopt(CURL *curl, CURLoption option, ...);
CURL_EXTERN CURLcode curl_easy_perform(CURL *curl);
CURL_EXTERN void curl_easy_cleanup(CURL *curl);

/*
 * NAME curl_easy_getinfo()
 *
 * DESCRIPTION
 *
 * Request internal information from the curl session with this function.
 * The third argument MUST be pointing to the specific type of the used option
 * which is documented in each manpage of the option. The data pointed to
 * will be filled in accordingly and can be relied upon only if the function
 * returns CURLE_OK. This function is intended to get used *AFTER* a performed
 * transfer, all results from this function are undefined until the transfer
 * is completed.
 */
CURL_EXTERN CURLcode curl_easy_getinfo(CURL *curl, CURLINFO info, ...);

#ifdef __cplusplus
}
#endif
