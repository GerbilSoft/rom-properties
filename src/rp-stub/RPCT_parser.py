#!/usr/bin/env python3
# RPCT error code string table builder.
#
# Converts a text file containing a list of RPCT error code strings
# and prints a string table header.
#
# Syntax: RPCT_parser.py infile outfile
#
# Format: Number|Name|Description
# Number is always parsed as decimal.
# Empty lines or lines starting with '#' are ignored.
#

# NOTE: All exceptions will be ignored and printed
# to the console.

import sys

if len(sys.argv) != 3:
	print('RPCT error code string table builder')
	print(f'Syntax: {sys.argv[0]} infile outfile')
	sys.exit(1)

# String table. Starts with one string, an empty string.
string_table = bytearray(b'\x00')

# String dictionary. Maps strings to offsets within the string table.
string_dict = {"": 0}

# Dictionary of entries.
# - Key: Mapper number
# - Value: Tuple: (Name, Description)
# Name and Description are indexes into string_table.
high_errcode = 0	# highest valid error index
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
		if len(arr) != 3:
			raise ValueError(f'Incorrect number of splits on line {str(line_number)}.')

		# Check if we have an existing error code entry.
		errcode = int(arr[0])
		if errcode in entry_dict:
			raise ValueError(f'Duplicate entry for error code {arr[0]}.')
		if errcode > high_errcode:
			high_errcode = errcode

		name_idx = 0
		desc_idx = 0

		# Check if Name and Description are already in the string table.
		# If they aren't, add them to the string table.

		# Name
		if arr[1] in string_dict:
			name_idx = string_dict[arr[1]]
		else:
			name_idx = len(string_table)
			string_table.extend(arr[1].encode('UTF-8'))
			string_table.append(0)
			string_dict[arr[1]] = name_idx

		# Description
		if arr[2] in string_dict:
			desc_idx = string_dict[arr[2]]
		else:
			desc_idx = len(string_table)
			string_table.extend(arr[2].encode('UTF-8'))
			string_table.append(0)
			string_dict[arr[2]] = desc_idx

		# Add the tuple.
		entry_dict[errcode] = (name_idx, desc_idx)

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
	f"/** RPCT error code strings (generated from {sys.argv[1]}) **/\n"
	"#pragma once\n\n"
	"#include <stdint.h>\n\n"
	"static const char RPCT_error_code_strtbl[] =\n"
)

# Print up to 64 characters per line, including NULL bytes.
# Control codes and non-ASCII characters will be escaped.
# NOTE: Control characters may cause it to be slightly more
# than 64 characters per line, depending on where they show up.
string_table_len = len(string_table)
i = 0
f_out.write("\t\"")
last_was_hex = False
for c in string_table:
	if i >= 64:
		f_out.write("\"\n\t\"")
		last_was_hex = False
		i = 0

	if c < 32 or c >= 128:
		if i != 0 and not last_was_hex:
			f_out.write("\" \"")
			i += 3
		last_was_hex = True
		f_out.write("\\x{0:0{1}x}".format(c, 2))
		i += 4
	elif c == 0x22:	# double-quotes
		if last_was_hex:
			f_out.write("\" \"")
			i += 3
		last_was_hex = False
		f_out.write('\\"')
		i += 2
	else:
		if last_was_hex:
			f_out.write("\" \"")
			i += 3
		last_was_hex = False
		f_out.write(chr(c))
		i += 1
f_out.write("\";\n\n")

f_out.write(
	"typedef struct _RPCT_Err_String_t {\n"
	f"\t{idx_type} name_idx;\n"
	f"\t{idx_type} desc_idx;\n"
	"} RPCT_Err_String_t;\n\n"
	"static const RPCT_Err_String_t RPCT_Err_String_offtbl[] = {\n"
)

for errcode in range(high_errcode+1):
	if errcode % 10 == 0:
		if errcode != 0:
			f_out.write("\n")
		f_out.write(f"\t/* Error code {errcode} */\n")

	try:
		entry = entry_dict[errcode]
	except KeyError:
		# No error code. Print an empty entry.
		f_out.write("\t{0, 0},\n")
		continue

	# Print the entry.
	f_out.write(f"\t{{{entry[0]}, {entry[1]}}},\n")
f_out.write("};\n")
