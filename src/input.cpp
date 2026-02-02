#define _USE_MATH_DEFINES

#include "input.hpp"
#include <glm/gtc/type_ptr.hpp>

using namespace fxed;

Mouse::Mouse(Window &window, bool lock) : delta(0), window(window), lock(lock) {
	position.x = window.getWidth() / 2.;
	position.y = window.getHeight() / 2.;
	glfwSetCursorPos(window.getHandle(), position.x, position.y);
	update();
}

Mouse::Mouse(Window &window) : Mouse(window, false) {}

void Mouse::update() {
	glm::dvec2 newPos;

	glfwGetCursorPos(window.getHandle(), &newPos.x, &newPos.y);

	delta = newPos - position;

	position = newPos;

	if (lock && !disableMouseWhenLockedAndHidden) {
		position.x = window.getWidth() / 2;
		position.y = window.getHeight() / 2;
		glfwSetCursorPos(window.getHandle(), position.x, position.y);
	}
}

void Mouse::hide() {
	if (lock && disableMouseWhenLockedAndHidden) {
		glfwSetInputMode(window.getHandle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	} else {
#ifndef __EMSCRIPTEN__
		glfwSetInputMode(window.getHandle(), GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
#else
		glfwSetInputMode(window.getHandle(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
#endif
	}
	visible = false;
}

void Mouse::show() {
	glfwSetInputMode(window.getHandle(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	visible = true;
}

void Keyboard::init(Window *window) {
	Keyboard::window = window;
	glfwSetKeyCallback(window->getHandle(), handleInput);
}

void Keyboard::handleInput(GLFWwindow *window, int key, int scancode, int action, int mods) {
	for (auto fun : callbacks) {
		fun(window, key, scancode, action, mods);
	}
}

int Keyboard::getKey(Window &window, int key) { return glfwGetKey(window.getHandle(), key); }

int Keyboard::getKey(int key) { return glfwGetKey(window->getHandle(), key); }

void Keyboard::addKeyCallback(
	const std::function<void(GLFWwindow *window, int key, int scancode, int action, int mods)> &callback) {
	callbacks.push_back(callback);
}

