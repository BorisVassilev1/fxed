#include "text_rendering.hpp"
#include "buffer_utils.hpp"
#include "nri.hpp"
#include "resource_manager.hpp"
using namespace fxed;

TextMesh::TextMesh(nri::NRI &nri, nri::CommandQueue &q, std::size_t maxCharCount)
	: fxed::Mesh(nri, maxCharCount * 4, maxCharCount * 6, nri::MEMORY_TYPE_UPLOAD),
	  maxCharCount(maxCharCount),
	  bounds(0, 0) {
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

TextMesh::TextMesh(TextMesh &&other) noexcept
	: fxed::Mesh(std::move(other)),
	  vertexData(other.vertexData),
	  indexData(other.indexData),
	  maxCharCount(other.maxCharCount),
	  bounds(other.bounds) {
	other.vertexData = nullptr;
	other.indexData	 = nullptr;
}

TextMesh &TextMesh::operator=(TextMesh &&other) noexcept {
	if (this != &other) {
		fxed::Mesh::operator=(std::move(other));
		vertexData	 = other.vertexData;
		indexData	 = other.indexData;
		maxCharCount = other.maxCharCount;
		bounds		 = other.bounds;

		other.vertexData = nullptr;
		other.indexData	 = nullptr;
	}
	return *this;
}

TextMeshInstanced::TextMeshInstanced(nri::NRI &nri, nri::CommandQueue &q, std::size_t maxInstanceCount)
	: instanceData(nullptr, 0) {
	instanceBuffer = nri.createBuffer(maxInstanceCount * sizeof(InstanceData), nri::BUFFER_USAGE_VERTEX);
	indexBuffer	   = nri.createBuffer(6 * sizeof(uint32_t), nri::BUFFER_USAGE_INDEX);

	auto [offsets, memReq] = getBufferOffsets({instanceBuffer.get(), indexBuffer.get()});
	memory = allocateBindMemory(nri, {instanceBuffer.get(), indexBuffer.get()}, nri::MEMORY_TYPE_UPLOAD);

	auto *instanceDataPtr = (InstanceData *)memory->map();
	if (!instanceDataPtr) {
		dbLog(dbg::LOG_ERROR, "Failed to map instance buffer for TextMeshInstanced");
		THROW_RUNTIME_ERR("Failed to map instance buffer for TextMeshInstanced");
	}

	// Fill index buffer with quad indices (0, 1, 2, 2, 3, 0)
	uint32_t *indexData = (uint32_t *)((char *)instanceDataPtr + indexBuffer->getOffset());
	indexData[0]		= 0;
	indexData[1]		= 1;
	indexData[2]		= 2;
	indexData[3]		= 2;
	indexData[4]		= 3;
	indexData[5]		= 0;

	instanceData = StaticVector<InstanceData>((InstanceData *)((char *)instanceDataPtr + instanceBuffer->getOffset()),
											  maxInstanceCount);
};

void TextMeshInstanced::bind(nri::CommandBuffer &cmdBuffer) const {
	instanceBuffer->bindAsVertexBuffer(cmdBuffer, 0, 0, sizeof(InstanceData));
	indexBuffer->bindAsIndexBuffer(cmdBuffer, 0, nri::IndexType::INDEX_TYPE_UINT32);
}

void TextMeshInstanced::draw(nri::CommandBuffer &cmdBuffer, nri::GraphicsProgram &program) const {
	if (instanceData.empty()) return;
	program.drawIndexed(cmdBuffer, 6, instanceData.size(), 0, 0, 0);
}

std::vector<nri::VertexBinding> TextMeshInstanced::getVertexBindings() {
	return {{
		0, sizeof(InstanceData), nri::VERTEX_INPUT_RATE_INSTANCE,
		{
			{0, nri::FORMAT_R32G32B32A32_SFLOAT, 0, nri::VertexInputRate::VERTEX_INPUT_RATE_INSTANCE, "COLOR"},
			{1, nri::FORMAT_R32G32_SFLOAT, 4 * sizeof(float), nri::VertexInputRate::VERTEX_INPUT_RATE_INSTANCE,
			 "TRANSLATION"},
			{2, nri::FORMAT_R32G32_SINT, 6 * sizeof(float), nri::VertexInputRate::VERTEX_INPUT_RATE_INSTANCE,
			 "CHAR_INDEX_DRAW_MODE"},
		}}};
}

struct PushConstants {
	glm::ivec2			viewportSize;
	glm::vec2			translation = {0, 0};
	nri::ResourceHandle textureHandle;
	float				textSize;
	float				time;
	nri::ResourceHandle glyphGeometryBufferHandle;
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
ResourceID TextRenderer::shader_TWO_ID	= ResourceID::invalid();

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
				.buildGraphicsProgram();	 // this uses the same vertex bindings as the text shader

		cursorShaderID = ResourceManager::getInstance().addShader(std::move(cursorShader));
	}

	sb->clearShaderModules();
	if (shader_TWO_ID.invalid()) {
		auto shader =
			sb->addShaderModule(nri::ShaderCreateInfo{"shaders/text_instanced.hlsl", "VSMain", nri::SHADER_TYPE_VERTEX})
				.addShaderModule(
					nri::ShaderCreateInfo{"shaders/text_instanced.hlsl", "PSMain", nri::SHADER_TYPE_FRAGMENT})
				.setVertexBindings(fxed::TextMeshInstanced::getVertexBindings())
				.setPrimitiveType(nri::PRIMITIVE_TYPE_TRIANGLES)
				.setPushConstantRanges({{0, sizeof(PushConstants)}})
				.buildGraphicsProgram();
		shader_TWO_ID = ResourceManager::getInstance().addShader(std::move(shader));
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

	PushConstants pushConstants{.viewportSize			   = renderState.viewportSize,
								.translation			   = renderState.translation,
								.textureHandle			   = font.getHandle(),
								.textSize				   = fontSize,
								.time					   = 0,
								.glyphGeometryBufferHandle = font.getGlyphGeometryBufferHandle()};

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

void TextRenderer::renderText(nri::CommandBuffer &cmdBuf, const fxed::TextMeshInstanced &textMesh,
							  const TextRenderState &renderState) {
	auto &shader = (nri::GraphicsProgram &)shader_TWO_ID;
	shader.bind(cmdBuf);

	PushConstants pushConstants{.viewportSize			   = renderState.viewportSize,
								.translation			   = renderState.translation,
								.textureHandle			   = font.getHandle(),
								.textSize				   = fontSize,
								.time					   = 0,
								.glyphGeometryBufferHandle = font.getGlyphGeometryBufferHandle()};

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

// glm::vec2 TextMesh::updateText(fxed::any_input_range<char32_t> &&text, fxed::FontAtlas &font, glm::ivec2
// cursorPos,
//							   float lineWidth) {
//	return updateText<fxed::any_input_range<char32_t>>(std::move(text), font, cursorPos, lineWidth);
// }
