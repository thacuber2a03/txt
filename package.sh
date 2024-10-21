#!/usr/bin/env bash
set -e
mkdir -p release
make
cp txt release/
cp examples/ release/ -r
cp LICENSE release/
cp main.wren release/
cp api.md release/
echo "done, now to compile the Windows version"
