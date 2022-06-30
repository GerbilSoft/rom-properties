#!/bin/sh
nm -C -D "$1" | grep " [TBDi] " | cut -d ' ' -f 2- | sort -u
