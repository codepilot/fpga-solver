#pragma once

#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_NO_CONSTRUCTORS
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include <vulkan/vulkan.hpp>
#include <expected>
#include <bitset>
#include <iostream>
#include "MemoryMappedFile.h"
#include "each.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace vk_route {
	template<typename T> inline static T label(vk::UniqueDevice& device, std::string str, T item) noexcept {
		device->setDebugUtilsObjectNameEXT({ .objectType{item.get().objectType}, .objectHandle{std::bit_cast<uint64_t>(item.get())}, .pObjectName{str.c_str()} });
		return item;
	}

	template<typename T> inline static std::vector<T> labels(vk::UniqueDevice& device, std::string str, std::vector<T> items) noexcept {
		std::ranges::for_each(items, [&, i = 0](T& item) mutable {
			std::string label_n{ std::format("{}[{}]", str, i++) };
			device->setDebugUtilsObjectNameEXT({ .objectType{item.get().objectType}, .objectHandle{std::bit_cast<uint64_t>(item.get())}, .pObjectName{label_n.c_str() } });
			});
		return items;
	}

	class AlignedAllocation {
	public:
		void* pointer;
		size_t size;
		template<typename T>
		std::span<T> get_span() const noexcept {
			return std::span<T>(reinterpret_cast<T*>(pointer), size / sizeof(T));
		}
		AlignedAllocation() = default;

		AlignedAllocation(size_t _alignment, size_t _size) :
#ifdef _MSC_VER
			pointer{ _aligned_malloc(_size, _alignment) }, size{ _size }
#else
			pointer{ std::aligned_alloc(_alignment, _size) }, size{ _size }
#endif
		{
		}

		AlignedAllocation(AlignedAllocation& other) = delete;
		AlignedAllocation& operator=(AlignedAllocation& other) = delete;
		AlignedAllocation(AlignedAllocation&& other) noexcept : pointer{ std::exchange(other.pointer, nullptr) }, size{ std::exchange(other.size, 0) } {}
		void clear() noexcept {
			if (pointer) {
#ifdef _MSC_VER
				_aligned_free(pointer); pointer = nullptr;
#else
				std::free(pointer); pointer = nullptr;
#endif
			}
			size = 0;
		}
		AlignedAllocation& operator=(AlignedAllocation&& other) noexcept {
			clear();
			pointer = std::exchange(other.pointer, nullptr);
			size = std::exchange(other.size, 0);
			return *this;
		}

		~AlignedAllocation() {
			if (pointer) {
#ifdef _MSC_VER
				_aligned_free(pointer);
#else
				std::free(pointer);
#endif
			}

		}
	};
};

#include "DeviceMemoryBuffer.hpp"
#include "HostMemoryBuffer.hpp"

namespace vk_route {
	class vu {
	public:

		inline static std::vector<uint32_t> make_v_queue_family(std::span<vk::QueueFamilyProperties2> queue_families) noexcept {
			std::vector<uint32_t> queueFamilyIndices;
			each<uint32_t>(queue_families, [&](uint32_t queue_family_idx, VkQueueFamilyProperties2& queue_familiy_properties) noexcept {
				if ((queue_familiy_properties.queueFamilyProperties.queueFlags & VkQueueFlagBits::VK_QUEUE_COMPUTE_BIT) != VK_QUEUE_COMPUTE_BIT) {
					return;
				}
				queueFamilyIndices.emplace_back(queue_family_idx);
				});
			return queueFamilyIndices;
		}

		inline static vk::UniqueDevice create_device(vk::PhysicalDevice physical_device, std::span<vk::QueueFamilyProperties2> queue_families, uint32_t qfi_compute, uint32_t qfi_transfer) {
			std::vector<vk::DeviceQueueCreateInfo> v_queue_create_info;
			std::vector<std::vector<float>> all_queue_priorities;

			each<uint32_t>(queue_families, [&](uint32_t queue_family_idx, VkQueueFamilyProperties2& queue_familiy_properties) {
				if (queue_family_idx != qfi_compute && queue_family_idx != qfi_transfer) return;
				decltype(auto) queue_priorities{ all_queue_priorities.emplace_back(std::vector<float>(static_cast<size_t>(queue_familiy_properties.queueFamilyProperties.queueCount), 1.0f)) };
				v_queue_create_info.emplace_back(vk::DeviceQueueCreateInfo{
					.queueFamilyIndex{queue_family_idx},
					.queueCount{queue_familiy_properties.queueFamilyProperties.queueCount},
					.pQueuePriorities{queue_priorities.data()},
				});
			});

			vk::StructureChain<
				vk::DeviceCreateInfo,
				vk::PhysicalDeviceVulkan12Features,
				// vk::PhysicalDeviceShaderSubgroupUniformControlFlowFeaturesKHR,
				vk::PhysicalDeviceVulkan13Features> deviceCreateInfo;
			deviceCreateInfo.get<vk::DeviceCreateInfo>().setQueueCreateInfoCount(static_cast<uint32_t>(v_queue_create_info.size()));
			deviceCreateInfo.get<vk::DeviceCreateInfo>().setQueueCreateInfos(v_queue_create_info);
			deviceCreateInfo.get<vk::PhysicalDeviceVulkan13Features>().setSynchronization2(true);
			deviceCreateInfo.get<vk::PhysicalDeviceVulkan13Features>().setComputeFullSubgroups(true);
			deviceCreateInfo.get<vk::PhysicalDeviceVulkan13Features>().setMaintenance4(true);
			deviceCreateInfo.get<vk::PhysicalDeviceVulkan12Features>().setBufferDeviceAddress(true);
			deviceCreateInfo.get<vk::PhysicalDeviceVulkan12Features>().setTimelineSemaphore(true);
			std::vector<const char*> enabled_extensions;
			// enabled_extensions.emplace_back("VK_EXT_external_memory_host");
			// enabled_extensions.emplace_back("VK_KHR_shader_subgroup_uniform_control_flow");

			deviceCreateInfo.get<vk::DeviceCreateInfo>().setPEnabledExtensionNames(enabled_extensions);
			// deviceCreateInfo.get<vk::PhysicalDeviceShaderSubgroupUniformControlFlowFeaturesKHR>().setShaderSubgroupUniformControlFlow(true);

			auto device{ physical_device.createDeviceUnique(deviceCreateInfo.get<vk::DeviceCreateInfo>()).value };
			VULKAN_HPP_DEFAULT_DISPATCHER.init(device.get());

			device->setDebugUtilsObjectNameEXT({ .objectType{vk::ObjectType::eDevice}, .objectHandle{std::bit_cast<uint64_t>(device.get())}, .pObjectName{"Device"} });

			return device;
		}

		inline static vk::UniqueShaderModule make_shader_module(vk::UniqueDevice& device, std::string spv_path) noexcept {
			MemoryMappedFile mmf_spirv{ spv_path };
			auto s_spirv{ mmf_spirv.get_span<uint32_t>() };

			auto shaderModule{ label(device, "shaderModule", device->createShaderModuleUnique({.codeSize{s_spirv.size_bytes()}, .pCode{s_spirv.data()}}).value) };

			return shaderModule;
		}

		template<std::size_t binding_count>
		inline static vk::UniqueDescriptorSetLayout make_descriptor_set_layout(vk::UniqueDevice& device) noexcept {

			std::array<vk::DescriptorSetLayoutBinding, binding_count> descriptorSetLayoutBindings;
			each<uint32_t>(descriptorSetLayoutBindings, [](uint32_t descriptorSetLayoutBindingIndex, vk::DescriptorSetLayoutBinding& descriptorSetLayoutBinding) {
				descriptorSetLayoutBinding = {
					.binding{descriptorSetLayoutBindingIndex},
					.descriptorType{vk::DescriptorType::eStorageBuffer},
					.descriptorCount{1},
					.stageFlags{vk::ShaderStageFlagBits::eCompute},
				};
				});

			auto descriptorSetLayoutUnique{ label(device, "descriptorSetLayoutUnique", device->createDescriptorSetLayoutUnique({
				.bindingCount{descriptorSetLayoutBindings.size()},
				.pBindings{descriptorSetLayoutBindings.data()},
			}).value) };

			return descriptorSetLayoutUnique;

		}

		struct ShaderPushConstants {
			vk::DeviceAddress src_buf;
			vk::DeviceAddress dst_buf;
			uint32_t multiplicand;
		};

		inline static vk::UniquePipelineLayout make_pipeline_layout(vk::UniqueDevice& device, vk::UniqueDescriptorSetLayout& descriptorSetLayout) noexcept {
			const std::array<vk::PushConstantRange, 1> pushConstantRange{ {{
				.stageFlags{vk::ShaderStageFlagBits::eCompute},
				.offset{0},
				.size{sizeof(ShaderPushConstants)},
			}} };

			std::array<vk::DescriptorSetLayout, 1> descriptorSetLayouts{
				{
					descriptorSetLayout.get()
				}
			};

			auto pipelineLayoutUnique{ label(device, "pipelineLayoutUnique", device->createPipelineLayoutUnique({
				.setLayoutCount{descriptorSetLayouts.size()},
				.pSetLayouts{descriptorSetLayouts.data()},
				.pushConstantRangeCount{pushConstantRange.size()},
				.pPushConstantRanges{pushConstantRange.data()},
			}).value) };

			return pipelineLayoutUnique;
		}

		inline static std::vector<vk::UniquePipeline> make_compute_pipelines(vk::UniqueDevice& device, vk::UniqueShaderModule& simple_comp, vk::UniquePipelineLayout& pipelineLayout, vk::UniquePipelineCache& pipelineCache) noexcept {
			std::array<vk::SpecializationMapEntry, 4> specializationMapEntries{ {
				{.constantID{0}, .offset{0}, .size{sizeof(uint32_t)}, },
				{.constantID{1}, .offset{4}, .size{sizeof(uint32_t)}, },
				{.constantID{2}, .offset{8}, .size{sizeof(uint32_t)}, },
				{.constantID{3}, .offset{12}, .size{sizeof(uint32_t)}, },
			} };
			std::array<uint32_t, 4> specializationData{ 1024, 1, 1, 1 };
			vk::SpecializationInfo specializationInfo;
			specializationInfo.setData<uint32_t>(specializationData);
			specializationInfo.setMapEntries(specializationMapEntries);

			auto computePipelines{ labels(device, "computePipelines", device->createComputePipelinesUnique(pipelineCache.get(), {{{
				.flags{vk::PipelineCreateFlagBits::eDispatchBase},
				.stage{
					.flags{vk::PipelineShaderStageCreateFlagBits::eRequireFullSubgroups},
					.stage{vk::ShaderStageFlagBits::eCompute},
					.module{simple_comp.get()},
					.pName{"main"},
					.pSpecializationInfo{&specializationInfo},
				},
				.layout{pipelineLayout.get()},
			}} }).value) };

			return computePipelines;
		}

		inline static vk::UniqueDescriptorPool make_descriptor_pool(vk::UniqueDevice& device, uint32_t binding_count) noexcept {

			std::array<vk::DescriptorPoolSize, 1> descriptorPoolSize{ {{
				.type{vk::DescriptorType::eStorageBuffer},
				.descriptorCount{binding_count},
			}} };

			return label(device, "descriptorPool", device->createDescriptorPoolUnique({
				.flags{vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet},
				.maxSets{descriptorPoolSize.size()},
				.poolSizeCount{descriptorPoolSize.size()},
				.pPoolSizes{descriptorPoolSize.data()},
				}).value);
		}

		inline static std::vector<vk::UniqueDescriptorSet> make_descriptor_sets(vk::UniqueDevice& device, vk::UniqueDescriptorSetLayout& descriptorSetLayout, vk::UniqueDescriptorPool& descriptorPool) noexcept {
			std::array<vk::DescriptorSetLayout, 1> descriptorSetLayouts{
				{
					descriptorSetLayout.get()
				}
			};

			return labels(device, "descriptorSets", device->allocateDescriptorSetsUnique({
				.descriptorPool{descriptorPool.get()},
				.descriptorSetCount{descriptorSetLayouts.size()},
				.pSetLayouts{descriptorSetLayouts.data()},
				}).value);
		}

		inline static vk::UniqueQueryPool make_query_pool(vk::UniqueDevice& device) noexcept {
			return label(device, "queryPool", device->createQueryPoolUnique({
				.queryType{vk::QueryType::eTimestamp},
				.queryCount{2},
				}).value);
		}

		inline static vk::UniqueCommandPool make_command_pool(vk::UniqueDevice& device, uint32_t queueFamilyIndex) noexcept {
			return label(device, "commandPool", device->createCommandPoolUnique({ .flags{vk::CommandPoolCreateFlagBits::eTransient}, .queueFamilyIndex{queueFamilyIndex} }).value);
		}

		inline static std::vector<vk::UniqueCommandBuffer> make_command_buffers(vk::UniqueDevice& device, vk::UniqueCommandPool& commandPool) noexcept {
			return labels(device, "commandBuffers", device->allocateCommandBuffersUnique({
				.commandPool{commandPool.get()},
				.level{vk::CommandBufferLevel::ePrimary},
				.commandBufferCount{1},
			}).value);
		}

		inline static std::vector<uint32_t> get_graphics_queue_families(std::span<vk::QueueFamilyProperties2> queue_families) noexcept {
			std::vector<uint32_t> v_queue_family;
			each<uint32_t>(queue_families, [&](uint32_t queue_family_index, vk::QueueFamilyProperties2& properties) {
				if ((properties.queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics) == vk::QueueFlagBits::eGraphics) {
					v_queue_family.emplace_back(queue_family_index);
				}
			});
			return v_queue_family;
		}

		inline static std::vector<uint32_t> get_compute_queue_families(std::span<vk::QueueFamilyProperties2> queue_families) noexcept {
			std::vector<uint32_t> v_queue_family;
			static constexpr auto mask_graphics_compute{ vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute };
			each<uint32_t>(queue_families, [&](uint32_t queue_family_index, vk::QueueFamilyProperties2& properties) {
				if ((properties.queueFamilyProperties.queueFlags & mask_graphics_compute) == vk::QueueFlagBits::eCompute) {
					v_queue_family.emplace_back(queue_family_index);
				}
			});
			if (!v_queue_family.empty()) return v_queue_family;
			return get_graphics_queue_families(queue_families);
		}

		inline static std::vector<uint32_t> get_transfer_queue_families(std::span<vk::QueueFamilyProperties2> queue_families) noexcept {
			std::vector<uint32_t> queue_family;
			static constexpr auto mask_graphics_compute{ vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute | vk::QueueFlagBits::eTransfer };
			each<uint32_t>(queue_families, [&](uint32_t queue_family_index, vk::QueueFamilyProperties2& properties) {
				if ((properties.queueFamilyProperties.queueFlags & mask_graphics_compute) == vk::QueueFlagBits::eTransfer) {
					queue_family.emplace_back(queue_family_index);
				}
			});
			if (!queue_family.empty()) return queue_family;
			return get_compute_queue_families(queue_families);
		}

		inline static std::vector<vk::Queue> make_queues_from_v_index(vk::UniqueDevice& device, std::span<vk::QueueFamilyProperties2> queue_families, uint32_t queueFamilyIndex) {
			// uint32_t queueFamilyIndex{ v_queue_index.front() };
			decltype(auto) queueFamily{ queue_families[queueFamilyIndex] };
			std::vector<vk::Queue> queues;

#ifdef _DEBUG
			std::cout << std::format("make {} queues {}\n", queueFamily.queueFamilyProperties.queueCount, vk::to_string(queueFamily.queueFamilyProperties.queueFlags));
#endif
			for (uint32_t queueIndex = 0; queueIndex < queueFamily.queueFamilyProperties.queueCount; ++queueIndex) {
				queues.emplace_back(device->getQueue2({ .queueFamilyIndex{queueFamilyIndex}, .queueIndex{queueIndex} }));
			}

			return queues;
		}

		inline static std::vector<vk::Queue> make_compute_queues(vk::UniqueDevice& device, std::span<vk::QueueFamilyProperties2> queue_families) noexcept {
			std::vector<uint32_t> v_queue_index{ get_compute_queue_families(queue_families) };
			return make_queues_from_v_index(device, queue_families, v_queue_index.front());
		}

		inline static std::vector<vk::Queue> make_transfer_queues(vk::UniqueDevice& device, std::span<vk::QueueFamilyProperties2> queue_families) noexcept {
			std::vector<uint32_t> v_queue_index{ get_transfer_queue_families(queue_families) };
			return make_queues_from_v_index(device, queue_families, v_queue_index.front());
		}

		inline static vk::UniqueSemaphore make_timeline_semaphore(vk::UniqueDevice& device, uint64_t initial_value=0) noexcept {
			vk::StructureChain<vk::SemaphoreCreateInfo, vk::SemaphoreTypeCreateInfo> createInfo;
			createInfo.get<vk::SemaphoreTypeCreateInfo>().setSemaphoreType(vk::SemaphoreType::eTimeline);
			createInfo.get<vk::SemaphoreTypeCreateInfo>().setInitialValue(initial_value);
			return device->createSemaphoreUnique(createInfo.get<vk::SemaphoreCreateInfo>()).value;
		}

	};

};
