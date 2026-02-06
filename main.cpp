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

struct PushConstants {
	glm::ivec2			viewportSize;
	glm::ivec2			translation = {0, 0};
	nri::ResourceHandle textureHandle;
	float				textSize = 24.0f;
};

struct PushConstantsCursor {
	glm::ivec2 viewportSize;
	glm::ivec2 translation = {0, 0};
	glm::vec2 cursorPos;
	float	   textSize;
	float time;
};

int main(int argc, char *argv[]) {
	auto nri = nri::Factory::getInstance().createNRI("Vulkan", nri::CreateBits::GLFW);

	fxed::Window window(*nri, 800, 600, "Vulkan NRI Window");

	// glfwSetWindowAttrib(window.getHandle(), GLFW_DECORATED, !glfwGetWindowAttrib(window.getHandle(),
	// GLFW_DECORATED));
	auto &win		= window.getNativeWindow();
	win->clearColor = glm::vec4(30 / 255.f, 30 / 255.f, 46 / 255.f, 1.0f);

	std::string fontPath = fxed::Font::getDefaultSystemFontPath();
	dbLog(dbg::LOG_INFO, "Using font: %s", fontPath.c_str());

	auto font = fxed::Font(*nri, window.getMainQueue(), fontPath.data(), 512);

	auto textureHandle = font.getHandle();

	auto sb		= nri->createProgramBuilder();
	auto shader = sb->addShaderModule(nri::ShaderCreateInfo{"shaders/text.hlsl", "VSMain", nri::SHADER_TYPE_VERTEX})
					  .addShaderModule(nri::ShaderCreateInfo{"shaders/text.hlsl", "PSMain", nri::SHADER_TYPE_FRAGMENT})
					  .setVertexBindings(fxed::TextMesh::getVertexBindings())
					  .setPrimitiveType(nri::PRIMITIVE_TYPE_TRIANGLES)
					  .setPushConstantRanges({{0, sizeof(PushConstants)}})
					  .buildGraphicsProgram();

	sb->clearShaderModules();
	auto cursorShader =
		sb->addShaderModule(nri::ShaderCreateInfo{"shaders/cursor.hlsl", "VSMain", nri::SHADER_TYPE_VERTEX})
			.addShaderModule(nri::ShaderCreateInfo{"shaders/cursor.hlsl", "PSMain", nri::SHADER_TYPE_FRAGMENT})
			.setPushConstantRanges({{0, sizeof(PushConstantsCursor)}})
			.buildGraphicsProgram();

	// load argv[1] if exists
	std::string text;
	if (argc > 1) {
		std::ifstream file(argv[1]);
		if (file.is_open()) {
			text.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
			file.close();
		} else {
			dbLog(dbg::LOG_ERROR, "Failed to open file: %s", argv[1]);
		}
	}

	auto				 textMesh = fxed::TextMesh(*nri, window.getMainQueue(), 10000);
	TextEditor			 textEditor(text);
	TextEditorController textEditorController(textEditor);

	auto cursorMesh = std::make_unique<fxed::QuadMesh>(*nri, window.getMainQueue(), glm::vec2(0.1, 1.0f));

	PushConstants pushConstants{
		.viewportSize  = {window.getWidth(), window.getHeight()},
		.translation   = {0, 0},
		.textureHandle = textureHandle,
		.textSize	   = 24.0f,
	};

	auto resizeCallback = [&](GLFWwindow *, int w, int h) { pushConstants.viewportSize = {w, h}; };
	resizeCallback(nullptr, window.getWidth(), window.getHeight());

	window.addResizeCallback(resizeCallback);

	fxed::Keyboard::addKeyCallback([&](GLFWwindow *window, int key, int, int action, int mods) {
		// zoom with ctrl + +/-
		if (action == GLFW_PRESS || action == GLFW_REPEAT) {
			if (mods & GLFW_MOD_CONTROL) {
				if (key == GLFW_KEY_KP_ADD || key == GLFW_KEY_EQUAL) {
					pushConstants.textSize += 1;
				} else if (key == GLFW_KEY_KP_SUBTRACT || key == GLFW_KEY_MINUS) {
					pushConstants.textSize -= 1;
				}
			}
		}
	});
	fxed::Mouse mouse(window);
	fxed::Mouse::addScrollCallback([&](GLFWwindow *, double, double yOffset) {
		if (fxed::Keyboard::getKey(GLFW_KEY_LEFT_CONTROL) || fxed::Keyboard::getKey(GLFW_KEY_RIGHT_CONTROL)) {
			pushConstants.textSize += (float)yOffset;
		} else {
			pushConstants.translation.y += std::copysign(1.f, yOffset) * 2.0f;
		}
	});

	while (!window.shouldClose()) {
		window.beginFrame();

		win->beginFrame();
		auto &cmdBuf = win->getCurrentCommandBuffer();

		shader->bind(cmdBuf);
		win->beginRendering(cmdBuf, win->getCurrentRenderTarget());

		shader->setPushConstants(cmdBuf, &pushConstants, sizeof(pushConstants), 0);

		std::string text = textEditor.getText();
		glm::vec2 cursorRealPos;
		cursorRealPos = textMesh.updateText(std::span<const char>{text.begin(), text.end()}, font, textEditor.getCursorPos());

		glm::vec2 screenBounds = glm::vec2(pushConstants.viewportSize) / pushConstants.textSize;
		 // clamp translation to prevent moving text out of screen
		pushConstants.translation.x = std::clamp<float>(pushConstants.translation.x, -cursorRealPos.x + 1, screenBounds.x - cursorRealPos.x - 1);
		pushConstants.translation.y = std::clamp<float>(pushConstants.translation.y, -cursorRealPos.y + 1, screenBounds.y - cursorRealPos.y - 1);

		textMesh.bind(cmdBuf);
		textMesh.draw(cmdBuf, *shader);

		PushConstantsCursor cursorPushConstants{
			.viewportSize = {window.getWidth(), window.getHeight()},
			.translation  = pushConstants.translation,
			.cursorPos	   = cursorRealPos,
			.textSize	   = pushConstants.textSize,
			.time		   = (textEditorController.milisecondsSinceLastMove() < 200) ? 0 : (float)glfwGetTime()
		};
		cursorShader->bind(cmdBuf);
		cursorShader->setPushConstants(cmdBuf, &cursorPushConstants, sizeof(cursorPushConstants), 0);
		cursorMesh->bind(cmdBuf);
		cursorMesh->draw(cmdBuf, *cursorShader);

		win->endRendering(cmdBuf);
		win->endFrame();

		window.swapBuffers();
	}

	return 0;
}
