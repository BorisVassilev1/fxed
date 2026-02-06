#pragma once

#include <memory>

#include "nri.hpp"
#include "utils.hpp"

namespace fxed {
class Font {
	struct FontData;

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

	FontData *data;

	std::unique_ptr<nri::Image2D>	 image;
	std::unique_ptr<nri::Allocation> imageAllocation;
	std::unique_ptr<nri::ImageView>	 imageView;

   public:
	DELETE_COPY_AND_ASSIGNMENT(Font);

	Font(nri::NRI &nri, nri::CommandQueue &q, const char *fontfilename, uint32_t atlasSize);
	~Font();

	auto getHandle() { return imageView->getHandle(); }

	GlyphBox getGlyphBox(uint32_t c) const;
	int getAtlasSize() const { return image->getWidth(); }

	static std::string getDefaultSystemFontPath();
};
}	  // namespace fxed
