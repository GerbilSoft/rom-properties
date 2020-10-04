#!/bin/sh
for SIZE in 16 24 32 64; do
	convert -grayscale Rec601Luma -alpha set -channel A -evaluate Divide 2 ach-${SIZE}x${SIZE}.png ach-gray-${SIZE}x${SIZE}.png
done
