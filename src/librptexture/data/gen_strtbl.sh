#!/bin/sh
./strtbl_parser.py dxgiFormat DX10Formats_data.txt DX10Formats_data.h
./strtbl_parser.py vkEnum_base vkEnum_base_data.txt vkEnum_base_data.h
./strtbl_parser.py vkEnum_1000156xxx vkEnum_1000156xxx_data.txt vkEnum_1000156xxx_data.h
./strtbl_parser.py vkEnum_PVRTC vkEnum_PVRTC_data.txt vkEnum_PVRTC_data.h
./strtbl_parser.py vkEnum_ASTC vkEnum_ASTC_data.txt vkEnum_ASTC_data.h
