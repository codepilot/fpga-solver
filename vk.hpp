#pragma once

#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_NO_CONSTRUCTORS

#include <vulkan/vulkan.hpp>
#include <expected>

namespace vk_route {
	class SingleDevice {
	public:
		vk::PhysicalDevice physical_device;
		vk::PhysicalDeviceMemoryProperties2 memory_properties;
		std::span<vk::MemoryType> memory_types;
		std::vector<vk::QueueFamilyProperties2> queue_families;
		std::vector<uint32_t> queueFamilyIndices;
		vk::UniqueDevice device;
		using UniqueDedicatedMemoryBuffer = std::tuple<vk::UniqueBuffer, vk::UniqueDeviceMemory, vk::MemoryRequirements2>;
		UniqueDedicatedMemoryBuffer binding0;
		UniqueDedicatedMemoryBuffer binding1;
		UniqueDedicatedMemoryBuffer bounce_in;
		UniqueDedicatedMemoryBuffer bounce_out;
		vk::UniqueShaderModule simple_comp;
		vk::UniqueDescriptorSetLayout descriptorSetLayout;

		static std::vector<uint32_t> make_v_queue_family(std::span<vk::QueueFamilyProperties2> queue_families) noexcept {
			std::vector<uint32_t> queueFamilyIndices;
			each<uint32_t>(queue_families, [&](uint32_t queue_family_idx, VkQueueFamilyProperties2& queue_familiy_properties) noexcept {
				if ((queue_familiy_properties.queueFamilyProperties.queueFlags & VkQueueFlagBits::VK_QUEUE_COMPUTE_BIT) != VK_QUEUE_COMPUTE_BIT) {
					return;
				}
				queueFamilyIndices.emplace_back(queue_family_idx);
			});
			return queueFamilyIndices;
		}

		static vk::UniqueDevice create_device(vk::PhysicalDevice physical_device, std::span<vk::QueueFamilyProperties2> queue_families) {
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

			return physical_device.createDeviceUnique(deviceCreateInfo.get<vk::DeviceCreateInfo>()).value;
		}

		static UniqueDedicatedMemoryBuffer make_buffer_binding0(vk::UniqueDevice &device, std::span<uint32_t> queueFamilyIndices, std::span<vk::MemoryType> memory_types) {
			vk::BufferCreateInfo binding0_bci{
				.size{1024ull * 1024ull},
				.usage{vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst},
				.sharingMode{vk::SharingMode::eExclusive},
				.queueFamilyIndexCount{static_cast<uint32_t>(queueFamilyIndices.size())},
				.pQueueFamilyIndices{queueFamilyIndices.data()},
			};

			auto binding0_mem_requirements{ device->getBufferMemoryRequirements(vk::DeviceBufferMemoryRequirements{.pCreateInfo{&binding0_bci}}) };
			std::cout << std::format("binding0_mem_requirements: {}\n", binding0_mem_requirements.memoryRequirements.size);
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

			auto temp_buffer{ device->createBufferUnique(binding0_bci).value };
			binding0_ai.get<vk::MemoryDedicatedAllocateInfo>().buffer = temp_buffer.get();
			
			auto temp_memory{ device->allocateMemoryUnique(binding0_ai.get<vk::MemoryAllocateInfo>()).value };

			device->bindBufferMemory(temp_buffer.get(), temp_memory.get(), 0ull);

			return std::make_tuple<vk::UniqueBuffer, vk::UniqueDeviceMemory, vk::MemoryRequirements2>(
				std::move(temp_buffer),
				std::move(temp_memory),
				std::move(binding0_mem_requirements)
			);
		}

		static UniqueDedicatedMemoryBuffer make_buffer_binding1(vk::UniqueDevice& device, std::span<uint32_t> queueFamilyIndices, std::span<vk::MemoryType> memory_types) {
			vk::BufferCreateInfo binding1_bci{
				.size{1024ull * 1024ull},
				.usage{vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferSrc},
				.sharingMode{vk::SharingMode::eExclusive},
				.queueFamilyIndexCount{static_cast<uint32_t>(queueFamilyIndices.size())},
				.pQueueFamilyIndices{queueFamilyIndices.data()},
			};

			auto binding1_mem_requirements{ device->getBufferMemoryRequirements(vk::DeviceBufferMemoryRequirements{.pCreateInfo{&binding1_bci}}) };
			std::cout << std::format("binding1_mem_requirements: {}\n", binding1_mem_requirements.memoryRequirements.size);
			auto binding1_buffer{ device->createBufferUnique(binding1_bci).value };
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
			auto binding1_memory{ device->allocateMemoryUnique(binding1_ai.get<vk::MemoryAllocateInfo>()).value };
			device->bindBufferMemory(binding1_buffer.get(), binding1_memory.get(), 0ull);

			return std::make_tuple<vk::UniqueBuffer, vk::UniqueDeviceMemory, vk::MemoryRequirements2>(
				std::move(binding1_buffer),
				std::move(binding1_memory),
				std::move(binding1_mem_requirements)
			);

		}

		static UniqueDedicatedMemoryBuffer make_buffer_bounce_in(vk::UniqueDevice& device, std::span<uint32_t> queueFamilyIndices, std::span<vk::MemoryType> memory_types) {
			vk::BufferCreateInfo bounce_in_bci{
				.size{1024ull * 1024ull},
				.usage{vk::BufferUsageFlagBits::eTransferSrc},
				.sharingMode{vk::SharingMode::eExclusive},
				.queueFamilyIndexCount{static_cast<uint32_t>(queueFamilyIndices.size())},
				.pQueueFamilyIndices{queueFamilyIndices.data()},
			};
			auto bounce_in_mem_requirements{ device->getBufferMemoryRequirements(vk::DeviceBufferMemoryRequirements{.pCreateInfo{&bounce_in_bci}}) };
			std::cout << std::format("bounce_in_mem_requirements:   {}\n", bounce_in_mem_requirements.memoryRequirements.size);
			auto bounce_in_buffer{ device->createBufferUnique(bounce_in_bci).value };
			VkMemoryPropertyFlags bounce_in_needed_flags{ VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT };
			vk::StructureChain<vk::MemoryAllocateInfo, vk::MemoryDedicatedAllocateInfo> bounce_in_ai;
			bounce_in_ai.get<vk::MemoryAllocateInfo>().allocationSize = bounce_in_mem_requirements.memoryRequirements.size;
			bounce_in_ai.get<vk::MemoryAllocateInfo>().memoryTypeIndex = static_cast<uint32_t>(
				std::distance(
					memory_types.begin(),
					std::ranges::find_if(
						memory_types,
						[&](const VkMemoryType& memoryType)-> bool { return (memoryType.propertyFlags & bounce_in_needed_flags) == bounce_in_needed_flags; }
					)
				)
			);
			bounce_in_ai.get<vk::MemoryDedicatedAllocateInfo>().buffer = bounce_in_buffer.get();
			auto bounce_in_memory{ device->allocateMemoryUnique(bounce_in_ai.get<vk::MemoryAllocateInfo>()).value };
			device->bindBufferMemory(bounce_in_buffer.get(), bounce_in_memory.get(), 0ull);
			return std::make_tuple<vk::UniqueBuffer, vk::UniqueDeviceMemory, vk::MemoryRequirements2>(
				std::move(bounce_in_buffer),
				std::move(bounce_in_memory),
				std::move(bounce_in_mem_requirements)
			);

		}

		static UniqueDedicatedMemoryBuffer make_buffer_bounce_out(vk::UniqueDevice& device, std::span<uint32_t> queueFamilyIndices, std::span<vk::MemoryType> memory_types) {
			vk::BufferCreateInfo bounce_out_bci{
				.size{1024ull * 1024ull},
				.usage{vk::BufferUsageFlagBits::eTransferDst},
				.sharingMode{vk::SharingMode::eExclusive},
				.queueFamilyIndexCount{static_cast<uint32_t>(queueFamilyIndices.size())},
				.pQueueFamilyIndices{queueFamilyIndices.data()},
			};
			auto bounce_out_mem_requirements{ device->getBufferMemoryRequirements(vk::DeviceBufferMemoryRequirements{.pCreateInfo{&bounce_out_bci}}) };
			std::cout << std::format("bounce_out_mem_requirements:   {}\n", bounce_out_mem_requirements.memoryRequirements.size);
			auto bounce_out_buffer{ device->createBufferUnique(bounce_out_bci).value };
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
			auto bounce_out_memory{ device->allocateMemoryUnique(bounce_out_ai.get<vk::MemoryAllocateInfo>()).value };
			device->bindBufferMemory(bounce_out_buffer.get(), bounce_out_memory.get(), 0ull);
			return std::make_tuple<vk::UniqueBuffer, vk::UniqueDeviceMemory, vk::MemoryRequirements2>(
				std::move(bounce_out_buffer),
				std::move(bounce_out_memory),
				std::move(bounce_out_mem_requirements)
			);

		}

		static vk::UniqueShaderModule make_shader_module(vk::UniqueDevice& device, std::string spv_path) noexcept {
			MemoryMappedFile mmf_spirv{ spv_path };
			auto s_spirv{ mmf_spirv.get_span<uint32_t>() };
			vk::ShaderModuleCreateInfo shaderModuleCreateInfo{
				.codeSize{s_spirv.size_bytes()}, //byte size
				.pCode{s_spirv.data()},
			};
			return device->createShaderModuleUnique(shaderModuleCreateInfo).value;
		}

		static void setup_bounce_in(vk::UniqueDevice& device, UniqueDedicatedMemoryBuffer &bounce_in) noexcept {
			auto bounce_in_mapped_ptr{ device->mapMemory(std::get<1>(bounce_in).get(), 0ull, VK_WHOLE_SIZE).value };
			std::span<uint8_t> bounce_in_mapped(reinterpret_cast<uint8_t*>(bounce_in_mapped_ptr), std::get<2>(bounce_in).memoryRequirements.size);
			std::cout << std::format("bounce_in_mapped<T> count: {}, bytes: {}\n", bounce_in_mapped.size(), bounce_in_mapped.size_bytes());
			std::ranges::fill(bounce_in_mapped, 0x55);
			device->flushMappedMemoryRanges({
				{
					vk::MappedMemoryRange{
						.memory{std::get<1>(bounce_in).get()},
						.offset{static_cast<uint64_t>(std::distance(reinterpret_cast<uint8_t*>(bounce_in_mapped_ptr), bounce_in_mapped.data()))},
						.size{bounce_in_mapped.size_bytes()},
					}
				}
				});
			device->unmapMemory(std::get<1>(bounce_in).get());

		}

		template<size_t binding_count>
		static vk::UniqueDescriptorSetLayout make_descriptor_set_layout(vk::UniqueDevice& device) noexcept {

			std::array<vk::DescriptorSetLayoutBinding, binding_count> descriptorSetLayoutBindings;
			each<uint32_t>(descriptorSetLayoutBindings, [](uint32_t descriptorSetLayoutBindingIndex, vk::DescriptorSetLayoutBinding &descriptorSetLayoutBinding) {
				descriptorSetLayoutBinding = {
					.binding{descriptorSetLayoutBindingIndex},
					.descriptorType{vk::DescriptorType::eStorageBuffer},
					.descriptorCount{1},
					.stageFlags{vk::ShaderStageFlagBits::eCompute},
				};
			});

			const vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{
				.bindingCount{descriptorSetLayoutBindings.size()},
				.pBindings{descriptorSetLayoutBindings.data()},
			};

			return device->createDescriptorSetLayoutUnique(descriptorSetLayoutCreateInfo).value;
		}

		static inline constexpr size_t binding_count{ 2ull };

		SingleDevice(vk::PhysicalDevice physical_device) noexcept :
			physical_device{ physical_device },
			memory_properties{ physical_device.getMemoryProperties2() },
			memory_types{ std::span(memory_properties.memoryProperties.memoryTypes).first(memory_properties.memoryProperties.memoryTypeCount) },
			queue_families{ physical_device.getQueueFamilyProperties2() },
			queueFamilyIndices{ make_v_queue_family(queue_families) },
			device{ create_device(physical_device, queue_families) },
			binding0{ make_buffer_binding0(device, queueFamilyIndices, memory_types) },
			binding1{ make_buffer_binding1(device, queueFamilyIndices, memory_types) },
			bounce_in{ make_buffer_bounce_in(device, queueFamilyIndices, memory_types) },
			bounce_out{ make_buffer_bounce_out(device, queueFamilyIndices, memory_types) },
			simple_comp{make_shader_module(device, "simple.comp.glsl.spv")},
			descriptorSetLayout{make_descriptor_set_layout<binding_count>(device) }
		{
			show_features();

			std::cout << std::format("heaps: {}\n", memory_properties.memoryProperties.memoryHeapCount);
			std::cout << std::format("types: {}\n", memory_properties.memoryProperties.memoryTypeCount);
			std::cout << std::format("queue_families: {}\n", queue_families.size());
			std::cout << std::format("device: 0x{:x}\n", std::bit_cast<uintptr_t>(device.get()));

			setup_bounce_in(device, bounce_in);


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

			const vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{
				.setLayoutCount{descriptorSetLayouts.size()},
				.pSetLayouts{descriptorSetLayouts.data()},
				.pushConstantRangeCount{pushConstantRange.size()},
				.pPushConstantRanges{pushConstantRange.data()},
			};
			auto pipelineLayout{ device->createPipelineLayoutUnique(pipelineLayoutCreateInfo).value };


			vk::PipelineShaderStageCreateInfo pipelineShaderStageCreateInfo{
				.stage{vk::ShaderStageFlagBits::eCompute},
				.module{simple_comp.get()},
				.pName{"main"},
			};

			std::array<vk::ComputePipelineCreateInfo, 1> computePipelineCreateInfos{ {{
				.stage{pipelineShaderStageCreateInfo},
				.layout{pipelineLayout.get()},
			}} };

			auto pipeline_cache{ device->createPipelineCacheUnique(vk::PipelineCacheCreateInfo{}).value };
			auto compute_pipelines{ device->createComputePipelinesUnique(pipeline_cache.get(), computePipelineCreateInfos).value };


			std::array<vk::DescriptorPoolSize, 1> descriptorPoolSize{ {{
				.type{vk::DescriptorType::eStorageBuffer},
				.descriptorCount{binding_count},
			}} };

			vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo{
				.maxSets{descriptorPoolSize.size()},
				.poolSizeCount{descriptorPoolSize.size()},
				.pPoolSizes{descriptorPoolSize.data()},
			};

			auto descriptor_pool{ device->createDescriptorPoolUnique(descriptorPoolCreateInfo).value };

			vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo{
				.descriptorPool{descriptor_pool.get()},
				.descriptorSetCount{descriptorSetLayouts.size()},
				.pSetLayouts{descriptorSetLayouts.data()},

			};
			auto descriptor_sets{ device->allocateDescriptorSets(descriptorSetAllocateInfo).value };


			std::array<vk::DescriptorBufferInfo, 1> binding0_bufferInfos{ {{.buffer{std::get<0>(binding0).get()}, .offset{}, .range{VK_WHOLE_SIZE}}} };
			std::array<vk::DescriptorBufferInfo, 1> binding1_bufferInfos{ {{.buffer{std::get<0>(binding1).get()}, .offset{}, .range{VK_WHOLE_SIZE}}} };

			device->updateDescriptorSets(std::array<vk::WriteDescriptorSet, binding_count>{
				{
					{
						.dstSet{ descriptor_sets.at(0) },
						.dstBinding{ 0 },
						.dstArrayElement{ 0 },
						.descriptorCount{ binding0_bufferInfos.size() },
						.descriptorType{ vk::DescriptorType::eStorageBuffer },
						.pBufferInfo{ binding0_bufferInfos.data() },
					},
				{
					.dstSet{descriptor_sets.at(0)},
					.dstBinding{1},
					.dstArrayElement{0},
					.descriptorCount{1},
					.descriptorType{vk::DescriptorType::eStorageBuffer},
					.pBufferInfo{binding1_bufferInfos.data()},
				}
				}
			}, {});


			const vk::QueryPoolCreateInfo queryPoolCreateInfo{
				.queryType{vk::QueryType::eTimestamp},
				.queryCount{2},
			};
			auto query_pool{ device->createQueryPoolUnique(queryPoolCreateInfo).value };

			const vk::CommandPoolCreateInfo commandPoolCreateInfo{
				.queueFamilyIndex{static_cast<uint32_t>(std::distance(queue_families.begin(), std::ranges::find(queue_families, VK_QUEUE_COMPUTE_BIT, [](VkQueueFamilyProperties2& props)-> VkQueueFlagBits {
					return std::bit_cast<VkQueueFlagBits>(props.queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT);
				})))},
			};
			auto command_pool{ device->createCommandPoolUnique(commandPoolCreateInfo).value };

			const vk::CommandBufferAllocateInfo commandBufferAllocateInfo{
				.commandPool{command_pool.get()},
				.level{vk::CommandBufferLevel::ePrimary},
				.commandBufferCount{1},
			};
			auto command_buffers{ device->allocateCommandBuffersUnique(commandBufferAllocateInfo).value };

			vk::CommandBufferBeginInfo commandBufferBeginInfo = {
				.flags{vk::CommandBufferUsageFlagBits::eOneTimeSubmit},
			};
			command_buffers.at(0)->begin(commandBufferBeginInfo);

			{
				std::array<vk::BufferCopy2, 1> a_regions{ {{
					.srcOffset{0},
					.dstOffset{0},
					.size{std::get<2>(binding0).memoryRequirements.size},
				}} };
				const vk::CopyBufferInfo2 pCopyBufferInfo{
					.srcBuffer{std::get<0>(bounce_in).get()},
					.dstBuffer{std::get<0>(binding0).get()},
					.regionCount{a_regions.size()},
					.pRegions{a_regions.data()},
				};
				command_buffers.at(0)->copyBuffer2(pCopyBufferInfo);

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
				command_buffers.at(0)->pipelineBarrier2({
					.dependencyFlags{},
					.bufferMemoryBarrierCount{a_bufferMemoryBarrier.size()},
					.pBufferMemoryBarriers{a_bufferMemoryBarrier.data()},
					});

			}


			command_buffers.at(0)->bindPipeline(vk::PipelineBindPoint::eCompute, compute_pipelines.at(0).get());

			// std::vector<vk::DescriptorSet> v_descriptor_set;
			// v_descriptor_set.reserve(descriptor_sets.size());
			// std::ranges::transform(descriptor_sets, std::back_inserter(v_descriptor_set), [](vk::DescriptorSet& descriptorSet)-> VkDescriptorSet { return descriptorSet.descriptorSet; });

			command_buffers.at(0)->bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout.get(), 0, descriptor_sets, {});
			std::array<uint32_t, 1> mask{ 0xff0000ffu };
			command_buffers.at(0)->pushConstants(pipelineLayout.get(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(mask), mask.data());
			command_buffers.at(0)->resetQueryPool(query_pool.get(), 0, 2);
			command_buffers.at(0)->writeTimestamp(vk::PipelineStageFlagBits::eComputeShader, query_pool.get(), 0);
			command_buffers.at(0)->dispatch(1, 1, 1);
			command_buffers.at(0)->writeTimestamp(vk::PipelineStageFlagBits::eComputeShader, query_pool.get(), 1);

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
				command_buffers.at(0)->pipelineBarrier2({
					.dependencyFlags{},
					.bufferMemoryBarrierCount{a_bufferMemoryBarrier.size()},
					.pBufferMemoryBarriers{a_bufferMemoryBarrier.data()},
					});

				std::array<vk::BufferCopy2, 1> a_regions{ {{
					.srcOffset{0},
					.dstOffset{0},
					.size{std::get<2>(binding0).memoryRequirements.size},
				}} };
				const vk::CopyBufferInfo2 pCopyBufferInfo{
					.srcBuffer{std::get<0>(binding1).get()},
					.dstBuffer{std::get<0>(bounce_out).get()},
					.regionCount{a_regions.size()},
					.pRegions{a_regions.data()},
				};
				command_buffers.at(0)->copyBuffer2(pCopyBufferInfo);
			}

			command_buffers.at(0)->end();

			vk::DeviceQueueInfo2 queueInfo{
				.queueFamilyIndex{commandPoolCreateInfo.queueFamilyIndex},
				.queueIndex{0}
			};
			auto queue{ device->getQueue2(queueInfo) };

			std::vector<vk::CommandBuffer> v_command_buffers;
			v_command_buffers.reserve(command_buffers.size());
			std::ranges::transform(command_buffers, std::back_inserter(v_command_buffers), [](vk::UniqueCommandBuffer& commandBuffer)-> vk::CommandBuffer { return commandBuffer.get(); });

			std::array<vk::SubmitInfo, 1> submits{ {{
				.commandBufferCount{static_cast<uint32_t>(v_command_buffers.size())},
				.pCommandBuffers{v_command_buffers.data()},
			}} };
			queue.submit(submits);
			queue.waitIdle();

			{
				auto bounce_out_mapped_ptr{ device->mapMemory(std::get<1>(bounce_out).get(), 0ull, VK_WHOLE_SIZE).value };
				std::span<uint32_t> bounce_out_mapped(reinterpret_cast<uint32_t*>(bounce_out_mapped_ptr), std::get<2>(bounce_out).memoryRequirements.size / sizeof(uint32_t));
				std::cout << std::format("bounce_out_mapped<T> count: {}, bytes: {}\n", bounce_out_mapped.size(), bounce_out_mapped.size_bytes());
				for (auto b : bounce_out_mapped.first(256)) {
					std::cout << std::format("{:08x} ", b);
				}
				std::cout << "\n";

				device->unmapMemory(std::get<1>(bounce_out).get());
			}
			auto query_results{ device->getQueryPoolResults<uint64_t>(query_pool.get(), 0, 2, sizeof(std::array<uint64_t, 2>), sizeof(uint64_t), vk::QueryResultFlagBits::e64 | vk::QueryResultFlagBits::eWait).value };

			auto physical_device_properties{ physical_device.getProperties() };
			std::cout << std::format("deviceName {}\n", static_cast<std::string_view>(physical_device_properties.deviceName));
			std::chrono::nanoseconds run_time{ static_cast<int64_t>(static_cast<double>(query_results[1] - query_results[0]) * static_cast<double>(physical_device_properties.limits.timestampPeriod)) };
			std::cout << std::format("query_diff: {}\n", run_time);
		}
		void show_features() const noexcept {
			auto features2{ physical_device.getFeatures2<
				vk::PhysicalDeviceFeatures2,
				vk::PhysicalDeviceVulkan11Features,
				vk::PhysicalDeviceVulkan12Features,
				vk::PhysicalDeviceVulkan13Features>() };

			std::cout << std::format("synchronization2: {}\n", features2.get<vk::PhysicalDeviceVulkan13Features>().synchronization2);
		}
		inline static void use_physical_device(vk::PhysicalDevice physical_device) noexcept {
			SingleDevice sd{ physical_device };
		}
	};
	void init() noexcept {
		auto instance_version{ vk::enumerateInstanceVersion() };
		std::cout << std::format("instance_version: 0x{:08x}\n", instance_version.value);

		auto instance_layer_properties{ vk::enumerateInstanceLayerProperties().value };
		std::cout << std::format("Instance::count_layers().value(): {}\n", instance_layer_properties.size());

		auto instance_extensions{ vk::enumerateInstanceExtensionProperties().value };
		std::cout << std::format("instance_extensions: {}\n", instance_extensions.size() );

		vk::ApplicationInfo applicationInfo{
			.pApplicationName{__FILE__},
			.apiVersion{VK_HEADER_VERSION_COMPLETE},
		};
		auto instance{ vk::createInstanceUnique({
			.pApplicationInfo{&applicationInfo},
		}).value };

		std::cout << std::format("instance: 0x{:x}\n", std::bit_cast<uintptr_t>(instance.get()));

		auto physical_devices{ instance->enumeratePhysicalDevices().value };

		std::cout << std::format("physical_devices.size(): {}\n", physical_devices.size());
		std::ranges::for_each(physical_devices, SingleDevice::use_physical_device);
	}
};

