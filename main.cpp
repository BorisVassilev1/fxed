#include <iostream>

#include "nriFactory.hpp"
#include "glfw_window.hpp"

int main() {
	auto nri = nri::Factory::getInstance().createNRI("Vulkan", nri::CreateBits::GLFW);

	fxed::Window window(*nri, 800, 600, "Vulkan NRI Window");

	glfwSetWindowAttrib(window.getHandle(), GLFW_DECORATED, !glfwGetWindowAttrib(window.getHandle(), GLFW_DECORATED));


	while(!window.shouldClose()) {
		window.beginFrame();

	


		window.swapBuffers();
	}

	return 0;
}
