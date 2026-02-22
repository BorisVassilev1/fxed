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

	glm::vec2 updateText(std::span<const char32_t> text, const fxed::FontAtlas &font, glm::ivec2 cursorPos = {0, 0});
};
}	  // namespace fxed
