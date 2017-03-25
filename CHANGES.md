# Changes

## v1.0 (unreleased)

* New features:
  * Property page viewers now support subtabs. This is used for Windows
    executables that contain version and manifest resources.
  * JPEG is now supported for image downloads from external image databases.
    GameTDB uses JPEG for certain image types, including Nintendo DS covers.
  * Added support for downloading Nintendo DS, GameCube, and Wii cover scans.
    This includes cover and full cover scans for all three, and 3D cover scans
    for GameCube and Wii.
  * !!! Multiple image sizes are now supported for external image downloads.
    GameTDB has higher-resolution scans for certain image types, including
    Nintendo DS cover scans. These high-resolution scans are used if a larger
    thumbnail size is requested by the file browser. A user configuration
    option will be added later to disable high-resolution image downloads.
  * (Windows) Physical block devices are now supported. This allows viewing
    ROM Properties for certain types of physical media, e.g. Wii DVD-R backups.
    Currently only the property page is supported. Thumbnails (and icons)
    are not supported for block devices.
  * Nintendo DS: Slot-2 PassMe ROM images are now recognized.

* New systems supported:
  * Windows/DOS: Executables, dynamic link libraries, and other types of
    executable files are supported.
  * Nintendo Wii U: Full disc images (\*.wud) are supported, with image
    downloads for disc, cover, 3D cover, and full cover scans.

* Bug fixes:
  * Fixed an inverted "Copy Protected" condition for Dreamcast VMI files.
  * Fixed age ratings not showing up for Japanese Nintendo DSi and Wii games.
  * (GNOME) The libnautilus-extension path is no longer hard-coded to
    /usr/lib64/. This prevented it from working correctly on anything but
    64-bit Linux systems that used the older multilib path, which means
    the Ubuntu GNOME packages did not work.

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
    statically linked.
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
