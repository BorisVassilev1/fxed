#include <iostream>

#include <fstream>

#include "font.hpp"
#include "input.hpp"
#include "mesh.hpp"
#include "nri.hpp"
#include "nriFactory.hpp"
#include "glfw_window.hpp"
#include "text_editor.hpp"
#include "text_rendering.hpp"
#include "utf8_convert.hpp"
#include "pane.hpp"

int main(int argc, char *argv[]) {
	// 🚀 asd
	// 🐊
	// 🐊
	// 🐊
	auto nri = nri::Factory::getInstance().createNRI("Vulkan", nri::CreateBits::GLFW);
	//
	// 🔔 asd
	fxed::Window window(*nri, 800, 600, "Vulkan NRI Window");

	// glfwSetWindowAttrib(window.getHandle(), GLFW_DECORATED, !glfwGetWindowAttrib(window.getHandle(),
	// GLFW_DECORATED));
	auto &win		= window.getNativeWindow();
	win->clearColor = glm::vec4(30 / 255.f, 30 / 255.f, 46 / 255.f, 1.0f);

	// std::string fontPath = fxed::Font::getDefaultSystemFontPath();
	std::string fontPath = fxed::FontAtlas::findFontPath("FantasqueSansM Nerd Font:weight=regular");
	dbLog(dbg::LOG_INFO, "Using font: %s", fontPath.c_str());

	auto fallbackChain = fxed::FontFallbackChain({
		fontPath,
		fxed::FontAtlas::findFontPath("Noto Color Emoji"),	   // fallback for emojis
	});

	fxed::TextRenderer textRenderer(*nri, window.getMainQueue(),
									fxed::FontAtlas(*nri, window.getMainQueue(), std::move(fallbackChain), 512, 20));

	// load argv[1] if exists
	std::u32string text;
	if (argc > 1) {
		std::ifstream file(argv[1]);
		if (file.is_open()) {
			std::ranges::copy(std::ranges::istream_view<fxed::RawChar>(file) | fxed::to_utf32,
							  std::back_inserter(text));

			file.close();
		} else {
			dbLog(dbg::LOG_ERROR, "Failed to open file: ", argv[1]);
		}
	}

	std::shared_ptr<fxed::TextEditorPane> pane = std::make_unique<fxed::TextEditorPane>(
		*nri, window.getMainQueue(), window.getWidth(), window.getHeight(), textRenderer, TextEditor(text));

	TextEditorController editorController(pane->getEditor());

	fxed::SplitPane splitPane(*nri, window.getMainQueue(), window.getWidth(), window.getHeight(), true);
	splitPane.setChild(pane, 0);
	//splitPane.setChild(pane, 1);

	window.addResizeCallback([&](GLFWwindow *, int w, int h) { splitPane.resize(w, h); });

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
					std::ofstream file(argv[1] ? argv[1] : "output.txt");
					if (file.is_open()) {
						std::u32string text = pane->getEditor().getText();
						std::ranges::copy(text | fxed::to_utf8, std::ostream_iterator<char>(std::cout));
						std::ranges::copy(text | fxed::to_utf8, std::ostream_iterator<char>(file));
						dbLog(dbg::LOG_INFO, "Saved text to output.txt");
					} else {
						dbLog(dbg::LOG_ERROR, "Failed to open output.txt for writing");
					}
				}
			}
		}
	});

	fxed::Mouse mouse(window);
	fxed::Mouse::addScrollCallback([&](GLFWwindow *, double, double yOffset) {
		if (fxed::Keyboard::getKey(GLFW_KEY_LEFT_CONTROL) || fxed::Keyboard::getKey(GLFW_KEY_RIGHT_CONTROL)) {
			auto fontSize = textRenderer.getFontSize();
			fontSize += std::copysign(1.f, (float)yOffset) * 1.0f;
			fontSize = std::max(2.f, fontSize);
			textRenderer.setFontSize(fontSize);
		} else {
			pane->scroll(0, std::copysign(1.f, yOffset) * 1.0f);
		}
	});

	while (!window.shouldClose()) {
		window.beginFrame();
		bool res = win->beginFrame();
		if (!res) {
			window.swapBuffers();
			continue;
		}

		auto &cmdBuf = win->getCurrentCommandBuffer();
		win->beginRendering(cmdBuf, win->getCurrentRenderTarget());

		splitPane.render(cmdBuf);

		win->endRendering(cmdBuf);
		win->endFrame();

		window.swapBuffers();
	}

	nri->synchronize();

	return 0;
}
