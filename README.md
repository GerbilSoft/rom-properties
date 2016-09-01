# ROM Properties Page shell extension

Work in progress; do not use unless you plan on providing feedback.

For feedback, visit the Gens/GS IRC channel: [irc://irc.badnik.zone/GensGS](irc://irc.badnik.zone/GensGS)

Or use the Mibbit Web IRC client: http://mibbit.com/?server=irc.badnik.zone&channel=#GensGS

## Quick Install Instructions

Currently, the ROM Properties Page shell extension is compatible with the
following platforms:
* KDE 4.x
* KDE Frameworks 5.x
* Windows XP (and later)

### Linux

On Ubuntu, you will need build-essential and the following development packages:
* All: libcurl-dev
* KDE 4.x: libqt4-dev, kdelibs5-dev
* KDE 5.x: qtbase5-dev, kio-dev

Clone the repository, then:
* cd rom-properties
* mkdir build
* cd build
* cmake .. -DCMAKE_INSTALL_PREFIX=/usr
* make
* sudo make install
* (KDE 4.x) kbuildsycoca4 --noincremental
* (KDE 5.x) kbuildsycoca5 --noincremental

NOTE: Neither KDE 4.x nor KDE 5.x will find the rom-properties plugin if it's
installed in /usr/local/. It must be installed in /usr/.

After installing, restart Dolphin, then right-click a supported ROM or disc image
and click Properties. If everything worked correctly, you should see a
"ROM Properties" tab that shows information from the ROM header.

### Windows

The Windows version requires one of the following compilers: (minimum versions)
* MS Visual C++ 2010 with the Windows 7 SDK
* gcc-4.5 with MinGW-w64

Clone the repository, then:
* cd rom-properties
* mkdir build
* cd build
* cmake ..
* make
* cd src\win32
* regsvr32 rom-properties.dll

Caveats:
* Registering rom-properties.dll hard-codes the full path in the registry. Moving the file will break the registration.
* If building with MSVC, you may need to specify -G "NMake Makefiles".
* CMake does not support building for multiple architectures at once. For Win64, a 64-bit build will work for Windows Explorer, but will not work in any 32-bit programs.

## Current OS Feature Support Level

|     Platform     | Properties Tab | Thumbnails | Icons |
|------------------|:--------------:|:----------:|:-----:|
| KDE 4.x          |       Yes      |     Yes    |  N/A  |
| KDE 5.x          |       Yes      |     Yes    |  N/A  |
| XFCE (Thunar)    |       No       |     No     |  N/A  |
| GNOME (Nautilus) |       No       |     No     |  N/A  |
| Windows          |       Yes      |     Yes    |  Yes  |

Notes:
* The KDE 4.x and 5.x plugins share most of the code. The only differences
  are in the plugin interface (due to automoc issues) and the .desktop file.
* The XFCE and GNOME file managers both use a similar interface for generating
  thumbnails, but with a different metadata file. Hence, once thumbnail support
  is implemented for one of them, it will also be implemented for the other.
* Windows supports separate icon and thumbnail handlers, which is why a
  separate "icon" feature is listed. Linux desktop environments generally
  use the file's MIME type to determine the icon, so custom icons are always
  implemented using the thumbnail interface.

## Current ROM Feature Support Level

|       System      | Properties Tab | Internal Images | External Scans |
|:-----------------:|:--------------:|:---------------:|:--------------:|
| Sega Mega Drive   |       Yes      |       N/A       |       No       |
| Nintendo DS(i)    |       Yes      |       Icon      |       No       |
| Nintendo GameCube |       Yes      |        No       |      Disc      |
| Nintendo Wii      |       Yes      |        No       |      Disc      |

Notes:
* Internal icon, banner, and media scan refers to artwork contained within
  the ROM and/or disc image. These images are typically displayed on the
  system's main menu prior to starting the game.
  * "N/A" here means the ROM or disc image doesn't have this feature.
  * "No" indicates that the feature is present but not currently implemented.
* External media and box scans refers to scans from an external database,
  such as GameTDB.com for GameCube and Wii.
  * "No" indicates no database is currently available for this system.
  * Anything else indicates what types of images are available.

There will eventually be a configuration window for setting which image
will be used for thumbnails (and icons on Windows).

## ROM Formats Supported

* Sega Mega Drive: Plain binary (\*.gen, \*.bin)
* Nintendo DS(i): Decrypted (\*.nds)
* Nintendo GameCube: 1:1 disc image (\*.iso, \*.gcm) [DiscEx-shrunken images work too]
* Nintendo Wii: 1:1 disc image (\*.iso, \*.gcm), WBFS disc image (\*.wbfs)

## External Media Downloads

Currently, two systems (GameCube and Wii) support the use of external media
scans through GameTDB.com. The current release of the ROM Properties Page shell
extension will always attempt to download images from GameTDB.com if thumbnail
preview is enabled and a valid GameCube or Wii disc image is present in the
current directory. An option to disable automatic downloads will be added in
a future version.

Downloaded images are cached to the following directory:
* Linux: `~/.cache/rom-properties/`
* Windows: `%LOCALAPPDATA%\rom-properties\cache`

The directory structure matches the source site, so e.g. a disc image of
Super Smash Bros. Brawl would be downloaded to
`~/.cache/rom-properties/wii/disc/US/RSBE01.png`. Note that if the download
fails for any reason, a 0-byte dummy file will be placed in the cache,
which tells the shell extension not to attempt to download the file again.
[FIXME: If the download fails due to no network connectivity, it shouldn't
do this.]

If you have an offline copy of the GameTDB image database, you can copy
it to the ROM Properties Page cache directory to allow the extension to
use the pre-downloaded version instead of downloading images as needed.

## Credits

* [GBATEK](http://problemkaputt.de/gbatek.htm): Game Boy Advance, Nintendo DS,
  and Nintendo DSi technical information. Used for ROM format information for
  those systems.
* [WiiBrew](http://wiibrew.org/wiki/Main_Page): Wii homebrew and reverse
  engineering. Used for Wii and GameCube disc format information.
* [GameTDB](http://www.gametdb.com/): Database of games for various game
  consoles. Used for automatic downloading of disc scans for Wii and GameCube.
