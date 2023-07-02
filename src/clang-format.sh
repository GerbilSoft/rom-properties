#!/bin/sh
find . -regex '.*\.\(c\|cpp\|h\|hpp\|m\|mm\|h\.in\)' -exec clang-format --style=file -i --verbose {} +
