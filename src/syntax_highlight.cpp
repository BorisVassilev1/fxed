#include "syntax_highlight.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string_view>
#include <unordered_map>

extern "C" {
#include <tree_sitter/api.h>
}

extern "C" const TSLanguage *tree_sitter_cpp(void);

using namespace fxed;

namespace {

// catppuccin-mocha inspired palette, matching the editor's existing background colors
constexpr glm::vec4 COLOR_TEXT		 = {0.804f, 0.839f, 0.956f, 1.0f};	   // text
constexpr glm::vec4 COLOR_KEYWORD	 = {0.796f, 0.651f, 0.969f, 1.0f};	   // mauve
constexpr glm::vec4 COLOR_STRING	 = {0.651f, 0.890f, 0.631f, 1.0f};	   // green
constexpr glm::vec4 COLOR_NUMBER	 = {0.980f, 0.702f, 0.529f, 1.0f};	   // peach
constexpr glm::vec4 COLOR_COMMENT	 = {0.424f, 0.439f, 0.525f, 1.0f};	   // overlay0
constexpr glm::vec4 COLOR_FUNCTION	 = {0.537f, 0.706f, 0.980f, 1.0f};	   // blue
constexpr glm::vec4 COLOR_TYPE		 = {0.976f, 0.886f, 0.686f, 1.0f};	   // yellow
constexpr glm::vec4 COLOR_BUILTIN	 = {0.953f, 0.545f, 0.659f, 1.0f};	   // red
constexpr glm::vec4 COLOR_CONSTANT	 = {0.961f, 0.878f, 0.863f, 1.0f};	   // rosewater
constexpr glm::vec4 COLOR_PROPERTY	 = {0.580f, 0.886f, 0.835f, 1.0f};	   // teal
constexpr glm::vec4 COLOR_LABEL	 = {0.455f, 0.780f, 0.925f, 1.0f};	   // sapphire
constexpr glm::vec4 COLOR_OPERATOR	 = {0.537f, 0.863f, 0.922f, 1.0f};	   // sky
constexpr glm::vec4 COLOR_MODULE	 = {0.706f, 0.745f, 0.996f, 1.0f};	   // lavender

const std::unordered_map<std::string_view, glm::vec4> kCaptureColors{
	{"keyword",			COLOR_KEYWORD },
	{"string",			COLOR_STRING  },
	{"number",			COLOR_NUMBER  },
	{"comment",			COLOR_COMMENT },
	{"function",		COLOR_FUNCTION},
	{"function.special", COLOR_FUNCTION},
	{"type",			COLOR_TYPE	  },
	{"variable.builtin", COLOR_BUILTIN },
	{"constant",		COLOR_CONSTANT},
	{"property",		COLOR_PROPERTY},
	{"label",			COLOR_LABEL	  },
	{"operator",		COLOR_OPERATOR},
	{"delimiter",		COLOR_TEXT	  },
	{"module",			COLOR_MODULE  },
	{"variable",		COLOR_TEXT	  },
};

glm::vec4 colorForCapture(std::string_view name) {
	if (auto it = kCaptureColors.find(name); it != kCaptureColors.end()) return it->second;
	if (auto dot = name.find('.'); dot != std::string_view::npos) {
		if (auto it = kCaptureColors.find(name.substr(0, dot)); it != kCaptureColors.end()) return it->second;
	}
	return COLOR_TEXT;
}

std::string readFile(const char *path) {
	std::ifstream file(path, std::ios::binary);
	if (!file.is_open()) {
		dbLog(dbg::LOG_ERROR, "Failed to open highlight query file: ", path);
		return {};
	}
	std::ostringstream ss;
	ss << file.rdbuf();
	return ss.str();
}

void encodeUtf8(char32_t codepoint, std::string &out) {
	if (codepoint <= 0x7F) {
		out.push_back(static_cast<char>(codepoint));
	} else if (codepoint <= 0x7FF) {
		out.push_back(static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F)));
		out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
	} else if (codepoint <= 0xFFFF) {
		out.push_back(static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F)));
		out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
		out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
	} else if (codepoint <= 0x10FFFF) {
		out.push_back(static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07)));
		out.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
		out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
		out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
	}
}

/// maps a byte offset in the utf8 buffer back to a codepoint index; `starts` holds the byte offset of
/// every codepoint plus a trailing sentinel equal to the total byte length.
uint32_t byteToCodepointIndex(const std::vector<uint32_t> &starts, uint32_t byteOffset) {
	auto it = std::upper_bound(starts.begin(), starts.end(), byteOffset);
	return static_cast<uint32_t>(it - starts.begin()) - 1;
}

/// evaluates the (#match? @capture "regex") predicates attached to a pattern; other predicate kinds are ignored.
bool evaluatePredicates(const TSQuery *query, uint32_t patternIndex, const TSQueryMatch &match,
						 std::string_view source) {
	uint32_t					 stepCount = 0;
	const TSQueryPredicateStep *steps	   = ts_query_predicates_for_pattern(query, patternIndex, &stepCount);

	uint32_t i = 0;
	while (i < stepCount) {
		uint32_t start = i;
		while (i < stepCount && steps[i].type != TSQueryPredicateStepTypeDone) ++i;
		uint32_t stepEnd = i;
		if (i < stepCount) ++i;	   // skip the Done sentinel
		if (stepEnd - start != 3) continue;
		if (steps[start].type != TSQueryPredicateStepTypeString ||
			steps[start + 1].type != TSQueryPredicateStepTypeCapture ||
			steps[start + 2].type != TSQueryPredicateStepTypeString)
			continue;

		uint32_t	nameLen;
		const char *name = ts_query_string_value_for_id(query, steps[start].value_id, &nameLen);
		if (std::string_view(name, nameLen) != "match?") continue;

		const TSQueryCapture *capture = nullptr;
		for (uint32_t c = 0; c < match.capture_count; ++c) {
			if (match.captures[c].index == steps[start + 1].value_id) {
				capture = &match.captures[c];
				break;
			}
		}
		if (!capture) continue;

		uint32_t	nodeStart = ts_node_start_byte(capture->node);
		uint32_t	nodeEnd	  = ts_node_end_byte(capture->node);
		std::string nodeText(source.substr(nodeStart, nodeEnd - nodeStart));

		uint32_t	regexLen;
		const char *regexStr = ts_query_string_value_for_id(query, steps[start + 2].value_id, &regexLen);
		try {
			std::regex re(std::string(regexStr, regexLen));
			if (!std::regex_search(nodeText, re)) return false;
		} catch (const std::regex_error &) { return false; }
	}
	return true;
}

}	  // namespace

CppHighlighter::CppHighlighter() {
	parser = ts_parser_new();
	ts_parser_set_language(parser, tree_sitter_cpp());

	std::string querySource = readFile("queries/cpp/highlights.scm");

	uint32_t	 errorOffset;
	TSQueryError errorType;
	query = ts_query_new(tree_sitter_cpp(), querySource.data(), (uint32_t)querySource.size(), &errorOffset,
						  &errorType);
	if (!query) {
		dbLog(dbg::LOG_ERROR, "Failed to compile C++ highlight query at byte ", errorOffset, ", error type ",
			  (int)errorType);
	}
}

CppHighlighter::~CppHighlighter() {
	if (query) ts_query_delete(query);
	if (parser) ts_parser_delete(parser);
}

CppHighlighter::CppHighlighter(CppHighlighter &&other) noexcept : parser(other.parser), query(other.query) {
	other.parser = nullptr;
	other.query	 = nullptr;
}

CppHighlighter &CppHighlighter::operator=(CppHighlighter &&other) noexcept {
	if (this != &other) {
		if (query) ts_query_delete(query);
		if (parser) ts_parser_delete(parser);
		parser		 = other.parser;
		query		 = other.query;
		other.parser = nullptr;
		other.query	 = nullptr;
	}
	return *this;
}

std::vector<glm::vec4> CppHighlighter::highlight(const std::u32string &text) {
	std::vector<glm::vec4> colors(text.size(), COLOR_TEXT);
	if (!parser || !query || text.empty()) return colors;

	std::string			   utf8;
	std::vector<uint32_t> codepointByteOffsets;
	utf8.reserve(text.size());
	codepointByteOffsets.reserve(text.size() + 1);
	for (char32_t c : text) {
		codepointByteOffsets.push_back((uint32_t)utf8.size());
		encodeUtf8(c, utf8);
	}
	codepointByteOffsets.push_back((uint32_t)utf8.size());

	TSTree *tree = ts_parser_parse_string(parser, nullptr, utf8.data(), (uint32_t)utf8.size());
	if (!tree) return colors;
	TSNode root = ts_tree_root_node(tree);

	TSQueryCursor *cursor = ts_query_cursor_new();
	ts_query_cursor_exec(cursor, query, root);

	TSQueryMatch match;
	while (ts_query_cursor_next_match(cursor, &match)) {
		if (!evaluatePredicates(query, match.pattern_index, match, utf8)) continue;

		for (uint32_t c = 0; c < match.capture_count; ++c) {
			const TSQueryCapture &capture = match.captures[c];

			uint32_t	nameLen;
			const char *name  = ts_query_capture_name_for_id(query, capture.index, &nameLen);
			glm::vec4	color = colorForCapture(std::string_view(name, nameLen));

			uint32_t startCp = byteToCodepointIndex(codepointByteOffsets, ts_node_start_byte(capture.node));
			uint32_t endCp	 = byteToCodepointIndex(codepointByteOffsets, ts_node_end_byte(capture.node));
			for (uint32_t i = startCp; i < endCp && i < colors.size(); ++i) colors[i] = color;
		}
	}

	ts_query_cursor_delete(cursor);
	ts_tree_delete(tree);
	return colors;
}
