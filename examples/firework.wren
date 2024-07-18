// check examples/mouse.wren
import "txt" for TXT as T
import "random" for Random

class Firework {
	static size { 8 }
	static res { 25 }

	construct new(x, y) {
		_x = x
		_y = y
		_r = 0

		var r = Random.new()
		_c = [r.int(255), r.int(255), r.int(255)]
	}

	update(dt) {
		if (_r >= 1) return
		_r = _r + dt*2
		// quad ease out
		var r = (1-(1-_r)*(1-_r)) * Firework.size

		for (i in 0...Firework.res) {
			var t = i/Firework.res * Num.tau
			T.color = _c
			T.write(_x+t.cos*r, _y+t.sin*r, "#")
		}
	}
}

