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

## Domains accessed:

* https://art.gametdb.com/ - Box, cover, and media scans for Nintendo
  GameCube, Wii, Wii U, DS, and 3DS games.
* https://amiibo.life/ - Images of Nintendo amiibo products.
* https://rpdb.gerbilsoft.com/ - Title screen images of Nintendo
  Game Boy Advance games.

## Security features

As of rom-properties v1.5, the downloading functionality has been
isolated into a single component, `rp-download`. The `rp-download`
program uses the following security features:

* Linux: seccomp(), AppArmor
* OpenBSD: pledge()
* Windows: Low-integrity process (on Vista+)
