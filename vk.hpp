#pragma once

#include "vu.hpp"
#include "each.h"

namespace vk_route {
	class SingleDevice {
	public:
		vk::PhysicalDevice physical_device;
		vk::StructureChain<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceExternalMemoryHostPropertiesEXT> physical_device_properties;
		vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features, vk::PhysicalDeviceVulkan12Features, vk::PhysicalDeviceVulkan13Features> physical_device_features;
		vk::StructureChain<vk::PhysicalDeviceMemoryProperties2, vk::PhysicalDeviceMemoryBudgetPropertiesEXT> memory_properties;
		std::span<vk::MemoryType> memory_types;
		std::vector<vk::QueueFamilyProperties2> queue_families;
		std::vector<uint32_t> queueFamilyIndices;
		vk::UniqueDevice device;
		UniqueDedicatedMemoryBuffer binding0;
		UniqueDedicatedMemoryBuffer binding1;
		MemoryMappedFile mmf_bounce_in;
		HostMemoryBuffer bounce_in;
		HostMemoryBuffer bounce_out;
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

		inline void record_command_buffer(vk::UniqueCommandBuffer &commandBuffer) noexcept {
			commandBuffer->begin({ .flags{} });

			{
				std::array<vk::BufferCopy2, 1> a_regions{ {{
					.srcOffset{0},
					.dstOffset{0},
					.size{std::get<2>(binding0).memoryRequirements.size},
				}} };

				commandBuffer->copyBuffer2({
					.srcBuffer{bounce_in.buffer.get()},
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

				commandBuffer->pipelineBarrier2({
					.dependencyFlags{},
					.bufferMemoryBarrierCount{a_bufferMemoryBarrier.size()},
					.pBufferMemoryBarriers{a_bufferMemoryBarrier.data()},
				});

			}

			commandBuffer->bindPipeline(vk::PipelineBindPoint::eCompute, computePipelines.at(0).get());

			std::vector<vk::DescriptorSet> v_descriptorSets;
			v_descriptorSets.reserve(descriptorSets.size());
			std::ranges::transform(descriptorSets, std::back_inserter(v_descriptorSets), [](vk::UniqueDescriptorSet& uds)->vk::DescriptorSet { return uds.get(); });

			commandBuffer->bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout.get(), 0, v_descriptorSets, {});
			std::array<uint32_t, 1> mask{ 0xff0000ffu };
			commandBuffer->pushConstants(pipelineLayout.get(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(mask), mask.data());
			commandBuffer->resetQueryPool(queryPool.get(), 0, 2);
			commandBuffer->writeTimestamp2(vk::PipelineStageFlagBits2::eComputeShader, queryPool.get(), 0);
			commandBuffer->dispatchBase(1, 0, 0, 1, 1, 1);
			commandBuffer->writeTimestamp2(vk::PipelineStageFlagBits2::eComputeShader, queryPool.get(), 1);

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

				commandBuffer->pipelineBarrier2({
					.dependencyFlags{},
					.bufferMemoryBarrierCount{a_bufferMemoryBarrier.size()},
					.pBufferMemoryBarriers{a_bufferMemoryBarrier.data()},
					});

				std::array<vk::BufferCopy2, 1> a_regions{ {{
					.srcOffset{0},
					.dstOffset{0},
					.size{std::get<2>(binding0).memoryRequirements.size},
				}} };

				commandBuffer->copyBuffer2({
					.srcBuffer{std::get<0>(binding1).get()},
					.dstBuffer{bounce_out.buffer.get()},
					.regionCount{a_regions.size()},
					.pRegions{a_regions.data()},
				});
			}

			commandBuffer->end();
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
			auto heaps{ std::span(memory_properties.get<vk::PhysicalDeviceMemoryProperties2>().memoryProperties.memoryHeaps).first(memory_properties.get<vk::PhysicalDeviceMemoryProperties2>().memoryProperties.memoryHeapCount) };
			auto types{ std::span(memory_properties.get<vk::PhysicalDeviceMemoryProperties2>().memoryProperties.memoryTypes).first(memory_properties.get<vk::PhysicalDeviceMemoryProperties2>().memoryProperties.memoryTypeCount) };
			auto budgets{ std::span(memory_properties.get<vk::PhysicalDeviceMemoryBudgetPropertiesEXT>().heapBudget).first(memory_properties.get<vk::PhysicalDeviceMemoryProperties2>().memoryProperties.memoryHeapCount) };
			auto usages{ std::span(memory_properties.get<vk::PhysicalDeviceMemoryBudgetPropertiesEXT>().heapUsage).first(memory_properties.get<vk::PhysicalDeviceMemoryProperties2>().memoryProperties.memoryHeapCount) };
			each<uint32_t>(types, [&](uint32_t type_index, const vk::MemoryType& type) {
				decltype(auto) heap{ heaps[type.heapIndex] };
				std::cout << std::format("heap[{}]: size: {:.3f} GiB, budget: {:.3f} GiB, usage: {:.3f} GiB, heapFlags: {}, type[{}]: typeFlags: {} \n",
					type.heapIndex,
					std::scalbln(static_cast<double>(heap.size), -30l),
					std::scalbln(static_cast<double>(budgets[type.heapIndex]), -30l),
					std::scalbln(static_cast<double>(usages[type.heapIndex]), -30l),
					vk::to_string(heap.flags),
					type_index,
					vk::to_string(type.propertyFlags)
				);
			});
			std::cout << std::format("heaps: {}\n", memory_properties.get<vk::PhysicalDeviceMemoryProperties2>().memoryProperties.memoryHeapCount);
			std::cout << std::format("types: {}\n", memory_properties.get<vk::PhysicalDeviceMemoryProperties2>().memoryProperties.memoryTypeCount);
			std::cout << std::format("queue_families: {}\n", queue_families.size());
			std::cout << std::format("device: 0x{:x}\n", std::bit_cast<uintptr_t>(device.get()));
#endif
		}

		inline SingleDevice(vk::PhysicalDevice physical_device) noexcept :
			physical_device{ physical_device },
			physical_device_properties{ physical_device.getProperties2<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceExternalMemoryHostPropertiesEXT>() },
			physical_device_features{ physical_device.getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features, vk::PhysicalDeviceVulkan12Features, vk::PhysicalDeviceVulkan13Features>() },
			memory_properties{ physical_device.getMemoryProperties2<vk::PhysicalDeviceMemoryProperties2, vk::PhysicalDeviceMemoryBudgetPropertiesEXT>() },
			memory_types{ std::span(memory_properties.get<vk::PhysicalDeviceMemoryProperties2>().memoryProperties.memoryTypes).first(memory_properties.get<vk::PhysicalDeviceMemoryProperties2>().memoryProperties.memoryTypeCount) },
			queue_families{ physical_device.getQueueFamilyProperties2() },
			queueFamilyIndices{ vu::make_v_queue_family(queue_families) },
			device{ vu::create_device(physical_device, queue_families) },
			binding0{ vu::make_binding_buffer(device, queueFamilyIndices, memory_types, 1024ull * 1024ull, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst, "binding0") },
			binding1{ vu::make_binding_buffer(device, queueFamilyIndices, memory_types, 1024ull * 1024ull, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferSrc, "binding1") },
			mmf_bounce_in{ "bounce_in.bin" },
			bounce_in{ device, queueFamilyIndices, memory_types, physical_device_properties, 1024ull * 1024ull, vk::BufferUsageFlagBits::eTransferSrc, "bounce_in" },
			bounce_out{ device, queueFamilyIndices, memory_types, physical_device_properties, 1024ull * 1024ull, vk::BufferUsageFlagBits::eTransferDst, "bounce_out" },
			simple_comp{ vu::make_shader_module(device, "simple.comp.glsl.spv")},
			descriptorSetLayout{ vu::make_descriptor_set_layout<binding_count>(device) },
			pipelineLayout{ vu::make_pipeline_layout(device, descriptorSetLayout)},
			pipelineCache{ label(device, "pipelineCache", device->createPipelineCacheUnique(vk::PipelineCacheCreateInfo{}).value)},
			computePipelines{ vu::make_compute_pipelines(device, simple_comp, pipelineLayout, pipelineCache) },
			descriptorPool{ vu::make_descriptor_pool(device, binding_count)},
			descriptorSets{ vu::make_descriptor_sets(device, descriptorSetLayout, descriptorPool) },
			queryPool{ vu::make_query_pool(device) },
			commandPool{ vu::make_command_pool(device, queue_families) },
			commandBuffers{ vu::make_command_buffers(device, commandPool)},
			queue{ vu::make_queue(device, queue_families)}
		{

			show_features();

			updateDescriptorSets();

			record_command_buffer(commandBuffers.at(0));

		}

		inline void setup_bounce_in() noexcept {
			auto bounce_in_mapped{ bounce_in.allocation.get_span<uint8_t>() };
#ifdef _DEBUG
			std::cout << std::format("bounce_in_mapped<T> count: {}, bytes: {}\n", bounce_in_mapped.size(), bounce_in_mapped.size_bytes());
#endif
			std::ranges::copy(mmf_bounce_in.get_span<uint8_t>(), bounce_in_mapped.begin());
		}

		inline void check_bounce_out() const noexcept {
			auto bounce_out_mapped{ bounce_out.allocation.get_span<uint32_t>() };
#ifdef _DEBUG
			std::cout << std::format("bounce_out_mapped<T> count: {}, bytes: {}\n", bounce_out_mapped.size(), bounce_out_mapped.size_bytes());
#endif
			bool is_unexpected{ false };
			for (auto b : bounce_out_mapped.first(512).last(256)) {
				if (b != 0xaaffffac) {
					std::cout << std::format("{:08x} ", b);
					is_unexpected = true;
				}
			}
			if (is_unexpected) std::cout << "\n";
		}

		inline std::chrono::nanoseconds do_steps() {
			setup_bounce_in();

			submit();

			waitIdle();

			check_bounce_out();

			auto run_time{ get_run_time() };
			std::cout << std::format("query_diff: {}\n", run_time);
			return run_time;
		}

		inline static void use_physical_device(vk::PhysicalDevice physical_device) noexcept {
			auto physical_device_extensions{ physical_device.enumerateDeviceExtensionProperties().value };
			auto contains_VK_KHR_shader_subgroup_uniform_control_flow{ std::ranges::contains(physical_device_extensions, true, [](vk::ExtensionProperties& extensionProperties)->bool { return std::string_view(extensionProperties.extensionName) == "VK_KHR_shader_subgroup_uniform_control_flow"; }) };
			if (!contains_VK_KHR_shader_subgroup_uniform_control_flow) {
				std::cout << "missing VK_KHR_shader_subgroup_uniform_control_flow\n";
				return;
			}

			auto physical_device_properties{ physical_device.getProperties2<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceExternalMemoryHostPropertiesEXT>() };
			auto physical_device_features{ physical_device.getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features, vk::PhysicalDeviceVulkan12Features, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceShaderSubgroupUniformControlFlowFeaturesKHR>() };

			if (!physical_device_features.get<vk::PhysicalDeviceShaderSubgroupUniformControlFlowFeaturesKHR>().shaderSubgroupUniformControlFlow) {
				std::cout << "missing shaderSubgroupUniformControlFlow\n";
				return;
			}

			if (!physical_device_features.get<vk::PhysicalDeviceVulkan13Features>().computeFullSubgroups) {
				std::cout << "missing computeFullSubgroups\n";
				return;
			}

			if (!physical_device_features.get<vk::PhysicalDeviceVulkan13Features>().synchronization2) {
				std::cout << "missing synchronization2\n";
				return;
			}

			SingleDevice sd{ physical_device };
			sd.do_steps();
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

#ifdef _DEBUG
		auto messenger{ instance->createDebugUtilsMessengerEXTUnique(vk::DebugUtilsMessengerCreateInfoEXT{
			.flags{},
			.messageSeverity{
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
			},
			.messageType{
				vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
				vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
				vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
				vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding
			},
			.pfnUserCallback{
				[](
					VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
					VkDebugUtilsMessageTypeFlagsEXT messageTypes,
					const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
					void* pUserData
				)->VkBool32 {
					std::cout << std::format("{}\n", pCallbackData->pMessage);
					return false;
				}},
			.pUserData{},
		}).value };
#endif

		auto physical_devices{ instance->enumeratePhysicalDevices().value };

#ifdef _DEBUG
		std::cout << std::format("physical_devices.size(): {}\n", physical_devices.size());
#endif
		std::ranges::for_each(physical_devices, SingleDevice::use_physical_device);
	}
};
