import "txt" for TXT
import "random" for Random

class Game {
	construct new() {
		TXT.title = "hello world!"
		TXT.size = [15, 10]
		TXT.fontSize = 32
	}

	centerWrite(text, off) {
		TXT.write((TXT.width-text.count)/2, TXT.height/2+off, text)
	}

	update(dt) {
		if (TXT.keyPressed("escape")) TXT.exit()

		TXT.color = 60
		TXT.clear(".")
		var msg = "hi there!"
		var foot = "=" * msg.count
		TXT.color = 255
		centerWrite(msg, -1)
		centerWrite(foot, 0)
		centerWrite(TXT.version, 0)
	}
}
