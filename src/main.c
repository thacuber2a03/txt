#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "raylib.h"
#include "txt.h"

#include "font.inl"
extern unsigned char font_ttf[];
extern unsigned int font_ttf_len;

txtGlobal G = {0};

static void die(const char* fmt, ...)
{
	fprintf(stderr, "txt [fatal]: ");

	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

	exit(-1);
}

static void debugPrint(WrenVM* vm, const char* text)
{
	printf("%s", text);
}

static void errorPrint(WrenVM* vm, WrenErrorType errorType,
                       const char* module, const int line,
                       const char* msg)
{
	switch (errorType)
	{
		case WREN_ERROR_COMPILE:
		case WREN_ERROR_STACK_TRACE:
			fprintf(stderr, "[in module '%s', line %d] %s\n", module, line, msg);
			break;
		case WREN_ERROR_RUNTIME:
			fprintf(stderr, "Error: %s\n", msg);
			break;
	}
}

#define TXT_MODNAME "txt"

#define MODULE_SEP "/"
#ifdef __unix__
#define SEPARATOR '/'
#else
#define SEPARATOR '\\'
#endif

static char *module = NULL;

static void freeImport(WrenVM* vm,
	const char* name, WrenLoadModuleResult res)
{
	if (module) free(module);
}

static char *readFile(char *path, const char* module)
{
	FILE* f = fopen(path, "rb");
	if (!f) return NULL;

	fseek(f, 0, SEEK_END);
	int size = ftell(f);
	rewind(f);

	char* source;
	if (!(source = malloc(size+1)))
		die("couldn't allocate enough memory for loading '%s'\n", module);

	if (fread(source, 1, size, f) != size)
		die("couldn't read file: %s\n", strerror(errno));

	source[size] = '\0';
	fclose(f);
	return source;
}

static WrenLoadModuleResult loadModule(WrenVM* vm, const char* name)
{
	WrenLoadModuleResult res = {0};
	res.onComplete = &freeImport;

	// default to Wren's builtin 'random' and 'meta'
	// that's a whole can of worms I'm really not touching
	if (!strcmp(name, "random") || !strcmp(name, "meta"))
		return res;

	if (!strcmp(name, TXT_MODNAME))
	{
		res.source = txtClass;
		return res;
	}

	// just straight up fopen the file
	// idk if this is a proper solution to be honest
	//
	// update 2 years later: you're only compiling
	// for Windows and Linux what makes you think that
	// it's "not a proper solution" shut up

	module = NULL;
	int modlen = strlen(name);
	module = malloc(modlen + sizeof ".wren" + 1);

	strcpy(module, name);
	for (int i = 0; i < modlen; i++)
		if (module[i] == '/') module[i] = SEPARATOR;
	strcpy(module+modlen, ".wren");
	module[modlen + sizeof ".wren"] = '\0';

	res.source = readFile(module, name);
	if (module == NULL)
	{
		strcpy(module+modlen, ".txt");
		res.source = readFile(module, name);
	}

	free(module);
	module = (char*)res.source;

	return res;
}

int main(int argc, char* argv[])
{
	// load file
	const char* startFile = "main.wren";
	if (argc > 1) startFile = argv[1];

	FILE* f = fopen(startFile, "rb");
	if (!f) die("couldn't open '%s': %s\n(did you mean to open another file?)\n", startFile, strerror(errno));

	fseek(f, 0, SEEK_END);
	int fileSize = ftell(f);
	fseek(f, 0, SEEK_SET);
	G.code = malloc(fileSize+1);

	if (G.code)
	{
		int readlen = fread(G.code, 1, fileSize, f);
		if (readlen != fileSize)
		{
			free(G.code);
			die("couldn't read file: %s\n", strerror(errno));
		}
		G.code[fileSize] = '\0';
		fclose(f);
	}
	else
	{
		free(G.code);
		die("couldn't allocate enough space for code.");
	}

	G.consoleSize = (Vector2){32,32};
	G.currentBgColor = BLACK;
	G.currentColor = WHITE;
	G.totalCells = G.consoleSize.x*G.consoleSize.y;
	int totalBytes = G.totalCells*CELL_SIZE;
	G.screen = malloc(totalBytes);
	memset(G.screen, 0, totalBytes);
	G.fontSize = 16;

	WrenConfiguration config;
	wrenInitConfiguration(&config);
	config.writeFn = &debugPrint;
	config.errorFn = &errorPrint;
	config.loadModuleFn = loadModule;
	config.bindForeignMethodFn = &bindTxtMethods;

	WrenVM *vm = wrenNewVM(&config);

	SetTraceLogLevel(LOG_NONE);
	InitWindow(0, 0, "a txt game");
	SetWindowState(FLAG_WINDOW_HIDDEN);
	SetExitKey(KEY_NULL);

	G.font = LoadFontFromMemory(".ttf", font_ttf, font_ttf_len, G.fontSize, NULL, 657);

	if (wrenInterpret(vm, "main", G.code) != WREN_RESULT_SUCCESS) return -1;

	// get the class' handle
	wrenEnsureSlots(vm, 1);
	wrenGetVariable(vm, "main", "Game", 0);

	if (wrenGetSlotType(vm, 0) != WREN_TYPE_UNKNOWN)
		die("'Game' is missing or isn't a class\n");

	WrenHandle* gameClass = wrenGetSlotHandle(vm, 0);
	WrenHandle* constructor = wrenMakeCallHandle(vm, "new()");
	wrenSetSlotHandle(vm, 0, gameClass);
	if (wrenCall(vm, constructor) != WREN_RESULT_SUCCESS) return -1;
	wrenReleaseHandle(vm, constructor);

	// assuming font is square, which it just so happens to be
	Vector2 fontSizeV = MeasureTextEx(G.font, "e", G.fontSize, 1);
	Vector2 screenSize = Vector2Multiply(fontSizeV, G.consoleSize);

	SetWindowPosition(
		(GetMonitorWidth(0)-screenSize.x)/2,
		(GetMonitorHeight(0)-screenSize.y)/2
	);

	SetWindowSize(screenSize.x, screenSize.y);
	ClearWindowState(FLAG_WINDOW_HIDDEN);

	WrenHandle* gameInstance = wrenGetSlotHandle(vm, 0);
	WrenHandle* updateMethod = wrenMakeCallHandle(vm, "update(_)");
	wrenReleaseHandle(vm, gameClass);

	while (!(WindowShouldClose() || G.close))
	{
		wrenEnsureSlots(vm, 1);
		wrenSetSlotHandle(vm, 0, gameInstance);
		wrenSetSlotDouble(vm, 1, GetFrameTime());
		if (wrenCall(vm, updateMethod) != WREN_RESULT_SUCCESS) return -1;

		BeginDrawing();

		for (int i = 0; i < G.totalCells; i++)
		{
			int c = i*CELL_SIZE;
			char chr = G.screen[c];
			if (chr == 0) continue;
			Color fg, bg;
			fg.r = G.screen[c+1];
			fg.g = G.screen[c+2];
			fg.b = G.screen[c+3];
			fg.a = 255;

			bg.r = G.screen[c+4];
			bg.g = G.screen[c+5];
			bg.b = G.screen[c+6];
			bg.a = 255;

			Vector2 pos = {
				i%((int)G.consoleSize.x)*G.fontSize,
				i/((int)G.consoleSize.x)*G.fontSize,
			};

			DrawRectangleV(pos, MeasureTextEx(G.font, "a", G.fontSize, 1), bg);
			DrawTextEx(G.font, (char[]){chr, '\0'}, pos, G.fontSize, 1, fg);
		}

		G.currentColor = WHITE;
		G.currentBgColor = BLACK;
		EndDrawing();
	}

	wrenReleaseHandle(vm, updateMethod);
	wrenReleaseHandle(vm, gameInstance);
	wrenFreeVM(vm);

	CloseWindow();

	free(G.screen);
	free(G.code);
	return 0;
}
