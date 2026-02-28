#include "vk_nri.hpp"
#include <iostream>
#include <ostream>
#include <fstream>
#include <format>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

#include "vk_my_raii.hpp"
#include "vulkan/vulkan.hpp"

#ifdef _WIN32
	#include <wrl.h>
	#include <dxcapi.h>
	#pragma comment(lib, "dxcompiler.lib")
#else
	#include <dxc/dxcapi.h>
#endif

#ifdef NDEBUG
	#include "shader_registry.hpp"
#endif

#include "dxc_include_handler.hpp"
#include "nriFactory.hpp"
#include "nri.hpp"

namespace nri {

static VkFormat nriFormat2vkFormat[] = {
	VK_FORMAT_UNDEFINED,
	VK_FORMAT_R4G4_UNORM_PACK8,
	VK_FORMAT_R4G4B4A4_UNORM_PACK16,
	VK_FORMAT_B4G4R4A4_UNORM_PACK16,
	VK_FORMAT_R5G6B5_UNORM_PACK16,
	VK_FORMAT_B5G6R5_UNORM_PACK16,
	VK_FORMAT_R5G5B5A1_UNORM_PACK16,
	VK_FORMAT_B5G5R5A1_UNORM_PACK16,
	VK_FORMAT_A1R5G5B5_UNORM_PACK16,
	VK_FORMAT_R8_UNORM,
	VK_FORMAT_R8_SNORM,
	VK_FORMAT_R8_USCALED,
	VK_FORMAT_R8_SSCALED,
	VK_FORMAT_R8_UINT,
	VK_FORMAT_R8_SINT,
	VK_FORMAT_R8_SRGB,
	VK_FORMAT_R8G8_UNORM,
	VK_FORMAT_R8G8_SNORM,
	VK_FORMAT_R8G8_USCALED,
	VK_FORMAT_R8G8_SSCALED,
	VK_FORMAT_R8G8_UINT,
	VK_FORMAT_R8G8_SINT,
	VK_FORMAT_R8G8_SRGB,
	VK_FORMAT_R8G8B8_UNORM,
	VK_FORMAT_R8G8B8_SNORM,
	VK_FORMAT_R8G8B8_USCALED,
	VK_FORMAT_R8G8B8_SSCALED,
	VK_FORMAT_R8G8B8_UINT,
	VK_FORMAT_R8G8B8_SINT,
	VK_FORMAT_R8G8B8_SRGB,
	VK_FORMAT_B8G8R8_UNORM,
	VK_FORMAT_B8G8R8_SNORM,
	VK_FORMAT_B8G8R8_USCALED,
	VK_FORMAT_B8G8R8_SSCALED,
	VK_FORMAT_B8G8R8_UINT,
	VK_FORMAT_B8G8R8_SINT,
	VK_FORMAT_B8G8R8_SRGB,
	VK_FORMAT_R8G8B8A8_UNORM,
	VK_FORMAT_R8G8B8A8_SNORM,
	VK_FORMAT_R8G8B8A8_USCALED,
	VK_FORMAT_R8G8B8A8_SSCALED,
	VK_FORMAT_R8G8B8A8_UINT,
	VK_FORMAT_R8G8B8A8_SINT,
	VK_FORMAT_R8G8B8A8_SRGB,
	VK_FORMAT_B8G8R8A8_UNORM,
	VK_FORMAT_B8G8R8A8_SNORM,
	VK_FORMAT_B8G8R8A8_USCALED,
	VK_FORMAT_B8G8R8A8_SSCALED,
	VK_FORMAT_B8G8R8A8_UINT,
	VK_FORMAT_B8G8R8A8_SINT,
	VK_FORMAT_B8G8R8A8_SRGB,
	VK_FORMAT_A8B8G8R8_UNORM_PACK32,
	VK_FORMAT_A8B8G8R8_SNORM_PACK32,
	VK_FORMAT_A8B8G8R8_USCALED_PACK32,
	VK_FORMAT_A8B8G8R8_SSCALED_PACK32,
	VK_FORMAT_A8B8G8R8_UINT_PACK32,
	VK_FORMAT_A8B8G8R8_SINT_PACK32,
	VK_FORMAT_A8B8G8R8_SRGB_PACK32,
	VK_FORMAT_A2R10G10B10_UNORM_PACK32,
	VK_FORMAT_A2R10G10B10_SNORM_PACK32,
	VK_FORMAT_A2R10G10B10_USCALED_PACK32,
	VK_FORMAT_A2R10G10B10_SSCALED_PACK32,
	VK_FORMAT_A2R10G10B10_UINT_PACK32,
	VK_FORMAT_A2R10G10B10_SINT_PACK32,
	VK_FORMAT_A2B10G10R10_UNORM_PACK32,
	VK_FORMAT_A2B10G10R10_SNORM_PACK32,
	VK_FORMAT_A2B10G10R10_USCALED_PACK32,
	VK_FORMAT_A2B10G10R10_SSCALED_PACK32,
	VK_FORMAT_A2B10G10R10_UINT_PACK32,
	VK_FORMAT_A2B10G10R10_SINT_PACK32,
	VK_FORMAT_R16_UNORM,
	VK_FORMAT_R16_SNORM,
	VK_FORMAT_R16_USCALED,
	VK_FORMAT_R16_SSCALED,
	VK_FORMAT_R16_UINT,
	VK_FORMAT_R16_SINT,
	VK_FORMAT_R16_SFLOAT,
	VK_FORMAT_R16G16_UNORM,
	VK_FORMAT_R16G16_SNORM,
	VK_FORMAT_R16G16_USCALED,
	VK_FORMAT_R16G16_SSCALED,
	VK_FORMAT_R16G16_UINT,
	VK_FORMAT_R16G16_SINT,
	VK_FORMAT_R16G16_SFLOAT,
	VK_FORMAT_R16G16B16_UNORM,
	VK_FORMAT_R16G16B16_SNORM,
	VK_FORMAT_R16G16B16_USCALED,
	VK_FORMAT_R16G16B16_SSCALED,
	VK_FORMAT_R16G16B16_UINT,
	VK_FORMAT_R16G16B16_SINT,
	VK_FORMAT_R16G16B16_SFLOAT,
	VK_FORMAT_R16G16B16A16_UNORM,
	VK_FORMAT_R16G16B16A16_SNORM,
	VK_FORMAT_R16G16B16A16_USCALED,
	VK_FORMAT_R16G16B16A16_SSCALED,
	VK_FORMAT_R16G16B16A16_UINT,
	VK_FORMAT_R16G16B16A16_SINT,
	VK_FORMAT_R16G16B16A16_SFLOAT,
	VK_FORMAT_R32_UINT,
	VK_FORMAT_R32_SINT,
	VK_FORMAT_R32_SFLOAT,
	VK_FORMAT_R32G32_UINT,
	VK_FORMAT_R32G32_SINT,
	VK_FORMAT_R32G32_SFLOAT,
	VK_FORMAT_R32G32B32_UINT,
	VK_FORMAT_R32G32B32_SINT,
	VK_FORMAT_R32G32B32_SFLOAT,
	VK_FORMAT_R32G32B32A32_UINT,
	VK_FORMAT_R32G32B32A32_SINT,
	VK_FORMAT_R32G32B32A32_SFLOAT,
	VK_FORMAT_R64_UINT,
	VK_FORMAT_R64_SINT,
	VK_FORMAT_R64_SFLOAT,
	VK_FORMAT_R64G64_UINT,
	VK_FORMAT_R64G64_SINT,
	VK_FORMAT_R64G64_SFLOAT,
	VK_FORMAT_R64G64B64_UINT,
	VK_FORMAT_R64G64B64_SINT,
	VK_FORMAT_R64G64B64_SFLOAT,
	VK_FORMAT_R64G64B64A64_UINT,
	VK_FORMAT_R64G64B64A64_SINT,
	VK_FORMAT_R64G64B64A64_SFLOAT,
	VK_FORMAT_B10G11R11_UFLOAT_PACK32,
	VK_FORMAT_E5B9G9R9_UFLOAT_PACK32,
	VK_FORMAT_D16_UNORM,
	VK_FORMAT_X8_D24_UNORM_PACK32,
	VK_FORMAT_D32_SFLOAT,
	VK_FORMAT_S8_UINT,
	VK_FORMAT_D16_UNORM_S8_UINT,
	VK_FORMAT_D24_UNORM_S8_UINT,
	VK_FORMAT_D32_SFLOAT_S8_UINT,
};

std::pair<const char *, bool> extensions[] = {
	{"VK_KHR_surface", true},
#ifdef __linux__
	{"VK_KHR_xlib_surface", true}, {"VK_KHR_wayland_surface", false}, {"VK_KHR_xcb_surface", false},
#elif defined(_WIN32)
	{"VK_KHR_win32_surface", true},
#endif
	{"VK_EXT_debug_utils", false},
};

static std::vector<const char *> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
	VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
};

static void printDeviceQueueFamiliesInfo(const vkb::PhysicalDevice &device) {
	uint32_t							   queueFamilyCount = 0;
	std::vector<vk::QueueFamilyProperties> queueFamilies;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	queueFamilies.resize(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
											 (VkQueueFamilyProperties *)queueFamilies.data());

	dbLog(dbg::LOG_DEBUG, "Device has ", queueFamilies.size(), " queue families:");
	int i = 0;
	for (const auto &queueFamily : queueFamilies) {
		dbLog(dbg::LOG_DEBUG, " Queue Family ", i, ":");
		dbLog(dbg::LOG_DEBUG, "  Queue Count: ", queueFamily.queueCount);
		dbLog(dbg::LOG_DEBUG, "  Queue Flags: ");
		if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) { dbLog(dbg::LOG_DEBUG, "   - Graphics"); }
		if (queueFamily.queueFlags & vk::QueueFlagBits::eCompute) { dbLog(dbg::LOG_DEBUG, "   - Compute"); }
		if (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer) { dbLog(dbg::LOG_DEBUG, "   - Transfer"); }
		if (queueFamily.queueFlags & vk::QueueFlagBits::eSparseBinding) {
			dbLog(dbg::LOG_DEBUG, "   - Sparse Binding");
		}
		if (queueFamily.queueFlags & vk::QueueFlagBits::eProtected) { dbLog(dbg::LOG_DEBUG, "   - Protected"); }
		if (queueFamily.queueFlags & vk::QueueFlagBits::eVideoDecodeKHR) { dbLog(dbg::LOG_DEBUG, "   - Video Decode"); }
		if (queueFamily.queueFlags & vk::QueueFlagBits::eVideoEncodeKHR) { dbLog(dbg::LOG_DEBUG, "   - Video Encode"); }
		if (queueFamily.queueFlags & vk::QueueFlagBits::eOpticalFlowNV) { dbLog(dbg::LOG_DEBUG, "   - Optical Flow"); }
		i++;
	}
}

void VulkanNRI::createInstance() {
	vkb::InstanceBuilder builder;
	builder.set_app_name("VulkanNRI");
#ifdef NDEBUG
	builder.request_validation_layers(false);
#else
	builder.request_validation_layers(true);
#endif
	builder.set_engine_name("NoEngine");
	builder.set_engine_version(1);
	builder.require_api_version(1, 2);
	builder.set_minimum_instance_version(1, 2);

	for (const auto &[ext, required] : extensions) {
		builder.enable_extension(ext);
	}

	auto instRet = builder.build();
	if (!instRet) {
		throw std::runtime_error(std::format("Failed to create Vulkan instance: {}", instRet.error().message()));
	}
	instance  = std::move(instRet.value());
	inst_disp = instance.make_table();
}

void VulkanNRI::pickPhysicalDevice() {
	vkb::PhysicalDeviceSelector selector{instance};

	selector.set_minimum_version(1, 2);
	selector.require_present(true);
	selector.defer_surface_initialization();
	selector.allow_any_gpu_device_type(true);

	auto physDevRet = selector.select();
	if (!physDevRet) {
		if (physDevRet.error() == vkb::PhysicalDeviceError::no_suitable_device) {
			const auto &detailed_reasons = physDevRet.detailed_failure_reasons();
			if (!detailed_reasons.empty()) {
				std::cerr << "GPU Selection failure reasons:\n";
				for (const std::string &reason : detailed_reasons) {
					std::cerr << reason << "\n";
				}
			}
		}
		THROW_RUNTIME_ERR(std::format("Failed to select physical device: {}", physDevRet.error().message()));
	}

	bool res	   = false;
	physicalDevice = std::move(physDevRet.value());
	for (const auto &ext : deviceExtensions) {
		res = physicalDevice.enable_extension_if_present(ext);
		if (!res) { THROW_RUNTIME_ERR(std::format("Required device extension {} is not supported", ext)); }
	}
	res = physicalDevice.enable_extension_features_if_present(vk::PhysicalDeviceVulkan12Features()
																  .setBufferDeviceAddress(true)
																  .setShaderSampledImageArrayNonUniformIndexing(true)
																  .setShaderStorageBufferArrayNonUniformIndexing(true)
																  .setShaderStorageImageArrayNonUniformIndexing(true));
	if (!res) { THROW_RUNTIME_ERR("Failed to enable required Vulkan 1.2 features"); }
	res = physicalDevice.enable_extension_features_if_present(
		vk::PhysicalDeviceDynamicRenderingFeatures().setDynamicRendering(true));
	if (!res) { THROW_RUNTIME_ERR("Failed to enable required dynamic rendering features"); }
	dbLog(dbg::LOG_INFO, "Selected physical device: ", physicalDevice.name);
}

void VulkanNRI::createLogicalDevice() {
	vkb::DeviceBuilder builder{physicalDevice};

	auto deviceRet = builder.build();
	if (!deviceRet) {
		THROW_RUNTIME_ERR(std::format("Failed to create logical device: {}", deviceRet.error().message()));
	}
	device	 = std::move(deviceRet.value());
	auto gqi = device.get_queue_index(vkb::QueueType::graphics);
	if (!gqi.has_value()) { THROW_RUNTIME_ERR("Failed to get graphics queue index"); }
	queueFamilyIndices.graphicsFamily = gqi.value();

	disp = device.make_table();
}
}	  // namespace nri
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
namespace nri {
static void prepareGLFW() {
	if (!glfwInit()) { throw std::runtime_error("Failed to initialize GLFW"); }
	if (!glfwVulkanSupported()) { throw std::runtime_error("GLFW: Vulkan not supported"); }

	uint32_t	 count		= 0;
	const char **extensions = glfwGetRequiredInstanceExtensions(&count);
	if (extensions == nullptr) { throw std::runtime_error("GLFW: Failed to get required instance extensions"); }

	dbLog(dbg::LOG_INFO, "GLFW required instance extensions:");
	for (uint32_t i = 0; i < count; i++) {
		dbLog(dbg::LOG_INFO, " ", extensions[i]);
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
}

VulkanNRI::VulkanNRI(CreateBits createBits)
	: NRI(createBits),
	  instance(),
	  physicalDevice(),
	  device(),
	  defaultCommandPool(nullptr),
	  descriptorAllocator(std::nullopt) {
	if (createBits & CreateBits::GLFW) { prepareGLFW(); }

	createInstance();
	pickPhysicalDevice();
	createLogicalDevice();

	auto dcp		   = createCommandPool();
	defaultCommandPool = std::move(static_cast<VulkanCommandPool &>(*dcp));
	descriptorAllocator.emplace(*this);

	vk::PhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &*physicalDeviceProperties);
	dbLog(dbg::LOG_INFO, "VulkanNRI initialized with device: ", physicalDeviceProperties.deviceName);
}

VulkanNRI::~VulkanNRI() {
	vkDeviceWaitIdle(device);
	// VulkanMemoryCache::destroy();
}
VulkanDescriptorAllocator::VulkanDescriptorAllocator(VulkanNRI &nri)
	: nri(nri), pool(nullptr), descriptorSetLayout(nullptr), bigDescriptorSet(nullptr) {
	// query the maximum number of descriptors we can allocate for each type
	vk::PhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(&*nri.getPhysicalDevice(), &*properties);
	auto &limits = properties.limits;

	limits.maxPerStageDescriptorUniformBuffers = std::min(limits.maxPerStageDescriptorUniformBuffers, 500u);
	limits.maxPerStageDescriptorStorageBuffers = std::min(limits.maxPerStageDescriptorStorageBuffers, 500u);
	limits.maxPerStageDescriptorSampledImages  = std::min(limits.maxPerStageDescriptorSampledImages, 500u);
	limits.maxPerStageDescriptorStorageImages  = std::min(limits.maxPerStageDescriptorStorageImages, 500u);

	std::array<vk::DescriptorPoolSize, 4> poolSizes = {
		vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, limits.maxPerStageDescriptorSampledImages),
		vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, limits.maxPerStageDescriptorStorageImages),
		vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, limits.maxPerStageDescriptorUniformBuffers),
		vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, limits.maxPerStageDescriptorStorageBuffers),
	};

	uint32_t totalDescriptors = 0;
	for (const auto &poolSize : poolSizes) {
		totalDescriptors += poolSize.descriptorCount;
	}
	vk::DescriptorPoolCreateInfo poolInfo(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, totalDescriptors,
										  poolSizes.size(), poolSizes.data());

	vk::DescriptorPool descriptorPool;
	vkCreateDescriptorPool(&*nri.getDevice(), &*poolInfo, nullptr, vkc(&descriptorPool)());
	pool = vkraii::DescriptorPool(nri.getDevice().device, descriptorPool);

	std::array<vk::DescriptorSetLayoutBinding, 4> samplerBindings = {
		vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eCombinedImageSampler, poolSizes[0].descriptorCount,
									   vk::ShaderStageFlagBits::eAll, nullptr),
		vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageImage, poolSizes[1].descriptorCount,
									   vk::ShaderStageFlagBits::eAll, nullptr),
		vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eUniformBuffer, poolSizes[2].descriptorCount,
									   vk::ShaderStageFlagBits::eAll, nullptr),
		vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eStorageBuffer, poolSizes[3].descriptorCount,
									   vk::ShaderStageFlagBits::eAll, nullptr),
	};

	vk::DescriptorSetLayoutCreateInfo layoutInfo({}, samplerBindings.size(), samplerBindings.data());

	const vk::DescriptorBindingFlags bindingFlags = {};

	const vk::DescriptorBindingFlags bindingFlagsArr[] = {bindingFlags, bindingFlags, bindingFlags, bindingFlags,
														  bindingFlags};

	vk::DescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo(samplerBindings.size(), bindingFlagsArr);
	layoutInfo.pNext = &bindingFlagsInfo;

	vk::DescriptorSetLayout descriptorSetLayout;
	vkCreateDescriptorSetLayout(&*nri.getDevice(), &*layoutInfo, nullptr, vkc(&descriptorSetLayout)());
	this->descriptorSetLayout = vkraii::DescriptorSetLayout(nri.getDevice().device, descriptorSetLayout);

	vk::DescriptorSetAllocateInfo allocInfo(pool, 1, &descriptorSetLayout);
	vk::DescriptorSet			  descriptorSet;
	vkAllocateDescriptorSets(&*nri.getDevice(), &*allocInfo, vkc(&descriptorSet)());
	bigDescriptorSet = vkraii::DescriptorSet(nri.getDevice().device, pool, descriptorSet);
}

ResourceHandle VulkanDescriptorAllocator::addUniformBufferDescriptor(const VulkanBuffer &buffer) {
	uint32_t descriptorIndex = currentBufferDescriptorIndex++;

	vk::DescriptorBufferInfo bufferInfo(buffer.getBuffer(), 0, VK_WHOLE_SIZE);

	vk::WriteDescriptorSet descriptorWrite(bigDescriptorSet, 2, descriptorIndex, 1, vk::DescriptorType::eUniformBuffer,
										   nullptr, &bufferInfo, nullptr);

	vkUpdateDescriptorSets(&*nri.getDevice(), 1, (VkWriteDescriptorSet *)&descriptorWrite, 0, nullptr);
	dbLog(dbg::LOG_INFO, "Added uniform buffer descriptor at index ", descriptorIndex);

	return ResourceHandle(ResourceType::RESOURCE_TYPE_BUFFER, false, descriptorIndex);
}

ResourceHandle VulkanDescriptorAllocator::addStorageBufferDescriptor(const VulkanBuffer &buffer) {
	uint32_t descriptorIndex = currentBufferDescriptorIndex++;

	dbLog(dbg::LOG_INFO, "Adding storage buffer descriptor at index ", descriptorIndex);
	vk::DescriptorBufferInfo bufferInfo(buffer.getBuffer(), 0, VK_WHOLE_SIZE);

	vk::WriteDescriptorSet descriptorWrite(bigDescriptorSet, 3, descriptorIndex, 1, vk::DescriptorType::eStorageBuffer,
										   nullptr, &bufferInfo, nullptr);

	vkUpdateDescriptorSets(&*nri.getDevice(), 1, (VkWriteDescriptorSet *)&descriptorWrite, 0, nullptr);
	dbLog(dbg::LOG_INFO, "Added storage buffer descriptor at index ", descriptorIndex);

	return ResourceHandle(ResourceType::RESOURCE_TYPE_BUFFER, true, descriptorIndex);
}

ResourceHandle VulkanDescriptorAllocator::addSamplerImageDescriptor(const VulkanTexture2D &image) {
	uint32_t descriptorIndex = currentImageDescriptorIndex++;

	vk::DescriptorImageInfo imageInfo(*(image.getSampler()), *(image.get()), vk::ImageLayout::eShaderReadOnlyOptimal);

	vk::WriteDescriptorSet descriptorWrite(bigDescriptorSet, 0, descriptorIndex, 1,
										   vk::DescriptorType::eCombinedImageSampler, &imageInfo, nullptr, nullptr);

	vkUpdateDescriptorSets(&*nri.getDevice(), 1, (VkWriteDescriptorSet *)&descriptorWrite, 0, nullptr);
	dbLog(dbg::LOG_INFO, "Added sampler image descriptor at index ", descriptorIndex);

	return ResourceHandle(ResourceType::RESOURCE_TYPE_IMAGE_SAMPLER, false, descriptorIndex);
}

ResourceHandle VulkanDescriptorAllocator::addStorageImageDescriptor(const VulkanStorageImage2D &image) {
	uint32_t descriptorIndex = currentImageDescriptorIndex++;

	vk::DescriptorImageInfo imageInfo(nullptr, *(image.get()), vk::ImageLayout::eGeneral);

	vk::WriteDescriptorSet descriptorWrite(bigDescriptorSet, 1, descriptorIndex, 1, vk::DescriptorType::eStorageImage,
										   &imageInfo, nullptr, nullptr);

	vkUpdateDescriptorSets(&*nri.getDevice(), 1, (VkWriteDescriptorSet *)&descriptorWrite, 0, nullptr);
	dbLog(dbg::LOG_INFO, "Added storage image descriptor at index ", descriptorIndex);

	return ResourceHandle(ResourceType::RESOURCE_TYPE_STORAGE_IMAGE, true, descriptorIndex);
}

vk::MemoryPropertyFlags typeRequest2vkMemoryProperty[] = {
	vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
	vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
	vk::MemoryPropertyFlagBits::eDeviceLocal};

VulkanAllocation::VulkanAllocation(VulkanNRI &nri, MemoryRequirements memoryRequirements)
	: memory(nullptr), device(nri.getDevice()), size(memoryRequirements.size) {
	assert(memoryRequirements.typeRequest >= 0);
	assert(memoryRequirements.typeRequest < MemoryTypeRequest::_MEMORY_TYPE_NUM);
	vk::MemoryPropertyFlags properties = typeRequest2vkMemoryProperty[(uint32_t)memoryRequirements.typeRequest];

	vk::PhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(nri.getPhysicalDevice(), &*memProperties);

	uint32_t memoryTypeIndex = -1;
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			memoryTypeIndex = i;
			break;
		}
	}

	if (memoryTypeIndex == -1u) { throw std::runtime_error("Failed to find suitable memory type!"); }

	vk::MemoryAllocateFlagsInfo allocFlagsInfo(vk::MemoryAllocateFlagBits::eDeviceAddress, {});
	vk::MemoryAllocateInfo		allocInfo(memoryRequirements.size, memoryTypeIndex, &allocFlagsInfo);

	vk::DeviceMemory allocatedMemory;
	vkAllocateMemory(nri.getDevice(), &*allocInfo, nullptr, vkc(&allocatedMemory)());
	memory = vkraii::DeviceMemory(nri.getDevice().device, allocatedMemory);
}

void *VulkanAllocation::map() {
	assert(memory != nullptr);
	void *data = nullptr;
	vkMapMemory(device, memory, 0, size, 0, &data);
	if (data == nullptr) {
		dbLog(dbg::LOG_ERROR, "Failed to map memory!");
		throw std::runtime_error("Failed to map memory!");
	}
	return data;
}

void VulkanAllocation::unmap() {
	assert(memory != nullptr);
	vkUnmapMemory(device, memory);
}

ResourceHandle VulkanBuffer::createHandle() const {
	return nri->getDescriptorAllocator().addStorageBufferDescriptor(*this);
}

VulkanBuffer::VulkanBuffer(VulkanNRI &nri, std::size_t size, BufferUsage usage)
	: nri(&nri), buffer(nullptr), allocation(nullptr), offset(0), size(size) {
	vk::BufferUsageFlags bufferUsageFlags;
	if (usage & BufferUsage::BUFFER_USAGE_VERTEX)
		bufferUsageFlags |= vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress;
	if (usage & BufferUsage::BUFFER_USAGE_INDEX)
		bufferUsageFlags |= vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress;
	if (usage & BufferUsage::BUFFER_USAGE_UNIFORM) bufferUsageFlags |= vk::BufferUsageFlagBits::eUniformBuffer;
	if (usage & BufferUsage::BUFFER_USAGE_STORAGE)
		bufferUsageFlags |= vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress;
	if (usage & BufferUsage::BUFFER_USAGE_TRANSFER_SRC) bufferUsageFlags |= vk::BufferUsageFlagBits::eTransferSrc;
	if (usage & BufferUsage::BUFFER_USAGE_TRANSFER_DST) bufferUsageFlags |= vk::BufferUsageFlagBits::eTransferDst;
	if (usage & BufferUsage::BUFFER_USAGE_ACCELERATION_STRUCTURE)
		bufferUsageFlags |=
			vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress;
	if (usage & BufferUsage::BUFFER_USAGE_SHADER_BINDING_TABLE)
		bufferUsageFlags |=
			vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress;

	vk::BufferCreateInfo bufferInfo({}, size, bufferUsageFlags, vk::SharingMode::eExclusive);

	vk::Buffer createdBuffer;
	vkCreateBuffer(nri.getDevice(), &*bufferInfo, nullptr, vkc(&createdBuffer)());
	buffer = vkraii::Buffer(nri.getDevice().device, createdBuffer);
}

MemoryRequirements VulkanBuffer::getMemoryRequirements() {
	vk::MemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(nri->getDevice(), buffer, &*memRequirements);

	return MemoryRequirements(memRequirements.size, MemoryTypeRequest::MEMORY_TYPE_DEVICE, memRequirements.alignment);
}

void VulkanBuffer::bindMemory(Allocation &allocation, std::size_t offset) {
	VulkanAllocation &vulkanAllocation = static_cast<VulkanAllocation &>(allocation);
	this->offset					   = offset;

	vkBindBufferMemory(nri->getDevice(), buffer, vulkanAllocation.getMemory(), offset);
	this->allocation = &vulkanAllocation;
}
void *VulkanBuffer::map(std::size_t offset, std::size_t size) {
	assert(allocation != nullptr);
	void *data = nullptr;
	vkMapMemory(allocation->getDevice(), allocation->getMemory(), offset, size, 0, &data);
	if (data == nullptr) { throw std::runtime_error("Failed to map buffer memory."); }
	return data;
}

void VulkanBuffer::unmap() {
	assert(allocation != nullptr);

	// vk::MappedMemoryRange memoryRange(allocation->getMemory(), offset, VK_WHOLE_SIZE);
	// auto				  res = allocation->getDevice().flushMappedMemoryRanges(1, &memoryRange);
	// assert(res == vk::Result::eSuccess);

	vkUnmapMemory(allocation->getDevice(), allocation->getMemory());
}

std::size_t VulkanBuffer::getSize() const { return this->size; }

std::size_t VulkanBuffer::getOffset() const { return this->offset; }

void VulkanBuffer::copyFrom(CommandBuffer &commandBuffer, Buffer &srcBuffer, std::size_t srcOffset,
							std::size_t dstOffset, std::size_t size) {
	auto &vkCmdBuf = static_cast<VulkanCommandBuffer &>(commandBuffer);
	auto &vkSrcBuf = static_cast<VulkanBuffer &>(srcBuffer);

	if (!vkCmdBuf.isRecording) vkCmdBuf.begin();

	// TODO: put the right barriers here
	// vk::BufferMemoryBarrier bufferBarrier(vk::AccessFlagBits::eHostWrite,
	//									  vk::AccessFlagBits::eTransferRead, VK_QUEUE_FAMILY_IGNORED,
	//									  VK_QUEUE_FAMILY_IGNORED, vkSrcBuf.buffer, srcOffset, size);
	// vkCmdBuf.commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eHost,
	//									   vk::PipelineStageFlagBits::eTransfer, {}, {}, {bufferBarrier}, {});

	vk::BufferCopy copyRegion(srcOffset, dstOffset, size);
	vkCmdCopyBuffer(vkCmdBuf.commandBuffer, vkSrcBuf.buffer, this->buffer, 1, vkc(&copyRegion)());

	vk::BufferMemoryBarrier bufferBarrier2(vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eMemoryRead,
										   VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, this->buffer, dstOffset,
										   size);

	vkCmdPipelineBarrier(vkCmdBuf.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, {},
						 0, nullptr, 1, vkc(&bufferBarrier2)(), 0, nullptr);
}

void VulkanBuffer::bindAsVertexBuffer(CommandBuffer &commandBuffer, uint32_t binding, std::size_t offset,
									  std::size_t stride) {
	auto &vkCmdBuf = static_cast<VulkanCommandBuffer &>(commandBuffer);
	static_cast<void>(stride);

	// vkCmdBuf.commandBuffer.bindVertexBuffers(binding, vkBuffer, vk::DeviceSize(offset));
	vkCmdBindVertexBuffers(vkCmdBuf.commandBuffer, binding, 1, vkc(&*buffer)(), &offset);
}

vk::IndexType nriIndexType2vkIndexType[] = {
	vk::IndexType::eUint16,
	vk::IndexType::eUint32,
};

void VulkanBuffer::bindAsIndexBuffer(CommandBuffer &commandBuffer, std::size_t offset, IndexType indexType) {
	auto		 &vkCmdBuf	  = static_cast<VulkanCommandBuffer &>(commandBuffer);
	vk::Buffer	  vkBuffer	  = this->buffer;
	vk::IndexType vkIndexType = nriIndexType2vkIndexType[static_cast<int>(indexType)];
	assert(int(vkIndexType) != -1);
	vkCmdBindIndexBuffer(vkCmdBuf.commandBuffer, vkBuffer, offset, (VkIndexType)vkIndexType);
}

vk::DeviceAddress VulkanBuffer::getAddress() {
	vk::BufferDeviceAddressInfo addressInfo;
	addressInfo.setBuffer(buffer);
	return vkGetBufferDeviceAddress(nri->getDevice(), &*addressInfo);
}

VulkanImageView::VulkanImageView(VulkanNRI &nri, vkraii::ImageView &&imgView, vk::Format fmt)
	: ImageView(), nri(&nri), imageView(std::move(imgView)), format(fmt) {}

VulkanRenderTarget::VulkanRenderTarget(VulkanNRI &nri, VulkanImage2D &image2D)
	: VulkanImageView(nri, nullptr, image2D.getFormat()) {
	vk::ImageViewCreateInfo imageViewInfo(
		{}, image2D.get(), vk::ImageViewType::e2D, vk::Format(this->format),
		vk::ComponentMapping(vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity,
							 vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity),
		vk::ImageSubresourceRange(image2D.getAspectFlags(), 0, 1, 0, 1));
	vk::ImageView createdImageView;
	vkCreateImageView(nri.getDevice(), &*imageViewInfo, nullptr, vkc(&createdImageView)());
	imageView = vkraii::ImageView(nri.getDevice().device, createdImageView);
}

ResourceHandle VulkanRenderTarget::createHandle() const {
	dbLog(dbg::LOG_ERROR, "VulkanRenderTarget::createHandle not implemented!");
	return ResourceHandle::INVALID_HANDLE;
}

VulkanTexture2D::VulkanTexture2D(VulkanNRI &nri, VulkanImage2D &image2D)
	: VulkanImageView(nri, nullptr, image2D.getFormat()), sampler(nullptr) {
	vk::ImageViewCreateInfo imageViewInfo(
		{}, image2D.get(), vk::ImageViewType::e2D, vk::Format(this->format),
		vk::ComponentMapping(vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity,
							 vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity),
		vk::ImageSubresourceRange(image2D.getAspectFlags(), 0, 1, 0, 1));
	vk::ImageView createdImageView;
	vkCreateImageView(nri.getDevice(), &*imageViewInfo, nullptr, vkc(&createdImageView)());
	imageView = vkraii::ImageView(nri.getDevice().device, createdImageView);

	vk::SamplerCreateInfo samplerInfo({}, vk::Filter::eNearest, vk::Filter::eNearest, vk::SamplerMipmapMode::eNearest,
									  vk::SamplerAddressMode::eClampToEdge, vk::SamplerAddressMode::eClampToEdge,
									  vk::SamplerAddressMode::eClampToEdge, 0.0f, VK_FALSE, 1.0f, VK_FALSE,
									  vk::CompareOp::eAlways, 0.0f, 0.0f, vk::BorderColor::eFloatTransparentBlack,
									  VK_FALSE);
	vk::Sampler			  createdSampler;
	vkCreateSampler(nri.getDevice(), &*samplerInfo, nullptr, vkc(&createdSampler)());
	sampler = vkraii::Sampler(nri.getDevice().device, createdSampler);
}

ResourceHandle VulkanTexture2D::createHandle() const {
	return nri->getDescriptorAllocator().addSamplerImageDescriptor(*this);
}

VulkanStorageImage2D::VulkanStorageImage2D(VulkanNRI &nri, VulkanImage2D &image2D)
	: VulkanImageView(nri, nullptr, image2D.getFormat()) {
	vk::ImageViewCreateInfo imageViewInfo(
		{}, image2D.get(), vk::ImageViewType::e2D, vk::Format(this->format),
		vk::ComponentMapping(vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity,
							 vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity),
		vk::ImageSubresourceRange(image2D.getAspectFlags(), 0, 1, 0, 1));
	vk::ImageView createdImageView;
	vkCreateImageView(nri.getDevice(), &*imageViewInfo, nullptr, vkc(&createdImageView)());
	imageView = vkraii::ImageView(nri.getDevice().device, createdImageView);
}

ResourceHandle VulkanStorageImage2D::createHandle() const {
	return nri->getDescriptorAllocator().addStorageImageDescriptor(*this);
}

MemoryRequirements &MemoryRequirements::setTypeRequest(MemoryTypeRequest tr) {
	typeRequest = tr;
	return *this;
}

MemoryRequirements &MemoryRequirements::setAlignment(std::size_t a) {
	alignment = a;
	return *this;
}

std::unique_ptr<Image2D> VulkanNRI::createImage2D(uint32_t width, uint32_t height, Format format, ImageUsage usage) {
	return std::make_unique<VulkanImage2D>(*this, width, height, format, usage);
}

std::unique_ptr<Buffer> VulkanNRI::createBuffer(std::size_t size, BufferUsage usage) {
	return std::make_unique<VulkanBuffer>(*this, size, usage);
}

MemoryRequirements VulkanImage2D::getMemoryRequirements() {
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(*device, image.get(), &memRequirements);

	return MemoryRequirements(memRequirements.size, MemoryTypeRequest::MEMORY_TYPE_DEVICE, memRequirements.alignment);
}

void VulkanImage2D::bindMemory(Allocation &allocation, std::size_t offset) {
	VulkanAllocation &vulkanAllocation = static_cast<VulkanAllocation &>(allocation);

	vkBindImageMemory(*device, image.get(), vulkanAllocation.getMemory(), offset);
}

void VulkanImage2D::clear(CommandBuffer &commandBuffer, glm::vec4 color) {
	auto &vkBuf = static_cast<VulkanCommandBuffer &>(commandBuffer);

	vk::ClearColorValue v{color.r, color.g, color.b, color.a};
	vkBuf.begin();

	transitionLayout(vkBuf, vk::ImageLayout::eTransferDstOptimal, vk::AccessFlagBits::eNone,
					 vk::AccessFlagBits::eTransferWrite, vk::PipelineStageFlagBits::eTopOfPipe,
					 vk::PipelineStageFlagBits::eTransfer);

	// vkBuf.commandBuffer.clearColorImage(image.get(), vk::ImageLayout::eTransferDstOptimal, v,
	//									{vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)});

	vk::ImageSubresourceRange range(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
	vkCmdClearColorImage(vkBuf.commandBuffer, image.get(), (VkImageLayout)vk::ImageLayout::eTransferDstOptimal,
						 (VkClearColorValue *)&v, 1, vkc(&range)());
}

void VulkanImage2D::prepareForPresent(CommandBuffer &commandBuffer) {
	auto &vkBuf = static_cast<VulkanCommandBuffer &>(commandBuffer);

	transitionLayout(vkBuf, vk::ImageLayout::ePresentSrcKHR,
					 vk::AccessFlagBits::eTransferWrite | vk::AccessFlagBits::eColorAttachmentWrite,
					 vk::AccessFlagBits::eMemoryRead,
					 vk::PipelineStageFlagBits::eTransfer | vk::PipelineStageFlagBits::eColorAttachmentOutput,
					 vk::PipelineStageFlagBits::eBottomOfPipe);
}
void VulkanImage2D::prepareForStorage(CommandBuffer &commandBuffer) {
	auto &vkBuf = static_cast<VulkanCommandBuffer &>(commandBuffer);

	transitionLayout(vkBuf, vk::ImageLayout::eGeneral,
					 vk::AccessFlagBits::eTransferWrite | vk::AccessFlagBits::eColorAttachmentWrite,
					 vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite,
					 vk::PipelineStageFlagBits::eTransfer | vk::PipelineStageFlagBits::eColorAttachmentOutput,
					 vk::PipelineStageFlagBits::eComputeShader | vk::PipelineStageFlagBits::eFragmentShader |
						 vk::PipelineStageFlagBits::eVertexShader);
}

void VulkanImage2D::prepareForTexture(CommandBuffer &commandBuffer) {
	auto &vkBuf = static_cast<VulkanCommandBuffer &>(commandBuffer);

	transitionLayout(vkBuf, vk::ImageLayout::eShaderReadOnlyOptimal,
					 vk::AccessFlagBits::eTransferWrite | vk::AccessFlagBits::eColorAttachmentWrite,
					 vk::AccessFlagBits::eShaderRead,
					 vk::PipelineStageFlagBits::eTransfer | vk::PipelineStageFlagBits::eColorAttachmentOutput,
					 vk::PipelineStageFlagBits::eFragmentShader | vk::PipelineStageFlagBits::eVertexShader |
						 vk::PipelineStageFlagBits::eComputeShader);
}

void VulkanImage2D::copyFrom(CommandBuffer &commandBuffer, Buffer &srcBuffer, std::size_t srcOffset,
							 uint32_t srcRowPitch) {
	auto &vkCmdBuf = static_cast<VulkanCommandBuffer &>(commandBuffer);
	auto &vkSrcBuf = static_cast<VulkanBuffer &>(srcBuffer);
	if (!vkCmdBuf.isRecording) vkCmdBuf.begin();
	transitionLayout(vkCmdBuf, vk::ImageLayout::eTransferDstOptimal, vk::AccessFlagBits::eNone,
					 vk::AccessFlagBits::eTransferWrite, vk::PipelineStageFlagBits::eTopOfPipe,
					 vk::PipelineStageFlagBits::eTransfer);
	vk::BufferImageCopy region(srcOffset, srcRowPitch, 0, vk::ImageSubresourceLayers(aspectFlags, 0, 0, 1),
							   vk::Offset3D(0, 0, 0), vk::Extent3D(width, height, 1));
	vkCmdCopyBufferToImage(vkCmdBuf.commandBuffer, vkSrcBuf.getBuffer(), image.get(),
						   (VkImageLayout)vk::ImageLayout::eTransferDstOptimal, 1, vkc(&region)());
}

vk::ImageAspectFlags VulkanImage2D::getAspectFlags(vk::Format format) {
	switch (format) {
		case vk::Format::eUndefined: throw std::runtime_error("Undefined format has no aspect flags.");
		default: break;
		case vk::Format::eD32SfloatS8Uint:
		case vk::Format::eD24UnormS8Uint:
		case vk::Format::eD16UnormS8Uint: return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil; ;
		case vk::Format::eD32Sfloat:
		case vk::Format::eD16Unorm: return vk::ImageAspectFlagBits::eDepth;
	}
	return vk::ImageAspectFlagBits::eColor;
}

void VulkanImage2D::transitionLayout(CommandBuffer &commandBuffer, vk::ImageLayout newLayout, vk::AccessFlags srcAccess,
									 vk::AccessFlags dstAccess, vk::PipelineStageFlags srcStage,
									 vk::PipelineStageFlags dstStage) {
	auto &vkBuf = static_cast<VulkanCommandBuffer &>(commandBuffer);

	vk::ImageMemoryBarrier barrier(srcAccess, dstAccess, layout, newLayout, vk::QueueFamilyIgnored,
								   vk::QueueFamilyIgnored, image.get(), {aspectFlags, 0, 1, 0, 1});
	vkBuf.begin();
	vkCmdPipelineBarrier(vkBuf.commandBuffer, (VkPipelineStageFlags)srcStage, (VkPipelineStageFlags)dstStage, 0, 0,
						 nullptr, 0, nullptr, 1, vkc(&barrier)());
}

VulkanImage2D::VulkanImage2D(VulkanNRI &nri, uint32_t width, uint32_t height, Format format, ImageUsage usage)
	: nri(&nri),
	  image(nullptr),
	  device(&nri.getDevice()),
	  layout(vk::ImageLayout::eUndefined),
	  format((vk::Format)nriFormat2vkFormat[static_cast<int>(format)]),
	  aspectFlags(getAspectFlags(this->format)),
	  width(width),
	  height(height) {
	assert(format != Format::FORMAT_UNDEFINED);
	assert(format < Format::_FORMAT_NUM);
	assert(this->format != vk::Format::eUndefined);
	assert(int(this->format) != -1);
	assert(width > 0 && height > 0);
	vk::Format vkFormat = (vk::Format)nriFormat2vkFormat[static_cast<int>(format)];

	vk::ImageCreateInfo imageInfo({}, vk::ImageType::e2D, vk::Format(vkFormat),
								  vk::Extent3D(static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1), 1, 1,
								  vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
								  static_cast<vk::ImageUsageFlagBits>(usage), vk::SharingMode::eExclusive, 0, nullptr,
								  vk::ImageLayout::eUndefined);

	vk::Image img = nullptr;
	vkCreateImage(nri.getDevice(), &*imageInfo, nullptr, vkc(&img)());
	this->image = vkraii::Image((vk::Device)*device, img);
}

std::unique_ptr<ImageView> VulkanImage2D::createRenderTargetView() {
	return std::make_unique<VulkanRenderTarget>(*nri, *this);
}

std::unique_ptr<ImageView> VulkanImage2D::createTextureView() { return std::make_unique<VulkanTexture2D>(*nri, *this); }

std::unique_ptr<ImageView> VulkanImage2D::createStorageView() {
	return std::make_unique<VulkanStorageImage2D>(*nri, *this);
}

std::unique_ptr<CommandPool> VulkanNRI::createCommandPool() {
	vk::CommandPoolCreateInfo poolCI(vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
									 queueFamilyIndices.graphicsFamily.value());

	vk::CommandPool pool = nullptr;
	vkCreateCommandPool(device, &*poolCI, nullptr, vkc(&pool)());

	return std::make_unique<VulkanCommandPool>(vkraii::CommandPool(device.device, pool));
}

std::unique_ptr<ProgramBuilder> VulkanNRI::createProgramBuilder() {
	return std::make_unique<VulkanProgramBuilder>(*this);
}

std::unique_ptr<CommandQueue> VulkanNRI::createCommandQueue() {
	vk::Queue queue = nullptr;
	vkGetDeviceQueue(device, queueFamilyIndices.graphicsFamily.value(), 0, vkc(&queue)());

	return std::make_unique<VulkanCommandQueue>(vkraii::Queue(device.device, queue));
};

VulkanWindow::VulkanWindow(VulkanNRI &nri)
	: Window(nri),
	  surface(nullptr),
	  swapChain(),
	  presentQueue(nullptr),
	  imageAvailableSemaphore(nullptr),
	  renderFinishedSemaphore(nullptr),
	  inFlightFence(nullptr) {
	vk::Semaphore			semaphore = nullptr;
	vk::SemaphoreCreateInfo semaphoreInfo;
	vkCreateSemaphore(nri.getDevice(), vkc(&semaphoreInfo)(), nullptr, vkc(&semaphore)());
	imageAvailableSemaphore = vkraii::Semaphore(nri.getDevice().device, semaphore);

	vkCreateSemaphore(nri.getDevice(), vkc(&semaphoreInfo)(), nullptr, vkc(&semaphore)());
	renderFinishedSemaphore = vkraii::Semaphore(nri.getDevice().device, semaphore);

	vk::FenceCreateInfo fenceInfo(vk::FenceCreateFlagBits::eSignaled);
	vk::Fence			fence = nullptr;
	vkCreateFence(nri.getDevice(), vkc(&fenceInfo)(), nullptr, vkc(&fence)());
	inFlightFence = vkraii::Fence(nri.getDevice().device, fence);

	if (imageAvailableSemaphore == nullptr || renderFinishedSemaphore == nullptr || inFlightFence == nullptr) {
		throw std::runtime_error("Failed to create synchronization objects for a frame!");
	}
	commandBuffer = std::unique_ptr<VulkanCommandBuffer>(
		(VulkanCommandBuffer *)nri.createCommandBuffer(nri.getDefaultCommandPool()).release());
}

VulkanWindow::~VulkanWindow() {
	if (swapChain) vkb::destroy_swapchain(swapChain);
}

static std::pair<vk::Format, vk::ColorSpaceKHR> chooseSurfaceFormat(
	const std::vector<vk::SurfaceFormatKHR> &availableFormats) {
	// if the surface format list only includes one entry with eUndefined,
	// there is no preferred format, so we assume eB8G8R8A8Unorm
	if (availableFormats.size() == 1 && availableFormats[0].format == vk::Format::eUndefined) {
		return {vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear};
	}

	// check for preferred format
	for (const auto &availableFormat : availableFormats) {
		if (availableFormat.format == vk::Format::eB8G8R8A8Unorm &&
			availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
			return {availableFormat.format, availableFormat.colorSpace};
		}
	}

	// otherwise just pick the first available
	return {availableFormats[0].format, availableFormats[0].colorSpace};
}

void VulkanWindow::setSurface(vkraii::SurfaceKHR &&surface) {
	auto &nri = static_cast<VulkanNRI &>(this->nri);

	this->surface = std::move(surface);

	std::vector<vk::SurfaceFormatKHR> surfaceformats;
	uint32_t						  surfaceFormatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(nri.getPhysicalDevice(), *this->surface, &surfaceFormatCount, nullptr);
	surfaceformats.resize(surfaceFormatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(nri.getPhysicalDevice(), *this->surface, &surfaceFormatCount,
										 (VkSurfaceFormatKHR *)surfaceformats.data());

	if (surfaceformats.empty()) { throw std::runtime_error("Failed to get Vulkan surface formats!"); }
	std::tie(surfaceFormat, surfaceColorSpace) = chooseSurfaceFormat(surfaceformats);
	dbLog(dbg::LOG_INFO, "Chosen surface format: ", vk::to_string(surfaceFormat),
		  ", color space: ", vk::to_string(surfaceColorSpace));
}

void VulkanWindow::createSwapChain(uint32_t &width, uint32_t &height) {
	dbLog(dbg::LOG_INFO, "Creating swapchain for window with size ", width, "x", height);
	this->width	 = width;
	this->height = height;
	auto &nri	 = static_cast<VulkanNRI &>(this->nri);
	if (this->surface == nullptr) { THROW_RUNTIME_ERR("surface is not set for VulkanWindow!"); }

	vk::SurfaceCapabilitiesKHR capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(nri.getPhysicalDevice(), *surface,
											  (VkSurfaceCapabilitiesKHR *)&capabilities);
	width  = std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	height = std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

	vkb::SwapchainBuilder swapchainBuilder{nri.getPhysicalDevice(), nri.getDevice(), *surface};
	swapchainBuilder.set_desired_present_mode((VkPresentModeKHR)vk::PresentModeKHR::eFifo);
	swapchainBuilder.set_desired_format((VkSurfaceFormatKHR)vk::SurfaceFormatKHR(surfaceFormat, surfaceColorSpace));
	swapchainBuilder.set_desired_extent(width, height);
	swapchainBuilder.set_desired_min_image_count(capabilities.minImageCount + 1);
	swapchainBuilder.set_required_min_image_count(capabilities.minImageCount);
	swapchainBuilder.set_old_swapchain(swapChain);
	swapchainBuilder.set_image_usage_flags(
		VkImageUsageFlags(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst));
	swapchainBuilder.set_pre_transform_flags(VkSurfaceTransformFlagBitsKHR(vk::SurfaceTransformFlagBitsKHR::eIdentity));

	auto swapchainResult = swapchainBuilder.build();
	if (!swapchainResult.has_value()) {
		THROW_RUNTIME_ERR(std::format("Failed to create swapchain: {}", swapchainResult.error().message()));
	}
	swapChain = std::move(swapchainResult.value());

	this->swapChainImages.clear();
	std::vector<vk::Image> swapChainImages;
	uint32_t			   imageCount;
	vkGetSwapchainImagesKHR(nri.getDevice(), swapChain, &imageCount, nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(nri.getDevice(), swapChain, &imageCount, (VkImage *)swapChainImages.data());

	for (const auto &image : swapChainImages) {
		VulkanImage2D nriImage = VulkanImage2D(nri, image, vk::ImageLayout::eUndefined, vk::Format::eB8G8R8A8Unorm,
											   nri.getDevice(), width, height);

		VulkanRenderTarget renderTarget = VulkanRenderTarget(nri, nriImage);

		this->swapChainImages.emplace_back(std::move(nriImage), std::move(renderTarget));
		this->swapChainImages.back().image.transitionLayout(
			*commandBuffer, vk::ImageLayout::ePresentSrcKHR, vk::AccessFlagBits::eNone, vk::AccessFlagBits::eMemoryRead,
			vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eBottomOfPipe);
	}
}

void VulkanWindow::beginFrame() {
	auto &nri = static_cast<const VulkanNRI &>(this->nri);

	vk::Result result =
		(vk::Result)vkWaitForFences(nri.getDevice(), 1, (VkFence *)&*inFlightFence, VK_TRUE, UINT64_MAX);
	assert(result == vk::Result::eSuccess);
	// nri.getDevice().resetFences({inFlightFence});
	result = (vk::Result)vkResetFences(nri.getDevice(), 1, (VkFence *)&*inFlightFence);
	assert(result == vk::Result::eSuccess);
	// auto imageIndex			= swapChain.acquireNextImage(UINT64_MAX, *imageAvailableSemaphore, nullptr);
	uint32_t imageIndex;
	result = (vk::Result)vkAcquireNextImageKHR(nri.getDevice(), swapChain, UINT64_MAX, *imageAvailableSemaphore,
											   nullptr, &imageIndex);
	assert(result == vk::Result::eSuccess || result == vk::Result::eSuboptimalKHR);
	this->currentImageIndex = imageIndex;

	swapChainImages[imageIndex].image.transitionLayout(
		*commandBuffer, vk::ImageLayout::eColorAttachmentOptimal, vk::AccessFlagBits::eMemoryRead,
		vk::AccessFlagBits::eColorAttachmentWrite, vk::PipelineStageFlagBits::eBottomOfPipe,
		vk::PipelineStageFlagBits::eColorAttachmentOutput);
}

ImageAndViewRef VulkanWindow::getCurrentRenderTarget() {
	auto &sci = swapChainImages[currentImageIndex];
	return ImageAndViewRef(sci.image, sci.view);
}
CommandBuffer &VulkanWindow::getCurrentCommandBuffer() { return *commandBuffer; }

void VulkanWindow::endFrame() {
	getCurrentRenderTarget().image.prepareForPresent(*commandBuffer);

	commandBuffer->end();
	auto				  &nri	  = static_cast<const VulkanNRI &>(this->nri);
	vk::PipelineStageFlags stages = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
	vk::SubmitInfo		   submitInfo(1, &(*imageAvailableSemaphore), &stages, 1, &*commandBuffer->commandBuffer, 1,
									  &(*renderFinishedSemaphore));
	vkQueueSubmit(*presentQueue.queue, 1, &*submitInfo, *inFlightFence);

	vk::PresentInfoKHR presentInfo = vk::PresentInfoKHR(
		1, &*renderFinishedSemaphore, 1, (vk::SwapchainKHR *)&swapChain.swapchain, &currentImageIndex, nullptr);

	vk::Result res = (vk::Result)vkQueuePresentKHR(*presentQueue.queue, &*presentInfo);
	switch (res) {
		case vk::Result::eSuccess: break;
		case vk::Result::eErrorOutOfDateKHR:
		case vk::Result::eSuboptimalKHR:
			vkDeviceWaitIdle(nri.getDevice());
			uint32_t width, height;
			createSwapChain(width, height);
			break;
		case vk::Result::eErrorSurfaceLostKHR: break;
		default: assert(res == vk::Result::eSuccess);
	}

	vkQueueWaitIdle(*presentQueue.queue);
}

void VulkanWindow::beginRendering(CommandBuffer &cmdBuf, const ImageAndViewRef &renderTarget) {
	auto *rtp = dynamic_cast<const VulkanRenderTarget *>(&renderTarget.view);
	assert(rtp != nullptr);
	auto &rt	   = *rtp;
	auto &img	   = static_cast<const VulkanImage2D &>(renderTarget.image);
	auto &vkCmdBuf = static_cast<VulkanCommandBuffer &>(cmdBuf);
	auto &vknri	   = static_cast<const VulkanNRI &>(this->nri);

	vk::RenderingInfo renderingInfo{};
	renderingInfo.sType		 = vk::StructureType::eRenderingInfo;
	renderingInfo.renderArea = vk::Rect2D({0, 0}, {img.getWidth(), img.getHeight()});
	vk::RenderingAttachmentInfo colorAttachment{};
	colorAttachment.sType		= vk::StructureType::eRenderingAttachmentInfo;
	colorAttachment.imageView	= *rt.get();
	colorAttachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
	colorAttachment.loadOp		= vk::AttachmentLoadOp::eClear;
	colorAttachment.storeOp		= vk::AttachmentStoreOp::eStore;
	vk::ClearValue clearValue;
	clearValue.color.setFloat32({this->clearColor.r, this->clearColor.g, this->clearColor.b, this->clearColor.a});
	colorAttachment.clearValue		   = clearValue;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments	   = &colorAttachment;
	renderingInfo.layerCount		   = 1;

	vknri.getDispatchTable().cmdBeginRenderingKHR(vkCmdBuf.commandBuffer, vkc(&renderingInfo)());

	vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(img.getWidth()), static_cast<float>(img.getHeight()), 0.0f,
						  1.0f);
	vkCmdSetViewport(vkCmdBuf.commandBuffer, 0, 1, vkc(&viewport)());

	vk::Rect2D rect({0, 0}, {img.getWidth(), img.getHeight()});
	vkCmdSetScissor(vkCmdBuf.commandBuffer, 0, 1, vkc(&rect)());
}

void VulkanWindow::endRendering(CommandBuffer &cmdBuf) {
	auto &vknri = static_cast<const VulkanNRI &>(this->nri);
	vknri.getDispatchTable().cmdEndRenderingKHR(static_cast<VulkanCommandBuffer &>(cmdBuf).commandBuffer);
}

vk::PrimitiveTopology nriPrimitiveType2vkTopology[] = {
	vk::PrimitiveTopology::eTriangleList, vk::PrimitiveTopology::eTriangleStrip, vk::PrimitiveTopology::eLineList,
	vk::PrimitiveTopology::eLineStrip,	  vk::PrimitiveTopology::ePointList,
};

// TODO: merge with DX12 implementation
std::pair<std::vector<vkraii::ShaderModule>, std::vector<vk::PipelineShaderStageCreateInfo>> VulkanProgramBuilder::
	createShaderModules(std::vector<ShaderCreateInfo> &&stagesInfo, const vkb::Device &device) {
	std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
	std::vector<vkraii::ShaderModule>			   shaderModules;

	CComPtr<IDxcCompiler3> compiler;
#ifndef NDEBUG
	HRESULT hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
#endif
	assert(SUCCEEDED(hr) && "Failed to create DX Compiler.");

	CComPtr<IDxcUtils> utils;
#ifndef NDEBUG
	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
#endif
	assert(SUCCEEDED(hr) && "Failed to create DX Utils.");

	auto includeHandler = std::make_unique<CustomIncludeHandler>(utils);

	for (const auto &stageInfo : stagesInfo) {
		vk::ShaderStageFlagBits stage;
		switch (stageInfo.shaderType) {
			case ShaderType::SHADER_TYPE_VERTEX: stage = vk::ShaderStageFlagBits::eVertex; break;
			case ShaderType::SHADER_TYPE_FRAGMENT: stage = vk::ShaderStageFlagBits::eFragment; break;
			case ShaderType::SHADER_TYPE_COMPUTE: stage = vk::ShaderStageFlagBits::eCompute; break;
			case ShaderType::SHADER_TYPE_RAYGEN: stage = vk::ShaderStageFlagBits::eRaygenKHR; break;
			case ShaderType::SHADER_TYPE_CLOSEST_HIT: stage = vk::ShaderStageFlagBits::eClosestHitKHR; break;
			case ShaderType::SHADER_TYPE_ANY_HIT: stage = vk::ShaderStageFlagBits::eAnyHitKHR; break;
			case ShaderType::SHADER_TYPE_MISS: stage = vk::ShaderStageFlagBits::eMissKHR; break;
			default:
				dbLog(dbg::LOG_ERROR, "Unsupported shader type: ", static_cast<int>(stageInfo.shaderType));
				throw std::runtime_error("Unsupported shader stage!");
		}
		const std::filesystem::path sourceFile{stageInfo.sourceFile};

		std::string filename = sourceFile.filename().string();
		std::replace(filename.begin(), filename.end(), '.', '_');
		std::string cacheFileName = std::format("{}_{}.spv", filename, stageInfo.entryPoint);
#ifndef NDEBUG
		CComPtr<IDxcBlob> sourceBlob;
		std::wstring	  wSourceFile = std::wstring(stageInfo.sourceFile.begin(), stageInfo.sourceFile.end());
		HRESULT			  hr		  = includeHandler->LoadSource(wSourceFile.c_str(), &sourceBlob);

		if (FAILED(hr)) {
			dbLog(dbg::LOG_ERROR, "Failed to load shader source file: ", stageInfo.sourceFile);
			THROW_RUNTIME_ERR(std::format("Failed to load shader source file: {}", stageInfo.sourceFile));
		}

		std::vector<LPCWSTR> arguments;
		arguments.push_back(L"-E");
		std::wstring entryPoint = std::wstring(stageInfo.entryPoint.begin(), stageInfo.entryPoint.end());
		arguments.push_back(entryPoint.c_str());
		arguments.push_back(L"-T");
		switch (stageInfo.shaderType) {
			case ShaderType::SHADER_TYPE_VERTEX: arguments.push_back(L"vs_6_6"); break;
			case ShaderType::SHADER_TYPE_FRAGMENT: arguments.push_back(L"ps_6_6"); break;
			case ShaderType::SHADER_TYPE_COMPUTE: arguments.push_back(L"cs_6_6"); break;
			case ShaderType::SHADER_TYPE_RAYGEN: arguments.push_back(L"lib_6_6"); break;
			case ShaderType::SHADER_TYPE_CLOSEST_HIT: arguments.push_back(L"lib_6_6"); break;
			case ShaderType::SHADER_TYPE_ANY_HIT: arguments.push_back(L"lib_6_6"); break;
			case ShaderType::SHADER_TYPE_MISS: arguments.push_back(L"lib_6_6"); break;
			default:
				dbLog(dbg::LOG_ERROR, "Unsupported shader type: ", static_cast<int>(stageInfo.shaderType));
				throw std::runtime_error("Unsupported shader stage!");
		}
		arguments.push_back(L"-spirv");
		arguments.push_back(L"-D");
		arguments.push_back(L"VULKAN");
		arguments.push_back(L"-D");
		arguments.push_back(L"SHADER");
		arguments.push_back(L"-I");
		arguments.push_back(L"./shaders/");
		arguments.push_back(L"-fvk-use-dx-layout");
		arguments.push_back(L"-fspv-target-env=vulkan1.2");
		arguments.push_back(L"-HV");
		arguments.push_back(L"2021");

		DxcBuffer buffer{};
		buffer.Ptr		= sourceBlob->GetBufferPointer();
		buffer.Size		= sourceBlob->GetBufferSize();
		buffer.Encoding = 0;

		dbLog(dbg::LOG_DEBUG, "\n\tCompiling shader: ", stageInfo.sourceFile,
			  "\n\tEntry point: ", stageInfo.entryPoint);

		includeHandler->reset();
		CComPtr<IDxcResult> result;
		hr = compiler->Compile(&buffer, arguments.data(), static_cast<UINT32>(arguments.size()), includeHandler.get(),
							   IID_PPV_ARGS(&result));
		if (FAILED(hr)) { throw std::runtime_error(std::format("Failed to compile shader: {}", stageInfo.sourceFile)); }

		CComPtr<IDxcBlobEncoding> errorBuffer;
		result->GetErrorBuffer(&errorBuffer);
		if (errorBuffer != nullptr && errorBuffer->GetBufferSize() > 0) {
			std::string errorMessage(reinterpret_cast<const char *>(errorBuffer->GetBufferPointer()),
									 errorBuffer->GetBufferSize());
			std::cerr << "Shader compilation warnings/errors: " << errorMessage << std::endl;
		}

		CComPtr<IDxcBlob> spirvBlob;
		result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&spirvBlob), nullptr);

		// write to shader cache
		std::filesystem::path cachePath = std::filesystem::path("shadercache") / cacheFileName;
		std::filesystem::create_directories(cachePath.parent_path());
		std::ofstream ofs(cachePath, std::ios::binary);
		ofs.write(reinterpret_cast<const char *>(spirvBlob->GetBufferPointer()), spirvBlob->GetBufferSize());
		ofs.close();
		vk::ShaderModuleCreateInfo shaderModuleInfo({}, spirvBlob->GetBufferSize(),
													reinterpret_cast<const uint32_t *>(spirvBlob->GetBufferPointer()));
#else	  // NDEBUG
		auto it = g_shaders.find(cacheFileName);
		if (it == g_shaders.end()) {
			dbLog(dbg::LOG_ERROR, "Shader cache not found for: ", cacheFileName);
			THROW_RUNTIME_ERR(std::format("Shader cache not found for: {}", cacheFileName));
		}
		auto					  &shaderdata = it->second;
		vk::ShaderModuleCreateInfo shaderModuleInfo({}, shaderdata.length,
													reinterpret_cast<const uint32_t *>(shaderdata.data));
#endif

		vk::ShaderModule shaderModule = nullptr;
		vkCreateShaderModule(device, &*shaderModuleInfo, nullptr, vkc(&shaderModule)());
		shaderModules.push_back(vkraii::ShaderModule(device.device, shaderModule));

		vk::PipelineShaderStageCreateInfo shaderStageInfo({}, stage, *shaderModules.back(),
														  stageInfo.entryPoint.c_str());
		shaderStages.push_back(shaderStageInfo);
	}
	return {std::move(shaderModules), std::move(shaderStages)};
}

std::vector<vk::PushConstantRange> VulkanProgramBuilder::createPushConstantRanges(
	const std::vector<PushConstantRange> &nriPushConstantRanges) {
	std::vector<vk::PushConstantRange> vkPushConstantRanges;
	for (const auto &nriPushConstantRange : nriPushConstantRanges) {
		vkPushConstantRanges.emplace_back(vk::ShaderStageFlagBits::eAll, nriPushConstantRange.offset,
										  nriPushConstantRange.size);
	}
	return vkPushConstantRanges;
}

vk::VertexInputRate nriInputRate2vkInputRate[] = {
	vk::VertexInputRate::eVertex,
	vk::VertexInputRate::eInstance,
};

std::unique_ptr<GraphicsProgram> VulkanProgramBuilder::buildGraphicsProgram() {
	auto [shaderModules, shaderStages] = createShaderModules(std::move(shaderStagesInfo), nri.getDevice());

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
	auto						 pushConstantRanges = createPushConstantRanges(this->pushConstantRanges);
	pipelineLayoutInfo.setPushConstantRangeCount(pushConstantRanges.size());
	pipelineLayoutInfo.setPPushConstantRanges(pushConstantRanges.data());
	pipelineLayoutInfo.setSetLayoutCount(1);
	pipelineLayoutInfo.setPSetLayouts(&*nri.getDescriptorAllocator().getDescriptorSetLayout());
	vk::PipelineLayout pipelineLayout = nullptr;
	vkCreatePipelineLayout(nri.getDevice(), &*pipelineLayoutInfo, nullptr, vkc(&pipelineLayout)());

	std::vector<vk::VertexInputBindingDescription>	 bindingDescriptions;
	std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;
	for (const auto &binding : this->vertexBindings) {
		vk::VertexInputRate inputRate = nriInputRate2vkInputRate[static_cast<int>(binding.inputRate)];
		assert(int(inputRate) != -1);
		vk::VertexInputBindingDescription bindingDescription(binding.binding, binding.stride, inputRate);
		bindingDescriptions.push_back(bindingDescription);

		for (const auto &attribute : binding.attributes) {
			vk::VertexInputAttributeDescription attributeDescription(attribute.location, binding.binding,
																	 vk::Format(attribute.format), attribute.offset);
			attributeDescriptions.push_back(attributeDescription);
		}
	}

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo({}, bindingDescriptions.size(), bindingDescriptions.data(),
														   attributeDescriptions.size(), attributeDescriptions.data());

	vk::PrimitiveTopology vkTopology = nriPrimitiveType2vkTopology[static_cast<int>(primitiveType)];
	assert(int(vkTopology) != -1);
	vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo({}, vkTopology, VK_FALSE);

	vk::Viewport						viewport(0.0f, 0.0f, 800.0f, 600.0f, 0.0f, 1.0f);
	vk::Rect2D							scissor({0, 0}, {800, 600});
	vk::PipelineViewportStateCreateInfo viewportStateInfo({}, 1, &viewport, 1, &scissor);

	vk::PipelineRasterizationStateCreateInfo rasterizerInfo({}, VK_FALSE, VK_FALSE, vk::PolygonMode::eFill,
															vk::CullModeFlagBits::eNone, vk::FrontFace::eClockwise,
															VK_FALSE, 0.0f, 0.0f, 0.0f, 1.0f);

	vk::PipelineMultisampleStateCreateInfo multisampleInfo({}, vk::SampleCountFlagBits::e1, VK_FALSE, 1.0f, nullptr,
														   VK_FALSE, VK_FALSE);

	vk::PipelineColorBlendAttachmentState colorBlendAttachment(
		VK_TRUE, vk::BlendFactor::eOne, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd, vk::BlendFactor::eOne,
		vk::BlendFactor::eZero, vk::BlendOp::eAdd,
		vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
			vk::ColorComponentFlagBits::eA);

	vk::PipelineColorBlendStateCreateInfo colorBlending({}, VK_FALSE, vk::LogicOp::eCopy, 1, &colorBlendAttachment,
														{0.0f, 0.0f, 0.0f, 0.0f});

	std::array<vk::DynamicState, 3>	   dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor,
														vk::DynamicState::eBlendConstants};
	vk::PipelineDynamicStateCreateInfo dynamicStateInfo({}, dynamicStates.size(), dynamicStates.data());

	vk::Format						colorFormat = vk::Format::eB8G8R8A8Unorm;
	vk::PipelineRenderingCreateInfo pipelineRenderingInfo(0, 1, &colorFormat);

	vk::GraphicsPipelineCreateInfo pipelineInfo(
		{}, static_cast<uint32_t>(shaderStages.size()), shaderStages.data(), &vertexInputInfo, &inputAssemblyInfo,
		nullptr,													   // tessellation
		&viewportStateInfo, &rasterizerInfo, &multisampleInfo, {},	   // depth stencil
		&colorBlending, &dynamicStateInfo, pipelineLayout, vk::RenderPass(nullptr), 0, nullptr, -1,
		&pipelineRenderingInfo);

	vk::Pipeline pipeline = nullptr;
	vkCreateGraphicsPipelines(nri.getDevice(), nullptr, 1, &*pipelineInfo, nullptr, vkc(&pipeline)());

	return std::make_unique<VulkanGraphicsProgram>(nri, vkraii::Pipeline(nri.getDevice().device, pipeline),
												   vkraii::PipelineLayout(nri.getDevice().device, pipelineLayout));
}

std::unique_ptr<ComputeProgram> VulkanProgramBuilder::buildComputeProgram() {
	auto [shaderModules, shaderStages] = createShaderModules(std::move(shaderStagesInfo), nri.getDevice());
	assert(shaderStages.size() == 1 && "Compute program must have exactly one shader stage.");

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
	vk::PipelineLayout			 pipelineLayout = nullptr;
	vkCreatePipelineLayout(nri.getDevice(), &*pipelineLayoutInfo, nullptr, vkc(&pipelineLayout)());

	vk::ComputePipelineCreateInfo pipelineInfo({}, shaderStages[0], pipelineLayout);

	vk::Pipeline pipeline = nullptr;
	vkCreateComputePipelines(nri.getDevice(), nullptr, 1, &*pipelineInfo, nullptr, vkc(&pipeline)());
	return std::make_unique<VulkanComputeProgram>(nri, vkraii::Pipeline(nri.getDevice().device, pipeline),
												  vkraii::PipelineLayout(nri.getDevice().device, pipelineLayout));
}

std::unique_ptr<RayTracingProgram> VulkanProgramBuilder::buildRayTracingProgram(nri::CommandBuffer &commandBuffer) {
	static_cast<void>(commandBuffer);
	dbLog(dbg::LOG_ERROR, "Ray tracing pipeline creation is not implemented yet!");
	THROW_RUNTIME_ERR("Ray tracing pipeline creation is not implemented yet!");
	return nullptr;
}

void VulkanProgram::bind(CommandBuffer &commandBuffer) {
	auto &vkCmdBuf = static_cast<VulkanCommandBuffer &>(commandBuffer);
	vkCmdBindPipeline(vkCmdBuf.commandBuffer, (VkPipelineBindPoint)bindPoint, pipeline);

	auto &descriptorSet = static_cast<VulkanNRI &>(nri).getDescriptorAllocator().getDescriptorSet();
	vkCmdBindDescriptorSets(vkCmdBuf.commandBuffer, (VkPipelineBindPoint)bindPoint, *pipelineLayout, 0, 1,
							vkc(&*descriptorSet)(), 0, nullptr);
}

void VulkanProgram::unbind(CommandBuffer &commandBuffer) {
	static_cast<void>(commandBuffer);
	// No unbind in Vulkan
}

void VulkanProgram::setPushConstants(CommandBuffer &commandBuffer, const void *data, std::size_t size,
									 std::size_t offset) {
	auto &vkCmdBuf = static_cast<VulkanCommandBuffer &>(commandBuffer);
	vkCmdPushConstants(*vkCmdBuf.commandBuffer, *pipelineLayout, (VkShaderStageFlags)vk::ShaderStageFlagBits::eAll,
					   offset, size, data);
}

void VulkanGraphicsProgram::draw(CommandBuffer &commandBuffer, uint32_t vertexCount, uint32_t instanceCount,
								 uint32_t firstVertex, uint32_t firstInstance) {
	auto &vkCmdBuf = static_cast<VulkanCommandBuffer &>(commandBuffer);
	vkCmdDraw(vkCmdBuf.commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void VulkanGraphicsProgram::drawIndexed(CommandBuffer &commandBuffer, uint32_t indexCount, uint32_t instanceCount,
										uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
	auto &vkCmdBuf = static_cast<VulkanCommandBuffer &>(commandBuffer);
	vkCmdDrawIndexed(vkCmdBuf.commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void VulkanComputeProgram::dispatch(CommandBuffer &commandBuffer, uint32_t groupCountX, uint32_t groupCountY,
									uint32_t groupCountZ) {
	auto &vkCmdBuf = static_cast<VulkanCommandBuffer &>(commandBuffer);
	vkCmdDispatch(vkCmdBuf.commandBuffer, groupCountX, groupCountY, groupCountZ);
}

void VulkanMemoryCache::assureSize(VulkanNRI &nri, std::size_t size) {
	if (size <= buffer.getSize()) return;
	dbLog(dbg::LOG_DEBUG, "Resizing acceleration structure scratch buffer to ", size, " bytes.");
	buffer = VulkanBuffer(nri, size,
						  BufferUsage::BUFFER_USAGE_STORAGE | BufferUsage::BUFFER_USAGE_TRANSFER_SRC |
							  BufferUsage::BUFFER_USAGE_TRANSFER_DST);
	allocation =
		VulkanAllocation(nri, buffer.getMemoryRequirements().setTypeRequest(MemoryTypeRequest::MEMORY_TYPE_DEVICE));
	buffer.bindMemory(allocation, 0);
}

VulkanBuffer &VulkanMemoryCache::getAccelerationStructureScratch(VulkanNRI									&nri,
																 vk::AccelerationStructureBuildSizesInfoKHR &sizeInfo) {
	assureSize(nri, sizeInfo.buildScratchSize);
	return buffer;
}

VulkanMemoryCache &VulkanMemoryCache::getInstance() {
	static VulkanMemoryCache instance;
	return instance;
}

void VulkanMemoryCache::destroy() {
	getInstance().buffer	 = VulkanBuffer(nullptr);
	getInstance().allocation = VulkanAllocation(nullptr);
}

std::unique_ptr<Window> VulkanNRI::createGLFWWindow(GLFWwindow *glfwWindow) {
	vk::SurfaceKHR surface;
	{
		VkSurfaceKHR cSurface;
		VkResult	 result = glfwCreateWindowSurface(instance, glfwWindow, nullptr, &cSurface);
		if (result != VK_SUCCESS) {
			THROW_RUNTIME_ERR(std::format("Failed to create Vulkan surface for GLFW window: {}", int(result)));
		}
		surface = vk::SurfaceKHR(cSurface);
	}
	auto window = std::make_unique<VulkanWindow>(*this);
	window->setSurface(vkraii::SurfaceKHR(instance.instance, surface));

	vk::Bool32 presentSupport;
	vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndices.graphicsFamily.value(),
										 *window->getSurface(), &presentSupport);

	if (!presentSupport) { throw std::runtime_error("Selected GPU does not support presentation to the surface!"); }

	uint32_t width;
	uint32_t height;
	window->createSwapChain(width, height);

	vk::Queue presentQueue;
	vkGetDeviceQueue(device, queueFamilyIndices.graphicsFamily.value(), 0, vkc(&presentQueue)());
	window->getPresentQueue() = vkraii::Queue(device.device, presentQueue);

	return window;
}

std::unique_ptr<Allocation> VulkanNRI::allocateMemory(MemoryRequirements memoryRequirements) {
	return std::make_unique<VulkanAllocation>(*this, memoryRequirements);
}

std::unique_ptr<CommandBuffer> VulkanNRI::createCommandBuffer(const CommandPool &pool) {
	vk::CommandBufferAllocateInfo allocInfo(static_cast<const VulkanCommandPool &>(pool).commandPool,
											vk::CommandBufferLevel::ePrimary, 1);
	vk::CommandBuffer			  buffers;
	vkAllocateCommandBuffers(device, &*allocInfo, vkc(&buffers)());
	return std::make_unique<VulkanCommandBuffer>(vkraii::CommandBuffer(device.device, buffers));
};

CommandQueue::SubmitKey VulkanCommandQueue::submit(CommandBuffer &commandBuffer) {
	auto &cmdBuf = static_cast<VulkanCommandBuffer &>(commandBuffer);
	cmdBuf.end();
	vk::CommandBuffer vk = cmdBuf.commandBuffer;
	vk::SubmitInfo	  submitInfo(0, nullptr, nullptr, 1, &vk);
	vkQueueSubmit(queue, 1, vkc(&submitInfo)(), nullptr);

	return 0;	  // TODO: fix this
}

void VulkanCommandQueue::wait(SubmitKey key) {
	static_cast<void>(key);		// TODO: fix this
	vkQueueWaitIdle(queue);
}

static int __asd = []() {
	Factory::getInstance().registerNRI(
		"Vulkan", [](CreateBits bits) -> std::unique_ptr<NRI> { return std::make_unique<VulkanNRI>(bits); });
	return 0;
}();
}	  // namespace nri
