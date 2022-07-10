#!/usr/bin/env python3
# CBM C64 Cartridge Type string table builder
#
# Converts a text file containing a list of C64 cartridge types
# and prints a string table header.
#
# Syntax: CBM_C64_Cartridge_type.py infile outfile
#
# File syntax: Number|Name
# Number is always parsed as decimal.
# Empty lines or lines starting with '#' are ignored.
#

# NOTE: All exceptions will be ignored and printed
# to the console.

import sys

if len(sys.argv) != 3:
	print('CBM C64 Cartridge Type string table builder')
	print(f'Syntax: {sys.argv[0]} infile outfile')
	sys.exit(1)

# String table. Starts with one string, an empty string.
string_table = bytearray(b'\x00')

# String dictionary. Maps strings to offsets within the string table.
string_dict = {"": 0}

# Dictionary of entries.
# - Key: C64 cartridge type
# - Value: Name
# Name is an index into string_table.
high_cart_type = 0	# highest valid C64 cartridge type
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
		cart_type = int(arr[0])
		if cart_type in entry_dict:
			raise ValueError(f'Duplicate entry for CBM C64 cartridge type {arr[0]}.')
		if cart_type > high_cart_type:
			high_cart_type = cart_type

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
		entry_dict[cart_type] = name_idx

		# Next line.
		line = f_in.readline()

# Open output file.
f_out = open(sys.argv[2], 'w')

# Create the struct.
# String table index type depends on the maximum string index:
# - 255: uint8_t
# - 65535: uint16_t
# - otherwise: uint32_t
idx_type = ''
if len(string_table) < 256:
	idx_type = 'uint8_t'
elif len(string_table) < 65536:
	idx_type = 'uint16_t'
else:
	idx_type = 'uint32_t'

f_out.write(
	f"/** CBM C64 Cartridge Types (generated from {sys.argv[1]}) **/\n\n"
	"#include <stdint.h>\n\n"
	"static const char8_t CBM_C64_cart_type_strtbl[] =\n"
)

# Convert back to UTF-8 for printing.
string_table_utf8 = string_table.decode('UTF-8')

# Print up to 64 characters per line, including NULL bytes.
# Control characters will be escaped.
# NOTE: Control characters may cause it to be slightly more
# than 64 characters per line, depending on where they show up.
i = 0
f_out.write("\tU8(\"")
last_was_hex = False
for c in string_table_utf8:
	if i >= 64:
		f_out.write("\")\n\tU8(\"")
		last_was_hex = False
		i = 0

	if ord(c) < 32:
		if i != 0 and not last_was_hex:
			f_out.write("\") U8(\"")
			i += 7
		last_was_hex = True
		f_out.write("\\x{0:0{1}x}".format(ord(c), 2))
		i += 4
	else:
		if last_was_hex:
			f_out.write("\") U8(\"")
			i += 7
		last_was_hex = False
		f_out.write(c)
		i += 1
f_out.write("\");\n\n")

f_out.write(f"static const {idx_type} CBM_C64_cart_type_offtbl[] = {{\n")

for cart_type in range(high_cart_type+1):
	if cart_type % 32 == 0:
		if cart_type != 0:
			f_out.write("\n\n")
		f_out.write(f"\t/* CBM C64 cartridge type {str(cart_type)} */\n\t")
	elif cart_type % 8 == 0:
		f_out.write("\n\t")

	try:
		entry = entry_dict[cart_type]
	except KeyError:
		# No format. Print an empty entry.
		f_out.write("0,")
		continue

	# Print the entry.
	f_out.write(f"{str(entry)},")
f_out.write("\n};\n")
