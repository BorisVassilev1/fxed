#pragma once

#include <memory>

#include "nri.hpp"
#include "packing.hpp"
#include "utils.hpp"
#include "font_data.hpp"

namespace fxed {

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
	std::pair<GlyphBox, int> getGlyphBox(uint32_t c, uint32_t size) const;
	friend class FontAtlas;
};

class FontAtlas {
	struct FontData;

	FontData *data;

	std::unique_ptr<nri::Image2D>	 image;
	std::unique_ptr<nri::Allocation> imageAllocation;
	std::unique_ptr<nri::ImageView>	 imageView;
	FontFallbackChain				 fallbackChain;
	nri::NRI						&nri;
	nri::CommandQueue				&q;
	uint32_t						 fontSize = 48;
	bool atlasChanged = false;

	int addGlyphToAtlas(uint32_t c);
	void uploadAtlasToGPU();

   public:
	DELETE_COPY_AND_ASSIGNMENT(FontAtlas);
	FontAtlas(FontAtlas &&other)
		: data(other.data),
		  image(std::move(other.image)),
		  imageAllocation(std::move(other.imageAllocation)),
		  imageView(std::move(other.imageView)),
		  fallbackChain(std::move(other.fallbackChain)),
		  nri(other.nri),
		  q(other.q),
		  fontSize(other.fontSize) {
		other.data = nullptr;
	}

	FontAtlas(nri::NRI &nri, nri::CommandQueue &q, FontFallbackChain &&fallbackChain, uint32_t atlasSize,
			  uint32_t fontSize = 48);
	~FontAtlas();

	void resize(uint32_t newSize);
	void syncWithGPU();

	auto getHandle() { return imageView->getHandle(); }

	GlyphBox getGlyphBox(uint32_t c);
	int		 getAtlasSize() const { return image->getWidth(); }

	static std::string getDefaultSystemFontPath();
	static std::string findFontPath(std::string_view fontName);
};

}	  // namespace fxed
