#!/usr/bin/env python3
# DirectX 10 Formats string table builder
#
# Converts a text file containing a list of DirectX 10 formats
# and prints a string table header.
#
# Syntax: DX10Formats_parser.py infile outfile
#
# File syntax: Number|Name
# Number is always parsed as decimal.
# Empty lines or lines starting with '#' are ignored.
#

# NOTE: All exceptions will be ignored and printed
# to the console.

import sys

if len(sys.argv) != 3:
	print('DirectX 10 Formats string table builder')
	print(f'Syntax: {sys.argv[0]} infile outfile')
	sys.exit(1)

# String table. Starts with one string, an empty string.
string_table = bytearray(b'\x00')

# String dictionary. Maps strings to offsets within the string table.
string_dict = {"": 0}

# Dictionary of entries.
# - Key: DirectX 10 format number (DXGI)
# - Value: Name
# Name is an index into string_table.
high_dx10fmt = 0	# highest valid DX10 format index
entry_dict = { }

# Read lines from the input file.
line_number = 0
with open(sys.argv[1], 'r') as f_in:
	line = f_in.readline()
	while line:
		line_number += 1
		line = line.strip()
		if not line or line.startswith('#'):
			# Empty line or comment. Keep going.
			line = f_in.readline()
			continue

		# Split the line on '|'.
		arr = line.split('|')
		if len(arr) != 2:
			raise ValueError(f'Incorrect number of splits on line {str(line_number)}.')

		# Check if we have an existing entry.
		dx10fmt = int(arr[0])
		if dx10fmt in entry_dict:
			raise ValueError(f'Duplicate entry for DX10Format {arr[0]}.')
		if dx10fmt > high_dx10fmt:
			high_dx10fmt = dx10fmt

		# Check if Name is already in the string table.
		# If it isn't, add it to the string table.

		# Name
		if arr[1] in string_dict:
			name_idx = string_dict[arr[1]]
		else:
			name_idx = len(string_table)
			string_table.extend(arr[1].encode('UTF-8'))
			string_table.append(0)
			string_dict[arr[1]] = name_idx

		# Add the name to the dictionary.
		entry_dict[dx10fmt] = name_idx

		# Next line.
		line = f_in.readline()

# Open output file.
f_out = open(sys.argv[2], 'w')

# Create the struct.
# If the maximum string index is less than 65536,
# string table indexes will be uint16_t. Otherwise,
# they will be uint32_t.
idx_type = 'uint16_t' if len(string_table) < 65536 else 'uint32_t'

f_out.write(
	f"/** DirectX 10 Formats (generated from {sys.argv[1]}) **/\n\n"
	"#include <stdint.h>\n\n"
	"static const char8_t dxgiFormat_strtbl[] =\n"
)

# Print up to 64 characters per line, including NULL bytes.
# Control codes and non-ASCII characters will be escaped.
# NOTE: Control characters may cause it to be slightly more
# than 64 characters per line, depending on where they show up.
string_table_len = len(string_table)
i = 0
f_out.write("\tU8(\"")
last_was_hex = False
for c in string_table:
	if i >= 64:
		f_out.write("\")\n\tU8(\"")
		last_was_hex = False
		i = 0

	if c < 32 or c >= 128:
		if i != 0 and not last_was_hex:
			f_out.write("\") U8(\"")
			i += 7
		last_was_hex = True
		f_out.write("\\x{0:0{1}x}".format(c, 2))
		i += 4
	else:
		if last_was_hex:
			f_out.write("\") U8(\"")
			i += 7
		last_was_hex = False
		f_out.write(chr(c))
		i += 1
f_out.write("\");\n\n")

f_out.write(f"static const {idx_type} dxgiFormat_offtbl[] = {{\n")

for dx10fmt in range(high_dx10fmt+1):
	if dx10fmt % 32 == 0:
		if dx10fmt != 0:
			f_out.write("\n\n")
		f_out.write(f"\t/* DX10 Format {str(dx10fmt)} */\n\t")
	elif dx10fmt % 8 == 0:
		f_out.write("\n\t")

	try:
		entry = entry_dict[dx10fmt]
	except KeyError:
		# No format. Print an empty entry.
		f_out.write("0,")
		continue

	# Print the entry.
	f_out.write(f"{str(entry)},")
f_out.write("\n};\n")
