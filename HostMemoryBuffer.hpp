#pragma once

namespace vk_route {
	class HostMemoryBuffer {
	public:
		vk::StructureChain<vk::BufferCreateInfo> bci;
		vk::StructureChain<vk::MemoryRequirements2, vk::MemoryDedicatedRequirements> requirements;
		vk::StructureChain<vk::MemoryAllocateInfo> allocateInfo;
		vk::UniqueDeviceMemory memory;
		vk::UniqueBuffer buffer;
		vk::Result bindResult;
		void* host_pointer;

		template<typename T>
		std::span<T> get_span() const noexcept {
			return std::span<T>(reinterpret_cast<T*>(host_pointer), bci.get<vk::BufferCreateInfo>().size / sizeof(T));
		}

		static vk::StructureChain<vk::BufferCreateInfo> make_bci(std::span<uint32_t> queueFamilyIndices, vk::DeviceSize bufferSize, vk::BufferUsageFlags bufferUsage) noexcept {
			vk::StructureChain<vk::BufferCreateInfo> bci;
			bci.get<vk::BufferCreateInfo>().setSize(bufferSize);
			bci.get<vk::BufferCreateInfo>().setUsage(bufferUsage);
			bci.get<vk::BufferCreateInfo>().setSharingMode(vk::SharingMode::eExclusive);
			return bci;
		}

		static vk::StructureChain<vk::MemoryAllocateInfo> make_allocate_info(
            vk::UniqueDevice& device,
            std::span<vk::MemoryHeap> memory_heaps,
            std::span<vk::MemoryType> memory_types,
            vk::StructureChain<vk::MemoryRequirements2, vk::MemoryDedicatedRequirements>& requirements
        ) noexcept {

			std::bitset<32> allowed_types(static_cast<uint64_t>(requirements.get<vk::MemoryRequirements2>().memoryRequirements.memoryTypeBits));
			vk::MemoryPropertyFlags needed_flags{ vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostCached };
			for (uint32_t allowed_type_idx = 0; allowed_type_idx < allowed_types.size(); allowed_type_idx++) {
				if (!allowed_types[allowed_type_idx]) continue;
				decltype(auto) memoryType{ memory_types[allowed_type_idx] };
				decltype(auto) memoryHeap{ memory_heaps[memoryType.heapIndex] };
				if (requirements.get<vk::MemoryRequirements2>().memoryRequirements.size > memoryHeap.size) {
					allowed_types.reset(allowed_type_idx);
				}
				if ((memoryType.propertyFlags & needed_flags) != needed_flags) {
					allowed_types.reset(allowed_type_idx);
				}
			}
#ifdef _DEBUG
			for (uint32_t allowed_type_idx = 0; allowed_type_idx < allowed_types.size(); allowed_type_idx++) {
				if (!allowed_types[allowed_type_idx]) continue;
				decltype(auto) memoryType{ memory_types[allowed_type_idx] };
				std::cout << std::format("allowed heap[{}] type[{}] properties: {}\n", memoryType.heapIndex, allowed_type_idx, vk::to_string(memoryType.propertyFlags));
			}
#endif
			vk::StructureChain<vk::MemoryAllocateInfo> allocateInfo;
			allocateInfo.get<vk::MemoryAllocateInfo>().setAllocationSize(requirements.get<vk::MemoryRequirements2>().memoryRequirements.size);
			allocateInfo.get<vk::MemoryAllocateInfo>().memoryTypeIndex = static_cast<uint32_t>(
				std::distance(
					memory_types.begin(),
					std::ranges::find_if(
						memory_types,
						[&, memory_type_index = 0u](const vk::MemoryType& memoryType) mutable -> bool {
							const uint32_t current_memory_type_index{ memory_type_index++ };
							if (!allowed_types[current_memory_type_index]) return false;
							return true;
						}
					)
				)
			);
#ifdef _DEBUG
			std::cout << std::format("heap[{}] type[{}] properties: {}\n",
				memory_types[allocateInfo.get<vk::MemoryAllocateInfo>().memoryTypeIndex].heapIndex,
				allocateInfo.get<vk::MemoryAllocateInfo>().memoryTypeIndex,
				vk::to_string(memory_types[allocateInfo.get<vk::MemoryAllocateInfo>().memoryTypeIndex].propertyFlags));
#endif
			return allocateInfo;
		}

		HostMemoryBuffer(
			vk::UniqueDevice& device,
			std::span<uint32_t> queueFamilyIndices,
			std::span<vk::MemoryHeap> memory_heaps,
			std::span<vk::MemoryType> memory_types,
			vk::StructureChain<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceExternalMemoryHostPropertiesEXT>& physical_device_properties,
			vk::DeviceSize bufferSize,
			vk::BufferUsageFlags bufferUsage,
			std::string bufferLabel
		) noexcept :
			bci{ make_bci(queueFamilyIndices, bufferSize, bufferUsage) },
			requirements{ device->getBufferMemoryRequirements<vk::MemoryRequirements2, vk::MemoryDedicatedRequirements>(vk::DeviceBufferMemoryRequirements{.pCreateInfo{&bci.get<vk::BufferCreateInfo>()} }) },
			allocateInfo{ make_allocate_info(device, memory_heaps, memory_types, requirements) },
			memory{ label(device, bufferLabel, device->allocateMemoryUnique(allocateInfo.get<vk::MemoryAllocateInfo>()).value) },
			buffer{ label(device, bufferLabel, device->createBufferUnique(bci.get<vk::BufferCreateInfo>()).value) },
			bindResult{ device->bindBufferMemory2({ {.buffer{buffer.get()}, .memory{memory.get()}, .memoryOffset{0ull} } }) },
			host_pointer{ device->mapMemory(memory.get(), 0, VK_WHOLE_SIZE).value }
		{
#ifdef _DEBUG
			std::cout << std::format("{} mem_requirements: {}\n", bufferLabel, requirements.get<vk::MemoryRequirements2>().memoryRequirements.size);
#endif
		}
	};
};