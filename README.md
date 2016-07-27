# ROM Properties Page shell extension

Work in progress; do not use unless you plan on providing feedback.

For feedback, visit the Gens/GS IRC channel: [irc://irc.badnik.zone/GensGS](irc://irc.badnik.zone/GensGS)

Or use the Mibbit Web IRC client: http://mibbit.com/?server=irc.badnik.zone&channel=#GensGS

## Quick Install Instructions

Currently, the ROM Properties Page shell extension is compatible with the
following platforms:
* KDE 4.x
* KDE Frameworks 5.x

On Ubuntu, you will need build-essential and the following development packages:
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

## Current OS Feature Support Level

|     Platform     | Properties Tab | Thumbnails | Icons |
|------------------|:--------------:|:----------:|:-----:|
| KDE 4.x          |       Yes      |     Yes    |  N/A  |
| KDE 5.x          |       Yes      |     Yes    |  N/A  |
| XFCE (Thunar)    |       No       |     No     |  N/A  |
| GNOME (Nautilus) |       No       |     No     |  N/A  |
| Windows          |       No       |     No     |   No  |

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

|       System      | Properties Tab | Internal Icon | Internal Banner | Internal Media Scan | External Media Scan | External Box Scan |
|:-----------------:|:--------------:|:-------------:|:---------------:|:-------------------:|:-------------------:|:-----------------:|
| Sega Mega Drive   |       Yes      |      N/A      |       N/A       |         N/A         |          No         |         No        |
| Nintendo DS(i)    |       Yes      |      Yes      |       N/A       |         N/A         |          No         |         No        |
| Nintendo GameCube |       Yes      |       No      |        No       |         N/A         |          No         |         No        |
| Nintendo Wii      |       Yes      |       No      |       N/A       |         N/A         |          No         |         No        |

Notes:
* Internal icon, banner, and media scan refers to artwork contained within
  the ROM and/or disc image. These images are typically displayed on the
  system's main menu prior to starting the game.
* External media and box scans refers to scans from an external database,
  such as GameTDB.com for GameCube and Wii.

There will eventually be a configuration window for setting which image
will be used for thumbnails (and icons on Windows).

## ROM Formats Supported

* Sega Mega Drive: Plain binary (\*.gen, \*.bin)
* Nintendo DS(i): Decrypted (\*.nds)
* Nintendo GameCube: 1:1 disc image (\*.iso, \*.gcm) [DiscEx-shrunken images work too]
* Nintendo Wii: 1:1 disc image (\*.iso, \*.gcm), WBFS disc image (\*.wbfs)
