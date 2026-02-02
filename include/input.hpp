#pragma once
#include <functional>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "glfw_window.hpp"

namespace fxed {
// mouse will have different coordinates relative to different windows
// so it cannot be static.
class Mouse {
	glm::dvec2 position, delta;
	Window	  &window;

	Mouse();

   public:
	bool visible						 = false;	  ///< is mouse visible
	bool lock							 = false;	  ///< is mouse locked
	bool disableMouseWhenLockedAndHidden = false;	  ///< it says it

	/**
	 * @brief Constructs a Mouse for \a window
	 *
	 * @param window - window to attach the mouse to
	 * @param lock - should mouse be locked in the center of the window
	 */
	Mouse(Window &window, bool lock);
	/**
	 * @brief Constructs a Mouse for \a window
	 *
	 * @param window - window to attach the mouse to
	 */
	Mouse(Window &window);
	/**
	 * @brief Updates the mouse. Should be called every frame!
	 */
	void update();
	/**
	 * @brief Hides the mouse
	 */
	void hide();
	/**
	 * @brief Shows the mouse
	 */
	void show();

	inline glm::dvec2 getPosition() { return position; }
	inline glm::dvec2 getDelta() { return delta; }
};

// keyboard will always have the same behaviour with all windows
// so it can be entirely static
/**
 * @brief static Keyboard class
 */
class Keyboard {
	inline static Window *window = nullptr;

	inline static std::vector<std::function<void(GLFWwindow *, int, int, int, int)>> callbacks;

	static void handleInput(GLFWwindow *, int, int, int, int);
	Keyboard();

   public:
	/**
	 * @brief initialize Keyboard for \a window. Can be called several times to capture input from multiple windows.
	 *
	 * @param window - a Window to attach to
	 */
	static void init(Window *window);
	/**
	 * @brief Get the state of a key
	 *
	 * @param window - a Window to check for keypress
	 * @param key - key to get the state of
	 * @return int - state of \a key
	 */
	static int getKey(Window &window, int key);
	/**
	 * @brief Get the state of a key. Same as getKey(Window &window, int key) , but for the last window that
	 * init(Window *window) has been called for.
	 *
	 * @param key - key to get the state of
	 * @return int - stateof \a key
	 */
	static int getKey(int key);
	/**
	 * @brief Adds a callback to be called on change of state of any key.
	 *
	 * @param callback - callback that will be called on every change of key state on every window.
	 */
	static void addKeyCallback(const std::function<void(GLFWwindow *, int, int, int, int)> &callback);
};

}	  // namespace fxed
