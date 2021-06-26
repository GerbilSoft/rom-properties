# Network Access Information

The ROM Properties Page Shell Extension has functions to access various
Internet databases in order to download artwork.

This functionality can be disabled by using the `rp-config` utility,
or by setting the following value in the configuration file:

```
[Downloads]
ExtImageDownload=false
```

The configuration file is located at:
* Linux: `~/.config/rom-properties/rom-properties.conf`
* Windows: `%APPDATA%\rom-properties\rom-properties.conf`

Downloaded images are saved in the following directory:
* Linux: `~/.cache/rom-properties/`
* Windows XP: `%USERPROFILE%\Local Settings\Application Data\rom-properties`
* Windows Vista and later: `%USERPROFILE%\AppData\LocalLow`

NOTE: Prior to rom-properties v1.5, the Windows cache directory was
`%LOCALAPPDATA%\rom-properties`. It was moved to `LocalLow` in order
to allow `rp-download` to run as a low-integrity process. Existing
files were not migrated over, so those can either be moved manually
or deleted.

## Domains accessed:

* https://art.gametdb.com/ - Box, cover, and media scans for Nintendo
  GameCube, Wii, Wii U, DS, and 3DS games.
* https://amiibo.life/ - Images of Nintendo amiibo products.
* https://rpdb.gerbilsoft.com/ - Title screen images for the following
  systems:
  * Neo Geo Pocket (Color)
  * Nintendo Game Boy
  * Nintendo Game Boy Color
  * Nintendo Game Boy Advance
  * Sega Mega Drive / Genesis, Sega CD, 32X, Pico
  * Super NES
  * WonderSwan (Color)

## Security features

As of rom-properties v1.5, the downloading functionality has been
isolated into a single component, `rp-download`. The `rp-download`
program uses the following security features:

* Linux: seccomp(), AppArmor
* OpenBSD: pledge()
* Windows: Low-integrity process (on Vista+)
