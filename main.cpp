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

int main(int argc, char *argv[]) {
	// 🚀 asd
	// 🐊
	// 🐊
	// 🐊
	auto nri = nri::Factory::getInstance().createNRI("Vulkan", nri::CreateBits::GLFW);
	//
	// 🔔 asd
	fxed::Window window(*nri, 800, 600, "Vulkan NRI Window");

	glfwSetWindowAttrib(window.getHandle(), GLFW_DECORATED, !glfwGetWindowAttrib(window.getHandle(), GLFW_DECORATED));
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
			dbLog(dbg::LOG_ERROR, "Failed to open file: %s", argv[1]);
		}
	}

	// std::ranges::copy(text | fxed::to_utf8, std::ostream_iterator<char>(std::cout));

	auto				 textMesh = fxed::TextMesh(*nri, window.getMainQueue(), 10000);
	TextEditor			 textEditor(text);
	TextEditorController textEditorController(textEditor);

	auto cursorMesh = std::make_unique<fxed::QuadMesh>(*nri, window.getMainQueue(), glm::vec2(0.1, 1.0f));

	auto resizeCallback = [&](GLFWwindow *, int w, int h) { textRenderer.viewportSize = {w, h}; };
	resizeCallback(nullptr, window.getWidth(), window.getHeight());

	window.addResizeCallback(resizeCallback);

	fxed::Keyboard::addKeyCallback([&](GLFWwindow *, int key, int, int action, int mods) {
		// zoom with ctrl + +/-
		if (action == GLFW_PRESS || action == GLFW_REPEAT) {
			if (mods & GLFW_MOD_CONTROL) {
				if (key == GLFW_KEY_KP_ADD || key == GLFW_KEY_EQUAL) {
					auto fontSize = textRenderer.getFontSize();
					textRenderer.setFontSize((uint32_t)fontSize + 2);
				} else if (key == GLFW_KEY_KP_SUBTRACT || key == GLFW_KEY_MINUS) {
					auto fontSize = textRenderer.getFontSize();
					textRenderer.setFontSize(std::max(2U, (uint32_t)fontSize - 2));
				}
			}
		}
	});
	fxed::Mouse mouse(window);
	fxed::Mouse::addScrollCallback([&](GLFWwindow *, double, double yOffset) {
		if (fxed::Keyboard::getKey(GLFW_KEY_LEFT_CONTROL) || fxed::Keyboard::getKey(GLFW_KEY_RIGHT_CONTROL)) {
			auto fontSize = textRenderer.getFontSize();
			textRenderer.setFontSize((uint32_t)std::max(2.f, fontSize + std::copysign(1.f, (float)yOffset) * 2.0f));
		} else {
			textRenderer.translation.y += std::copysign(1.f, yOffset) * 2.0f;
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

		std::u32string text = textEditor.getText();
		glm::vec2	   cursorRealPos;
		textRenderer.getFont().syncWithGPU();
		cursorRealPos = textMesh.updateText(std::span<const char32_t>{text.begin(), text.end()}, textRenderer.getFont(),
											textEditor.getCursorPos(), window.getWidth());

		if (textEditorController.hasCursorMoved()) {
			glm::vec2 screenBounds = glm::vec2(textRenderer.viewportSize) / textRenderer.getFontSize();
			// clamp translation to prevent moving text out of screen
			textRenderer.translation.x =
				std::clamp<float>(textRenderer.translation.x, -cursorRealPos.x, screenBounds.x - cursorRealPos.x - 1);
			textRenderer.translation.y = std::clamp<float>(textRenderer.translation.y, -cursorRealPos.y + 1,
														   screenBounds.y - cursorRealPos.y - 1);
		}
		auto time				= glfwGetTime();
		textRenderer.showCursor = textEditorController.milisecondsSinceLastMove() < 200 || (time - int64_t(time)) < 0.5;

		textRenderer.renderText(cmdBuf, textMesh, cursorRealPos);

		win->endRendering(cmdBuf);
		win->endFrame();

		textEditorController.resetCursorMoved();
		window.swapBuffers();
	}

	nri->synchronize();

	return 0;
}
