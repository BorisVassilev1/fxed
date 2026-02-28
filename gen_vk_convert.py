#!/usr/bin/python3

import xml.etree.ElementTree as ET

path = "https://raw.githubusercontent.com/KhronosGroup/Vulkan-Headers/refs/heads/main/registry/vk.xml"

import urllib.request as urllib
response = urllib.urlopen(path)

tree = ET.parse(response)
root = tree.getroot()

print('''
#pragma once
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_core.h"
namespace vk_convert {
template <class U, class V>
struct vk_c_convert {};

template <class U>
struct vk_c_convert<U, U> {
	U u;
	vk_c_convert(U u) : u(u) {}
	U operator()() { return u; }
};

template <class U, class V>
struct vk_c_convert_impl {
    static_assert(sizeof(U) == sizeof(V), "Size of U and V must be the same");
	U u;
	vk_c_convert_impl(U u) : u(u) {}
	V operator()() { return reinterpret_cast<V>(u); }
};

#define VK_C_CONVERT(U, V)                                       \\
	template <>                                                  \\
	struct vk_c_convert<U, V> : public vk_c_convert_impl<U, V> { \\
		using vk_c_convert_impl<U, V>::vk_c_convert_impl;        \\
	};                                                           \\
	template <class U_>                                          \\
		requires std::same_as<U_, U>                             \\
	vk_c_convert(U_ u) -> vk_c_convert<U, V>;

#define VK_C_CONVERT_PTR(U, V) VK_C_CONVERT(U *, V *)

''')

types = root.find('types')
for t in types.findall('type'):
    category = t.get('category')
    name = t.get('name')
    if name is None:
        continue
    if category not in ['struct', 'handle']:
        continue
    print(f'// {category} {name}')
    if not name.startswith('Vk'): continue
    if name in [
            'VkBaseInStructure', 
            'VkBaseOutStructure', 
            'VkPipelineCacheStageValidationIndexEntry',
            'VkPipelineCacheSafetyCriticalIndexEntry',
            'VkPipelineCacheHeaderVersionSafetyCriticalOne',
            'VkPhysicalDeviceVariablePointerFeatures',
            'VkPhysicalDeviceShaderDrawParameterFeatures',
            'VkFaultData',
            'VkFaultCallbackInfo',
            'VkPipelineOfflineCreateInfo',
            'VkPhysicalDeviceVulkanSC10Properties',
            'VkPipelinePoolSize',
            'VkDeviceObjectReservationCreateInfo',
            'VkCommandPoolMemoryReservationCreateInfo',
            'VkCommandPoolMemoryConsumption',
            'VkPhysicalDeviceVulkanSC10Features',
            'VkPhysicalDeviceDescriptorHeapTensorPropertiesARM'
            ]:
        continue
    if name.endswith('KHR') or \
            name.endswith('EXT') or \
            name.endswith('NV') or \
            name.endswith('AMD') or \
            name.endswith('INTEL') or \
            name.endswith('GOOGLE') or \
            name.endswith('FUCHSIA') or \
            name.endswith('GGP') or \
            name.endswith('NN') or \
            name.endswith('SEC') or \
            name.endswith('QNX') or \
            name.endswith('QCOM') or \
            name.endswith('MVK') or \
            name.endswith('ANDROID') or \
            name.endswith('APPLE') or \
            name.endswith('VALVE') or \
            name.endswith('AMDX') or \
            name.endswith('OHOS') :
        continue
    print(f'VK_C_CONVERT_PTR(vk::{name[2:]}, {name})')

print('''
VK_C_CONVERT_PTR(vk::BufferView, VkBufferView)
VK_C_CONVERT_PTR(vk::CommandPool, VkCommandPool)
VK_C_CONVERT_PTR(vk::Buffer, VkBuffer)
VK_C_CONVERT_PTR(vk::DescriptorPool, VkDescriptorPool)
VK_C_CONVERT_PTR(vk::DescriptorSet, VkDescriptorSet)
VK_C_CONVERT_PTR(vk::DescriptorSetLayout, VkDescriptorSetLayout)
VK_C_CONVERT_PTR(vk::Device, VkDevice)
VK_C_CONVERT_PTR(vk::DeviceMemory, VkDeviceMemory)
VK_C_CONVERT_PTR(vk::Event, VkEvent)
VK_C_CONVERT_PTR(vk::Fence, VkFence)
VK_C_CONVERT_PTR(vk::Semaphore, VkSemaphore)
VK_C_CONVERT_PTR(vk::Image, VkImage)
VK_C_CONVERT_PTR(vk::ImageView, VkImageView)
VK_C_CONVERT_PTR(vk::Pipeline, VkPipeline)
VK_C_CONVERT_PTR(vk::PipelineCache, VkPipelineCache)
VK_C_CONVERT_PTR(vk::PipelineLayout, VkPipelineLayout)
VK_C_CONVERT_PTR(vk::QueryPool, VkQueryPool)
VK_C_CONVERT_PTR(vk::RenderPass, VkRenderPass)
VK_C_CONVERT_PTR(vk::Sampler, VkSampler)
VK_C_CONVERT_PTR(vk::ShaderModule, VkShaderModule)
VK_C_CONVERT_PTR(vk::SwapchainKHR, VkSwapchainKHR)
VK_C_CONVERT_PTR(vk::Queue, VkQueue)
VK_C_CONVERT_PTR(vk::CommandBuffer, VkCommandBuffer)

#undef VK_C_CONVERT
#undef VK_C_CONVERT_PTR
}
template <class U, class V>
using vkc = vk_convert::vk_c_convert<U, V>;

''')
