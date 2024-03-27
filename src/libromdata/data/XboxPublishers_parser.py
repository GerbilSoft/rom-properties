#!/usr/bin/env python3
# Xbox Third-Party Publishers string table builder
#
# Converts a text file containing a list of Xbox third-party publishers
# and prints a string table header.
#
# Syntax: XboxPublishers_parser.py infile outfile
#
# File syntax: ID|Name
# ID is a two-character ID. Valid characters: [0-9A-Z]
# Empty lines or lines starting with '#' are ignored.
#

# NOTE: All exceptions will be ignored and printed
# to the console.

import sys

if len(sys.argv) != 3:
	print('Xbox Third-Party Publishers string table builder')
	print(f'Syntax: {sys.argv[0]} infile outfile')
	sys.exit(1)

# String table. Starts with one string, an empty string.
# XboxPublishers: Also has "<unlicensed>".
string_table = bytearray(b'\x00<unlicensed>\x00')

# String dictionary. Maps strings to offsets within the string table.
# XboxPublishers: Also has "<unlicensed>".
string_dict = {'': 0, '<unlicensed>': 1}

# Dictionary of entries.
# - Key: Third-party publisher ID
# - Value: Name
# Name is an index into string_table.
high_publisher = 'AA'	# highest valid Xbox third-party publisher index
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
		publisher = arr[0]
		if publisher in entry_dict:
			raise ValueError(f'Duplicate entry for publisher {arr[0]}.')
		if publisher > high_publisher:
			high_publisher = publisher

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
		entry_dict[publisher] = name_idx

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
	f"/** Xbox Third-Party Publishers (generated from {sys.argv[1]}) **/\n"
	"#pragma once\n\n"
	"#include <stdint.h>\n\n"
	"static const char XboxPublishers_strtbl[] =\n"
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
	else:
		if last_was_hex:
			f_out.write("\" \"")
			i += 3
		last_was_hex = False
		f_out.write(chr(c))
		i += 1
f_out.write("\";\n\n")

f_out.write(f"static const {idx_type} XboxPublishers_offtbl[] = {{\n")

# XboxPublishers uses two-character alpha-numeric IDs.
# Valid characters: [A-Z]
char_range = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'

i = 0
for pub1 in char_range:
	for pub2 in char_range:
		publisher = pub1 + pub2

		if i % 32 == 0:
			if i != 0:
				f_out.write("\n\n")
			f_out.write(f"\t/* Xbox Third-Party Publisher '{pub1}{pub2}' */\n\t")
		elif i % 8 == 0:
			f_out.write("\n\t")

		try:
			entry = entry_dict[publisher]
		except KeyError:
			# No format. Print an empty entry.
			f_out.write("0,")
			if publisher == high_publisher:
				break
			i += 1
			continue

		# Print the entry.
		f_out.write(f"{str(entry)},")
		if publisher == high_publisher:
			break
		i += 1
f_out.write("\n};\n")
