#include "text_rendering.hpp"
#include "nri.hpp"
#include "resource_manager.hpp"
using namespace fxed;

TextMesh::TextMesh(nri::NRI &nri, nri::CommandQueue &q, std::size_t maxCharCount)
	: fxed::Mesh(nri, maxCharCount * 4, maxCharCount * 6, nri::MEMORY_TYPE_UPLOAD), maxCharCount(maxCharCount), bounds(0, 0) {
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

struct PushConstants {
	glm::ivec2			viewportSize;
	glm::vec2			translation = {0, 0};
	nri::ResourceHandle textureHandle;
	float				textSize;
	float				time;
};

struct PushConstantsCursor {
	glm::ivec2 viewportSize;
	glm::vec2  translation = {0, 0};
	glm::vec2  cursorPos;
	float	   textSize;
	float	   time;
};

ResourceID TextRenderer::cursorMeshID	= ResourceID::invalid();
ResourceID TextRenderer::shaderID		= ResourceID::invalid();
ResourceID TextRenderer::cursorShaderID = ResourceID::invalid();

TextRenderer::TextRenderer(nri::NRI &nri, nri::CommandQueue &queue, fxed::FontAtlas &&font)
	: font(std::move(font)), fontSize(this->font.getFontSize()) {
	auto sb = nri.createProgramBuilder();
	if (shaderID.invalid()) {
		auto shader =
			sb->addShaderModule(nri::ShaderCreateInfo{"shaders/text.hlsl", "VSMain", nri::SHADER_TYPE_VERTEX})
				.addShaderModule(nri::ShaderCreateInfo{"shaders/text.hlsl", "PSMain", nri::SHADER_TYPE_FRAGMENT})
				.setVertexBindings(fxed::TextMesh::getVertexBindings())
				.setPrimitiveType(nri::PRIMITIVE_TYPE_TRIANGLES)
				.setPushConstantRanges({{0, sizeof(PushConstants)}})
				.buildGraphicsProgram();
		shaderID = ResourceManager::getInstance().addShader(std::move(shader));
	}

	sb->clearShaderModules();
	if (cursorShaderID.invalid()) {
		auto cursorShader =
			sb->addShaderModule(nri::ShaderCreateInfo{"shaders/cursor.hlsl", "VSMain", nri::SHADER_TYPE_VERTEX})
				.addShaderModule(nri::ShaderCreateInfo{"shaders/cursor.hlsl", "PSMain", nri::SHADER_TYPE_FRAGMENT})
				.setPushConstantRanges({{0, sizeof(PushConstantsCursor)}})
				.buildGraphicsProgram();

		cursorShaderID = ResourceManager::getInstance().addShader(std::move(cursorShader));
	}

	if (cursorMeshID.invalid()) {
		auto cursorMesh = std::make_unique<fxed::QuadMesh>(nri, queue, glm::vec2(0.1f, 1.0f));
		cursorMeshID	= ResourceManager::getInstance().addMesh(std::move(cursorMesh));
	}
}

float TextRenderer::getFontSize() const { return fontSize; }

void TextRenderer::setFontSize(uint32_t size) {
	fontSize = static_cast<float>(size);
	font.resize(size);
	version++;
}

void TextRenderer::renderText(nri::CommandBuffer &cmdBuf, const fxed::TextMesh &textMesh,
							  const TextRenderState &renderState) {
	auto &shader = (nri::GraphicsProgram &)shaderID;
	shader.bind(cmdBuf);

	PushConstants pushConstants{.viewportSize  = renderState.viewportSize,
								.translation   = renderState.translation,
								.textureHandle = font.getHandle(),
								.textSize	   = fontSize,
								.time		   = 0};

	shader.setPushConstants(cmdBuf, &pushConstants, sizeof(pushConstants), 0);

	textMesh.bind(cmdBuf);
	textMesh.draw(cmdBuf, shader);

	if (renderState.showCursor) {
		PushConstantsCursor cursorPushConstants{.viewportSize = renderState.viewportSize,
												.translation  = pushConstants.translation,
												.cursorPos	  = renderState.cursorPos,
												.textSize	  = pushConstants.textSize,
												.time		  = 0};

		auto &cursorShader = (nri::GraphicsProgram &)cursorShaderID;
		cursorShader.bind(cmdBuf);
		cursorShader.setPushConstants(cmdBuf, &cursorPushConstants, sizeof(cursorPushConstants), 0);
		auto &cursorMesh = (fxed::Mesh &)cursorMeshID;
		cursorMesh.bind(cmdBuf);
		cursorMesh.draw(cmdBuf, cursorShader);
	}
}
