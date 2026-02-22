#pragma once
#include <font.hpp>
namespace fxed {
class AtlasPacker {
   protected:
	uint32_t width, height;

   public:
	AtlasPacker(uint32_t width, uint32_t height) : width(width), height(height) {}
	virtual ~AtlasPacker() = default;

	virtual std::optional<fxed::Rectangle> pack(fxed::Rectangle rect) = 0;

	virtual std::optional<fxed::Rectangle> pack(fxed::Rectangle rect, uint32_t padding) {
		rect.w += 2 * padding;
		rect.h += 2 * padding;
		auto result = pack(rect);
		if (result) {
			result->x += padding;
			result->y += padding;
			result->w -= 2 * padding;
			result->h -= 2 * padding;
		}
		return result;
	}
};

class RowAtlasPacker : public AtlasPacker {
	std::vector<uint32_t> rowOffsets;
	uint32_t			  rowHeight;

   public:
	RowAtlasPacker(uint32_t width, uint32_t height, uint32_t rowHeight)
		: AtlasPacker(width, height), rowHeight(rowHeight) {
		assert(rowHeight > 0 && rowHeight <= height);
		rowOffsets.resize(height / rowHeight, 0);
	}

	std::optional<fxed::Rectangle> pack(fxed::Rectangle rect) override {
		assert(rect.x == 0 && rect.y == 0);
		if (rect.w > (int)width || rect.h > (int)rowHeight) { return std::nullopt; }

		for (size_t i = 0; i < rowOffsets.size(); i++) {
			if (rowOffsets[i] + rect.w <= width && (i + 1) * rect.h <= height) {
				fxed::Rectangle result{.x = (int)rowOffsets[i], .y = int(i * rowHeight), .w = rect.w, .h = rect.h};
				rowOffsets[i] += rect.w;
				return result;
			}
		}
		return std::nullopt;
	}

	using AtlasPacker::pack;
};
}	  // namespace fxed
