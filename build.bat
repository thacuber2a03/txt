@echo off

set locs=-Lwinlib -Iinclude
set libs=-lwren -lraylib -lgdi32 -lopengl32 -lwinmm

gcc src\*.c -o txt %locs% %libs% -Wno-discarded-qualifiers -static
