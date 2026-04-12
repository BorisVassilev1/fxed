#pragma once

#include <cstdint>
#include "any_range.hpp"
#include "font.hpp"
#include "mesh.hpp"
#include "nri.hpp"
#include "resource_manager.hpp"
#include "utils.hpp"
namespace fxed {

class TextMesh : public fxed::Mesh {
	Vertex	   *vertexData;
	uint32_t   *indexData;
	std::size_t maxCharCount;
	glm::vec2	bounds;

   public:
	TextMesh(nri::NRI &nri, nri::CommandQueue &q, std::size_t maxCharCount);
	DELETE_COPY_AND_ASSIGNMENT(TextMesh);

	glm::vec2 updateText(fxed::any_input_range<char32_t> &&text, fxed::FontAtlas &font, glm::ivec2 cursorPos = {0, 0}, float lineWidth = 0);

	glm::vec2 getBounds() const { return bounds; }
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
};

enum class CharacterDrawMode { ALPHA = 0, COLOR = 1, MSDF = 2 };


}	  // namespace fxed
