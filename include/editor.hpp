#pragma once

#include "glfw_window.hpp"
#include "input.hpp"
#include "nri.hpp"
#include "pane.hpp"
#include "text_rendering.hpp"

namespace fxed {

class Editor {
   private:
	nri::NRI &nri;
	Window window;
	Mouse mouse;
	TextRenderer textRenderer;

	std::unique_ptr<Pane> rootPane;

	void setupCallbacks();

   public:
	Editor(nri::NRI &nri, uint32_t width, uint32_t height);

	static Editor &getInstance();
	static Editor *instance;

	void mainLoop();

	void openFile(const std::filesystem::path &path);
	
};

};	   // namespace fxed
