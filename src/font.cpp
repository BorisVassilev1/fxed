#include "font.hpp"
#include <format>

#include "buffer_utils.hpp"
#include "nri.hpp"
#include "packing.hpp"
#include "static_vector.hpp"
#include "utils.hpp"

#include "ft2build.h"
#include FT_FREETYPE_H

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

using namespace fxed;

// if uint is not a valid type, define it as unsigned int
using uint = unsigned int;

// copy pasta from msdfgen
/// Reference to a 2D image bitmap or a buffer acting as one. Pixel storage not owned or managed by the object.
template <typename T, int N = 1>
struct BitmapRef {
	T	*pixels;
	uint width, height;

	inline BitmapRef() : pixels(NULL), width(0), height(0) {}
	inline BitmapRef(T *pixels, int width, int height) : pixels(pixels), width(width), height(height) {}

	inline T *operator()(int x, int y) const { return pixels + N * (width * y + x); }
};

// copy pasta from msdfgen Constant reference to a 2D image bitmap or a buffer acting as one. Pixel storage not owned or
// managed by the object.
template <typename T, int N = 1>
struct BitmapConstRef {
	const T *pixels;
	uint	 width, height;

	inline BitmapConstRef() : pixels(NULL), width(0), height(0) {}
	inline BitmapConstRef(const T *pixels, int width, int height) : pixels(pixels), width(width), height(height) {}
	inline BitmapConstRef(const BitmapRef<T, N> &orig) : pixels(orig.pixels), width(orig.width), height(orig.height) {}

	inline const T *operator()(int x, int y) const { return pixels + N * (width * y + x); }
};

class ImageAtlasStorage {
	mutable void *data = nullptr;
	uint32_t	  width;
	uint32_t	  height;

	static const nri::Format format = nri::Format::FORMAT_R32G32B32A32_SFLOAT;
	static const int		 N		= 4;
	using T							= float;

   public:
	ImageAtlasStorage(uint32_t width, uint32_t height, void *data) : data(data), width(width), height(height) {}

	void put(int x, int y, const BitmapConstRef<float, 4> &subBitmap) {
		assert(data != nullptr);
		assert(x + subBitmap.width <= width && y + subBitmap.height <= height);		// FIX BOUNDS CHECK
		for (uint j = 0; j < subBitmap.height; ++j) {
			for (uint i = 0; i < subBitmap.width; ++i) {
				((T *)data)[(y + j) * width * N + (x * N) + i * N + 0] =
					subBitmap.pixels[j * subBitmap.width * 4 + i * 4 + 0];
				((T *)data)[(y + j) * width * N + (x * N) + i * N + 1] =
					subBitmap.pixels[j * subBitmap.width * 4 + i * 4 + 1];
				((T *)data)[(y + j) * width * N + (x * N) + i * N + 2] =
					subBitmap.pixels[j * subBitmap.width * 4 + i * 4 + 2];
				((T *)data)[(y + j) * width * N + (x * N) + i * N + 3] =
					subBitmap.pixels[j * subBitmap.width * 4 + i * 4 + 3];
			}
		}
	}

	void put_flipy(int x, int y, const BitmapConstRef<float, 3> &subBitmap) {
		assert(data != nullptr);
		assert(x + subBitmap.width <= width && y + subBitmap.height <= height);		// FIX BOUNDS CHECK
		for (uint _j = 0; _j < subBitmap.height; ++_j) {
			for (uint i = 0; i < subBitmap.width; ++i) {
				auto j = subBitmap.height - 1 - _j;
				((T *)data)[(y + _j) * width * N + (x * N) + i * N + 0] =
					subBitmap.pixels[j * subBitmap.width * 3 + i * 3 + 0];
				((T *)data)[(y + _j) * width * N + (x * N) + i * N + 1] =
					subBitmap.pixels[j * subBitmap.width * 3 + i * 3 + 1];
				((T *)data)[(y + _j) * width * N + (x * N) + i * N + 2] =
					subBitmap.pixels[j * subBitmap.width * 3 + i * 3 + 2];
				((T *)data)[(y + _j) * width * N + (x * N) + i * N + 3] = 1.0f;
			}
		}
	}

	void put(int x, int y, const BitmapConstRef<float, 1> &subBitmap) {
		assert(data != nullptr);
		assert(x + subBitmap.width <= width && y + subBitmap.height <= height);		// FIX BOUNDS CHECK
		for (uint j = 0; j < subBitmap.height; ++j) {
			for (uint i = 0; i < subBitmap.width; ++i) {
				((T *)data)[(y + j) * width * N + (x * N) + i * N + 3] = subBitmap.pixels[j * subBitmap.width + i];
			}
		}
	}

	void put(int x, int y, const BitmapConstRef<uint8_t, 1> &subBitmap) {
		assert(data != nullptr);
		assert(x + subBitmap.width <= width && y + subBitmap.height <= height);		// FIX BOUNDS CHECK
		for (uint j = 0; j < subBitmap.height; ++j) {
			for (uint i = 0; i < subBitmap.width; ++i) {
				((T *)data)[(y + j) * width * N + (x * N) + i * N + 3] =
					subBitmap.pixels[j * subBitmap.width + i] / 255.f;
			}
		}
	}

	void put(int x, int y, const BitmapConstRef<uint8_t, 4> &subBitmap) {
		assert(data != nullptr);
		assert(x + subBitmap.width <= width && y + subBitmap.height <= height);		// FIX BOUNDS CHECK
		for (uint j = 0; j < subBitmap.height; ++j) {
			for (uint i = 0; i < subBitmap.width; ++i) {
				((T *)data)[(y + j) * width * N + (x * N) + i * N + 0] =
					subBitmap.pixels[j * subBitmap.width * 4 + i * 4 + 2] / 255.f;
				((T *)data)[(y + j) * width * N + (x * N) + i * N + 1] =
					subBitmap.pixels[j * subBitmap.width * 4 + i * 4 + 1] / 255.f;
				((T *)data)[(y + j) * width * N + (x * N) + i * N + 2] =
					subBitmap.pixels[j * subBitmap.width * 4 + i * 4 + 0] / 255.f;
				((T *)data)[(y + j) * width * N + (x * N) + i * N + 3] =
					subBitmap.pixels[j * subBitmap.width * 4 + i * 4 + 3] / 255.f;
			}
		}
	}

	void get(int x, int y, const BitmapRef<float, 4> &subBitmap) const {
		for (uint j = 0; j < subBitmap.height; ++j) {
			for (uint i = 0; i < subBitmap.width; ++i) {
				subBitmap.pixels[j * subBitmap.width * 4 + i * 4 + 0] =
					((T *)data)[(y + j) * width * N + (x * N) + i * N + 0];
				subBitmap.pixels[j * subBitmap.width * 4 + i * 4 + 1] =
					((T *)data)[(y + j) * width * N + (x * N) + i * N + 1];
				subBitmap.pixels[j * subBitmap.width * 4 + i * 4 + 2] =
					((T *)data)[(y + j) * width * N + (x * N) + i * N + 2];
				subBitmap.pixels[j * subBitmap.width * 4 + i * 4 + 3] =
					((T *)data)[(y + j) * width * N + (x * N) + i * N + 3];
			}
		}
	}

	void get(int x, int y, const BitmapRef<float, 3> &subBitmap) const {
		for (uint j = 0; j < subBitmap.height; ++j) {
			for (uint i = 0; i < subBitmap.width; ++i) {
				subBitmap.pixels[j * subBitmap.width * 3 + i * 3 + 0] =
					((T *)data)[(y + j) * width * N + (x * N) + i * N + 0];
				subBitmap.pixels[j * subBitmap.width * 3 + i * 3 + 1] =
					((T *)data)[(y + j) * width * N + (x * N) + i * N + 1];
				subBitmap.pixels[j * subBitmap.width * 3 + i * 3 + 2] =
					((T *)data)[(y + j) * width * N + (x * N) + i * N + 2];
			}
		}
	}

	void get(int x, int y, const BitmapRef<float, 1> &subBitmap) const {
		for (uint j = 0; j < subBitmap.height; ++j) {
			for (uint i = 0; i < subBitmap.width; ++i) {
				subBitmap.pixels[j * subBitmap.width + i] = ((T *)data)[(y + j) * width * N + (x * N) + i * N + 3];
			}
		}
	}

	void *getDataAt(int x, int y) const {
		assert(data != nullptr);
		return (void *)((T *)data + (y * width * N + x * N));
	}

	uint32_t getStride() const { return width * N * sizeof(T); }
};

struct FontAtlas::FontData {
	StaticVector<std::pair<GlyphBox, int>> glyphBoxes;
	std::unordered_map<uint32_t, int>	   codepointToGlyphBoxIndex;
	RowAtlasPacker						   atlasPacker;
	ImageAtlasStorage					   atlasStorage;

	FontData(uint32_t atlasSize, uint32_t fontSize, void *imageData, void *glyphGeometryData, std::size_t maxGlyphCount)
		: glyphBoxes(glyphGeometryData, maxGlyphCount),
		  codepointToGlyphBoxIndex(std::unordered_map<uint32_t, int>()),
		  atlasPacker(RowAtlasPacker(atlasSize, atlasSize, fontSize + 2)),
		  atlasStorage(ImageAtlasStorage(atlasSize, atlasSize, imageData)) {}
};

struct FontFallbackChain::FontFallbackChainData {
	std::vector<FT_Face> fontFaces;
};

int FontAtlas::addGlyphToAtlas(uint32_t c) {
	try {
		auto *added = data->glyphBoxes.push_back(this->fallbackChain.getGlyphBox(c, fontSize));
		if (!added) {
			dbLog(dbg::LOG_ERROR, "Failed to add glyph for codepoint ", c, " to atlas: max glyph count exceeded");
			return -1;
		}

		auto &box	= added->first;
		auto &index = added->second;
		auto &face	= this->fallbackChain.data->fontFaces[index];

		auto result						  = data->glyphBoxes.size() - 1;
		data->codepointToGlyphBoxIndex[c] = result;

		if (face->glyph->format == FT_GLYPH_FORMAT_BITMAP) {
			uint32_t width	= face->glyph->bitmap.width;
			uint32_t height = face->glyph->bitmap.rows;

			if (height > fontSize) {
				// resize to fontSize x fontSize
				double	 scale	= fontSize / (double)height;
				double	 scale2 = 1.0 / (double)height;
				uint32_t w		= (uint32_t)(width * scale);
				uint32_t h		= (uint32_t)(height * scale);

				auto rect = data->atlasPacker.pack({0, 0, (int)w, (int)h}, 1);
				if (!rect) {
					dbLog(dbg::LOG_ERROR, "Failed to pack glyph for codepoint ", c, " with size ", w, "x", h,
						  " into atlas");
					return -1;
				}
				box.rect = *rect;

				box.bounds.b *= scale2;
				box.bounds.l *= scale2;
				box.bounds.r *= scale2;
				box.bounds.t *= scale2;
				box.advance *= scale2;

				std::vector<uint8_t> resizedBitmap(w * h * 4);

				stbir_resize_uint8_generic(face->glyph->bitmap.buffer, width, height, face->glyph->bitmap.pitch,
										   resizedBitmap.data(), w, h, w * 4, 4, 3, STBIR_FLAG_ALPHA_PREMULTIPLIED,
										   STBIR_EDGE_ZERO, STBIR_FILTER_DEFAULT, STBIR_COLORSPACE_LINEAR, nullptr);
				BitmapConstRef<uint8_t, 4> bitmap(resizedBitmap.data(), w, h);
				data->atlasStorage.put(box.rect.x, box.rect.y, bitmap);

				return result;
			}
		}

		// regular packing
		auto rect = data->atlasPacker.pack(box.rect, 1);
		if (!rect) {
			dbLog(dbg::LOG_ERROR, "Failed to pack glyph for codepoint ", c, " with size ", box.rect.w, "x", box.rect.h,
				  " into atlas");
			return -1;
		}
		box.rect = *rect;

		box.bounds.b /= fontSize;
		box.bounds.l /= fontSize;
		box.bounds.r /= fontSize;
		box.bounds.t /= fontSize;
		box.advance /= fontSize;

		if (face->glyph->format == FT_GLYPH_FORMAT_OUTLINE)
			if (int e = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL)) {
				dbLog(dbg::LOG_ERROR, "Failed to render glyph for codepoint ", c, " error: 0x", std::hex, e, std::dec);
				return -1;
			}

		switch (face->glyph->bitmap.pixel_mode) {
			case FT_PIXEL_MODE_GRAY: {
				BitmapConstRef<uint8_t, 1> bitmap((const uint8_t *)face->glyph->bitmap.buffer,
												  face->glyph->bitmap.width, face->glyph->bitmap.rows);
				data->atlasStorage.put(box.rect.x, box.rect.y, bitmap);
			} break;
			case FT_PIXEL_MODE_BGRA: {
				BitmapConstRef<uint8_t, 4> bitmap((const uint8_t *)face->glyph->bitmap.buffer,
												  face->glyph->bitmap.width, face->glyph->bitmap.rows);
				data->atlasStorage.put(box.rect.x, box.rect.y, bitmap);
			} break;
			default: {
				dbLog(dbg::LOG_ERROR, "Unsupported pixel mode for glyph of codepoint ", c, ": ",
					  (int)face->glyph->bitmap.pixel_mode);
				return -1;
			} break;
		}

		return result;
	} catch (const std::exception &e) {
		dbLog(dbg::LOG_WARNING, "Glyph for codepoint ", c, " not found in any font in the fallback chain: ", e.what());
		return -1;
	}
}

FontAtlas::FontAtlas(nri::NRI &nri, nri::CommandQueue &q, FontFallbackChain &&fallbackChain, uint32_t atlasSize,
					 uint32_t fontSize, uint32_t maxGlyphCount)
	: data(nullptr),
	  fallbackChain(std::move(fallbackChain)),
	  nri(nri),
	  q(q),
	  fontSize(fontSize),
	  maxGlyphCount(maxGlyphCount) {
	image = nri.createImage2D(atlasSize, atlasSize, nri::FORMAT_R32G32B32A32_SFLOAT,
							  nri::IMAGE_USAGE_SAMPLED | nri::IMAGE_USAGE_TRANSFER_DST);
	glyphGeometryBuffer =
		nri.createBuffer(maxGlyphCount * sizeof(GlyphBox), nri::BUFFER_USAGE_STORAGE | nri::BUFFER_USAGE_TRANSFER_DST);

	auto [offsets, memReq] = getBufferOffsets(*image, *glyphGeometryBuffer);
	gpuAllocation		   = allocateBindMemory(nri, nri::MEMORY_TYPE_DEVICE, *image, *glyphGeometryBuffer);

	imageView = image->createTextureView();

	uploadBuffer	 = nri.createBuffer(memReq.size, nri::BUFFER_USAGE_TRANSFER_SRC);
	uploadAllocation = allocateBindMemory(nri, nri::MEMORY_TYPE_UPLOAD, *uploadBuffer);
	char *uploadData = (char *)uploadAllocation->map();

	data = new FontData(atlasSize, fontSize, uploadData + offsets[0], uploadData + offsets[1], maxGlyphCount);

	uploadAtlasToGPU();
}

FontAtlas::~FontAtlas() { delete data; }

void FontAtlas::resize(uint32_t newSize) {
	auto oldSize = fontSize;
	newSize		 = std::min(newSize, 32u);
	if (oldSize == newSize) return;

	dbLog(dbg::LOG_INFO, "glyphboxes count before resize: ", data->glyphBoxes.size());
	data->glyphBoxes.clear();
	data->atlasPacker.setRowHeight(newSize + 2);

	fontSize = newSize;

	for (auto [c, _] : data->codepointToGlyphBoxIndex) {
		addGlyphToAtlas(c);
	}
	dbLog(dbg::LOG_INFO, "glyphboxes count after resize: ", data->glyphBoxes.size());

	uploadAtlasToGPU();
}
void FontAtlas::syncWithGPU() {
	if (atlasChanged) {
		uploadAtlasToGPU();
		atlasChanged = false;
	}
}

void FontAtlas::uploadAtlasToGPU() {
	nri::CommandPool &commandPool	= nri.getDefaultCommandPool();
	auto			  commandBuffer = nri.createCommandBuffer(commandPool);
	commandBuffer->begin();
	image->prepareForTransferDst(*commandBuffer);
	image->copyFrom(*commandBuffer, *uploadBuffer, 0, data->atlasStorage.getStride() / sizeof(float) / 4);
	image->prepareForTexture(*commandBuffer);
	if (!data->glyphBoxes.empty())
		glyphGeometryBuffer->copyFrom(*commandBuffer, *uploadBuffer, glyphGeometryBuffer->getOffset(), 0,
									  data->glyphBoxes.size() * (sizeof(GlyphBox) + sizeof(int)));
	commandBuffer->end();

	auto key = q.submit(*commandBuffer);
	q.wait(key);
}

static const int tabSize = 4;

std::pair<fxed::GlyphBox, int> FontAtlas::getGlyphBox(uint32_t c) {
	auto it = data->codepointToGlyphBoxIndex.find(c);
	if (it != data->codepointToGlyphBoxIndex.end()) {
		assert(it->second >= 0 && it->second < (int)data->glyphBoxes.size());
		return {data->glyphBoxes[it->second].first, it->second};
	} else if (c == U'\t') {
		auto [spaceBox, i] = getGlyphBox(U' ');
		return {GlyphBox{.bounds   = {0, spaceBox.bounds.t, spaceBox.advance * tabSize, spaceBox.bounds.b},
						 .rect	   = {0, 0, 0, 0},
						 .index	   = -1,
						 .advance  = spaceBox.advance * tabSize,
						 .isBitmap = false},
				i};
	} else {
		auto i = addGlyphToAtlas(c);
		if (i == -1) { return getGlyphBox(U'?'); }
		atlasChanged = true;
		return {data->glyphBoxes[i].first, i};
	}
}

FontFallbackChain::FontFallbackChain(const std::vector<std::string_view> &fonts) : data(new FontFallbackChainData()) {
	FT_Library ft;
	if (FT_Init_FreeType(&ft)) { THROW_RUNTIME_ERR("Could not initialize FreeType library!"); }

	for (const auto &fontPath : fonts) {
		FT_Face face;
		if (FT_New_Face(ft, fontPath.data(), 0, &face)) {
			dbLog(dbg::LOG_ERROR, "Failed to load font face from path: ", fontPath);
			continue;
		}
		dbLog(dbg::LOG_INFO, "Loaded font face from path: ", fontPath);
		data->fontFaces.push_back(face);
	}
}

std::pair<fxed::GlyphBox, int> FontFallbackChain::getGlyphBox(uint32_t c, uint32_t size) const {
	for (unsigned int i = 0; i < data->fontFaces.size(); i++) {
		FT_Face &face	   = data->fontFaces[i];
		FT_UInt	 loadFlags = FT_LOAD_DEFAULT;

		FT_UInt glyphIndex = FT_Get_Char_Index(face, c);

		if (FT_HAS_COLOR(face)) {
			loadFlags |= FT_LOAD_COLOR;
			if (FT_HAS_FIXED_SIZES(face)) {
				int bestSizeIndex = 0;
				int bestSizeDiff  = std::numeric_limits<int>::max();
				for (int j = 0; j < face->num_fixed_sizes; j++) {
					int sizeDiff = std::abs((int)face->available_sizes[j].height - (int)size);
					if (sizeDiff < bestSizeDiff) {
						bestSizeDiff  = sizeDiff;
						bestSizeIndex = j;
					}
				}
				FT_Select_Size(face, bestSizeIndex);
			}
		} else {
			FT_Set_Pixel_Sizes(face, 0, size);
		}

		if (glyphIndex != 0) {
			uint32_t e = FT_Load_Glyph(face, glyphIndex, loadFlags);
			if (e) {
				dbLog(dbg::LOG_ERROR, "Failed to load glyph for codepoint ", c, " in font index ", i, " error: 0x",
					  std::hex, e, std::dec);
				continue;
			}

			GlyphBox box{
				.bounds	  = {face->glyph->metrics.horiBearingX / 64.0f, face->glyph->metrics.horiBearingY / 64.0f,
							 (face->glyph->metrics.horiBearingX + face->glyph->metrics.width) / 64.0f,
							 (face->glyph->metrics.horiBearingY - face->glyph->metrics.height) / 64.0f},
				.rect	  = {0, 0, (int)face->glyph->bitmap.width, (int)face->glyph->bitmap.rows},
				.index	  = (int)glyphIndex,
				.advance  = face->glyph->advance.x / 64.0f,
				.isBitmap = face->glyph->format == FT_GLYPH_FORMAT_BITMAP};

			return {box, i};
		}
	}
	THROW_RUNTIME_ERR(std::format("Glyph '{}' not found in any font in the fallback chain!", c));
}

FontFallbackChain::~FontFallbackChain() {
	if (data == nullptr) { return; }
	for (auto &face : data->fontFaces) {
		FT_Done_Face(face);
	}
	delete data;
}

#ifdef _WIN32
	#include <windows.h>
	#include <shlobj.h>
	#include <string>

static std::string findWindowsFontPath(const std::string_view &fontName) {
	char fontsPath[MAX_PATH];

	// Get Windows Fonts directory
	if (SHGetFolderPathA(NULL, CSIDL_FONTS, NULL, 0, fontsPath) == S_OK) {
		// Common default fonts in order of preference
		std::string segoeui = std::string(fontsPath) + "\\" + std::string(fontName) + ".ttf";	  // Preferred font
		std::string tahoma	= std::string(fontsPath) + "\\tahoma.ttf";							  // Older fallback

		// Check which exists
		if (GetFileAttributesA(segoeui.c_str()) != INVALID_FILE_ATTRIBUTES) { return segoeui; }
		if (GetFileAttributesA(tahoma.c_str()) != INVALID_FILE_ATTRIBUTES) { return tahoma; }
	}
	return "";
}
#endif

#ifdef __linux__
	#include <fontconfig/fontconfig.h>
	#include <string>

static std::string findLinuxFontPath(const std::string_view &fontName) {
	std::string fontPath;

	FcConfig  *config  = FcInitLoadConfigAndFonts();
	FcPattern *pattern = FcNameParse((const FcChar8 *)fontName.data());

	FcConfigSubstitute(config, pattern, FcMatchPattern);
	FcDefaultSubstitute(pattern);

	FcResult   result;
	FcPattern *font = FcFontMatch(config, pattern, &result);

	if (font) {
		FcChar8 *file = nullptr;
		if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch) { fontPath = (char *)file; }
		FcPatternDestroy(font);
	}

	FcPatternDestroy(pattern);
	FcConfigDestroy(config);

	return fontPath;
}
#endif

std::string FontAtlas::findFontPath(std::string_view fontName) {
	std::string result;
#ifdef _WIN32
	result = findWindowsFontPath(fontName);
#elif __linux__
	result = findLinuxFontPath(fontName);
#else
	result = "";	 // Unsupported platform
#endif
	if (result.empty()) { result = getDefaultSystemFontPath(); }
	return result;
}

std::string FontAtlas::getDefaultSystemFontPath() {
#ifdef _WIN32
	return findWindowsFontPath("segoeui");
#elif __linux__
	return findLinuxFontPath("DejaVu Sans Mono:style=Regular");
#else
	return "";	   // Unsupported platform
#endif
}
