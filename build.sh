#!/bin/sh

set -xe

CFLAGS="-Wall -Wextra -ggdb"
LIBS="-lraylib"

mkdir -p ./target/

gcc $CFLAGS -o ./target/drawingthing ./src/main.c $LIBS