#include "buffer_utils.hpp"

namespace fxed {

std::tuple<std::vector<std::size_t>, nri::MemoryRequirements> getBufferOffsets(
	const std::vector<nri::Buffer *> &buffers) {
	std::vector<nri::MemoryRequirements> memReqs;
	for (auto buffer : buffers) {
		memReqs.push_back(buffer->getMemoryRequirements());
	}

	std::size_t				 totalSize = 0;
	std::vector<std::size_t> offsets;
	for (const auto &req : memReqs) {
		if (totalSize % req.alignment != 0) { totalSize += req.alignment - (totalSize % req.alignment); }
		offsets.push_back(totalSize);

		totalSize += req.size;
	}
	return {offsets, nri::MemoryRequirements(totalSize, nri::MemoryTypeRequest::MEMORY_TYPE_DEVICE, 0)};
}

std::unique_ptr<nri::Allocation> allocateBindMemory(nri::NRI &nri, const std::vector<nri::Buffer *> &buffers,
													nri::MemoryTypeRequest memoryTypeRequest) {
	auto [offsets, memReq] = getBufferOffsets(buffers);

	return allocateBindMemory(nri, offsets, memReq.setTypeRequest(memoryTypeRequest), buffers);
}

std::unique_ptr<nri::Allocation> allocateBindMemory(nri::NRI &nri, const std::vector<std::size_t> &bufferOffsets,
													nri::MemoryRequirements			  memReq,
													const std::vector<nri::Buffer *> &buffers) {
	auto allocation = nri.allocateMemory(memReq);
	for (std::size_t i = 0; i < buffers.size(); i++) {
		buffers[i]->bindMemory(*allocation, bufferOffsets[i]);
	}
	return allocation;
}

}	  // namespace fxed
