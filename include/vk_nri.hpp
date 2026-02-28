#pragma once
#include <vulkan/vulkan_core.h>
#include <memory>
#include <optional>
#include <iostream>

#include <variant>

// #include <slang/slang.h>
// #include <slang/slang-com-ptr.h>

#include "nri.hpp"
#include "vulkan/vulkan.hpp"
#include "vk_my_raii.hpp"
#include "VkBootstrap.h"

namespace nri {
class VulkanNRI;
class VulkanBuffer;
class VulkanImage2D;
class VulkanTLAS;
class VulkanImageView;
class VulkanTexture2D;
class VulkanStorageImage2D;

/// A helper class to manage ownership of an object or a reference to an object
/// template arguments can be for example: <vkraii::Buffer, vk::Buffer>
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
	VulkanNRI				   &nri;
	vkraii::DescriptorPool		pool;
	vkraii::DescriptorSetLayout descriptorSetLayout;
	vkraii::DescriptorSet		bigDescriptorSet;

	int currentBufferDescriptorIndex = 0;
	int currentImageDescriptorIndex	 = 0;

   public:
	VulkanDescriptorAllocator(VulkanNRI &nri);

	ResourceHandle addUniformBufferDescriptor(const VulkanBuffer &buffer);
	ResourceHandle addStorageBufferDescriptor(const VulkanBuffer &buffer);
	ResourceHandle addSamplerImageDescriptor(const VulkanTexture2D &image);
	ResourceHandle addStorageImageDescriptor(const VulkanStorageImage2D &image);

	auto	   &getDescriptorSet() { return bigDescriptorSet; }
	const auto &getDescriptorSet() const { return bigDescriptorSet; }

	auto	   &getDescriptorSetLayout() { return descriptorSetLayout; }
	const auto &getDescriptorSetLayout() const { return descriptorSetLayout; }
};

class VulkanAllocation : public Allocation {
	vkraii::DeviceMemory memory;
	vk::Device			 device;
	std::size_t			 size;

   public:
	VulkanAllocation(std::nullptr_t) : memory(nullptr), device(nullptr), size(0) {}
	VulkanAllocation(VulkanNRI &nri, MemoryRequirements memoryRequirements);

	vk::DeviceMemory getMemory() { return memory; }
	vk::Device		 getDevice() { return device; }
	std::size_t		 getSize() const override { return size; }

	void *map() override;
	void  unmap() override;
};

class VulkanBuffer : public Buffer {
	VulkanNRI		 *nri;
	vkraii::Buffer	  buffer;
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
	VulkanNRI		 *nri;
	vkraii::ImageView imageView;
	vk::Format		  format;

   public:
	VulkanImageView(VulkanNRI &nri, vkraii::ImageView &&imgView, vk::Format fmt);
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
	vkraii::Sampler sampler;

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
	VulkanNRI							*nri;
	OwnerOrNot<vkraii::Image, vk::Image> image;
	vkb::Device							*device;
	vk::ImageLayout						 layout;
	vk::Format							 format;
	vk::ImageAspectFlags				 aspectFlags;

	uint32_t width;
	uint32_t height;

	static vk::ImageAspectFlags getAspectFlags(vk::Format format);

   public:
	void transitionLayout(CommandBuffer &commandBuffer, vk::ImageLayout newLayout, vk::AccessFlags srcAccess,
						  vk::AccessFlags dstAccess, vk::PipelineStageFlags srcStage, vk::PipelineStageFlags dstStage);

	VulkanImage2D(VulkanNRI &nri, vk::Image img, vk::ImageLayout layout, vk::Format fmt, vkb::Device &dev,
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
	vkraii::CommandPool commandPool;

	VulkanCommandPool(vkraii::CommandPool &&pool) : commandPool(std::move(pool)) {}
};

class VulkanCommandQueue : public CommandQueue {
   public:
	vkraii::Queue queue;

	VulkanCommandQueue(vkraii::Queue &&q) : queue(std::move(q)) {}

	SubmitKey submit(CommandBuffer &commandBuffer) override;
	void	  wait(SubmitKey key) override;
};

class VulkanCommandBuffer : public CommandBuffer {
   public:
	vkraii::CommandBuffer commandBuffer;
	bool				  isRecording;

	void begin() override {
		if (!isRecording) {
			vk::CommandBufferBeginInfo beginInfo;
			vkBeginCommandBuffer(commandBuffer, (VkCommandBufferBeginInfo *)&beginInfo);
			isRecording = true;
		}
	}

	void end() override {
		if (isRecording) {
			vkEndCommandBuffer(commandBuffer);
			isRecording = false;
		}
	}

	VulkanCommandBuffer(vkraii::CommandBuffer &&cmdBuf) : commandBuffer(std::move(cmdBuf)), isRecording(false) {}
};

class VulkanProgramBuilder : public ProgramBuilder {
   protected:
	VulkanNRI &nri;

	std::pair<std::vector<vkraii::ShaderModule>, std::vector<vk::PipelineShaderStageCreateInfo>> createShaderModules(
		std::vector<ShaderCreateInfo> &&stagesInfo, const vkb::Device &device);

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
	VulkanNRI			  &nri;
	vkraii::Pipeline	   pipeline;
	vkraii::PipelineLayout pipelineLayout;
	vk::PipelineBindPoint  bindPoint;

   public:
	VulkanProgram(VulkanNRI &nri, vkraii::Pipeline &&ppln, vkraii::PipelineLayout &&layout,
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
	VulkanGraphicsProgram(VulkanNRI &nri, vkraii::Pipeline &&ppln, vkraii::PipelineLayout &&layout)
		: VulkanProgram(nri, std::move(ppln), std::move(layout), vk::PipelineBindPoint::eGraphics) {}

	void draw(CommandBuffer &commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
			  uint32_t firstInstance) override;
	void drawIndexed(CommandBuffer &commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
					 int32_t vertexOffset, uint32_t firstInstance) override;
};

class VulkanComputeProgram : public VulkanProgram, public ComputeProgram {
   public:
	VulkanComputeProgram(VulkanNRI &nri, vkraii::Pipeline &&ppln, vkraii::PipelineLayout &&layout)
		: VulkanProgram(nri, std::move(ppln), std::move(layout), vk::PipelineBindPoint::eCompute) {}

	void dispatch(CommandBuffer &commandBuffer, uint32_t groupCountX, uint32_t groupCountY,
				  uint32_t groupCountZ) override;
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

class VulkanWindow : public Window {
	vkraii::SurfaceKHR surface;
	vkb::Swapchain	   swapChain;
	VulkanCommandQueue presentQueue;

	vkraii::Semaphore imageAvailableSemaphore;
	vkraii::Semaphore renderFinishedSemaphore;
	vkraii::Fence	  inFlightFence;

	std::vector<ImageAndView<VulkanImage2D, VulkanRenderTarget>> swapChainImages;

	std::unique_ptr<VulkanCommandBuffer> commandBuffer;

	uint32_t width = 0, height = 0;
	uint32_t currentImageIndex = 0;

	vk::Format		  surfaceFormat;
	vk::ColorSpaceKHR surfaceColorSpace;

   protected:
   public:
	VulkanWindow(VulkanNRI &nri);
	~VulkanWindow();

	void			createSwapChain(uint32_t &width, uint32_t &height);
	void			beginFrame() override;
	void			endFrame() override;
	ImageAndViewRef getCurrentRenderTarget() override;
	CommandBuffer  &getCurrentCommandBuffer() override;

	void beginRendering(CommandBuffer &cmdBuf, const ImageAndViewRef &renderTarget) override;
	void endRendering(CommandBuffer &cmdBuf) override;

	const vkraii::SurfaceKHR									 &getSurface() { return surface; }
	void														  setSurface(vkraii::SurfaceKHR &&surf);
	vkb::Swapchain												 &getSwapChain() { return swapChain; }
	vkraii::Queue												 &getPresentQueue() { return presentQueue.queue; }
	std::vector<ImageAndView<VulkanImage2D, VulkanRenderTarget>> &getSwapChainImages() { return swapChainImages; }

	CommandQueue &getMainQueue() override { return presentQueue; }
};

class VulkanNRI : public NRI {
	vkb::Instance			   instance;
	vkb::PhysicalDevice		   physicalDevice;
	vkb::Device				   device;
	vkb::InstanceDispatchTable inst_disp;
	vkb::DispatchTable		   disp;

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

	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
	};

	const vkb::Instance		  &getInstance() const { return instance; }
	const vkb::Device		  &getDevice() const { return device; }
	vkb::Device				  &getDevice() { return device; }
	const vkb::PhysicalDevice &getPhysicalDevice() const { return physicalDevice; }
	CommandPool				  &getDefaultCommandPool() override { return defaultCommandPool; }
	VulkanDescriptorAllocator &getDescriptorAllocator() {
		assert(descriptorAllocator.has_value());
		return descriptorAllocator.value();
	}
	auto getDispatchTable() const { return disp; }
	auto getInstanceDispatchTable() const { return inst_disp; }

	bool shouldFlipY() const override { return true; }
	bool supportsRayTracing() const override { return true; }
	bool supportsTextures() const override { return true; }
	void synchronize() const override { vkDeviceWaitIdle(device); }

   private:
	QueueFamilyIndices queueFamilyIndices;
};
}	  // namespace nri
