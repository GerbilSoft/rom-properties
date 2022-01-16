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
  * Commodore 64/128 (cartridges)
  * Neo Geo Pocket (Color)
  * Nintendo Game Boy
  * Nintendo Game Boy Color
  * Nintendo Game Boy Advance
  * Sega Mega Drive / Genesis, Sega CD, 32X, Pico
  * Super NES
  * WonderSwan (Color)

### ROM Information Used

The following information is used as the lookup key in order to retrieve
images from the online databases:

* Commodore 64/128 cartridges: CRC32 of up to 16 KB of ROM data, and
  cartridge type.
* GameCube, Wii, Wii U: 6-character Game ID (e.g. `GALE01`) and region code.
* Game Boy, Game Boy Color: Game title (e.g. `POKEMON RED`), or game ID
  (e.g. `BXTJ01`) if available. Region code and publisher ID are also used.
* Game Boy Advance: 6-character Game ID (e.g. `AXVE01`)
* Nintendo DS: 4-character Game ID (e.g. `ATRE`)
* Nintendo 3DS: Product ID (e.g. `CTR-P-AXCE`) and region code.
* amiibo: 64-bit amiibo ID (e.g. `00000000-00000002`)
* Neo Geo Pocket (Color): Game ID (e.g. `NEOP0001`). For certain games
  with invalid IDs (e.g. homebrew), the game title is also used.
* Sega Mega Drive and related: Serial number (e.g. `GM 00001009-00`)
  and region code.
* Super NES: Game title and region code (e.g. `SNS-SUPER MARIOWORLD-USA`),
  or 2-character or 4-character game ID if present (e.g. `SNS-YI-USA`).
* WonderSwan (Color): Game ID (e.g. `SWJ-BAN001`), plus the Mono/Color flag.

## Security features

As of rom-properties v1.5, the downloading functionality has been
isolated into a single component, `rp-download`. The `rp-download`
program uses the following security features:

* Linux: seccomp(), AppArmor
* OpenBSD: pledge()
* Windows: Low-integrity process (on Vista+)
