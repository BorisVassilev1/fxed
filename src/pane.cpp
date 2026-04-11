#include "pane.hpp"
#include "text_rendering.hpp"

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
	: position(0), size(width, height) {
	if (!backgroundShaderID.isValid()) {
		auto sb = nri.createProgramBuilder();
		auto backgroundShader =
			sb->addShaderModule(nri::ShaderCreateInfo{"shaders/pane.hlsl", "VSMain", nri::SHADER_TYPE_VERTEX})
				.addShaderModule(nri::ShaderCreateInfo{"shaders/pane.hlsl", "PSMain", nri::SHADER_TYPE_FRAGMENT})
				.setVertexBindings(fxed::QuadMesh::getVertexBindings())
				.setPrimitiveType(nri::PRIMITIVE_TYPE_TRIANGLES)
				.setPushConstantRanges({{0, sizeof(PushConstants)}})
				.buildGraphicsProgram();
		backgroundShaderID = fxed::ResourceManager::getInstance().addShader(std::move(backgroundShader));
		dbLog(dbg::LOG_INFO, "Created background shader with ID: ", backgroundShaderID);
	}
	if (!backgroundMeshID.isValid()) {
		auto backgroundMesh = std::make_unique<fxed::QuadMesh>(nri, queue, glm::vec2{2.0, 2.0});
		backgroundMeshID	= fxed::ResourceManager::getInstance().addMesh(std::move(backgroundMesh));
	}
	this->setActive();
}

void fxed::Pane::render(nri::CommandBuffer &cmdBuf) {
	auto &backgroundShader = fxed::ResourceManager::getInstance().getShader(backgroundShaderID);
	auto &backgroundMesh   = fxed::ResourceManager::getInstance().getMesh(backgroundMeshID);

	backgroundShader.bind(cmdBuf);
	backgroundMesh.bind(cmdBuf);

	PushConstants pushConstants{
		.color0		  = glm::vec3(30 / 255.f, 30 / 255.f, 46 / 255.f),
		.borderSize	  = borderSize,
		.color1		  = glm::vec3(0.5f, 0.5f, 0.5f),
		.time		  = 0,
		.viewportSize = size,
	};
	if (activePane == this) { pushConstants.color1 = glm::vec3(1.0f, 1.0f, 1.0f); }

	backgroundShader.setPushConstants(cmdBuf, &pushConstants, sizeof(pushConstants), 0);
	cmdBuf.setViewport(position.x, position.y, size.x, size.y, 0.0f, 1.0f);
	cmdBuf.setScissor(position.x, position.y, size.x, size.y);

	backgroundMesh.draw(cmdBuf, backgroundShader);
}

fxed::Pane *fxed::Pane::activePane;
fxed::Pane *fxed::Pane::getActivePane() { return activePane; }

uint32_t fxed::Pane::getWidth() const { return size.x; }
uint32_t fxed::Pane::getHeight() const { return size.y; }

void fxed::Pane::setActive() { activePane = this; }

bool fxed::Pane::containsPoint(glm::ivec2 point) const {
	return point.x >= position.x && point.x < position.x + size.x && point.y >= position.y &&
		   point.y < position.y + size.y;
}

void fxed::Pane::resize(uint32_t newWidth, uint32_t newHeight) { size = {newWidth, newHeight}; }

void fxed::Pane::setTransform(uint32_t posX, uint32_t posY, uint32_t width, uint32_t height) {
	position = {posX, posY};
	resize(width, height);
}

void fxed::Pane::scroll(fxed::Mouse &, int, int) {}
void fxed::Pane::mouseClick(fxed::Mouse &, int button, int action, int) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) { setActive(); }
}

void fxed::Pane::mouseMove(fxed::Mouse &, double, double) {}
void fxed::Pane::charInput(unsigned int) {}
void fxed::Pane::keyInput(int, int, int, int) {}

fxed::TextPane::TextPane(nri::NRI &nri, nri::CommandQueue &queue, uint32_t width, uint32_t height,
						 TextRenderer &textRenderer)
	: Pane(nri, queue, width, height), textRenderer(textRenderer), renderState(), textMesh(nri, queue, 10000) {
	renderState.viewportSize = {width, height};
	renderState.translation	 = {0, 1};
}

void fxed::TextPane::render(nri::CommandBuffer &cmdBuf) {
	// if the text renderer changed atlas may have changed, so we need to update the text mesh
	if (textRenderer.getVersion() != textRendererVersion && !text.empty()) {
		updateText(text);
		textRendererVersion = textRenderer.getVersion();
	}
	// don't allow scrolling up before the first line
	renderState.translation.y = std::min(renderState.translation.y, 1.f);

	renderState.translation.y = std::max(renderState.translation.y, -textMesh.getBounds().y + 1);

	renderState.translation.x = std::min(renderState.translation.x, 0.f);

	Pane::render(cmdBuf);
	TextRenderState currentRenderState = renderState;
	currentRenderState.translation += (borderSize) / textRenderer.getFontSize();
	textRenderer.renderText(cmdBuf, textMesh, currentRenderState);
}

void fxed::TextPane::scroll(fxed::Mouse &mouse, int deltaX, int deltaY) {
	Pane::scroll(mouse, deltaX, deltaY);
	renderState.translation += glm::ivec2(deltaX, deltaY);
}

void fxed::TextPane::resize(uint32_t newWidth, uint32_t newHeight) {
	Pane::resize(newWidth, newHeight);
	renderState.viewportSize = {newWidth, newHeight};
	if (!text.empty()) updateText(text);
};

void fxed::TextPane::setTransform(uint32_t posX, uint32_t posY, uint32_t width, uint32_t height) {
	Pane::setTransform(posX, posY, width, height);
	renderState.viewportSize = {width, height};
}

void fxed::TextPane::updateText(const std::u32string &text) {
	this->text			  = text;
	renderState.cursorPos = textMesh.updateText(std::span<const char32_t>{text.begin(), text.end()},
												textRenderer.getFont(), glm::ivec2(0, 0), getWidth() - 2 * borderSize);
	textRenderer.getFont().syncWithGPU();
}

fxed::TextEditorPane::TextEditorPane(nri::NRI &nri, nri::CommandQueue &queue, uint32_t width, uint32_t height,
									 TextRenderer &textRenderer, DefaultTextEditor &&editor)
	: TextPane(nri, queue, width, height, textRenderer), editor(std::move(editor)) {}

void fxed::TextEditorPane::render(nri::CommandBuffer &cmdBuf) {
	TextPane::render(cmdBuf);

	glm::vec2 &cursorRealPos = renderState.cursorPos;
	textRenderer.getFont().syncWithGPU();
	// TODO: this is hacky
	if (editor.hasTextChanged() || editor.hasCursorMoved() || textRenderer.getVersion() != textRendererVersion) {
		auto text	  = editor.getTextRange();
		cursorRealPos = textMesh.updateText(text, textRenderer.getFont(), editor.getCursorPos(), getWidth());
		editor.resetTextChanged();
		textRendererVersion = textRenderer.getVersion();
	}

	if (editor.hasCursorMoved()) {
		glm::vec2 screenBounds = glm::vec2(renderState.viewportSize) / textRenderer.getFontSize();
		// clamp translation to prevent moving text out of screen
		renderState.translation.x =
			std::clamp<float>(renderState.translation.x, -cursorRealPos.x, screenBounds.x - cursorRealPos.x - 1);
		renderState.translation.y =
			std::clamp<float>(renderState.translation.y, -cursorRealPos.y + 1, screenBounds.y - cursorRealPos.y - 1);
	}

	auto time =
		std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
			.count() /
		1000.f;
	renderState.showCursor = editor.milisecondsSinceLastMove() < 200 || (time - int64_t(time)) < 0.5;

	editor.resetCursorMoved();
}

void fxed::TextEditorPane::charInput(unsigned int codepoint) {
	TextPane::charInput(codepoint);
	editor.insertChar(codepoint);
}

void fxed::TextEditorPane::keyInput(int key, int scancode, int action, int mods) {
	TextPane::keyInput(key, scancode, action, mods);
	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		switch (key) {
			case GLFW_KEY_BACKSPACE: this->editor.deleteChar(); break;
			case GLFW_KEY_ENTER: this->editor.insertChar('\n'); break;
			case GLFW_KEY_TAB: this->editor.insertChar('\t'); break;
			case GLFW_KEY_LEFT: this->editor.moveCursor(-1, 0); break;
			case GLFW_KEY_RIGHT: this->editor.moveCursor(1, 0); break;
			case GLFW_KEY_UP: this->editor.moveCursor(0, -1); break;
			case GLFW_KEY_DOWN: this->editor.moveCursor(0, 1); break;
			default: break;
		}
	}
}

void fxed::TextEditorPane::undo() { editor.undo(); }
void fxed::TextEditorPane::redo() { editor.redo(); }

fxed::SplitPane::SplitPane(nri::NRI &nri, nri::CommandQueue &queue, uint32_t width, uint32_t height, bool isVertical,
						   float splitRatio)
	: Pane(nri, queue, width, height), isVertical(isVertical), splitRatio(splitRatio) {
	resize(width, height);
}

void fxed::SplitPane::render(nri::CommandBuffer &cmdBuf) {
	if (child1) child1->render(cmdBuf);
	if (child2) child2->render(cmdBuf);
}

void fxed::SplitPane::resize(uint32_t newWidth, uint32_t newHeight) {
	Pane::resize(newWidth, newHeight);
	if (isVertical) {
		if (child1) child1->setTransform(position.x, position.y, newWidth * splitRatio, newHeight);
		if (child2)
			child2->setTransform(position.x + newWidth * splitRatio, position.y, newWidth * (1 - splitRatio),
								 newHeight);
	} else {
		if (child1) child1->setTransform(position.x, position.y, newWidth, newHeight * splitRatio);
		if (child2)
			child2->setTransform(position.x, position.y + newHeight * splitRatio, newWidth,
								 newHeight * (1 - splitRatio));
	}
}

void fxed::SplitPane::setTransform(uint32_t posX, uint32_t posY, uint32_t width, uint32_t height) {
	Pane::setTransform(posX, posY, width, height);
	resize(width, height);
}

void fxed::SplitPane::mouseClick(fxed::Mouse &mouse, int button, int action, int mods) {
	auto position = mouse.getPosition();
	// check if we are close to the split line for dragging
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (isVertical) {
			float splitX = this->position.x + this->size.x * splitRatio;
			if (std::abs(position.x - splitX) < 5) {
				isDragging = action == GLFW_PRESS;
				return;
			}
		} else {
			float splitY = this->position.y + this->size.y * splitRatio;
			if (std::abs(position.y - splitY) < 5) {
				isDragging = action == GLFW_PRESS;
				return;
			}
		}
	}

	if (child1 && child1->containsPoint(position)) {
		child1->setActive();
		child1->mouseClick(mouse, button, action, mods);
	} else if (child2 && child2->containsPoint(position)) {
		child2->setActive();
		child2->mouseClick(mouse, button, action, mods);
	} else {
		Pane::mouseClick(mouse, button, action, mods);
	}
}

void fxed::SplitPane::mouseMove(fxed::Mouse &mouse, double deltaX, double deltaY) {
	if (isDragging) {
		if (isVertical) {
			float newSplitRatio = (mouse.getPosition().x - position.x) / size.x;
			setSplitRatio(newSplitRatio);
		} else {
			float newSplitRatio = (mouse.getPosition().y - position.y) / size.y;
			setSplitRatio(newSplitRatio);
		}
		return;
	}

	auto position = mouse.getPosition();
	if (child1 && child1->containsPoint(position)) {
		child1->mouseMove(mouse, deltaX, deltaY);
	} else if (child2 && child2->containsPoint(position)) {
		child2->mouseMove(mouse, deltaX, deltaY);
	} else {
		Pane::mouseMove(mouse, deltaX, deltaY);
	}
}

void fxed::SplitPane::scroll(fxed::Mouse &mouse, int deltaX, int deltaY) {
	auto position = mouse.getPosition();
	if (child1 && child1->containsPoint(position)) {
		child1->scroll(mouse, deltaX, deltaY);
	} else if (child2 && child2->containsPoint(position)) {
		child2->scroll(mouse, deltaX, deltaY);
	} else {
		Pane::scroll(mouse, deltaX, deltaY);
	}
}

void fxed::SplitPane::setSplitRatio(float ratio) {
	splitRatio = std::clamp(ratio, 0.0f, 1.0f);
	resize(this->size.x, this->size.y);
}

void fxed::SplitPane::setVertical(bool isVertical) {
	this->isVertical = isVertical;
	resize(this->size.x, this->size.y);
}

void fxed::SplitPane::setChild(std::shared_ptr<Pane> &&child, int index) {
	if (index == 0) {
		child1 = std::move(child);
		child1->setActive();
	} else if (index == 1) {
		child2 = std::move(child);
		child2->setActive();
	}
}
