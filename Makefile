locs = -Llib -Iinclude
libs = -lwren -lraylib -lgdi32 -lopengl32 -lwinmm

main: 
	gcc src\\*.c -o txt $(locs) $(libs) -static -Wno-discarded-qualifiers
