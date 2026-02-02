#pragma once

#include <memory>
#include <type_traits>
#include <sstream>
#include <mutex>
#include <string>

#define JOB(name, ...)                     \
	static int _job_##name = []() -> int { \
		__VA_ARGS__;                       \
		return 0;                          \
	}();

#define STR(x) #x

#include <ranges>
#include <algorithm>
namespace beamcast {
template <auto N>
struct string_literal {
	constexpr string_literal(const char (&str)[N]) { std::ranges::copy_n(str, N, value); }

	char value[N];

	constexpr operator const char *() const { return value; }
};

struct string_hash {
	using is_transparent = void;
	[[nodiscard]] size_t operator()(const char *txt) const { return std::hash<std::string_view>{}(txt); }
	[[nodiscard]] size_t operator()(std::string_view txt) const { return std::hash<std::string_view>{}(txt); }
	[[nodiscard]] size_t operator()(const std::string &txt) const { return std::hash<std::string>{}(txt); }
};

std::string getString(std::istream &os);

}	  // namespace beamcast

namespace dbg {

#if defined(__GNUC__) || defined(__clang__)
	#include <cxxabi.h>
template <class T>
auto type_name() {
	typedef typename std::remove_reference<T>::type TR;

	std::unique_ptr<char, void (*)(void *)> own(abi::__cxa_demangle(typeid(TR).name(), nullptr, nullptr, nullptr),
												std::free);

	std::string r = own != nullptr ? own.get() : typeid(TR).name();
	if (std::is_const<TR>::value) r += " const";
	if (std::is_volatile<TR>::value) r += " volatile";
	if (std::is_lvalue_reference<T>::value) r += "&";
	else if (std::is_rvalue_reference<T>::value) r += "&&";
	return r;
}

inline auto typename_demangle(const char *n) {
	std::unique_ptr<char, void (*)(void *)> own(abi::__cxa_demangle(n, nullptr, nullptr, nullptr), std::free);
	std::string								r = own != nullptr ? own.get() : n;
	return r;
}

template <class T>
inline auto type_name(T *v) {
	return typename_demangle(typeid(v).name());
}
#else
template <class T>
auto type_name() {
	typedef typename std::remove_reference<T>::type TR;
	std::string										r = typeid(TR).name();
	if (std::is_const<TR>::value) r += " const";
	if (std::is_volatile<TR>::value) r += " volatile";
	if (std::is_lvalue_reference<T>::value) r += "&";
	else if (std::is_rvalue_reference<T>::value) r += "&&";
	return r;
}
template <class T>
inline auto type_name(T *v) {
	return typeid(v).name();
}
#endif

/**
 *
 * @brief Log levels
 */
enum {
	LOG_DEBUG	= (0),
	LOG_INFO	= (1),
	LOG_WARNING = (2),
	LOG_ERROR	= (3),
};

#define COLOR_RESET	 "\033[0m"
#define COLOR_RED	 "\x1B[0;91m"
#define COLOR_GREEN	 "\x1B[0;92m"
#define COLOR_YELLOW "\x1B[0;93m"

static const char *log_colors[]{COLOR_GREEN, COLOR_RESET, COLOR_YELLOW, COLOR_RED};

std::mutex &getMutex();

/**
 * @brief prints to std::cerr
 *
 * @return 1
 */
template <class... Types>
bool inline f_dbLog(std::ostream &out, Types... args) {
	std::lock_guard lock(dbg::getMutex());
	(out << ... << args) << std::flush;
	return 1;
}

/**
 * @def dbLog(severity, ...)
 * If severity is greater than the definition DBG_LOG_LEVEL, prints all arguments to std::cerr
 */

#ifndef NDEBUG
	#define DBG_DEBUG
	#ifndef DBG_LOG_LEVEL
		#define DBG_LOG_LEVEL -1
	#endif
#else
	#ifndef DBG_LOG_LEVEL
		#define DBG_LOG_LEVEL 1
	#endif
#endif

#define dbLog(severity, ...)                                                                                       \
	{                                                                                                              \
		if constexpr (severity >= DBG_LOG_LEVEL) {                                                                 \
			if constexpr (severity >= dbg::LOG_WARNING) {                                                          \
				dbg::f_dbLog(std::cerr, dbg::log_colors[severity], "[", #severity, "] ", __VA_ARGS__, COLOR_RESET, \
							 '\n');                                                                                \
			} else {                                                                                               \
				dbg::f_dbLog(std::cout, dbg::log_colors[severity], "[", #severity, "] ", __VA_ARGS__, COLOR_RESET, \
							 '\n');                                                                                \
			}                                                                                                      \
		}                                                                                                          \
	}
#define dbLogR(severity, ...)                                                                               \
	{                                                                                                       \
		if constexpr (severity >= DBG_LOG_LEVEL) {                                                          \
			if constexpr (severity >= dbg::LOG_WARNING) {                                                   \
				dbg::f_dbLog(std::cerr, '\r', dbg::log_colors[severity], "[", #severity, "] ", __VA_ARGS__, \
							 COLOR_RESET);                                                                  \
			} else {                                                                                        \
				dbg::f_dbLog(std::cout, '\r', dbg::log_colors[severity], "[", #severity, "] ", __VA_ARGS__, \
							 COLOR_RESET);                                                                  \
			}                                                                                               \
		}                                                                                                   \
	}

#define THROW_RUNTIME_ERR(message) \
	throw std::runtime_error("At " + std::string(__PRETTY_FUNCTION__) + ":\n\t" + message + COLOR_RESET);

#define DELETE_COPY_AND_ASSIGNMENT(TYPE)         \
	TYPE(const TYPE &other)			   = delete; \
	TYPE &operator=(const TYPE &other) = delete;

}	  // namespace dbg
