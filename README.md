# ROM Properties Page shell extension

Work in progress; do not use unless you plan on providing feedback.

For feedback, visit the Gens/GS IRC channel: [irc://irc.badnik.zone/GensGS](irc://irc.badnik.zone/GensGS)

Or use the Mibbit Web IRC client: http://mibbit.com/?server=irc.badnik.zone&channel=#GensGS

## Quick Install Instructions

Currently, the ROM Properties Page shell extension is only compatible with KDE4.

On Ubuntu, you will need build-essential and the following development packages:
* libqt4-dev
* kdelibs4-dev

Clone the repository, then:
* cd rom-properties
* mkdir build
* cd build
* cmake .. -DCMAKE_INSTALL_PREFIX=/usr
* make
* sudo make install
* kbuildsycoca4 --noincremental

NOTE: KDE4 will not find the rom-properties plugin if it's installed in /usr/local/. It must be installed in /usr/.

After installing, restart Dolphin, then right-click an uncompressed Sega Mega Drive ROM image and click Properties. If everything worked correctly, you should see a "ROM Properties" tab that shows information from the ROM header.
