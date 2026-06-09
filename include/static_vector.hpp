#pragma once

#include <cstddef>
#include <optional>

template <class T>
class StaticVector {
	T		   *data;
	std::size_t _size;
	std::size_t _capacity;

   public:
	StaticVector(void *data, std::size_t capacity) : data((T *)data), _size(0), _capacity(capacity) {}
	StaticVector(const StaticVector &)				  = default;
	StaticVector &operator=(const StaticVector &)	  = default;
	StaticVector(StaticVector &&) noexcept			  = default;
	StaticVector &operator=(StaticVector &&) noexcept = default;

	auto begin() { return data; }
	auto end() { return data + _size; }
	T	&operator[](std::size_t index) { return data[index]; }

	T *push_back(const T &value) {
		if (_size < _capacity) {
			data[_size] = value;
			return &data[_size++];
		} else {
			return nullptr;
		}
	}

	std::size_t size() const { return _size; }
	std::size_t capacity() const { return _capacity; }
	void		clear() { _size = 0; }
	bool		empty() const { return _size == 0; }
};
