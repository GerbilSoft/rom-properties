#!/usr/bin/env python3
# Single-element string table builder (TCHAR version for http-status-codes)
#
# Converts a text file containing a list of strings and prints a string
# table header. "Single-element" means it's just a string array, not an
# array of structs.
#
# Syntax: strtbl_parser.py prefix infile outfile
#
# File syntax: ID|Name
# ID is always parsed as decimal.
# Empty lines or lines starting with '#' are ignored.
#

# NOTE: All exceptions will be ignored and printed
# to the console.

import sys

if len(sys.argv) != 4:
	print('Single-element string table builder')
	print(f'Syntax: {sys.argv[0]} infile outfile')
	sys.exit(1)

prefix = sys.argv[1]
infile = sys.argv[2]
outfile = sys.argv[3]

# String table. Starts with one string, an empty string.
string_table = bytearray(b'\x00')

# String dictionary. Maps strings to offsets within the string table.
string_dict = {"": 0}

# Dictionary of entries.
# - Key: String table index
# - Value: Name
# Name is an index into string_table.
high_idx = 0	# highest valid string table index
entry_dict = { }

# Read lines from the input file.
line_number = 0
with open(infile, 'r') as f_in:
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
		key = int(arr[0])
		if key in entry_dict:
			raise ValueError(f'Duplicate entry for index {arr[0]}.')
		if key > high_idx:
			high_idx = key

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
		entry_dict[key] = name_idx

		# Next line.
		line = f_in.readline()

# Open output file.
f_out = open(outfile, 'w')

# Create the arrays.
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
	f"/** {prefix} (generated from {infile}) **/\n"
	"#pragma once\n\n"
	"#include <stdint.h>\n\n"
	f"static const TCHAR {prefix}_strtbl[] =\n"
)

# Print up to 64 characters per line, including NULL bytes.
# Control codes and non-ASCII characters will be escaped.
# NOTE: Control characters may cause it to be slightly more
# than 64 characters per line, depending on where they show up.
string_table_len = len(string_table)
i = 0
f_out.write("\t_T(\"")
last_was_hex = False
for c in string_table:
	if i >= 64:
		f_out.write("\")\n\t_T(\"")
		last_was_hex = False
		i = 0

	if c < 32 or c >= 128:
		if i != 0 and not last_was_hex:
			f_out.write("\") _T(\"")
			i += 3
		last_was_hex = True
		f_out.write("\\x{0:0{1}x}".format(c, 2))
		i += 4
	else:
		if last_was_hex:
			f_out.write("\") _T(\"")
			i += 3
		last_was_hex = False
		f_out.write(chr(c))
		i += 1
f_out.write("\");\n\n")

f_out.write(f"typedef struct _HttpStatusMsg_t {{\n\tuint16_t code;\n\t{idx_type} offset;\n}} HttpStatusMsg_t;\n\n")

f_out.write(f"static const HttpStatusMsg_t {prefix}_offtbl[] = {{\n")

last100 = 0
for key in range(high_idx+1):
	try:
		entry = entry_dict[key]
	except KeyError:
		# No entry for this key.
		continue

	this100 = key // 100
	if last100 != 0 and last100 != this100:
		f_out.write("\n")
	last100 = this100

	# Print the entry.
	f_out.write(f"\t{{{str(key)}, {str(entry)}}},\n")
f_out.write("};\n")
