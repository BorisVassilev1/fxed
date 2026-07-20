#pragma once

#include <string>
#include <vector>
#include "glm/glm.hpp"
#include "utils.hpp"

typedef struct TSParser TSParser;
typedef struct TSQuery  TSQuery;

namespace fxed {

/// Parses C++ source with tree-sitter and produces a syntax-highlighting color per UTF-32 codepoint.
class CppHighlighter {
	TSParser *parser = nullptr;
	TSQuery	 *query	 = nullptr;

   public:
	CppHighlighter();
	~CppHighlighter();
	DELETE_COPY_AND_ASSIGNMENT(CppHighlighter);
	CppHighlighter(CppHighlighter &&other) noexcept;
	CppHighlighter &operator=(CppHighlighter &&other) noexcept;

	/// returns one color per UTF-32 codepoint in `text`
	std::vector<glm::vec4> highlight(const std::u32string &text);
};

}	  // namespace fxed
