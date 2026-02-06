#pragma once

#include "font.hpp"
#include "input.hpp"

class TextEditorBase {
public:
	virtual void insertChar(char c) = 0;
	virtual void deleteChar()			 = 0;
	virtual void moveCursor(int dx, int dy) = 0;
	virtual std::string getText() const	 = 0;
};

class TextEditorController {
	TextEditorBase &editor;
	std::chrono::high_resolution_clock::time_point lastMoveTime = std::chrono::high_resolution_clock::now();

public:
	TextEditorController(TextEditorBase &editor) : editor(editor) {
		fxed::Keyboard::addCharCallback([this, &editor](GLFWwindow *window, unsigned int codepoint) {
			editor.insertChar((char)codepoint);
			lastMoveTime = std::chrono::high_resolution_clock::now();
		});
		fxed::Keyboard::addKeyCallback([this, &editor](GLFWwindow *window, int key, int scancode, int action, int mods) {
			if (action == GLFW_PRESS || action == GLFW_REPEAT) {
				if (key == GLFW_KEY_BACKSPACE) {
					editor.deleteChar();
				} else if (key == GLFW_KEY_ENTER) {
					editor.insertChar('\n');
				} else if (key == GLFW_KEY_TAB) {
					editor.insertChar('\t');
				} else if (key == GLFW_KEY_LEFT) {
					editor.moveCursor(-1, 0);
				} else if (key == GLFW_KEY_RIGHT) {
					editor.moveCursor(1, 0);
				} else if (key == GLFW_KEY_UP) {
					editor.moveCursor(0, -1);
				} else if (key == GLFW_KEY_DOWN) {
					editor.moveCursor(0, 1);
				}
				lastMoveTime = std::chrono::high_resolution_clock::now();
			}
		});
	}

	size_t milisecondsSinceLastMove() const {
		auto now = std::chrono::high_resolution_clock::now();
		return std::chrono::duration_cast<std::chrono::milliseconds>(now - lastMoveTime).count();
	}
};

class TextEditor : public TextEditorBase {
	glm::ivec2 cursorPos{0, 0};
	std::vector<std::string> lines;

   public:

	TextEditor() { lines.emplace_back(); }
	TextEditor(std::string_view text) {
		size_t pos = 0;
		while (pos < text.size()) {
			size_t nextPos = text.find('\n', pos);
			if (nextPos == std::string_view::npos) {
				lines.emplace_back(text.substr(pos));
				break;
			} else {
				lines.emplace_back(text.substr(pos, nextPos - pos));
				pos = nextPos + 1;
			}
		}
		if(lines.empty()) {
			lines.emplace_back();
		}
	}

	void insertChar(char c) override {
		if (c == '\n') {
			std::string newLine = lines[cursorPos.y].substr(cursorPos.x);
			lines[cursorPos.y].resize(cursorPos.x);
			lines.insert(lines.begin() + cursorPos.y + 1, std::move(newLine));
			cursorPos.y++;
			cursorPos.x = 0;
		} else {
			lines[cursorPos.y].insert(lines[cursorPos.y].begin() + cursorPos.x, c);
			cursorPos.x++;
		}
	}

	void deleteChar() override {
		if (cursorPos.x > 0) {
			lines[cursorPos.y].erase(lines[cursorPos.y].begin() + cursorPos.x - 1);
			cursorPos.x--;
		} else if (cursorPos.y > 0) {
			cursorPos.x = lines[cursorPos.y - 1].size();
			lines[cursorPos.y - 1] += lines[cursorPos.y];
			lines.erase(lines.begin() + cursorPos.y);
			cursorPos.y--;
		}
	}

	void moveCursor(int dx, int dy) override {
		cursorPos.y = std::clamp(cursorPos.y + dy, 0, (int32_t)lines.size() - 1);
		cursorPos.x = std::clamp(cursorPos.x + dx, 0, (int32_t)lines[cursorPos.y].size());
	}

	std::string getText() const override {
		std::string result;
		for (const auto &line : lines) {
			result += line + '\n';
		}
		return result;
	}

	glm::ivec2 getCursorPos() const { return cursorPos; }
};
