locs = -Iinclude
libs = -lraylib -lwren -lm
cflags = -Wno-discarded-qualifiers

ifeq ($(OS),Windows_NT)
	locs += -Lwinlib
	libs += -lgdi32 -lopengl32 -lwinmm
	cflags += -static
else
	locs += -Llib
endif

main: 
	gcc src/*.c -o txt $(locs) $(libs) $(cflags)
