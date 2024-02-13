#pragma once

#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_NO_CONSTRUCTORS
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include <vulkan/vulkan.hpp>
#include <expected>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace vk_route {
	class SingleDevice {
	public:
		vk::PhysicalDevice physical_device;
		vk::StructureChain<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceExternalMemoryHostPropertiesEXT> physical_device_properties;
		vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features, vk::PhysicalDeviceVulkan12Features, vk::PhysicalDeviceVulkan13Features> physical_device_features;
		vk::PhysicalDeviceMemoryProperties2 memory_properties;
		std::span<vk::MemoryType> memory_types;
		std::vector<vk::QueueFamilyProperties2> queue_families;
		std::vector<uint32_t> queueFamilyIndices;
		vk::UniqueDevice device;
		using UniqueDedicatedMemoryBuffer = std::tuple<vk::UniqueBuffer, vk::UniqueDeviceMemory, vk::MemoryRequirements2>;
		using UniqueHostMemoryBuffer = std::tuple<vk::UniqueBuffer, vk::UniqueDeviceMemory, vk::MemoryRequirements2, void *>;
		UniqueDedicatedMemoryBuffer binding0;
		UniqueDedicatedMemoryBuffer binding1;
		MemoryMappedFile mmf_bounce_in;
		UniqueHostMemoryBuffer bounce_in;
		UniqueDedicatedMemoryBuffer bounce_out;
		vk::UniqueShaderModule simple_comp;
		vk::UniqueDescriptorSetLayout descriptorSetLayout;
		vk::UniquePipelineLayout pipelineLayout;
		vk::UniquePipelineCache pipelineCache;
		std::vector<vk::UniquePipeline> computePipelines;
		vk::UniqueDescriptorPool descriptorPool;
		std::vector<vk::UniqueDescriptorSet> descriptorSets;
		vk::UniqueQueryPool queryPool;
		vk::UniqueCommandPool commandPool;
		std::vector<vk::UniqueCommandBuffer> commandBuffers;
		vk::Queue queue;

		template<typename T> inline static T label(vk::UniqueDevice& device, std::string str, T item) noexcept {
			device->setDebugUtilsObjectNameEXT({ .objectType{item.get().objectType}, .objectHandle{std::bit_cast<uint64_t>(item.get())}, .pObjectName{str.c_str()} });
			return item;
		}

		template<typename T> inline static std::vector<T> labels(vk::UniqueDevice& device, std::string str, std::vector<T> items) noexcept {
			std::ranges::for_each(items, [&, i=0](T& item) mutable {
				std::string label_n{ std::format("{}[{}]", str, i++) };
				device->setDebugUtilsObjectNameEXT({ .objectType{item.get().objectType}, .objectHandle{std::bit_cast<uint64_t>(item.get())}, .pObjectName{label_n.c_str() }});
			});
			return items;
		}

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

		inline static vk::UniqueDevice create_device(vk::PhysicalDevice physical_device, std::span<vk::QueueFamilyProperties2> queue_families) {
			std::vector<vk::DeviceQueueCreateInfo> v_queue_create_info;
			std::vector<std::vector<float>> all_queue_priorities;
			all_queue_priorities.reserve(queue_families.size());

			each<uint32_t>(queue_families, [&](uint32_t queue_family_idx, VkQueueFamilyProperties2& queue_familiy_properties) {
				if ((queue_familiy_properties.queueFamilyProperties.queueFlags & VkQueueFlagBits::VK_QUEUE_COMPUTE_BIT) != VK_QUEUE_COMPUTE_BIT) {
					return;
				}
				decltype(auto) queue_priorities{ all_queue_priorities.emplace_back(std::vector<float>(static_cast<size_t>(queue_familiy_properties.queueFamilyProperties.queueCount), 1.0f)) };
				v_queue_create_info.emplace_back(vk::DeviceQueueCreateInfo{
					.queueFamilyIndex{queue_family_idx},
					.queueCount{queue_familiy_properties.queueFamilyProperties.queueCount},
					.pQueuePriorities{queue_priorities.data()},
				});
			});

			vk::StructureChain<vk::DeviceCreateInfo, vk::PhysicalDeviceVulkan13Features> deviceCreateInfo;
			deviceCreateInfo.get<vk::DeviceCreateInfo>().setQueueCreateInfoCount(static_cast<uint32_t>(v_queue_create_info.size()));
			deviceCreateInfo.get<vk::DeviceCreateInfo>().setQueueCreateInfos(v_queue_create_info);
			deviceCreateInfo.get<vk::PhysicalDeviceVulkan13Features>().setSynchronization2(true);
			std::array<const char* const, 1> enabled_extensions{ "VK_EXT_external_memory_host" };
			deviceCreateInfo.get<vk::DeviceCreateInfo>().setPEnabledExtensionNames(enabled_extensions);

			auto device{ physical_device.createDeviceUnique(deviceCreateInfo.get<vk::DeviceCreateInfo>()).value };
			VULKAN_HPP_DEFAULT_DISPATCHER.init(device.get());

			device->setDebugUtilsObjectNameEXT({.objectType{vk::ObjectType::eDevice}, .objectHandle{std::bit_cast<uint64_t>(device.get())}, .pObjectName{"Device"}});

			return device;
		}

		inline static UniqueDedicatedMemoryBuffer make_buffer_binding0(vk::UniqueDevice &device, std::span<uint32_t> queueFamilyIndices, std::span<vk::MemoryType> memory_types) {
			vk::BufferCreateInfo binding0_bci{
				.size{1024ull * 1024ull},
				.usage{vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst},
				.sharingMode{vk::SharingMode::eExclusive},
				.queueFamilyIndexCount{static_cast<uint32_t>(queueFamilyIndices.size())},
				.pQueueFamilyIndices{queueFamilyIndices.data()},
			};

			auto binding0_mem_requirements{ device->getBufferMemoryRequirements(vk::DeviceBufferMemoryRequirements{.pCreateInfo{&binding0_bci}}) };
#ifdef _DEBUG
			std::cout << std::format("binding0_mem_requirements: {}\n", binding0_mem_requirements.memoryRequirements.size);
#endif
			vk::StructureChain<vk::MemoryAllocateInfo, vk::MemoryDedicatedAllocateInfo> binding0_ai;

			binding0_ai.get<vk::MemoryAllocateInfo>().allocationSize = binding0_mem_requirements.memoryRequirements.size;

			VkMemoryPropertyFlags binding0_needed_flags{ VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
			binding0_ai.get<vk::MemoryAllocateInfo>().memoryTypeIndex = static_cast<uint32_t>(
				std::distance(
					memory_types.begin(),
					std::ranges::find_if(
						memory_types,
						[&](const VkMemoryType& memoryType)-> bool { return (memoryType.propertyFlags & binding0_needed_flags) == binding0_needed_flags; }
					)
				)
			);

			auto binding0_buffer{ label(device, "binding0", device->createBufferUnique(binding0_bci).value) };
			binding0_ai.get<vk::MemoryDedicatedAllocateInfo>().buffer = binding0_buffer.get();
			
			auto binding0_memory{ label(device, "binding0", device->allocateMemoryUnique(binding0_ai.get<vk::MemoryAllocateInfo>()).value) };

			device->bindBufferMemory(binding0_buffer.get(), binding0_memory.get(), 0ull);

			return std::make_tuple<vk::UniqueBuffer, vk::UniqueDeviceMemory, vk::MemoryRequirements2>(
				std::move(binding0_buffer),
				std::move(binding0_memory),
				std::move(binding0_mem_requirements)
			);
		}

		inline static UniqueDedicatedMemoryBuffer make_buffer_binding1(vk::UniqueDevice& device, std::span<uint32_t> queueFamilyIndices, std::span<vk::MemoryType> memory_types) {
			vk::BufferCreateInfo binding1_bci{
				.size{1024ull * 1024ull},
				.usage{vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferSrc},
				.sharingMode{vk::SharingMode::eExclusive},
				.queueFamilyIndexCount{static_cast<uint32_t>(queueFamilyIndices.size())},
				.pQueueFamilyIndices{queueFamilyIndices.data()},
			};

			auto binding1_mem_requirements{ device->getBufferMemoryRequirements(vk::DeviceBufferMemoryRequirements{.pCreateInfo{&binding1_bci}}) };
#ifdef _DEBUG
			std::cout << std::format("binding1_mem_requirements: {}\n", binding1_mem_requirements.memoryRequirements.size);
#endif
			auto binding1_buffer{ label(device, "binding1", device->createBufferUnique(binding1_bci).value)};
			VkMemoryPropertyFlags binding1_needed_flags{ VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
			vk::StructureChain<vk::MemoryAllocateInfo, vk::MemoryDedicatedAllocateInfo> binding1_ai;
			binding1_ai.get<vk::MemoryAllocateInfo>().allocationSize = binding1_mem_requirements.memoryRequirements.size;
			binding1_ai.get<vk::MemoryAllocateInfo>().memoryTypeIndex = static_cast<uint32_t>(
				std::distance(
					memory_types.begin(),
					std::ranges::find_if(
						memory_types,
						[&](const VkMemoryType& memoryType)-> bool { return (memoryType.propertyFlags & binding1_needed_flags) == binding1_needed_flags; }
					)
				)
			);
			binding1_ai.get<vk::MemoryDedicatedAllocateInfo>().buffer = binding1_buffer.get();
			auto binding1_memory{ label(device, "binding1", device->allocateMemoryUnique(binding1_ai.get<vk::MemoryAllocateInfo>()).value) };
			device->bindBufferMemory(binding1_buffer.get(), binding1_memory.get(), 0ull);

			return std::make_tuple<vk::UniqueBuffer, vk::UniqueDeviceMemory, vk::MemoryRequirements2>(
				std::move(binding1_buffer),
				std::move(binding1_memory),
				std::move(binding1_mem_requirements)
			);

		}

		inline static UniqueHostMemoryBuffer make_buffer_bounce_in(vk::UniqueDevice& device, std::span<uint32_t> queueFamilyIndices, std::span<vk::MemoryType> memory_types, vk::StructureChain<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceExternalMemoryHostPropertiesEXT> &physical_device_properties) {

			vk::StructureChain<vk::BufferCreateInfo, vk::ExternalMemoryBufferCreateInfo> bounce_in_bci;
			bounce_in_bci.get<vk::BufferCreateInfo>().setSize(1024ull * 1024ull);
			bounce_in_bci.get<vk::BufferCreateInfo>().setUsage(vk::BufferUsageFlagBits::eTransferSrc);
			bounce_in_bci.get<vk::BufferCreateInfo>().setSharingMode(vk::SharingMode::eExclusive);
			bounce_in_bci.get<vk::BufferCreateInfo>().setQueueFamilyIndices(queueFamilyIndices);
			bounce_in_bci.get<vk::ExternalMemoryBufferCreateInfo>().setHandleTypes(vk::ExternalMemoryHandleTypeFlagBits::eHostAllocationEXT);
			auto bounce_in_mem_requirements{ device->getBufferMemoryRequirements(vk::DeviceBufferMemoryRequirements{.pCreateInfo{&bounce_in_bci.get<vk::BufferCreateInfo>()}})};
#ifdef _DEBUG
			std::cout << std::format("bounce_in_mem_requirements:   {}\n", bounce_in_mem_requirements.memoryRequirements.size);
#endif
			auto bounce_in_buffer{ label(device, "bounce_in", device->createBufferUnique(bounce_in_bci.get<vk::BufferCreateInfo>()).value) };
			vk::StructureChain<vk::MemoryAllocateInfo, vk::ImportMemoryHostPointerInfoEXT> bounce_in_ai;
			bounce_in_ai.get<vk::MemoryAllocateInfo>().allocationSize = bounce_in_mem_requirements.memoryRequirements.size;
#ifdef _MSC_VER
			auto host_pointer{ _aligned_malloc(bounce_in_mem_requirements.memoryRequirements.size, physical_device_properties.get<vk::PhysicalDeviceExternalMemoryHostPropertiesEXT>().minImportedHostPointerAlignment ) };
#else
			auto host_pointer{ std::aligned_alloc(physical_device_properties.get<vk::PhysicalDeviceExternalMemoryHostPropertiesEXT>().minImportedHostPointerAlignment, bounce_in_mem_requirements.memoryRequirements.size) };
#endif
			auto hostPointerProperties{ device->getMemoryHostPointerPropertiesEXT(vk::ExternalMemoryHandleTypeFlagBits::eHostAllocationEXT, host_pointer).value };
			bounce_in_ai.get<vk::MemoryAllocateInfo>().memoryTypeIndex = static_cast<uint32_t>(
				std::distance(
					memory_types.begin(),
					std::ranges::find_if(
						memory_types,
						[&](const VkMemoryType& memoryType)-> bool { return (memoryType.propertyFlags & hostPointerProperties.memoryTypeBits) == hostPointerProperties.memoryTypeBits; }
					)
				)
			);
			bounce_in_ai.get<vk::ImportMemoryHostPointerInfoEXT>().setHandleType(vk::ExternalMemoryHandleTypeFlagBits::eHostAllocationEXT);

			bounce_in_ai.get<vk::ImportMemoryHostPointerInfoEXT>().setPHostPointer(host_pointer);

			auto bounce_in_memory{ label(device, "bounce_in", device->allocateMemoryUnique(bounce_in_ai.get<vk::MemoryAllocateInfo>()).value) };

			device->bindBufferMemory(bounce_in_buffer.get(), bounce_in_memory.get(), 0ull);
			return std::make_tuple<vk::UniqueBuffer, vk::UniqueDeviceMemory, vk::MemoryRequirements2>(
				std::move(bounce_in_buffer),
				std::move(bounce_in_memory),
				std::move(bounce_in_mem_requirements),
				host_pointer
			);

		}

		inline static UniqueDedicatedMemoryBuffer make_buffer_bounce_out(vk::UniqueDevice& device, std::span<uint32_t> queueFamilyIndices, std::span<vk::MemoryType> memory_types) {
			vk::BufferCreateInfo bounce_out_bci{
				.size{1024ull * 1024ull},
				.usage{vk::BufferUsageFlagBits::eTransferDst},
				.sharingMode{vk::SharingMode::eExclusive},
				.queueFamilyIndexCount{static_cast<uint32_t>(queueFamilyIndices.size())},
				.pQueueFamilyIndices{queueFamilyIndices.data()},
			};
			auto bounce_out_mem_requirements{ device->getBufferMemoryRequirements(vk::DeviceBufferMemoryRequirements{.pCreateInfo{&bounce_out_bci}}) };
#ifdef _DEBUG
			std::cout << std::format("bounce_out_mem_requirements:   {}\n", bounce_out_mem_requirements.memoryRequirements.size);
#endif
			auto bounce_out_buffer{ label(device, "bounce_out", device->createBufferUnique(bounce_out_bci).value) };
			VkMemoryPropertyFlags bounce_out_needed_flags{ VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT };
			vk::StructureChain<vk::MemoryAllocateInfo, vk::MemoryDedicatedAllocateInfo> bounce_out_ai;
			bounce_out_ai.get<vk::MemoryAllocateInfo>().allocationSize = bounce_out_mem_requirements.memoryRequirements.size;
			bounce_out_ai.get<vk::MemoryAllocateInfo>().memoryTypeIndex = static_cast<uint32_t>(
				std::distance(
					memory_types.begin(),
					std::ranges::find_if(
						memory_types,
						[&](const VkMemoryType& memoryType)-> bool { return (memoryType.propertyFlags & bounce_out_needed_flags) == bounce_out_needed_flags; }
					)
				)
			);
			bounce_out_ai.get<vk::MemoryDedicatedAllocateInfo>().buffer = bounce_out_buffer.get();
			auto bounce_out_memory{ label(device, "bounce_out", device->allocateMemoryUnique(bounce_out_ai.get<vk::MemoryAllocateInfo>()).value) };

			device->bindBufferMemory(bounce_out_buffer.get(), bounce_out_memory.get(), 0ull);
			return std::make_tuple<vk::UniqueBuffer, vk::UniqueDeviceMemory, vk::MemoryRequirements2>(
				std::move(bounce_out_buffer),
				std::move(bounce_out_memory),
				std::move(bounce_out_mem_requirements)
			);

		}

		inline static vk::UniqueShaderModule make_shader_module(vk::UniqueDevice& device, std::string spv_path) noexcept {
			MemoryMappedFile mmf_spirv{ spv_path };
			auto s_spirv{ mmf_spirv.get_span<uint32_t>() };

			auto shaderModule{ label(device, "shaderModule", device->createShaderModuleUnique({.codeSize{s_spirv.size_bytes()}, .pCode{s_spirv.data()}}).value) };

			return shaderModule;
		}

		inline static void setup_bounce_in(vk::UniqueDevice& device, UniqueHostMemoryBuffer &bounce_in, MemoryMappedFile &mmf_bounce_in) noexcept {
			std::span<uint8_t> bounce_in_mapped(reinterpret_cast<uint8_t*>(std::get<3>(bounce_in)), std::get<2>(bounce_in).memoryRequirements.size);
#ifdef _DEBUG
			std::cout << std::format("bounce_in_mapped<T> count: {}, bytes: {}\n", bounce_in_mapped.size(), bounce_in_mapped.size_bytes());
#endif
			std::ranges::copy(mmf_bounce_in.get_span<uint8_t>(), bounce_in_mapped.begin());
		}

		inline static void check_bounce_out(vk::UniqueDevice& device, UniqueDedicatedMemoryBuffer& bounce_out) {
			auto bounce_out_mapped_ptr{ device->mapMemory(std::get<1>(bounce_out).get(), 0ull, VK_WHOLE_SIZE).value };
			std::span<uint32_t> bounce_out_mapped(reinterpret_cast<uint32_t*>(bounce_out_mapped_ptr), std::get<2>(bounce_out).memoryRequirements.size / sizeof(uint32_t));
#ifdef _DEBUG
			std::cout << std::format("bounce_out_mapped<T> count: {}, bytes: {}\n", bounce_out_mapped.size(), bounce_out_mapped.size_bytes());
#endif
			bool is_unexpected{ false };
			for (auto b : bounce_out_mapped.first(256)) {
				if (b != 0xaaffffac) {
					std::cout << std::format("{:08x} ", b);
					is_unexpected = true;
				}
			}
			if(is_unexpected) std::cout << "\n";

			device->unmapMemory(std::get<1>(bounce_out).get());
		}

		template<std::size_t binding_count>
		inline static vk::UniqueDescriptorSetLayout make_descriptor_set_layout(vk::UniqueDevice& device) noexcept {

			std::array<vk::DescriptorSetLayoutBinding, binding_count> descriptorSetLayoutBindings;
			each<uint32_t>(descriptorSetLayoutBindings, [](uint32_t descriptorSetLayoutBindingIndex, vk::DescriptorSetLayoutBinding &descriptorSetLayoutBinding) {
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

		inline static vk::UniquePipelineLayout make_pipeline_layout(vk::UniqueDevice& device, vk::UniqueDescriptorSetLayout &descriptorSetLayout) noexcept {
			const std::array<vk::PushConstantRange, 1> pushConstantRange{ {{
				.stageFlags{vk::ShaderStageFlagBits::eCompute},
				.offset{0},
				.size{sizeof(uint32_t)},
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

		inline static std::vector<vk::UniquePipeline> make_compute_pipelines(vk::UniqueDevice& device, vk::UniqueShaderModule &simple_comp, vk::UniquePipelineLayout &pipelineLayout, vk::UniquePipelineCache &pipelineCache) noexcept {
			std::array<vk::SpecializationMapEntry, 1> specializationMapEntries{ {{
					.constantID{0},
					.offset{0},
					.size{sizeof(uint32_t)},
				}} };
			std::array<uint32_t, 1> specializationData{ 1 };
			vk::SpecializationInfo specializationInfo;
			specializationInfo.setData<uint32_t>(specializationData);
			specializationInfo.setMapEntries(specializationMapEntries);

			auto computePipelines{ labels(device, "computePipelines", device->createComputePipelinesUnique(pipelineCache.get(), {{{
				.stage{
					.stage{vk::ShaderStageFlagBits::eCompute},
					.module{simple_comp.get()},
					.pName{"main"},
					.pSpecializationInfo{&specializationInfo},
				},
				.layout{pipelineLayout.get()},
			}} }).value) };

			return computePipelines;
		}

		inline static vk::UniqueDescriptorPool make_descriptor_pool(vk::UniqueDevice& device) noexcept {

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

		inline static std::vector<vk::UniqueDescriptorSet> make_descriptor_sets(vk::UniqueDevice& device, vk::UniqueDescriptorSetLayout& descriptorSetLayout, vk::UniqueDescriptorPool &descriptorPool) noexcept {
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

		inline static uint32_t selectQueueFamilyIndex(std::span<vk::QueueFamilyProperties2> queue_families) {
			return static_cast<uint32_t>(std::distance(queue_families.begin(), std::ranges::find(queue_families, VK_QUEUE_COMPUTE_BIT, [](VkQueueFamilyProperties2& props)-> VkQueueFlagBits {
				return std::bit_cast<VkQueueFlagBits>(props.queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT);
			})));
		}

		inline static vk::UniqueCommandPool make_command_pool(vk::UniqueDevice& device, std::span<vk::QueueFamilyProperties2> queue_families) noexcept {
			return label(device, "commandPool", device->createCommandPoolUnique({.queueFamilyIndex{selectQueueFamilyIndex(queue_families)}}).value);
		}

		inline static constexpr std::size_t binding_count{ 2ull };

		inline void updateDescriptorSets() {
			std::array<vk::DescriptorBufferInfo, 1> binding0_bufferInfos{ {{.buffer{std::get<0>(binding0).get()}, .offset{}, .range{VK_WHOLE_SIZE}}} };
			std::array<vk::DescriptorBufferInfo, 1> binding1_bufferInfos{ {{.buffer{std::get<0>(binding1).get()}, .offset{}, .range{VK_WHOLE_SIZE}}} };

			device->updateDescriptorSets(std::array<vk::WriteDescriptorSet, binding_count>{
				{
					{
						.dstSet{ descriptorSets.at(0).get()},
						.dstBinding{ 0 },
						.dstArrayElement{ 0 },
						.descriptorCount{ binding0_bufferInfos.size() },
						.descriptorType{ vk::DescriptorType::eStorageBuffer },
						.pBufferInfo{ binding0_bufferInfos.data() },
					},
					{
						.dstSet{descriptorSets.at(0).get()},
						.dstBinding{1},
						.dstArrayElement{0},
						.descriptorCount{binding1_bufferInfos.size()},
						.descriptorType{vk::DescriptorType::eStorageBuffer},
						.pBufferInfo{binding1_bufferInfos.data()},
					}
				}
			}, {});
		}

		inline static auto make_command_buffers(vk::UniqueDevice& device, vk::UniqueCommandPool &commandPool) noexcept {
			return labels(device, "commandBuffers", device->allocateCommandBuffersUnique({
				.commandPool{commandPool.get()},
				.level{vk::CommandBufferLevel::ePrimary},
				.commandBufferCount{1},
			}).value);
		}

		inline void record_command_buffer() noexcept {
			commandBuffers.at(0)->begin({ .flags{} });

			{
				std::array<vk::BufferCopy2, 1> a_regions{ {{
					.srcOffset{0},
					.dstOffset{0},
					.size{std::get<2>(binding0).memoryRequirements.size},
				}} };

				commandBuffers.at(0)->copyBuffer2({
					.srcBuffer{std::get<0>(bounce_in).get()},
					.dstBuffer{std::get<0>(binding0).get()},
					.regionCount{a_regions.size()},
					.pRegions{a_regions.data()},
				});

				std::array<vk::BufferMemoryBarrier2, 1> a_bufferMemoryBarrier{ {{
					.srcStageMask{vk::PipelineStageFlagBits2::eTransfer},
					.srcAccessMask{vk::AccessFlagBits2::eTransferWrite},
					.dstStageMask{vk::PipelineStageFlagBits2::eComputeShader},
					.dstAccessMask{vk::AccessFlagBits2::eShaderStorageRead},
					.srcQueueFamilyIndex{},
					.dstQueueFamilyIndex{},
					.buffer{std::get<0>(binding0).get()},
					.offset{0},
					.size{VK_WHOLE_SIZE},
				}} };

				commandBuffers.at(0)->pipelineBarrier2({
					.dependencyFlags{},
					.bufferMemoryBarrierCount{a_bufferMemoryBarrier.size()},
					.pBufferMemoryBarriers{a_bufferMemoryBarrier.data()},
				});

			}

			commandBuffers.at(0)->bindPipeline(vk::PipelineBindPoint::eCompute, computePipelines.at(0).get());

			std::vector<vk::DescriptorSet> v_descriptorSets;
			v_descriptorSets.reserve(descriptorSets.size());
			std::ranges::transform(descriptorSets, std::back_inserter(v_descriptorSets), [](vk::UniqueDescriptorSet& uds)->vk::DescriptorSet { return uds.get(); });

			commandBuffers.at(0)->bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout.get(), 0, v_descriptorSets, {});
			std::array<uint32_t, 1> mask{ 0xff0000ffu };
			commandBuffers.at(0)->pushConstants(pipelineLayout.get(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(mask), mask.data());
			commandBuffers.at(0)->resetQueryPool(queryPool.get(), 0, 2);
			commandBuffers.at(0)->writeTimestamp(vk::PipelineStageFlagBits::eComputeShader, queryPool.get(), 0);
			commandBuffers.at(0)->dispatch(1, 1, 1);
			commandBuffers.at(0)->writeTimestamp(vk::PipelineStageFlagBits::eComputeShader, queryPool.get(), 1);

			{
				std::array<vk::BufferMemoryBarrier2, 1> a_bufferMemoryBarrier{ {{
					.srcStageMask{vk::PipelineStageFlagBits2::eComputeShader},
					.srcAccessMask{vk::AccessFlagBits2::eShaderStorageWrite},
					.dstStageMask{vk::PipelineStageFlagBits2::eTransfer},
					.dstAccessMask{vk::AccessFlagBits2::eTransferRead},
					.srcQueueFamilyIndex{},
					.dstQueueFamilyIndex{},
					.buffer{std::get<0>(binding1).get()},
					.offset{0},
					.size{VK_WHOLE_SIZE},
				}} };

				commandBuffers.at(0)->pipelineBarrier2({
					.dependencyFlags{},
					.bufferMemoryBarrierCount{a_bufferMemoryBarrier.size()},
					.pBufferMemoryBarriers{a_bufferMemoryBarrier.data()},
					});

				std::array<vk::BufferCopy2, 1> a_regions{ {{
					.srcOffset{0},
					.dstOffset{0},
					.size{std::get<2>(binding0).memoryRequirements.size},
				}} };

				commandBuffers.at(0)->copyBuffer2({
					.srcBuffer{std::get<0>(binding1).get()},
					.dstBuffer{std::get<0>(bounce_out).get()},
					.regionCount{a_regions.size()},
					.pRegions{a_regions.data()},
				});
			}

			commandBuffers.at(0)->end();
		}

		inline void submit() noexcept {
			std::vector<vk::CommandBuffer> v_command_buffers;
			v_command_buffers.reserve(commandBuffers.size());
			std::ranges::transform(commandBuffers, std::back_inserter(v_command_buffers), [](vk::UniqueCommandBuffer& commandBuffer)-> vk::CommandBuffer { return commandBuffer.get(); });

			queue.submit({ {{
				.commandBufferCount{static_cast<uint32_t>(v_command_buffers.size())},
				.pCommandBuffers{v_command_buffers.data()},
			}} });
		}

		inline static vk::Queue make_queue(vk::UniqueDevice& device, std::span<vk::QueueFamilyProperties2> queue_families) noexcept {
			return device->getQueue2({
				.queueFamilyIndex{selectQueueFamilyIndex(queue_families)},
				.queueIndex{0},
			});
		}

		inline void waitIdle() noexcept {
			queue.waitIdle();
		}

		inline std::vector<uint64_t> get_queries_results() noexcept {
			return device->getQueryPoolResults<uint64_t>(queryPool.get(), 0, 2, sizeof(std::array<uint64_t, 2>), sizeof(uint64_t), vk::QueryResultFlagBits::e64 | vk::QueryResultFlagBits::eWait).value;
		}

		inline std::chrono::nanoseconds get_run_time() noexcept {
			auto query_results{ get_queries_results() };
			return static_cast<std::chrono::nanoseconds>(static_cast<int64_t>(static_cast<double>(query_results[1] - query_results[0]) * static_cast<double>(physical_device_properties.get<vk::PhysicalDeviceProperties2>().properties.limits.timestampPeriod)));
		}

		inline void show_features() const noexcept {
			std::cout << std::format("deviceName {}\n", static_cast<std::string_view>(physical_device_properties.get<vk::PhysicalDeviceProperties2>().properties.deviceName));
#ifdef _DEBUG
			std::cout << std::format("synchronization2: {}\n", physical_device_features.get<vk::PhysicalDeviceVulkan13Features>().synchronization2);
			std::cout << std::format("heaps: {}\n", memory_properties.memoryProperties.memoryHeapCount);
			std::cout << std::format("types: {}\n", memory_properties.memoryProperties.memoryTypeCount);
			std::cout << std::format("queue_families: {}\n", queue_families.size());
			std::cout << std::format("device: 0x{:x}\n", std::bit_cast<uintptr_t>(device.get()));
#endif
		}

		inline SingleDevice(vk::PhysicalDevice physical_device) noexcept :
			physical_device{ physical_device },
			physical_device_properties{ physical_device.getProperties2<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceExternalMemoryHostPropertiesEXT>() },
			physical_device_features{ physical_device.getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features, vk::PhysicalDeviceVulkan12Features, vk::PhysicalDeviceVulkan13Features>() },
			memory_properties{ physical_device.getMemoryProperties2() },
			memory_types{ std::span(memory_properties.memoryProperties.memoryTypes).first(memory_properties.memoryProperties.memoryTypeCount) },
			queue_families{ physical_device.getQueueFamilyProperties2() },
			queueFamilyIndices{ make_v_queue_family(queue_families) },
			device{ create_device(physical_device, queue_families) },
			binding0{ make_buffer_binding0(device, queueFamilyIndices, memory_types) },
			binding1{ make_buffer_binding1(device, queueFamilyIndices, memory_types) },
			mmf_bounce_in{ "bounce_in.bin" },
			bounce_in{ make_buffer_bounce_in(device, queueFamilyIndices, memory_types, physical_device_properties) },
			bounce_out{ make_buffer_bounce_out(device, queueFamilyIndices, memory_types) },
			simple_comp{make_shader_module(device, "simple.comp.glsl.spv")},
			descriptorSetLayout{make_descriptor_set_layout<binding_count>(device) },
			pipelineLayout{make_pipeline_layout(device, descriptorSetLayout)},
			pipelineCache{ label(device, "pipelineCache", device->createPipelineCacheUnique(vk::PipelineCacheCreateInfo{}).value)},
			computePipelines{make_compute_pipelines(device, simple_comp, pipelineLayout, pipelineCache) },
			descriptorPool{make_descriptor_pool(device)},
			descriptorSets{ make_descriptor_sets(device, descriptorSetLayout, descriptorPool) },
			queryPool{ make_query_pool(device) },
			commandPool{ make_command_pool(device, queue_families) },
			commandBuffers{make_command_buffers(device, commandPool)},
			queue{make_queue(device, queue_families)}
		{

			show_features();

			updateDescriptorSets();

			record_command_buffer();

		}

		inline ~SingleDevice() {
			if (std::get<3>(bounce_in)) {
#ifdef _MSC_VER
				_aligned_free(std::get<3>(bounce_in));
#else
				std::free(std::get<3>(bounce_in));
#endif
			}
		}

		inline std::chrono::nanoseconds do_steps() {
			setup_bounce_in(device, bounce_in, mmf_bounce_in);

			submit();

			waitIdle();

			check_bounce_out(device, bounce_out);

			auto run_time{ get_run_time() };
			std::cout << std::format("query_diff: {}\n", run_time);
			return run_time;
		}

		inline static void use_physical_device(vk::PhysicalDevice physical_device) noexcept {
			SingleDevice sd{ physical_device };
			sd.do_steps();
		}
	};
	void init() noexcept {
		VULKAN_HPP_DEFAULT_DISPATCHER.init();
		{
			MemoryMappedFile mmf_bounce_in{ "bounce_in.bin", 1024ull * 1024ull };
			auto s_bounce_in{ mmf_bounce_in.get_span<uint8_t>() };
			std::ranges::fill(s_bounce_in, 0x55);
		}

		auto instance_version{ vk::enumerateInstanceVersion() };
#ifdef _DEBUG
		std::cout << std::format("instance_version: 0x{:08x}\n", instance_version.value);
#endif

		auto instance_layer_properties{ vk::enumerateInstanceLayerProperties().value };
#ifdef _DEBUG
		std::cout << std::format("Instance::count_layers().value(): {}\n", instance_layer_properties.size());
#endif

		auto instance_extensions{ vk::enumerateInstanceExtensionProperties().value };
#ifdef _DEBUG
		std::cout << std::format("instance_extensions: {}\n", instance_extensions.size() );
#endif

		vk::ApplicationInfo applicationInfo{
			.pApplicationName{__FILE__},
			.apiVersion{VK_HEADER_VERSION_COMPLETE},
		};

		vk::InstanceCreateInfo instanceCreateInfo;
		instanceCreateInfo.setPApplicationInfo(&applicationInfo);

		std::array<const char* const, 1> instanceExtensions{ "VK_EXT_debug_utils" };
		instanceCreateInfo.setPEnabledExtensionNames(instanceExtensions);
		auto instance{ vk::createInstanceUnique(instanceCreateInfo).value };
		VULKAN_HPP_DEFAULT_DISPATCHER.init(instance.get());
#ifdef _DEBUG
		std::cout << std::format("instance: 0x{:x}\n", std::bit_cast<uintptr_t>(instance.get()));
#endif

		auto physical_devices{ instance->enumeratePhysicalDevices().value };

#ifdef _DEBUG
		std::cout << std::format("physical_devices.size(): {}\n", physical_devices.size());
#endif
		std::ranges::for_each(physical_devices, SingleDevice::use_physical_device);
	}
};

