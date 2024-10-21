cfiles := $(wildcard src/*.c)
locs := -Iinclude
libs := -lraylib -lwren -lm
cflags := -Wno-discarded-qualifiers -g

exename := txt

ifeq ($(OS),Windows_NT)
	out := .\$(exename).exe
	locs += -Lwinlib
	libs += -lgdi32 -lopengl32 -lwinmm
	cflags += -static -fPIC
else
	out := ./$(exename)
	locs += -Llib
endif

.PHONY: clean run

$(out): $(cfiles) $(wildcard include/*.h)
	gcc $(cflags) $(cfiles) $(locs) $(libs) -o $@

clean:
	rm $(out)

run: $(out)
	$(out) $(file)
