#include "txt.h"

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof(arr[0]))

#define defineForeignMethod(name) static void txt##name(WrenVM* vm)
#define foreignMethod(name) if (strstr(signature, #name)) return txt##name
#define getSetForeign(name) \
	if (strstr(signature, #name)) \
	{ \
		if (params == 1) return txtset##name; \
		return txtget##name;\
	}

const char* txtClass = "\
class TXT { \n\
	foreign static width() \n\
	static width { TXT.width() } \n\
	foreign static width(w) \n\
	foreign static height() \n\
	static height { TXT.height() } \n\
	foreign static height(h) \n\
	static size { TXT.size() } \n\
	foreign static size(s) \n\
	foreign static size(w,h) \n\
	foreign static move(x,y) \n\
	foreign static title(t) \n\
	foreign static fontSize(px) \n\
	foreign static exit() \n\
\n\
	foreign static clear(char) \n\
	foreign static clear() \n\
	static write(x, y, value) { write_(x, y, value.toString) } \n\
	foreign static write_(x,y,text) \n\
	foreign static read(x, y) \n\
	foreign static color(r,g,b) \n\
	foreign static color(g) \n\
	foreign static bgColor(r,g,b) \n\
	foreign static bgColor(g) \n\
\n\
	foreign static mousePos() \n\
	static mousePos { TXT.mousePos() } \n\
	foreign static mouseDown(button) \n\
	foreign static mousePressed(button) \n\
	foreign static keyDown(key) \n\
	foreign static keyPressed(key) \n\
	foreign static charPressed() \n\
	static charPressed { TXT.charPressed() } \n\
}\
";

static char* typenames[] =
{
	[WREN_TYPE_NUM] = "number",
	[WREN_TYPE_BOOL] = "bool",
	[WREN_TYPE_STRING] = "string",
	[WREN_TYPE_NULL] = "null",
	[WREN_TYPE_LIST] = "list",
	[WREN_TYPE_MAP] = "map",
	[WREN_TYPE_UNKNOWN] = "unknown",
	[WREN_TYPE_FOREIGN] = "foreign",
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

static void txtThrowErr(const char* msg, ...)
{
	char err[256];

	va_list args;
	va_start(args, msg);
	vsprintf(err, msg, args);
	va_end(args);

	wrenEnsureSlots(G.vm, 1);
	wrenSetSlotString(G.vm, 0, err);
	wrenAbortFiber(G.vm, 0);
}

static void txtEnsureType(int slot, WrenType type)
{
	WrenType actualType = wrenGetSlotType(G.vm, slot);
	if (actualType != type)
		txtThrowErr("expected %s, got %s", typenames[type], typenames[actualType]);
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
	cell %= G.totalCells;
	int i = cell*CELL_SIZE;
	G.screen[i] = c;

	G.screen[i+1] = G.currentColor.r;
	G.screen[i+2] = G.currentColor.g;
	G.screen[i+3] = G.currentColor.b;

	G.screen[i+4] = G.currentBgColor.r;
	G.screen[i+5] = G.currentBgColor.g;
	G.screen[i+6] = G.currentBgColor.b;
}

static void txtWriteChar(int x, int y, char c)
{
	txtWriteCharIdx((y*G.consoleSize.x+x), c);
}

static char txtGetCharIdx(int cell)
{
	cell %= G.totalCells;
	return G.screen[cell*CELL_SIZE];
}

static char txtGetChar(int x, int y)
{
	return txtGetCharIdx(y*G.consoleSize.x+x);
}

static void txtResize(uint8_t w, uint8_t h)
{
	G.consoleSize.x = w;
	G.consoleSize.y = h;
	G.totalCells = w*h;
	G.screen = realloc(G.screen, G.totalCells*CELL_SIZE);

	SetWindowSize(
		G.fontSize*G.consoleSize.x,
		G.fontSize*G.consoleSize.y
	);
}

defineForeignMethod(setwidth)
{
	txtEnsureType(1, WREN_TYPE_NUM);
	txtResize(wrenGetSlotDouble(G.vm, 1), G.consoleSize.y);
}

defineForeignMethod(getwidth) { wrenSetSlotDouble(vm, 0, G.consoleSize.x); }

defineForeignMethod(setheight)
{
	txtEnsureType(1, WREN_TYPE_NUM);
	txtResize(G.consoleSize.x, wrenGetSlotDouble(G.vm, 1));
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
	txtEnsureType(1, WREN_TYPE_NUM);
	txtEnsureType(2, WREN_TYPE_NUM);

	txtResize(
		wrenGetSlotDouble(vm, 1),
		wrenGetSlotDouble(vm, 2)
	);
}

defineForeignMethod(setsizelist)
{
	txtEnsureType(1, WREN_TYPE_LIST);

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
	txtEnsureType(1, WREN_TYPE_NUM);
	txtEnsureType(2, WREN_TYPE_NUM);

	int dx = wrenGetSlotDouble(G.vm, 1);
	int dy = wrenGetSlotDouble(G.vm, 2);

	Vector2 pos = GetWindowPosition();
	SetWindowPosition(
		pos.x+dx*G.fontSize,
		pos.y+dy*G.fontSize
	);
}

defineForeignMethod(title)
{
	txtEnsureType(1, WREN_TYPE_STRING);
	SetWindowTitle(wrenGetSlotString(vm, 1));
}

defineForeignMethod(fontSize)
{
	txtEnsureType(1, WREN_TYPE_NUM);
	G.fontSize = wrenGetSlotDouble(vm, 1);
	txtResize(G.consoleSize.x, G.consoleSize.y);
}

defineForeignMethod(clear)
{
	txtEnsureType(1, WREN_TYPE_STRING);

	const char* chr = wrenGetSlotString(G.vm, 1);
	for (int i = 0; i < G.totalCells; i++) txtWriteCharIdx(i, *chr);
}

defineForeignMethod(clearSpaces)
{
	for (int i = 0; i < G.totalCells; i++) txtWriteCharIdx(i, ' ');
}

defineForeignMethod(write)
{
	txtEnsureType(1, WREN_TYPE_NUM);
	txtEnsureType(2, WREN_TYPE_NUM);

	int x = wrenGetSlotDouble(vm, 1);
	int y = wrenGetSlotDouble(vm, 2);
	const char* text = wrenGetSlotString(vm, 3);

	for (int i = 0; i < strlen(text); i++) txtWriteChar(x+i, y, text[i]);
}

defineForeignMethod(read)
{
	txtEnsureType(1, WREN_TYPE_NUM);
	txtEnsureType(2, WREN_TYPE_NUM);

	int x = wrenGetSlotDouble(G.vm, 1);
	int y = wrenGetSlotDouble(G.vm, 2);
	const char c = txtGetChar(x, y);

	wrenSetSlotBytes(G.vm, 0, &c, 1);
}

defineForeignMethod(color)
{
	txtEnsureType(1, WREN_TYPE_NUM);
	txtEnsureType(2, WREN_TYPE_NUM);
	txtEnsureType(3, WREN_TYPE_NUM);

	G.currentColor.r = wrenGetSlotDouble(vm, 1);
	G.currentColor.g = wrenGetSlotDouble(vm, 2);
	G.currentColor.b = wrenGetSlotDouble(vm, 3);
}

defineForeignMethod(grayscale)
{
	txtEnsureType(1, WREN_TYPE_NUM);

	int g = wrenGetSlotDouble(vm, 1);
	G.currentColor.r = g;
	G.currentColor.g = g;
	G.currentColor.b = g;
}

defineForeignMethod(bgColor)
{
	txtEnsureType(1, WREN_TYPE_NUM);
	txtEnsureType(2, WREN_TYPE_NUM);
	txtEnsureType(3, WREN_TYPE_NUM);

	G.currentBgColor.r = wrenGetSlotDouble(vm, 1);
	G.currentBgColor.g = wrenGetSlotDouble(vm, 2);
	G.currentBgColor.b = wrenGetSlotDouble(vm, 3);
}

defineForeignMethod(bgGrayscale)
{
	txtEnsureType(1, WREN_TYPE_NUM);

	int g = wrenGetSlotDouble(vm, 1);
	G.currentBgColor.r = g;
	G.currentBgColor.g = g;
	G.currentBgColor.b = g;
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
	txtEnsureType(1, WREN_TYPE_STRING);

	const char* buttonName = wrenGetSlotString(vm, 1);
	int button = -1;
	if (!strcasecmp(buttonName, "left"))   button = MOUSE_BUTTON_LEFT;
	if (!strcasecmp(buttonName, "right"))  button = MOUSE_BUTTON_RIGHT;
	if (!strcasecmp(buttonName, "middle")) button = MOUSE_BUTTON_MIDDLE;
	if (button == -1) txtThrowErr("unknown button name");

	wrenEnsureSlots(vm, 1);
	wrenSetSlotBool(vm, 0, IsMouseButtonDown(button));
}

defineForeignMethod(mousePressed)
{
	txtEnsureType(1, WREN_TYPE_STRING);

	const char* buttonName = wrenGetSlotString(vm, 1);
	int button = -1;
	if (!strcasecmp(buttonName, "left"))   button = MOUSE_BUTTON_LEFT;
	if (!strcasecmp(buttonName, "right"))  button = MOUSE_BUTTON_RIGHT;
	if (!strcasecmp(buttonName, "middle")) button = MOUSE_BUTTON_MIDDLE;
	if (button == -1) txtThrowErr("unknown button name");

	wrenEnsureSlots(vm, 1);
	wrenSetSlotBool(vm, 0, IsMouseButtonPressed(button));
}

static int getKeyFromKeyname(const char* keyname)
{
	// I'm so sorry
	if (!strcasecmp(keyname, "q"     )) return KEY_Q;
	if (!strcasecmp(keyname, "w"     )) return KEY_W;
	if (!strcasecmp(keyname, "e"     )) return KEY_E;
	if (!strcasecmp(keyname, "r"     )) return KEY_R;
	if (!strcasecmp(keyname, "t"     )) return KEY_T;
	if (!strcasecmp(keyname, "y"     )) return KEY_Y;
	if (!strcasecmp(keyname, "u"     )) return KEY_U;
	if (!strcasecmp(keyname, "i"     )) return KEY_I;
	if (!strcasecmp(keyname, "o"     )) return KEY_O;
	if (!strcasecmp(keyname, "p"     )) return KEY_P;
	if (!strcasecmp(keyname, "a"     )) return KEY_A;
	if (!strcasecmp(keyname, "s"     )) return KEY_S;
	if (!strcasecmp(keyname, "d"     )) return KEY_D;
	if (!strcasecmp(keyname, "f"     )) return KEY_F;
	if (!strcasecmp(keyname, "g"     )) return KEY_G;
	if (!strcasecmp(keyname, "h"     )) return KEY_H;
	if (!strcasecmp(keyname, "j"     )) return KEY_J;
	if (!strcasecmp(keyname, "k"     )) return KEY_K;
	if (!strcasecmp(keyname, "l"     )) return KEY_L;
	if (!strcasecmp(keyname, "z"     )) return KEY_Z;
	if (!strcasecmp(keyname, "x"     )) return KEY_X;
	if (!strcasecmp(keyname, "c"     )) return KEY_C;
	if (!strcasecmp(keyname, "v"     )) return KEY_V;
	if (!strcasecmp(keyname, "b"     )) return KEY_B;
	if (!strcasecmp(keyname, "n"     )) return KEY_N;
	if (!strcasecmp(keyname, "m"     )) return KEY_M;

	if (!strcasecmp(keyname, "0"     )) return KEY_ZERO;
	if (!strcasecmp(keyname, "1"     )) return KEY_ONE;
	if (!strcasecmp(keyname, "2"     )) return KEY_TWO;
	if (!strcasecmp(keyname, "3"     )) return KEY_THREE;
	if (!strcasecmp(keyname, "4"     )) return KEY_FOUR;
	if (!strcasecmp(keyname, "5"     )) return KEY_FIVE;
	if (!strcasecmp(keyname, "6"     )) return KEY_SIX;
	if (!strcasecmp(keyname, "7"     )) return KEY_SEVEN;
	if (!strcasecmp(keyname, "8"     )) return KEY_EIGHT;
	if (!strcasecmp(keyname, "9"     )) return KEY_NINE;

	if (!strcasecmp(keyname, "up"    )) return KEY_UP;
	if (!strcasecmp(keyname, "down"  )) return KEY_DOWN;
	if (!strcasecmp(keyname, "left"  )) return KEY_LEFT;
	if (!strcasecmp(keyname, "right" )) return KEY_RIGHT;
	if (!strcasecmp(keyname, "space" )) return KEY_SPACE;
	if (!strcasecmp(keyname, "escape")) return KEY_ESCAPE;
	return KEY_NULL;
}

defineForeignMethod(keyDown)
{
	txtEnsureType(1, WREN_TYPE_STRING);

	const char* keyname = wrenGetSlotString(vm, 1);
	int key = getKeyFromKeyname(keyname);
	if (key == KEY_NULL) txtThrowErr("unknown key name");

	wrenSetSlotBool(vm, 0, IsKeyDown(key));
}

defineForeignMethod(keyPressed)
{
	txtEnsureType(1, WREN_TYPE_STRING);

	const char* keyname = wrenGetSlotString(vm, 1);
	int key = getKeyFromKeyname(keyname);
	if (key == KEY_NULL) txtThrowErr("unknown key name");

	wrenSetSlotBool(vm, 0, IsKeyPressed(key));
}

defineForeignMethod(charPressed) {
	int chr = GetCharPressed();
	int size;
	char* utf8char = CodepointToUTF8(chr, &size);
	wrenSetSlotBytes(vm, 0, utf8char, size);
}

defineForeignMethod(exit) { G.close = true; }

WrenForeignMethodFn bindTxtMethods(WrenVM* vm, const char* module, const char* className,
                                   bool isStatic, const char* signature)
{
	int params = getSignatureParams(signature);

	getSetForeign(width);
	getSetForeign(height);
	if (strstr(signature, "title")) return txttitle;

	if (strstr(signature, "size"))
	{
		if (params == 2) return txtsetsize;
		else if (params == 1) return txtsetsizelist;
		return txtgetsize;
	}

	if (strstr(signature, "move"))
	{
		if (params == 2) return txtmove;
		//if (params == 1) return txtmovelist;
		return NULL;
	}

	foreignMethod(fontSize);

	if (strstr(signature, "clear"))
	{
		if (params == 0) return txtclearSpaces;
		return txtclear;
	}

	if (strstr(signature, "write_")) return txtwrite;

	foreignMethod(read);

	if (strstr(signature, "color"))
	{
		if (params == 1) return txtgrayscale;
		return txtcolor;
	}

	if (strstr(signature, "bgColor"))
	{
		if (params == 1) return txtbgGrayscale;
		return txtbgColor;
	}

	foreignMethod(mousePos);
	foreignMethod(mouseDown);
	foreignMethod(mousePressed);
	foreignMethod(keyDown);
	foreignMethod(keyPressed);
	foreignMethod(charPressed);
	foreignMethod(exit);
}
