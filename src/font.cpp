#include "font.hpp"
#include <format>

#include <msdf-atlas-gen/msdf-atlas-gen.h>

#include "nri.hpp"
#include "packing.hpp"
#include "utils.hpp"

#include "ft2build.h"
#include FT_FREETYPE_H

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

using namespace fxed;

class ImageAtlasStorage {
	mutable std::unique_ptr<nri::Buffer>	 uploadBuffer;
	mutable std::unique_ptr<nri::Allocation> uploadBufferAllocation;
	mutable void							*data = nullptr;
	uint32_t								 width;
	uint32_t								 height;

	static const nri::Format format = nri::Format::FORMAT_R32G32B32A32_SFLOAT;
	static const int		 N		= 4;
	using T							= float;

   public:
	ImageAtlasStorage(nri::NRI &nri, uint32_t width, uint32_t height) : width(width), height(height) {
		dbLog(dbg::LOG_INFO, "Creating ImageAtlasStorage ", width, "x", height);
		uploadBuffer = nri.createBuffer(width * height * N * sizeof(T), nri::BUFFER_USAGE_TRANSFER_SRC);
		uploadBufferAllocation =
			nri.allocateMemory(uploadBuffer->getMemoryRequirements().setTypeRequest(nri::MEMORY_TYPE_UPLOAD));
		uploadBuffer->bindMemory(*uploadBufferAllocation, 0);
		data = uploadBuffer->map(0, uploadBuffer->getSize());
	}

	~ImageAtlasStorage() { unmap(); }

	void unmap() {
		if (data) {
			if (uploadBuffer) uploadBuffer->unmap();
			data = nullptr;
		}
	}
	void put(int x, int y, const msdfgen::BitmapConstRef<float, 4> &subBitmap) {
		assert(data != nullptr);
		assert(x + subBitmap.width <= width && y + subBitmap.height <= height);		// FIX BOUNDS CHECK
		for (int j = 0; j < subBitmap.height; ++j) {
			for (int i = 0; i < subBitmap.width; ++i) {
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

	void put(int x, int y, const msdfgen::BitmapConstRef<float, 1> &subBitmap) {
		assert(data != nullptr);
		assert(x + subBitmap.width <= width && y + subBitmap.height <= height);		// FIX BOUNDS CHECK
		for (int j = 0; j < subBitmap.height; ++j) {
			for (int i = 0; i < subBitmap.width; ++i) {
				((T *)data)[(y + j) * width * N + (x * N) + i * N + 3] = subBitmap.pixels[j * subBitmap.width + i];
			}
		}
	}

	void put(int x, int y, const msdfgen::BitmapConstRef<uint8_t, 1> &subBitmap) {
		assert(data != nullptr);
		assert(x + subBitmap.width <= width && y + subBitmap.height <= height);		// FIX BOUNDS CHECK
		for (int j = 0; j < subBitmap.height; ++j) {
			for (int i = 0; i < subBitmap.width; ++i) {
				((T *)data)[(y + j) * width * N + (x * N) + i * N + 3] =
					subBitmap.pixels[j * subBitmap.width + i] / 255.f;
			}
		}
	}

	void put(int x, int y, const msdfgen::BitmapConstRef<uint8_t, 4> &subBitmap) {
		assert(data != nullptr);
		assert(x + subBitmap.width <= width && y + subBitmap.height <= height);		// FIX BOUNDS CHECK
		for (int j = 0; j < subBitmap.height; ++j) {
			for (int i = 0; i < subBitmap.width; ++i) {
				uint32_t value = subBitmap.pixels[j * subBitmap.width + i];
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

	void get(int x, int y, const msdfgen::BitmapRef<float, 4> &subBitmap) const {
		for (int j = 0; j < subBitmap.height; ++j) {
			for (int i = 0; i < subBitmap.width; ++i) {
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

	void get(int x, int y, const msdfgen::BitmapRef<float, 3> &subBitmap) const {
		for (int j = 0; j < subBitmap.height; ++j) {
			for (int i = 0; i < subBitmap.width; ++i) {
				subBitmap.pixels[j * subBitmap.width * 3 + i * 3 + 0] =
					((T *)data)[(y + j) * width * N + (x * N) + i * N + 0];
				subBitmap.pixels[j * subBitmap.width * 3 + i * 3 + 1] =
					((T *)data)[(y + j) * width * N + (x * N) + i * N + 1];
				subBitmap.pixels[j * subBitmap.width * 3 + i * 3 + 2] =
					((T *)data)[(y + j) * width * N + (x * N) + i * N + 2];
			}
		}
	}

	void get(int x, int y, const msdfgen::BitmapRef<float, 1> &subBitmap) const {
		for (int j = 0; j < subBitmap.height; ++j) {
			for (int i = 0; i < subBitmap.width; ++i) {
				subBitmap.pixels[j * subBitmap.width + i] = ((T *)data)[(y + j) * width * N + (x * N) + i * N + 3];
			}
		}
	}

	void *getDataAt(int x, int y) const {
		assert(data != nullptr);
		return (void *)((T *)data + (y * width * N + x * N));
	}

	uint32_t getStride() const { return width * N * sizeof(T); }

	// this is hack
	auto &getUploadBuffer() const { return uploadBuffer; }
	auto &getUploadBufferAllocation() const { return uploadBufferAllocation; }
};

struct FontAtlas::FontData {
	std::vector<std::pair<GlyphBox, int>> glyphBoxes;
	std::map<uint32_t, int>				  codepointToGlyphBoxIndex;
	RowAtlasPacker						  atlasPacker;
	ImageAtlasStorage					  atlasStorage;

	FontData(uint32_t atlasSize, uint32_t fontSize, nri::NRI &nri)
		: glyphBoxes(std::vector<std::pair<GlyphBox, int>>()),
		  codepointToGlyphBoxIndex(std::map<uint32_t, int>()),
		  atlasPacker(RowAtlasPacker(atlasSize, atlasSize, fontSize + 2)),
		  atlasStorage(ImageAtlasStorage(nri, atlasSize, atlasSize)) {}
};

struct FontFallbackChain::FontFallbackChainData {
	std::vector<FT_Face> fontFaces;
};

int FontAtlas::addGlyphToAtlas(uint32_t c) {
	try {
		data->glyphBoxes.push_back(this->fallbackChain.getGlyphBox(c, fontSize));

		auto &box	= data->glyphBoxes.back().first;
		auto &index = data->glyphBoxes.back().second;
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
				msdfgen::BitmapConstRef<uint8_t, 4> bitmap(resizedBitmap.data(), w, h);
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
				msdfgen::BitmapConstRef<uint8_t, 1> bitmap((const uint8_t *)face->glyph->bitmap.buffer,
														   face->glyph->bitmap.width, face->glyph->bitmap.rows);
				data->atlasStorage.put(box.rect.x, box.rect.y, bitmap);
			} break;
			case FT_PIXEL_MODE_BGRA: {
				msdfgen::BitmapConstRef<uint8_t, 4> bitmap((const uint8_t *)face->glyph->bitmap.buffer,
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
					 uint32_t fontSize)
	: data(new FontData(atlasSize, fontSize, nri)),
	  fallbackChain(std::move(fallbackChain)),
	  nri(nri),
	  q(q),
	  fontSize(fontSize) {
	std::unique_ptr<nri::Buffer>	 &uploadBuffer			 = data->atlasStorage.getUploadBuffer();
	std::unique_ptr<nri::Allocation> &uploadBufferAllocation = data->atlasStorage.getUploadBufferAllocation();

	if (uploadBuffer == nullptr || uploadBufferAllocation == nullptr) {
		dbLog(dbg::LOG_ERROR, "Failed to create font image: upload buffer is null");
		THROW_RUNTIME_ERR("Failed to create font image: upload buffer is null");
	}

	image = nri.createImage2D(atlasSize, atlasSize, nri::FORMAT_R32G32B32A32_SFLOAT,
							  nri::IMAGE_USAGE_SAMPLED | nri::IMAGE_USAGE_TRANSFER_DST);

	imageAllocation = nri.allocateMemory(image->getMemoryRequirements().setTypeRequest(nri::MEMORY_TYPE_DEVICE));
	image->bindMemory(*imageAllocation, 0);

	imageView = image->createTextureView();

	uploadAtlasToGPU();
}

FontAtlas::~FontAtlas() { delete data; }

void FontAtlas::resize(uint32_t newSize) {
	auto oldSize = fontSize;
	newSize		 = std::min(newSize, 32u);
	if (oldSize == newSize) return;

	data->glyphBoxes.clear();
	data->atlasPacker.setRowHeight(newSize + 2);

	fontSize = newSize;

	for (auto [c, _] : data->codepointToGlyphBoxIndex) {
		addGlyphToAtlas(c);
	}

	uploadAtlasToGPU();
}
void FontAtlas::syncWithGPU() {
	if (atlasChanged) {
		uploadAtlasToGPU();
		atlasChanged = false;
	}
}

void FontAtlas::uploadAtlasToGPU() {
	auto &uploadBuffer = data->atlasStorage.getUploadBuffer();

	nri::CommandPool &commandPool	= nri.getDefaultCommandPool();
	auto			  commandBuffer = nri.createCommandBuffer(commandPool);
	commandBuffer->begin();
	image->prepareForStorage(*commandBuffer);
	image->copyFrom(*commandBuffer, *uploadBuffer, 0, 0);
	image->prepareForTexture(*commandBuffer);
	commandBuffer->end();

	auto key = q.submit(*commandBuffer);
	q.wait(key);
}

static const int tabSize = 4;

fxed::GlyphBox FontAtlas::getGlyphBox(uint32_t c) {
	auto it = data->codepointToGlyphBoxIndex.find(c);
	if (it != data->codepointToGlyphBoxIndex.end()) {
		return data->glyphBoxes[it->second].first;
	} else if (c == U'\t') {
		auto spaceBox = getGlyphBox(U' ');
		return GlyphBox{.index	 = -1,
						.advance = spaceBox.advance * tabSize,
						.bounds	 = {0, spaceBox.bounds.t, spaceBox.advance * tabSize, spaceBox.bounds.b},
						.rect	 = {0, 0, 0, 0}};
	} else {
		auto i = addGlyphToAtlas(c);
		if (i == -1) { THROW_RUNTIME_ERR(std::format("Glyph '{}' not found in any font in the fallback chain!", c)); }
		atlasChanged = true;
		return data->glyphBoxes[i].first;
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

		dbLog(dbg::LOG_DEBUG, "Font face ", fontPath, " has ", face->num_glyphs, " glyphs");
		dbLog(dbg::LOG_DEBUG, "Font face ", fontPath, " has ", face->num_fixed_sizes, " fixed sizes");
		for (int i = 0; i < face->num_fixed_sizes; i++) {
			dbLog(dbg::LOG_DEBUG, "strike ", i, ": ", face->available_sizes[i].width, "x",
				  face->available_sizes[i].height);
		}

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
			GlyphBox box{.index	  = (int)glyphIndex,
						 .advance = face->glyph->advance.x / 64.0,
						 .bounds  = {face->glyph->metrics.horiBearingX / 64.0, face->glyph->metrics.horiBearingY / 64.0,
									 (face->glyph->metrics.horiBearingX + face->glyph->metrics.width) / 64.0,
									 (face->glyph->metrics.horiBearingY - face->glyph->metrics.height) / 64.0},
						 .rect	  = {0, 0, (int)face->glyph->bitmap.width, (int)face->glyph->bitmap.rows}};

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
