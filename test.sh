#!/bin/sh

DIR="./test"
BIN="$DIR/t"

set -xeu

gcc -o $BIN $DIR/abyss.c && $BIN 
# ...

rm $BIN
