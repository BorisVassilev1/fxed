#include "text_rendering.hpp"
using namespace fxed;

TextMesh::TextMesh(nri::NRI &nri, nri::CommandQueue &q, std::size_t maxCharCount)
	: fxed::Mesh(nri, maxCharCount * 4, maxCharCount * 6, nri::MEMORY_TYPE_UPLOAD), maxCharCount(maxCharCount) {
	vertexData = (Vertex *)memory->map();
	if (!vertexData) {
		dbLog(dbg::LOG_ERROR, "Failed to map vertex buffer for TextMesh");
		THROW_RUNTIME_ERR("Failed to map vertex buffer for TextMesh");
	}
	indexData = (uint32_t *)((char *)vertexData + indexBuffer->getOffset() - vertexAttributes->getOffset());
	if (!indexData) {
		dbLog(dbg::LOG_ERROR, "Failed to map index buffer for TextMesh");
		THROW_RUNTIME_ERR("Failed to map index buffer for TextMesh");
	}
}

glm::vec2 TextMesh::updateText(std::span<const char32_t> text, const fxed::FontAtlas &font, glm::ivec2 cursorPos) {
	glm::vec2 cursorPosResult = cursorPos;
	size_t	  offset		  = 0;
	size_t	  indexCount	  = 0;
	int		  advanceY		  = 0;
	int		  advanceX		  = 0;
	double	  advance		  = 0.0;

	size_t j = 0;
	for (size_t i = 0; i < text.size(); i++) {
		if (j >= maxCharCount) {
			dbLog(dbg::LOG_WARNING, "TextMesh max character count exceeded, truncating text");
			break;
		}
		if (text[i] == '\n') {
			advanceY += 1;
			advance	 = 0.0;
			advanceX = 0;
			continue;
		}

		auto box = font.getGlyphBox(text[i]);
		if (text[i] > 255 || !std::isspace(text[i])) {
			vertexData[4 * j + 0].x = advance + box.bounds.l;
			vertexData[4 * j + 0].y = advanceY - box.bounds.t;
			vertexData[4 * j + 1].x = advance + box.bounds.l;
			vertexData[4 * j + 1].y = advanceY - box.bounds.b;
			vertexData[4 * j + 2].x = advance + box.bounds.r;
			vertexData[4 * j + 2].y = advanceY - box.bounds.b;
			vertexData[4 * j + 3].x = advance + box.bounds.r;
			vertexData[4 * j + 3].y = advanceY - box.bounds.t;

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
			offset += 4;
			indexCount += 6;
			++j;
		}

		advanceX += 1;
		advance += box.advance;
		if (cursorPos.x == advanceX && cursorPos.y == advanceY) { cursorPosResult = glm::vec2(advance, advanceY); }
	}

	this->indexCount = static_cast<uint32_t>(indexCount);
	return cursorPosResult;
}
