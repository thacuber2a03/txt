#ifndef TXT_H
#define TXT_H

#include <stdint.h>
#include <stdarg.h>

#include "wren.h"
#include "raylib.h"
#include "raymath.h"

/**
 * each cell contains, in order:
 * 	the character to display
 * 	the rgb bytes for the background/foreground color
 */

// 7 bytes per cell
// unsatisfactory
#define CELL_SIZE 7

typedef struct
{
	Vector2 consoleSize;
	int totalCells;
	char* title;
	char* code;

	uint8_t* screen;
	Color currentColor, currentBgColor;

	Font font;
	int fontSize;

	bool close;
} txtGlobal;

extern txtGlobal G;

WrenForeignMethodFn bindTxtMethods(
	WrenVM* vm,
	const char* module,
	const char* className,
	bool isStatic,
	const char* signature
);

extern const char* txtClass;

#endif
