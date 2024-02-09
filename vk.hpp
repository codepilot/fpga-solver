#pragma once

#include <vulkan/vulkan.h>
#include <expected>

class vk {
public:

	class Queue {
	public:
		VkQueue queue;
		std::expected<void, VkResult> waitIdle() {
			auto result{ vkQueueWaitIdle(queue) };
			if (result != VkResult::VK_SUCCESS) {
				return std::unexpected<VkResult>(result);
			}
			return std::expected<void, VkResult>();
		}

		std::expected<void, VkResult> submit(std::span<VkSubmitInfo> submits) {
			auto result{ vkQueueSubmit(queue, submits.size(), submits.data(), nullptr) };
			if (result != VkResult::VK_SUCCESS) {
				return std::unexpected<VkResult>(result);
			}
			return std::expected<void, VkResult>();
		}
	};

	class CommandBuffer {
	public:
		VkDevice device;
		VkCommandPool commandPool;
		VkCommandBuffer commandBuffer;

		CommandBuffer() noexcept : device{ nullptr } { }
		CommandBuffer(VkDevice _device, VkCommandBuffer _commandBuffer) noexcept : device{ _device }, commandPool{ nullptr }, commandBuffer{ _commandBuffer } { }
		CommandBuffer(VkDevice _device, VkCommandPool _commandPool, VkCommandBuffer _commandBuffer) noexcept : device{ _device }, commandPool{ _commandPool }, commandBuffer{ _commandBuffer } { }
		CommandBuffer(CommandBuffer&& other) noexcept : device{ std::exchange(other.device, nullptr) }, commandPool{ std::exchange(other.commandPool, nullptr) }, commandBuffer{ std::exchange(other.commandBuffer, nullptr) } { }
		CommandBuffer(CommandBuffer& other) = delete;
		CommandBuffer(const CommandBuffer& other) = delete;
		CommandBuffer& operator=(CommandBuffer& other) = delete;
		~CommandBuffer() noexcept {
			if (device && commandPool && commandBuffer) vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
		}

		std::expected<void, VkResult> begin(const VkCommandBufferBeginInfo* pBeginInfo) noexcept {
			auto result{ vkBeginCommandBuffer(commandBuffer, pBeginInfo) };
			if (result != VkResult::VK_SUCCESS) {
				return std::unexpected<VkResult>(result);
			}
			return std::expected<void, VkResult>();
		}

		std::expected<void, VkResult> end() noexcept {
			auto result{ vkEndCommandBuffer(commandBuffer) };
			if (result != VkResult::VK_SUCCESS) {
				return std::unexpected<VkResult>(result);
			}
			return std::expected<void, VkResult>();
		}

		void bindComputePipeline(VkPipeline pipeline) {
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
		}
		void bindComputeDescriptorSets(VkPipelineLayout layout, std::span<VkDescriptorSet> descriptorSets) {
			vkCmdBindDescriptorSets(commandBuffer,
				VK_PIPELINE_BIND_POINT_COMPUTE,
				layout,
				0,
				descriptorSets.size(),
				descriptorSets.data(),
				0,
				nullptr
			);
		}
		template<typename T>
		void pushComputeConstants(VkPipelineLayout layout, std::span<T> values) noexcept {
			vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, values.size_bytes(), values.data());
		}

		void cmdResetQueryPool(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount) noexcept {
			vkCmdResetQueryPool(commandBuffer, queryPool, firstQuery, queryCount);
		}

		void writeComputeTimestamp(VkQueryPool queryPool, uint32_t query) noexcept {
			vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, queryPool, query);
		}
		void dispatch(uint32_t groupCountX = 1u, uint32_t groupCountY = 1u, uint32_t groupCountZ = 1u) noexcept {
			vkCmdDispatch(commandBuffer, groupCountX, groupCountY, groupCountZ);
		}
		void copy(const VkCopyBufferInfo2* pCopyBufferInfo) noexcept {
			vkCmdCopyBuffer2(commandBuffer, pCopyBufferInfo);
		}
		void compute_to_transfer_barrier() noexcept {
			vkCmdPipelineBarrier(
				commandBuffer,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_DEPENDENCY_DEVICE_GROUP_BIT,
				0, nullptr, 0, nullptr, 0, nullptr
			);
		}
	};

	class CommandPool {
	public:
		VkDevice device;
		VkCommandPool commandPool;

		CommandPool() noexcept : device{ nullptr } { }
		CommandPool(VkDevice _device, VkCommandPool _commandPool) noexcept : device{ _device }, commandPool{ _commandPool } { }
		CommandPool(CommandPool&& other) noexcept : device{ std::exchange(other.device, nullptr) }, commandPool{ std::exchange(other.commandPool, nullptr) } { }
		CommandPool(CommandPool& other) = delete;
		CommandPool(const CommandPool& other) = delete;
		CommandPool& operator=(CommandPool& other) = delete;
		~CommandPool() noexcept {
			if (device && commandPool) vkDestroyCommandPool(device, commandPool, nullptr);
		}
	};

	class QueryPool {
	public:
		VkDevice device;
		VkQueryPool queryPool;

		QueryPool() noexcept : device{ nullptr } { }
		QueryPool(VkDevice _device, VkQueryPool _queryPool) noexcept : device{ _device }, queryPool{ _queryPool } { }
		QueryPool(QueryPool&& other) noexcept : device{ std::exchange(other.device, nullptr) }, queryPool{ std::exchange(other.queryPool, nullptr) } { }
		QueryPool(QueryPool& other) = delete;
		QueryPool(const QueryPool& other) = delete;
		QueryPool& operator=(QueryPool& other) = delete;
		~QueryPool() noexcept {
			if (device && queryPool) vkDestroyQueryPool(device, queryPool, nullptr);
		}
		template<class _Ty, size_t _Size>
		std::expected<std::vector<_Ty>, VkResult> get_results(VkQueryResultFlags flags) noexcept {
			std::vector<_Ty> v_data(_Size);
			auto s_data{ std::span<_Ty, _Size>(v_data) };
			auto result{ vkGetQueryPoolResults(
				device,
				queryPool,
				0,
				_Size,
				s_data.size_bytes(),
				s_data.data(),
				sizeof(_Ty),
				flags
			) };
			if (result != VkResult::VK_SUCCESS) {
				return std::unexpected<VkResult>(result);
			}
			return std::expected<std::vector<_Ty>, VkResult>(std::move(v_data));
		}
	};

	class DescriptorSet {
	public:
		VkDevice device;
		VkDescriptorPool descriptorPool;
		VkDescriptorSet descriptorSet;

		DescriptorSet() noexcept : device{ nullptr } { }
		DescriptorSet(VkDevice _device, VkDescriptorSet _descriptorSet) noexcept : device{ _device }, descriptorPool{ nullptr }, descriptorSet{ _descriptorSet } { }
		DescriptorSet(VkDevice _device, VkDescriptorPool _descriptorPool, VkDescriptorSet _descriptorSet) noexcept : device{ _device }, descriptorPool{ _descriptorPool }, descriptorSet{ _descriptorSet } { }
		DescriptorSet(DescriptorSet&& other) noexcept : device{ std::exchange(other.device, nullptr) }, descriptorPool{ std::exchange(other.descriptorPool, nullptr) }, descriptorSet{ std::exchange(other.descriptorSet, nullptr) } { }
		DescriptorSet(DescriptorSet& other) = delete;
		DescriptorSet(const DescriptorSet& other) = delete;
		DescriptorSet& operator=(DescriptorSet& other) = delete;
		~DescriptorSet() noexcept {
			if (device && descriptorPool && descriptorSet) vkFreeDescriptorSets(device, descriptorPool, 1, &descriptorSet);
		}
	};

	class DescriptorPool {
	public:
		VkDevice device;
		VkDescriptorPool descriptorPool;

		DescriptorPool() noexcept : device{ nullptr } { }
		DescriptorPool(VkDevice _device, VkDescriptorPool _descriptorPool) noexcept : device{ _device }, descriptorPool{ _descriptorPool } { }
		DescriptorPool(DescriptorPool&& other) noexcept : device{ std::exchange(other.device, nullptr) }, descriptorPool{ std::exchange(other.descriptorPool, nullptr) } { }
		DescriptorPool(DescriptorPool& other) = delete;
		DescriptorPool(const DescriptorPool& other) = delete;
		DescriptorPool& operator=(DescriptorPool& other) = delete;
		~DescriptorPool() noexcept {
			if (device && descriptorPool) vkDestroyDescriptorPool(device, descriptorPool, nullptr);
		}
	};

	class Pipeline {
	public:
		VkDevice device;
		VkPipeline pipeline;

		Pipeline() noexcept : device{ nullptr } { }
		Pipeline(VkDevice _device, VkPipeline _pipeline) noexcept : device{ _device }, pipeline{ _pipeline } { }
		Pipeline(Pipeline&& other) noexcept : device{ std::exchange(other.device, nullptr) }, pipeline{ std::exchange(other.pipeline, nullptr) } { }
		Pipeline(Pipeline& other) = delete;
		Pipeline(const Pipeline& other) = delete;
		Pipeline& operator=(Pipeline& other) = delete;
		~Pipeline() noexcept {
			if (device && pipeline) vkDestroyPipeline(device, pipeline, nullptr);
		}
	};

	class PipelineLayout {
	public:
		VkDevice device;
		VkPipelineLayout pipelineLayout;

		PipelineLayout() noexcept : device{ nullptr } { }
		PipelineLayout(VkDevice _device, VkPipelineLayout _pipelineLayout) noexcept : device{ _device }, pipelineLayout{ _pipelineLayout } { }
		PipelineLayout(PipelineLayout&& other) noexcept : device{ std::exchange(other.device, nullptr) }, pipelineLayout{ std::exchange(other.pipelineLayout, nullptr) } { }
		PipelineLayout(PipelineLayout& other) = delete;
		PipelineLayout(const PipelineLayout& other) = delete;
		PipelineLayout& operator=(PipelineLayout& other) = delete;
		~PipelineLayout() noexcept {
			if (device && pipelineLayout) vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		}
	};

	class DescriptorSetLayout {
	public:
		VkDevice device;
		VkDescriptorSetLayout descriptorSetLayout;

		DescriptorSetLayout() noexcept : device{ nullptr } { }
		DescriptorSetLayout(VkDevice _device, VkDescriptorSetLayout _descriptorSetLayout) noexcept : device{ _device }, descriptorSetLayout{ _descriptorSetLayout } { }
		DescriptorSetLayout(DescriptorSetLayout&& other) noexcept : device{ std::exchange(other.device, nullptr) }, descriptorSetLayout{ std::exchange(other.descriptorSetLayout, nullptr) } { }
		DescriptorSetLayout(DescriptorSetLayout& other) = delete;
		DescriptorSetLayout(const DescriptorSetLayout& other) = delete;
		DescriptorSetLayout& operator=(DescriptorSetLayout& other) = delete;
		~DescriptorSetLayout() noexcept {
			if (device && descriptorSetLayout) vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
		}
		/*
		* vkGetDescriptorSetLayoutSupport
		*/
	};

	class ShaderModule {
	public:
		VkDevice device;
		VkShaderModule shaderModule;

		ShaderModule() noexcept : device{ nullptr } { }
		ShaderModule(VkDevice _device, VkShaderModule _shaderModule) noexcept : device{ _device }, shaderModule{ _shaderModule } { }
		ShaderModule(ShaderModule&& other) noexcept : device{ std::exchange(other.device, nullptr) }, shaderModule{ std::exchange(other.shaderModule, nullptr) } { }
		ShaderModule(ShaderModule& other) = delete;
		ShaderModule(const ShaderModule& other) = delete;
		ShaderModule& operator=(ShaderModule& other) = delete;
		~ShaderModule() noexcept {
			if (device && shaderModule) vkDestroyShaderModule(device, shaderModule, nullptr);
		}

	};

	class Memory {
	public:
		VkDevice device;
		VkDeviceMemory memory;
		size_t size;

		Memory() noexcept : device{ nullptr } { }
		Memory(VkDevice _device, VkDeviceMemory _memory, size_t _size) noexcept : device{ _device }, memory{ _memory }, size{ _size } { }
		Memory(Memory&& other) noexcept : device{ std::exchange(other.device, nullptr) }, memory{ std::exchange(other.memory, nullptr) }, size{ std::exchange(other.size, 0) } { }
		Memory(Memory& other) = delete;
		Memory(const Memory& other) = delete;
		Memory& operator=(Memory& other) = delete;
		~Memory() noexcept {
			if (device && memory) vkFreeMemory(device, memory, nullptr);
		}

		/*
		* vkGetDeviceMemoryCommitment
		* vkGetDeviceMemoryOpaqueCaptureAddress
		* vkFlushMappedMemoryRanges and vkInvalidateMappedMemoryRanges
		*/
		template<class _Ty, size_t _Extent = std::dynamic_extent>
		std::expected<std::span<_Ty, _Extent>, VkResult> map() {
			void* pData{};
			auto result{ vkMapMemory(device, memory, 0, VK_WHOLE_SIZE, 0, &pData) };
			if (result != VkResult::VK_SUCCESS) {
				return std::unexpected<VkResult>(result);
			}
			return std::expected<std::span<_Ty, _Extent>, VkResult>(std::span<_Ty, _Extent>(reinterpret_cast<_Ty *>(pData), size / sizeof(_Ty)));
		}
		void unmap() {
			vkUnmapMemory(device, memory);
		}

		std::expected<void, VkResult> flush() {
			std::array<VkMappedMemoryRange, 1> memoryRanges{{{
				.sType{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE},
				.memory{memory},
				.offset{0},
				.size{size},
			}}};
			auto result{ vkFlushMappedMemoryRanges(device, memoryRanges.size(), memoryRanges.data()) };
			if (result != VkResult::VK_SUCCESS) {
				return std::unexpected<VkResult>(result);
			}
			return std::expected<void, VkResult>();
		}

		std::expected<void, VkResult> invalidate() {
			std::array<VkMappedMemoryRange, 1> memoryRanges{ {{
				.sType{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE},
				.memory{memory},
				.offset{0},
				.size{size},
			}} };
			auto result{ vkInvalidateMappedMemoryRanges(device, memoryRanges.size(), memoryRanges.data()) };
			if (result != VkResult::VK_SUCCESS) {
				return std::unexpected<VkResult>(result);
			}
			return std::expected<void, VkResult>();
		}

	};

	class Buffer {
	public:
		VkDevice device;
		VkBuffer buffer;

		Buffer() noexcept : device{ nullptr } { }
		Buffer(VkDevice _device, VkBuffer _buffer) noexcept : device{ _device }, buffer{ _buffer } { }
		Buffer(Buffer&& other) noexcept : device{ std::exchange(other.device, nullptr) }, buffer{ std::exchange(other.buffer, nullptr) } { }
		Buffer(Buffer& other) = delete;
		Buffer(const Buffer& other) = delete;
		Buffer& operator=(Buffer& other) = delete;
		~Buffer() noexcept {
			if (device && buffer) vkDestroyBuffer(device, buffer, nullptr);
		}
		VkMemoryRequirements2 get_memory_requirements(const VkBufferCreateInfo* pCreateInfo) const noexcept {
			VkMemoryRequirements2 pMemoryRequirements{
				.sType{VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2},
			};
			VkBufferMemoryRequirementsInfo2 pInfo{
				.sType{VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2},
				.buffer{buffer},
			};
			vkGetBufferMemoryRequirements2(device, &pInfo, &pMemoryRequirements);
			return pMemoryRequirements;
		}
		std::expected<Memory, VkResult> dedicated_allocate_memory(VkMemoryAllocateInfo& pAllocateInfo) noexcept {
			VkDeviceMemory memory{};
			VkMemoryDedicatedAllocateInfo dedicatedInfo{
				.sType{VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO},
				.buffer{buffer},
			};
			pAllocateInfo.pNext = &dedicatedInfo;
			auto result{ vkAllocateMemory(device, &pAllocateInfo, nullptr, &memory) };
			if (result != VkResult::VK_SUCCESS) {
				return std::unexpected<VkResult>(result);
			}
			bind(memory).value();
			return std::expected<Memory, VkResult>(Memory(device, memory, pAllocateInfo.allocationSize));
		}
		std::expected<void, VkResult> bind(VkDeviceMemory memory) noexcept {
			std::array<VkBindBufferMemoryInfo, 1> bindInfos{{{
					.sType{VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO},
					.buffer{buffer},
					.memory{memory},
					.memoryOffset{0},
				}}};
			auto result{ vkBindBufferMemory2(device, bindInfos.size(), bindInfos.data())};
			if (result != VkResult::VK_SUCCESS) {
				return std::unexpected<VkResult>(result);
			}
			return std::expected<void, VkResult>();
		}
	};

	struct BufferCreateInfo {
		std::vector<uint32_t> queueFamilyIndices;
		VkBufferCreateInfo bufferCreateInfo;

		inline static std::vector<uint32_t> make_queueFamilyIndices(std::span<VkDeviceQueueCreateInfo> s_queue_create_info) noexcept {
			std::vector<uint32_t> queueFamilyIndices;
			queueFamilyIndices.reserve(s_queue_create_info.size());
			for (auto&& info : s_queue_create_info) {
				queueFamilyIndices.emplace_back(info.queueFamilyIndex);
			}
			return queueFamilyIndices;
		}

		BufferCreateInfo(
			std::span<VkDeviceQueueCreateInfo> s_queue_create_info,
			VkBufferCreateFlags flags,
			VkDeviceSize size,
			VkBufferUsageFlags usage,
			VkSharingMode sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE
		) noexcept :
			queueFamilyIndices{ make_queueFamilyIndices(s_queue_create_info) },
			bufferCreateInfo{
				.sType{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO},
				.flags{flags},
				.size{size},
				.usage{usage},
				.sharingMode{sharingMode},
				.queueFamilyIndexCount{static_cast<uint32_t>(queueFamilyIndices.size())},
				.pQueueFamilyIndices{queueFamilyIndices.data()}
			}
		{

		}
		operator const VkBufferCreateInfo* () const noexcept {
			return &bufferCreateInfo;
		}
	};

	class Device {
	public:
		VkDevice device;

		Device() noexcept : device{ nullptr } { }
		Device(VkDevice _device) noexcept : device{ _device } { }
		Device(Device&& other) noexcept : device{ std::exchange(other.device, nullptr) } { }
		Device(Device& other) = delete;
		Device(const Device& other) = delete;
		Device& operator=(Device& other) = delete;

		~Device() noexcept {
			if (device) vkDestroyDevice(device, nullptr);
		}

		VkMemoryRequirements2 get_buffer_memory_requirements(const VkBufferCreateInfo* pCreateInfo) const noexcept {
			VkMemoryRequirements2 pMemoryRequirements{
				.sType{VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2},
			};
			VkDeviceBufferMemoryRequirements pInfo{
				.sType{VK_STRUCTURE_TYPE_DEVICE_BUFFER_MEMORY_REQUIREMENTS},
				.pCreateInfo{pCreateInfo}
			};
			vkGetDeviceBufferMemoryRequirements(device, &pInfo, &pMemoryRequirements);
			return pMemoryRequirements;
		}

		std::expected<Buffer, VkResult> create_buffer(const VkBufferCreateInfo* pCreateInfo) noexcept {
			VkBuffer buffer{};
			auto result{ vkCreateBuffer(device, pCreateInfo, nullptr, &buffer) };
			if (result != VkResult::VK_SUCCESS) {
				return std::unexpected<VkResult>(result);
			}
			return std::expected<Buffer, VkResult>(Buffer(device, buffer));
		}

		std::expected<Memory, VkResult> allocate_memory(const VkMemoryAllocateInfo* pAllocateInfo) noexcept {
			VkDeviceMemory pMemory{};
			auto result{ vkAllocateMemory(device, pAllocateInfo, nullptr, &pMemory) };
			if (result != VkResult::VK_SUCCESS) {
				return std::unexpected<VkResult>(result);
			}
			return std::expected<Memory, VkResult>(Memory(device, pMemory, pAllocateInfo->allocationSize));
		}

		std::expected<ShaderModule, VkResult> create_shader_module(const VkShaderModuleCreateInfo* pCreateInfo) noexcept {
			VkShaderModule shaderModule{};
			auto result{ vkCreateShaderModule(device, pCreateInfo, nullptr, &shaderModule) };
			if (result != VkResult::VK_SUCCESS) {
				return std::unexpected<VkResult>(result);
			}
			return std::expected<ShaderModule, VkResult>(ShaderModule(device, shaderModule));
		}

		std::expected<DescriptorSetLayout, VkResult> create_descriptor_set_layout(const VkDescriptorSetLayoutCreateInfo* pCreateInfo) noexcept {
			VkDescriptorSetLayout descriptorSetLayout{};
			auto result{ vkCreateDescriptorSetLayout(device, pCreateInfo, nullptr, &descriptorSetLayout) };
			if (result != VkResult::VK_SUCCESS) {
				return std::unexpected<VkResult>(result);
			}
			return std::expected<DescriptorSetLayout, VkResult>(DescriptorSetLayout(device, descriptorSetLayout));
		}

		std::expected<PipelineLayout, VkResult> create_pipeline_layout(const VkPipelineLayoutCreateInfo* pCreateInfo) noexcept {
			VkPipelineLayout pipelineLayout{};
			auto result{ vkCreatePipelineLayout(device, pCreateInfo, nullptr, &pipelineLayout) };
			if (result != VkResult::VK_SUCCESS) {
				return std::unexpected<VkResult>(result);
			}
			return std::expected<PipelineLayout, VkResult>(PipelineLayout(device, pipelineLayout));
		}

		std::expected<std::vector<Pipeline>, VkResult> create_compute_pipelines(std::span<VkComputePipelineCreateInfo> pCreateInfos) noexcept {
			std::vector<VkPipeline> vk_pipelines(pCreateInfos.size());

			auto result{ vkCreateComputePipelines(device, nullptr, pCreateInfos.size(), pCreateInfos.data(), nullptr, vk_pipelines.data())};
			if (result != VkResult::VK_SUCCESS) {
				return std::unexpected<VkResult>(result);
			}
			std::vector<Pipeline> pipelines;
			pipelines.reserve(pCreateInfos.size());
			std::ranges::transform(vk_pipelines, std::back_inserter(pipelines), [&](VkPipeline pipeline)-> Pipeline { return Pipeline(device, pipeline); });
			return std::expected<std::vector<Pipeline>, VkResult>(std::move(pipelines));
		}

		std::expected<DescriptorPool, VkResult> create_descriptor_pool(const VkDescriptorPoolCreateInfo* pCreateInfo) noexcept {
			VkDescriptorPool descriptorPool{};
			auto result{ vkCreateDescriptorPool(device, pCreateInfo, nullptr, &descriptorPool) };
			if (result != VkResult::VK_SUCCESS) {
				return std::unexpected<VkResult>(result);
			}
			return std::expected<DescriptorPool, VkResult>(DescriptorPool(device, descriptorPool));
		}

		std::expected<std::vector<DescriptorSet>, VkResult> create_descriptor_sets(const VkDescriptorSetAllocateInfo* pAllocateInfo) noexcept {
			std::vector<VkDescriptorSet> vk_descriptorSets(pAllocateInfo->descriptorSetCount);

			auto result{ vkAllocateDescriptorSets(device, pAllocateInfo, vk_descriptorSets.data()) };
			if (result != VkResult::VK_SUCCESS) {
				return std::unexpected<VkResult>(result);
			}
			std::vector<DescriptorSet> descriptorSets;
			descriptorSets.reserve(vk_descriptorSets.size());
			//std::ranges::transform(vk_descriptorSets, std::back_inserter(descriptorSets), [&](VkDescriptorSet descriptorSet)-> DescriptorSet { return DescriptorSet(device, pAllocateInfo->descriptorPool, descriptorSet); });
			std::ranges::transform(vk_descriptorSets, std::back_inserter(descriptorSets), [&](VkDescriptorSet descriptorSet)-> DescriptorSet { return DescriptorSet(device, descriptorSet); });
			return std::expected<std::vector<DescriptorSet>, VkResult>(std::move(descriptorSets));
		}

		void updateDescriptorSets(std::span<VkWriteDescriptorSet> descriptorWrites, std::span<VkCopyDescriptorSet> descriptorCopies) noexcept {
			vkUpdateDescriptorSets(device, descriptorWrites.size(), descriptorWrites.data(), descriptorCopies.size(), descriptorCopies.data());
		}

		void updateDescriptorSets(std::span<VkWriteDescriptorSet> descriptorWrites) noexcept {
			vkUpdateDescriptorSets(device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
		}

		void updateDescriptorSets(std::span<VkCopyDescriptorSet> descriptorCopies) noexcept {
			vkUpdateDescriptorSets(device, 0, nullptr, descriptorCopies.size(), descriptorCopies.data());
		}

		std::expected<QueryPool, VkResult> create_query_pool(const VkQueryPoolCreateInfo* pCreateInfo) noexcept {
			VkQueryPool queryPool{};
			auto result{ vkCreateQueryPool(device, pCreateInfo, nullptr, &queryPool) };
			if (result != VkResult::VK_SUCCESS) {
				return std::unexpected<VkResult>(result);
			}
			return std::expected<QueryPool, VkResult>(QueryPool(device, queryPool));
		}

		std::expected<CommandPool, VkResult> create_command_pool(const VkCommandPoolCreateInfo* pCreateInfo) noexcept {
			VkCommandPool commandPool{};
			auto result{ vkCreateCommandPool(device, pCreateInfo, nullptr, &commandPool) };
			if (result != VkResult::VK_SUCCESS) {
				return std::unexpected<VkResult>(result);
			}
			return std::expected<CommandPool, VkResult>(CommandPool(device, commandPool));
		}

		std::expected<std::vector<CommandBuffer>, VkResult> create_command_buffers(const VkCommandBufferAllocateInfo* pAllocateInfo) noexcept {
			std::vector<VkCommandBuffer> vk_commandBuffers(pAllocateInfo->commandBufferCount);

			auto result{ vkAllocateCommandBuffers(device, pAllocateInfo, vk_commandBuffers.data()) };
			if (result != VkResult::VK_SUCCESS) {
				return std::unexpected<VkResult>(result);
			}
			std::vector<CommandBuffer> commandBuffers;
			commandBuffers.reserve(vk_commandBuffers.size());
			std::ranges::transform(vk_commandBuffers, std::back_inserter(commandBuffers), [&](VkCommandBuffer commandBuffer)-> CommandBuffer { return CommandBuffer(device, pAllocateInfo->commandPool, commandBuffer); });
			return std::expected<std::vector<CommandBuffer>, VkResult>(std::move(commandBuffers));
		}

		Queue get_queue(const VkDeviceQueueInfo2* pQueueInfo) const noexcept {
			VkQueue queue{};
			vkGetDeviceQueue2(device, pQueueInfo, &queue);
			return { .queue{queue} };
		}


	};

	class PhysicalDevice {
	public:
		VkPhysicalDevice physicalDevice;

		VkPhysicalDeviceProperties get_properties() const noexcept {
			VkPhysicalDeviceProperties pProperties{};
			vkGetPhysicalDeviceProperties(physicalDevice, &pProperties);
			return pProperties;
		}

		class PhysicalDeviceFeatures {
		public:
			VkPhysicalDeviceVulkan13Features vulkan13Features;
			VkPhysicalDeviceFeatures2 features2;
			PhysicalDeviceFeatures() :
				vulkan13Features{ .sType{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES}, .synchronization2{true} },
				features2{ .sType{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2}, .pNext{&vulkan13Features} }
			{

			}
			operator VkPhysicalDeviceFeatures2* () noexcept {
				return &features2;
			}
		};

		PhysicalDeviceFeatures get_features() const noexcept {
			PhysicalDeviceFeatures features;
			vkGetPhysicalDeviceFeatures2(physicalDevice, features);
			return features;
		}

		uint32_t count_queue_families() const noexcept {
			uint32_t pQueueFamilyPropertyCount{};
			vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &pQueueFamilyPropertyCount, nullptr);
			return pQueueFamilyPropertyCount;
		}

		std::vector<VkQueueFamilyProperties2> get_queue_families() const noexcept {
			uint32_t pQueueFamilyPropertyCount{ count_queue_families() };
			std::vector<VkQueueFamilyProperties2> pQueueFamilyProperties(static_cast<size_t>(pQueueFamilyPropertyCount), VkQueueFamilyProperties2{ .sType{VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2} });
			vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &pQueueFamilyPropertyCount, pQueueFamilyProperties.data());
			pQueueFamilyProperties.resize(pQueueFamilyPropertyCount);
			return pQueueFamilyProperties;
		}

		struct MemInfo : VkMemoryHeap {
			std::vector<VkMemoryPropertyFlags> propertyFlags;
		};
		struct DeviceMemInfo {
			std::vector<MemInfo> memoryHeaps;
			std::vector<VkMemoryType> memoryTypes;
		};
		DeviceMemInfo get_memory_properties() const {
			VkPhysicalDeviceMemoryProperties2 pMemoryProperties{
				.sType{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2},
			};
			vkGetPhysicalDeviceMemoryProperties2(physicalDevice, &pMemoryProperties);
			std::span<VkMemoryType> memoryTypes(pMemoryProperties.memoryProperties.memoryTypes, pMemoryProperties.memoryProperties.memoryTypeCount);
			std::span<VkMemoryHeap> memoryHeaps(pMemoryProperties.memoryProperties.memoryHeaps, pMemoryProperties.memoryProperties.memoryHeapCount);
			DeviceMemInfo v_memoryInfo;
			v_memoryInfo.memoryHeaps.reserve(memoryHeaps.size());
			v_memoryInfo.memoryTypes.reserve(memoryTypes.size());
			for (auto&& memoryHeap : memoryHeaps) {
				v_memoryInfo.memoryHeaps.emplace_back(memoryHeap);
			}
			for (auto&& memoryType : memoryTypes) {
				v_memoryInfo.memoryHeaps[memoryType.heapIndex].propertyFlags.emplace_back(memoryType.propertyFlags);
				v_memoryInfo.memoryTypes.emplace_back(memoryType);
			}
			return v_memoryInfo;
		}

		std::expected<Device, VkResult> create_device(const VkDeviceCreateInfo& pCreateInfo) {
			VkDevice device{};
			auto result{ vkCreateDevice(physicalDevice, &pCreateInfo, nullptr, &device) };
			if (result != VkResult::VK_SUCCESS) {
				return std::unexpected<VkResult>(result);
			}
			return std::expected<Device, VkResult>(device);
		}

	};

	class Instance {
	public:
		VkInstance instance;

		Instance() noexcept : instance{ nullptr } { }
		Instance(VkInstance _instance) noexcept : instance{ _instance } { }
		Instance(Instance&& other) noexcept : instance{ std::exchange(other.instance, nullptr) } { }
		Instance(Instance& other) = delete;
		Instance(const Instance& other) = delete;
		Instance& operator=(Instance& other) = delete;

		~Instance() noexcept {
			if (instance) vkDestroyInstance(instance, nullptr);
		}

		static std::expected<uint32_t, VkResult> get_version() noexcept {
			uint32_t pApiVersion{};
			auto result{ vkEnumerateInstanceVersion(&pApiVersion) };
			if (result != VkResult::VK_SUCCESS) {
				return std::unexpected<VkResult>(result);
			}
			return std::expected<uint32_t, VkResult>(pApiVersion);
		}

		static std::expected<Instance, VkResult> create(const VkInstanceCreateInfo* pCreateInfo) noexcept {
			VkInstance instance{};
			auto result{ vkCreateInstance(pCreateInfo, nullptr, &instance) };
			if (result != VkResult::VK_SUCCESS) {
				return std::unexpected<VkResult>(result);
			}
			return std::expected<Instance, VkResult>(instance);
		}

		static std::expected<uint32_t, VkResult> count_layers() noexcept {
			uint32_t pPropertyCount{};
			auto result{ vkEnumerateInstanceLayerProperties(&pPropertyCount, nullptr) };
			if (result != VkResult::VK_SUCCESS) {
				return std::unexpected<VkResult>(result);
			}
			return std::expected<uint32_t, VkResult>(pPropertyCount);
		}

		static std::expected<uint32_t, VkResult> count_extensions(std::string layer_name = {}) noexcept {
			uint32_t pPropertyCount{};
			auto result{ vkEnumerateInstanceExtensionProperties(layer_name.empty() ? nullptr : layer_name.c_str(), &pPropertyCount, nullptr) };
			if (result != VkResult::VK_SUCCESS) {
				return std::unexpected<VkResult>(result);
			}
			return std::expected<uint32_t, VkResult>(pPropertyCount);
		}

		static std::expected<std::map<std::string, uint32_t>, VkResult> get_extensions(std::string layer_name = {}) noexcept {
			uint32_t pPropertyCount{ count_extensions(layer_name).value() };
			std::vector<VkExtensionProperties> properties(static_cast<size_t>(pPropertyCount));

			auto result{ vkEnumerateInstanceExtensionProperties(layer_name.empty() ? nullptr : layer_name.c_str(), &pPropertyCount, properties.data()) };
			if (result != VkResult::VK_SUCCESS) {
				return std::unexpected<VkResult>(result);
			}
			properties.resize(pPropertyCount);
			std::map<std::string, uint32_t> ret;
			for (auto&& property : properties) {
				ret.insert({ property.extensionName, property.specVersion });
			}
			return std::expected<std::map<std::string, uint32_t>, VkResult>(ret);
		}

		std::expected<uint32_t, VkResult> count_physical_devices() const noexcept {
			uint32_t pPhysicalDeviceCount{};
			auto result{ vkEnumeratePhysicalDevices(instance, &pPhysicalDeviceCount, nullptr) };
			if (result != VkResult::VK_SUCCESS) {
				return std::unexpected<VkResult>(result);
			}
			return std::expected<uint32_t, VkResult>(pPhysicalDeviceCount);
		}

		std::expected<std::vector<PhysicalDevice>, VkResult> get_physical_devices() const noexcept {
			uint32_t pPhysicalDeviceCount{ count_physical_devices().value() };
			std::vector<PhysicalDevice> pPhysicalDevices(static_cast<size_t>(pPhysicalDeviceCount));
			static_assert(sizeof(PhysicalDevice) == sizeof(VkPhysicalDevice));

			auto result{ vkEnumeratePhysicalDevices(instance, &pPhysicalDeviceCount, &pPhysicalDevices.data()->physicalDevice) };
			if (result != VkResult::VK_SUCCESS) {
				return std::unexpected<VkResult>(result);
			}
			pPhysicalDevices.resize(pPhysicalDeviceCount);

			return std::expected<std::vector<PhysicalDevice>, VkResult>(pPhysicalDevices);
		}

	};

	static void init() noexcept {
		auto instance_version{ Instance::get_version().value() };
		std::cout << std::format("instance_version: 0x{:08x}\n", instance_version);

		std::cout << std::format("Instance::count_layers().value(): {}\n", Instance::count_layers().value());

		auto instance_extensions{ Instance::get_extensions().value() };
		std::cout << std::format("instance_extensions: {}\n", instance_extensions.size());

		VkApplicationInfo applicationInfo{
			.sType{VK_STRUCTURE_TYPE_APPLICATION_INFO},
			.pApplicationName{__FILE__},
			.apiVersion{VK_HEADER_VERSION_COMPLETE},
		};
		VkInstanceCreateInfo createInfo{
			.sType{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO},
			.pApplicationInfo{&applicationInfo},
		};
		auto instance{ Instance::create(&createInfo).value() };
		std::cout << std::format("instance: 0x{:x}\n", std::bit_cast<uintptr_t>(instance.instance));

		std::cout << std::format("instance.count_physical_devices().value(): {}\n", instance.count_physical_devices().value());

		auto physical_devices{ instance.get_physical_devices().value() };
		std::cout << std::format("physical_devices.size(): {}\n", physical_devices.size());

		for (auto&& physical_device : physical_devices) {
			auto physical_device_properties{ physical_device.get_properties() };
			std::cout << std::format("deviceName {}\n", physical_device_properties.deviceName);

			auto features{ physical_device.get_features() };
			std::cout << std::format("multiViewport: {}\n", features.features2.features.multiViewport);
			std::cout << std::format("synchronization2: {}\n", features.vulkan13Features.synchronization2);

			auto memory_properties{ physical_device.get_memory_properties() };
			std::cout << std::format("heaps: {}\n", memory_properties.memoryHeaps.size());
			std::cout << std::format("types: {}\n", memory_properties.memoryTypes.size());

			std::vector<VkDeviceQueueCreateInfo> v_queue_create_info;
			auto queue_families{ physical_device.get_queue_families() };
			std::cout << std::format("queue_families: {}\n", queue_families.size());
			std::vector<std::vector<float>> all_queue_priorities;
			all_queue_priorities.reserve(queue_families.size());
			each<uint32_t>(queue_families, [&](uint32_t queue_family_idx, VkQueueFamilyProperties2& queue_familiy_properties) {
				if ((queue_familiy_properties.queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT) != VK_QUEUE_COMPUTE_BIT) {
					return;
				}
				decltype(auto) queue_priorities{ all_queue_priorities.emplace_back(std::vector<float>(static_cast<size_t>(queue_familiy_properties.queueFamilyProperties.queueCount), 1.0f)) };
				v_queue_create_info.emplace_back(VkDeviceQueueCreateInfo{
					.sType{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO},
					.queueFamilyIndex{queue_family_idx},
					.queueCount{queue_familiy_properties.queueFamilyProperties.queueCount},
					.pQueuePriorities{queue_priorities.data()},
					});
				});

			VkDeviceCreateInfo pCreateInfo{
				.sType{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO},
				.queueCreateInfoCount{static_cast<uint32_t>(v_queue_create_info.size())},
				.pQueueCreateInfos{v_queue_create_info.data()},
			};
			auto device{ physical_device.create_device(pCreateInfo).value() };
			std::cout << std::format("device: 0x{:x}\n", std::bit_cast<uintptr_t>(device.device));

			auto binding0_bci{ BufferCreateInfo(v_queue_create_info, 0, 1024ull * 1024ull, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_SHARING_MODE_EXCLUSIVE) };
			auto binding1_bci{ BufferCreateInfo(v_queue_create_info, 0, 1024ull * 1024ull, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE) };
			auto binding0_mem_requirements{ device.get_buffer_memory_requirements(binding0_bci) };
			auto binding1_mem_requirements{ device.get_buffer_memory_requirements(binding1_bci) };
			std::cout << std::format("binding0_mem_requirements: {}\n", binding0_mem_requirements.memoryRequirements.size);
			std::cout << std::format("binding1_mem_requirements: {}\n", binding1_mem_requirements.memoryRequirements.size);

			auto binding0_buffer{ device.create_buffer(binding0_bci).value() };
			auto binding1_buffer{ device.create_buffer(binding1_bci).value() };

			VkMemoryPropertyFlags binding0_needed_flags{ VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };
			VkMemoryPropertyFlags binding1_needed_flags{ VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

			VkMemoryAllocateInfo binding0_allocateInfo{
				.sType{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO},
				// const void* pNext;
				.allocationSize{binding0_mem_requirements.memoryRequirements.size},
				.memoryTypeIndex{
					static_cast<uint32_t>(
						std::distance(
							memory_properties.memoryTypes.begin(),
							std::ranges::find_if(
								memory_properties.memoryTypes,
								[&](const VkMemoryType& memoryType)-> bool { return (memoryType.propertyFlags & binding0_needed_flags) == binding0_needed_flags; }
							)
						)
					)
				}
			};
			VkMemoryAllocateInfo binding1_allocateInfo{
				.sType{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO},
				// const void* pNext;
				.allocationSize{binding1_mem_requirements.memoryRequirements.size},
				.memoryTypeIndex{
					static_cast<uint32_t>(
						std::distance(
							memory_properties.memoryTypes.begin(),
							std::ranges::find_if(
								memory_properties.memoryTypes,
								[&](const VkMemoryType& memoryType)-> bool { return (memoryType.propertyFlags & binding1_needed_flags) == binding1_needed_flags; }
							)
						)
					)
				}
			};
			auto binding0_memory{ binding0_buffer.dedicated_allocate_memory(binding0_allocateInfo).value() };
			auto binding1_memory{ binding1_buffer.dedicated_allocate_memory(binding1_allocateInfo).value() };
			{
				auto binding0_mapped{ binding0_memory.map<uint8_t>().value() };
				std::cout << std::format("binding0_mapped<T> count: {}, bytes: {}\n", binding0_mapped.size(), binding0_mapped.size_bytes());
				std::ranges::fill(binding0_mapped, 0x55);
				binding0_memory.flush().value();
				binding0_memory.invalidate().value();
				binding0_memory.unmap();
			}


			MemoryMappedFile mmf_spirv{ "simple.comp.glsl.spv" };
			auto s_spirv{ mmf_spirv.get_span<uint32_t>() };
			VkShaderModuleCreateInfo shaderModuleCreateInfo{
				.sType{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO},
				.codeSize{s_spirv.size_bytes()}, //byte size
				.pCode{s_spirv.data()},
			};
			auto shaderModule{ device.create_shader_module(&shaderModuleCreateInfo).value() };

			std::array<VkDescriptorSetLayoutBinding, 2> descriptorSetLayoutBindings{
				{
					{
						.binding{0},
						.descriptorType{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER},
						.descriptorCount{1},
						.stageFlags{VK_SHADER_STAGE_COMPUTE_BIT},
					},
					{
						.binding{1},
						.descriptorType{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER},
						.descriptorCount{1},
						.stageFlags{VK_SHADER_STAGE_COMPUTE_BIT},
					},
				}
			};
			const VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {
				.sType{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO},
				.bindingCount{descriptorSetLayoutBindings.size()},
				.pBindings{descriptorSetLayoutBindings.data()},
			};
			auto descriptorSetLayout{ device.create_descriptor_set_layout(&descriptorSetLayoutCreateInfo).value() };

			std::array<VkDescriptorSetLayout, 1> descriptorSetLayouts{{descriptorSetLayout.descriptorSetLayout}};
			const std::array<VkPushConstantRange, 1> pushConstantRange{{{
				.stageFlags{VK_SHADER_STAGE_COMPUTE_BIT},
				.offset{0},
				.size{sizeof(uint32_t)},
			}}};

			const VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
				.sType{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO},
				.setLayoutCount{descriptorSetLayouts.size()},
				.pSetLayouts{descriptorSetLayouts.data()},
				.pushConstantRangeCount{pushConstantRange.size()},
				.pPushConstantRanges{pushConstantRange.data()},
			};
			auto pipelineLayout{ device.create_pipeline_layout(&pipelineLayoutCreateInfo).value() };

			VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo{
				.sType{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO},
				.stage{VK_SHADER_STAGE_COMPUTE_BIT},
				.module{shaderModule.shaderModule},
				.pName{"main"},
			};
			std::array<VkComputePipelineCreateInfo, 1> computePipelineCreateInfos{{{
				.sType{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO},
				.stage{pipelineShaderStageCreateInfo},
				.layout{pipelineLayout.pipelineLayout},
			}}};
			auto compute_pipelines{ device.create_compute_pipelines(computePipelineCreateInfos).value() };

			std::array<VkDescriptorPoolSize, 1> descriptorPoolSize{{{
				.type{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER},
				.descriptorCount{descriptorSetLayoutBindings.size()},
			}}};
			VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{
				.sType{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO},
				.maxSets{descriptorPoolSize.size()},
				.poolSizeCount{descriptorPoolSize.size()},
				.pPoolSizes{descriptorPoolSize.data()},
			};

			auto descriptor_pool{ device.create_descriptor_pool(&descriptorPoolCreateInfo).value() };

			VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{
				.sType{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO},
				.descriptorPool{descriptor_pool.descriptorPool},
				.descriptorSetCount{descriptorSetLayouts.size()},
				.pSetLayouts{descriptorSetLayouts.data()},

			};
			auto descriptor_sets{ device.create_descriptor_sets(&descriptorSetAllocateInfo).value() };

			std::array<VkDescriptorBufferInfo, 1> binding0_bufferInfos{{{.buffer{binding0_buffer.buffer}, .offset{}, .range{VK_WHOLE_SIZE}}}};
			std::array<VkDescriptorBufferInfo, 1> binding1_bufferInfos{{{.buffer{binding1_buffer.buffer}, .offset{}, .range{VK_WHOLE_SIZE}}}};

			std::array<VkWriteDescriptorSet, 2> descriptorWrites{
				{
					{
						.sType{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET},
						.dstSet{descriptor_sets.at(0).descriptorSet},
						.dstBinding{0},
						.dstArrayElement{0},
						.descriptorCount{binding0_bufferInfos.size()},
						.descriptorType{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER},
						.pBufferInfo{binding0_bufferInfos.data()},
					},
					{
						.sType{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET},
						.dstSet{descriptor_sets.at(0).descriptorSet},
						.dstBinding{1},
						.dstArrayElement{0},
						.descriptorCount{1},
						.descriptorType{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER},
						.pBufferInfo{binding1_bufferInfos.data()},
					}
				}
			};
			device.updateDescriptorSets(descriptorWrites);

			const VkQueryPoolCreateInfo queryPoolCreateInfo{
				.sType{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO},
				.queryType{VK_QUERY_TYPE_TIMESTAMP},
				.queryCount{2},
			};
			auto query_pool{ device.create_query_pool(&queryPoolCreateInfo).value() };

			const VkCommandPoolCreateInfo commandPoolCreateInfo{
				.sType{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO},
				.queueFamilyIndex{static_cast<uint32_t>(std::distance(queue_families.begin(), std::ranges::find(queue_families, VK_QUEUE_COMPUTE_BIT, [](VkQueueFamilyProperties2 &props)-> VkQueueFlagBits {
					return std::bit_cast<VkQueueFlagBits>(props.queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT);
				})))},
			};
			auto command_pool{ device.create_command_pool(&commandPoolCreateInfo).value() };

			const VkCommandBufferAllocateInfo commandBufferAllocateInfo{
				.sType{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO},
				.commandPool{command_pool.commandPool},
				.level{VK_COMMAND_BUFFER_LEVEL_PRIMARY},
				.commandBufferCount{1},
			};
			auto command_buffers{ device.create_command_buffers(&commandBufferAllocateInfo).value() };

			VkCommandBufferBeginInfo commandBufferBeginInfo = {
				.sType{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO},
				.flags{VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT},
			};
			command_buffers.at(0).begin(&commandBufferBeginInfo);
			command_buffers.at(0).bindComputePipeline(compute_pipelines.at(0).pipeline);

			std::vector<VkDescriptorSet> v_descriptor_set;
			v_descriptor_set.reserve(descriptor_sets.size());
			std::ranges::transform(descriptor_sets, std::back_inserter(v_descriptor_set), [](vk::DescriptorSet &descriptorSet)-> VkDescriptorSet { return descriptorSet.descriptorSet; });

			command_buffers.at(0).bindComputeDescriptorSets(pipelineLayout.pipelineLayout, v_descriptor_set);
			std::array<uint32_t, 1> mask{ 0xff0000ffu };
			command_buffers.at(0).pushComputeConstants<uint32_t>(pipelineLayout.pipelineLayout, mask);
			command_buffers.at(0).cmdResetQueryPool(query_pool.queryPool, 0, 2);
			command_buffers.at(0).writeComputeTimestamp(query_pool.queryPool, 0);
			command_buffers.at(0).dispatch();
			command_buffers.at(0).writeComputeTimestamp(query_pool.queryPool, 1);

			command_buffers.at(0).compute_to_transfer_barrier();

			std::array<VkBufferCopy2, 1> a_regions{{{
				.sType{VK_STRUCTURE_TYPE_BUFFER_COPY_2},
				.srcOffset{0},
				.dstOffset{0},
				.size{binding0_mem_requirements.memoryRequirements.size},
			}}};
			const VkCopyBufferInfo2 pCopyBufferInfo{
				.sType{VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2},
				.srcBuffer{binding1_buffer.buffer},
				.dstBuffer{binding0_buffer.buffer},
				.regionCount{a_regions.size()},
				.pRegions{a_regions.data()},
			};
			command_buffers.at(0).copy(&pCopyBufferInfo);
			command_buffers.at(0).end();

			VkDeviceQueueInfo2 queueInfo{
				.sType{VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2},
				.queueFamilyIndex{commandPoolCreateInfo.queueFamilyIndex},
				.queueIndex{0}
			};
			auto queue{ device.get_queue(&queueInfo) };

			std::vector<VkCommandBuffer> v_command_buffers;
			v_command_buffers.reserve(command_buffers.size());
			std::ranges::transform(command_buffers, std::back_inserter(v_command_buffers), [](vk::CommandBuffer& commandBuffer)-> VkCommandBuffer { return commandBuffer.commandBuffer; });

			std::array<VkSubmitInfo, 1> submits{{{
				.sType{ VK_STRUCTURE_TYPE_SUBMIT_INFO },
				.commandBufferCount{static_cast<uint32_t>(v_command_buffers.size())},
				.pCommandBuffers{v_command_buffers.data()},
			}}};
			queue.submit(submits).value();
			queue.waitIdle().value();

			{
				auto binding0_mapped{ binding0_memory.map<uint32_t>().value().first(256) };
				std::cout << std::format("binding0_mapped<T> count: {}, bytes: {}\n", binding0_mapped.size(), binding0_mapped.size_bytes());
				//binding0_memory.flush().value();
				//binding0_memory.invalidate().value();
				for (auto b : binding0_mapped) {
					std::cout << std::format("{:08x} ", b);
				}
				std::cout << "\n";
				binding0_memory.unmap();
			}
			auto query_results{ query_pool.get_results<uint64_t, 2>(VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT).value() };

			std::chrono::nanoseconds run_time{ static_cast<int64_t>( static_cast<double>(query_results[1] - query_results[0]) * static_cast<double>(physical_device_properties.limits.timestampPeriod)) };
			std::cout << std::format("query_diff: {}\n", run_time);
		}
	}
};

