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

	fxed::Editor editor(*nri, 800, 600);

	// glfwSetWindowAttrib(window.getHandle(), GLFW_DECORATED, !glfwGetWindowAttrib(window.getHandle(),
	// GLFW_DECORATED));

	// load argv[1] if exists

	if (argc > 1) {
		auto path = std::filesystem::path(argv[1]);
		if (!std::filesystem::exists(path)) {
			dbLog(dbg::LOG_ERROR, "File or directory does not exist: ", path);
			return 1;
		}
		if (std::filesystem::is_directory(path)) {
			dbLog(dbg::LOG_INFO, "Opening folder: ", path);
			editor.setFolder(path);
		} else {
			dbLog(dbg::LOG_INFO, "Opening file: ", path);
			editor.openFile(path);
		}
	}

	editor.mainLoop();

	nri->synchronize();

	return 0;
}
