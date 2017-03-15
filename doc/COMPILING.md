# Compiling rom-properties

## Linux

On Ubuntu, you will need build-essential and the following development packages:
* All: cmake libcurl-dev nettle-dev zlib1g-dev libpng-dev libjpeg-dev
* KDE 4.x: libqt4-dev kdelibs5-dev
* KDE 5.x: qtbase5-dev kio-dev
* XFCE: libglib2.0-dev libgtk2.0-dev libthunarx-2-dev
* GNOME: libgtk-3-dev libnautilus-extension-dev

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
* Ubuntu: pkg-create-debsym

In the top-level source directory, run this command:
* ```debuild -i -us -uc -b```

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
* mkdir build
* cd build
* cmake .. -G "Visual Studio 15 2017"
* make
* cd src\win32
* regsvr32 rom-properties.dll

Replace "Visual Studio 15 2017" with the version of Visual Studio you have
installed. Add "Win64" after the year for a 64-bit version.

Caveats:
* Registering rom-properties.dll hard-codes the full path in the registry.
  Moving the file will break the registration.
* CMake does not support building for multiple architectures at once. For
  Win64, a 64-bit build will work for Windows Explorer, but will not work
  in any 32-bit programs.
* Due to file extension conflicts, the Windows version currently only
  registers itself for a subset of extensions. This does not include
  .iso or .bin. A future update may change extension registration to
  handle "all files".

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
