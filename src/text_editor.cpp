#include "text_editor.hpp"

using namespace fxed;

bool InsertCharAction::forward(TextStateBase &editor) {
	editor.setCursor(positionBefore);
	editor.insertChar(c);
	positionAfter = editor.getCursorPos();
	return true;
}

void InsertCharAction::backward(TextStateBase &editor) {
	editor.setCursor(positionAfter);
	editor.deleteChar();
	assert(editor.getCursorPos() == positionBefore);
}

void InsertCharAction::print(std::ostream &os) const {
	os << "Insert '" << (char)c << "' at (" << positionBefore.x << ", " << positionBefore.y << ")";
}

bool DeleteCharAction::forward(TextStateBase &editor) {
	editor.setCursor(positionBefore);
	c			  = editor.deleteChar();
	positionAfter = editor.getCursorPos();
	return c != '\0';
}

void DeleteCharAction::backward(TextStateBase &editor) {
	editor.setCursor(positionAfter);
	editor.insertChar(c);
	assert(editor.getCursorPos() == positionBefore);
}

void DeleteCharAction::print(std::ostream &os) const {
	os << "Delete '" << (char)c << "' at (" << positionBefore.x << ", " << positionBefore.y << ")";
}

std::ostream &operator<<(std::ostream &os, const Action &action) {
	action.print(os);
	return os;
}

int32_t TextState::measureLineOffset(int line, int charOffset) const {
	if (charOffset == -1) return -1;
	int32_t offset = 0;
	for (int i = 0; i < charOffset; i++) {
		offset += lines[line][i] == '\t' ? 4 : 1;
	}
	return offset;
}

TextState::TextState(std::u32string_view text) {
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
	textChanged	 = true;
	lastMoveTime = std::chrono::high_resolution_clock::now();
	cursorMoved	 = true;
}

void TextState::insertChar(char32_t c) {
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
	currentMax	 = measureLineOffset(cursorPos.y, cursorPos.x);
	cursorMoved	 = true;
	lastMoveTime = std::chrono::high_resolution_clock::now();
	textChanged	 = true;
}

char32_t TextState::deleteChar() {
	char32_t deletedChar = '\0';
	if (cursorPos.x > 0) {
		deletedChar = lines[cursorPos.y][cursorPos.x - 1];
		lines[cursorPos.y].erase(lines[cursorPos.y].begin() + cursorPos.x - 1);
		cursorPos.x--;
	} else if (cursorPos.y > 0) {
		deletedChar = '\n';
		cursorPos.x = lines[cursorPos.y - 1].size();
		lines[cursorPos.y - 1] += lines[cursorPos.y];
		lines.erase(lines.begin() + cursorPos.y);
		cursorPos.y--;
	}
	currentMax	 = measureLineOffset(cursorPos.y, cursorPos.x);
	cursorMoved	 = true;
	lastMoveTime = std::chrono::high_resolution_clock::now();
	textChanged	 = true;
	return deletedChar;
}

void TextState::moveCursor(int dx, int dy) {
	int32_t currentLineOffset = measureLineOffset(cursorPos.y, cursorPos.x);
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
	cursorPos.x	 = std::clamp(newCursorX, 0, (int32_t)lines[cursorPos.y].size());
	cursorMoved	 = true;
	lastMoveTime = std::chrono::high_resolution_clock::now();
}

void TextState::setCursor(glm::ivec2 pos) {
	cursorPos.y	 = std::clamp(pos.y, 0, (int32_t)lines.size() - 1);
	cursorPos.x	 = std::clamp(pos.x, 0, (int32_t)lines[cursorPos.y].size());
	currentMax	 = measureLineOffset(cursorPos.y, cursorPos.x);
	lastMoveTime = std::chrono::high_resolution_clock::now();
	cursorMoved	 = true;
}

char32_t TextState::getCharAt(glm::ivec2 pos) const {
	if (pos.y < 0 || pos.y >= (int32_t)lines.size() || pos.x < 0 || pos.x >= (int32_t)lines[pos.y].size()) {
		return U'\0';
	}
	return lines[pos.y][pos.x];
}

std::u32string TextState::getText() const {
	std::u32string result;
	for (const auto &line : lines) {
		result += line + U'\n';
	}
	return result;
}

glm::ivec2 TextState::getCursorPos() const { return cursorPos; }

bool TextState::hasCursorMoved() const { return cursorMoved; }
bool TextState::hasTextChanged() const { return textChanged; }
void TextState::resetCursorMoved() { cursorMoved = false; }
void TextState::resetTextChanged() { textChanged = false; }

size_t TextState::milisecondsSinceLastMove() const {
	auto now = std::chrono::high_resolution_clock::now();
	return std::chrono::duration_cast<std::chrono::milliseconds>(now - lastMoveTime).count();
}
