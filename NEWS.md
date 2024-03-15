# Changes

## v2.4 (released 2024/03/??)

* New features:
  * IFUNC resolvers have been rewritten to use our own CPUID code again,
    which is more efficient because, among other things, it doesn't do
    string comparisons.

* New parsers:
  * PalmOS: Palm OS executables and resource files (.prc). Thumbnailing is
    supported (the largest and highest color-depth icon is selected).
    * Fixes #407: [Feature Request] Add support for Palm OS apps
      * Requested by @xxmichibxx.

* Bug fixes:
  * On Linux, rp-config now correctly detects KDE Plasma 6 and uses the
    KF6 version of the UI instead of the KF5 version.
  * Linux, armhf/aarch64: Add missing syscalls to the seccomp whitelist.
    This fixes unit tests in the Launchpad build system.
  * Windows: Fix a crash when viewing the ROM Properties tab through
    the Directory Opus file browser.
    * This regressed in v1.7.
      * Commit: cff90b5309a5bad29bdc4065abda743c32875ffa
      * [win32] RP_ShellPropSheetExt: Initial scrolling for data widgets.
    * Fixes #405: Crash when used inside Directory Opus
      * Reported by @Kugelblitz360.
  * Fixed several issues on big-endian systems, e.g. PowerPC.
    * DreamcastSave: Byteswapping was incorrectly enabled on little-endian
      instead of big-endian. This didn't have any actual effect on LE, since
      the le*_to_cpu() macros are no-ops on little-endian systems, but it
      resulted in failures on big-endian systems.
    * rp_image::swizzle_cpp(): Fix swizzling on big-endian systems.
    * ImageDecoderLinearTest: Fix 15-bit/16-bit tests.
  * Linux: Fix build on GTK+ 3.x versions earlier than 3.15.8.
    * gtk_popover_set_transitions_enabled() was added in GTK+ 3.15.8.
      It's now used for GTK+ versions [3.15.8, 3.21.5).
  * GTK: Fix the "Convert to PNG" context menu item not showing up.
    * Affects: v2.2.1 - v2.3

## v2.3 (released 2024/03/03)

* New features:
  * Extended attributes: On Linux, XFS attributes are now displayed if they're
    available and any are set. This includes the project ID for project quotas.
  * rpcli can now extract mipmap levels from supported texture files.
    * The following parsers support mipmaps: GodotSTEX, KhronosKTX, KhronosKTX2,
      PowerVR3, ValveVTF, and DirectDrawSurface.
    * The following parsers don't support mipmaps yet, but the format does:
      SegaPVR
  * ImageDecoderTest now has mipmap tests for various formats.
  * The GTK4 UI frontend now uses GdkTexture instead of GdkPixbuf, which has
    been deprecated. Cairo is also used for certain image transformations.
  * GTK4: Achievements are now checked when the Nautilus properties page is
    opened. There's no way to wait for the user to select the "ROM Properties"
    section, since NautilusPropertiesModel is an abstract model, not an actual
    GtkWidget.
  * Windows: Dark Mode is now supported on Windows 10 1809 and later in the
    installation program and rp-config.
    * Portions of the Dark Mode functionality were taken from:
      * win32-darkmode: https://github.com/ysc3839/win32-darkmode [MIT license]
      * TortoiseGit: https://gitlab.com/tortoisegit/tortoisegit/-/blob/HEAD/src/Utils/Theme.cpp [GPLv2]
      * Notepad++: https://github.com/notepad-plus-plus/notepad-plus-plus/tree/master/PowerEditor/src/WinControls [GPLv3]
    * Due to Notepad++ using GPLv3, any Windows builds that use Dark Mode
      will also be considered GPLv3.
    * Dark Mode is also partially supported in the properties pages when using
      tools such as StartAllBack, though it has some issues right now.
  * Sparse disc images, e.g. CISO and GCZ, are now handled by the RomDataFactory
    class instead of requiring each RomData subclass to handle it. This means
    that all supported sparse disc images can be used for any console.
    * This was originally implemented to support ZISO and PSP CISO for PS2 disc
      images, but it also allows unusual combinations like DAX and JISO for
      GameCube disc images.
    * Fixes #397: Could you add support for PS2 ISO's compressed to zso and cso?
      * Reported by @60fpshacksrock.
  * Linux: KDE Frameworks 6 is fully supported.
  * New Romanian translation (ro_RO), contributed by @ionuttbara.
  * New Italian translation (it_IT), contributed by MaRod92.

* New parsers:
  * Wim: Microsoft Windows Images, used by the Windows installer starting with
    Windows Vista. Contributed by @ecumber.
    * Pull requests: #391, #392
  * CBMDOS: Commodore DOS floppy disk images. Supports D64, D71, D80, D82, D81,
    D67, and (mostly supports) G64 and G71 images, plus GEOS file icons.
  * ColecoVision: ColecoVision ROM images. Supports reading the title screen
    message and copyright/release year from .col images, among other things.
    Requires a .col file extension due to lack of magic number.
  * Intellivision: Intellivision ROM images. Supports reading the game title
    and copyright year (if present), and some flags. Requires a .int or .itv
    file extension due to lack of magic number.

* New parser features:
  * DMG: MMM01 and MBC1M multicarts are now detected, and the internal ROM
    headers are shown in separate tabs.
    * For MMM01, the main ROM header is actually located at 0xF8000 or 0x78000,
      which means it was previously getting RPDB title screen information from
      one of the included ROMs instead of the menu ROM.
    * The title screen is now correctly showing the main menu.
    * 256 KiB M161 ROMs are not supported yet.
    * Fixes #394: GameBoy multicarts: MBC1M and MMM01
      * Requested by @DankRank.
  * SPC: Parse duration and export it as both field data and metadata.
  * SegaPVR: Handle GVR CI4 and CI8 textures. Note that these textures don't
    have a built-in palette (they expect a palette to be provided elsewhere),
    so a grayscale palette is used.
    * CI8 is untested due to a lack of test images. If you have any original
      (i.e. from a licensed game) GVR CI8 textures, please submit them in a
      GitHub issue.
  * Added mipmap support to the following texture format parsers:
    * DirectDrawSurface
    * KhronosKTX
  * EXE: Add CPU type 0x0601 for PowerPC big-endian. (MSVC for Mac)
    * Xbox 360 CPU is now "PowerPC (big-endian; Xenon)".
    * Fixes #396: PE machine value 0x0601 == PowerPC big-endian (classic Mac)
      * Reported by @Wack0.
  * DirectDrawSurface: Detect DXT5nm (normal map) and swizzle the Red and Alpha
    channels for proper display. Also show both sRGB and Normal Map attributes
    as set by various nVidia tools.
  * GameCube: DPF and RPF sparse disc images are now supported.

* Bug fixes:
  * Windows: Truncate ListView strings to a maximum of 259+1 characters. (259
    actual characters plus 1 NULL terminator.) The ListView control can only
    handle strings up to this limit; longer strings can trigger assertions.
    * Related to: PR #392
  * GTK: Add SOVERSIONs to all of the dlopen()'d library filenames. This fixes
    the UI frontends on Ubuntu systems that don't have the dev packages for
    the file browser extension libraries installed, e.g. libnautilus-extension-dev.
    * This was broken since rom-properties v1.6, i.e. when the various GTK3 UI
      frontends were merged into a single library.
  * Unix XAttr reader: Fix off-by-one that prevented a file's last attribute
    from being displayed.
  * Windows: Using any option in the "Options" menu aside from exporting or
    copying text or JSON would result in the option being disabled unless the
    properties window as closed or reopened.
    * This bug was introduced in v1.8.
  * Windows: Handle filenames with unpaired UTF-16 surrogate characters.
    * Fixes #390: Windows: Add UTF-16 filename functions to handle filenames
      with unpaired UTF-16 surrogate characters
  * DMGSpecialCases: Fix an incorrect bounds check for CGB special cases.
  * Windows: rp-config's AchievementsTab incorrectly showed timestamps in UTC.
    They are now displayed in the local timezone.
  * GTK4 rp-config: Fix an issue where the Key Manager tab didn't show the
    "Open" dialog properly.
  * Windows rp-config: Fix the Key Manager tab not showing up at all.
    * This regressed in v2.2.1.
  * Nintendo3DS: Added non-standard MIME types used by Citra.
    * See issue #382: Errors in KDE
      * This particular issue was diagnosed by @dnmodder.
  * KDE: Fixed dragging images from ListData views to the file browser. Note
    that Dolphin 23.04.3 has issues receiving drags when using Wayland, but
    it works when using X11.
    * This broke in Oct 2020 when the ListData views were reworked from
      QTreeWidget to QTreeView in order to implement sorting.
    * Affects: v1.8 - v2.2.1
  * GTK: Fixed dragging the main icon from RomDataView to the file browser.
    * Affects: v1.8 - v2.2.1
  * ImageDecoder: fromLinear32_resolve() might not have been selecting SSSE3
    in some cases because it was using RP_CPU_HasSSSE3() instead of gcc's
    __builtin_cpu_supports(), which isn't available during IFUNC resolution.
  * MegaDrive: Fix incorrect shift values that broke reporting for five I/O
    devices: Team Player, Light Gun, Activator, Tablet, Paddle
    * Affects: v2.0 - v2.2.1
  * NES: Fix NES 2.0 PRG RAM size. (non-battery-backed)
    * This was apparently broken since it was originally implemented in v1.1.
    * Affects: v1.1 - v2.2.1
  * Windows: Fix a memory leak when decrypting AES-CTR data (e.g. Nintendo 3DS
    ROM images) on Windows XP.
  * Fix a crash when decoding PNGs or other zlib-encoded data on Windows XP.
    * Affects: v2.2 - v2.2.1
  * NASOSReader: Fix detection of dual-layer Wii NASOS images.
  * GameCubeSave: Allow files with no icon or comment. (address == 0xFFFFFFFF)
    * Reported by RedBees.
  * GameCube: The "Region-Free" region code isn't valid for GCN; only Wii.
    * ...unless certain debugging hardware is connected.
    * Fixes #400: Wrong region listed on a GameCube ISO?
      * Reported by @loser2023sgyt.
  * GTK UI frontend: Fix "standard sorting" in RFT_LISTDATA fields.
    * GTK3 defaults to case-insensitive sorting, which doesn't match our
      assumptions that "standard sorting" is case-sensitive.
  * KDE: XAttrView didn't show up on KF5 older than 5.89.0.

* Other changes:
  * Nintendo3DS: The "Options" menu no longer shows a grayed-out "Extract SRL"
    option if the file does not contain an embedded SRL.
  * Consolidated the Linux and FreeBSD XAttr readers into a single file with
    a few #ifdef's. Partially based on KIO's FileProtocol::copyXattrs().
    * Also partially supports macOS.
  * Windows: Better error reporting if a file could not be opened.
    * For example, if a file is missing, it now shows "No such file or directory"
      instead of just "Input/output error".
    * This bug has been present since at least v1.4.
  * General: The custom reference counting implementation, RefBase, which was
    previously used by RomData, IRpFile, and many other base classes, has been
    removed in favor of std::shared_ptr<>. shared_ptr does have a bit more
    overhead for both code and RAM, but it significantly simplifies the code.
  * Windows: The "xattr" tab now respects the LC_ALL and/or LC_MESSAGES
    environment variable to allow for easier multi-language testing.
  * Windows: On ARM64 Windows 11, register for i386, arm64, and arm64ec.
    * Only if using build 21262 or later. (RTM is 22000)
    * On earlier versions, only arm64 will be registered, since ARM64EC
      was added at the same time as amd64 emulation.
    * Also, support for 32-bit ARM applications was dropped in build 25905.
    * Fixes #398: Installing on ARM64 shows an error that the AMD64 version of the DLL couldn't be registered
      * Reported by @kristibektashi.

## v2.2.1 (released 2023/07/30)

* New parser features:
  * EXE: Add IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION.
    * Fixes #388: PE subsystems list missing Windows Boot Application
      * Reported by @Wack0.

* Bug fixes:
  * JPEG images using the Exif container are now detected properly. This fixes
    loading certain cover art from the PS1/PS2 section of RPDB.
    * Fixes #386: Some ps2 games never get their boxart
      * Reported by @Masamune3210.
  * KDE: Fix an issue where achievements are checked when the properties dialog
    is opened instead of when the "ROM Properties" tab is selected.
  * GameCube: Un-break partial support for WIA/RVZ images.
    * This was broken in v2.2.
    * Fixes #389: Gamecube Property sheet is blank
      * Reported by @Masamune3210.
  * Windows: Don't square thumbnails for anything on Windows Vista and later.
    * On Windows XP, we'll still square the icons.
    * Thumbnails for images that were taller than they were wide appeared
      squished with squaring enabled, even though the generated image is
      correct. Not sure why this is happening...
    * Windows XP had issues with non-square icons, but Windows 7 seems to be
      handling them without any problems.
    * Fixes #385: Ratio of ps2 longbox thumbnails looks wrong?
      * Reported by @Masamune3210.
  * KDE: Fix metadata extraction. v2.2 added JSON plugin metadata to the
    forwarder plugins. Unfortunately, KFileMetaData uses a completely different
    format than other KDE plugins, which broke metadata extraction.
    * This was broken in v2.2.
  * NES: Update mappers; Fix detection of mapper 458, submappers 2 and 3.

* Other changes:
  * Preliminary support for Windows on ARM. The Windows distribution includes
    DLLs compiled for ARM, ARM64, and ARM64EC, and svrplus has been updated
    to register these DLLs if an ARM system is detected. Note that svrplus
    has *not* been tested on any ARM systems yet.

## v2.2 (released 2023/07/01)

* New features:
  * PlayStationDisc: Added external cover images using RPDB, mirrored from
    the following GitHub repositories:
    * https://github.com/xlenore/psx-covers
    * https://github.com/xlenore/ps2-covers
    * Fixes #371: PlayStation 1 and 2 covers
      * Reported by @DankRank.
  * Extended attribute viewer tab. Supports viewing MS-DOS and (on Linux)
    EXT2 attributes on supported file systems, plus POSIX extended attributes.
    * On Windows, alternate data streams are displayed as extended attributes.
    * Windows xattr functionality currently does not work on Windows XP.
  * KDE UI frontend now only uses JSON loading when compiled with KDE Frameworks
    5.89.0 or later.
  * GameCube: Display the Wii update partition date if available.
    * Requested by @johnsanc314.

* New parser features:
  * EXEData: Fix ordering of Alpha64 so binaries are detected properly.
    * Fixes #380: Alpha64 binaries are showing up as Unknown
      * Reported by @XenoPanther.

* Bug fixes:
  * WiiWAD: Add application/x-doom-wad for compatibility with some systems that
    assume all .wad files are Doom WADs.
  * WiiWAD: application/x-dsi-tad -> application/x-nintendo-dsi-tad
  * KDE: Revert RpQImageBackend registration removal in RomThumbCreator. This
    is needed when using KDE Frameworks 5.99 or earlier, since the
    ThumbCreator interface is not considered a KPlugin.
    * Fixes #372: KF5 frontend crashes when displaying thumbnails
      (rp_image_backend doesn't get initialized)
    * Merged #374: [kde] register RpQImageBackend when RomThumbCreator is used
      * Reported and submitted by @DankRank.
  * KDE: Fix split debug install paths.
    * Merged #373: [kde] Fix split debug install paths.
      * Submitted by @DankRank.
  * DirectDrawSurface: Fix detection of BC4S and BC5S, but disable signed
    texture decoding right now because it doesn't currently work.
  * DirectDrawSurface: Fix detection for some L8 and A8L8 textures, as well as
    some uncompressed textures that have a DXGI format set but not the legacy
    bitmask values.
  * Windows: The "Convert to PNG" context menu item now uses the system PNG
    file icon. Note that icon transparency is currently broken.
  * GodotSTEX: Update v4 support for changes in the final version of Godot 4:
    * PVRTC pixel formats have been removed.
    * ETCn no longer has swapped R/B channels.
    * Fix format flags not showing up at all.
  * GameCube: Show the raw version number of unrecognized Wii Menu versions,
    and fix the raw version number for 4.2K.
    * Reported by @johnsanc314.
  * GameCube: Handle standalone Wii update partitions containing Incrementing
    Values.
    * Reported by @johnsanc314.
  * DMG: Handle the 'H' cartridge HW type for IR carts. (HuC1, HuC3)
    * Merged #359: Update DMG.cpp
      * Reported by @MarioMasta64.
  * CacheTab: Allow deletion of "version.txt", used by the Update Checker.
  * Windows rp-config: Fix checkbox states when toggling external image
    downloads.

* Other changes:
  * EXE: Don't show import/export tables for .NET executables, since they only
    have a single import, MSCOREEE!_CorExeMain.
  * New unit test RomHeaderTest that runs `rpcli` against a set of known ROM
    headers and reference text and JSON output. Currently has headers for
    MegaDrive (including 32X and Pico), N64, NES, Sega 8-bit (SMS and GG),
    SNES, SufamiTurbo, DMG, and GameBoyAdvance.
  * libromdata's SOVERSION was bumped to 3 due to, among other things, the
    librptext split.

## v2.1 (released 2022/12/24)

* New features:
  * Initial support for Nautilus 43 (GTK4). Nautilus 43 reworked the property
    page interface so it only supports key/value pairs, so a lot of the
    functionality (lists, bitfields, images) isn't implemented at the moment.
    * Fixes #332: Compat with GTK4 and Nautilus 4.x
      * Reported by @Amnesia1000.
  * rp-config: The GTK, KDE, and Windows UI frontends now have an update checker
    in the About tab. When you click the About tab, it will compare the current
    version against the version listed on the RPDB server. If the current
    version is older, a link will be provided to the GitHub releases page.
  * Metered connections are now detected on Linux systems when using NetworkManager
    and on Windows 10 v2004 and later. rom-properties can be configured to either
    download lower-resolution images or no images when on metered connections,
    separately from the unmetered setting.
  * rp-stub now accepts a thumbnail size of 0 to indicate a "full-size thumbnail"
    is requested.
  * Added a right-click option for "Convert to PNG" for supported texture formats,
    e.g. DirectDraw Surfaces and Khronos KTX/KTX2.

* New parser features:
  * KhronosKTX2: Add support for RG88 textures.
  * KhronosKTX2: Textures with the KTXswizzle attribute set will have the color
    channels swizzled as specified. Includes SSSE3-optimized swizzling.

* Bug fixes:
  * ELF: The "TLS" symbol type was missing, resulting in an off-by-one
    error for some symbol types.
    * Merged #359: [libromdata] ELFPrivate::addSymbolFields(): fix the symbol type names
      * Submitted by @DankRank.
  * xdg: Install rp-config.desktop correctly.
    * Fixes #367: "rp-config.desktop" file error
      * Reported by @Amnesia1000.
  * Some text fields in certain ROMs that were documented as Latin-1
    contain C0/C1 control codes, which causes problems with rpcli.
    Handle these fields as cp1252 instead.
    * Fixes #365: Issues parsing some DS ROMs
      * Reported by @mariomadproductions.
  * [gtk] rp-config: Fix saving if the files don't exist initially.
    * Fixes #368: rp-config does not save configuration and keys in Manjaro Gnome.
      * Reported by @Amnesia1000.
  * [xdg] rom-properties.xml: Add magic strings for amiibo files.
    * Fixes #370: Amiibos without thumbnails (Gnome)
      * Reported by @Amnesia1000.
  * Thumbnail downscaling was never implemented. It's implemented now, so thumbnail
    caching should work better on some systems.
  * Thumbnail bilinear filtering should have been enabled for 8:7 aspect ratio
    correction, but it wasn't. It is now. This affects SNES and some Mega Drive
    thumbnails.
  * xdg: Add inode/blockdevice to re-enable physical CD/DVD support on Linux.
    * v2.0 removed application/octet-stream, which otherwise covered this use case.
  * [gtk] rp-config: Achievement unlock times were incorrectly shown in UTC. They're now
    shown in the local timezone.

* Other changes:
  * libromdata's SOVERSION was bumped to 2 due to an ABI change in
    librpfile's MemFile and VectorFile classes.
  * Bundled zlib-ng: Fix for systems with a CPU that supports AVX2 but
    an OS that doesn't, e.g. Windows XP.
    * Fixes #361: Crash in rp-config on Windows XP
    * Merged #362: zlib-ng: Check that the OS supports saving the YMM registers before enabling AVX2
      * Reported and submitted by @ccawley2011.

## v2.0 (released 2022/09/24)

* New features:
  * The configuration UI has been ported to GTK. Users of GTK-based
    desktops will no longer need to install the KDE4 or KF5 UI frontends
    in order to use rp-config.
  * libromdata is now compiled as a shared library on Windows and Linux.
    This means an extra DLL (romdata-1.dll) or SO (libromdata.so.1) will
    be included with the distribution. The advantage of splitting it out
    is that all the UI frontends are now significantly smaller.
    * WARNING: Binary compatibility between releases is NOT guaranteed.
      The SOVERSION will be incremented if the ABI is known to have broken
      between releases, but I can't guarantee that it will always remain
      compatible.
    * IFUNC resolvers now use gcc's built-in CPU flag functions because
      the regular rom-properties functions aren't available due to PLT
      shenanigans. IFUNC now requires gcc-4.8+ or clang-6.0+.
  * Many large string tables have been converted from arrays of C strings
    to a single string with an offset table using Python scripts. This
    reduces memory usage by eliminating one pointer per string, and it
    reduces the number of relocations, which improves startup time.
    * DirectDrawSurface formats were previously converted manually, but
      there were errors in the ASTC formats, so DDS has been converted to
      use a Python script and these errors are fixed.
  * rpcli: New option '-d' to skip RFT_LISTDATA fields with more than
    10 items. Useful for skipping e.g. Windows DLL imports/exports.

* New parsers:
  * Atari7800: Atari 7800 ROM images with an A78 header.

* New parser features:
  * ELF: OSABI 102 (Cell LV2) is now detected.
  * DirectDrawSurface: Support non-standard ASTC FourCCs.
  * GameCube, PSP: Add missing MIME types. Among other things, this fixes
    missing metadata for GameCube CISO files.
  * EXE: Detect hybrid COM/NE executables, i.e. Multitasking DOS 4.0's
    IBMDOS.COM.
  * EXE: List DLL exports and imports. (PE, NE)
    * Fixes #348: Add an dll import/export function tab?
      * Reported by @vaualbus.
    * Merged #349: EXE PE: Exports/Imports tabs
      * Submitted by @DankRank.
    * Merged #350: EXE NE: Entries tab
      * Submitted by @DankRank.
    * Merged #355: Follow-up fixes for ne-entries
      * Submitted by @DankRank.
    * Merged #356: EXE NE: imports tab
      * Submitted by @DankRank.
  * ELF: List shared library names and symbol imports.
    * Merged #357: ELF dynamic stuff and symbols
      * Submitted by @DankRank.
    * Merged #358: More columns in ELF symbol tables
      * Submitted by @DankRank.

* Bug fixes:
  * Lua: Fix a crash on Windows where systemName() sometimes returned
    an invalid pointer when using the GUI frontend.
  * Nintendo3DS:
    * Fixed NCCH detection for CFAs with an "icon" file but no ".code" file,
      which is usually the case for DLC CIAs.
    * Handle badly-decrypted NCSD/CCI images that don't set the NoCrypto flag.
      * https://github.com/d0k3/GodMode9/commit/98c1b25bb0ebb1e35a7387ee34714d5fcf4b29df
      * https://github.com/d0k3/GodMode9/issues/575
  * N64:
    * OS versions are major.minor, e.g. 2.0, not 20. In addition, there should
      not be a space between "OS" and the number, e.g. "OS2.0K".
      * Fixes #339, reported by @slp32.
    * Don't check the clock rate when checking the magic number.
      * Fixes detection of e.g. Star Fox 64 and Cruis'n USA, which have a
        value set for clock rate instead of using the system default.
      * Fixes #340, reported by @slp32.
    * Display the ROM header clock rate. If 0, "default" will be shown.
  * ELF: Some byteswaps were missing, which may have broken reading certain
    big-endian ELFs on little-endian and vice-versa.
  * Linux: Fix walking the process tree to detect the desktop environment.
    This wasn't working properly due to two variables being named `ret`.
    This has been broken since v1.7.
  * GameCube: Improve the text encoding heuristic for BNR1 metadata.
  * XboxXPR: Handle XPRs with non-power-of-two image dimensions. Forza
    Motorsport (2005) uses these textures extensively. Some textures don't
    have the usual image size fields filled in, resulting in no image; others
    have incorrect values, resulting in a broken image.
    * This was reported by Trash_Bandatcoot.
  * Linux: Don't attempt to fopen() a pipe or socket. This can hang, since
    fopen() doesn't pass O_NONBLOCK.
    * Fixes #351, reported by @IntriguingTiles.
  * Windows: Fixed a crash in Java applications when using Java's file open
    dialog in some cases, e.g. Ghidra.
    * Fixes #352, reported by @RibShark.
  * rpcli: Fix table widths when printing strings that contain Unicode code
    points over U+0080 and/or fullwidth characters.
    * Fixes #353, reported by @DankRank.
  * GameCubeSave: Trim trailing CRs. (TMNT Mutant Melee [GE5EA4])
  * EXE: Fix a segmentation fault when viewing 16-bit EXEs compiled using
    Visual Basic.
    * This broke in v1.9.
  * PlayStation: Fix detection of 2352-byte/2448-byte CD images.
    * This broke in v1.8.
    * Fixes #354, reported by @DankRank.

* Other changes:
  * Windows: DLL loading has been hardened by using LoadLibraryEx().
    * This is supported on Windows 8 and later with no adjustments.
    * On Windows Vista or Windows 7, this requires [KB2533623](https://support.microsoft.com/kb/2533623).
    * Not supported on Windows XP, so it will fall back to the previous
      DLL loading behavior.
  * Metadata: The Description property is now used for descriptions
    instead of the Subject property where available.
  * Linux: Improved startup notification support. Tested and working with
    KDE on X11 and Wayland, and GTK3/GTK4 on Wayland. GTK3/GTK4 on X11
    doesn't seem to be handling it properly, but that might just be an
    issue with it running on KDE instead of a GTK-based desktop environment.
  * Linux: The .desktop files no longer list application/octet-stream as a
    supported MIME type. The system MIME database should have proper entries
    for all supported files. For unsupported file, an XML file is included
    with custom MIME type definitions.

## v1.9 (released 2022/05/22)

* New features:
  * OpenMP can now be used to improve decoding performance for some image
    codecs, including ASTC and BC7.
  * A .desktop file has been added for rp-config on Linux systems, which
    adds it to the applications menu on most desktop environments.

* New parsers:
  * GodotSTEX: Godot 3 and 4 texture files. Supports most linear encodings,
    S3TC, BC4, BC5, RGTC, BC7, PVRTC-I, ETC1, ETC2, and ASTC. Note that
    ASTC isn't officially supported; the format value 0x25 is used by
    Sonic Colors Ultimate. (0x25 is used for a Basis Universal format in
    Godot 4.)
  * ASTC: ASTC texture format. This is a minimal header format for textures
    encoded using ASTC.
  * CBMCart: Commodore ROM cartridges, using VICE 3.0's .CRT format. Supports
    external title screen images for C64 and C128 cartridges.
  * Lua: PUC LUA binary format (.lub).

* New parser features:
  * Added ASTC decoding. All texture formats that support ASTC have been
    updated to allow decoding ASTC textures. (HDR is not supported, and
    the LDR decoder is rather slow.)
  * The GBS parser now partially supports the older GBR format.
  * GameCube: IOS version is now shown for Wii discs.
  * EXE: Parse timestamps in early NE executables.
  * Xbox_XBE: Display "Init Flags".
  * NES: Improved nametable mirroring display.

* New compressed texture formats:
  * EAC R11 and RG11, which uses ETC2's alpha channel compression for
    1-channel and 2-channel textures. Note that the channels are truncated
    from 11-bit to 8-bit for display purposes, and signed int versions
    might not be displayed correctly.

* Bug fixes:
  * EXE:
    * Improve runtime DLL detection in some cases.
    * Improve detection of certain EFI executables.
  * SNES: Fix detection of games that declare usage of the S-RTC chip
    in the ROM header. (sd2snes menu)
  * Windows: Don't edit registered Applications when adding icon handling
    registry keys. If a program is associated with an emulator filetype,
    that program's icon will be used for IExtractIcon, but the rom-properties
    thumbnail will still be used for thumbnail previews. This should fix
    some issues with Windows 8/10/11 file associations.
    * See issues: #241, #318, #319
  * Windows: Don't square images before returning them for IExtractImage and
    IThumbnailProviders. Icons are still squared for IExtractIcon, since
    Windows doesn't really like non-square icons.
  * PVRTC-I requires power-of-2 textures. We're currently using a
    slightly-modified PVRTC-I decoder for PVRTC-II as well, so we have to
    enforce power-of-2 textures for PVRTC-II for now.
  * DXT3 decoding now always uses the 4-color (c0 > c1) palette. Previously,
    I implemented this as (c0 ≤ c1) due to misleading documentation, which
    didn't work correctly, so I disabled it entirely. Implementing it as
    (c0 > c1) works correctly with the existing test images.
  * KDE: Work around a KIO issue where thumbnails with a stride not equal
    to width * bytespp are broken. Usually only shows up in 24-bit images
    with unusual widths, e.g. hi_mark_sq.ktx .

* Other changes:
  * Significantly improved the peformance of the RGB9_E5 decoder.

## v1.8.3 (released 2021/08/03)

* Bug fixes:
  * Fix a division by zero when reading PSP CISO/CSO images.
    This bug was reported by @NotaInutilis in issue #286.
  * Fix a crash in the BC7 and PVRTC image decoders if the image dimensions
    were not a multiple of the tile size.
  * rp-download: Allow clone3(), which is needed on systems that have
    by glibc-2.34.

* Other changes:
  * GTK+ frontends now use libgsound if available.
  * Experimental support for GTK4 has been added. It's not available by
    default (configuration is commented out in CMake), since none of the
    major file browsers support GTK4 yet.

## v1.8.2 (released 2021/07/19)

* Bug fixes:
  * rp-download: Allow the rt_sigprocmask() syscall. This is needed on
    Ubuntu 20.04 and later, and possibly other systems that use systemd.
    * This was reported by gold lightning.

## v1.8.1 (released 2021/07/19)

* Bug fixes:
  * Fix svrplus.exe not detecting the MSVC runtime at all.
    * Fixes #312, reported by @SCOTT0852.
  * Fix tinyxml2.dll packaging in the Windows build.
    * Fixes #313, reported by @ccawley2011.
  * Fix delay-loading of tinyxml2.dll when viewing Windows executables.
    * Fixes #313, reported by @ccawley2011.

* Other changes:
  * Updated the amiibo database to be current as of 2021/07/14.

## v1.8 (released 2021/07/18)

* New features:
  * An achievements system has been added. By viewing certain types of files
    or performing certain actions, achievements can be unlocked. Achievements
    can be viewed in rp-config.
  * Enabled sorting on all RFT_LISTDATA fields.
  * Improved automatic column sizing in RFT_LISTDATA fields on all platforms.
    * Windows: Significantly improved column sizing by overriding ListView's
      default sizing function, which doesn't work properly for strings that
      have multiple lines.
    * GTK+: Combined the icon/checkbox column with column 0.
  * CD-ROM: Added support for 2448-byte sector images. Currently supported by
    the generic ISO parser and PlayStationDisc. Support for other systems may
    be added later on, but subchannels generally aren't used on Sega Mega CD,
    Sega Saturn, Sega Dreamcast, or PlayStation Portable.
  * New Spanish translation (es_ES), contributed by @Amnesia1000.
  * KDE: rp-config now has a Thumbnail Cache tab, similar to the Windows
    version.
  * rp-config: The default language for images for PAL titles downloaded from
    GameTDB can now be set.

* New parsers:
  * WonderSwan: Bandai WonderSwan (Color) ROM images. Supports external title
    screens using RPDB.
  * TGA: TrueVision TARGA image format. Most encodings are supported; some
    have alpha channel issues. Thumbnailing is enabled on Windows but not
    Linux, since most Linux desktop environments support TGA.

* New parser features:
  * NGPC: Added external title screens using RPDB.
  * Xbox360_XDBF:
    * Added metadata extraction.
    * Added (partial) support for GPD files. Avatar Awards aren't parsed
      in GPD files at the moment.
  * Xbox360_XEX: System firmware XEXes use the XEX1 key. This is indicated
    using the "Cardea Key" flag.
    * Fixes #273, reported by @Masamune3210.
  * GameCom: Added support for RLE-compressed icons. "Game.com Internet"
    and "Tiger Web Link" are the only two titles known to use them.
    * Fixes #278, reported by @simontime.
  * MegaDrive:
    * Detect Teradrive ROMs. Some extensions are also detected, but are
      not displayed at the moment.
    * Handle the 'W' region code as used by EverDrive OS ROMs.
    * Significantly improved Sega Pico detection, including handling
      non-standard system names and region codes.
    * Added more I/O devices. The I/O device field is now a string instead
      of a bitfield, since most games only support one or two devices.
    * Handle some more unusual ROM headers, including "early" headers
      that have 32 bytes for titles instead of 48, and some incorrect
      region code offsets.
    * Support for external title screens has been added using RPDB.
    * Added metadata properties. Currently supports publisher and title.
      Domestic title is used if available; otherwise, export title is used.
    * Improved detection of Sega Pico ROM images.
  * Amiibo: Split the database out of the C++ code and into a database file.
  * Xbox360_STFS: Add two more file extensions for some Bethesda games:
    * Fallout: `.fxs`
    * Skyrim: `.exs`
    * Fixes #303, reported by @60fpshacksrock.
  * KhronosKTX, KhronosKTX2: Fix thumbnailing registration on Windows.
  * ISO:
    * Basic support for CD-i images. (PVD only)
    * Basic support for El Torito boot image detection. Currently only
      displays if x86 and/or EFI boot images are present, but not any
      specifics.

* Bug fixes:
  * GameCube: Detect incrementing values partitions in encrypted images.
    * Fixes #269, reported by @Masamune3210.
  * KDE:
    * Ensure the "Thumb::URI" value is urlencoded.
    * Fixed a massive file handle leak. Affects v1.5 and later.
  * Fixed a potential crash when loading an invalid PNG image.
  * Windows: Fixed a column sizing issue that caused XDBF Gamerscore columns
    to be too wide.
  * Xbox360_STFS: Fixed a crash that happened in some cases.
  * XboxDisc: Fix an edge case where XGD3 discs that have a video partition
    whose timestamp matches an XGD2 timestamp are not handled correctly.
    * Affects: "Kinect Rush a Disney Pixar Adventure" (4D5309B6)
    * Fixes #291, reported by @Masamune3210.
  * EXE: Don't show the "dangerous permission" overlay for Win32 executables
    that don't have a manifest.
  * NintendoDS:
    * Add "EN" fallback for external artwork from GameTDB.
    * Preserve the RSA key area for "cloneboot" when trimming ROMs.
  * Re-enabled localization for texture parsers. This was broken with the
    conversion to librptexture in v1.5.
  * rpcli: Fix a segfault with JSON output on MD ROMs that have empty
    string fields.
  * NES: Improved internal footer detection.

* Security Notes:
  * pngcheck-2.3.0 was previously used to validate PNG images before loading
    them with libpng. New security fixes for pngcheck were released in December
    2020 with the caveat that because the code was crufty and unmaintained,
    there may still be security issues. Because of this, pngcheck has been
    removed entirely. Other security hardening methods, such as running image
    decoders in a separate low-privilege process, may be implemented in the
    future.

## v1.7.3 (released 2020/09/25)

* Bug fixes:
  * GameCube: Fix incorrect GCZ data size checks.
    * This *really* fixes #262, reported by @Amnesia1000.

## v1.7.2 (released 2020/09/24)

* Bug fixes:
  * PlayStationDisc: Fix a crash when parsing discs that don't have SYSTEM.CNF
    but do have PSX.EXE.
    * Fixes #258, reported by @TwilightSlick.
  * GameCube: Register the ".gcz" and ".rvz" file extensions on Windows.
    * Fixes #262, reported by @Amnesia1000.

* Other changes:
  * Xbox360_XEX: Removed the "Savegame ID" field. This seems to be 0 in all
    tested games.
    * Fixes #272, reported by @Masamune3210.

## v1.7.1 (released 2020/09/21)

* Bug fixes:
  * Windows: Fixed a last-minute bug that caused field values in the property
    page to sometimes be completely invisible.
    * This bug was reported by @Whovian9369.

## v1.7 (released 2020/09/20)

* New features:
  * An "Options" button is now present in the property page. The Options
    button allows for exporting the ROM properties in text and JSON format,
    using the same mechanism as rpcli's output.
  * A new "ROM operations" subsystem allows for certain operations to be
    performed on supported ROM images. Currently, Nintendo DS supports
    trimming and untrimming ROMs, and encrypting and decrypting the Secure
    Area. These operations are available in the "Options" menu. Note that
    modifying the Secure Area requires having the Nintendo DS Blowfish key
    as `nds-blowfish.bin` in the ROM Properties configuration directory.
  * Nintendo3DS: New ROM operation for extracting SRLs from DSiWare CIAs.

* New parsers:
  * PSP: PlayStation Portable disc images. Supports both PSP game and UMD
    video discs in ISO, CISOv1, CISOv2, ZISO, JISO, and DAX formats.
    * PARAM.SFO and firmware updates aren't parsed yet.

* New parser features:
  * Dreamcast: Added metadata properties.
  * NintendoDS:
    * Use the full title in metadata properties if available.
    * Indicate the specific Security Data that's present.
  * Nintendo3DSFirm: Added some more homebrews and show sighax status.
  * DMG: Added support for ROMs with 512-byte copier headers.
  * WiiWAD: Preliminary support for Nintendo DSi TAD packages as used by
    DSi Nmenu. These have the same basic format as WADs, including ticket
    and TMD, but the contents are different.
  * WiiWAD: New ROM operation for extracting SRLs from DSi TAD packages.
  * EXE: Show the "dangerous permissions" overlay for Windows executables
    that have requestedExecutionLevel == requireAdministrator.

* Bug fixes:
  * PlayStationDisc:
    * Allow discs that lack SYSTEM.CNF if they have PSX.EXE.
    * Handle boot filenames with two backslashes.
  * KDE: Fix thumbnailing of files with '#' and/or '?' in their filenames.
  * Win32: Default InfoTip value incorrectly used PreviewDetails.
  * Win32: Register PreviewDetails and InfoTip per extension instead of
    using a ProgID. Using a ProgID breaks .cmd files on Windows 8.
    * This fixes issue #242, reported by @mariomadproductions.
  * DMG: SRAM bank size is 8 KB, not 16 KB.
    * This fixes issue #246, reported by @Icesythe7.
  * PlayStationDisc: Fix a file handle leak.
    * This fixes issue #247, reported by @Masamune3210.
  * IsoPartition, XDVDFSPartition: Fix some more file handle leaks.
    * This fixes issue #249, reported by @Masamune3210.
  * WiiPartition: Fixed a potential memory corruption issue when reading
    a partial block from an unencrypted disc image.
  * WiiWAD: Animated icons are now reported correctly by `rpcli`.
  * KDE: Fixed a memory leak in the overlay icon handler. This has been
    present since overlay icons were introduced in v1.4.
  * KDE: Split the .desktop files into one for ThumbCreator and one for
    KPropertiesDialog. This fixes e.g. Windows EXE thumbnailing, which
    is supported by another plugin on KDE, but not by rom-properties;
    however, rom-properties *does* support EXE for properties functions.
    With both ThumbCreator and KPropertiesDialog in one .desktop file,
    all MIME types were taken over by rom-properties for ThumbCreator,
    even if those types weren't supported.
  * XboxDisc: Fixed broken detection for some XGD3 discs, including
    "2014 FIFA World Cup".
    * This fixes issue #253, reported by @Masamune3210.

* Other changes:
  * Windows: MSVC 2012 is now the minimum required compiler version if
    compiling without test suites; MSVC 2015 with test suites.
  * Linux: A system-wide cache directory (/usr/share/rom-properties/cache)
    is now supported. The primary use case is for systems that sandbox the
    thumbnailing process, which prevents rom-properties from downloading
    files from the Internet and from using the ~/.cache/ directory.
  * Windows: Property dialogs now show vertical scrollbars if necessary.
    This partially fixes issue #204, reported by @InternalLoss.

## v1.6.1 (released 2020/07/13)

* Bug fixes:
  * Windows: Fixed a guaranteed crash when viewing the "ROM Properties" tab.
    Fixes issue #236.
  * Downloads: Make sure Unicode sequences are properly escaped.
  * Windows XP: Added a workaround for RPDB not connecting over https due to
    WinInet.dll not supporting SNI.

* Other changes:
  * Windows: The rp-download User-Agent string now includes the OS version
    number.

## v1.6 (released 2020/07/12)

* New parsers:
  * PlayStationDisc: New parser for PS1 and PS2 disc images.
  * PlayStationEXE: Basic parser for PS1 executables.

* New parser features:
  * Xbox360_XEX: Handle delta patches somewhat differently. We can't check the
    encryption key right now, so skip that check. Also skip reading the EXE
    and XDBF sections, since we don't have the full executable in a delta patch.
  * Xbox360_STFS: Partial support for handling the embedded default.xex and/or
    default.xexp file using the Xbox360_XEX class.
  * Added external title screen images using the official ROM Properties online
    database server.
    * Supported systems: Game Boy (original, Color, Advance), Super NES
  * Game Boy Advance: Added metadata properties for Title and Publisher.
    (Same as Game Boy and Game Boy Color.)
  * NES: Added more (unused) mappers for TNES format.
  * GameCube: Added support for more formats:
    * Split .wbfs/.wbf1 files
    * GCZ compressed images
    * RVZ compressed images (header only; similar to WIA)
  * SNES: Significantly improved detection for ROM images with headers that
    don't quite match the specification. Added SPC7110 and other custom chip
    detection.
  * SNES: Added metadata properties.
  * SufamiTurbo: New parser for Sufami Turbo mini-carts for Super Famicom.
    This could have been part of SNES, but the header is completely different
    and is always located at $0000.
  * ISO: Added support for High Sierra Format CDs.
  * Cdrom2352Reader: Mode 2 sectors are now read correctly.
  * WiiSave: Show permissions as if they're Unix-style permissions, and show
    the "No Copy from NAND" flag.
  * WiiU: Get the GameTDB region code for boxart from the game ID.
  * XboxPublishers: New publishers lookup class for Xbox and Xbox 360.
  * GameCube: Detect more types of standalone update partitions.

* Bug fixes:
  * WiiWAD: Fix DLC icons no longer working after updating CBCReader to update
    its internal position correctly, which was needed for Xbox 360 XDBF files.
  * RP_ExtractImage: Fix crash with dangling shortcut files. This bug was
    reported by @Midou36O.
  * Nintendo3DS: Show the issuer for CCI images. This was shown for CIAs, but
    we forgot to add it for CCI.
  * XboxDisc: Fixed an incorrect double-unreference when opening original Xbox
    ISO images that have a `default.xbe` file that isn't readable by the
    XboxXBE parser. (Issue #219; reported by @cfas1)
  * Nintendo3DS: CVer CIAs have an 8-byte meta section.
  * iQuePlayer: Fixed file type identification issues.
  * KhronosKTX: Fixed preview of textures with more than one array element.
  * GTK+ 3.x (Nautilus) UI frontends: Fixed a minor memory leak that leaked a
    file URI every time a File Properties dialog was opened.
  * D-Bus Thumbnailer (XFCE): Fixed an incorrect MimeTypes key that broke
    thumbnailing on systems that use tumblerd. This was broken starting
    with v1.4.
  * EXE: Fixed crashes for VFT_UNKNOWN and IMAGE_SUBSYSTEM_UNKNOWN.
    The latter case occurs with Wine's built-in DLLs.

* Other changes:
  * Split file handling and CPU/byteorder code from librpbase into two
    libraries: librpfile and librpcpu.
  * The MATE, Cinnamon, and XFCE (GTK+ 3.x) plugins have all been merged
    into the GNOME plugin, and it is now named "gtk3". Among other things,
    this makes it possible to use the Ubuntu 18.04 "Bionic" packages on
    Linux Mint 19.3, which has an Ubuntu 18.04 base but uses the GTK+ 3.x
    version of Thunar.
  * KeyManager: Added full support for Wii U devkit otp.bin files, as well as
    debug Korean and vWii keys.

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
