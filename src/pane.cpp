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

fxed::Pane::Pane(nri::NRI &nri, nri::CommandQueue &queue, uint32_t width, uint32_t height)
	: width(width), height(height) {
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
		.viewportSize = {width, height},
	};

	backgroundShader.setPushConstants(cmdBuf, &pushConstants, sizeof(pushConstants), 0);

	backgroundMesh.draw(cmdBuf, backgroundShader);
}

fxed::Pane *fxed::Pane::activePane;
fxed::Pane *fxed::Pane::getActivePane() { return activePane; }

uint32_t fxed::Pane::getWidth() const { return width; }
uint32_t fxed::Pane::getHeight() const { return height; }

void fxed::Pane::resize(uint32_t newWidth, uint32_t newHeight) {
	width  = newWidth;
	height = newHeight;
}

void fxed::Pane::scroll(int, int) {}

fxed::TextPane::TextPane(nri::NRI &nri, nri::CommandQueue &queue, uint32_t width, uint32_t height,
						 TextRenderer &textRenderer)
	: Pane(nri, queue, width, height), textRenderer(textRenderer), textMesh(nri, queue, 10000) {
	renderState.viewportSize = {800, 600};
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
