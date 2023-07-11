# Amiibo database binary format

Key points:
* Most file offsets are absolute.
* String table offsets are relative to the beginning of the string table.
* Lengths are in bytes.
* All values are stored in little-endian format.

## Header

```c
typedef struct _AmiiboBinHeader {
	char magic[8];		// [0x000] "RPAMIB10"
	uint32_t strtbl_offset;	// [0x008] String table
	uint32_t strtbl_len;	// [0x00C]

	// Page 21 (characters)
	uint32_t cseries_offset;// [0x010] Series table
	uint32_t cseries_len;	// [0x014]
	uint32_t char_offset;	// [0x018] Character table
	uint32_t char_len;	// [0x01C]
	uint32_t cvar_offset;	// [0x020] Character variant table
	uint32_t cvar_len;	// [0x024]

	// Page 22 (amiibos)
	uint32_t aseries_offset;// [0x028] amiibo series table
	uint32_t as_len;	// [0x02C]
	uint32_t amiibo_offset;	// [0x030] amiibo ID table
	uint32_t amiibo_len;	// [0x034]

	// Reserved
	uint32_t reserved[18];	// [0x038]
} AmiiboBinHeader;
```

## String Table

String table is usually stored at the end of the file and contains
NULL-terminated strings. It must start with a NULL byte to allow
string offset 0 to represent an empty string.

## Character Series Table

Series table is an array of string table offsets. The index of the
series offset indicates the series ID. DWORD 0 is series `0x000,
DWORD 1 is series `0x004`, etc.

## Character Table

Character table contains a pair of values for each character.

```c
typedef struct _CharTableEntry {
	uint32_t char_id;	// Character ID
	uint32_t name;		// Character name (string table)
} CharTableEntry;
```

Note that amiibo character IDs are technically only 16-bit.
If the high bit of char_id is set, this character has variants, so the
variant information must be checked in the character variant table.

## Character Variant Table

The character variant table is sorted by char_id, so `bsearch()` can be used.

```c
typedef struct _CharVariantTableEntry {
	uint16_t char_id;	// Character ID
	uint8_t var_id;		// Variant ID
	uint8_t reserved;
	uint32_t name;		// Character variant name (string table)
} CharVariantTableEntry;
```

## amiibo Series Table

Series table is an array of string table offsets. The index of the
series offset indicates the series ID. Unlike the Character Series
Table, the amiibo Series Table is not multipled by 4, so DWORD 0 is
series 0, DWORD 1 is series 1, etc.

## amiibo ID table

The p.22 amiibo ID is used as an index into the table.

```c
typedef struct _AmiiboIDTableEntry {
	uint16_t release_no;	// Release number
	uint8_t wave_no;	// Wave number
	uint8_t reserved;
	uint32_t name;		// amiibo name (string table)
} AmiiboIDTableEntry;
```
