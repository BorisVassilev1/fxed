#include <iostream>

#include "font.hpp"
#include "input.hpp"
#include "mesh.hpp"
#include "nri.hpp"
#include "nriFactory.hpp"
#include "glfw_window.hpp"

struct PushConstants {
	glm::vec2			translate;
	glm::vec2			scale;
	nri::ResourceHandle textureHandle;
	float textSize = 24.0f;
};

int main() {
	auto nri = nri::Factory::getInstance().createNRI("Vulkan", nri::CreateBits::GLFW);

	fxed::Window window(*nri, 800, 600, "Vulkan NRI Window");

	// glfwSetWindowAttrib(window.getHandle(), GLFW_DECORATED, !glfwGetWindowAttrib(window.getHandle(),
	// GLFW_DECORATED));
	auto &win		= window.getNativeWindow();
	win->clearColor = glm::vec4(30 / 255.f, 30 / 255.f, 46 / 255.f, 1.0f);

	// std::string_view fontPath = "/usr/share/fonts/TTF/HackNerdFontMono-Regular.ttf";
	//std::string_view fontPath = "/usr/local/share/fonts/f/FantasqueSansMNerdFont_Regular.ttf";
	// std::string_view fontPath = "/usr/share/fonts/gsfonts/StandardSymbolsPS.otf";
	std::string fontPath = Font::getDefaultSystemFontPath();

	auto font = Font(*nri, window.getMainQueue(), fontPath.data(), 512);

	auto textureHandle = font.getHandle();
	auto mesh		   = std::make_unique<fxed::QuadMesh>(*nri, window.getMainQueue(), 1.0f);

	auto sb		= nri->createProgramBuilder();
	auto shader = sb->addShaderModule(
						nri::ShaderCreateInfo{PROJECT_ROOT_DIR "/shaders/text.hlsl", "VSMain", nri::SHADER_TYPE_VERTEX})
					  .addShaderModule(nri::ShaderCreateInfo{PROJECT_ROOT_DIR "/shaders/text.hlsl", "PSMain",
															 nri::SHADER_TYPE_FRAGMENT})
					  .setVertexBindings(mesh->getVertexBindings())
					  .setPrimitiveType(nri::PRIMITIVE_TYPE_TRIANGLES)
					  .setPushConstantRanges({{0, sizeof(PushConstants)}})
					  .buildGraphicsProgram();

	std::vector<float>	  textVertices;
	std::vector<float>	  textTexCoords;
	std::vector<uint32_t> textIndices;
	std::string_view	  text	   = "The quick brown fox jumps over the lazy dog.\nNew line test.";
	size_t				  offset   = 0;
	double				  advanceY = 0.0;
	double				  advance  = 0.0;

	const double pixel = 1.0 / 24.0;	 // font size
	for (size_t i = 0; i < text.size(); i++) {
		if (text[i] == '\n') {
			advanceY += 1.0;
			advance = 0.0;
			continue;
		}

		auto box = font.getGlyphBox(text[i]);
		textVertices.push_back(advance + box.bounds.l - pixel);
		textVertices.push_back(advanceY - box.bounds.t + pixel);
		textVertices.push_back(advance + box.bounds.l - pixel);
		textVertices.push_back(advanceY - box.bounds.b + pixel);
		textVertices.push_back(advance + box.bounds.r - pixel);
		textVertices.push_back(advanceY - box.bounds.b + pixel);
		textVertices.push_back(advance + box.bounds.r - pixel);
		textVertices.push_back(advanceY - box.bounds.t + pixel);

		textIndices.push_back(offset + 0);
		textIndices.push_back(offset + 1);
		textIndices.push_back(offset + 2);
		textIndices.push_back(offset + 2);
		textIndices.push_back(offset + 3);
		textIndices.push_back(offset + 0);

		textTexCoords.push_back(box.rect.x + 1);
		textTexCoords.push_back(box.rect.y + box.rect.h - 1);
		textTexCoords.push_back(box.rect.x + 1);
		textTexCoords.push_back(box.rect.y + 1);
		textTexCoords.push_back(box.rect.x + box.rect.w - 1);
		textTexCoords.push_back(box.rect.y + 1);
		textTexCoords.push_back(box.rect.x + box.rect.w - 1);
		textTexCoords.push_back(box.rect.y + box.rect.h - 1);

		advance += box.advance;
		offset += 4;
	}
	for (auto &v : textTexCoords) {
		v /= 512.0f;	 // atlas size
	}

	auto textMesh = std::make_unique<fxed::Mesh>(*nri, window.getMainQueue(), std::span(textVertices),
												 std::span(textTexCoords), std::span(textIndices));

	PushConstants pushConstants{
		.translate	   = {-1.0f, 0.0f},
		.scale		   = {0.2f, 0.2f},
		.textureHandle = textureHandle,
	};

	auto resizeCallback = [&](GLFWwindow *, int w, int h) {
		float aspect		  = static_cast<float>(w) / static_cast<float>(h);
		pushConstants.scale.x = pushConstants.scale.y / aspect;
		pushConstants.textSize = h * pushConstants.scale.y;
	};
	resizeCallback(nullptr, window.getWidth(), window.getHeight());
	
	window.addResizeCallback(resizeCallback);

	fxed::Keyboard::addKeyCallback([&](GLFWwindow *window, int key, int, int action, int mods) {
		// zoom with ctrl + +/-
		if (action == GLFW_PRESS || action == GLFW_REPEAT) {
			if (mods & GLFW_MOD_CONTROL) {
				if (key == GLFW_KEY_KP_ADD || key == GLFW_KEY_EQUAL) {
					pushConstants.scale *= 1.1f;
				} else if (key == GLFW_KEY_KP_SUBTRACT || key == GLFW_KEY_MINUS) {
					pushConstants.scale *= 0.9f;
				}
			}
		}
	});

	while (!window.shouldClose()) {
		window.beginFrame();

		// move with wasd
		if (fxed::Keyboard::getKey(GLFW_KEY_A) == GLFW_PRESS) pushConstants.translate.x += 1.f * window.deltaTime;
		if (fxed::Keyboard::getKey(GLFW_KEY_D) == GLFW_PRESS) pushConstants.translate.x -= 1.f * window.deltaTime;
		if (fxed::Keyboard::getKey(GLFW_KEY_W) == GLFW_PRESS) pushConstants.translate.y += 1.f * window.deltaTime;
		if (fxed::Keyboard::getKey(GLFW_KEY_S) == GLFW_PRESS) pushConstants.translate.y -= 1.f * window.deltaTime;

		auto &cmdBuf = win->getCurrentCommandBuffer();

		shader->bind(cmdBuf);
		win->beginRendering(cmdBuf, win->getCurrentRenderTarget());

		shader->setPushConstants(cmdBuf, &pushConstants, sizeof(pushConstants), 0);

		textMesh->bind(cmdBuf);
		textMesh->draw(cmdBuf, *shader);

		win->endRendering(cmdBuf);

		window.swapBuffers();
	}

	return 0;
}
