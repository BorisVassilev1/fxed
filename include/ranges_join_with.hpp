#pragma once

#include <iostream>
#include <ranges>
#include <type_traits>
#include <vector>
#include "utils.hpp"

namespace fxed {
template <std::ranges::input_range R>
class JoinWith : public std::ranges::view_interface<JoinWith<R>> {
	R base;

	using InnerRange	 = std::ranges::range_value_t<R>;
	using InnerValueType = std::ranges::range_value_t<InnerRange>;

	InnerValueType separator;

   public:
	JoinWith() = default;
	JoinWith(R &&base, InnerValueType separator) : base(std::forward<R>(base)), separator(separator) {}
	JoinWith(const R &base, InnerValueType separator) : base(base), separator(separator) {}

	JoinWith(JoinWith &&other)				   = default;
	JoinWith(const JoinWith &other)			   = delete;
	JoinWith &operator=(JoinWith &&other)	   = default;
	JoinWith &operator=(const JoinWith &other) = delete;

	// base - range of ranges, e.g. vector<vector<int>>
	// Inner - range of elements, e.g. vector<int>
	template <class Base, class Inner>
	class iterator;
	template <class Base>
	class sentinel;

	template <class Base, class Inner>
	class iterator {
		std::ranges::iterator_t<Base>  it;
		std::ranges::sentinel_t<Base>  end;
		std::ranges::iterator_t<Inner> innerIt;
		std::ranges::sentinel_t<Inner> innerEnd;
		InnerValueType				   separator;
		bool						   isSeparator = false;

	   public:
		using iterator_category = std::input_iterator_tag;
		using value_type		= std::ranges::range_value_t<Inner>;
		using difference_type	= std::ptrdiff_t;
		using pointer			= void;
		using reference			= std::add_const_t<std::ranges::range_value_t<Inner>> &;

		iterator() = default;
		template <class Iterator, class Sentinel>
		iterator(Iterator &&it, Sentinel &&end, std::ranges::range_value_t<Inner> separator)
			: it(std::forward<Iterator>(it)), end(end), separator(separator) {
			if (it != end) {
				innerIt	 = std::ranges::begin(*it);
				innerEnd = std::ranges::end(*it);
			} else dbLog(dbg::LOG_WARNING, "JoinWith: base range is empty");
		}

		reference operator*() const {
			if (isSeparator) {
				return separator;
			} else {
				return *innerIt;
			}
		}

		iterator &operator++() {
			if (isSeparator) {
				++it;
				if (it != end) {
					innerIt	 = std::ranges::begin(*it);
					innerEnd = std::ranges::end(*it);
				}
			} else {
				++innerIt;
			}

			if (innerIt == innerEnd && it != end) {
				isSeparator = true;
			} else {
				isSeparator = false;
			}

			return *this;
		}

		iterator operator++(int) {
			iterator temp = *this;
			++(*this);
			return temp;
		}

		template <class OtherBase, class OtherInner>
		bool operator==(const iterator<OtherBase, OtherInner> &other) const {
			return it == other.it && (isSeparator == other.isSeparator) && (isSeparator || innerIt == other.innerIt);
		}
		template <class OtherBase, class OtherInner>
		bool operator!=(const iterator<OtherBase, OtherInner> &other) const {
			return it != other.it || (isSeparator != other.isSeparator) || (!isSeparator && innerIt != other.innerIt);
		}

		template <class OtherBase>
		bool operator==(const sentinel<OtherBase> &s) const {
			return it == s.end;
		}
		template <class OtherBase>
		bool operator!=(const sentinel<OtherBase> &s) const {
			return it != s.end;
		}
	};

	template <class Base>
	class sentinel {
		std::ranges::sentinel_t<Base> end;

	   public:
		friend class iterator<std::remove_const_t<Base>, InnerRange>;
		friend class iterator<std::add_const_t<Base>, InnerRange>;

		sentinel() = default;
		template <class Sentinel>
		sentinel(Sentinel &&end) : end(std::forward<Sentinel>(end)) {}

		template <class OtherBase, class OtherInner>
		bool operator==(const iterator<OtherBase, OtherInner> &it) const {
			return it.it == end;
		}
		template <class OtherBase, class OtherInner>
		bool operator!=(const iterator<OtherBase, OtherInner> &it) const {
			return it.it != end;
		}
	};

	auto end() { return sentinel<R>(std::ranges::end(base)); }
	auto end() const { return sentinel<const R>(std::ranges::end(base)); }
	auto begin() {
		return iterator<R, std::ranges::range_value_t<R>>(std::ranges::begin(base), std::ranges::end(base), separator);
	}
	auto begin() const {
		return iterator<const R, std::ranges::range_value_t<R>>(std::ranges::begin(base), std::ranges::end(base),
																separator);
	}
};

template <std::ranges::input_range R>
JoinWith(R &&r, std::ranges::range_value_t<R> separator) -> JoinWith<std::views::all_t<R>>;

template <std::ranges::input_range R>
JoinWith(const R &r, std::ranges::range_value_t<R> separator) -> JoinWith<std::views::all_t<const R>>;

static_assert(std::ranges::range<JoinWith<std::vector<std::vector<int>>>>, "JoinWith should be a range");
static_assert(std::ranges::viewable_range<JoinWith<std::vector<std::vector<int>>>>,
			  "JoinWith should be a viewable range");
static_assert(std::movable<JoinWith<std::vector<std::vector<int>>>>, "JoinWith should be movable");
static_assert(std::ranges::view<JoinWith<std::vector<std::vector<int>>>>, "JoinWith should be a view");

template <class Separator>
struct join_with_fn : public std::ranges::view_interface<join_with_fn<Separator>> {
	Separator separator;
	join_with_fn() = default;
	template <class Separator2>
	join_with_fn(Separator2 &&separator) : separator(std::forward<Separator2>(separator)) {}

	join_with_fn(const join_with_fn &other)			   = default;
	join_with_fn(join_with_fn &&other)				   = default;
	join_with_fn &operator=(const join_with_fn &other) = default;
	join_with_fn &operator=(join_with_fn &&other)	   = default;

	template <std::ranges::input_range R>
	auto operator()(R &&r) const {
		return JoinWith{std::forward<R>(r), separator};
	}
};

template <class Separator>
constexpr inline const join_with_fn<Separator> join_with(Separator separator) {
	return join_with_fn<Separator>(separator);
}

}	  // namespace fxed

template <class Separator>
auto operator|(auto &&r, const fxed::join_with_fn<Separator> &fn) {
	return fn(std::forward<decltype(r)>(r));
}
