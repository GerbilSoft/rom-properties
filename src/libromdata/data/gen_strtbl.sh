#!/bin/sh
set -ev
./strtbl_parser.py CBM_C64_cart_type CBM_C64_Cartridge_Type_data.txt CBM_C64_Cartridge_Type_data.h
./strtbl_parser.py ELFMachineTypes ELFMachineTypes_data.txt ELFMachineTypes_data.h
./strtbl_parser.py ELF_OSABI ELF_OSABI_data.txt ELF_OSABI_data.h
./strtbl_parser.py SegaTCode SegaPublishers_data.txt SegaPublishers_data.h
