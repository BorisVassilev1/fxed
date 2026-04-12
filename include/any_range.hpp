#pragma once

#include <ranges>
#include <memory>

namespace fxed {
template <typename T>
class any_input_range {
   public:
	struct sentinel;
	struct iterator;

   private:
	struct IterBase {
		virtual const T &current()						 = 0;
		virtual void	 advance()						 = 0;
		virtual bool	 equals(const sentinel &s) const = 0;
		virtual ~IterBase()								 = default;
	};

	std::unique_ptr<IterBase> iterImpl;

	template <std::ranges::input_range R>
	struct IterImpl : IterBase {
		std::ranges::iterator_t<R> it;
		std::ranges::sentinel_t<R> end;

	   public:
		IterImpl(R &&range) : it(std::ranges::begin(range)), end(std::ranges::end(range)) {}

		const T &current() override { return *it; }
		void	 advance() override { ++it; }
		bool	 equals(const sentinel &) const override { return it == end; }
	};

   public:
	template <std::ranges::input_range R>
	any_input_range(R &&range) : iterImpl(std::make_unique<IterImpl<R>>(std::forward<R>(range))) {}

	any_input_range(any_input_range &&other)				 = default;
	any_input_range(const any_input_range &other)			 = default;
	any_input_range &operator=(any_input_range &&other)		 = default;
	any_input_range &operator=(const any_input_range &other) = default;

	struct sentinel {
		bool operator==(const iterator &it) const { return it.iterImpl->equals(*this); }
		bool operator!=(const iterator &it) const { return !it.iterImpl->equals(*this); }
	};

	struct iterator {
		using iterator_category = std::input_iterator_tag;
		using value_type		= T;
		using difference_type	= std::ptrdiff_t;
		using pointer			= void;
		using reference			= const T &;

		IterBase *iterImpl;

		iterator() : iterImpl(nullptr) {}
		iterator(const iterator &other)			   = default;
		iterator(iterator &&other)				   = default;
		iterator &operator=(const iterator &other) = default;
		iterator &operator=(iterator &&other)	   = default;

		iterator(IterBase *impl) : iterImpl(impl) {}

		reference operator*() const { return iterImpl->current(); }

		iterator &operator++() {
			iterImpl->advance();
			return *this;
		}
		iterator operator++(int) {
			iterator temp = *this;
			iterImpl->advance();
			return temp;
		}

		bool operator==(const sentinel &s) const { return iterImpl->equals(s); }
		bool operator!=(const sentinel &s) const { return !iterImpl->equals(s); }
		bool operator==(const iterator &other) const { return iterImpl == other.iterImpl; }
		bool operator!=(const iterator &other) const { return iterImpl != other.iterImpl; }
	};

	iterator begin() { return iterator{iterImpl.get()}; }
	iterator begin() const { return iterator{iterImpl.get()}; }
	sentinel end() { return sentinel{}; }
	sentinel end() const { return sentinel{}; }
};

static_assert(std::ranges::input_range<any_input_range<int>>);
static_assert(std::ranges::range<any_input_range<int>>);

}	  // namespace fxed
