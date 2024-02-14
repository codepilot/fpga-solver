#pragma once

#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_NO_CONSTRUCTORS
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include <vulkan/vulkan.hpp>
#include <expected>

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

	class DeviceMemoryBuffer {
	public:
		vk::StructureChain<vk::BufferCreateInfo> bci;
		vk::StructureChain<vk::MemoryRequirements2, vk::MemoryDedicatedRequirements> requirements;
		vk::UniqueBuffer buffer;
		vk::StructureChain<vk::MemoryAllocateInfo, vk::MemoryDedicatedAllocateInfo, vk::MemoryAllocateFlagsInfo> allocateInfo;
		vk::UniqueDeviceMemory memory;
		vk::Result bindResult;
		vk::DeviceAddress address;

		static vk::StructureChain<vk::BufferCreateInfo> make_bci(std::span<uint32_t> queueFamilyIndices, vk::DeviceSize bufferSize, vk::BufferUsageFlags bufferUsage) noexcept {
			vk::StructureChain<vk::BufferCreateInfo> bci;
			bci.get<vk::BufferCreateInfo>().setSize(bufferSize);
			bci.get<vk::BufferCreateInfo>().setUsage(bufferUsage);
			bci.get<vk::BufferCreateInfo>().setSharingMode(vk::SharingMode::eExclusive);
			bci.get<vk::BufferCreateInfo>().setQueueFamilyIndices(queueFamilyIndices);
			return bci;
		}

		static vk::StructureChain<vk::MemoryAllocateInfo, vk::MemoryDedicatedAllocateInfo, vk::MemoryAllocateFlagsInfo> make_allocate_info(
			vk::UniqueDevice& device,
			std::span<vk::MemoryType> memory_types,
			vk::StructureChain<vk::MemoryRequirements2, vk::MemoryDedicatedRequirements>& requirements,
			vk::UniqueBuffer &buffer
		) noexcept {

			vk::StructureChain<vk::MemoryAllocateInfo, vk::MemoryDedicatedAllocateInfo, vk::MemoryAllocateFlagsInfo> allocateInfo;
			allocateInfo.get<vk::MemoryAllocateInfo>().setAllocationSize(requirements.get<vk::MemoryRequirements2>().memoryRequirements.size);
			VkMemoryPropertyFlags needed_flags{ VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
			allocateInfo.get<vk::MemoryAllocateInfo>().memoryTypeIndex = static_cast<uint32_t>(
				std::distance(
					memory_types.begin(),
					std::ranges::find_if(
						memory_types,
						[&](const VkMemoryType& memoryType)-> bool { return (memoryType.propertyFlags & needed_flags) == needed_flags; }
					)
				)
			);

			allocateInfo.get<vk::MemoryAllocateFlagsInfo>().setFlags(vk::MemoryAllocateFlagBits::eDeviceAddress);
			allocateInfo.get<vk::MemoryDedicatedAllocateInfo>().setBuffer(buffer.get());
			return allocateInfo;
		}

		DeviceMemoryBuffer(
			vk::UniqueDevice& device,
			std::span<uint32_t> queueFamilyIndices,
			std::span<vk::MemoryType> memory_types,
			vk::StructureChain<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceExternalMemoryHostPropertiesEXT>& physical_device_properties,
			vk::DeviceSize bufferSize,
			vk::BufferUsageFlags bufferUsage,
			std::string bufferLabel
		) noexcept :
			bci{ make_bci(queueFamilyIndices, bufferSize, bufferUsage) },
			requirements{ device->getBufferMemoryRequirements<vk::MemoryRequirements2, vk::MemoryDedicatedRequirements>(vk::DeviceBufferMemoryRequirements{.pCreateInfo{&bci.get<vk::BufferCreateInfo>()} }) },
			buffer{ label(device, bufferLabel, device->createBufferUnique(bci.get<vk::BufferCreateInfo>()).value) },
			allocateInfo{ make_allocate_info(device, memory_types, requirements, buffer) },
			memory{ label(device, bufferLabel, device->allocateMemoryUnique(allocateInfo.get<vk::MemoryAllocateInfo>()).value) },
			bindResult{ device->bindBufferMemory2({ {.buffer{buffer.get()}, .memory{memory.get()}, .memoryOffset{0ull} } }) },
			address{ device->getBufferAddress({.buffer{buffer.get()}})}
		{
#ifdef _DEBUG
			std::cout << std::format("{} mem_requirements: {}\n", bufferLabel, requirements.get<vk::MemoryRequirements2>().memoryRequirements.size);
#endif
		}
	};

	class HostMemoryBuffer {
	public:
		vk::StructureChain<vk::BufferCreateInfo, vk::ExternalMemoryBufferCreateInfo> bci;
		vk::MemoryRequirements2 requirements;
		AlignedAllocation allocation;
		vk::StructureChain<vk::MemoryAllocateInfo, vk::ImportMemoryHostPointerInfoEXT> allocateInfo;
		vk::UniqueDeviceMemory memory;
		vk::UniqueBuffer buffer;

		static vk::StructureChain<vk::BufferCreateInfo, vk::ExternalMemoryBufferCreateInfo> make_bci(std::span<uint32_t> queueFamilyIndices, vk::DeviceSize bufferSize, vk::BufferUsageFlags bufferUsage) noexcept {
			vk::StructureChain<vk::BufferCreateInfo, vk::ExternalMemoryBufferCreateInfo> bci;
			bci.get<vk::BufferCreateInfo>().setSize(bufferSize);
			bci.get<vk::BufferCreateInfo>().setUsage(bufferUsage);
			bci.get<vk::BufferCreateInfo>().setSharingMode(vk::SharingMode::eExclusive);
			bci.get<vk::BufferCreateInfo>().setQueueFamilyIndices(queueFamilyIndices);
			bci.get<vk::ExternalMemoryBufferCreateInfo>().setHandleTypes(vk::ExternalMemoryHandleTypeFlagBits::eHostAllocationEXT);
			return bci;
		}

		static vk::StructureChain<vk::MemoryAllocateInfo, vk::ImportMemoryHostPointerInfoEXT> make_allocate_info(vk::UniqueDevice& device, std::span<vk::MemoryType> memory_types, vk::MemoryRequirements2& requirements, AlignedAllocation& allocation) noexcept {
			vk::StructureChain<vk::MemoryAllocateInfo, vk::ImportMemoryHostPointerInfoEXT> allocateInfo;
			allocateInfo.get<vk::MemoryAllocateInfo>().setAllocationSize(requirements.memoryRequirements.size);
			auto hostPointerProperties{ device->getMemoryHostPointerPropertiesEXT(vk::ExternalMemoryHandleTypeFlagBits::eHostAllocationEXT, allocation.pointer).value };
			allocateInfo.get<vk::MemoryAllocateInfo>().memoryTypeIndex = static_cast<uint32_t>(
				std::distance(
					memory_types.begin(),
					std::ranges::find_if(
						memory_types,
						[&](const VkMemoryType& memoryType)-> bool { return (memoryType.propertyFlags & hostPointerProperties.memoryTypeBits) == hostPointerProperties.memoryTypeBits; }
					)
				)
				);
			allocateInfo.get<vk::ImportMemoryHostPointerInfoEXT>().setHandleType(vk::ExternalMemoryHandleTypeFlagBits::eHostAllocationEXT);
			allocateInfo.get<vk::ImportMemoryHostPointerInfoEXT>().setPHostPointer(allocation.pointer);
			return allocateInfo;
		}

		HostMemoryBuffer(
			vk::UniqueDevice& device,
			std::span<uint32_t> queueFamilyIndices,
			std::span<vk::MemoryType> memory_types,
			vk::StructureChain<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceExternalMemoryHostPropertiesEXT>& physical_device_properties,
			vk::DeviceSize bufferSize,
			vk::BufferUsageFlags bufferUsage,
			std::string bufferLabel
		) noexcept :
			bci{ make_bci(queueFamilyIndices, bufferSize, bufferUsage) },
			requirements{ device->getBufferMemoryRequirements(vk::DeviceBufferMemoryRequirements{.pCreateInfo{&bci.get<vk::BufferCreateInfo>()} }) },
			allocation{ physical_device_properties.get<vk::PhysicalDeviceExternalMemoryHostPropertiesEXT>().minImportedHostPointerAlignment , requirements.memoryRequirements.size },
			allocateInfo{ make_allocate_info(device, memory_types, requirements, allocation) },
			memory{ label(device, bufferLabel, device->allocateMemoryUnique(allocateInfo.get<vk::MemoryAllocateInfo>()).value) },
			buffer{ label(device, bufferLabel, device->createBufferUnique(bci.get<vk::BufferCreateInfo>()).value) }
		{
#ifdef _DEBUG
			std::cout << std::format("{} mem_requirements: {}\n", bufferLabel, requirements.memoryRequirements.size);
#endif

			device->bindBufferMemory2({ {
				.buffer{buffer.get()},
				.memory{memory.get()},
				.memoryOffset{0ull},
			} });
		}
	};

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

			vk::StructureChain<
				vk::DeviceCreateInfo,
				vk::PhysicalDeviceVulkan12Features,
				// vk::PhysicalDeviceShaderSubgroupUniformControlFlowFeaturesKHR,
				vk::PhysicalDeviceVulkan13Features> deviceCreateInfo;
			deviceCreateInfo.get<vk::DeviceCreateInfo>().setQueueCreateInfoCount(static_cast<uint32_t>(v_queue_create_info.size()));
			deviceCreateInfo.get<vk::DeviceCreateInfo>().setQueueCreateInfos(v_queue_create_info);
			deviceCreateInfo.get<vk::PhysicalDeviceVulkan13Features>().setSynchronization2(true);
			deviceCreateInfo.get<vk::PhysicalDeviceVulkan13Features>().setComputeFullSubgroups(true);
			deviceCreateInfo.get<vk::PhysicalDeviceVulkan12Features>().setBufferDeviceAddress(true);
			std::vector<const char*> enabled_extensions;
			enabled_extensions.emplace_back("VK_EXT_external_memory_host");
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

		inline static uint32_t selectQueueFamilyIndex(std::span<vk::QueueFamilyProperties2> queue_families) {
			return static_cast<uint32_t>(std::distance(queue_families.begin(), std::ranges::find(queue_families, VK_QUEUE_COMPUTE_BIT, [](VkQueueFamilyProperties2& props)-> VkQueueFlagBits {
				return std::bit_cast<VkQueueFlagBits>(props.queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT);
				})));
		}

		inline static vk::UniqueCommandPool make_command_pool(vk::UniqueDevice& device, std::span<vk::QueueFamilyProperties2> queue_families) noexcept {
			return label(device, "commandPool", device->createCommandPoolUnique({ .queueFamilyIndex{selectQueueFamilyIndex(queue_families)} }).value);
		}

		inline static auto make_command_buffers(vk::UniqueDevice& device, vk::UniqueCommandPool& commandPool) noexcept {
			return labels(device, "commandBuffers", device->allocateCommandBuffersUnique({
				.commandPool{commandPool.get()},
				.level{vk::CommandBufferLevel::ePrimary},
				.commandBufferCount{1},
				}).value);
		}

		inline static vk::Queue make_queue(vk::UniqueDevice& device, std::span<vk::QueueFamilyProperties2> queue_families) noexcept {
			return device->getQueue2({
				.queueFamilyIndex{selectQueueFamilyIndex(queue_families)},
				.queueIndex{0},
				});
		}

	};

};
