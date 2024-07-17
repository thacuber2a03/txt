locs = -Iinclude
libs = -lraylib -lwren -lm
cflags = -Wno-discarded-qualifiers -O3

ifeq ($(OS),Windows_NT)
	locs += -Lwinlib
	libs += -lgdi32 -lopengl32 -lwinmm
	cflags += -static -fPIC
else
	locs += -Llib
endif

.PHONY: main clean

main: 
	gcc $(cflags) src/*.c $(locs) $(libs) -o txt

clean:
	rm ./txt
