#include "txt.h"

#include "font.h"
extern unsigned char font_ttf[];
extern unsigned int font_ttf_len;

txtGlobal G = {0};

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
			printf("[in module '%s', line %d] %s\n", module, line, msg);
			break;
		case WREN_ERROR_RUNTIME:
			printf("Error: %s\n", msg);
			break;
	}
}

static void freeImport(WrenVM* vm, const char* name, WrenLoadModuleResult res)
{
	free(res.source);
}

static WrenLoadModuleResult loadModule(WrenVM* vm, const char* name)
{
	WrenLoadModuleResult res = {0};

	// just straight up fopen the file
	// idk if this is a proper solution to be honest
	FILE* f = fopen(name, "rb");
	if (!f) return res;

	fseek(f, 0, SEEK_END);
	int size = ftell(f);
	fseek(f, 0, SEEK_SET);
	char* source = malloc(size);

	if (source)
		if (fread(source, 1, size, f) == size)
		{
			source[size] = '\0';
			res.source = source;
		}

	res.onComplete = &freeImport;

	return res;
}

static void resetColors()
{
	G.currentColor = WHITE;
	G.currentBgColor = BLACK;
}

int main(int argc, char* argv[])
{
	// load file
	const char* startFile = "main.wren";
	if (argc > 1) startFile = argv[1];

	FILE* f = fopen(startFile, "rb");
	if (!f)
	{
		printf("file '%s' does not exist.", startFile);
		return -1;
	}

	fseek(f, 0, SEEK_END);
	int fileSize = ftell(f);
	fseek(f, 0, SEEK_SET);
	if (fileSize > 1073741824) // 1 GiB
	{
		printf("the file is way too large, what the heck?");
		return -1;
	}
	G.code = malloc(fileSize+1);

	if (G.code)
	{
		int readlen = fread(G.code, 1, fileSize, f);
		if (readlen != fileSize)
		{
			printf("couldn't read file: %s\n", strerror(errno));
			free(G.code);
			return -1;
		}
		G.code[fileSize] = '\0';
		fclose(f);
	}
	else
	{
		printf("couldn't allocate space for code.");
		free(G.code);
		return -1;
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
	config.loadModuleFn = &loadModule;
	config.bindForeignMethodFn = &bindTxtMethods;

	G.vm = wrenNewVM(&config);

	SetTraceLogLevel(LOG_NONE);
	InitWindow(0, 0, "a txt game");
	SetExitKey(KEY_NULL);

	G.font = LoadFontFromMemory(".ttf", font_ttf, font_ttf_len, G.fontSize, NULL, 657);

	// assuming font is square,
	// which it just so happens to be
	Vector2 fontSizeV = MeasureTextEx(G.font, "e", G.fontSize, 1);
	Vector2 screenSize = Vector2Multiply(fontSizeV, G.consoleSize);

	SetWindowPosition(
		GetMonitorWidth(0)/2-screenSize.x/2,
		GetMonitorHeight(0)/2-screenSize.y/2
	);

	SetWindowSize(screenSize.x, screenSize.y);

	if (wrenInterpret(G.vm, "main", txtClass) != WREN_RESULT_SUCCESS) return -1;
	if (wrenInterpret(G.vm, "main", G.code) != WREN_RESULT_SUCCESS) return -1;

	// get the class' handle
	wrenEnsureSlots(G.vm, 1);
	wrenGetVariable(G.vm, "main", "Game", 0);
	if (wrenGetSlotType(G.vm, 0) != WREN_TYPE_UNKNOWN)
	{
		printf("missing 'Game' class. must be named verbatim.");
		return -1;
	}
	WrenHandle* gameClass = wrenGetSlotHandle(G.vm, 0);

	// construct a "Game" instance
	WrenHandle* constructor = wrenMakeCallHandle(G.vm, "new()");
	wrenEnsureSlots(G.vm, 1);
	wrenSetSlotHandle(G.vm, 0, gameClass);
	if (wrenCall(G.vm, constructor) != WREN_RESULT_SUCCESS) return -1;
	wrenReleaseHandle(G.vm, constructor);

	// get it's handle and the update method
	wrenEnsureSlots(G.vm, 1);
	WrenHandle* gameInstance = wrenGetSlotHandle(G.vm, 0);
	WrenHandle* updateMethod = wrenMakeCallHandle(G.vm, "update(_)");

	// release the class' handle
	wrenReleaseHandle(G.vm, gameClass);

	while (!(WindowShouldClose() || G.close))
	{
		wrenEnsureSlots(G.vm, 1);
		wrenSetSlotHandle(G.vm, 0, gameInstance);
		wrenSetSlotDouble(G.vm, 1, GetFrameTime());
		if (wrenCall(G.vm, updateMethod) != WREN_RESULT_SUCCESS) return -1;

		BeginDrawing();
		ClearBackground(BLACK);
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
				i%(int)G.consoleSize.x*G.fontSize,
				i/(int)G.consoleSize.x*G.fontSize
			};

			DrawRectangleV(pos, MeasureTextEx(G.font, "a", G.fontSize, 1), bg);
			DrawTextEx(G.font, (char[]){chr, '\0'}, pos, G.fontSize, 1, fg);
		}
		resetColors();
		EndDrawing();
	}

	wrenReleaseHandle(G.vm, updateMethod);
	wrenReleaseHandle(G.vm, gameInstance);
	wrenFreeVM(G.vm);

	CloseWindow();

	free(G.screen);
	free(G.code);
	return 0;
}
