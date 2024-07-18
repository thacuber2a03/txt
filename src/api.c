#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "txt.h"

#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))
#define clamp(v, a, b) max(a, min(v, b))

#define defineForeignMethod(name) static void txtApi_##name(WrenVM* vm)
#define foreignMethod(name) if (strstr(signature, #name)) return txtApi_##name
#define privateForeignMethod(name) if (strstr(signature, #name "_")) return txtApi_##name

#define getSetForeign(name) do { \
		if (strstr(signature, #name)) \
		{ \
			if (params == 1) return txtApi_set##name; \
			return txtApi_get##name;\
		} \
	} while(0)

#define MIN_SCREEN_SIZE 8
#define MAX_SCREEN_SIZE 128

const char* txtClass = "\
class TXT { \n\
	static version { \"1.0\" } \n\
	foreign static width \n\
	foreign static width=(w) \n\
	foreign static height \n\
	foreign static height=(h) \n\
	foreign static size \n\
	foreign static size=(p) \n\
	static size(w, h) { size = [w, h] } \n\
	foreign static move(p) \n\
	static move(x, y) { move([x, y]) } \n\
	foreign static title=(t) \n\
	foreign static fontSize=(px) \n\
	foreign static exit() \n\
\n\
	foreign static clear(char) \n\
	foreign static clear() \n\
\n\
	foreign static write_(p, text) \n\
	static write(x, y, value) { write_([x, y], value.toString) } \n\
	static write(p, value)    { write_(p, value.toString)      } \n\
\n\
	foreign static read(p) \n\
	static read(x, y) { read([x, y]) } \n\
\n\
	foreign static charInfo(p) \n\
	static charInfo(x, y) { charInfo([x, y]) } \n\
\n\
	foreign static color(r,g,b) \n\
	static color(g) {\n\
		System.print(\"WARNING: Calling deprecated method 'color(_)'\") \n\
		color = g \n\
	} \n\
\n\
	static color=(g) { \n\
		if (g is Num) { \n\
			color(g, g, g) \n\
		} else if (g is List) { \n\
			color(g[0], g[1], g[2]) \n\
		} else { \n\
			Fiber.abort(\"expected Num or List, got %(g.type)\") \n\
		} \n\
	} \n\
\n\
	foreign static bgColor(r,g,b) \n\
	static bgColor(g) {\n\
		System.print(\"WARNING: Calling deprecated method 'bgColor(_)'\") \n\
		bgColor = g \n\
	} \n\
\n\
	static bgColor=(g) { \n\
		if (g is Num) { \n\
			bgColor(g, g, g) \n\
		} else if (g is List) { \n\
			bgColor(g[0], g[1], g[2]) \n\
		} else { \n\
			Fiber.abort(\"expected Num or List, got %(g.type)\") \n\
		} \n\
	} \n\
\n\
	foreign static mousePos \n\
	foreign static mouseDown(button) \n\
	foreign static mousePressed(button) \n\
	foreign static keyDown(key) \n\
	foreign static keyDown \n\
	foreign static keyPressed(key) \n\
	foreign static keyPressed \n\
	foreign static charsPressed \n\
}\
";

static char* typenames[] =
{
	[WREN_TYPE_NUM] = "Num",
	[WREN_TYPE_BOOL] = "Bool",
	[WREN_TYPE_STRING] = "String",
	[WREN_TYPE_NULL] = "Null",
	[WREN_TYPE_LIST] = "List",
	[WREN_TYPE_MAP] = "Map",
	[WREN_TYPE_UNKNOWN] = "unknown object", // "unknown object (this is a bug)",
	[WREN_TYPE_FOREIGN] = "foreign object", // "foreign object (this is a bug)",
};

/* I'm not going to get rid of this after all the time I spent writing it
static char* keynames[] = {
	"'", ",", "-", ".", "/",
	"0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
	";", "=",
	"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l",
	"m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z",
	"[", "\\", "]", "`",
	"space", "escape", "enter", "tab", "backspace", "insert", "delete",
	"right", "left", "up", "down",
	"page up", "page down", "home", "end",
	"caps lock", "scroll down", "num lock", "print screen", "pause",
	"f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "f10", "f11", "f12",
	"left shift", "left ctrl", "left alt", "left super",
	"right shift", "right ctrl", "right alt", "right super", "menu",
	"keypad 0", "keypad 1", "keypad 2", "keypad 3", "keypad 4","keypad 5",
	"keypad 6", "keypad 7", "keypad 8", "keypad 9",
	"keypad .", "keypad /", "keypad *", "keypad -", "keypad +", "keypad enter",
	"keypad ="
};
*/

static void txtThrowErr(WrenVM *vm, const char* msg, ...)
{
	char err[256];

	va_list args;
	va_start(args, msg);
	vsprintf(err, msg, args);
	va_end(args);

	wrenEnsureSlots(vm, 1);
	wrenSetSlotString(vm, 0, err);
	wrenAbortFiber(vm, 0);
}

static int txtEnsureType(WrenVM *vm, int slot, WrenType expected)
{
	WrenType type = wrenGetSlotType(vm, slot);
	if (type != expected)
	{
		txtThrowErr(vm, "expected %s, got %s", typenames[expected], typenames[type]);
		return 0;
	}
	return 1;
}

static int getSignatureParams(const char* signature)
{
	int params = 0;

	for (int i = 0; i < strlen(signature); i++)
		if (signature[i] == '_') params++;

	return params;
}

static void txtWriteCharIdx(int cell, char c)
{
	cell = (int)Wrap(cell, 0, G.totalCells);
	int i = cell*CELL_SIZE;
	G.screen[i] = c;

	G.screen[i+1] = G.currentColor.r;
	G.screen[i+2] = G.currentColor.g;
	G.screen[i+3] = G.currentColor.b;

	G.screen[i+4] = G.currentBgColor.r;
	G.screen[i+5] = G.currentBgColor.g;
	G.screen[i+6] = G.currentBgColor.b;
}

static inline void txtWriteChar(int x, int y, char c)
{
	txtWriteCharIdx(y*G.consoleSize.x+x, c);
}

static inline uint8_t *txtGetCharIdx(int cell)
{
	cell %= G.totalCells;
	return &G.screen[cell*CELL_SIZE];
}

static inline uint8_t *txtGetChar(int x, int y)
{
	return txtGetCharIdx(y*G.consoleSize.x+x);
}

static void txtResize(int w, int h)
{
	if (w < 0) w = MAX_SCREEN_SIZE + w+1;
	if (h < 0) h = MAX_SCREEN_SIZE + h+1;

	G.consoleSize = Vector2Clamp(
		(Vector2){w, h},
		(Vector2){MIN_SCREEN_SIZE, MIN_SCREEN_SIZE},
		(Vector2){MAX_SCREEN_SIZE, MAX_SCREEN_SIZE}
	);

	G.totalCells = w*h;
	size_t bytes = G.totalCells*CELL_SIZE;
	G.screen = realloc(G.screen, bytes);
	memset(G.screen, 0, bytes);

	SetWindowSize(
		G.fontSize*G.consoleSize.x,
		G.fontSize*G.consoleSize.y
	);
}

defineForeignMethod(setwidth)
{
	if (!txtEnsureType(vm, 1, WREN_TYPE_NUM)) return;
	txtResize(wrenGetSlotDouble(vm, 1), G.consoleSize.y);
	wrenSetSlotNull(vm, 0);
}

defineForeignMethod(getwidth) { wrenSetSlotDouble(vm, 0, G.consoleSize.x); }

defineForeignMethod(setheight)
{
	if (!txtEnsureType(vm, 1, WREN_TYPE_NUM)) return;
	txtResize(G.consoleSize.x, wrenGetSlotDouble(vm, 1));
	wrenSetSlotNull(vm, 0);
}

defineForeignMethod(getheight) { wrenSetSlotDouble(vm, 0, G.consoleSize.y); }

defineForeignMethod(getsize)
{
	wrenEnsureSlots(vm, 3);
	wrenSetSlotNewList(vm, 0);

	wrenSetSlotDouble(vm, 1, G.consoleSize.x);
	wrenInsertInList(vm, 0, -1, 1);
	wrenSetSlotDouble(vm, 2, G.consoleSize.x);
	wrenInsertInList(vm, 0, -1, 2);
}

defineForeignMethod(setsize)
{
	if (!txtEnsureType(vm, 1, WREN_TYPE_LIST)) return;

	wrenEnsureSlots(vm, 3);
	wrenGetListElement(vm, 1, 1, 2);
	wrenGetListElement(vm, 1, 0, 1);

	txtResize(
		wrenGetSlotDouble(vm, 1),
		wrenGetSlotDouble(vm, 2)
	);
}

defineForeignMethod(move)
{
	if (!txtEnsureType(vm, 1, WREN_TYPE_NUM)) return;
	if (!txtEnsureType(vm, 2, WREN_TYPE_NUM)) return;

	int dx = wrenGetSlotDouble(vm, 1),
	    dy = wrenGetSlotDouble(vm, 2);

	Vector2 pos = GetWindowPosition();
	SetWindowPosition(
		pos.x+dx*G.fontSize,
		pos.y+dy*G.fontSize
	);
}

defineForeignMethod(title)
{
	if (!txtEnsureType(vm, 1, WREN_TYPE_STRING)) return;
	SetWindowTitle(wrenGetSlotString(vm, 1));
}

defineForeignMethod(fontSize)
{
	if (!txtEnsureType(vm, 1, WREN_TYPE_NUM)) return;
	G.fontSize = wrenGetSlotDouble(vm, 1);
	G.fontSize = clamp(G.fontSize, 8, 32);
	txtResize(G.consoleSize.x, G.consoleSize.y);
}

defineForeignMethod(clear)
{
	if (!txtEnsureType(vm, 1, WREN_TYPE_STRING)) return;

	const char* chr = wrenGetSlotString(vm, 1);
	for (int i = 0; i < G.totalCells; i++) txtWriteCharIdx(i, *chr);
}

defineForeignMethod(clearSpaces)
{
	for (int i = 0; i < G.totalCells; i++) txtWriteCharIdx(i, ' ');
}

defineForeignMethod(write)
{
	if (!txtEnsureType(vm, 1, WREN_TYPE_LIST)) return;
	if (!txtEnsureType(vm, 2, WREN_TYPE_STRING)) return;

	wrenEnsureSlots(vm, 4);
	wrenGetListElement(vm, 1, 0, 3);
	wrenGetListElement(vm, 1, 1, 4);

	int x = wrenGetSlotDouble(vm, 3);
	int y = wrenGetSlotDouble(vm, 4);
	const char* text = wrenGetSlotString(vm, 2);

	int len = strlen(text);
	for (int i = 0; i < strlen(text); i++)
		txtWriteChar(x+i, y, text[i]);
	wrenSetSlotDouble(vm, 0, len);
}

defineForeignMethod(read)
{
	if (!txtEnsureType(vm, 1, WREN_TYPE_LIST)) return;

	wrenEnsureSlots(vm, 4);
	wrenGetListElement(vm, 1, 0, 2);
	wrenGetListElement(vm, 1, 1, 3);

	int x = wrenGetSlotDouble(vm, 2);
	int y = wrenGetSlotDouble(vm, 3);
	const char c = *txtGetChar(x, y);

	wrenSetSlotBytes(vm, 0, &c, 1);
}

defineForeignMethod(charInfo)
{
	if (!txtEnsureType(vm, 1, WREN_TYPE_LIST)) return;

	wrenEnsureSlots(vm, 4);
	wrenGetListElement(vm, 1, 0, 2);
	wrenGetListElement(vm, 1, 1, 3);

	int x = wrenGetSlotDouble(vm, 2),
	    y = wrenGetSlotDouble(vm, 3);

	// ohhhhh boy this is going to be a wild ride
	wrenEnsureSlots(vm, 5);
	wrenSetSlotNewMap(vm, 0);

	const uint8_t *c = txtGetChar(x, y);
	wrenSetSlotBytes(vm, 1, (char*)c, 1);
	wrenSetSlotString(vm, 2, "char");
	wrenSetMapValue(vm, 0, 2, 1);

	// foreground color
	wrenSetSlotNewList(vm, 1);
	wrenSetSlotDouble(vm, 2, c[1]); wrenInsertInList(vm, 1, 0, 2);
	wrenSetSlotDouble(vm, 3, c[2]); wrenInsertInList(vm, 1, 1, 3);
	wrenSetSlotDouble(vm, 4, c[3]); wrenInsertInList(vm, 1, 2, 4);

	wrenSetSlotString(vm, 2, "fg");
	wrenSetMapValue(vm, 0, 2, 1);

	// background color
	wrenSetSlotNewList(vm, 1);
	wrenSetSlotDouble(vm, 2, c[4]); wrenInsertInList(vm, 1, 0, 2);
	wrenSetSlotDouble(vm, 3, c[5]); wrenInsertInList(vm, 1, 1, 3);
	wrenSetSlotDouble(vm, 4, c[6]); wrenInsertInList(vm, 1, 2, 4);

	wrenSetSlotString(vm, 2, "bg");
	wrenSetMapValue(vm, 0, 2, 1);
}

defineForeignMethod(color)
{
	if (!txtEnsureType(vm, 1, WREN_TYPE_NUM)) return;
	if (!txtEnsureType(vm, 2, WREN_TYPE_NUM)) return;
	if (!txtEnsureType(vm, 3, WREN_TYPE_NUM)) return;

	G.currentColor.r = (uint8_t)wrenGetSlotDouble(vm, 1);
	G.currentColor.g = (uint8_t)wrenGetSlotDouble(vm, 2);
	G.currentColor.b = (uint8_t)wrenGetSlotDouble(vm, 3);
}

defineForeignMethod(bgColor)
{
	if (!txtEnsureType(vm, 1, WREN_TYPE_NUM)) return;
	if (!txtEnsureType(vm, 2, WREN_TYPE_NUM)) return;
	if (!txtEnsureType(vm, 3, WREN_TYPE_NUM)) return;

	G.currentBgColor.r = (uint8_t)wrenGetSlotDouble(vm, 1);
	G.currentBgColor.g = (uint8_t)wrenGetSlotDouble(vm, 2);
	G.currentBgColor.b = (uint8_t)wrenGetSlotDouble(vm, 3);
}

defineForeignMethod(mousePos)
{
	wrenEnsureSlots(vm, 4);
	wrenSetSlotNewList(vm, 0);

	int x = GetMouseX()/G.fontSize,
	    y = GetMouseY()/G.fontSize;

	wrenSetSlotDouble(vm, 1, x);
	wrenSetSlotDouble(vm, 2, y);

	wrenInsertInList(vm, 0, -1, 1);
	wrenInsertInList(vm, 0, -1, 2);
}

defineForeignMethod(mouseDown)
{
	if (!txtEnsureType(vm, 1, WREN_TYPE_STRING)) return;

	const char* buttonName = wrenGetSlotString(vm, 1);
	int button = -1;
	if (!strcmp(buttonName, "left"))   button = MOUSE_BUTTON_LEFT;
	if (!strcmp(buttonName, "right"))  button = MOUSE_BUTTON_RIGHT;
	if (!strcmp(buttonName, "middle")) button = MOUSE_BUTTON_MIDDLE;
	if (button == -1) txtThrowErr(vm, "unknown button name");

	wrenEnsureSlots(vm, 1);
	wrenSetSlotBool(vm, 0, IsMouseButtonDown(button));
}

defineForeignMethod(mousePressed)
{
	if (!txtEnsureType(vm, 1, WREN_TYPE_STRING)) return;

	const char* buttonName = wrenGetSlotString(vm, 1);
	int button = -1;
	if (!strcmp(buttonName, "left"))   button = MOUSE_BUTTON_LEFT;
	if (!strcmp(buttonName, "right"))  button = MOUSE_BUTTON_RIGHT;
	if (!strcmp(buttonName, "middle")) button = MOUSE_BUTTON_MIDDLE;
	if (button == -1) txtThrowErr(vm, "unknown button name");

	wrenEnsureSlots(vm, 1);
	wrenSetSlotBool(vm, 0, IsMouseButtonPressed(button));
}

static int getKeyFromKeyname(const char* keyname)
{
	// I'm so sorry
	if (!strcmp(keyname, "q"        )) return KEY_Q;
	if (!strcmp(keyname, "w"        )) return KEY_W;
	if (!strcmp(keyname, "e"        )) return KEY_E;
	if (!strcmp(keyname, "r"        )) return KEY_R;
	if (!strcmp(keyname, "t"        )) return KEY_T;
	if (!strcmp(keyname, "y"        )) return KEY_Y;
	if (!strcmp(keyname, "u"        )) return KEY_U;
	if (!strcmp(keyname, "i"        )) return KEY_I;
	if (!strcmp(keyname, "o"        )) return KEY_O;
	if (!strcmp(keyname, "p"        )) return KEY_P;
	if (!strcmp(keyname, "a"        )) return KEY_A;
	if (!strcmp(keyname, "s"        )) return KEY_S;
	if (!strcmp(keyname, "d"        )) return KEY_D;
	if (!strcmp(keyname, "f"        )) return KEY_F;
	if (!strcmp(keyname, "g"        )) return KEY_G;
	if (!strcmp(keyname, "h"        )) return KEY_H;
	if (!strcmp(keyname, "j"        )) return KEY_J;
	if (!strcmp(keyname, "k"        )) return KEY_K;
	if (!strcmp(keyname, "l"        )) return KEY_L;
	if (!strcmp(keyname, "z"        )) return KEY_Z;
	if (!strcmp(keyname, "x"        )) return KEY_X;
	if (!strcmp(keyname, "c"        )) return KEY_C;
	if (!strcmp(keyname, "v"        )) return KEY_V;
	if (!strcmp(keyname, "b"        )) return KEY_B;
	if (!strcmp(keyname, "n"        )) return KEY_N;
	if (!strcmp(keyname, "m"        )) return KEY_M;

	if (!strcmp(keyname, "0"        )) return KEY_ZERO;
	if (!strcmp(keyname, "1"        )) return KEY_ONE;
	if (!strcmp(keyname, "2"        )) return KEY_TWO;
	if (!strcmp(keyname, "3"        )) return KEY_THREE;
	if (!strcmp(keyname, "4"        )) return KEY_FOUR;
	if (!strcmp(keyname, "5"        )) return KEY_FIVE;
	if (!strcmp(keyname, "6"        )) return KEY_SIX;
	if (!strcmp(keyname, "7"        )) return KEY_SEVEN;
	if (!strcmp(keyname, "8"        )) return KEY_EIGHT;
	if (!strcmp(keyname, "9"        )) return KEY_NINE;

	if (!strcmp(keyname, "up"       )) return KEY_UP;
	if (!strcmp(keyname, "down"     )) return KEY_DOWN;
	if (!strcmp(keyname, "left"     )) return KEY_LEFT;
	if (!strcmp(keyname, "right"    )) return KEY_RIGHT;
	if (!strcmp(keyname, "space"    )) return KEY_SPACE;

	if (!strcmp(keyname, "lshift"   )) return KEY_LEFT_SHIFT;
	if (!strcmp(keyname, "rshift"   )) return KEY_RIGHT_SHIFT;
	if (!strcmp(keyname, "lctrl"    )) return KEY_LEFT_CONTROL;
	if (!strcmp(keyname, "rctrl"    )) return KEY_RIGHT_CONTROL;
	if (!strcmp(keyname, "tab"      )) return KEY_TAB;
	if (!strcmp(keyname, "enter"    )) return KEY_ENTER;
	if (!strcmp(keyname, "return"   )) return KEY_ENTER;
	if (!strcmp(keyname, "escape"   )) return KEY_ESCAPE;
	if (!strcmp(keyname, "backspace")) return KEY_BACKSPACE;
	if (!strcmp(keyname, "delete"   )) return KEY_DELETE;
	return KEY_NULL;
}

defineForeignMethod(keyDown)
{
	if (!txtEnsureType(vm, 1, WREN_TYPE_STRING)) return;

	const char* keyname = wrenGetSlotString(vm, 1);
	int key = getKeyFromKeyname(keyname);
	if (key == KEY_NULL) txtThrowErr(vm, "unknown key name");

	wrenSetSlotBool(vm, 0, IsKeyDown(key));
}

defineForeignMethod(keyPressed)
{
	if (!txtEnsureType(vm, 1, WREN_TYPE_STRING)) return;

	const char* keyname = wrenGetSlotString(vm, 1);
	int key = getKeyFromKeyname(keyname);
	if (key == KEY_NULL) txtThrowErr(vm, "unknown key name");

	wrenSetSlotBool(vm, 0, IsKeyPressed(key));
}

defineForeignMethod(charPressed)
{
	int chr = GetCharPressed();
	if (chr == 0)
	{
		wrenSetSlotString(vm, 0, "");
		return;
	}

	int size;
	const char* utf8char = CodepointToUTF8(chr, &size);
	wrenSetSlotBytes(vm, 0, utf8char, size);
}

defineForeignMethod(anyKeyPressed) { wrenSetSlotBool(vm, 0, GetKeyPressed()); }

defineForeignMethod(exit) { G.close = true; }

WrenForeignMethodFn bindTxtMethods(WrenVM* vm, const char* module, const char* className,
                                   bool isStatic, const char* signature)
{
	int params = getSignatureParams(signature);

	getSetForeign(width);
	getSetForeign(height);
	foreignMethod(title);

	getSetForeign(size);
	foreignMethod(move);
	foreignMethod(fontSize);

	if (strstr(signature, "clear"))
	{
		if (params == 0) return txtApi_clearSpaces;
		return txtApi_clear;
	}

	privateForeignMethod(write);

	foreignMethod(read);
	foreignMethod(charInfo);
	foreignMethod(color);
	foreignMethod(bgColor);
	foreignMethod(mousePos);
	foreignMethod(mouseDown);
	foreignMethod(mousePressed);
	foreignMethod(keyDown);

	if (strstr(signature, "keyPressed"))
	{
		if (params == 0) return txtApi_anyKeyPressed;
		return txtApi_keyPressed;
	}

	foreignMethod(charsPressed);
	foreignMethod(exit);

	return NULL;
}
