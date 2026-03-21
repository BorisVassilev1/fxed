#pragma once

#include <cstdint>
#include "font.hpp"
#include "mesh.hpp"
#include "nri.hpp"
#include "utils.hpp"
namespace fxed {

class TextMesh : public fxed::Mesh {
	Vertex	   *vertexData;
	uint32_t   *indexData;
	std::size_t maxCharCount;

   public:
	TextMesh(nri::NRI &nri, nri::CommandQueue &q, std::size_t maxCharCount);
	DELETE_COPY_AND_ASSIGNMENT(TextMesh);

	glm::vec2 updateText(std::span<const char32_t> text, fxed::FontAtlas &font, glm::ivec2 cursorPos = {0, 0},
						 float lineWidth = 0);
};

struct TextRenderState {
	glm::ivec2 translation{0, 0};
	glm::ivec2 viewportSize;
	glm::vec2  cursorPos;
	float	   fontSize;
	bool	   showCursor = true;
};

class TextRenderer {
	fxed::FontAtlas						  font;
	std::unique_ptr<fxed::QuadMesh>		  cursorMesh;
	std::unique_ptr<nri::GraphicsProgram> shader;
	std::unique_ptr<nri::GraphicsProgram> cursorShader;

   public:
	fxed::FontAtlas &getFont() { return font; }
	float			 getFontSize() const;
	void			 setFontSize(uint32_t size);

	TextRenderer(nri::NRI &nri, nri::CommandQueue &queue, fxed::FontAtlas &&font);
	DELETE_COPY_AND_ASSIGNMENT(TextRenderer);

	void renderText(nri::CommandBuffer &cmdBuf, const fxed::TextMesh &textMesh, const TextRenderState &renderState);
};

}	  // namespace fxed
