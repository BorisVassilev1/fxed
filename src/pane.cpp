#include <fstream>

#include "pane.hpp"
#include "GLFW/glfw3.h"
#include "editor.hpp"
#include "file_tree.hpp"
#include "mesh.hpp"
#include "nri.hpp"
#include "text_rendering.hpp"
#include "utf8_convert.hpp"

struct PushConstants {
	glm::vec3  color0;
	int		   borderSize;
	glm::vec3  color1;
	float	   time;
	glm::ivec2 viewportSize;
	float	   alpha;
};

fxed::ResourceID fxed::Pane::backgroundShaderID = fxed::ResourceID::invalid();
fxed::ResourceID fxed::Pane::backgroundMeshID	= fxed::ResourceID::invalid();

fxed::Pane::Pane(nri::NRI &nri, nri::CommandQueue &queue, uint32_t width, uint32_t height, std::u32string_view name)
	: nri(nri), queue(queue), position(0), size(width, height), name(std::move(name)) {
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
}

void fxed::Pane::render(nri::CommandBuffer &cmdBuf) {
	auto &backgroundShader = fxed::ResourceManager::getInstance().getShader(backgroundShaderID);
	auto &backgroundMesh   = fxed::ResourceManager::getInstance().getMesh(backgroundMeshID);

	backgroundShader.bind(cmdBuf);
	backgroundMesh.bind(cmdBuf);

	PushConstants pushConstants{.color0		  = glm::vec3(30 / 255.f, 30 / 255.f, 46 / 255.f),
								.borderSize	  = borderSize,
								.color1		  = glm::vec3(0.5f, 0.5f, 0.5f),
								.time		  = 0,
								.viewportSize = size,
								.alpha		  = 1.0f};
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

void fxed::Pane::setActive() {
	std::string nameUtf8;
	std::ranges::copy(name | fxed::to_utf8, std::back_inserter(nameUtf8));
	activePane = this;
	dbLog(dbg::LOG_INFO, "Active pane set to: ", nameUtf8);
}

bool fxed::Pane::containsPoint(glm::ivec2 point) const {
	return point.x >= position.x && point.x < position.x + size.x && point.y >= position.y &&
		   point.y < position.y + size.y;
}

std::u32string_view fxed::Pane::getName() const { return name; }

void fxed::Pane::resize(uint32_t newWidth, uint32_t newHeight) { size = {newWidth, newHeight}; }

void fxed::Pane::setTransform(uint32_t posX, uint32_t posY, uint32_t width, uint32_t height) {
	position = {posX, posY};
	resize(width, height);
}

void fxed::Pane::scroll(fxed::Mouse &, double, double) {}
void fxed::Pane::mouseClick(fxed::Mouse &, int button, int action, int) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) { setActive(); }
}

void fxed::Pane::mouseMove(fxed::Mouse &, double, double) {}
void fxed::Pane::charInput(unsigned int) {}
void fxed::Pane::keyInput(int, int, int, int) {}

fxed::TextPane::TextPane(nri::NRI &nri, nri::CommandQueue &queue, uint32_t width, uint32_t height,
						 TextRenderer &textRenderer)
	: Pane(nri, queue, width, height, U"Pane"),
	  textRenderer(textRenderer),
	  renderState(),
	  textMesh(nri, queue, 100000) {
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

void fxed::TextPane::scroll(fxed::Mouse &mouse, double deltaX, double deltaY) {
	Pane::scroll(mouse, deltaX, deltaY);
	renderState.translation += glm::ivec2(deltaX * scrollSpeed, deltaY * scrollSpeed);
}

void fxed::TextPane::resize(uint32_t newWidth, uint32_t newHeight) {
	if (newWidth == (uint32_t)size.x && newHeight == (uint32_t)size.y) return;
	Pane::resize(newWidth, newHeight);
	renderState.viewportSize = {newWidth, newHeight};
	if (wordWrap) updateText(text);
};

void fxed::TextPane::setTransform(uint32_t posX, uint32_t posY, uint32_t width, uint32_t height) {
	Pane::setTransform(posX, posY, width, height);
	renderState.viewportSize = {width, height};
}

void fxed::TextPane::updateText(const std::u32string &text) {
	this->text			  = text;
	renderState.cursorPos = textMesh.updateText<std::span<const char32_t>>(
		std::span<const char32_t>{text.begin(), text.end()}, textRenderer.getFont(), cursorPos,
		wordWrap ? getWidth() - 2 * borderSize : 0);
	textRenderer.getFont().syncWithGPU();
}

void fxed::TextPane::updateText(fxed::any_input_range<char32_t> &&text) {
	this->text.clear();
	std::ranges::copy(text, std::back_inserter(this->text));
	updateText(this->text);
}

fxed::TextEditorPane::TextEditorPane(nri::NRI &nri, nri::CommandQueue &queue, uint32_t width, uint32_t height,
									 TextRenderer &textRenderer, DefaultTextEditor &&editor)
	: TextPane(nri, queue, width, height, textRenderer), editor(std::move(editor)) {}

void fxed::TextEditorPane::render(nri::CommandBuffer &cmdBuf) {
	glm::vec2 &cursorRealPos = renderState.cursorPos;
	textRenderer.getFont().syncWithGPU();
	// TODO: this is hacky
	if (editor.hasTextChanged() || editor.hasCursorMoved() || textRenderer.getVersion() != textRendererVersion) {
		auto &&text	  = editor.getTextRange();
		cursorPos	  = editor.getCursorPos();
		cursorRealPos = textMesh.updateText(text, textRenderer.getFont(), cursorPos, wordWrap ? getWidth() : 0);
		this->text.clear();
		std::ranges::copy(text, std::back_inserter(this->text));
		editor.resetTextChanged();
		textRendererVersion = textRenderer.getVersion();
	}

	if (editor.hasCursorMoved()) {
		glm::vec2 screenBounds = glm::vec2(renderState.viewportSize) / textRenderer.getFontSize();
		// clamp translation to prevent moving text out of screen
		renderState.translation.x =
			std::clamp<float>(renderState.translation.x, -cursorRealPos.x, screenBounds.x - cursorRealPos.x - 0.5);
		renderState.translation.y =
			std::clamp<float>(renderState.translation.y, -cursorRealPos.y + 1, screenBounds.y - cursorRealPos.y - 1);
	}

	auto time =
		std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
			.count() /
		1000.f;
	renderState.showCursor = editor.milisecondsSinceLastMove() < 200 || (time - int64_t(time)) < 0.5;
	TextPane::render(cmdBuf);

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

fxed::FileTextEditorPane::FileTextEditorPane(nri::NRI &nri, nri::CommandQueue &queue, uint32_t width, uint32_t height,
											 TextRenderer &textRenderer, const std::filesystem::path &filePath)
	: TextEditorPane(nri, queue, width, height, textRenderer), filePath(filePath) {
	std::ifstream file(filePath);
	if (file.is_open()) {
		this->getEditor() = DefaultTextEditor(std::ranges::istream_view<fxed::RawChar>(file) | fxed::to_utf32);
		updateText(std::ranges::istream_view<fxed::RawChar>(file) | fxed::to_utf32);
	} else {
		dbLog(dbg::LOG_ERROR, "Failed to open file: ", filePath);
	}
	this->name.clear();
	std::ranges::copy(fxed::getIconForFile(filePath) | fxed::to_utf32, std::back_inserter(this->name));
	std::ranges::copy(filePath.filename().string() | fxed::to_utf32, std::back_inserter(this->name));
}

const std::filesystem::path &fxed::FileTextEditorPane::getFilePath() const { return filePath; }

void fxed::FileTextEditorPane::saveToFile() {
	std::ofstream file(filePath);
	if (file.is_open()) {
		std::ranges::copy(editor.getTextRange() | fxed::to_utf8, std::ostream_iterator<char>(file));
		dbLog(dbg::LOG_INFO, "Saved file: ", filePath);
	} else {
		dbLog(dbg::LOG_ERROR, "Failed to open file for writing: ", filePath);
	}
}

fxed::SplitPane::SplitPane(nri::NRI &nri, nri::CommandQueue &queue, uint32_t width, uint32_t height, bool isVertical,
						   float splitRatio)
	: Pane(nri, queue, width, height), isVertical(isVertical), splitRatio(splitRatio) {
	resize(width, height);
	this->name = U"SplitPane";
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
		child1->mouseClick(mouse, button, action, mods);
	} else if (child2 && child2->containsPoint(position)) {
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

void fxed::SplitPane::scroll(fxed::Mouse &mouse, double deltaX, double deltaY) {
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
	} else if (index == 1) {
		child2 = std::move(child);
	}
}

std::shared_ptr<fxed::Pane> &fxed::SplitPane::getChild(int index) {
	if (index == 0) {
		return child1;
	} else if (index == 1) {
		return child2;
	} else {
		throw std::out_of_range("Invalid child index");
	}
}

void fxed::FileTreePane::refreshListing() {
	std::stringstream ss;
	fileTree.print(ss);
	updateText(std::ranges::istream_view<fxed::RawChar>(ss) | fxed::to_utf32);
}

fxed::FileTreePane::FileTreePane(nri::NRI &nri, nri::CommandQueue &queue, uint32_t width, uint32_t height,
								 TextRenderer &textRenderer)
	: TextPane(nri, queue, width, height, textRenderer),
	  currentPath(std::filesystem::current_path()),
	  fileTree(currentPath),
	  selectedRow(1),
	  selectedIt(fileTree.begin()) {
	refreshListing();
	renderState.showCursor = false;
	this->wordWrap		   = false;
	this->name			   = U"FileTree";
}

void fxed::FileTreePane::render(nri::CommandBuffer &cmdBuf) {
	TextPane::render(cmdBuf);
	// draw a highlight behind the selected row
	auto &backgroundShader = fxed::ResourceManager::getInstance().getShader(backgroundShaderID);
	auto &backgroundMesh   = fxed::ResourceManager::getInstance().getMesh(backgroundMeshID);

	float		  rowHeight = textRenderer.getFontSize() * 1.2f;
	PushConstants pushConstants{.color0		  = glm::vec3(0.2f, 0.2f, 0.2f),
								.borderSize	  = 2,
								.color1		  = glm::vec3(1.0f, 1.0f, 1.0f),
								.time		  = 0,
								.viewportSize = glm::ivec2(size.x, rowHeight),
								.alpha		  = 0.0f};
	backgroundShader.bind(cmdBuf);
	backgroundShader.setPushConstants(cmdBuf, &pushConstants, sizeof(pushConstants), 0);
	backgroundMesh.bind(cmdBuf);
	glm::vec2 highlightPos =
		glm::vec2(position) + glm::vec2(0, selectedRow * rowHeight + textRenderer.getFontSize() * 0.1f);
	highlightPos.y += (renderState.translation.y - 1) * textRenderer.getFontSize();

	cmdBuf.setViewport(highlightPos.x, highlightPos.y, size.x, rowHeight, 0.0f, 1.0f);
	cmdBuf.setScissor(highlightPos.x, highlightPos.y, size.x, rowHeight);
	backgroundMesh.draw(cmdBuf, backgroundShader);
}

void fxed::FileTreePane::mouseClick(fxed::Mouse &mouse, int button, int action, int mods) {
	Pane::mouseClick(mouse, button, action, mods);
	auto position = mouse.getPosition();
	position -= this->position;
	position.y -= (renderState.translation.y - 1) * textRenderer.getFontSize();		// adjust for scrolling
	float rowHeight	 = textRenderer.getFontSize() * 1.2f;
	int	  clickedRow = position.y / rowHeight;
	if (clickedRow > 0) {	  // TODO: check upper bound as well
		{
			auto oldSelectedIt	= selectedIt;
			auto oldSelectedRow = selectedRow;
			selectedIt			= fileTree.begin();
			for (int i = 1; i < clickedRow && selectedIt != fileTree.end(); ++i) {
				++selectedIt;
			}
			if (selectedIt == fileTree.end()) {		// revert to old selection if we clicked out of bounds
				selectedIt	= oldSelectedIt;
				selectedRow = oldSelectedRow;
				return;
			} else {
				selectedRow = clickedRow;
			}
		}

		this->renderState.cursorPos = glm::vec2(0, selectedRow);
		if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
			auto &node = *selectedIt;
			if (auto *dirNode = dynamic_cast<fxed::FileTree::DirectoryNode *>(&node)) {
				dirNode->toggleOpen();
				refreshListing();
			} else if (auto *fileNode = dynamic_cast<fxed::FileTree::FileNode *>(&node)) {
				std::filesystem::path filePath = currentPath / fileNode->path;
				dbLog(dbg::LOG_INFO, "Selected file: ", filePath);
				Editor::getInstance().openFile(filePath);
			}
		}
	}
}

void fxed::FileTreePane::keyInput(int key, int scancode, int action, int mods) {
	Pane::keyInput(key, scancode, action, mods);
	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		if (key == GLFW_KEY_DOWN) {
			if (!selectedIt.isBack()) {
				++selectedIt;
				++selectedRow;
				this->renderState.cursorPos = glm::vec2(0, selectedRow);
			}
		} else if (key == GLFW_KEY_UP) {
			if (!selectedIt.isBegin()) {
				--selectedIt;
				--selectedRow;
				this->renderState.cursorPos = glm::vec2(0, selectedRow);
			}
		} else if (key == GLFW_KEY_ENTER || key == GLFW_KEY_RIGHT) {
			auto &node = *selectedIt;
			if (auto *dirNode = dynamic_cast<fxed::FileTree::DirectoryNode *>(&node)) {
				dirNode->toggleOpen();
				refreshListing();
			} else if (auto *fileNode = dynamic_cast<fxed::FileTree::FileNode *>(&node)) {
				if (key != GLFW_KEY_ENTER) return;
				std::filesystem::path filePath = currentPath / fileNode->path;
				dbLog(dbg::LOG_INFO, "Selected file: ", filePath);
				Editor::getInstance().openFile(filePath);
			}
		}
	}
}

void fxed::FileTreePane::setPath(const std::filesystem::path &p) {
	currentPath = p;
	fileTree	= FileTree(currentPath);
	selectedRow = 1;
	selectedIt	= fileTree.begin();
	refreshListing();
}

const std::filesystem::path &fxed::FileTreePane::getPath() const { return currentPath; }

void fxed::TabsPane::placeTab(std::shared_ptr<Pane> &pane) {
	pane->setTransform(position.x, position.y + textRenderer.getFontSize() * 1.5f, size.x,
					   size.y - textRenderer.getFontSize() * 1.5f);
}

fxed::TabsPane::TabsPane(nri::NRI &nri, nri::CommandQueue &queue, uint32_t width, uint32_t height,
						 TextRenderer &textRenderer)
	: Pane(nri, queue, width, height), textRenderer(textRenderer) {
	this->name = U"Tabs";
}

static std::u32string getTabName(const std::shared_ptr<fxed::Pane> &pane) {
	std::u32string tabName;
	std::ranges::copy(pane->getName(), std::back_inserter(tabName));
	tabName += U"  ";
	return tabName;
}

uint32_t fxed::TabsPane::addTab(std::shared_ptr<Pane> &&pane) {
	tabs.push_back(std::move(pane));
	setTransform(position.x, position.y, size.x, size.y);
	tabMeshes.emplace_back(nri, queue, 100);
	tabMeshes.back().updateText(getTabName(tabs.back()), textRenderer.getFont(), glm::ivec2(0, 0), 0);
	return tabs.size() - 1;
}

void fxed::TabsPane::render(nri::CommandBuffer &cmdBuf) {
	// render tab headers
	auto &backgroundShader = fxed::ResourceManager::getInstance().getShader(backgroundShaderID);
	auto &backgroundMesh   = fxed::ResourceManager::getInstance().getMesh(backgroundMeshID);

	if (textRenderer.getVersion() != textRendererVersion) {
		for (auto &tab : tabs) {
			tab->setTransform(position.x, position.y + textRenderer.getFontSize() * 1.5f, size.x,
							  size.y - textRenderer.getFontSize() * 1.5f);
		}
		for (size_t i = 0; i < tabs.size(); ++i) {
			tabMeshes[i].updateText(getTabName(tabs[i]), textRenderer.getFont(), glm::ivec2(0, 0), 0);
		}
		textRendererVersion = textRenderer.getVersion();
	}

	float		  tabHeight = textRenderer.getFontSize() * 1.5f;
	PushConstants pushConstants{.color0		  = glm::vec3(0.0f, 0.0f, 0.0f),
								.borderSize	  = 2,
								.color1		  = glm::vec3(1.0f, 1.0f, 1.0f),
								.time		  = 0,
								.viewportSize = glm::ivec2(0, tabHeight),
								.alpha		  = 0.0f};
	backgroundShader.bind(cmdBuf);
	backgroundMesh.bind(cmdBuf);

	float xOffset = 0;
	for (size_t i = 0; i < tabs.size(); ++i) {
		if (i == activeTab) {
			pushConstants.color1 = glm::vec3(1.0f, 1.0f, 1.0f);
		} else {
			pushConstants.color1 = glm::vec3(0.5f, 0.5f, 0.5f);
		}

		float tabWidth				 = tabMeshes[i].getBounds().x * textRenderer.getFontSize() + 20;
		pushConstants.viewportSize.x = tabWidth;

		backgroundShader.setPushConstants(cmdBuf, &pushConstants, sizeof(pushConstants), 0);
		glm::vec2 tabPos = glm::vec2(position) + glm::vec2(xOffset, 0);
		cmdBuf.setViewport(tabPos.x, tabPos.y, tabWidth, tabHeight, 0.0f, 1.0f);
		cmdBuf.setScissor(tabPos.x, tabPos.y, tabWidth, tabHeight);
		backgroundMesh.draw(cmdBuf, backgroundShader);

		xOffset += tabWidth;
	}

	xOffset = 0;
	for (size_t i = 0; i < tabs.size(); ++i) {
		float			tabWidth = tabMeshes[i].getBounds().x * textRenderer.getFontSize() + 20;
		glm::vec2		tabPos	 = glm::vec2(position) + glm::vec2(xOffset, 0);
		TextRenderState tabRenderState{
			.translation  = glm::vec2(0, 1),
			.viewportSize = glm::ivec2(tabWidth, tabHeight),
			.cursorPos	  = glm::vec2(0, 0),
			.showCursor	  = false,
		};
		cmdBuf.setViewport(tabPos.x + 10, tabPos.y, tabWidth, tabHeight, 0.0f, 1.0f);
		cmdBuf.setScissor(tabPos.x + 10, tabPos.y, tabWidth, tabHeight);
		textRenderer.renderText(cmdBuf, tabMeshes[i], tabRenderState);
		xOffset += tabWidth;
	}

	// render active child
	if (!tabs.empty()) {
		assert(activeTab >= 0 && activeTab < tabs.size());
		tabs[activeTab]->render(cmdBuf);
	}
}

void fxed::TabsPane::mouseClick(fxed::Mouse &mouse, int button, int action, int mods) {
	auto position = mouse.getPosition();
	position -= this->position;
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		float xOffset	= 0;
		float tabHeight = textRenderer.getFontSize() * 1.5f;
		for (size_t i = 0; i < tabs.size(); ++i) {
			float tabWidth			  = tabMeshes[i].getBounds().x * textRenderer.getFontSize() + 20;
			float closeButtonPosition = tabWidth - 10 - textRenderer.getFontSize();
			if (position.x >= xOffset && position.x < xOffset + closeButtonPosition && position.y >= 0 &&
				position.y < tabHeight) {
				setActiveTab(i);
				return;
			}
			if (position.x >= xOffset + closeButtonPosition && position.x < xOffset + tabWidth && position.y >= 0 &&
				position.y < tabHeight) {
				tabs.erase(tabs.begin() + i);
				tabMeshes.erase(tabMeshes.begin() + i);
				if (activeTab >= i) { setActiveTab(std::max(0u, activeTab - 1)); }
				return;
			}
			xOffset += tabWidth;
		}
	}

	if (!tabs.empty() && position.y >= textRenderer.getFontSize() * 1.5f) {
		assert(activeTab >= 0 && activeTab < tabs.size());
		tabs[activeTab]->mouseClick(mouse, button, action, mods);
	} else Pane::mouseClick(mouse, button, action, mods);
}

void fxed::TabsPane::resize(uint32_t newWidth, uint32_t newHeight) {
	Pane::resize(newWidth, newHeight);
	if (!tabs.empty()) {
		assert(activeTab >= 0 && activeTab < tabs.size());
		placeTab(tabs[activeTab]);
	}
}

void fxed::TabsPane::setTransform(uint32_t posX, uint32_t posY, uint32_t width, uint32_t height) {
	Pane::setTransform(posX, posY, width, height);
	if (!tabs.empty()) {
		assert(activeTab >= 0 && activeTab < tabs.size());
		placeTab(tabs[activeTab]);
	}
}

void fxed::TabsPane::scroll(fxed::Mouse &mouse, double deltaX, double deltaY) {
	if (!tabs.empty()) {
		assert(activeTab >= 0 && activeTab < tabs.size());
		tabs[activeTab]->scroll(mouse, deltaX, deltaY);
	} else {
		Pane::scroll(mouse, deltaX, deltaY);
	}
}

void fxed::TabsPane::mouseMove(fxed::Mouse &mouse, double deltaX, double deltaY) {
	if (!tabs.empty()) {
		assert(activeTab >= 0 && activeTab < tabs.size());
		tabs[activeTab]->mouseMove(mouse, deltaX, deltaY);
	} else {
		Pane::mouseMove(mouse, deltaX, deltaY);
	}
}

void fxed::TabsPane::setActiveTab(uint32_t index) {
	if (index < tabs.size()) {
		activeTab = index;
		tabs[activeTab]->setActive();
		placeTab(tabs[activeTab]);
	}
}
