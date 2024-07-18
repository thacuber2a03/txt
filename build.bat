@echo off

set locs=-Lwinlib -Iinclude
set libs=-lwren -lraylib -lgdi32 -lopengl32 -lwinmm
set flags=-Wno-discarded-qualifiers -static -O3

gcc %flags% src\*.c -o txt %locs% %libs%
