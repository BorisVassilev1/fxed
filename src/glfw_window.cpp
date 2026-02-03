#include "glfw_window.hpp"

#include <iostream>
#include <iomanip>
#include <assert.h>

#include "input.hpp"
#include "nri.hpp"
#include "utils.hpp"

using namespace fxed;

Window::Window(nri::NRI &nri, int width, int height, const char *name, bool vsync, bool resizable, GLFWmonitor *monitor)
	: width(width), height(height) {
	assert(glfwGetPrimaryMonitor() != NULL);

	const GLFWvidmode *mode = glfwGetVideoMode(monitor ? monitor : glfwGetPrimaryMonitor());

	glfwWindowHint(GLFW_RED_BITS, mode->redBits);
	glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
	glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
	glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

	glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GL_TRUE);
	glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

	if (resizable) glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	else glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(width, height, name, monitor, NULL);
	if (!window) {
		std::cerr << "glfwCreateWindow failed." << std::endl;
		glfwTerminate();
		exit(1);
	}

	Keyboard::init(this);

	glfwSetWindowCloseCallback(window, [](GLFWwindow *) {});
	Keyboard::addKeyCallback([&](GLFWwindow *window, int key, int, int action, int) {
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) { close(); }
	});
	glfwSetWindowSizeCallback(window, handleResize);
	addResizeCallback([this](GLFWwindow *window, int width, int height) {
		if (window != getHandle()) return;
		this->width	 = width;
		this->height = height;
	});

	// glfwMakeContextCurrent(window);

	glfwSwapInterval(vsync);

	nriWindow = nri.createGLFWWindow(window);
}

Window::Window(nri::NRI &nri, int width, int height, const char *name, bool vsync, bool resizable)
	: Window(nri, width, height, name, vsync, resizable, NULL) {}

Window::Window(nri::NRI &nri, int width, int height, const char *name, bool vsync)
	: Window(nri, width, height, name, vsync, true) {}

Window::Window(nri::NRI &nri, int width, int height, const char *name) : Window(nri, width, height, name, true) {}

int Window::getWidth() { return width; }
int Window::getHeight() { return height; }

glm::ivec2 Window::getPos() {
	int x, y;
	glfwGetWindowPos(window, &x, &y);
	return glm::ivec2(x, y);
}

GLFWwindow *Window::getHandle() { return window; }

bool Window::shouldClose() { return glfwWindowShouldClose(window); }

void Window::setShouldClose(bool b) { glfwSetWindowShouldClose(window, b); }

void Window::setEnableViewports(bool b) { this->enableViewports = b; }

void Window::swapBuffers() {
	nriWindow->endFrame();

	auto	  timeNow = std::chrono::high_resolution_clock::now();
	long long delta	  = std::chrono::duration_cast<std::chrono::nanoseconds>(timeNow - lastSwapTime).count();

	glfwSwapBuffers(window);

	timeNow				= std::chrono::high_resolution_clock::now();
	long long fullDelta = std::chrono::duration_cast<std::chrono::nanoseconds>(timeNow - lastSwapTime).count();
	deltaTime			= fullDelta / 1e9;
	globalTime += deltaTime;

	double logDelta = globalTime - lastPrintTime;
	++frames_last_interval;

	if (logDelta > 1.) {
		frameCallback(delta, fullDelta, frames_last_interval);
		lastPrintTime		 = globalTime;
		fps					 = frames_last_interval;
		frames_last_interval = 0;
	}

	lastSwapTime = timeNow;
}

void Window::beginFrame() {
	//glfwWaitEvents();
	glfwPollEvents();

	nriWindow->beginFrame();
}

void Window::close() { glfwSetWindowShouldClose(window, GLFW_TRUE); }

Window::~Window() {
	if (window != nullptr) {
		glfwDestroyWindow(window);
		window = nullptr;
	}
}

bool Window::isFocused() { return glfwGetWindowAttrib(window, GLFW_FOCUSED) == GLFW_TRUE; }

void Window::handleResize(GLFWwindow *window, int width, int height) {
	for (auto callback : resizeCallbacks) {
		callback(window, width, height);
	}
}

void Window::addResizeCallback(const std::function<void(GLFWwindow *, int, int)> &callback) {
	resizeCallbacks.push_back(callback);
}

void Window::defaultFrameCallback(long draw_time, long frame_time, long frames) {
	//std::cout << std::fixed << std::setprecision(2) << "\rdraw time: " << (draw_time / 1e6) << "ms, FPS: " << frames
	//		  << "         " << std::flush;		//<< std::endl;
}

void Window::setFrameCallback(void (*callback)(long, long, long)) { frameCallback = callback; }
