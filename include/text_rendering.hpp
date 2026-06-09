#pragma once

#include <cstdint>
#include "any_range.hpp"
#include "font.hpp"
#include "mesh.hpp"
#include "nri.hpp"
#include "resource_manager.hpp"
#include "static_vector.hpp"
#include "utils.hpp"
namespace fxed {

class TextMesh : public fxed::Mesh {
	Vertex	   *vertexData;
	uint32_t   *indexData;
	std::size_t maxCharCount;
	glm::vec2	bounds;
	bool		overflowed = false;

   public:
	TextMesh(nri::NRI &nri, nri::CommandQueue &q, std::size_t maxCharCount);
	DELETE_COPY_AND_ASSIGNMENT(TextMesh);
	TextMesh(TextMesh &&other) noexcept;
	TextMesh &operator=(TextMesh &&other) noexcept;

	// glm::vec2 updateText(fxed::any_input_range<char32_t> &&text, fxed::FontAtlas &font, glm::ivec2 cursorPos = {0,
	// 0}, 					 float lineWidth = 0);

	template <std::ranges::input_range R>
	glm::vec2 updateText(R &&text, fxed::FontAtlas &font, glm::ivec2 cursorPos = {0, 0}, float lineWidth = 0);

	glm::vec2 getBounds() const { return bounds; }
	bool	  isOverflowed() const { return overflowed; }
};

class TextMeshInstanced {
	std::unique_ptr<nri::Allocation> memory;
	std::unique_ptr<nri::Buffer>	 indexBuffer;
	std::unique_ptr<nri::Buffer>	 instanceBuffer;

	struct InstanceData {
		glm::vec4 color;
		glm::vec2 translation;
		int32_t	  charIndex;
		int32_t	  drawMode;
	};
	StaticVector<InstanceData> instanceData;	 /// pointer to mapped instance buffer memory
	glm::vec2				   bounds;
	bool					   overflowed = false;

   public:
	TextMeshInstanced(nri::NRI &nri, nri::CommandQueue &q, std::size_t maxInstanceCount);

	glm::vec2 getBounds() const { return bounds; }
	bool	  isOverflowed() const { return overflowed; }

	void bind(nri::CommandBuffer &cmdBuffer) const;
	void draw(nri::CommandBuffer &cmdBuffer, nri::GraphicsProgram &program) const;

	template <std::ranges::input_range R>
	glm::vec2 updateText(R &&text, fxed::FontAtlas &font, glm::ivec2 cursorPos = {0, 0}, float lineWidth = 0);

	static std::vector<nri::VertexBinding> getVertexBindings();
};

struct TextRenderState {
	glm::vec2  translation{0, 0};
	glm::ivec2 viewportSize;
	glm::vec2  cursorPos;
	bool	   showCursor = true;
};

class TextRenderer {
	fxed::FontAtlas	  font;
	static ResourceID cursorMeshID;
	static ResourceID shaderID;
	static ResourceID cursorShaderID;
	static ResourceID shader_TWO_ID;
	uint32_t		  version;
	float			  fontSize;

   public:
	fxed::FontAtlas &getFont() { return font; }
	float			 getFontSize() const;
	void			 setFontSize(uint32_t size);
	uint32_t		 getVersion() const { return version; }

	TextRenderer(nri::NRI &nri, nri::CommandQueue &queue, fxed::FontAtlas &&font);
	DELETE_COPY_AND_ASSIGNMENT(TextRenderer);

	void renderText(nri::CommandBuffer &cmdBuf, const fxed::TextMesh &textMesh, const TextRenderState &renderState);

	void renderText(nri::CommandBuffer &cmdBuf, const fxed::TextMeshInstanced &textMesh,
					const TextRenderState &renderState);
};

enum class CharacterDrawMode { ALPHA = 0, COLOR = 1, MSDF = 2 };

template <std::ranges::input_range R>
glm::vec2 TextMesh::updateText(R &&text, fxed::FontAtlas &font, glm::ivec2 cursorPos, float lineWidth) {
	bounds					  = glm::vec2(0, 0);
	glm::vec2 cursorPosResult = cursorPos;
	size_t	  offset		  = 0;
	size_t	  indexCount	  = 0;
	int		  advanceY		  = 0;
	double	  advanceDY		  = 0.0;
	int		  advanceX		  = 0;
	double	  advanceDX		  = 0.0;
	double	  lineHeight	  = 1.2;
	overflowed				  = false;

	size_t j = 0;
	for (auto i = text.begin(); i != text.end(); ++i) {
		if (j >= maxCharCount) {
			dbLog(dbg::LOG_WARNING, "TextMesh max character count exceeded, truncating text");
			break;
		}
		if (*i == '\n') {
			if (cursorPos.x == advanceX && cursorPos.y == advanceY) {
				cursorPosResult = glm::vec2(advanceDX, advanceDY);
			}

			advanceDY += lineHeight;
			advanceDX = 0.0;
			advanceY += 1;
			advanceX = 0;

			continue;
		}
		if (*i == 0 || *i == 13) continue;	   // skip null characters

		auto [box, index] = font.getGlyphBox(*i);
		if (lineWidth > 0 && advanceDX + box.advance >= lineWidth / font.getFontSize()) {
			advanceDY += lineHeight;
			advanceDX  = 0.0;
			overflowed = true;
		}

		if (*i > 255 || !std::isspace(*i)) {
			vertexData[4 * j + 0].x = advanceDX + box.bounds.l;
			vertexData[4 * j + 0].y = advanceDY - box.bounds.t;
			vertexData[4 * j + 1].x = advanceDX + box.bounds.l;
			vertexData[4 * j + 1].y = advanceDY - box.bounds.b;
			vertexData[4 * j + 2].x = advanceDX + box.bounds.r;
			vertexData[4 * j + 2].y = advanceDY - box.bounds.b;
			vertexData[4 * j + 3].x = advanceDX + box.bounds.r;
			vertexData[4 * j + 3].y = advanceDY - box.bounds.t;

			indexData[6 * j + 0] = offset + 0;
			indexData[6 * j + 1] = offset + 1;
			indexData[6 * j + 2] = offset + 2;
			indexData[6 * j + 3] = offset + 2;
			indexData[6 * j + 4] = offset + 3;
			indexData[6 * j + 5] = offset + 0;

			vertexData[4 * j + 0].u = (box.rect.x) / (float)font.getAtlasSize();
			vertexData[4 * j + 0].v = (box.rect.y + box.rect.h) / (float)font.getAtlasSize();
			vertexData[4 * j + 1].u = (box.rect.x) / (float)font.getAtlasSize();
			vertexData[4 * j + 1].v = (box.rect.y) / (float)font.getAtlasSize();
			vertexData[4 * j + 2].u = (box.rect.x + box.rect.w) / (float)font.getAtlasSize();
			vertexData[4 * j + 2].v = (box.rect.y) / (float)font.getAtlasSize();
			vertexData[4 * j + 3].u = (box.rect.x + box.rect.w) / (float)font.getAtlasSize();
			vertexData[4 * j + 3].v = (box.rect.y + box.rect.h) / (float)font.getAtlasSize();

			CharacterDrawMode drawMode = CharacterDrawMode::ALPHA;
			// if (font.getFontSize() >= 32) {
			//	drawMode = CharacterDrawMode::MSDF;
			// }

			if (box.isBitmap) { drawMode = CharacterDrawMode::COLOR; }

			bool drawmodeBit0 = static_cast<int>(drawMode) & 1;
			bool drawmodeBit1 = static_cast<int>(drawMode) & 2;
			for (int k = 0; k < 4; k++) {
				vertexData[4 * j + k].u = std::copysign(vertexData[4 * j + k].u, drawmodeBit0 ? -1.0f : 1.0f);
				vertexData[4 * j + k].v = std::copysign(vertexData[4 * j + k].v, drawmodeBit1 ? -1.0f : 1.0f);
			}

			bounds.x = std::max({bounds.x, vertexData[4 * j + 0].x, vertexData[4 * j + 1].x, vertexData[4 * j + 2].x,
								 vertexData[4 * j + 3].x});
			bounds.y = std::max({bounds.y, vertexData[4 * j + 0].y, vertexData[4 * j + 1].y, vertexData[4 * j + 2].y,
								 vertexData[4 * j + 3].y});

			offset += 4;
			indexCount += 6;
			++j;
		}

		if (cursorPos.x == advanceX && cursorPos.y == advanceY) { cursorPosResult = glm::vec2(advanceDX, advanceDY); }
		advanceX += 1;
		advanceDX += box.advance;
	}

	this->indexCount = static_cast<uint32_t>(indexCount);
	return cursorPosResult;
}

template <std::ranges::input_range R>
glm::vec2 TextMeshInstanced::updateText(R &&text, fxed::FontAtlas &font, glm::ivec2 cursorPos, float lineWidth) {
	bounds					  = glm::vec2(0, 0);
	glm::vec2 cursorPosResult = cursorPos;
	size_t	  offset		  = 0;
	size_t	  indexCount	  = 0;
	int		  advanceY		  = 0;
	double	  advanceDY		  = 0.0;
	int		  advanceX		  = 0;
	double	  advanceDX		  = 0.0;
	double	  lineHeight	  = 1.2;
	overflowed				  = false;

	size_t j = 0;
	instanceData.clear();
	for (auto i = text.begin(); i != text.end(); ++i) {
		if (j >= instanceData.capacity()) {
			dbLog(dbg::LOG_WARNING, "TextMesh max character count exceeded, truncating text");
			break;
		}
		if (*i == '\n') {
			if (cursorPos.x == advanceX && cursorPos.y == advanceY) {
				cursorPosResult = glm::vec2(advanceDX, advanceDY);
			}

			advanceDY += lineHeight;
			advanceDX = 0.0;
			advanceY += 1;
			advanceX = 0;

			continue;
		}
		if (*i == 0 || *i == 13) continue;	   // skip null characters

		auto [box, index] = font.getGlyphBox(*i);
		if (lineWidth > 0 && advanceDX + box.advance >= lineWidth / font.getFontSize()) {
			advanceDY += lineHeight;
			advanceDX  = 0.0;
			overflowed = true;
		}
		if (index == -1) {
			dbLog(dbg::LOG_ERROR, "Failed to get glyph box for codepoint ", (int)*i);
			continue;
		}

		if (*i > 255 || !std::isspace(*i)) {
			float box_bound_0_x = advanceDX + box.bounds.l;
			float box_bound_0_y = advanceDY - box.bounds.t;
			float box_bound_1_x = advanceDX + box.bounds.l;
			float box_bound_1_y = advanceDY - box.bounds.b;
			float box_bound_2_x = advanceDX + box.bounds.r;
			float box_bound_2_y = advanceDY - box.bounds.b;
			float box_bound_3_x = advanceDX + box.bounds.r;
			float box_bound_3_y = advanceDY - box.bounds.t;

			CharacterDrawMode drawMode = CharacterDrawMode::ALPHA;
			if (font.getFontSize() >= 32) { drawMode = CharacterDrawMode::MSDF; }

			if (box.isBitmap) { drawMode = CharacterDrawMode::COLOR; }
			float p = advanceDX + advanceDY;
			instanceData.push_back(InstanceData{
				//.color = glm::vec4(rand() / (float)RAND_MAX, rand() / (float)RAND_MAX, rand() /
				//(float)RAND_MAX, 1.0f),
				.color = glm::vec4(std::sin(p * 0.1f) * 0.5f + 0.5f, std::sin(p * 0.1f + 2) * 0.5f + 0.5f,
								   std::sin(p * 0.1f + 4) * 0.5f + 0.5f, 1.0f),
				.translation = glm::vec2(advanceDX, advanceDY),
				.charIndex	 = index,
				.drawMode	 = static_cast<int32_t>(drawMode),
			});

			bounds.x = std::max({bounds.x, box_bound_0_x, box_bound_1_x, box_bound_2_x, box_bound_3_x});
			bounds.y = std::max({bounds.y, box_bound_0_y, box_bound_1_y, box_bound_2_y, box_bound_3_y});

			offset += 4;
			indexCount += 6;
			++j;
		}

		if (cursorPos.x == advanceX && cursorPos.y == advanceY) { cursorPosResult = glm::vec2(advanceDX, advanceDY); }
		advanceX += 1;
		advanceDX += box.advance;
	}

	return cursorPosResult;
}

}	  // namespace fxed
