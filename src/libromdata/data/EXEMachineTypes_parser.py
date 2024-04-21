#!/usr/bin/env python3
# EXE Machine Types string table builder
#
# Converts a text file containing a list of EXE machine types
# and prints a string table header.
#
# Syntax: EXEPEMachineTypes_parser.py infile outfile
#
# File syntax: ID|Name
# ID is a 16-bit hexadecimal machine type.
# Empty lines or lines starting with '#' are ignored.
#

# NOTE: All exceptions will be ignored and printed
# to the console.

import sys

if len(sys.argv) != 4:
	print('EXE Machine Types string table builder')
	print(f'Syntax: {sys.argv[0]} infile outfile exeType')
	sys.exit(1)

# String table. Starts with one string, an empty string.
string_table = bytearray(b'\x00')

# String dictionary. Maps strings to offsets within the string table.
string_dict = {'': 0}

# Dictionary of entries.
# - Key: Machine type
# - Value: Name
# Name is an index into string_table.
high_machine = 0	# highest valid machine type
entry_dict = { }

# Specific EXE type (PE, LE)
exeType = sys.argv[3]

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
		machine_type = int(arr[0], 0)
		if machine_type in entry_dict:
			raise ValueError(f'Duplicate entry for machine type {arr[0]} ({machine_type}).')
		if machine_type > high_machine:
			high_machine = machine_type

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
		entry_dict[machine_type] = name_idx

		# Next line.
		line = f_in.readline()

# Open output file.
f_out = open(sys.argv[2], 'w')

# Create the struct.
# If the maximum string index is less than 65536,
# string table indexes will be uint16_t. Otherwise,
# they will be uint32_t.
if len(string_table) < 256:
	offsetType = 'uint8_t'
elif len(string_table) < 65536:
	offsetType = 'uint16_t'
else:
	offsetType = 'uint32_t'

# For LE, machine type is always 8-bit.
if high_machine < 256:
	machineType = 'uint8_t'
	machinePadding = 4
else:
	machineType = 'uint16_t'
	machinePadding = 6

f_out.write(
	f"/** EXE {exeType} Machine Types (generated from {sys.argv[1]}) **/\n"
	"#pragma once\n\n"
	"#include <stdint.h>\n\n"
	f"static const char EXE{exeType}MachineTypes_strtbl[] =\n"
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

f_out.write(f"struct EXE{exeType}MachineTypes_offtbl_t {{\n")
f_out.write(f"\t{machineType} machineType;\n")
f_out.write(f"\t{offsetType} offset;\n")
f_out.write("};\n\n")
f_out.write(f"static const EXE{exeType}MachineTypes_offtbl_t EXE{exeType}MachineTypes_offtbl[] = {{\n")

i = 0
for machineType in entry_dict.keys():
	offset = entry_dict[machineType]

	if i % 10 == 0:
		if i != 0:
			f_out.write("\n")

	f_out.write(f"\t{{{machineType:#0{machinePadding}x}, {offset}}},\n")
	i += 1
f_out.write("};\n")
