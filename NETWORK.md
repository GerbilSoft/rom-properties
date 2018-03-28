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

* http://art.gametdb.com/ - Box, cover, and media scans for Nintendo
  GameCube, Wii, Wii U, DS, and 3DS games.
* https://amiibo.life/ - Images of Nintendo amiibo products.
