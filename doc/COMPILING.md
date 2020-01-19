# Compiling rom-properties

## Linux

On Debian/Ubuntu, you will need build-essential and the following development
packages:
* All: cmake libcurl-dev zlib1g-dev libpng-dev libjpeg-dev nettle-dev pkg-config libtinyxml2-dev libbsd-dev mesa-common-dev gettext
* KDE 4.x: libqt4-dev kdelibs5-dev
* KDE 5.x: qtbase5-dev qttools5-dev-tools extra-cmake-modules libkf5kio-dev libkf5widgetsaddons-dev libkf5filemetadata-dev
* XFCE (GTK+ 2.x): libglib2.0-dev libgtk2.0-dev libgdk-pixbuf2.0-dev libthunarx-2-dev
* XFCE (GTK+ 3.x): libglib2.0-dev libgtk-3-dev libcairo2-dev libthunarx-3-dev
* GNOME: libglib2.0-dev libgtk-3-dev libcairo2-dev libnautilus-extension-dev
* MATE: libglib2.0-dev libgtk-3-dev libcairo2-dev libcaja-extension-dev
* Cinnamon: libglib2.0-dev libgtk-3-dev libcairo2-dev libnemo-extension-dev

NOTE: libkf5kio-dev was called kio-dev prior to Ubuntu 18.04.

On Red Hat/Fedora, you will need to install "C Development Tools and Libraries"
and the following development packages:
* All: cmake libcurl-devel zlib-devel libpng-devel libjpeg-turbo-devel nettle-devel tinyxml2-devel libbsd-devel mesa-libGL-devel gettext
* KDE 4.x: qt-devel kdelibs-devel
* KDE 5.x: qt5-qtbase-devel qt5-qttools extra-cmake-modules kf5-kio-devel kf5-kwidgetsaddons-devel kf5-kfilemetadata-devel
* XFCE (GTK+ 2.x): glib2-devel gtk2-devel gdk-pixbuf2-devel Thunar-devel
* XFCE (GTK+ 3.x): glib2-devel gtk3-devel cairo-devel Thunar-devel
* GNOME: glib2-devel gtk3-devel cairo-devel nautilus-devel
* MATE: glib2-devel gtk3-devel cairo-devel caja-devel
* Cinnamon: glib2-devel gtk3-devel cairo-devel nemo-devel

NOTE: XFCE's Thunar file browser requires the Tumbler D-Bus daemon to be
installed in order to create thumbnails.
* Debian/Ubuntu: tumbler
* Red Hat/Fedora: tumbler

NOTE 2: Thunar 1.8.0 switched to GTK+ 3.x. This version is not currently
available in most distribution repositories. The Fedora RPM listed above
may change by the time it hits the repositories, or it may stay the same
if they decide not to allow both ThunarX2 and ThunarX3 to be installed
side-by-side.

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

### Building .deb Packages

You will need to install the following:
* devscripts
* debhelper

In order to build debug symbol packages, you will need:
* Debian: debhelper >= 9.20151219
* Ubuntu: pkg-create-dbgsym

In the top-level source directory, run this command:
* `debuild -i -us -uc -b`

Assuming everything builds correctly, the .deb packages should be built in
the directory above the top-level source directory.

## Windows

The Windows version requires one of the following compilers: (minimum versions)
* Microsoft Visual C++ 2010 with the Windows 7 SDK
* gcc-4.5 with MinGW-w64
  * The MinGW build is currently somewhat broken, so MSVC is preferred.
    (The property page icon doesn't show up sometimes for Nintendo DS
     ROMs, and XP theming doesn't work because MinGW-w64 doesn't support
     isolation awareness for COM components.)

You will also need to install [CMake](https://cmake.org/download/), since the
project uses the CMake build system.

Clone the repository, then open an MSVC or MinGW command prompt and run the
following commands from your rom-properties repository directory:

MSVC 2010-2017:
```
mkdir build
cd build
cmake .. -G "Visual Studio 15 2017 Win64"
cmake --build . --config Release
```

Replace "Visual Studio 15 2017" with the version of Visual Studio you have
installed. Leave out "Win64" to build a 32-bit version.

MSVC 2019:
```
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019" -A "x64"
cmake --build . --config Release
```

Replace "x64" with "Win32" to build a 32-bit version.

After building, you will need to run `regsvr32 rom-properties.dll` from
the `build\bin\Release` directory as Administrator.

Caveats:
* Registering rom-properties.dll hard-codes the full path in the registry.
  Moving the file will break the registration.
* CMake does not support building for multiple architectures at once. For
  Win64, a 64-bit build will work for Windows Explorer, but will not work
  in any 32-bit programs.

See README.md for general usage information.

### Building Distribution Packages

The scripts/ directory has a Windows batch script, ```package.cmd```,
that can be used to build the two distribution packages:
* rom-properties-[version]-windows.zip: Standard distribution.
* rom-properties-[version]-windows.debug.zip: PDB files.

The script uses the installed version of MSVC (2010, 2012, 2013, 2015, or
2017) to build both 32-bit and 64-bit versions of the ROM Properties Page
shell extension, and then it packages everything together.

You must have CMake and Info-ZIP's zip.exe and unzip.exe in your path.
* CMake: https://cmake.org/download/
* Info-ZIP: http://www.info-zip.org/
