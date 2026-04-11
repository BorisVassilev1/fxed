#pragma once

#include <ranges>
#include <stack>
#include <string_view>
#include "font.hpp"
#include "input.hpp"
#include "ranges_join_with.hpp"

class TextStateBase {
   public:
	virtual void		   insertChar(char32_t c)		   = 0;
	virtual char32_t	   deleteChar()					   = 0;
	virtual void		   moveCursor(int dx, int dy)	   = 0;
	virtual void		   setCursor(glm::ivec2 pos)	   = 0;
	virtual glm::ivec2	   getCursorPos() const			   = 0;
	virtual char32_t	   getCharAt(glm::ivec2 pos) const = 0;
	virtual std::u32string getText() const				   = 0;

	virtual bool hasCursorMoved() const = 0;
	virtual bool hasTextChanged() const = 0;
	virtual void resetCursorMoved()		= 0;
	virtual void resetTextChanged()		= 0;

	virtual size_t milisecondsSinceLastMove() const = 0;
	virtual ~TextStateBase()						= default;
};

class Action {
   public:
	virtual bool forward(TextStateBase &editor)	 = 0;
	virtual void backward(TextStateBase &editor) = 0;
	virtual void print(std::ostream &os) const	 = 0;
	virtual ~Action()							 = default;
};

std::ostream &operator<<(std::ostream &os, const Action &action);

class InsertCharAction : public Action {
	glm::ivec2 positionBefore;
	glm::ivec2 positionAfter;
	char32_t   c;

   public:
	InsertCharAction(glm::ivec2 position, char32_t c) : positionBefore(position), positionAfter(0), c(c) {}

	char32_t getInsertedChar() const { return c; }

	/// returns true of successful
	bool forward(TextStateBase &editor) override;
	void backward(TextStateBase &editor) override;
	void print(std::ostream &os) const override;
};

class DeleteCharAction : public Action {
	glm::ivec2 positionBefore;
	glm::ivec2 positionAfter;
	char32_t   c;

   public:
	DeleteCharAction(glm::ivec2 position) : positionBefore(position), positionAfter(0), c(0) {}

	char32_t getDeletedChar() const { return c; }

	bool forward(TextStateBase &editor) override;
	void backward(TextStateBase &editor) override;
	void print(std::ostream &os) const override;
};

class TextState : public TextStateBase {
	glm::ivec2					cursorPos{0, 0};
	std::vector<std::u32string> lines;
	int							currentMax = 0;

	int32_t measureLineOffset(int line, int charOffset) const;

	bool										   cursorMoved	= false;
	bool										   textChanged	= true;
	std::chrono::high_resolution_clock::time_point lastMoveTime = std::chrono::high_resolution_clock::now();

   public:
	TextState() { lines.emplace_back(); }
	TextState(std::u32string_view text);
	void		   insertChar(char32_t c) override;
	char32_t	   deleteChar() override;
	void		   moveCursor(int dx, int dy) override;
	void		   setCursor(glm::ivec2 pos) override;
	char32_t	   getCharAt(glm::ivec2 pos) const override;
	std::u32string getText() const override;

	auto getTextRange() const { return lines | fxed::join_with(U'\n'); }

	glm::ivec2 getCursorPos() const override;

	bool hasCursorMoved() const override;
	bool hasTextChanged() const override;
	void resetCursorMoved() override;
	void resetTextChanged() override;

	size_t milisecondsSinceLastMove() const override;
};

template <class TextStateType>
class TextEditor : public TextStateBase {
	std::stack<std::unique_ptr<Action>> undoStack;
	std::stack<std::unique_ptr<Action>> redoStack;

	TextStateType textState;

   public:
	TextEditor() = default;
	TextEditor(auto &&textState) : textState(std::forward<decltype(textState)>(textState)) {}

	void insertChar(char32_t c) override {
		auto action = std::make_unique<InsertCharAction>(textState.getCursorPos(), c);
		if (!action->forward(textState)) return;
		undoStack.push(std::move(action));
		while (!redoStack.empty()) {
			redoStack.pop();
		}
	}

	char32_t deleteChar() override {
		auto action = std::make_unique<DeleteCharAction>(textState.getCursorPos());
		if (!action->forward(textState)) return U'\0';
		char32_t deletedChar = action->getDeletedChar();
		undoStack.push(std::move(action));
		while (!redoStack.empty()) {
			redoStack.pop();
		}
		return deletedChar;
	}

	void		   moveCursor(int dx, int dy) override { textState.moveCursor(dx, dy); }
	void		   setCursor(glm::ivec2 pos) override { textState.setCursor(pos); }
	glm::ivec2	   getCursorPos() const override { return textState.getCursorPos(); }
	char32_t	   getCharAt(glm::ivec2 pos) const override { return textState.getCharAt(pos); }
	std::u32string getText() const override { return textState.getText(); }
	auto		   getTextRange() const { return textState.getTextRange(); }

	void redo() {
		if (redoStack.empty()) return;
		auto action = std::move(redoStack.top());
		redoStack.pop();
		action->forward(textState);
		undoStack.push(std::move(action));
	}

	void undo() {
		if (undoStack.empty()) return;
		auto action = std::move(undoStack.top());
		undoStack.pop();
		action->backward(textState);
		redoStack.push(std::move(action));
	}

	bool hasCursorMoved() const override { return textState.hasCursorMoved(); }
	bool hasTextChanged() const override { return textState.hasTextChanged(); }
	void resetCursorMoved() override { textState.resetCursorMoved(); }
	void resetTextChanged() override { textState.resetTextChanged(); }

	size_t milisecondsSinceLastMove() const override { return textState.milisecondsSinceLastMove(); }
};

using DefaultTextEditor = TextEditor<TextState>;
