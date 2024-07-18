// you can always rename the import if it's too annoying :)
import "txt" for TXT as T
import "examples/firework" for Firework

class Game {
	construct new() {
		T.title = "fireworks"
		_e = []
	}

	update(dt) {
		var m = T.mousePos

		T.clear()
		T.write(0, 0, m)
		T.color = [255, 0, 0]
		T.write(m, "X")
		T.color = 255

		for (e in _e) e.update(dt)

		if (T.mousePressed("left")) _e.add(Firework.new(m[0], m[1]))
	}
}
