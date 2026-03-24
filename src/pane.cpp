#include "pane.hpp"

struct PushConstants {
	glm::vec3  color0;
	int		   borderSize;
	glm::vec3  color1;
	float	   time;
	glm::ivec2 viewportSize;
};

fxed::ResourceID fxed::Pane::backgroundShaderID = fxed::ResourceID::invalid();
fxed::ResourceID fxed::Pane::backgroundMeshID	= fxed::ResourceID::invalid();

fxed::Pane::Pane(nri::NRI &nri, nri::CommandQueue &queue, uint32_t width, uint32_t height) : size(width, height) {
	if (backgroundShaderID.invalid()) {
		auto sb = nri.createProgramBuilder();
		auto backgroundShader =
			sb->addShaderModule(nri::ShaderCreateInfo{"shaders/pane.hlsl", "VSMain", nri::SHADER_TYPE_VERTEX})
				.addShaderModule(nri::ShaderCreateInfo{"shaders/pane.hlsl", "PSMain", nri::SHADER_TYPE_FRAGMENT})
				.setVertexBindings(fxed::QuadMesh::getVertexBindings())
				.setPrimitiveType(nri::PRIMITIVE_TYPE_TRIANGLES)
				.setPushConstantRanges({{0, sizeof(PushConstants)}})
				.buildGraphicsProgram();
		backgroundShaderID = fxed::ResourceManager::getInstance().addShader(std::move(backgroundShader));
	}
	if (backgroundMeshID.invalid()) {
		auto backgroundMesh = std::make_unique<fxed::QuadMesh>(nri, queue, glm::vec2{2.0, 2.0});
		backgroundMeshID	= fxed::ResourceManager::getInstance().addMesh(std::move(backgroundMesh));
	}
}

void fxed::Pane::render(nri::CommandBuffer &cmdBuf) {
	auto &backgroundShader = fxed::ResourceManager::getInstance().getShader(backgroundShaderID);
	auto &backgroundMesh   = fxed::ResourceManager::getInstance().getMesh(backgroundMeshID);

	backgroundShader.bind(cmdBuf);
	backgroundMesh.bind(cmdBuf);

	PushConstants pushConstants{
		.color0		  = glm::vec3(30 / 255.f, 30 / 255.f, 46 / 255.f),
		.borderSize	  = 2,
		.color1		  = glm::vec3(1.0f, 1.0f, 1.0f),
		.time		  = 0,
		.viewportSize = size,
	};

	backgroundShader.setPushConstants(cmdBuf, &pushConstants, sizeof(pushConstants), 0);
	cmdBuf.setViewport(position.x, position.y, size.x, size.y, 0.0f, 1.0f);
	cmdBuf.setScissor(position.x, position.y, size.x, size.y);

	backgroundMesh.draw(cmdBuf, backgroundShader);
}

fxed::Pane *fxed::Pane::activePane;
fxed::Pane *fxed::Pane::getActivePane() { return activePane; }

uint32_t fxed::Pane::getWidth() const { return size.x; }
uint32_t fxed::Pane::getHeight() const { return size.y; }

void fxed::Pane::resize(uint32_t newWidth, uint32_t newHeight) { size = {newWidth, newHeight}; }

void fxed::Pane::setTransform(uint32_t posX, uint32_t posY, uint32_t width, uint32_t height) {
	position = {posX, posY};
	size	 = {width, height};
}

void fxed::Pane::scroll(int, int) {}

fxed::TextPane::TextPane(nri::NRI &nri, nri::CommandQueue &queue, uint32_t width, uint32_t height,
						 TextRenderer &textRenderer)
	: Pane(nri, queue, width, height), textRenderer(textRenderer), textMesh(nri, queue, 10000) {
	renderState.viewportSize = {width, height};
}

void fxed::TextPane::render(nri::CommandBuffer &cmdBuf) {
	Pane::render(cmdBuf);
	textRenderer.renderText(cmdBuf, textMesh, renderState);
}

void fxed::TextPane::scroll(int deltaX, int deltaY) { renderState.translation += glm::ivec2(deltaX, deltaY); }
void fxed::TextPane::resize(uint32_t newWidth, uint32_t newHeight) {
	Pane::resize(newWidth, newHeight);
	renderState.viewportSize = {newWidth, newHeight};
};

void fxed::TextPane::setTransform(uint32_t posX, uint32_t posY, uint32_t width, uint32_t height) {
	Pane::setTransform(posX, posY, width, height);
	renderState.viewportSize = {width, height};
}

fxed::TextEditorPane::TextEditorPane(nri::NRI &nri, nri::CommandQueue &queue, uint32_t width, uint32_t height,
									 TextRenderer &textRenderer, TextEditor &&editor)
	: TextPane(nri, queue, width, height, textRenderer), editor(std::move(editor)) {}

void fxed::TextEditorPane::render(nri::CommandBuffer &cmdBuf) {
	TextPane::render(cmdBuf);

	std::u32string text			 = editor.getText();
	glm::vec2	  &cursorRealPos = renderState.cursorPos;
	textRenderer.getFont().syncWithGPU();
	if (editor.hasTextChanged())
		cursorRealPos = textMesh.updateText(std::span<const char32_t>{text.begin(), text.end()}, textRenderer.getFont(),
											editor.getCursorPos(), getWidth());

	if (editor.hasCursorMoved()) {
		glm::vec2 screenBounds = glm::vec2(renderState.viewportSize) / textRenderer.getFontSize();
		// clamp translation to prevent moving text out of screen
		renderState.translation.x =
			std::clamp<float>(renderState.translation.x, -cursorRealPos.x, screenBounds.x - cursorRealPos.x - 1);
		renderState.translation.y =
			std::clamp<float>(renderState.translation.y, -cursorRealPos.y + 1, screenBounds.y - cursorRealPos.y - 1);
	}
	auto time			   = glfwGetTime();
	renderState.showCursor = editor.milisecondsSinceLastMove() < 200 || (time - int64_t(time)) < 0.5;

	editor.resetCursorMoved();
}
