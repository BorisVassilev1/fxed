#include "font.hpp"
#include <format>

#include <msdf-atlas-gen/msdf-atlas-gen.h>

#include "nri.hpp"
#include "utils.hpp"

using namespace msdf_atlas;

static nri::NRI *g_nri = nullptr;

template <class T = float>
class ImageAtlasStorage {
	mutable std::unique_ptr<nri::Buffer>	 uploadBuffer;
	mutable std::unique_ptr<nri::Allocation> uploadBufferAllocation;
	mutable void							*data = nullptr;
	uint32_t								 width;
	uint32_t								 height;

	const nri::Format format = nri::Format::FORMAT_R32G32B32A32_SFLOAT;
	const int		  N		 = 4;

   public:
	ImageAtlasStorage(uint32_t width, uint32_t height) : width(width), height(height) {
		dbLog(dbg::LOG_INFO, "Creating ImageAtlasStorage ", width, "x", height);
		uploadBuffer = g_nri->createBuffer(width * height * N * sizeof(T), nri::BUFFER_USAGE_TRANSFER_SRC);
		uploadBufferAllocation =
			g_nri->allocateMemory(uploadBuffer->getMemoryRequirements().setTypeRequest(nri::MEMORY_TYPE_UPLOAD));
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
	void put(int x, int y, const msdfgen::BitmapConstRef<float, 3> &subBitmap) {
		assert(data != nullptr);
		assert(x + subBitmap.width <= width && y + subBitmap.height <= height);		// FIX BOUNDS CHECK
		for (int j = 0; j < subBitmap.height; ++j) {
			for (int i = 0; i < subBitmap.width; ++i) {
				((T *)data)[(y + j) * width * N + (x * N) + i * N + 0] =
					subBitmap.pixels[j * subBitmap.width * 3 + i * 3 + 0];
				((T *)data)[(y + j) * width * N + (x * N) + i * N + 1] =
					subBitmap.pixels[j * subBitmap.width * 3 + i * 3 + 1];
				((T *)data)[(y + j) * width * N + (x * N) + i * N + 2] =
					subBitmap.pixels[j * subBitmap.width * 3 + i * 3 + 2];
				//((T *)data)[(y + j) * width * N + (x * N) + i * N + 3] = 1.0f;  // alpha
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
	// this is hack
	auto &getUploadBuffer() const { return uploadBuffer; }
	auto &getUploadBufferAllocation() const { return uploadBufferAllocation; }
};

struct Font::FontData {
	std::vector<GlyphGeometry> glyphs;
	FontGeometry			   fontGeometry;
};

Font::Font(nri::NRI &nri, nri::CommandQueue &q, const char *fontfilename, uint32_t atlasSize) : data(new FontData()) {
	g_nri = &nri;

	std::unique_ptr<nri::Buffer>	 uploadBuffer;
	std::unique_ptr<nri::Allocation> uploadBufferAllocation;

	if (msdfgen::FreetypeHandle *ft = msdfgen::initializeFreetype()) {
		if (msdfgen::FontHandle *font = msdfgen::loadFont(ft, fontfilename)) {
			data->fontGeometry = FontGeometry(&data->glyphs);

			// The second argument can be ignored unless you mix different font sizes in one atlas.
			// In the last argument, you can specify a charset other than ASCII.
			// To load specific glyph indices, use loadGlyphs instead.
			data->fontGeometry.loadCharset(font, 1.0, Charset::ASCII);
			const double maxCornerAngle = 3.0;
			for (GlyphGeometry &glyph : data->glyphs)
				glyph.edgeColoring(&msdfgen::edgeColoringInkTrap, maxCornerAngle, 0);

			TightAtlasPacker packer;
			packer.setDimensionsConstraint(DimensionsConstraint::SQUARE);
			packer.setDimensions(atlasSize, atlasSize);
			// packer.setMinimumScale(24.0);
			packer.setScale(24.0);
			packer.setPixelRange(2.0);
			packer.setMiterLimit(1.0);
			packer.setInnerPixelPadding(1);
			packer.pack(data->glyphs.data(), data->glyphs.size());
			int width = 0, height = 0;
			packer.getDimensions(width, height);

			ImmediateAtlasGenerator<		 //
				float,						 //
				3,							 //
				msdfGenerator,				 //
				ImageAtlasStorage<float>	 //
				>
				generator(width, height);
			// GeneratorAttributes can be modified to change the generator's default settings.
			GeneratorAttributes attributes;
			generator.setAttributes(attributes);
			generator.setThreadCount(1);
			// Generate atlas bitmap
			generator.generate(data->glyphs.data(), data->glyphs.size());

			uploadBuffer		   = std::move(generator.atlasStorage().getUploadBuffer());
			uploadBufferAllocation = std::move(generator.atlasStorage().getUploadBufferAllocation());

			// Cleanup
			msdfgen::destroyFont(font);
		}
		msdfgen::deinitializeFreetype(ft);
	}

	if (uploadBuffer == nullptr || uploadBufferAllocation == nullptr) {
		dbLog(dbg::LOG_ERROR, "Failed to create font image: upload buffer is null");
		THROW_RUNTIME_ERR("Failed to create font image: upload buffer is null");
	}

	image = nri.createImage2D(atlasSize, atlasSize, nri::FORMAT_R32G32B32A32_SFLOAT,
							  nri::IMAGE_USAGE_SAMPLED | nri::IMAGE_USAGE_TRANSFER_DST);

	imageAllocation = nri.allocateMemory(image->getMemoryRequirements().setTypeRequest(nri::MEMORY_TYPE_DEVICE));
	image->bindMemory(*imageAllocation, 0);

	imageView = image->createTextureView();

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
Font::~Font() { delete data; }

Font::GlyphBox Font::getGlyphBox(char c) {
	auto bp = data->fontGeometry.getGlyph(c);
	if (bp == nullptr) { THROW_RUNTIME_ERR(std::format("Glyph '{}' not found in font!", c)); }
	msdf_atlas::GlyphBox gb = *bp;
	GlyphBox			 box{.index	  = gb.index,
							 .advance = gb.advance,
							 .bounds  = {gb.bounds.l, gb.bounds.b, gb.bounds.r, gb.bounds.t},
							 .rect	  = {gb.rect.x, gb.rect.y, gb.rect.w, gb.rect.h}};
	return box;
}
