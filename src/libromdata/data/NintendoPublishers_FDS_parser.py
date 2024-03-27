#!/usr/bin/env python3
# Nintendo Publishers (FDS) string table builder
#
# Converts a text file containing a list of Nintendo third-party publishers (FDS)
# and prints a string table header.
#
# Syntax: NESMappers_parser.py infile outfile
#
# File syntax: ID|NameEN|NameJP
# ID is a two-digit hexadecimal value.
# Empty lines or lines starting with '#' are ignored.
#

# NOTE: All exceptions will be ignored and printed
# to the console.

import sys

if len(sys.argv) != 3:
	print('Nintendo Third-Party Publishers (FDS) string table builder')
	print(f'Syntax: {sys.argv[0]} infile outfile')
	sys.exit(1)

# String table. Starts with one string, an empty string.
string_table = bytearray(b'\x00<unlicensed>\x00<\xe9\x9d\x9e\xe5\x85\xac\xe8\xaa\x8d>\x00')

# String dictionary. Maps strings to offsets within the string table.
# NintendoPublishers_FDS: Also has "<unlicensed>" and "<非公認>".
string_dict = {'': 0, '<unlicensed>': 1, '<非公認>': 14}

# Dictionary of entries.
# - Key: Mapper number
# - Value: Tuple: (NameEN, NameJP)
# NameEN and NameJP are indexes into string_table.
high_publisher = 0		# highest valid publisher index
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

		# Check if we have an existing publisher entry.
		publisher = int(arr[0], 16)
		if publisher in entry_dict:
			raise ValueError(f'Duplicate entry for publisher {arr[0]}.')
		if publisher > high_publisher:
			high_publisher = publisher

		nameEN_idx = 0
		nameJP_idx = 0

		# Check if NameEN and NameJP are already in the string table.
		# If they aren't, add them to the string table.

		# NameEN
		if arr[1] in string_dict:
			nameEN_idx = string_dict[arr[1]]
		else:
			nameEN_idx = len(string_table)
			string_table.extend(arr[1].encode('UTF-8'))
			string_table.append(0)
			string_dict[arr[1]] = nameEN_idx

		# NameJP
		if arr[2] in string_dict:
			nameJP_idx = string_dict[arr[2]]
		else:
			nameJP_idx = len(string_table)
			string_table.extend(arr[2].encode('UTF-8'))
			string_table.append(0)
			string_dict[arr[2]] = nameJP_idx

		# Add the tuple.
		entry_dict[publisher] = (nameEN_idx, nameJP_idx)

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
	f"/** Nintendo Third-Party Publishers (FDS) (generated from {sys.argv[1]}) **/\n"
	"#pragma once\n\n"
	"#include <stdint.h>\n\n"
	"static const char NintendoPublishers_FDS_strtbl[] =\n"
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

f_out.write(
	"typedef struct _NintendoPublisher_FDS_Entry {\n"
	f"\t{idx_type} nameEN_idx;\n"
	f"\t{idx_type} nameUS_idx;\n"
	"} NintendoPublisher_FDS_Entry;\n\n"
	"static const NintendoPublisher_FDS_Entry NintendoPublishers_FDS_offtbl[] = {\n"
)

for publisher in range(high_publisher+1):
	if publisher % 32 == 0:
		if publisher != 0:
			f_out.write("\n\n")
		publisher_str = "{0:0{1}x}".format(publisher, 2).upper()
		f_out.write(f"\t/* Nintendo Third-Party Publisher (FDS) '{publisher_str}' */\n\t")
	elif publisher % 8 == 0:
		f_out.write("\n\t")

	try:
		entry = entry_dict[publisher]
	except KeyError:
		# No format. Print an empty entry.
		f_out.write("{0,0},")
		continue

	# Print the entry.
	f_out.write(f"{{{entry[0]},{entry[1]}}},")
f_out.write("\n};\n")
