#pragma once

#include <string_view>
#include "font.hpp"
#include "input.hpp"

class TextEditorBase {
   public:
	virtual void		   insertChar(char32_t c)	  = 0;
	virtual void		   deleteChar()				  = 0;
	virtual void		   moveCursor(int dx, int dy) = 0;
	virtual std::u32string getText() const			  = 0;
};

class TextEditorController {
	TextEditorBase								  &editor;
	std::chrono::high_resolution_clock::time_point lastMoveTime = std::chrono::high_resolution_clock::now();
	bool										   cursorMoved	= false;
	bool										   textChanged	= true;

   public:
	TextEditorController(TextEditorBase &editor) : editor(editor) {
		fxed::Keyboard::addCharCallback([this, &editor](GLFWwindow *window, unsigned int codepoint) {
			editor.insertChar((char32_t)codepoint);
			lastMoveTime = std::chrono::high_resolution_clock::now();
			cursorMoved	 = true;
			textChanged	 = true;
		});
		fxed::Keyboard::addKeyCallback(
			[this, &editor](GLFWwindow *window, int key, int scancode, int action, int mods) {
				if (action == GLFW_PRESS || action == GLFW_REPEAT) {
					if (key == GLFW_KEY_BACKSPACE) {
						editor.deleteChar();
						textChanged = true;
					} else if (key == GLFW_KEY_ENTER) {
						editor.insertChar('\n');
						textChanged = true;
					} else if (key == GLFW_KEY_TAB) {
						editor.insertChar('\t');
						textChanged = true;
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
					cursorMoved	 = true;
				}
			});
	}

	size_t milisecondsSinceLastMove() const {
		auto now = std::chrono::high_resolution_clock::now();
		return std::chrono::duration_cast<std::chrono::milliseconds>(now - lastMoveTime).count();
	}

	bool hasCursorMoved() const { return cursorMoved; }
	bool hasTextChanged() const { return textChanged; }
	void resetCursorMoved() { cursorMoved = false; }
	void resetTextChanged() { textChanged = false; }
};

class TextEditor : public TextEditorBase {
	glm::ivec2					cursorPos{0, 0};
	std::vector<std::u32string> lines;
	int							currentMax = 0;

	int32_t measureLineOffset(int line, int charOffset) const {
		if (charOffset == -1) return -1;
		int32_t offset = 0;
		for (int i = 0; i < charOffset; i++) {
			offset += lines[line][i] == '\t' ? 4 : 1;
		}
		return offset;
	}

   public:
	TextEditor() { lines.emplace_back(); }
	TextEditor(std::u32string_view text) {
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
		if (lines.empty()) { lines.emplace_back(); }
	}

	void insertChar(char32_t c) override {
		if (c == '\n') {
			std::u32string newLine = lines[cursorPos.y].substr(cursorPos.x);
			lines[cursorPos.y].resize(cursorPos.x);
			lines.insert(lines.begin() + cursorPos.y + 1, std::move(newLine));
			cursorPos.y++;
			cursorPos.x = 0;
		} else {
			lines[cursorPos.y].insert(lines[cursorPos.y].begin() + cursorPos.x, c);
			cursorPos.x++;
		}
		currentMax = measureLineOffset(cursorPos.y, cursorPos.x);
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
		currentMax = measureLineOffset(cursorPos.y, cursorPos.x);
	}

	void moveCursor(int dx, int dy) override {
		int32_t currentLineOffset = 0;
		for (int i = 0; i < cursorPos.x; i++) {
			currentLineOffset += lines[cursorPos.y][i] == '\t' ? 4 : 1;
		}
		cursorPos.y				 = std::clamp(cursorPos.y + dy, 0, (int32_t)lines.size() - 1);
		int32_t targetLineOffset = dx != 0 ? currentLineOffset + dx : currentMax;
		if (dx != 0) { currentMax = targetLineOffset; }
		int32_t newCursorX = 0;
		while (newCursorX < (int32_t)lines[cursorPos.y].size() && targetLineOffset > 0) {
			targetLineOffset -= lines[cursorPos.y][newCursorX] == '\t' ? 4 : 1;
			newCursorX++;
		}
		if (targetLineOffset < 0 && dx < 0) {
			newCursorX--;
			currentMax = measureLineOffset(cursorPos.y, newCursorX);
		}
		cursorPos.x = std::clamp(newCursorX, 0, (int32_t)lines[cursorPos.y].size());
	}

	std::u32string getText() const override {
		std::u32string result;
		for (const auto &line : lines) {
			result += line + U'\n';
		}
		return result;
	}

	glm::ivec2 getCursorPos() const { return cursorPos; }

	void setCursorPos(glm::ivec2 pos) {
		cursorPos.y = std::clamp(pos.y, 0, (int32_t)lines.size() - 1);
		cursorPos.x = std::clamp(pos.x, 0, (int32_t)lines[cursorPos.y].size());
	}
};
