#include "text_rendering.hpp"
#include "nri.hpp"
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

enum class CharacterDrawMode {
	ALPHA = 0,
	COLOR = 1,
	MSDF = 2
};

glm::vec2 TextMesh::updateText(std::span<const char32_t> text, fxed::FontAtlas &font, glm::ivec2 cursorPos,
							   float lineWidth) {
	glm::vec2 cursorPosResult = cursorPos;
	size_t	  offset		  = 0;
	size_t	  indexCount	  = 0;
	int		  advanceY		  = 0;
	double	  advanceDY		  = 0.0;
	int		  advanceX		  = 0;
	double	  advanceDX		  = 0.0;

	double lineWidthChars = lineWidth > 0 ? std::floor(lineWidth / font.getFontSize()) : 0;
	double lineHeight	  = 1.2;

	size_t j = 0;
	for (size_t i = 0; i < text.size(); i++) {
		if (j >= maxCharCount) {
			dbLog(dbg::LOG_WARNING, "TextMesh max character count exceeded, truncating text");
			break;
		}
		if (text[i] == '\n') {
			if (cursorPos.x == advanceX && cursorPos.y == advanceY) {
				cursorPosResult = glm::vec2(advanceDX, advanceDY);
			}

			advanceDY += lineHeight;
			advanceDX = 0.0;
			advanceY += 1;
			advanceX = 0;

			continue;
		}

		auto box = font.getGlyphBox(text[i]);
		if (lineWidth > 0 && advanceDX + box.advance >= lineWidthChars) {
			advanceDY += lineHeight;
			advanceDX = 0.0;
		}

		if (text[i] > 255 || !std::isspace(text[i])) {
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
			//if (font.getFontSize() >= 48) {
			//	drawMode = CharacterDrawMode::MSDF;
			//} else if (font.getFontSize() >= 24) {
			//	drawMode = CharacterDrawMode::ALPHA;
			//}

			if(box.isBitmap) {
				drawMode = CharacterDrawMode::COLOR;
			}

			bool drawmodeBit0 = static_cast<int>(drawMode) & 1;
			bool drawmodeBit1 = static_cast<int>(drawMode) & 2;
			for (int k = 0; k < 4; k++) {
				vertexData[4 * j + k].u = std::copysign(vertexData[4 * j + k].u, drawmodeBit0 ? -1.0f : 1.0f);
				vertexData[4 * j + k].v = std::copysign(vertexData[4 * j + k].v, drawmodeBit1 ? -1.0f : 1.0f);
			}

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

struct PushConstants {
	glm::ivec2			viewportSize;
	glm::ivec2			translation = {0, 0};
	nri::ResourceHandle textureHandle;
	float				textSize;
	float				time;
};

struct PushConstantsCursor {
	glm::ivec2 viewportSize;
	glm::ivec2 translation = {0, 0};
	glm::vec2  cursorPos;
	float	   textSize;
	float	   time;
};

TextRenderer::TextRenderer(nri::NRI &nri, nri::CommandQueue &queue, fxed::FontAtlas &&font) : font(std::move(font)) {
	auto sb = nri.createProgramBuilder();
	shader	= sb->addShaderModule(nri::ShaderCreateInfo{"shaders/text.hlsl", "VSMain", nri::SHADER_TYPE_VERTEX})
				 .addShaderModule(nri::ShaderCreateInfo{"shaders/text.hlsl", "PSMain", nri::SHADER_TYPE_FRAGMENT})
				 .setVertexBindings(fxed::TextMesh::getVertexBindings())
				 .setPrimitiveType(nri::PRIMITIVE_TYPE_TRIANGLES)
				 .setPushConstantRanges({{0, sizeof(PushConstants)}})
				 .buildGraphicsProgram();

	sb->clearShaderModules();
	cursorShader =
		sb->addShaderModule(nri::ShaderCreateInfo{"shaders/cursor.hlsl", "VSMain", nri::SHADER_TYPE_VERTEX})
			.addShaderModule(nri::ShaderCreateInfo{"shaders/cursor.hlsl", "PSMain", nri::SHADER_TYPE_FRAGMENT})
			.setPushConstantRanges({{0, sizeof(PushConstantsCursor)}})
			.buildGraphicsProgram();

	cursorMesh = std::make_unique<fxed::QuadMesh>(nri, queue, glm::vec2(0.1, 1.0f));
}

float TextRenderer::getFontSize() const { return static_cast<float>(font.getFontSize()); }
void  TextRenderer::setFontSize(uint32_t size) { font.resize(size); }

void TextRenderer::renderText(nri::CommandBuffer &cmdBuf, const fxed::TextMesh &textMesh, const TextRenderState &renderState) {
	shader->bind(cmdBuf);

	PushConstants pushConstants{.viewportSize  = renderState.viewportSize,
								.translation   = renderState.translation,
								.textureHandle = font.getHandle(),
								.textSize	   = renderState.fontSize,
								.time		   = 0};

	shader->setPushConstants(cmdBuf, &pushConstants, sizeof(pushConstants), 0);

	textMesh.bind(cmdBuf);
	textMesh.draw(cmdBuf, *shader);

	if (renderState.showCursor) {
		PushConstantsCursor cursorPushConstants{.viewportSize = renderState.viewportSize,
												.translation  = pushConstants.translation,
												.cursorPos	  = renderState.cursorPos,
												.textSize	  = pushConstants.textSize,
												.time		  = 0};

		cursorShader->bind(cmdBuf);
		cursorShader->setPushConstants(cmdBuf, &cursorPushConstants, sizeof(cursorPushConstants), 0);
		cursorMesh->bind(cmdBuf);
		cursorMesh->draw(cmdBuf, *cursorShader);
	}
}
