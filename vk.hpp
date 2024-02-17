#pragma once

#include "vu.hpp"
#include "each.h"

namespace vk_route {
	class SingleDevice {
	public:
		inline static constexpr uint64_t u64_compute_start{ 0ull };
		inline static constexpr uint64_t u64_compute_done{ 1ull };
		inline static constexpr uint64_t u64_compute_steps{ 1ull };
		vk::PhysicalDevice physical_device;
		vk::StructureChain<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceExternalMemoryHostPropertiesEXT> physical_device_properties;
		vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features, vk::PhysicalDeviceVulkan12Features, vk::PhysicalDeviceVulkan13Features> physical_device_features;
		vk::StructureChain<vk::PhysicalDeviceMemoryProperties2, vk::PhysicalDeviceMemoryBudgetPropertiesEXT> memory_properties;
		std::span<vk::MemoryHeap> memory_heaps;
		std::span<vk::MemoryType> memory_types;
		std::vector<vk::QueueFamilyProperties2> queue_families;
		uint32_t qfi_compute;
		uint32_t qfi_transfer;
		std::vector<uint32_t> queueFamilyIndices;
		vk::UniqueDevice device;
		DeviceMemoryBuffer binding0;
		DeviceMemoryBuffer binding1;
		MemoryMappedFile mmf_bounce_in;
		HostMemoryBuffer bounce_in;
		HostMemoryBuffer bounce_out;
		vk::UniqueShaderModule simple_comp;
		vk::UniqueDescriptorSetLayout descriptorSetLayout;
		vk::UniquePipelineLayout pipelineLayout;
		vk::UniquePipelineCache pipelineCache;
		std::vector<vk::UniquePipeline> computePipelines;
		// vk::UniqueDescriptorPool descriptorPool;
		// std::vector<vk::UniqueDescriptorSet> descriptorSets;
		vk::UniqueQueryPool queryPool;
		vk::UniqueCommandPool cp_compute;
		vk::UniqueCommandPool cp_transfer;
		std::vector<vk::UniqueCommandBuffer> vcb_compute;
		std::vector<vk::UniqueCommandBuffer> vcb_transfer_in;
		std::vector<vk::UniqueCommandBuffer> vcb_transfer_out;
		std::vector<vk::Queue> compute_queues;
		std::vector<vk::Queue> transfer_queues;
		vk::UniqueSemaphore primary_timeline;

		inline static constexpr std::size_t general_buffer_size{ 1024ull * 1024ull * 1024ull };
		inline static constexpr std::size_t workgroup_size{ 1024ull };
		inline static constexpr std::size_t invocations_needed{ general_buffer_size / workgroup_size / sizeof(uint32_t) };
		inline static constexpr std::size_t max_dispatch_invocations{ 65536ull };
		inline static constexpr std::size_t dispatches_needed{ invocations_needed / max_dispatch_invocations };

#if 0
		inline void updateDescriptorSets() {
			std::array<vk::DescriptorBufferInfo, 1> binding0_bufferInfos{ {{.buffer{binding0.buffer.get()}, .offset{}, .range{VK_WHOLE_SIZE}}} };
			std::array<vk::DescriptorBufferInfo, 1> binding1_bufferInfos{ {{.buffer{binding1.buffer.get()}, .offset{}, .range{VK_WHOLE_SIZE}}} };

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
#endif

		inline void compute_cb_record(vk::UniqueCommandBuffer &commandBuffer) noexcept {
			commandBuffer->begin({ .flags{} });

			{
				std::array<vk::BufferCopy2, 1> a_regions{ {{
					.srcOffset{0},
					.dstOffset{0},
					.size{binding0.bci.get<vk::BufferCreateInfo>().size},
				}} };

				commandBuffer->copyBuffer2({
					.srcBuffer{bounce_in.buffer.get()},
					.dstBuffer{binding0.buffer.get()},
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
					.buffer{binding0.buffer.get()},
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
			// v_descriptorSets.reserve(descriptorSets.size());
			// std::ranges::transform(descriptorSets, std::back_inserter(v_descriptorSets), [](vk::UniqueDescriptorSet& uds)->vk::DescriptorSet { return uds.get(); });

			// commandBuffer->bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout.get(), 0, v_descriptorSets, {});
			vu::ShaderPushConstants spc{ .src_buf{binding0.address}, .dst_buf{binding1.address}, .multiplicand{0xff0000ffu} };
			commandBuffer->pushConstants(pipelineLayout.get(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(spc), &spc);
			commandBuffer->resetQueryPool(queryPool.get(), 0, 2);
			commandBuffer->writeTimestamp2(vk::PipelineStageFlagBits2::eComputeShader, queryPool.get(), 0);
			commandBuffer->dispatchBase(0, 0, 0, max_dispatch_invocations, dispatches_needed, 1);
			commandBuffer->writeTimestamp2(vk::PipelineStageFlagBits2::eComputeShader, queryPool.get(), 1);

			{
				std::array<vk::BufferMemoryBarrier2, 1> a_bufferMemoryBarrier{ {{
					.srcStageMask{vk::PipelineStageFlagBits2::eComputeShader},
					.srcAccessMask{vk::AccessFlagBits2::eShaderStorageWrite},
					.dstStageMask{vk::PipelineStageFlagBits2::eTransfer},
					.dstAccessMask{vk::AccessFlagBits2::eTransferRead},
					.srcQueueFamilyIndex{},
					.dstQueueFamilyIndex{},
					.buffer{binding1.buffer.get()},
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
					.size{binding0.bci.get<vk::BufferCreateInfo>().size},
				}} };

				commandBuffer->copyBuffer2({
					.srcBuffer{binding1.buffer.get()},
					.dstBuffer{bounce_out.buffer.get()},
					.regionCount{a_regions.size()},
					.pRegions{a_regions.data()},
				});
			}

			commandBuffer->end();
		}

		inline void submit(uint64_t step_index) noexcept {
			std::vector<vk::CommandBufferSubmitInfo> command_buffer_submit_infos;
			command_buffer_submit_infos.reserve(vcb_compute.size());
			std::ranges::transform(vcb_compute, std::back_inserter(command_buffer_submit_infos), [](vk::UniqueCommandBuffer& commandBuffer)-> vk::CommandBufferSubmitInfo {
				vk::CommandBufferSubmitInfo submitInfo;
				submitInfo.setCommandBuffer(commandBuffer.get());
				return submitInfo;
			});

			std::array<vk::SemaphoreSubmitInfo, 1> waitSemaphoreSubmitInfo{ {{.semaphore{primary_timeline.get()}, .value{step_index * u64_compute_steps + u64_compute_start}, .stageMask{vk::PipelineStageFlagBits2::eComputeShader | vk::PipelineStageFlagBits2::eCopy }}} };
			std::array<vk::SemaphoreSubmitInfo, 1> signalSemaphoreSubmitInfo{ {{.semaphore{primary_timeline.get()}, .value{step_index * u64_compute_steps + u64_compute_done}, .stageMask{vk::PipelineStageFlagBits2::eComputeShader | vk::PipelineStageFlagBits2::eCopy}}} };

			std::array<vk::SubmitInfo2, 1> submitInfos{};
			submitInfos.front().setCommandBufferInfos(command_buffer_submit_infos);
			submitInfos.front().setWaitSemaphoreInfos(waitSemaphoreSubmitInfo);
			submitInfos.front().setSignalSemaphoreInfos(signalSemaphoreSubmitInfo);

			compute_queues.front().submit2(submitInfos);
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
			for (auto& queue_props : queue_families) {
				std::cout << std::format("count: {}, flags: {}\n", queue_props.queueFamilyProperties.queueCount, vk::to_string(queue_props.queueFamilyProperties.queueFlags));
			}
			std::cout << std::format("synchronization2: {}\n", physical_device_features.get<vk::PhysicalDeviceVulkan13Features>().synchronization2);
			auto heaps{ std::span(memory_properties.get<vk::PhysicalDeviceMemoryProperties2>().memoryProperties.memoryHeaps).first(memory_properties.get<vk::PhysicalDeviceMemoryProperties2>().memoryProperties.memoryHeapCount) };
			auto types{ std::span(memory_properties.get<vk::PhysicalDeviceMemoryProperties2>().memoryProperties.memoryTypes).first(memory_properties.get<vk::PhysicalDeviceMemoryProperties2>().memoryProperties.memoryTypeCount) };
			auto budgets{ std::span(memory_properties.get<vk::PhysicalDeviceMemoryBudgetPropertiesEXT>().heapBudget).first(memory_properties.get<vk::PhysicalDeviceMemoryProperties2>().memoryProperties.memoryHeapCount) };
			auto usages{ std::span(memory_properties.get<vk::PhysicalDeviceMemoryBudgetPropertiesEXT>().heapUsage).first(memory_properties.get<vk::PhysicalDeviceMemoryProperties2>().memoryProperties.memoryHeapCount) };
			each<uint32_t>(heaps, [&](uint32_t heap_index, const vk::MemoryHeap& heap) {
				each<uint32_t>(types, [&](uint32_t type_index, const vk::MemoryType& type) {
					if (heap_index != type.heapIndex) return;
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
			memory_heaps{ std::span(memory_properties.get<vk::PhysicalDeviceMemoryProperties2>().memoryProperties.memoryHeaps).first(memory_properties.get<vk::PhysicalDeviceMemoryProperties2>().memoryProperties.memoryHeapCount) },
			memory_types{ std::span(memory_properties.get<vk::PhysicalDeviceMemoryProperties2>().memoryProperties.memoryTypes).first(memory_properties.get<vk::PhysicalDeviceMemoryProperties2>().memoryProperties.memoryTypeCount) },
			queue_families{ physical_device.getQueueFamilyProperties2() },
			queueFamilyIndices{ qfi_compute, qfi_transfer },
			qfi_compute{ vu::get_compute_queue_families(queue_families).front() },
			qfi_transfer{ vu::get_transfer_queue_families(queue_families).front() },
			device{ vu::create_device(physical_device, queue_families, qfi_compute, qfi_transfer) },
			binding0{ device, queueFamilyIndices, memory_heaps, memory_types, physical_device_properties, general_buffer_size, vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eTransferDst, "binding0" },
			binding1{ device, queueFamilyIndices, memory_heaps, memory_types, physical_device_properties, general_buffer_size, vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eTransferSrc, "binding1" },
			mmf_bounce_in{ "bounce_in.bin" },
			bounce_in{ device, queueFamilyIndices, memory_heaps, memory_types, physical_device_properties, general_buffer_size, vk::BufferUsageFlagBits::eTransferSrc, "bounce_in" },
			bounce_out{ device, queueFamilyIndices, memory_heaps, memory_types, physical_device_properties, general_buffer_size, vk::BufferUsageFlagBits::eTransferDst, "bounce_out" },
			simple_comp{ vu::make_shader_module(device, "simple.comp.glsl.spv")},
			descriptorSetLayout{ vu::make_descriptor_set_layout<0>(device) },
			pipelineLayout{ vu::make_pipeline_layout(device, descriptorSetLayout)},
			pipelineCache{ label(device, "pipelineCache", device->createPipelineCacheUnique(vk::PipelineCacheCreateInfo{}).value)},
			computePipelines{ vu::make_compute_pipelines(device, simple_comp, pipelineLayout, pipelineCache) },
			// descriptorPool{ vu::make_descriptor_pool(device, 0)},
			// descriptorSets{ vu::make_descriptor_sets(device, descriptorSetLayout, descriptorPool) },
			queryPool{ vu::make_query_pool(device) },
			cp_compute{ label(device, "cp_compute", device->createCommandPoolUnique({.flags{vk::CommandPoolCreateFlagBits::eTransient}, .queueFamilyIndex{qfi_compute} }).value) },
			cp_transfer{ label(device, "cp_transfer", device->createCommandPoolUnique({.flags{vk::CommandPoolCreateFlagBits::eTransient}, .queueFamilyIndex{qfi_transfer} }).value) },
			vcb_compute{ labels(device, "vcb_compute", device->allocateCommandBuffersUnique({.commandPool{cp_compute.get()}, .level{vk::CommandBufferLevel::ePrimary}, .commandBufferCount{1}, }).value) },
			vcb_transfer_in{ labels(device, "vcb_transfer_in", device->allocateCommandBuffersUnique({.commandPool{cp_transfer.get()}, .level{vk::CommandBufferLevel::ePrimary}, .commandBufferCount{1}, }).value) },
			vcb_transfer_out{ labels(device, "vcb_transfer_out", device->allocateCommandBuffersUnique({.commandPool{cp_transfer.get()}, .level{vk::CommandBufferLevel::ePrimary}, .commandBufferCount{1}, }).value) },
			compute_queues{ vu::make_queues_from_v_index(device, queue_families, qfi_compute) },
			transfer_queues{ vu::make_queues_from_v_index(device, queue_families, qfi_transfer) },
			primary_timeline{ vu::make_timeline_semaphore(device, u64_compute_start) }
		{

			show_features();

			// updateDescriptorSets();

			compute_cb_record(vcb_compute.at(0));

		}

		inline void setup_bounce_in() noexcept {
			auto bounce_in_mapped{ bounce_in.get_span<uint8_t>() };
			std::ranges::copy(mmf_bounce_in.get_span<uint8_t>(), bounce_in_mapped.begin());
		}

		inline void check_bounce_out() const noexcept {
			auto bounce_out_mapped{ bounce_out.get_span<uint32_t>() };
			bool is_unexpected{ false };
			for (auto b : bounce_out_mapped) {
				if (b != 0xaaffffac) {
					std::cout << std::format("{:08x} ", b);
					is_unexpected = true;
				}
			}
			if (is_unexpected) std::cout << "\n";
		}

		inline std::chrono::nanoseconds do_steps(uint64_t step_index) {
			setup_bounce_in();

			submit(step_index);
			vk::SemaphoreWaitInfo semaphoreWaitInfo{};
			std::array<const vk::Semaphore, 1> waitSemaphores{primary_timeline.get()};
			std::array<uint64_t, waitSemaphores.size()> waitValues{ step_index * u64_compute_steps + u64_compute_done};
			semaphoreWaitInfo.setSemaphores(waitSemaphores);
			semaphoreWaitInfo.setValues(waitValues);
			device->waitSemaphores(semaphoreWaitInfo, UINT64_MAX);

			check_bounce_out();

			auto run_time{ get_run_time() };
			std::cout << std::format("query_diff: {}\n", std::chrono::duration_cast<std::chrono::milliseconds>(run_time));
			return run_time;
		}

		inline static void use_physical_device(vk::PhysicalDevice physical_device) noexcept {
			auto physical_device_extensions{ physical_device.enumerateDeviceExtensionProperties().value };
			
			// auto contains_VK_EXT_external_memory_host{ std::ranges::contains(physical_device_extensions, true, [](vk::ExtensionProperties& extensionProperties)->bool { return std::string_view(extensionProperties.extensionName) == "VK_EXT_external_memory_host"; }) };
			// if (!contains_VK_EXT_external_memory_host) {
			// 	std::cout << "missing VK_EXT_external_memory_host\n";
			// 	return;
			// }

			// auto contains_VK_KHR_shader_subgroup_uniform_control_flow{ std::ranges::contains(physical_device_extensions, true, [](vk::ExtensionProperties& extensionProperties)->bool { return std::string_view(extensionProperties.extensionName) == "VK_KHR_shader_subgroup_uniform_control_flow"; }) };
			// if (!contains_VK_KHR_shader_subgroup_uniform_control_flow) {
			// 	std::cout << "missing VK_KHR_shader_subgroup_uniform_control_flow\n";
			// 	return;
			// }

			auto physical_device_properties{ physical_device.getProperties2<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceExternalMemoryHostPropertiesEXT>() };
			auto physical_device_features{ physical_device.getFeatures2<
				vk::PhysicalDeviceFeatures2,
				// vk::PhysicalDeviceShaderSubgroupUniformControlFlowFeaturesKHR,
				vk::PhysicalDeviceVulkan11Features,
				vk::PhysicalDeviceVulkan12Features,
				vk::PhysicalDeviceVulkan13Features>() };

			// if (!physical_device_features.get<vk::PhysicalDeviceShaderSubgroupUniformControlFlowFeaturesKHR>().shaderSubgroupUniformControlFlow) {
			//	std::cout << "missing shaderSubgroupUniformControlFlow\n";
			//	return;
			//}

			if (!physical_device_features.get<vk::PhysicalDeviceVulkan13Features>().computeFullSubgroups) {
				std::cout << "missing computeFullSubgroups\n";
				return;
			}

			if (!physical_device_features.get<vk::PhysicalDeviceVulkan13Features>().synchronization2) {
				std::cout << "missing synchronization2\n";
				return;
			}

			SingleDevice sd{ physical_device };
			for (uint64_t step_index{}; step_index < 10; step_index++) {
				sd.do_steps(step_index);
			}
			// sd.do_steps();
		}
	};

	bool bounce_in_bin_exists() {
		MemoryMappedFile mmf_bounce_in{ "bounce_in.bin" };
		return mmf_bounce_in.fsize == SingleDevice::general_buffer_size;
	}

	void init() noexcept {
		VULKAN_HPP_DEFAULT_DISPATCHER.init();
		if(!bounce_in_bin_exists()) {
			MemoryMappedFile mmf_bounce_in{ "bounce_in.bin", SingleDevice::general_buffer_size };
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
