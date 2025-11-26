///******************************************************************************
//This file is part of the Newcastle Vulkan Tutorial Series
//
//Author:Rich Davison
//Contact:richgdavison@gmail.com
//License: MIT (see LICENSE file at the top of the source tree)
//*//////////////////////////////////////////////////////////////////////////////
//#pragma once
//#include "VulkanBuffers.h"
//
//namespace NCL::Rendering::Vulkan {
//	struct VulkanBuffer;
//
//	class BufferBuilder	{
//	public:
//		BufferBuilder(vk::Device device, VmaAllocator m_allocator);
//		BufferBuilder(VkDevice m_device, VmaAllocator m_allocator);
//
//		BufferBuilder& WithBufferUsage(vk::BufferUsageFlags flags);
//		BufferBuilder& WithBufferUsage(VkBufferUsageFlags flags);
//
//		BufferBuilder& WithMemoryProperties(vk::MemoryPropertyFlags flags);
//		BufferBuilder& WithMemoryProperties(VkMemoryPropertyFlags flags);
//
//		//Makes the buffer host visible, with mapping and random access
//		BufferBuilder& WithHostVisibility();
//		//Makes the buffer have an associated device address
//		BufferBuilder& WithDeviceAddress();
//
//		//Makes the buffer host coherent, with mapping and random access
//		BufferBuilder& WithPersistentMapping();
//
//		//Indicates to VMA that a new physical memory allocation must be made
//		BufferBuilder& WithUniqueAllocation();
//
//		BufferBuilder& WithConcurrentSharing();
//
//		~BufferBuilder() {};
//
//		VulkanBuffer Build(size_t byteSize, const std::string& name = "");
//
//	protected:
//		vk::Device					m_sourceDevice;
//		VmaAllocator				m_sourceAllocator;
//		VmaAllocationCreateInfo		m_vmaCreateInfo;
//		vk::BufferCreateInfo		m_vkCreateInfo;
//	};
//}
