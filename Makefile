cfiles := $(wildcard src/*.c)
locs := -Iinclude
libs := -lraylib -lwren -lm
cflags := -Wno-discarded-qualifiers -g
out := ./txt

ifeq ($(OS),Windows_NT)
	locs += -Lwinlib
	libs += -lgdi32 -lopengl32 -lwinmm
	cflags += -static -fPIC
	out += .exe
else
	locs += -Llib
endif

.PHONY: clean run

$(out): $(cfiles) $(wildcard include/*.h)
	gcc $(cflags) $(cfiles) $(locs) $(libs) -o $@

clean:
ifeq ($(OS),Windows_NT)
	del $(out)
else
	rm $(out)
endif

file ?= examples/hello.wren
run: $(out)
	$(out) $(file)
