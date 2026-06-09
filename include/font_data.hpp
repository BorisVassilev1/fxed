#pragma once

namespace fxed {

struct Rectangle {
	int x, y, w, h;
};

struct GlyphBox {
	struct {
		float l, b, r, t;
	} bounds;
	Rectangle rect;
	int	   index;
	float advance;
	bool isBitmap;
};

};	   // namespace fxed
