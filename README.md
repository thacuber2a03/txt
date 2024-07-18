# txt
a text based game framework, sorta like [cel7](https://rxi.itch.io/cel7) but not quite. it uses [Wren](https://wren.io/), the font is unchangeable and I didn't want to design a palette so you can use rgb (0-255)

## how to use

download latest txt version from releases

run `txt` in the command line, pass a file name to run from that

tada.wav

## games made with it

### shiftword
![image](https://github.com/user-attachments/assets/3678a830-3717-4382-8503-79aab9771f31)

a little game of shifting letters to make words

https://ppepp.itch.io/shiftword

## doc

you must make a 'Game' class with a `new` constructor and an `update` method that must also take in a parameter for the delta time

all of txt's behavior is within the `TXT` foreign class.

### window methods

note: all window position/size related methods are measured in cells

| method | description |
| :-: | :-: |
| `width`        | returns window width                                  |
| `width=(w)`    | sets window width to `w`                              |
| `height`       | returns window height                                 |
| `height=(h)`   | sets window height to `h`                             |
| `size`         | returns window size as a list                         |
| `size(w,h)`    | shorter version of calling `width(w)` and `height(h)` |
| `size(s)`      | sets window size to `s`, s must be a list             |
| `title=(t)`    | sets window title to `t`                              |
| `move(x,y)`    | moves window by `x`, `y`                              |
| `move(p)`      | shorter version of `move(x, y)`, takes a list         |
| `fontSize(px)` | sets font size to `px`                                |
| `exit()`       | exits txt                                             |

### drawing methods

| method | description |
| :-: | :-: |
| `clear(char)`      | clears screen with character `char`                                  |
| `write(x,y,value)` | writes `value` at position `x`, `y`                                  |
| `write(p,value)`   | version of `write` that takes in a list                              |
| `read(x,y)`        | reads character at position `x`, `y`                                 |
| `read(p)`          | version of `read` that takes in a list                               |
| `color(r,g,b)`     | sets color for new characters to `r`, `g`, `b`                       |
| `color=(g)`        | list version of `color(r,g,b)`, set to a num for grayscale instead   |
| `bgColor(r,g,b)`   | same as `color(r,g,b)` but sets background color instead             |
| `bgColor=(g)`      | list version of `bgColor(r,g,b)`, set to a num for grayscale instead |

note: both the current color and current background color will reset after the end of the `update` method

### input methods

| method | description |
| :-: | :-: |
| `mousePos`          | returns mouse position in cell coordinates as list                       |
| `mouseDown(btn)`    | checks if mouse button `btn` is down                                     |
| `mousePressed(btn)` | checks if mouse button `btn` was just pressed                            |
| `keyDown(key)`      | checks if keyboard key `key` is down                                     |
| `keyPressed(key)`   | checks if keyboard key `key` was just pressed                            |
| `keyPressed`        | returns whether *any* key has been pressed                               |
| `charsPressed`      | returns the character just pressed, call multiple times for chars queued |

allowed key names are a-z, 0-9, arrow keys (`right`, `left`, `up`, `down`), `space` and `escape`

allowed mouse button names are `left`, `middle` and `right`

### miscellaneous

| method | description |
| :-: | :-: |
| `version` | the release version, as a [semver](https://semver.org/) string |

## build

Windows:

run the `build.bat` file in the root directory

if you're more of a Makefile person you can also run `mingw32-make`

Linux:

run `make`

## attributions

made with [raylib](https://raylib.com/) by [Ramón Santamaria (@raysan5)](https://twitter.com/raysan5) and [Wren](https://wren.io) by [Robert Nystrom (@munificent)](https://stuffwithstuff.com/)

font is a recreation of IBM's CGA font: https://int10h.org/oldschool-pc-fonts/fontlist/font?ibm_cga
