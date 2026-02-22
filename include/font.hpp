#pragma once

#include <memory>

#include "nri.hpp"
#include "utils.hpp"

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

class FontFallbackChain {
	struct FontFallbackChainData;

	FontFallbackChainData *data;

   public:
	FontFallbackChain(const std::vector<std::string_view> &fonts);
	DELETE_COPY_AND_ASSIGNMENT(FontFallbackChain);
	FontFallbackChain(FontFallbackChain &&other) {
		data	   = other.data;
		other.data = nullptr;
	}
	~FontFallbackChain();

	/// returns the GlyphBox and the index of the font in the fallback chain that contains the glyph for the given
	/// codepoint, throws if not found
	std::pair<GlyphBox, int> getGlyphBox(uint32_t c) const;
	friend class FontAtlas;
};

class FontAtlas {
	struct FontData;

	FontData *data;

	std::unique_ptr<nri::Image2D>	 image;
	std::unique_ptr<nri::Allocation> imageAllocation;
	std::unique_ptr<nri::ImageView>	 imageView;
	FontFallbackChain				 fallbackChain;

   public:
	DELETE_COPY_AND_ASSIGNMENT(FontAtlas);
	FontAtlas(FontAtlas &&other)
		: data(other.data),
		  image(std::move(other.image)),
		  imageAllocation(std::move(other.imageAllocation)),
		  imageView(std::move(other.imageView)),
		  fallbackChain(std::move(other.fallbackChain)) {
		other.data = nullptr;
	}

	FontAtlas(nri::NRI &nri, nri::CommandQueue &q, FontFallbackChain &&fallbackChain, uint32_t atlasSize);
	~FontAtlas();

	auto getHandle() { return imageView->getHandle(); }

	GlyphBox getGlyphBox(uint32_t c) const;
	int		 getAtlasSize() const { return image->getWidth(); }

	static std::string getDefaultSystemFontPath();
	static std::string findFontPath(std::string_view fontName);
};

}	  // namespace fxed
