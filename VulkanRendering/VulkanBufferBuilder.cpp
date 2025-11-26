///******************************************************************************
//This file is part of the Newcastle Vulkan Tutorial Series
//
//Author:Rich Davison
//Contact:richgdavison@gmail.com
//License: MIT (see LICENSE file at the top of the source tree)
//*//////////////////////////////////////////////////////////////////////////////
//#include "VulkanBufferBuilder.h"
//#include "VulkanBuffers.h"
//#include "VulkanUtils.h"
//
//using namespace NCL;
//using namespace Rendering;
//using namespace Vulkan;
//
//BufferBuilder::BufferBuilder(vk::Device device, VmaAllocator allocator) {
//	m_sourceDevice	= device;
//	m_sourceAllocator = allocator;
//	m_vmaCreateInfo = {};
//	m_vmaCreateInfo.usage		= VMA_MEMORY_USAGE_AUTO;
//}
//
//BufferBuilder::BufferBuilder(VkDevice device, VmaAllocator allocator) {
//	m_sourceDevice = device;
//	m_sourceAllocator = allocator;
//	m_vmaCreateInfo = {};
//	m_vmaCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
//}
//
//BufferBuilder& BufferBuilder::WithBufferUsage(vk::BufferUsageFlags flags) {
//	m_vkCreateInfo.usage = flags;
//	return *this;
//}
//
//BufferBuilder& BufferBuilder::WithBufferUsage(VkBufferUsageFlags flags) {
//	m_vkCreateInfo.usage = vk::BufferUsageFlags(flags);
//	return *this;
//}
//
//BufferBuilder& BufferBuilder::WithMemoryProperties(vk::MemoryPropertyFlags flags) {
//	m_vmaCreateInfo.requiredFlags = (VkMemoryPropertyFlags)flags;
//	return *this;
//}
//
//BufferBuilder& BufferBuilder::WithMemoryProperties(VkMemoryPropertyFlags flags) {
//	m_vmaCreateInfo.requiredFlags = flags;
//	return *this;
//}
//
//BufferBuilder& BufferBuilder::WithHostVisibility() {
//	m_vmaCreateInfo.requiredFlags |= (VkMemoryPropertyFlags)vk::MemoryPropertyFlagBits::eHostVisible;
//	m_vmaCreateInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
//
//	return *this;
//}
///*
//This can be added via WithBufferUsage just fine, but it seems very
//separated from the other usages - everything else describes what
//you might want to store IN the buffer, while this is an intrinsic
//property OF the buffer(i.e it has an address, it doesn't store addresses)
//*/
//BufferBuilder& BufferBuilder::WithDeviceAddress() {
//	m_vkCreateInfo.usage |= vk::BufferUsageFlagBits::eShaderDeviceAddress;
//	return *this;
//}
//
//BufferBuilder& BufferBuilder::WithPersistentMapping() {
//	m_vmaCreateInfo.requiredFlags |= (VkMemoryPropertyFlags)vk::MemoryPropertyFlagBits::eHostCoherent;
//
//	m_vmaCreateInfo.flags |= (VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
//	return *this;
//}
//
//BufferBuilder& BufferBuilder::WithUniqueAllocation() {
//	m_vmaCreateInfo.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
//	return *this;
//}
//
//BufferBuilder& BufferBuilder::WithConcurrentSharing() {
//	m_vkCreateInfo.sharingMode = vk::SharingMode::eConcurrent;
//	return *this;
//}
//
//VulkanBuffer BufferBuilder::Build(size_t byteSize, const std::string& debugName) {
//	VulkanBuffer	outputBuffer;
//
//	outputBuffer.size = byteSize;
//	m_vkCreateInfo.size = byteSize;
//
//	outputBuffer.allocator = m_sourceAllocator;
//
//	vmaCreateBuffer(m_sourceAllocator, (VkBufferCreateInfo*)&m_vkCreateInfo, &m_vmaCreateInfo, (VkBuffer*)&(outputBuffer.buffer), &outputBuffer.allocationHandle, &outputBuffer.allocationInfo);
//
//	if (m_vkCreateInfo.usage & vk::BufferUsageFlagBits::eShaderDeviceAddress) {
//		outputBuffer.deviceAddress = m_sourceDevice.getBufferAddress(
//			{
//				.buffer = outputBuffer.buffer
//			}
//		);
//	}
//
//	if (!debugName.empty()) {
//		SetDebugName(m_sourceDevice, vk::ObjectType::eBuffer, GetVulkanHandle(outputBuffer.buffer), debugName);
//	}
//
//	return outputBuffer;
//}