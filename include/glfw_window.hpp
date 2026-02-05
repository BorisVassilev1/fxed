#pragma once
#include <chrono>

#include "nri.hpp"
#include "vk_nri.hpp"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace fxed {
/**
 * @brief A Window Wrapper.
 */
class Window {
	GLFWwindow	*window	   = nullptr;
	std::unique_ptr<nri::Window> nriWindow = nullptr;

	int											   width = -1, height = -1;
	std::chrono::high_resolution_clock::time_point lastSwapTime			= std::chrono::high_resolution_clock::now();
	double										   lastPrintTime		= 0;
	long										   frames_last_interval = 0;
	void (*frameCallback)(long, long, long)								= &defaultFrameCallback;
	bool enableViewports												= false;

	inline static std::vector<std::function<void(GLFWwindow *, int, int)>> resizeCallbacks;

	Window() {};
	static void handleResize(GLFWwindow *, int, int);
	static void defaultFrameCallback(long, long, long);

   public:
	// TODO: this cannot override mesh settings, but should
	bool   shade	  = true;	  ///< does not work
	bool   cullFace	  = true;	  ///< does not work
	double deltaTime  = 0;		  ///< length of last frame
	double globalTime = 0;		  ///< time passed from opening the window until the beginning of the current frame
	long   fps;					  ///< frames that happened in the last second
	Window(nri::NRI &nri, int width, int height, const char *name, bool vsync, bool resizable, GLFWmonitor *monitor);
	Window(nri::NRI &nri, int width, int height, const char *name, bool vsync, bool resizable);
	Window(nri::NRI &nri, int width, int height, const char *name, bool vsync);
	Window(nri::NRI &nri, int width, int height, const char *name);

	~Window();
	Window(const Window &other)			   = delete;
	Window &operator=(const Window &other) = delete;

	int			getWidth();
	int			getHeight();
	glm::ivec2	getPos();
	GLFWwindow *getHandle();
	bool		shouldClose();
	void		close();
	void		setShouldClose(bool);
	void		setEnableViewports(bool);
	void		swapBuffers();
	void		beginFrame();
	/**
	 * @brief Set a callback to be called every second with data about the draw time of the last frame.
	 *
	 * @param callback - function to be called every frame
	 */
	void setFrameCallback(void (*callback)(long, long, long));
	/**
	 * @brief Destroys and permanently invalidates the window. The destructor calls this.
	 */
	void destroy();
	/**
	 * @brief Checks if the window has focus from the user
	 *
	 * @return true - if it has focus
	 * @return false - otherwise
	 */
	bool isFocused();
	/**
	 * @brief Adds a callback that will be called every time the window is resized.
	 *
	 * @param callback - the function to be called.
	 */
	void addResizeCallback(const std::function<void(GLFWwindow *window, int width, int height)> &callback);

	auto &getMainQueue() { return nriWindow->getMainQueue(); }
	auto &getNativeWindow() { return nriWindow; }
};

}	  // namespace fxed
