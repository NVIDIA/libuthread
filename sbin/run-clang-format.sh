#!/bin/bash

PROJ_DIR=$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")
PROJ_DIR=$(dirname "${PROJ_DIR}")

find "${PROJ_DIR}/src" "${PROJ_DIR}/include" "${PROJ_DIR}/test" -iname *.h -o -iname *.c\
  | xargs clang-format -verbose -style=file -fallback-style=none -i