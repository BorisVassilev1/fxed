#include <iostream>

#include <fstream>

#include "editor.hpp"
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
#include "any_range.hpp"

int main(int argc, char *argv[]) {
	// 🚀 asd
	// 🐊
	// 🐊
	// 🐊
	auto nri = nri::Factory::getInstance().createNRI("Vulkan", nri::CreateBits::GLFW);
	//
	// 🔔 asd

	// const std::vector<std::string> lines = {"Hello, World!", "This is a test.", "😀"};
	// auto text = lines | fxed::join_with('\n');
	// std::ranges::copy(text, std::ostream_iterator<char>(std::cout));

	fxed::Editor editor(*nri, 800, 600);

	// glfwSetWindowAttrib(window.getHandle(), GLFW_DECORATED, !glfwGetWindowAttrib(window.getHandle(),
	// GLFW_DECORATED));

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

	if (argc > 1) { editor.openFile(argv[1]); }

	editor.mainLoop();

	nri->synchronize();

	return 0;
}
