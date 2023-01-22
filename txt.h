#ifndef TXT_H
#define TXT_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>

#include "wren.h"
#include "raylib.h"
#include "raymath.h"

/**
 * each cell contains, in order:
 * 	the character to display
 * 	the rgb bytes for the background/foreground color
 * 	(I don't feel like designing a color palette)
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
	WrenVM* vm;

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
