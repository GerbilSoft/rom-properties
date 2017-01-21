# Changes

## v0.9-beta2 (unreleased)

* New features:
  * New GTK+ frontends for XFCE's Thunar file manager (using GTK+ 2.x)
    and GNOME's Nautilus file manager (using GTK+ 3.x). These currently
    only implement the property page. Thumbnailing will be implemented
    before release.
    * While the GTK+ frontends share code, they are packaged separately in
      order to reduce dependencies on GNOME-only and XFCE-only systems.
  * Negative image caching for online databases (that is, caching the "this
    image does not exist" result) now expires after one week. This allows
    the image to be retrieved if it has since been uploaded to the database
    without manually clearing the local cache.
  * New command line frontend `rpcli`. This frontend lists the ROM information
    that would normally be displayed on the property page. It also has options
    for extracting internal images and downloading external images.

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
  * Nintendo Entertainment System: iNES, FDS, and TNES/TDS (3DS Virtual
    Console) formats are supported.

* Changes to existing systems:
  * GameCube: Fixed accidental swapping of Triforce and Wii system names.
  * GameCube: Some save files have an embedded NULL byte in the description
    field. This caused the description to get truncated. This case is now
    handled. (Example: "Baten Kaitos Origins" save files.)
  * Wii: Added support for RVT-R debug discs. The encryption key used for
    each partition is now displayed in the partition listing.
  * Game Boy Advance: Some ROM images that are intended for use as expansion
    packs for Nintendo DS games weren't recognized because they don't have
    the Nintendo logo data. These ROM images are now detected and marked
    as non-bootable Nintendo DS expansions.
  * Game Boy Advance: Fixed the entry point. The value was off by 8 bytes
    due to the way branches are handled in the ARM pipeline. The branch
    offset is also signed, so it could be negative (though this is unlikely).
  * Nintendo DS: Fixed an issue where the first frame of animated icons was
    not selected correctly. (Example: "Four Swords Adventures".)

* Other changes:
  * (Windows) Fixed anti-aliasing issues with monospaced fonts on the
    properties page.
  * libpng is now used on all platforms. Previously, GDI+'s PNG loader was
    used on Windows, and it had some odd quirks like loading grayscale images
    as 32-bit ARGB instead of paletted.
  * (Windows) zlib and libpng are now compiled as DLLs instead of being
    statically linked.
  * pngcheck: Fixed a race condition that could result in crashes if more
    than one thread attempted to load a PNG image at the same time.

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
