#pragma once

#include <cstdint>
#include <ranges>
#include <string>
#include <istream>

namespace fxed {

class RawChar {
	char c;

   public:
	RawChar() : c(0) {}
	RawChar(char c) : c(c) {}
	operator char() const { return c; }

	friend std::istream &operator>>(std::istream &is, RawChar &rc) {
		is.get(rc.c);
		return is;
	}
};

/// converts stream of chars to stream of char32_t, assuming the input is UTF-8 encoded
template <std::ranges::input_range R>
class ToUtf32 : public std::ranges::view_interface<ToUtf32<R>> {
	R base;

   public:
	ToUtf32() = default;
	ToUtf32(R &&base) : base(std::move(base)) {}
	ToUtf32(const R &base) : base(base) {}

	ToUtf32(ToUtf32 &&other)				 = default;
	ToUtf32(const ToUtf32 &other)			 = delete;
	ToUtf32 &operator=(ToUtf32 &&other)		 = default;
	ToUtf32 &operator=(const ToUtf32 &other) = delete;

	template <class Base>
	class sentinel;

	template <class Base>
	class iterator {
		std::ranges::iterator_t<Base> it;
		std::ranges::sentinel_t<Base> end;
		uint32_t					  codepoint = 0;
		int							  bytesRead = 0;

	   public:
		using iterator_category = std::input_iterator_tag;
		using value_type		= char32_t;
		using difference_type	= std::ptrdiff_t;
		using pointer			= void;
		using reference			= char32_t;

		friend class sentinel<std::remove_const_t<Base>>;
		friend class sentinel<std::add_const_t<Base>>;

		iterator() = default;
		template <class Iterator, class Sentinel>
		iterator(Iterator &&it, Sentinel &&end) : it(std::forward<Iterator>(it)), end(end) {
			readCodepoint();
		}

		char32_t  operator*() const { return codepoint; }
		iterator &operator++() {
			readCodepoint();
			return *this;
		}
		void operator++(int) { readCodepoint(); }
		template <class OtherBase>
		bool operator==(const iterator<OtherBase> &other) const {
			return it == other.it;
		}
		template <class OtherBase>
		bool operator!=(const iterator<OtherBase> &other) const {
			return it != other.it;
		}

		template <class OtherBase>
		bool operator==(const sentinel<OtherBase> &s) const {
			return it == s.end;
		}
		template <class OtherBase>
		bool operator!=(const sentinel<OtherBase> &s) const {
			return it != s.end;
		}

	   private:
		void readCodepoint() {
			if (it == end) {
				codepoint = 0;
				return;
			}
			unsigned char c = static_cast<unsigned char>(*it);
			// read up to 4 bytes for UTF-8 codepoint
			if (c <= 0x7F) {
				codepoint = c;
				bytesRead = 0;
			} else if ((c & 0xE0) == 0xC0) {
				codepoint = c & 0x1F;
				bytesRead = 1;
			} else if ((c & 0xF0) == 0xE0) {
				codepoint = c & 0x0F;
				bytesRead = 2;
			} else if ((c & 0xF8) == 0xF0) {
				codepoint = c & 0x07;
				bytesRead = 3;
			} else {
				// invalid UTF-8, skip
				codepoint = 0;
				bytesRead = 0;
			}
			++it;
			for (int i = 0; i < bytesRead; ++i) {
				if (it == end) {
					codepoint = 0;
					return;
				}
				c = static_cast<unsigned char>(*it);
				if ((c & 0xC0) != 0x80) {
					// invalid UTF-8, skip
					codepoint = 0;
					return;
				}
				codepoint = (codepoint << 6) | (c & 0x3F);
				++it;
			}
		}
	};

	auto begin() { return iterator<R>(std::ranges::begin(base), std::ranges::end(base)); }
	auto begin() const { return iterator<const R>(std::ranges::begin(base), std::ranges::end(base)); }

	template <class Base>
	class sentinel {
		std::ranges::sentinel_t<Base> end;

	   public:
		friend class iterator<std::remove_const_t<Base>>;
		friend class iterator<std::add_const_t<Base>>;

		sentinel() = default;
		sentinel(std::ranges::sentinel_t<R> end) : end(end) {}

		template <class OtherBase>
		bool operator==(const iterator<OtherBase> &it) const {
			return it.it == end;
		}
		template <class OtherBase>
		bool operator!=(const iterator<OtherBase> &it) const {
			return it.it != end;
		}
	};

	auto end() { return sentinel<R>(std::ranges::end(base)); }
	auto end() const { return sentinel<const R>(std::ranges::end(base)); }
};

template <std::ranges::input_range R>
ToUtf32(R &&r) -> ToUtf32<std::views::all_t<R>>;

template <std::ranges::input_range R>
ToUtf32(const R &r) -> ToUtf32<std::views::all_t<const R>>;

static_assert(std::ranges::range<ToUtf32<std::string_view>>);
static_assert(std::ranges::viewable_range<ToUtf32<std::string>>);
static_assert(std::movable<ToUtf32<std::string_view>>);
static_assert(std::ranges::view<ToUtf32<std::string_view>>);
static_assert(std::ranges::input_range<std::ranges::istream_view<char>>);
static_assert(std::ranges::input_range<ToUtf32<std::ranges::istream_view<RawChar>>>);

struct to_utf32_fn : public std::ranges::view_interface<to_utf32_fn> {
	template <std::ranges::input_range R>
	auto operator()(R &&r) const {
		return fxed::ToUtf32{std::forward<R>(r)};
	}
};

constexpr inline const to_utf32_fn to_utf32{};

template <std::ranges::input_range R>
class ToUtf8 : public std::ranges::view_interface<ToUtf8<R>> {
	R base;

   public:
	ToUtf8() = default;
	ToUtf8(R &&base) : base(std::move(base)) {}
	ToUtf8(const R &base) : base(base) {}

	ToUtf8(ToUtf8 &&other)				   = default;
	ToUtf8(const ToUtf8 &other)			   = delete;
	ToUtf8 &operator=(ToUtf8 &&other)	   = default;
	ToUtf8 &operator=(const ToUtf8 &other) = delete;

	template <class Base>
	class iterator;
	template <class Base>
	class sentinel;

	template <class Base>
	class iterator {
		std::ranges::iterator_t<Base> it;
		std::ranges::sentinel_t<Base> end;
		char						  c[4];
		int							  bytesToWrite = 0;

	   public:
		using iterator_category = std::input_iterator_tag;
		using value_type		= char;
		using difference_type	= std::ptrdiff_t;
		using pointer			= void;
		using reference			= char;

		friend class sentinel<std::remove_const_t<Base>>;
		friend class sentinel<std::add_const_t<Base>>;

		iterator() = default;
		template <class Iterator, class Sentinel>
		iterator(Iterator &&it, Sentinel &&end) : it(std::forward<Iterator>(it)), end(end) {
			writeCodepoint();
		}

		char	  operator*() const { return c[4 - bytesToWrite]; }
		iterator &operator++() {
			if (bytesToWrite > 0) {
				bytesToWrite--;
			} else {
				writeCodepoint();
			}
			return *this;
		}
		void operator++(int) {
			if (bytesToWrite > 0) {
				bytesToWrite--;
			} else {
				writeCodepoint();
			}
		}
		template <class OtherBase>
		bool operator==(const iterator<OtherBase> &other) const {
			return it == other.it && bytesToWrite == other.bytesToWrite;
		}
		template <class OtherBase>
		bool operator!=(const iterator<OtherBase> &other) const {
			return it != other.it || bytesToWrite != other.bytesToWrite;
		}
		template <class OtherBase>
		bool operator==(const sentinel<OtherBase> &s) const {
			return it == s.end && bytesToWrite == 0;
		}
		template <class OtherBase>
		bool operator!=(const sentinel<OtherBase> &s) const {
			return it != s.end || bytesToWrite != 0;
		}

	   private:
		void writeCodepoint() {
			if (it == end) {
				bytesToWrite = 0;
				return;
			}
			char32_t codepoint = *it++;
			if (codepoint <= 0x7F) {
				c[3]		 = static_cast<char>(codepoint);
				bytesToWrite = 1;
			} else if (codepoint <= 0x7FF) {
				c[2]		 = static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F));
				c[3]		 = static_cast<char>(0x80 | (codepoint & 0x3F));
				bytesToWrite = 2;
			} else if (codepoint <= 0xFFFF) {
				c[1]		 = static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F));
				c[2]		 = static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
				c[3]		 = static_cast<char>(0x80 | (codepoint & 0x3F));
				bytesToWrite = 3;
			} else if (codepoint <= 0x10FFFF) {
				c[0]		 = static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07));
				c[1]		 = static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
				c[2]		 = static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
				c[3]		 = static_cast<char>(0x80 | (codepoint & 0x3F));
				bytesToWrite = 4;
			} else {
				// invalid codepoint, skip
				bytesToWrite = 0;
			}
		}
	};

	template <class Base>
	class sentinel {
		std::ranges::sentinel_t<Base> end;

	   public:
		friend class iterator<std::remove_const_t<Base>>;
		friend class iterator<std::add_const_t<Base>>;

		sentinel() = default;
		sentinel(std::ranges::sentinel_t<R> end) : end(end) {}

		template <class OtherBase>
		bool operator==(const iterator<OtherBase> &it) const {
			return it.it == end && it.bytesToWrite == 0;
		}
		template <class OtherBase>
		bool operator!=(const iterator<OtherBase> &it) const {
			return it.it != end || it.bytesToWrite != 0;
		}
	};

	auto begin() { return iterator<R>(std::ranges::begin(base), std::ranges::end(base)); }
	auto begin() const { return iterator<const R>(std::ranges::begin(base), std::ranges::end(base)); }
	auto end() { return sentinel<R>(std::ranges::end(base)); }
	auto end() const { return sentinel<const R>(std::ranges::end(base)); }
};

template <std::ranges::input_range R>
ToUtf8(R &&r) -> ToUtf8<std::views::all_t<R>>;

template <std::ranges::input_range R>
ToUtf8(const R &r) -> ToUtf8<std::views::all_t<const R>>;

static_assert(std::ranges::range<ToUtf8<std::basic_string_view<char32_t>>>);
static_assert(std::ranges::viewable_range<ToUtf8<std::basic_string<char32_t>>>);
static_assert(std::movable<ToUtf8<std::basic_string_view<char32_t>>>);
static_assert(std::ranges::view<ToUtf8<std::basic_string_view<char32_t>>>);

struct to_utf8_fn : public std::ranges::view_interface<to_utf8_fn> {
	template <std::ranges::input_range R>
	auto operator()(R &&r) const {
		return fxed::ToUtf8{std::forward<R>(r)};
	}
};

constexpr inline const to_utf8_fn to_utf8{};

}	  // namespace fxed

auto operator|(auto &&r, const fxed::to_utf32_fn &fn) { return fn(std::forward<decltype(r)>(r)); }
auto operator|(auto &&r, const fxed::to_utf8_fn &fn) { return fn(std::forward<decltype(r)>(r)); }
