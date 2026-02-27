#pragma once

namespace fxed {

struct Rectangle {
	int x, y, w, h;
};

struct GlyphBox {
	int	   index;
	double advance;
	struct {
		double l, b, r, t;
	} bounds;
	Rectangle rect;
};

};	   // namespace fxed
