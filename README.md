# txt
a text based game framework, sorta like [cel7](https://rxi.itch.io/cel7) but not quite. it uses [Wren](https://wren.io/), the font is unchangeable and I didn't want to design a palette so you can use rgb (0-255)

## doc

you must make a 'Game' class with a `new` constructor and an `update` method that must also take in a parameter for the delta time.

all of txt's behavior is within the `TXT` foreign class and all position/size related methods are measured in cells

### window methods

| method | description |
| :-: | :-: |
| width()   | returns window width                             |
| width(w)  | sets window width to `w`                         |
| height()  | returns window height                            |
| height(h) | sets window height to `h`                        |
| size()    | returns window size as a list                    |
| size(w,h) | compressed version of `width(w)` and `height(h)` |
| size(s)   | sets window size to `s`, s must be a list        |
| title(t)  | sets window title to `t`                         |
| move(x,y) | moves window by `x`, `y`                         |
| exit()    | exits txt                                        |

### drawing methods

| method | description |
| :-: | :-: |
| clear(char)        | clears screen with character `char`                      |
| write(x,y,text)    | writes `text` at position `x`, `y`                       |
| read(x,y)          | reads character at position `x`, `y`                     |
| color(r,g,b)       | sets color for new characters to `r`, `g`, `b`           |
| color(g)           | grayscale version of `color(r,g,b)`                      |
| bgColor(r,g,b)     | same as `color(r,g,b)` but sets background color instead |
| bgColor(g)         | grayscale version of `bgColor(r,g,b)`                    |

### input methods

| method | description |
| :-: | :-: |
| mousePos()         | returns mouse position in cell coordinates as list       |
| mouseDown(btn)     | checks if mouse button `btn` is down                     |
| mousePressed(btn)  | checks if mouse button `btn` was just pressed            |
| keyDown(key)       | checks if keyboard key `key` is down                     |
| keyPressed(key)    | checks if keyboard key `key` was just pressed            |
| charPressed(char)  | returns character just pressed. (can be utf-8)           |

allowed key names are a-z, 0-9, arrow keys (`right`, `left`, `up`, `down`), `space` and `escape`

allowed mouse button names are `left`, `middle` and `right`

## build

run the `build.bat` file in the root directory

it currently only supports 64-bit windows, I don't know how to compile for other platforms

if you're more of a Makefile person you can also run `mingw32-make`

## attributions

made with [raylib](https://raylib.com/) by [Ramón Santamaria (@raysan5)](https://twitter.com/raysan5) and [Wren](https://wren.io) by [Robert Nystrom (@munificent)](https://stuffwithstuff.com/)

font is Press Start 2P
