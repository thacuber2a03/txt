import "txt" for TXT

class Game {
	construct new() {
		TXT.size(16, 16)
	}

	update(dt) {
		TXT.color(100)
		TXT.clear(".")
		var msg = "Hello world!"
		TXT.color(255)
		TXT.write(TXT.width()/2-msg.count/2, TXT.height()/2, msg)
	}
}
