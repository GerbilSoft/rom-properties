#!/bin/sh
nm -C -D "$1" | grep " [BDRTVi] " | cut -d ' ' -f 2- | sort -u
