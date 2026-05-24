#include <filesystem>
#include <fstream>
#include <memory>

#include "editor.hpp"
#include "nri.hpp"

using namespace fxed;

void Editor::setupCallbacks() {
	window.addResizeCallback([&](GLFWwindow *, int w, int h) { rootPane->resize(w, h); });

	fxed::Keyboard::addKeyCallback([&](GLFWwindow *, int key, int, int action, int mods) {
		// zoom with ctrl + +/-
		if (action == GLFW_PRESS || action == GLFW_REPEAT) {
			if (mods & GLFW_MOD_CONTROL) {
				if (key == GLFW_KEY_KP_ADD || key == GLFW_KEY_EQUAL) {
					textRenderer.setFontSize(textRenderer.getFontSize() + 1);
				} else if (key == GLFW_KEY_KP_SUBTRACT || key == GLFW_KEY_MINUS) {
					auto fontSize = textRenderer.getFontSize();
					fontSize	  = std::max(2.f, fontSize - 1);
					textRenderer.setFontSize(fontSize);
				} else if (key == GLFW_KEY_S) {
					if (fxed::Pane::activePane) {
						auto *textEditorPane = dynamic_cast<FileTextEditorPane *>(fxed::Pane::activePane);
						if (textEditorPane) {
							textEditorPane->saveToFile();
						}
					}
				} else if (key == GLFW_KEY_Z && !(mods & GLFW_MOD_SHIFT)) fxed::Pane::activePane->undo();
				else if (key == GLFW_KEY_R && !(mods & GLFW_MOD_SHIFT)) fxed::Pane::activePane->redo();
			}
		}
		if (fxed::Pane::activePane) { fxed::Pane::activePane->keyInput(key, 0, action, mods); }
	});

	fxed::Keyboard::addCharCallback([&](GLFWwindow *, unsigned int codepoint) {
		if (fxed::Pane::activePane) { fxed::Pane::activePane->charInput(codepoint); }
	});

	fxed::Mouse::addScrollCallback([&](GLFWwindow *, double, double yOffset) {
		if (fxed::Keyboard::getKey(GLFW_KEY_LEFT_CONTROL) || fxed::Keyboard::getKey(GLFW_KEY_RIGHT_CONTROL)) {
			auto fontSize = textRenderer.getFontSize();
			fontSize += std::copysign(1.f, (float)yOffset) * 1.0f;
			fontSize = std::max(2.f, fontSize);
			textRenderer.setFontSize(fontSize);
		} else {
			rootPane->scroll(mouse, 0, std::copysign(1.f, yOffset) * 1.0f);
		}
	});
	fxed::Mouse::addMouseButtonCallback(
		[&](GLFWwindow *, int button, int action, int mods) { rootPane->mouseClick(mouse, button, action, mods); });

	fxed::Mouse::addMouseMoveCallback(
		[&](GLFWwindow *, double xpos, double ypos) { rootPane->mouseMove(mouse, xpos, ypos); });
}

Editor::Editor(nri::NRI &nri, uint32_t width, uint32_t height)
	: nri(nri),
	  window(nri, width, height, "FxED Editor", false, true),
	  mouse(window),
	  textRenderer(nri, window.getMainQueue(),
				   fxed::FontAtlas(nri, window.getMainQueue(),
								   fxed::FontFallbackChain({
									   fxed::FontAtlas::findFontPath("FantasqueSansM Nerd Font:weight=regular"),
									   fxed::FontAtlas::findFontPath("Noto Color Emoji"),	  // fallback for emojis
								   }),
								   512, 20)) {
	rootPane			= std::make_unique<SplitPane>(nri, window.getMainQueue(), width, height, true);
	auto fileTreePane	= std::make_shared<FileTreePane>(nri, window.getMainQueue(), width, height, textRenderer);
	auto textEditorPane = std::make_shared<TabsPane>(nri, window.getMainQueue(), width, height, textRenderer);

	static_cast<SplitPane *>(rootPane.get())->setChild(std::move(fileTreePane), 0);
	static_cast<SplitPane *>(rootPane.get())->setChild(std::move(textEditorPane), 1);

	auto &win		= window.getNativeWindow();
	win->clearColor = glm::vec4(30 / 255.f, 30 / 255.f, 46 / 255.f, 1.0f);

	setupCallbacks();

	if (Editor::instance == nullptr) {
		Editor::instance = this;
	} else {
		THROW_RUNTIME_ERR("Multiple instances of Editor created!");
	}
}

Editor *Editor::instance = nullptr;

Editor &Editor::getInstance() {
	if (instance == nullptr) { THROW_RUNTIME_ERR("Editor instance not created yet!"); }
	return *instance;
}

void Editor::mainLoop() {
	auto &win = window.getNativeWindow();
	while (!window.shouldClose()) {
		window.beginFrame();
		mouse.update();
		bool res = win->beginFrame();
		if (!res) {
			window.swapBuffers();
			continue;
		}

		auto &cmdBuf = win->getCurrentCommandBuffer();
		win->beginRendering(cmdBuf, win->getCurrentRenderTarget());

		rootPane->render(cmdBuf);

		win->endRendering(cmdBuf);
		win->endFrame();

		window.swapBuffers();
	}
}

void Editor::openFile(const std::filesystem::path &path) {
	auto *splitPane = dynamic_cast<SplitPane *>(rootPane.get());
	assert(splitPane && "Root pane is not a SplitPane");

	auto *tabsPane = dynamic_cast<TabsPane *>(splitPane->getChild(1).get());
	assert(tabsPane && "Right child of root pane is not a TabsPane");

	const auto canonicalPath = std::filesystem::canonical(path);

	// check if file is already open in a tab
	for (size_t i = 0; i < tabsPane->getTabs().size(); ++i) {
		auto *textEditorPane = dynamic_cast<FileTextEditorPane *>(tabsPane->getTabs()[i].get());
		if (textEditorPane && textEditorPane->getFilePath() == canonicalPath) {
			tabsPane->setActiveTab(i);
			return;
		}
	}

	auto textEditorPane =
		std::make_unique<FileTextEditorPane>(nri, window.getMainQueue(), 100, 100, textRenderer, canonicalPath);

	tabsPane->addTab(std::move(textEditorPane));
}
