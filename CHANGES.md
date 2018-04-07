# Changes

## v1.3 - The Internationalization Release (released 201?/??/??)

* New features:
  * Most strings are now localizable. The gettext utilities and libintl
    library are used to provide localization support.
  * TODO: gettext/libintl is not currently provided for Windows.
  * TODO: Consider using boost::locale instead of libintl. Has the ability
    to select a different locale for rom-properties instead of using the
    system-wide locale, but has a 1 MB runtime library.
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

* New system features:
  * Super NES: Satellaview BS-X ROM headers are now decoded properly.
  * Wii: Unencrypted images from RVT-H Reader systems are now supported.
  * NES: Improved publisher lookup for FDS titles.
  * Wii U: Display game publishers.
  * DMG: Added more entry point formats.
  * DirectDrawSurface: Untested support for Xbox One textures.

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

* Other changes:
  * libromdata/ has been reorganized to use subdirectories for type of system.
    This currently includes "Console", "Handheld", "Texture", and "Other".

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
