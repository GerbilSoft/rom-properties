# Changes

## v1.6 (released 2020/??/??)

* New parser features:
  * Game Boy, Game Boy Color, Game Boy Advance: Added external title screen
    images using the official ROM Properties online database server.
  * Game Boy Advance: Added metadata properties for Title and Publisher.
    (Same as Game Boy and Game Boy Color.)

* Other changes:
  * Split file handling and CPU/byteorder code from librpbase into two
    libraries: librpfile and librpcpu.

## v1.5 (released 2020/03/13)

* New features:
  * Improved support for FreeBSD and DragonFly BSD:
    * FreeBSD 10.0's iconv() is used if available. Otherwise, libiconv is used.
    * Fix LDFLAG detection for compressed debug sections (and others).
    * General build fixes for differences in e.g. system headers.
  * The SCSI handlers now have a one-sector cache. This should improve
    performance in many cases, since the various RomData subclasses do small
    reads for certain things instead of one giant read.
    * FIXME: Need to make use of the cache in the "contiguous" read section.
  * Image handling has been split from librpbase to librptexture. This will
    allow the texture decoding functionality to be used by other programs.
  * Key Manager: Import the Korean key from Wii keys.bin if it's present.
  * Added a UI frontend for the Cinnamon Desktop Environment using the Nemo
    file browser.
  * Cached files now contain origin information indicating the URL they were
    downloaded from. This can be disabled in rp-config if it isn't wanted.
  * The GTK+ UI frontends now support GVfs for network transparency.
  * KDE and GTK+ UI frontends: Icons can now be dragged and dropped from the
    property tab to another file browser window to extract them. This includes
    the icon, banner, and (KDE only) icons from ListView widgets.
    * Drag & drop support for Windows will be implemented in a future release.
    * Note that the dragged images don't have filenames yet, so either they
      will be extracted with generic filenames, or you will be prompted to
      enter a filename.
    * Note that Thunar doesn't seem to be accepting the dropped image data.
      This will be resolved in a future release.
  * Downloading functionality for online databases has been split out of the
    DLL and into a separate executable. This will allow the downloading to be
    handled in a lower privilege environment.
    * Windows: On Vista and later, rp-download runs as a low-integrity process.
    * Added other security functionality for rp-download and rpcli:
      * Linux: AppArmor profiles, libseccomp
      * OpenBSD: pledge() [and tame() for old versions]
    * AppArmor profiles for rp-stub will be added in a future release.
  * Windows: The online database code has been rewritten to use WinInet
    directly instead of urlmon, which reduces overhead.
  * The UI frontends now show a dropdown box to select the language if the
    ROM image has multiple translations for e.g. the game title.
  * Linux: Thumbnailers now write the "Thumb::Image::Width" and
    "Thumb::Image::Height" properties where applicable. These represent the
    size of the original image that was thumbnailed.
  * GTK+ 3.x: Don't premultiply the image before saving it as a PNG image.
    Premultiplication is only needed when displaying the image using Cairo.

* New parsers:
  * DidjTex: Leapster Didj .tex and .texs texture files. For .texs, currently
    only the first texture of the archive is supported.
  * PowerVR3: PowerVR 3.0.0 textures. Used mainly by iOS applications. Not
    related to Sega Dreamcast PVR format.
  * PokemonMini: Pokémon Mini ROM images.
  * KhronosKTX2: Khronos KTX 2.0 texture format. (based on draft18)

* New parser features:
  * WiiWAD, iQuePlayer: Display the console IDs from tickets. This is usually
    0x00000000 for system titles and unlicensed copies.
  * KhronosKTX: Added support for BPTC (BC7) texture compression.
  * amiibo: Updated the internal database to be current as of 2020/01/11.
    Added some missing entries and fixed a few incorrect entries.
  * Xbox360_STFS: Title thumbnail images are now displayed as internal icons.

* New compressed texture formats:
  * PowerVR Texture Compression. Supports PVRTC-I 2bpp and 4bpp in PowerVR
    3.0.0, Khronos KTX, and DirectDraw Surface texture files. Uses code from
    the PowerVR Native SDK, which is redistributable under the MIT License.
  * PVRTC-II is partially supported.
    * TODO: Hard transition flag, images that aren't a multiple of the
      tile size.
  * RGB9_E5 is now supported in PowerVR 3.0, KTX, and DDS textures.
    * KTX and DDS are untested and may need adjustments.
    * RGB9_E5 is an HDR format, but rom-properties currently converts it
      to ARGB32 (LDR) for display purposes.

* Bug fixes:
  * Fixed misdetection of NCCH sections if keys are missing.
  * GameCube I8 image decoder: The palette was being generated incorrectly.
    This has been fixed, though since I8 images are uncommon, this probably
    didn't cause too many problems.
  * Xbox XPR0 texture decoder: Fix a NULL pointer dereference that might
    happen if the texture format is swizzled but texture decoding fails.
  * SegaPVR: Added the .svr extension. Without this, SVR decoding doesn't
    work on Windows because it doesn't get registered for .svr files.
  * Fixed detection issues that caused the Nintendo DS ROM image "Live On
    Card Live-R DS" to be detected as an Xbox 360 STFS package. Reported by
    @Whovian9369.
  * Fixed transparent gzip decompression for files where the gzipped version
    is actually larger than the uncompressed version.
  * KhronosKTX: The metadata keys should be treated as case-insensitive, since
    some programs write "KTXOrientation" instead of "KTXorientation".
  * dll-search.c: The Mate Desktop check incorrectly had two entries for MATE
    and lacked an entry for GNOME.
  * Nintendo3DS: Fixed NCCH detection for CFAs with an "icon" file but no
    ".code" file, which is usually the case for DLC CIAs.
    https://github.com/GerbilSoft/rom-properties/issues/208
  * GameCube: Fixed the GameTDB region code for Italy.
  * CurlDownloader: Use a case-insensitive check for HTTP headers.
    cURL provides headers in lowercase when accessing an HTTP/2 server.
    This fixes handling of mtimes and content length.
  * Fixed some CBC decryption issues on Windows XP. This mostly affected
    decrypting Xbox 360 executables.
  * Nintendo3DS: Fixed decryption of games where the title ID does not
    match the program ID. This seems to show up in Traditional Chinese
    releases that use a Japanese region code instead of Taiwan.
  * rpcli JSON output: Fixed RFT_LISTDATA commas, RFT_DIMENSIONS format,
    and external image URLs format. Also escaped double-quotes properly.
  * Linux: Added the "application/x-cso" MIME type for GameCube .ciso
    format on Linux. (Note that this technically refers to a different
    format, but GameCube .ciso is incorrectly identified as this.)
  * GameCube: Calculate the used partition size correctly for unencrypted
    Wii RVT-H Reader disc images.
  * EXE: VS_VERSION_INFO resources in 16-bit Windows executables are now
    displayed correctly if they're encoded using a code page other than
    Windows-1252.
  * Fixed GameTDB downloads for CHN and TWN region games. (Nintendo DS,
    Nintendo 3DS, GameCube, Wii)
  * MegaDrive: Fixed region code detection for locked-on ROMs. Previously,
    it would use the base ROM region code for both.
  * MachO: Fixed CPU subtype detection for 486SX.
  * KDE frontends: Disabled automatic mnemonics on checkboxes used to show
    bitfield values, e.g. region codes and hardware support.
  * S3TC, BC7: Images with sizes that aren't a multiple of the 4x4 tile size
    can now be decoded. It is assumed that an extra tile is present, and this
    tile will be truncated to match the specified dimensions.
    This bug was reported by @HyperPolygon64.
  * GameCube: Fixed handling of NDDEMO's game ID. Instead of reporting no
    fields due to the NULL byte, change it to an underscore.
  * Linux: Fixed an off-by-one in the shared library search function that
    could cause rp-config to fail if e.g. using a GTK+ (either 2.x or 3.x)
    desktop environment with only the KDE4 UI frontend installed.
  * GameCube: Region ID 'I' is Italy ("IT"), not Netherlands ("NL"). This may
    have affected GameTDB downloads.
  * KDE thumbnails: Fixed an incorrect "Thumb::MTime" entry in some cases.
    The entry was being saved as "Thumb::Size", which resulted in two
    "Thumb::Size" entries.
  * Windows: Fixed incorrect file extension registration of Mach-O dylibs
    and bundles. (".dylib.bundle" was registered instead of ".dylib" and
    ".bundle" as separate extensions.)
  * KDE: Fixed vertical sizing of RFT_LISTDATA fields in some cases, e.g.
    Nintendo3DS.
  * rp_image::squared(): Added (non-optimal) CI8 support; fixed a potential
    memory corruption issue for images that are taller than they are wide.

* Other changes:
  * Removed the internal copy of libjpeg-turbo. On Windows, gdiplus is now
    used for JPEG decoding. This setup was used before for PNG, but we now
    use more PNG functionality than is available from gdiplus, so libpng
    is staying put.
  * Xbox360_XEX: A warning is now shown if a required encryption key isn't
    available.
  * MegaDrive: Do a quick check of the 32X security code to verify that a
    32X ROM is in fact for 32X.
  * Renamed the KDE5 frontend to KF5 to match upstream branding guidelines.
  * rpcli: Always use the localized system name that matches the ROM image.

## v1.4.3 (released 2019/09/16)

* Bug fixes:
  * GdiReader:
    * Fixed pointer confusion that caused crashes.
      This bug was reported by @HyperPolygon64.
    * Added support for GDI+ISO (2048-byte sectors).

## v1.4.2 (released 2019/09/16)

* Bug fixes:
  * Xbox360_XDBF: Improved avatar award handling:
    * Fixed an assertion (debug builds only) if an empty XGAA table is present.
    * Show all avatar awards, even if they have duplicate IDs.
  * GameCubeBNR:
    * Metadata: Fixed gamename vs. gamename_full copy/paste error.
  * RomMetaData:
    * Handle nullptr strings gracefully instead of crashing.
      https://github.com/GerbilSoft/rom-properties/issues/195

## v1.4.1 (released 2019/09/07)

* New parser features:
  * amiibo: Updated the internal database to be current as of 2019/08/18.

* Bug fixes:
  * In some cases, the SCSI handlers for both Linux and Windows weren't working
    properly. In particular, on Linux, attempting to read "too much" data at
    once resulted in getting nothing, and on both systems, in some cases,
    passing a buffer larger than the number of requested sectors results in
    the sectors being loaded at the *end* of the buffer, not the beginning.
  * rp-config: If keys were imported from a supported key database file and
    settings saved, those keys wouldn't actually be saved, since the import
    function did not set the "changed" flag. As a workaround, modifying any key
    manually before saving would set the "changed" flag.

## v1.4 (released 2019/08/04)

* New features:
  * The Cairo graphics library is now used for GTK+ 3.x builds.
  * The un-premultiply function, used for DXT2 and DXT4 images, now has
    an SSE4.1-optimized implementation. The standard implementation has
    also been optimized to be somewhat faster.
  * The XFCE frontend can now be built for both GTK+ 2.x and 3.x.
  * File metadata is now exported to the desktop environment. This includes
    e.g. dimensions for textures and duration for audio. Currently supported
    on KDE and Windows.
  * Support for gzipped files. This is important for VGMs, which are commonly
    distributed in gzipped format (.vgz).
  * ROM images for Nintendo DS and 3DS with "dangerous" permissions now have
    an overlay icon on Windows and KDE. This normally happens with certain
    homebrew that requires full system access (or system titles), but it's
    also possible that someone could create a malicious homebrew title
    disguised as a retail game.
    * NOTE: The Windows implementation currently requires Windows Vista
      or later.
    * NOTE 2: The Windows implementation was disabled for this release due
      to unexplained crashes.
  * rpcli will now print multi-line entries in lists using multiple lines
    instead of showing a U+240A symbol between lines.
  * RFT_LISTDATA fields (ListView, etc.) can now have text alignment set
    by the various RomData subclasses.
  * Added an option to disable thumbnailing and metadata extraction on
    network file systems and "bad" file systems. The option is set by
    default to reduce slowdown.
  * Added a UI frontend for the MATE Desktop Environment using the Caja
    file browser. MATE 1.18 or later is required. (GTK+ 3.x)

* New parsers:
  * WiiWAD: Wii WAD packages. Contains WiiWare, Virtual Console, and other
    installable titles. Supports GameTDB image downloads for most WADs,
    and extracts the internal icon and banner for DLC WADs.
    * Also supports early devkit WADs that use a slightly different format.
  * WiiSave: Encrypted Wii save files. These are the data.bin files present
    on the SD card when copying a save file from the Wii Menu.
  * WiiWIBN: Wii banner files. These are contained in encrypted Wii save
    files and Wii DLC WADs, and are present as standalone files in unencrypted
    Wii save directories as `banner.bin`.
  * MachO: Mach-O binary parser for Mac OS X and related operating systems.
    Includes support for Universal Binaries.
  * NGPC: Neo Geo Pocket (Color) ROM image parser.
  * Xbox360_XDBF: Xbox 360 XDBF resource files. Only the variant used by
    XEX executables is supported at the moment.
  * Xbox360_XEX: Xbox 360 executables.
  * XboxXPR: Xbox XPR0 textures.
    * Used in save files and elsewhere.
    * Currently only supports XPR0. XPR1 and XPR2 are texture archives,
      and support for those may be added later.
  * ISO: ISO-9660 parser.
    * Parses the Primary Volume Descriptor for ISO-9660 images.
    * Sega Mega CD, Sega Saturn, and Sega Dreamcast parsers now show
      the ISO-9660 PVD using the new ISO-9660 parser.
  * XboxDisc: Xbox and Xbox 360 disc image parser.
    * Reads default.xbe and/or default.xex from the XDVDFS partition and
      calls the appropriate parser to handle these files.
    * Supports reading original discs if a supported DVD-ROM drive with Kreon
      firmware is connected.
  * Xbox_XBE: Original Xbox executables.
  * Xbox360_STFS: Xbox 360 packages.
  * iQuePlayer: iQue Player content metadata and ticket files.

* New audio parsers:
  * ADX: CRI ADX audio format. Used by many Sega games.
  * GBS: Game Boy Sound System. Used for Game Boy audio rips.
    * Supports GBS files embedded in GBS Player ROM images.
  * NSF: Nintendo Sound Format. Used for NES audio rips.
  * PSF: Portable Sound Format. Used for PlayStation audio rips.
    Subformats are used for several other platforms.
  * SAP: Atari 8-bit audio format.
  * SID: Commodore 64 SID Music format.
  * SNDH: Atari ST audio format. Supports uncompressed files and files
    compressed using the ICE! algorithm.
  * SPC: Super NES SPC-700 audio format.
  * VGM: Video Game Music format. Used for all sorts of video game
    audio rips, including Sega Mega Drive and Neo Geo Pocket.
  * BRSTM: Nintendo Wii BRSTM audio files.
  * BCSTM: Nintendo 3DS BCSTM and BCWAV audio files.
    * Also supports Nintendo Wii U BFSTM audio files.

* New parser features:
  * DMG: Added support for the GBX footer. This is used to indicate certain
    cartridge features that either can't be represented in the DMG header
    or aren't properly represented in e.g. unlicensed games.
  * DirectDrawSurface:
    * For non-DX10 textures, the equivalent DX10 format is now shown if it
      can be determined.
    * BC7 texture compression is now supported.
  * ELF:
    * Two PT_DYNAMIC headers are now displayed if present:
      * DT_FLAGS
      * DT_FLAGS_1
    * Linkage is now only shown for executables.
  * GameCube:
    * Split the GameCube opening.bnr code into a separate parser.
      This allows standalone GameCube opening.bnr files to be handled.
    * Display the title ID for Wii games, since it might be different than
      the game ID.
    * Show access rights, i.e. AHBPROT and DVD Video.
    * Detect Wii update partitions that have Boot2 and a single IOS, but
      not a System Menu.
    * Added support for NASOS images. (GCML, WII5, and WII9)
      * GCMM images are not supported yet because there doesn't seem to
        be any documentation on how to create one.
    * Added China and Taiwan region codes.
    * Added support for standalone Wii partitions. These are commonly used
      with the NKit tool to restore full Wii disc images.
  * NES: Added support for the internal footer present in some ROMs. This
    footer is used for, among other things, FamicomBox.
  * Nintendo3DS: Split the SMDH code into a separate parser. This should
    make maintenance easier.
  * NintendoDS:
    * The "Access Control" field (labeled "Permissions") is now shown for ROMs
      with DSi functionality. Both permissions and flags are now shown on a new
      tab, and they both use a listbox instead of a grid of checkboxes.
    * DSi flags are now shown for NDS ROMs released after the DSi, since these
      ROMs have some DSi-specific headers for signature verification, even if
      they aren't DSi-enhanced.
  * SegaPVR: Support for SVR files used on the Sony PlayStation 2.
    * Most formats are supported, excluding those that require an external palette.
    * Format 0x61 (rectangle, swizzled) is partially supported.
    * 4-bit textures >=128x128 might not be unswizzled correctly at the moment.

* Bug fixes:
  * Nintendo 3DS:
    * Handle DLC packages with more than 64 contents.
    * Correctly handle localized titles. Previously, English titles were
      used regardless of the system region.
  * Linux: Fixed a file handle leak that was only noticeable when handling
    more than 1,023 files in one invocation of the `rpcli` program.
  * GameCube: Fix bug that prevented "EUR" and "JPN" from being appended
    to region code descriptions if the game's BI2 region doesn't match
    the ID4 region.
  * EXE:
    * Fixed a crash that occurred when viewing the properties of a
      Windows 1.0 executable in any UI frontend other than rpcli.
    * Fixed inverted "windowsSetting" flags.
  * CBCReader: Added unaligned read support and fixed positioning issues
    when reading unencrypted data. This only affected the Xbox360_XEX
    parser; WiiSave, WiiWAD, and CIAReader were not affected.
  * MegaDrive: Fixed an off-by-one error that caused Mega CD disc images
    to be detected as Mega Drive ROM images.
  * ETC2: Fixed alpha decoding for GL_COMPRESSED_RGBA8_ETC2_EAC. Note that
    the test case image used the incorrect decoding, so tests didn't catch
    this bug.
  * SegaPVR: Fixed Small VQ decoding for 16x16 and 32x32 textures without
    mipmaps.

* Other changes:
  * Removed S2TC decoding. The first version of rom-properties with support
    for S3TC, v1.2, was released *after* the S3TC patents expired. Since
    the patents expired, S3TC can be unconditionally enabled, and we don't
    need the S2TC decoder or test images anymore.
  * The Linux UI frontends will no longer initialize if running as root.
    `rpcli` will still run as root for now, though it's not recommended.
  * The XDG MIME database rom-properties.xml is now installed on Linux
    systems. This provides MIME types for formats not currently present in
    FreeDesktop.org's shared-mime-info database.

## v1.3.3 (released 2018/08/25)

* GameCube: Fixed a crash when downloading external images for Disc 2
  from multi-disc games. Thanks to @Nomelas for reporting this bug.

## v1.3.2 (released 2018/06/30)

* Linux: Fixed a crash on GNOME and XFCE when using Ubuntu 16.04.
  IFUNC was not being disabled on gcc5 in the GTK+ directory.

* XFCE: The Specialized Thumbnailer file has a MimeTypes key, not MimeType.

## v1.3.1 (released 2018/06/12)

* Windows: Fixed a bug that caused Explorer to crash if a supported file had
  an internal image whose width was not a multiple of 8.

* Bug fixes:
  * Wii U: The .wux file extension wasn't registered on Windows, so it wasn't
    properly detected.

## v1.3 - The Internationalization Release (released 2018/06/02)

* New features:
  * Most strings are now localizable. The gettext utilities and libintl
    library are used to provide localization support.
  * All builds now use UTF-8 for text encoding. This increases overhead
    slightly on Windows due to required UTF-8 to UTF-16 conversion, but it
    reduces the DLL size. (The Qt build doesn't have much overhead difference,
    since it has to copy char16_t data into QString before using it anyway.)
    * The `rp_char` and `rp_string` typedefs have been removed.
  * The 16-bit and 32-bit array byteswapping functions have been optimized
    using MMX, SSE2, and SSSE3.

* New texture formats:
  * Khronos KTX textures: https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/
  * Valve VTF textures: https://developer.valvesoftware.com/wiki/Valve_Texture_Format
  * Valve VTF3 (PS3) textures: Reverse-engineered from Portal for PS3.

* New systems:
  * ELF: Executable and Linkable Format. Used by Linux, most Unix systems,
    and many other platforms, including GameCube and Wii. A modified version,
    RPX/RPL, is used on Wii U and is also supported.
  * game.com: Tiger game.com ROM images. Supports properties and icons.

* New system features:
  * Super NES: Satellaview BS-X ROM headers are now decoded properly.
  * Wii: Added support for unencrypted images from debug systems and GCM
    images with SDK headers.
  * NES: Improved publisher lookup for FDS titles.
  * Wii U: Display game publishers; added support for .wux disc images.
  * DMG: Added more entry point formats.
  * DirectDrawSurface: Untested support for Xbox One textures.
  * NintendoDS: Improved DSi flag decoding; added key index decoding.
  * N64: Fix CRC fields; added Release field.

* New compressed texture formats:
  * Ericsson ETC1 and ETC2
  * Variations of S3TC compression in different texture formats, including
    support for both RGTC and LATC in Khronos KTX files.
  * Support for DXT1 with and without 1-bit alpha. Alpha selection is supported
    by Khronos KTX and Valve VTF. For other texture file formats, the DXT1_A1
    algorithm is selected for maximum compatibility.

* Bug fixes:
  * EXE: "Product Version" was showing the same value as "File Version".
    It's usually the same, but there are cases where these version numbers
    are different, e.g. wrapper EXEs for interpreted programs.
  * rp-config: If a key is present in keys.conf but has no value, it was
    previously listed as invalid. It's now listed as empty.
  * KhronosKTX: Copy sBIT if the image needs to be vertically flipped.
  * Win32: The Properties tab sometimes didn't show up if another program
    claimed ownership of the file extension, e.g. VS2017 handling .dds files.
    The tab is now registered to handle all files, though it will only show
    up if the file is actually supported by rom-properties.
  * DreamcastSave: Trim spaces from the end of text fields.
  * KDE frontends: Enabled the "ROM Properties" tab on files located on the
    desktop.
  * MegaDrive: Fixed detection of some unlicensed ROMs that have extra text
    in the "System" field.
  * GameCube: Fixed a crash when reading a Wii disc image that has no game
    partition.
  * GTK+: Fixed a potential crash if running on a system that doesn't support
    SSSE3 instructions.
  * Win32: rp-config's option to clear the rom-properties cache didn't do
    anything. It now clears the rom-properties cache directory.

* Other changes:
  * libromdata/ has been reorganized to use subdirectories for type of system.
    This currently includes "Console", "Handheld", "Texture", and "Other".
  * rpcli no longer supports BMP output. Use PNG output instead.

## v1.2 - The Dreamcast Release (released 2017/11/12)

* New features:
  * rpcli, rp-stub, rp-thumbnailer-dbus: PNG images now have an `sBIT` chunk,
    which indicates the number of bits per channel that were present in the
    original image. In addition, RGB images that don't have an alpha channel
    are now saved as RGB PNGs instead of ARGB PNGs, which usually results in
    a smaller file.
  * Some functions are now optimized using SIMD instructions if supported by
    the host system's CPU:
    * Sega Mega Drive: SMD format decoding (SSE2)
    * 15/16-bit linear RGB decoding (SSE2)
    * 24-bit linear RGB image decoding (SSSE3)
    * 32-bit linear ARGB/xRGB image decoding (SSSE3)

* New systems:
  * Sega PVR and GVR decoding. These are texture files used in Sega games
    on Dreamcast, GameCube, and various PC ports.
  * Microsoft DirectDraw Surface decoding. These are texture files used in
    various PC games. Supports several uncompressed RGB and ARGB formats,
    DXTn, BC4, and BC5.
  * Nintendo Badge Arcade decoding. Supports both PRBS (badge) and CABS
    (badge set) files. PRBS decoding supports mega badges as well.
  * Sega Dreamcast disc images. Currently supports raw track 03 disc images
    in 2048-byte and 2352-byte sector formats, and GD-ROM cuesheets (\*.gdi).
  * Sega Saturn disc images. Currently supports raw track 01 disc images in
    2048-byte and 2352-byte sector formats.
  * Atari Lynx ROM images. Currently only supports headered images.

* New system features:
  * Nintendo DS: Detect "Mask ROM" separately from Secure Area encryption.
    This indicates if the $1000-$3FFF area is blank. This area cannot be read
    on hardware (or the method to read it is unknown), so it's always 0 bytes
    in dumped ROMs. The official SDK puts unknown pseudo-random data here, so
    DSiWare and Wii U VC SRLs will have non-zero data here.
  * Game Boy: "Game Boy Color" will now only be shown for GBC-exclusive ROMs
    instead of both GBC-exclusive and GBC-enhanced.
  * Nintendo 3DS: Display the game title that most closely matches the system
    language. This was already done for Nintendo DS titles, and is also being
    done for Nintendo Badge Arcade files, which use a language setup similar
    to Nintendo 3DS.

* Bug fixes:
  * Fixed decoding of BGR555 images. The lower part of the R channel was
    shifted the wrong amount, resulting in an incorrect Red value expansion.
  * (KDE4) rp_create_thumbnail(): Set the MIME type in the thumbnail file.  
  * Wii U: Make sure GameCube and Wii magic numbers aren't present in the
    disc header. This ensures that a GCN/Wii disc image with the game ID
    "WUP-" doesn't get detected as a Wii U disc image.
  * Fixed CIAReader failing to compile in no-crytpto builds.
  * (Windows) The icon in the property sheet page is now scaled instead of
    vertically cropped if it's larger than 32x32.
  * (Windows) The first text field on the properties page is no longer
    automatically selected when the page is first selected.
  * Nintendo 3DS:
    * Fixed detection of certain truncated CIA files.
    * Detect NDHT and NARC CIAs correctly.

* Other changes:
  * The PNG compression level is now set to the default value instead of 5.
    This corresponds to 6 with libpng-1.6.31 and zlib-1.2.11.
  * The system text conversion functions are now used for handling Latin-1
    (ISO-8859-1) encoding instead of our own conversion function. In particular,
    this means the C1 control characters (0x80-0x9F) are no longer invalid and
    will be converted to the corresponding Unicode code points.
  * When comparing byteswapped data to a constant, the byteswapping macro
    is now used on the constant instead of the variable. This is needed
    in order to byteswap the constant at compile time in MSVC builds.
    (gcc does this even if the byteswap is on the variable.)
  * Updated MiniZip to an unofficial fork: https://github.com/nmoinvaz/minizip
  * rp-config: Added an "About" tab.
  * Symbolic links are now resolved when searching for related files, e.g.
    pairs of *.VMI/*.VMS files for Dreamcast saves. For example, if you have
    `/dir/main.vmi` and `/dir/main.vms`, and a symlink `/save.vmi -> /dir/main.vmi`,
    the symlink will now be resolved and the VMS file will be loaded. This feature
    is supported on Linux and Windows (Vista or later).
    * Note that Windows Explorer appears to automatically dereference symlinks
      when using IExtractImage, which is what ends up being used anyway because
      IThumbnailProvider doesn't provide a full path.
  * rp-config: The key manager can now import keys from Wii U otp.bin files.

## v1.1 (released 2017/07/04)

* New features:
  * (KDE) rp-config is now available in both the KDE4 and KDE5 frontends.
    This is based on the rp-thumbnail stub that was previously used for the
    GNOME frontend. Both of them are now part of a new executable, rp-stub.
  * (KDE, Windows) rp-config now has a Key Manager tab, which allows you
    to edit and verify encryption keys. There is also a key import function,
    which allows you to import keys from certain types of key files.
  * (XFCE) Thumbnailing is now supported using tumblerd's D-Bus specialized
    thumbnailer interface. (#39)

* New systems:
  * Nintendo 3DS firmware binaries are now supported. For official FIRM
    binaries, the CRC32 is compared against an internal database of known
    versions. This currently includes 1.0 through 11.4. For unofficial
    FIRM, various heuristics are used to determine if it's a well-known
    homebrew title.
  * Sega 8-bit: Initial reader for Sega 8-bit ROM images. Currently supports
    Sega Master System and Game Gear ROMs that have the "TMR SEGA" header,
    as well as Codemasters and SDSC extra headers.

* New system features:
  * Encrypted DSiWare CIAs are now supported, provided you have the keys
    set up in keys.conf. (Slot0x3D, KeyX, KeyY-0 through KeyY-5; or,
    Slot0x3D KeyNormal-0 through KeyNormal-5)
  * Nintendo 3DS: The logo section is now checked and displayed. Official
    Nintendo logos and a few Homebrew logos are supported. Anything else
    is listed as "Custom".
  * Mega Drive: A new "Vector Table" tab has been added. This tab shows
    the Initial SP and Entry Point, plus several other vectors.
  * Mega Drive: A second ROM Header tab for locked-on ROMs has been added.
    This shows the ROM header information for locked-on ROMs for e.g.
    "Sonic 3 & Knuckles".
  * PlayStation Saves: Save formats other than PSV (\*.mcb, \*.mcx, \*.pda,
    \*.psx, \*.mcs, \*.ps1, and raw saves) are now supported.
  * Nintendo 3DS: Application permissions are now listed for CIAs and CCIs.
    (Requires decrypted images or the appropriate encryption keys.)
  * EXE: MS-DOS executables now show more information.

* Bug fixes:
  * (GNOME) The .thumbnailer file was not packaged in the Ubuntu pre-built
    packages, which prevented thumbnailing from working.
  * Fixed issues with Shift-JIS titles in some PS1 save files. The title
    fields had garbage characters after the main title, with NULLs before
    and after the garbage characters. (#83)
  * (Windows) Property page: Fixed icon resizing for icons smaller than
    32x32. This only affects PS1 save files at the moment.
  * (GTK+) In the file properties dialog, animated icons are now paused
    when the tab isn't visible. This reduces CPU usage.

## v1.0 (released 2017/05/20)

* New systems supported:
  * Windows/DOS: Executables, dynamic link libraries, and other types of
    executable files are supported. Includes parsing of version and
    manifest resources. Icon thumbnailing on non-Windows systems will
    be added in a future release.
  * Nintendo Wii U: Full disc images (\*.wud) are supported, with image
    downloads for disc, cover, 3D cover, and full cover scans.
  * Nintendo 3DS: SMDH, 3DSX, CCI (\*.3ds), CIA, and NCCH files are supported.
    Parts of some formats (CCI, CIA, NCCH) may require decryption keys.
    If the keys are not available, then some information will not be
    available.

* New features:
  * Property page viewers now support subtabs. This is used for Windows
    executables that contain version and manifest resources.
  * JPEG is now supported for image downloads from external image databases.
    GameTDB uses JPEG for certain image types, including Nintendo DS covers.
  * Added support for downloading Nintendo DS, GameCube, and Wii cover scans.
    This includes cover and full cover scans for all three, and 3D cover scans
    for GameCube and Wii.
  * Multiple image sizes are now supported for external image downloads.
    GameTDB has higher-resolution scans for certain image types, including
    Nintendo DS cover scans. These high-resolution scans are used if a larger
    thumbnail size is requested by the file browser. An option to disable
    downloading of high-resolution images is available in the user config file.
  * (Windows) Physical block devices are now supported. This allows viewing
    ROM Properties for certain types of physical media, e.g. Wii DVD-R backups.
    Currently only the property page is supported. Thumbnails (and icons)
    are not supported for block devices.
  * Nintendo DS: Slot-2 PassMe ROM images are now recognized.
  * (rpcli) New option "-k". This option will verify all known keys in
    keys.conf. Verification is done by decrypting a string that was encrypted
    with the original key and checking if the decrypted string is correct.
  * GameCube: Added partial support for WIA disc images. A copy of the disc
    header is stored in plaintext in WIA format, so disc header fields and
    external images are supported. Region code, age ratings, and the internal
    GCN banner are not supported.
  * (Windows) A configuration program, rp-config.exe, is now included. This
    allows you to configure the image type priority for thumbnails as well
    as download options. Linux versions will be added in a future release,
    though the underlying rom-properties.conf functionality is implemented
    and works on all platforms.
  * (Windows) A new installation program, svrplus, is provided. This program
    supercedes install.cmd and uninstall.cmd.
  * (rpcli) New option "-c". This option prints the system region codes.
    This is mostly useful for debugging.

* Bug fixes:
  * Fixed an inverted "Copy Protected" condition for Dreamcast VMI files.
  * Fixed age ratings not showing up for Japanese Nintendo DSi and Wii games.
  * Fixed access to files on Wii partitions located past 4 GB. This applies
    to logical addresses in sparse formats, e.g. WBFS.
  * Wii: The "Game Info" field may be two separate lines.
  * (GNOME) The libnautilus-extension path is no longer hard-coded to
    /usr/lib64/. This prevented it from working correctly on anything but
    64-bit Linux systems that used the older multilib path, which means
    the Ubuntu GNOME packages did not work.
  * (Windows) Fixed a crash if tinyxml2.dll is missing and a Windows EXE
    with an embedded manifest resource is selected. Also improved the
    Delay-Load function to use architecture-specific subdirectories.

* Other changes:
  * (Windows) The 32-bit EXEs have been moved out of the i386/ subdirectory
    and into the base directory. The 64-bit EXEs are still in the amd64/
    subdirectory.

## v0.9-beta2 (released 2017/02/07)

* New features:
  * New GTK+ frontends for XFCE's Thunar file manager (using GTK+ 2.x)
    and GNOME's Nautilus file manager (using GTK+ 3.x). These currently
    only implement the property page. Thumbnailing for Nautilus has been
    implemented. Thumbnailing for Thunar will be added later.
    * While the GTK+ frontends share code, they are packaged separately in
      order to reduce dependencies on GNOME-only and XFCE-only systems.
  * Negative image caching for online databases (that is, caching the "this
    image does not exist" result) now expires after one week. This allows
    the image to be retrieved if it has since been uploaded to the database
    without manually clearing the local cache.
  * New command line frontend `rpcli`. This frontend lists the ROM information
    that would normally be displayed on the property page. It also has options
    for extracting internal images and downloading external images.
  * The Windows version now registers for "common" file extensions, e.g. ".bin"
    and ".iso". The previous handlers are saved as fallbacks. If rom-properties
    cannot handle one of these files, the fallback handler is used.

* New systems supported:
  * Nintendo 64 ROM images: Z64, V64, swap2, and LE32 byteswap formats.
    Currently only supports text fields.
  * Super NES ROM images: Headered and non-headered images.
    Currently only supports text fields.
  * Sega Dreamcast save files: .VMS, .VMI, .VMS+.VMI, and .DCI formats.
    Supports text fields, icons, and the graphic eyecatch (as the banner).
    ICONDATA_VMS files are also supported.
  * Nintendo Virtual Boy ROM images: .VB format.
  * Nintendo amiibo NFC dumps: 540-byte .bin files. (On Windows, the .bin
    extension is not currently registered; alternatives are .nfc and .nfp)
  * Nintendo Entertainment System: iNES, FDS, QD, and TNES/TDS (3DS Virtual
    Console) formats are supported.

* Changes to existing systems:
  * GameCube: Fixed accidental swapping of Triforce and Wii system names.
  * GameCube: Some save files have an embedded NULL byte in the description
    field. This caused the description to get truncated. This case is now
    handled. (Example: "Baten Kaitos Origins" save files.)
  * Wii: Added support for RVT-R debug discs. The encryption key used for
    each partition is now displayed in the partition listing.
  * Wii: Show the used size of each partition in addition to the total size.
  * Wii: Show the game title from opening.bnr.
  * Game Boy Advance: Some ROM images that are intended for use as expansion
    packs for Nintendo DS games weren't recognized because they don't have
    the Nintendo logo data. These ROM images are now detected and marked
    as non-bootable Nintendo DS expansions.
  * Game Boy Advance: Fixed the entry point. The value was off by 8 bytes
    due to the way branches are handled in the ARM pipeline. The branch
    offset is also signed, so it could be negative (though this is unlikely).
  * Nintendo DS: Fixed an issue where the first frame of DSi animated icons
    was not selected correctly. (Example: "Four Swords Anniversary Edition".)
  * Nintendo DS: Show age ratings for DSi-enhanced and DSi-exclusive games.

* Other changes:
  * (Windows) Fixed anti-aliasing issues with monospaced fonts on the
    properties page.
  * (Windows) Fixed an incorrect half-pixel offset with some resized
    thumbnails, which resulted in the thumbnails "shifting" when Explorer
    switched from icons to thumbnails.
  * libpng is now used on all platforms. Previously, GDI+'s PNG loader was
    used on Windows, and it had some odd quirks like loading grayscale images
    as 32-bit ARGB instead of paletted.
  * (Windows) zlib and libpng are now compiled as DLLs instead of being
    statically linked. The DLLs are linked using delay-load, so they're
    not loaded until they're needed.
  * pngcheck: Fixed a race condition that could result in crashes if more
    than one thread attempted to load a PNG image at the same time.
  * Age ratings for CERO, ESRB, and AGCB are now converted to their official
    names instead of being displayed as numbers.

## v0.8.1 (Windows only) (released 2016/10/24)

* RP_ExtractIcon: Disabled icon caching. This was causing issues where
  all icons for supported ROM images were showing up as whichever file
  was the first to be processed.

## v0.8-winfix (Windows only) (released 2016/10/23)

* Fixed the working directory in the install.cmd and uninstall.cmd
  scripts. Windows switches to C:\\Windows\\System32 by default when
  running a program or batch file as Administrator, which prevented
  the installation scripts from finding the DLLs.

## v0.8-beta1 (released 2016/10/23)

* First beta release.

* Host platforms: Windows XP and later, KDE 4.x, KDE 5.x

* Target systems: Mega Drive (and add-ons), Nintendo DS(i), Nintendo
  GameCube (discs and save files), Nintendo Wii, Game Boy (Color),
  Game Boy Advance.

* Internal images supported for GameCube discs and save files, and
  Nintendo DS ROMs.

* Animated icons supported for GameCube save files and Nintendo DSi
  ROMs. Note that file browsers generally don't support animated icons,
  so they're only animated on the file properties page.

* External image downloads supported for GameCube and Wii discs
  via [GameTDB.com](http://www.gametdb.com/).

* Decryption subsystem for Wii discs. Currently only used to determine
  the System Update version if a Wii disc has a System Update partition.
  Keys are not included; you must manually add the encryption keys to
  ```keys.conf```. (See [README.md](README.md) for more information.)
