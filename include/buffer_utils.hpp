#pragma once

#include <memory>
#include <vector>
#include "nri.hpp"

namespace fxed {
std::tuple<std::vector<std::size_t>, nri::MemoryRequirements> getBufferOffsets(
	const std::vector<nri::Buffer *> &buffers);

std::unique_ptr<nri::Allocation> allocateBindMemory(nri::NRI &nri, const std::vector<nri::Buffer *> &buffers,
													nri::MemoryTypeRequest memoryTypeRequest);
std::unique_ptr<nri::Allocation> allocateBindMemory(nri::NRI &nri, const std::vector<std::size_t> &bufferOffsets,
													nri::MemoryRequirements			  memReq,
													const std::vector<nri::Buffer *> &buffers);

template <nri::RequiresMemory... Buffers>
std::tuple<std::array<std::size_t, sizeof...(Buffers)>, nri::MemoryRequirements> getBufferOffsets(Buffers &...buffers) {
	std::array<std::size_t, sizeof...(Buffers)> offsets;
	return [&]<std::size_t... I>(std::index_sequence<I...>) {
		std::size_t totalSize = 0;
		(
			[&] {
				auto req = buffers.getMemoryRequirements();
				if (totalSize % req.alignment != 0) { totalSize += req.alignment - (totalSize % req.alignment); }
				offsets[I] = totalSize;
				totalSize += req.size;
			}(),
			...);
		return std::make_tuple(offsets,
							   nri::MemoryRequirements(totalSize, nri::MemoryTypeRequest::MEMORY_TYPE_DEVICE, 0));
	}(std::index_sequence_for<Buffers...>{});
}

template <nri::RequiresMemory... Buffers>
std::unique_ptr<nri::Allocation> allocateBindMemory(nri::NRI &nri, nri::MemoryTypeRequest memoryTypeRequest,
													Buffers &...buffers) {
	auto [offsets, memReq] = getBufferOffsets(buffers...);
	auto allocation		   = nri.allocateMemory(memReq.setTypeRequest(memoryTypeRequest));
	[&]<std::size_t... I>(std::index_sequence<I...>) {
		(buffers.bindMemory(*allocation, offsets[I]), ...);
	}(std::index_sequence_for<Buffers...>{});
	return allocation;
}
}	  // namespace fxed
