# ROM Properties Page shell extension

Work in progress; do not use unless you plan on providing feedback.

For feedback, visit the Gens/GS IRC channel: [irc://irc.badnik.zone/GensGS](irc://irc.badnik.zone/GensGS)

Or use the Mibbit Web IRC client: http://mibbit.com/?server=irc.badnik.zone&channel=#GensGS

## Quick Install Instructions

Currently, the ROM Properties Page shell extension is compatible with the following platforms:
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

NOTE: Neither KDE 4.x nor KDE 5.x will find the rom-properties plugin if it's installed in /usr/local/. It must be installed in /usr/.

After installing, restart Dolphin, then right-click an uncompressed Sega Mega Drive ROM image and click Properties. If everything worked correctly, you should see a "ROM Properties" tab that shows information from the ROM header.
