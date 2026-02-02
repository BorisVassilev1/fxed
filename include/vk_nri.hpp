#pragma once
#include <memory>
#include <optional>
#include <iostream>

#include <variant>
#include <vulkan/vulkan_raii.hpp>

// #include <slang/slang.h>
// #include <slang/slang-com-ptr.h>

#include "nri.hpp"
#include "vulkan/vulkan.hpp"

namespace nri {
class VulkanNRI;
class VulkanBuffer;
class VulkanImage2D;
class VulkanTLAS;
class VulkanImageView;
class VulkanTexture2D;
class VulkanStorageImage2D;

/// A helper class to manage ownership of an object or a reference to an object
/// template arguments can be for example: <vk::raii::Buffer, vk::Buffer>
template <class Owner, class NotOwner>
class OwnerOrNot {
	std::variant<Owner, NotOwner> data;
	bool						  isOwner;

   public:
	OwnerOrNot(Owner &&owner) : data(std::move(owner)), isOwner(true) {}
	OwnerOrNot(NotOwner &notOwner) : data(notOwner), isOwner(false) {}

	OwnerOrNot(OwnerOrNot &&other)			  = default;
	OwnerOrNot &operator=(OwnerOrNot &&other) = default;

	NotOwner get() {
		if (isOwner) {
			return (NotOwner)std::get<Owner>(data);
		} else {
			return std::get<NotOwner>(data);
		}
	}
};

class VulkanDescriptorAllocator {
	VulkanNRI					 &nri;
	vk::raii::DescriptorPool	  pool;
	vk::raii::DescriptorSetLayout descriptorSetLayout;
	vk::raii::DescriptorSet		  bigDescriptorSet;

	int currentBufferDescriptorIndex = 0;
	int currentImageDescriptorIndex	 = 0;
	int currentASDescriptorIndex	 = 0;

   public:
	VulkanDescriptorAllocator(VulkanNRI &nri);

	ResourceHandle addUniformBufferDescriptor(const VulkanBuffer &buffer);
	ResourceHandle addStorageBufferDescriptor(const VulkanBuffer &buffer);
	ResourceHandle addSamplerImageDescriptor(const VulkanTexture2D &image);
	ResourceHandle addAccelerationStructureDescriptor(const VulkanTLAS &tlas);
	ResourceHandle addStorageImageDescriptor(const VulkanStorageImage2D &image);

	auto	   &getDescriptorSet() { return bigDescriptorSet; }
	const auto &getDescriptorSet() const { return bigDescriptorSet; }

	auto	   &getDescriptorSetLayout() { return descriptorSetLayout; }
	const auto &getDescriptorSetLayout() const { return descriptorSetLayout; }
};

class VulkanAllocation : public Allocation {
	vk::raii::DeviceMemory memory;
	vk::Device			   device;
	std::size_t			   size;

   public:
	VulkanAllocation(std::nullptr_t) : memory(nullptr), device(nullptr), size(0) {}
	VulkanAllocation(VulkanNRI &nri, MemoryRequirements memoryRequirements);

	vk::DeviceMemory getMemory() { return memory; }
	vk::Device		 getDevice() { return device; }
	std::size_t		 getSize() const override { return size; }
};

class VulkanBuffer : public Buffer {
	VulkanNRI		 *nri;
	vk::raii::Buffer  buffer;
	VulkanAllocation *allocation;

	std::size_t offset = 0;
	std::size_t size   = 0;

	ResourceHandle createHandle() const override;

   public:
	VulkanBuffer(std::nullptr_t) : nri(nullptr), buffer(nullptr), allocation(nullptr), offset(0), size(0) {}
	VulkanBuffer(VulkanNRI &nri, std::size_t size, BufferUsage usage);

	MemoryRequirements getMemoryRequirements() override;
	void			   bindMemory(Allocation &allocation, std::size_t offset) override;
	void			  *map(std::size_t offset, std::size_t size) override;
	void			   unmap() override;

	std::size_t getSize() const override;
	std::size_t getOffset() const override;
	const auto &getBuffer() const { return buffer; }
	auto	   &getBuffer() { return buffer; }

	void copyFrom(CommandBuffer &commandBuffer, Buffer &srcBuffer, std::size_t srcOffset, std::size_t dstOffset,
				  std::size_t size) override;
	void bindAsVertexBuffer(CommandBuffer &commandBuffer, uint32_t binding, std::size_t offset,
							std::size_t stride) override;
	void bindAsIndexBuffer(CommandBuffer &commandBuffer, std::size_t offset, IndexType indexType) override;

	vk::DeviceAddress getAddress();
};

class VulkanImageView : public ImageView {
   protected:
	VulkanNRI		   *nri;
	vk::raii::ImageView imageView;
	vk::Format			format;

   public:
	VulkanImageView(VulkanNRI &nri, vk::raii::ImageView &&imgView, vk::Format fmt);
	auto	   &get() { return imageView; }
	const auto &get() const { return imageView; }
	auto	   &getFormat() { return format; }
	const auto &getFormat() const { return format; }
};

class VulkanRenderTarget : public VulkanImageView {
   public:
	VulkanRenderTarget(VulkanNRI &nri, VulkanImage2D &image2D);

	ResourceHandle createHandle() const override;
};

class VulkanTexture2D : public VulkanImageView {
	vk::raii::Sampler sampler;

   public:
	VulkanTexture2D(VulkanNRI &nri, VulkanImage2D &image2D);

	ResourceHandle createHandle() const override;

	auto	   &getSampler() { return sampler; }
	const auto &getSampler() const { return sampler; }
};

class VulkanStorageImage2D : public VulkanImageView {
   public:
	VulkanStorageImage2D(VulkanNRI &nri, VulkanImage2D &image2D);
	ResourceHandle createHandle() const override;
};

class VulkanImage2D : public Image2D {
	VulkanNRI							  *nri;
	OwnerOrNot<vk::raii::Image, vk::Image> image;
	vk::raii::Device					  *device;
	vk::ImageLayout						   layout;
	vk::Format							   format;
	vk::ImageAspectFlags				   aspectFlags;

	uint32_t width;
	uint32_t height;

	static vk::ImageAspectFlags getAspectFlags(vk::Format format);

   public:
	void transitionLayout(CommandBuffer &commandBuffer, vk::ImageLayout newLayout, vk::AccessFlags srcAccess,
						  vk::AccessFlags dstAccess, vk::PipelineStageFlags srcStage, vk::PipelineStageFlags dstStage);

	VulkanImage2D(VulkanNRI &nri, vk::Image img, vk::ImageLayout layout, vk::Format fmt, vk::raii::Device &dev,
				  uint32_t width, uint32_t height)
		: nri(&nri),
		  image(img),
		  device(&dev),
		  layout(layout),
		  format(fmt),
		  aspectFlags(getAspectFlags(fmt)),
		  width(width),
		  height(height) {}

	VulkanImage2D(VulkanNRI &nri, uint32_t width, uint32_t height, Format format, ImageUsage usage);

	VulkanImage2D(VulkanImage2D &&other)			= default;
	VulkanImage2D &operator=(VulkanImage2D &&other) = default;

	MemoryRequirements getMemoryRequirements() override;
	void			   bindMemory(Allocation &allocation, std::size_t offset) override;

	void clear(CommandBuffer &commandBuffer, glm::vec4 color) override;

	void prepareForPresent(CommandBuffer &commandBuffer) override;
	void prepareForStorage(CommandBuffer &commandBuffer) override;
	void prepareForTexture(CommandBuffer &commandBuffer) override;
	void copyFrom(CommandBuffer &commandBuffer, Buffer &srcBuffer, std::size_t srcOffset,
				  uint32_t srcRowPitch) override;

	uint32_t getWidth() const override { return width; }
	uint32_t getHeight() const override { return height; }

	vk::ImageAspectFlags getAspectFlags() { return aspectFlags; }

	auto get() { return image.get(); }
	auto getFormat() { return format; }

	std::unique_ptr<ImageView> createRenderTargetView() override;
	std::unique_ptr<ImageView> createTextureView() override;
	std::unique_ptr<ImageView> createStorageView() override;
};

class VulkanCommandPool : public CommandPool {
   public:
	vk::raii::CommandPool commandPool;

	VulkanCommandPool(vk::raii::CommandPool &&pool) : commandPool(std::move(pool)) {}
};

class VulkanCommandQueue : public CommandQueue {
   public:
	vk::raii::Queue queue;

	VulkanCommandQueue(vk::raii::Queue &&q) : queue(std::move(q)) {}

	SubmitKey submit(CommandBuffer &commandBuffer) override;
	void	  wait(SubmitKey key) override;
};

class VulkanCommandBuffer : public CommandBuffer {
   public:
	vk::raii::CommandBuffer commandBuffer;
	bool					isRecording;

	void begin() override {
		if (!isRecording) {
			vk::CommandBufferBeginInfo beginInfo;
			commandBuffer.begin(beginInfo);
			isRecording = true;
		}
	}

	void end() override {
		if (isRecording) {
			commandBuffer.end();
			isRecording = false;
		}
	}

	VulkanCommandBuffer(vk::raii::CommandBuffer &&cmdBuf) : commandBuffer(std::move(cmdBuf)), isRecording(false) {}
};

class VulkanProgramBuilder : public ProgramBuilder {
   protected:
	VulkanNRI &nri;

	std::pair<std::vector<vk::raii::ShaderModule>, std::vector<vk::PipelineShaderStageCreateInfo>> createShaderModules(
		std::vector<ShaderCreateInfo> &&stagesInfo, const vk::raii::Device &device);

	std::vector<vk::PushConstantRange> createPushConstantRanges(
		const std::vector<PushConstantRange> &nriPushConstantRanges);

   public:
	VulkanProgramBuilder(VulkanNRI &nri) : nri(nri) {}

	std::unique_ptr<GraphicsProgram>   buildGraphicsProgram() override;
	std::unique_ptr<ComputeProgram>	   buildComputeProgram() override;
	std::unique_ptr<RayTracingProgram> buildRayTracingProgram(nri::CommandBuffer &cmdBuff) override;
};

class VulkanProgram : virtual Program {
   protected:
	VulkanNRI				&nri;
	vk::raii::Pipeline		 pipeline;
	vk::raii::PipelineLayout pipelineLayout;
	vk::PipelineBindPoint	 bindPoint;

   public:
	VulkanProgram(VulkanNRI &nri, vk::raii::Pipeline &&ppln, vk::raii::PipelineLayout &&layout,
				  vk::PipelineBindPoint bindPoint)
		: nri(nri), pipeline(std::move(ppln)), pipelineLayout(std::move(layout)), bindPoint(bindPoint) {}

	void bind(CommandBuffer &commandBuffer) override;
	void unbind(CommandBuffer &commandBuffer) override;

	void setPushConstants(CommandBuffer &commandBuffer, const void *data, std::size_t size,
						  std::size_t offset) override;

	friend class VulkanProgramBuilder;
};

class VulkanGraphicsProgram : public VulkanProgram, public GraphicsProgram {
   public:
	VulkanGraphicsProgram(VulkanNRI &nri, vk::raii::Pipeline &&ppln, vk::raii::PipelineLayout &&layout)
		: VulkanProgram(nri, std::move(ppln), std::move(layout), vk::PipelineBindPoint::eGraphics) {}

	void draw(CommandBuffer &commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
			  uint32_t firstInstance) override;
	void drawIndexed(CommandBuffer &commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
					 int32_t vertexOffset, uint32_t firstInstance) override;
};

class VulkanComputeProgram : public VulkanProgram, public ComputeProgram {
   public:
	VulkanComputeProgram(VulkanNRI &nri, vk::raii::Pipeline &&ppln, vk::raii::PipelineLayout &&layout)
		: VulkanProgram(nri, std::move(ppln), std::move(layout), vk::PipelineBindPoint::eCompute) {}

	void dispatch(CommandBuffer &commandBuffer, uint32_t groupCountX, uint32_t groupCountY,
				  uint32_t groupCountZ) override;
};

class VulkanRayTracingProgram : public VulkanProgram, public RayTracingProgram {
   public:
	VulkanBuffer	 sbtBuffer;
	VulkanAllocation sbtMemory;

	VulkanBuffer	 uploadBuffer;
	VulkanAllocation uploadMemory;

	vk::StridedDeviceAddressRegionKHR sbtRayGenRegion;	   // TODO: this should be private
	vk::StridedDeviceAddressRegionKHR sbtMissRegion;
	vk::StridedDeviceAddressRegionKHR sbtHitRegion;
	VulkanRayTracingProgram(VulkanNRI &nri, vk::raii::Pipeline &&ppln, vk::raii::PipelineLayout &&layout,
							VulkanBuffer &&sbtBuf, VulkanAllocation &&sbtMem, VulkanBuffer &&uploadBuf,
							VulkanAllocation &&uploadMem)
		: VulkanProgram(nri, std::move(ppln), std::move(layout), vk::PipelineBindPoint::eRayTracingKHR),
		  sbtBuffer(std::move(sbtBuf)),
		  sbtMemory(std::move(sbtMem)),
		  uploadBuffer(std::move(uploadBuf)),
		  uploadMemory(std::move(uploadMem)) {}

	void traceRays(CommandBuffer &commandBuffer, uint32_t width, uint32_t height, uint32_t depth) override;
};

class VulkanMemoryCache {
	VulkanBuffer	 buffer;
	VulkanAllocation allocation;

	VulkanMemoryCache() : buffer(nullptr), allocation(nullptr) {}

	void assureSize(VulkanNRI &nri, std::size_t size);

   public:
	VulkanBuffer &getAccelerationStructureScratch(VulkanNRI &nri, vk::AccelerationStructureBuildSizesInfoKHR &sizeInfo);

   private:		// this does not work currently
	static VulkanMemoryCache &getInstance();
	static void				  destroy();
};

class VulkanBLAS : public BLAS {
	VulkanNRI						  *nri;
	vk::raii::AccelerationStructureKHR accelerationStructure;
	VulkanBuffer					   asBuffer;
	VulkanAllocation				   asMemory;
	std::size_t						   indexOffset = 0;

	struct TemporaryBuildInfo {
		vk::AccelerationStructureBuildSizesInfoKHR	  sizeInfo;
		vk::AccelerationStructureGeometryKHR		  geometry;
		vk::AccelerationStructureBuildRangeInfoKHR	  buildRangeInfo;
		vk::AccelerationStructureBuildGeometryInfoKHR buildGeometryInfo;
		VulkanBuffer								 *vertexBuffer;
		VulkanBuffer								 *indexBuffer;
		VulkanBuffer								  scratchBuffer = nullptr;
		VulkanAllocation							  scratchMemory = nullptr;
	};
	std::unique_ptr<TemporaryBuildInfo> tempBuildInfo;

   public:
	VulkanBLAS(VulkanNRI &nri, VulkanBuffer &vertexBuffer, Format vertexFormat, std::size_t vertexOffset,
			   uint32_t vertexCount, std::size_t vertexStride, VulkanBuffer &indexBuffer, IndexType indexType,
			   std::size_t indexOffset);

	void build(CommandBuffer &commandBuffer) override;
	void buildFinished() override;

	auto			 &getAccelerationStructure() { return accelerationStructure; }
	const auto		 &getAccelerationStructure() const { return accelerationStructure; }
	vk::DeviceAddress getAddress() const;
};

class VulkanTLAS : public TLAS {
	VulkanNRI						  *nri;
	vk::raii::AccelerationStructureKHR as;
	VulkanBuffer					   asBuffer;
	VulkanAllocation				   asMemory;
	mutable ResourceHandle			   handle = ResourceHandle::INVALID_HANDLE;

	struct TemporaryBuildInfo {
		vk::AccelerationStructureGeometryKHR		  geometry;
		vk::AccelerationStructureBuildRangeInfoKHR	  buildRangeInfo;
		vk::AccelerationStructureBuildGeometryInfoKHR buildGeometryInfo;
		VulkanBuffer								  instanceUploadBuffer = nullptr;
		VulkanAllocation							  instanceUploadMemory = nullptr;

		VulkanBuffer	 instanceBuffer = nullptr;
		VulkanAllocation instanceMemory = nullptr;

		VulkanBuffer	 scratchBuffer = nullptr;
		VulkanAllocation scratchMemory = nullptr;
	};

	std::unique_ptr<TemporaryBuildInfo> tempBuildInfo;

   public:
	VulkanTLAS(VulkanNRI &nri, const std::span<const BLAS *> &blases,
			   std::optional<std::span<glm::mat3x4>> transforms = std::nullopt);

	void		   build(CommandBuffer &commandBuffer) override;
	void		   buildFinished() override;
	ResourceHandle getHandle() const override;

	auto	   &getTLAS() { return as; }
	const auto &getTLAS() const { return as; }
};

class VulkanWindow : public Window {
	vk::raii::SurfaceKHR   surface;
	vk::raii::SwapchainKHR swapChain;
	VulkanCommandQueue	   presentQueue;

	vk::raii::Semaphore imageAvailableSemaphore;
	vk::raii::Semaphore renderFinishedSemaphore;
	vk::raii::Fence		inFlightFence;

	std::vector<ImageAndView<VulkanImage2D, VulkanRenderTarget>> swapChainImages;
	std::optional<VulkanRenderTarget>							 depthImageView;
	std::optional<VulkanImage2D>								 depthImage;
	std::optional<VulkanAllocation>								 depthImageAllocation;

	std::unique_ptr<VulkanCommandBuffer> commandBuffer;

	uint32_t width = 0, height = 0;
	uint32_t currentImageIndex = 0;

	vk::Format surfaceFormat;
	vk::ColorSpaceKHR surfaceColorSpace;

   protected:
   public:
	VulkanWindow(VulkanNRI &nri);

	void			createSwapChain(uint32_t &width, uint32_t &height);
	void			beginFrame() override;
	void			endFrame() override;
	ImageAndViewRef getCurrentRenderTarget() override;

	void beginRendering(CommandBuffer &cmdBuf, const ImageAndViewRef &renderTarget) override;
	void endRendering(CommandBuffer &cmdBuf) override;

	const vk::raii::SurfaceKHR									 &getSurface() { return surface; }
	void														  setSurface(vk::raii::SurfaceKHR &&surf);
	vk::raii::SwapchainKHR										 &getSwapChain() { return swapChain; }
	vk::raii::Queue												 &getPresentQueue() { return presentQueue.queue; }
	std::vector<ImageAndView<VulkanImage2D, VulkanRenderTarget>> &getSwapChainImages() { return swapChainImages; }

	CommandQueue &getMainQueue() override { return presentQueue; }
};

class VulkanNRI : public NRI {
	vk::raii::Instance		 instance;
	vk::raii::PhysicalDevice physicalDevice;
	vk::raii::Device		 device;

	VulkanCommandPool						 defaultCommandPool;
	std::optional<VulkanDescriptorAllocator> descriptorAllocator;

	void createInstance();
	void pickPhysicalDevice();
	void createLogicalDevice();

   public:
	VulkanNRI(CreateBits createBits = CreateBits::DEFAULT);
	~VulkanNRI();

	std::unique_ptr<Buffer>		  createBuffer(std::size_t size, BufferUsage usage) override;
	std::unique_ptr<Image2D>	  createImage2D(uint32_t width, uint32_t height, Format fmt, ImageUsage usage) override;
	std::unique_ptr<Allocation>	  allocateMemory(MemoryRequirements memoryRequirements) override;
	std::unique_ptr<CommandQueue> createCommandQueue() override;
	std::unique_ptr<CommandBuffer>	createCommandBuffer(const CommandPool &commandPool) override;
	std::unique_ptr<CommandPool>	createCommandPool() override;
	std::unique_ptr<ProgramBuilder> createProgramBuilder() override;
	std::unique_ptr<Window>			createGLFWWindow(GLFWwindow *glfwWindow) override;
	std::unique_ptr<BLAS>			createBLAS(Buffer &vertexBuffer, Format vertexFormat, std::size_t vertexOffset,
											   uint32_t vertexCount, std::size_t vertexStride, Buffer &indexBuffer,
											   IndexType indexType, std::size_t indexOffset) override;

	std::unique_ptr<TLAS> createTLAS(const std::span<const BLAS *>		  &blases,
									 std::optional<std::span<glm::mat3x4>> transforms = std::nullopt) override;

	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
	};

	const vk::raii::Instance	   &getInstance() const { return instance; }
	const vk::raii::Device		   &getDevice() const { return device; }
	vk::raii::Device			   &getDevice() { return device; }
	const vk::raii::PhysicalDevice &getPhysicalDevice() const { return physicalDevice; }
	CommandPool					   &getDefaultCommandPool() override { return defaultCommandPool; }
	VulkanDescriptorAllocator	   &getDescriptorAllocator() {
		 assert(descriptorAllocator.has_value());
		 return descriptorAllocator.value();
	}

	bool shouldFlipY() const override { return true; }
	bool supportsRayTracing() const override { return true; }
	bool supportsTextures() const override { return true; }
	void synchronize() const override { device.waitIdle(); }

   private:
	QueueFamilyIndices queueFamilyIndices;
};
}	  // namespace nri
