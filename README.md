# ROM Properties Page shell extension

This shell extension adds a few nice features to file browsers for managing
video game ROM and disc images.

[![License: GPL v2](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html)<br>
[![AppVeyor Build status](https://ci.appveyor.com/api/projects/status/5lk15ct43jtmhejs/branch/master?svg=true)](https://ci.appveyor.com/project/GerbilSoft/rom-properties/branch/master)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/10146/badge.svg)](https://scan.coverity.com/projects/10146)<br>
[![CodeQL](https://github.com/GerbilSoft/rom-properties/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/GerbilSoft/rom-properties/actions/workflows/codeql-analysis.yml)

## v2.1

![Right-click menu in Thunar showing the "Convert to PNG" option](doc/img/rp-v2.1-thunar.ConvertToPNG.png)

Major changes in v2.1 include:

* Support for GTK4 UI frontends, e.g. Nautilus 43. Note that Nautilus 43 has
  significantly limited the functionality of property pages, so it's only
  partially implemented at the moment.

* Right-click menu option for "Convert to PNG" for supported texture files,
  e.g. DDS and KTX.

* rp-config now has a built-in update checker on the "About" tab.

* KTX2: RG88 texture format and swizzling are now supported.

Translators needed; file an issue if you'd like to get started on a new
translation, or submit a Pull Request if you have a translation ready to go.

See [`NEWS.md`](NEWS.md) for a full list of changes in v2.1.

## Feedback

This is a work in progress; feedback is encouraged. To leave feedback, you
can file an issue on GitHub, or visit the Gens/GS IRC channel:
[irc://irc.badnik.zone/GensGS](irc://irc.badnik.zone/GensGS)

Or use the Mibbit Web IRC client: http://mibbit.com/?server=irc.badnik.zone&channel=#GensGS

## Installation

Currently, the ROM Properties Page shell extension is compatible with the
following platforms:
* KDE Frameworks 5.x
* XFCE (GTK+ 2.x, GTK+ 3.x)
* GNOME and Unity (GTK+ 3.x)
* MATE Desktop (1.18+; GTK+ 3.x)
* Cinnamon Desktop
* Windows 7 (and later)

The following platforms are still compatible, but may not receive as much support:
* KDE 4.x
* Windows XP, Windows Server 2003, Windows Vista

On Windows Vista and later, you will need the MSVC 2015-2022 runtime:
* 32-bit: https://aka.ms/vs/17/release/VC_redist.x86.exe
* 64-bit: https://aka.ms/vs/17/release/VC_redist.x64.exe

On Windows XP/2003 and earlier, you will need the MSVC 2015-2017 runtime:
* 32-bit: https://aka.ms/vs/15/release/VC_redist.x86.exe
* 64-bit: https://aka.ms/vs/15/release/VC_redist.x64.exe

For instructions on compiling from source, see [`doc/COMPILING.md`](doc/COMPILING.md).

### Linux

Install the relevant .deb package, depending on what desktop environment you
are using and what your CPU is.

After installing, the plugin needs to be enabled in the Dolphin file browser:
* Close all instances of Dolphin.
* Start Dolphin.
* Click Control, Configure Dolphin.
* Click the "General" section, then the "Preview" tab.
* Check the "ROM Properties Page" item, then click OK.
* Enable previews in a directory containing a supported file type.

If installed correctly, thumbnails should be generated for the supported
file type. You can also right-click a file, select Properties, then click
the "ROM Properties" tab to view more information about the ROM image.

### Windows

Extract the ZIP archive to a directory, then run install.exe. The installer
requires administrator access, so click "Yes" if requested. In the installer,
click the "Install" button to register the ROM Properties Page DLL.

Note that this will hard-code the location of the DLL files in the registry,
so you may want to place the DLLs in a common location.

To uninstall the plugin, run install.exe again, then click the "Uninstall"
button.

## Current File Support Level

### Game Consoles

|           System          | Properties Tab | Metadata | Internal Images | External Images |
|:-------------------------:|:--------------:|:--------:|:---------------:|:---------------:|
| Commodore 64/128 .CRT     |       Yes      |    Yes   |       N/A       |      Title      |
| iQue Player ticket files  |       Yes      |    Yes   |   Icon, Banner  |        No       |
| Microsoft Xbox (XBE)      |       Yes      |    Yes   |       Icon      |        No       |
| Microsoft Xbox 360 (XEX)  |       Yes      |    Yes   |       Icon      |        No       |
| Microsoft Xbox 360 (STFS) |       Yes      |    Yes   |       Icon      |        No       |
| Microsoft Xbox Game Discs |       Yes      |    Yes   |       Icon      |        No       |
| NES                       |       Yes      |    No    |       N/A       |        No       |
| Super NES                 |       Yes      |    Yes   |       N/A       |      Title      |
| Nintendo 64               |       Yes      |    Yes   |       N/A       |        No       |
| Nintendo GameCube Discs   |       Yes      |    Yes   |      Banner     |   Disc, Covers  |
| Nintendo GameCube Banners |       Yes      |    Yes   |      Banner     |        No       |
| Nintendo GameCube Saves   |       Yes      |    Yes   |       Icon      |       N/A       |
| Nintendo Wii Discs        |       Yes      |    Yes   |        No       |   Disc, Covers  |
| Nintendo Wii WADs         |       Yes      |    Yes   |       Yes*      |  Title, Covers  |
| Nintendo Wii Saves        |       Yes      |    No    |       Yes       |       N/A       |
| Nintendo Wii U            |       Yes      |    No    |        No       |   Disc, Covers  |
| Sega 8-bit (SMS, GG)      |       Yes      |    Yes   |       N/A       |        No       |
| Sega Mega Drive           |       Yes      |    Yes   |       N/A       |      Title      |
| Sega Dreamcast            |       Yes      |    Yes   |      Media      |        No       |
| Sega Dreamcast Saves      |       Yes      |    Yes   |   Icon, Banner  |        No       |
| Sega Saturn               |       Yes      |    Yes   |       N/A       |        No       |
| Sony PlayStation Discs    |       Yes      |    No    |       N/A       |        No       |
| Sony PlayStation EXEs     |       Yes      |    No    |       N/A       |       N/A       |
| Sony PlayStation Saves    |       Yes      |    Yes   |       Icon      |       N/A       |
| Sony PlayStation 2 Discs  |       Yes      |    Yes   |       N/A       |        No       |

\* Internal images are only present in Wii DLC WADs.<br>
\* Sega Mega Drive includes Sega CD, 32X, and Pico.

### Handhelds

|             System            | Properties Tab | Metadata | Internal Images | External Images |
|:-----------------------------:|:--------------:|:--------:|:---------------:|:---------------:|
| Atari Lynx                    |       Yes      |    No    |       N/A       |        No       |
| Bandai WonderSwan (Color)     |       Yes      |    Yes   |       N/A       |      Title      |
| Neo Geo Pocket (Color)        |       Yes      |    Yes   |       N/A       |      Title      |
| Nintendo Game Boy (Color)     |       Yes      |    Yes   |       N/A       |      Title      |
| Nintendo Virtual Boy          |       Yes      |    No    |       N/A       |        No       |
| Nintendo Game Boy Advance     |       Yes      |    Yes   |       N/A       |      Title      |
| Nintendo DS(i)                |       Yes      |    Yes   |       Icon      |   Covers, Box   |
| Nintendo DSi TADs*            |     Partial    |    No    |        No       |        No       |
| Nintendo 3DS                  |       Yes      |    Yes   |       Icon      |   Covers, Box   |
| Pokémon Mini                  |       Yes      |    Yes   |       N/A       |        No       |
| Sony PlayStation Portable     |       Yes      |    Yes   |       Icon      |        No       |
| Tiger game.com                |       Yes      |    Yes   |       Icon      |        No       |

\* The Nintendo DSi TAD parser supports development TADs that are normally
   imported using DSi Nmenu. It does not currently support DSi exports from
   retail systems.<br>
\* The PSP parser supports both PSP game and UMD video discs, as well as
   several compressed disc formats: CISOv1, CISOv2, ZISO, JISO, and DAX.

### Texture Formats

|      Texture Format      | Properties Tab | Metadata | Internal Images | External Scans |
|:------------------------:|:--------------:|:--------:|:---------------:|:--------------:|
| ASTC container           |       Yes      |    Yes   |      Image      |       N/A      |
| Godot 3,4 .stex          |       Yes      |    Yes   |      Image      |       N/A      |
| Leapster Didj .tex       |       Yes      |    Yes   |      Image      |       N/A      |
| Khronos KTX              |       Yes      |    Yes   |      Image      |       N/A      |
| Khronos KTX2             |       Yes      |    Yes   |      Image      |       N/A      |
| Microsoft DirectDraw DDS |       Yes      |    Yes   |      Image      |       N/A      |
| Microsoft Xbox XPR       |       Yes      |    Yes   |      Image      |       N/A      |
| PowerVR 3.0.0            |       Yes      |    Yes   |      Image      |       N/A      |
| Sega PVR/GVR/SVR         |       Yes      |    Yes   |      Image      |       N/A      |
| TrueVision TGA           |       Yes      |    Yes   |      Image      |       N/A      |
| Valve VTF                |       Yes      |    Yes   |      Image      |       N/A      |
| Valve VTF3 (PS3)         |       Yes      |    Yes   |      Image      |       N/A      |

#### Texture Codecs

* Assorted linear RGB formats, including 15-bit, 16-bit, 24-bit and 32-bit per pixel.
  * Most of these formats have SSE2 and/or SSSE3-optimized decoders.
  * RGB9_E5 is supported, though it is currently converted to ARGB32 for
    display purposes. The decoder is also slow. (Contributions welcome.)
* Dreamcast: Twiddled and Vector-Quantized
* Nintendo DS: Tiled CI8 with BGR555 palette
* Nintendo 3DS: Tiled and twiddled RGB565
* GameCube: Tiled RGB5A3 and CI8 with RGB5A3 palette
* S3TC: DXT1, DXT2, DXT3, DXT4, DXT5, BC4, and BC5 codecs.
  * GameCube 2x2-tiled DXT1 is supported in GVR texture files.
* ETCn: ETC1, ETC2 (RGB, RGBA, RGB_A1), EAC
* BC7: Supported in multiple texture file formats.
  * The implementation is somewhat slow. (Contributions welcome.)
* PVRTC: Supported in multiple texture file formats.
  * PVRTC-II: Partially supported. The hard transition flag and images
    that aren't a multiple of the tile size are not supported.
* ASTC: Supported in multiple texture file formats.

### Audio Formats

|             System            | Properties Tab | Metadata | Internal Images | External Scans |
|:-----------------------------:|:--------------:|:--------:|:---------------:|:--------------:|
| Atari 8-bit SAP audio         |       Yes      |    Yes   |       N/A       |       N/A      |
| Atari ST SNDH audio           |       Yes      |    Yes   |       N/A       |       N/A      |
| CRI ADX ADPCM                 |       Yes      |    Yes   |       N/A       |       N/A      |
| Commodore 64 SID Music        |       Yes      |    Yes   |       N/A       |       N/A      |
| Game Boy Sound System         |       Yes      |    Yes   |       N/A       |       N/A      |
| Game Boy Ripped               |       Yes      |    N/A   |       N/A       |       N/A      |
| Nintendo 3DS BCSTM and BCWAV  |       Yes      |    Yes   |       N/A       |       N/A      |
| Nintendo Sound Format         |       Yes      |    Yes   |       N/A       |       N/A      |
| Nintendo Wii BRSTM            |       Yes      |    Yes   |       N/A       |       N/A      |
| Nintendo Wii U BFSTM          |       Yes      |    Yes   |       N/A       |       N/A      |
| Portable Sound Format         |       Yes      |    Yes   |       N/A       |       N/A      |
| Super NES SPC Format          |       Yes      |    Yes   |       N/A       |       N/A      |
| Video Game Music              |       Yes      |    Yes   |       N/A       |       N/A      |

### Other

|             System             | Properties Tab | Metadata | Internal Images | External Scans |
|:------------------------------:|:--------------:|:--------:|:---------------:|:--------------:|
| Executable and Linkable Format |       Yes      |    No    |       N/A       |       N/A      |
| ISO-9660 Disc Images           |       Yes      |   Yes    |        No       |       N/A      |
| PUC Lua binaries               |       Yes      |    No    |       N/A       |       N/A      |
| Mach-O Binaries                |       Yes      |    No    |       N/A       |       N/A      |
| Nintendo amiibo                |       Yes      |    No    |       N/A       |      Media     |
| Nintendo Badge Arcade          |       Yes      |    No    |      Image      |       N/A      |
| Windows/DOS Executables        |       Yes      |    No    |        No       |       N/A      |

Notes:
* Internal image refers to artwork contained within the ROM and/or disc image.
  These images are typically displayed on the system's main menu prior to
  starting the game.
  * "N/A" here means the ROM or disc image doesn't have this feature.
  * "No" indicates that the feature is present but not currently implemented.
* External scans refers to scans from an external database, such as GameTDB.com
  for GameCube and Wii.
  * "No" indicates no database is currently available for this system.
  * Anything else indicates what types of images are available.
  * For amiibo, "media" refers to the amiibo object, which may be a figurine,
    a card, or a plush.
* Windows executables may contain multiple icon sizes. Support for Windows icons
  will be added once support for multiple image sizes is added.
* Sega 8-bit only supports ROM images with a "TMR SEGA" header.

An initial configuration program is included with the Windows version of
rom-propreties 1.0. This allows you to configure which images will be used for
thumbnails on each system. The functionality is available on Linux as well, but
the UI hasn't been ported over yet. See `doc/rom-properties.conf.example` for
an example configuration file, which can be placed in `~/.config/rom-properties`.

## External Media Downloads

Certain parsers support the use of external media scans through an online
database, e.g. GameTDB.com. This is enabled by default, but you can customize
which scans are downloaded for which systems by running the configuration
program, `rp-config.exe`.

Downloaded images are cached to the following directory:
* Linux: `~/.cache/rom-properties/`
* Windows XP: `%LOCALAPPDATA%\rom-properties\cache`
* Windows Vista+: `%USERPROFILE%\AppData\LocalLow\rom-properties\cache`

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

## Decryption Keys

Some newer formats, including Wii disc images, have encrypted sections. The
shell extension includes decryption code for handling these images, but the
keys are not included. To install the keys, create a text file called
`keys.conf` in the rom-properties configuration directory:

* Linux: `~/.config/rom-properties/keys.conf`
* Windows: `%APPDATA%\rom-properties\keys.conf`

The `keys.conf` file uses INI format. An example file, `keys.conf.example`,
is included with the shell extension. This file has a list of all supported
keys, with placeholders instead of the actual key data. For example, a
`keys.conf` file with the supported keys for Wii looks like this:

```
[Keys]
rvl-common=[Wii common key]
rvl-korean=[Wii Korean key]
```

Replace the key placeholders with hexadecimal strings representing the key.
In this example, both keys are AES-128, so the hexadecimal strings should be
32 characters long.

NOTE: If a key is incorrect, any properties dialog that uses the key to
decrypt data will show an error message instead of the data in question.

### Nintendo DS Secure Area

To encrypt or decrypt the Nintendo DS Secure Area, the Blowfish key must be
named `nds-blowfish.bin` and placed in the same directory as `keys.conf`.

MD5sum: `c08c5afd9c6d9530817cd2033e3864d7`

## Unsupported File?

If you have a file that you believe should be supported by ROM Propeties, or
would like to see support added for a new type, file an issue on GitHub:
https://github.com/GerbilSoft/rom-properties/issues

## Credits

### Developers

* @GerbilSoft: Main developer.
* @DankRank: Contributor, bug tester.
* @CheatFreak: Bug tester, amiibo support.

### Translators

* @DankRank: Russian, Ukrainian
* @NullMagic2: Brazilian Portuguese

### Other Contributions

[This list is incomplete; if you think you should be listed here, file an issue.]

* @Tpot-SSL: Assistance with the game.com implementation.

### Websites

* [GBATEK](http://problemkaputt.de/gbatek.htm): Game Boy Advance, Nintendo DS,
  and Nintendo DSi technical information.
* [WiiBrew](http://wiibrew.org/wiki/Main_Page): Wii homebrew and reverse
  engineering. Used for Wii and GameCube disc and file format information.
* [GameTDB](http://www.gametdb.com/): Database of games for various game
  consoles. Used for automatic downloading of disc scans for Wii and GameCube.
* [Pan Docs](http://problemkaputt.de/pandocs.htm): Game Boy, Game Boy Color and
  Super Game Boy technical information.
* [Virtual Boy Programmers Manual](http://www.goliathindustries.com/vb/download/vbprog.pdf):
  Virtual Boy technical information.
* [Sega Retro](http://www.segaretro.org/Main_Page): Sega Mega Drive technical
  information, plus information for other Sega systems that will be supported
  in a future release.
* [PS3 Developer wiki](http://www.psdevwiki.com/ps3/) for information on the
  "PS1 on PS3" save file format.
* [Nocash PSX Specification Reference](http://problemkaputt.de/psx-spx.htm)
  for more information on PS1 save files.
* [amiibo.life](http://amiibo.life): Database of Nintendo amiibo figurines,
  cards, and plushes. Used for automatic downloading of amiibo images.
* [3dbrew](https://www.3dbrew.org): Nintendo 3DS homebrew and reverse
  engineering. Used for Nintendo 3DS file format information.
* [SMS Power](http://www.smspower.org): Sega 8-bit technical information.
* [Puyo Tools](https://github.com/nickworonekin/puyotools): Information on
  Sega's PVR, GVR, and related texture formats.
* [Badge Arcade Tool](https://github.com/CaitSith2/BadgeArcadeTool): Information
  on Nintendo Badge Arcade files.
* [Advanced Badge Editor](https://github.com/TheMachinumps/Advanced-badge-editor):
  Information on Nintendo Badge Arcade files.
* [HandyBug Documentation](http://handy.cvs.sourceforge.net/viewvc/handy/win32src/public/handybug/dvreadme.txt):
  Information on Atari Lynx cartridge format.
* [Khronos KTX File Format Specification](https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/):
  Information on the Khornos KTX texture file format.
* [Khronos KTX 2.0 File Format Specification](http://github.khronos.org/KTX-Specification/):
  Information on the Khornos KTX 2.0 texture file format.
* [Valve Texture Format](https://developer.valvesoftware.com/wiki/Valve_Texture_Format):
  Information on the Valve VTF texture file format.
* [nocash SNES hardware specifications](http://problemkaputt.de/fullsnes.htm):
  Super NES and Satellaview BS-X technical information.
* [Homebrew Development Kit for the Tiger game.com](https://github.com/Tpot-SSL/GameComHDK)
  by @Tpot-SSL: game.com technical information.
* [Low Level Bits: Parsing Mach-O Files](https://lowlevelbits.org/parsing-mach-o-files/)
  for Mac OS X Mach-O binary information.
* [Free60.org Wiki archive](https://free60project.github.io/wiki/) for Xbox 360
  technical information.
* [Xenia Project](https://github.com/xenia-project/xenia) for Xbox 360 technical information.
* [Xbox Dev Wiki](https://xboxdevwiki.net/) for Original Xbox technical information.
* [.XBE File Format by Caustik](http://www.caustik.com/cxbx/download/xbe.htm) for
  information about the Original Xbox .XBE executable format.
* [Cxbx-Reloaded](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded) for Original Xbox
  technical information, including XPR pixel formats.
* [iQueBrew](http://www.iquebrew.org/index.php?title=Main_Page) for iQue Player
  technical information.
* [maxcso](https://github.com/unknownbrackets/maxcso) for documentation on the
  compressed formats used by unofficial Sony PlayStation Portable disc image
  loaders.
* [Basis Universal](https://github.com/BinomialLLC/basis_universal) for the ASTC
  decoder, which was derived from the [Android Open Source Project](https://source.android.com/).
* [Godot Engine](https://github.com/godotengine/godot) for documentation on
  Godot's own .stex format.
* [Vulkan SDK for Android](https://arm-software.github.io/vulkan-sdk/_a_s_t_c.html)
  for the ASTC file format header.
* [NEZ Plug](http://nezplug.sourceforge.net/) for basic GBR specifications.
