/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "VulkanBuffer.h"
#include "VulkanMemoryManager.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;


VulkanBuffer::VulkanBuffer() {
	buffer = nullptr;
	size = 0;
	deviceAddress = 0;
}

VulkanBuffer::VulkanBuffer(VulkanBuffer&& obj) : Buffer(std::move(obj)) {
	buffer			= obj.buffer;
	deviceAddress	= obj.deviceAddress;
	size			= obj.size;
	sourceManager	= obj.sourceManager;
	obj.buffer		= VK_NULL_HANDLE;
	obj.sourceManager = nullptr;
	obj.size		= 0;
}

VulkanBuffer& VulkanBuffer::operator=(VulkanBuffer&& obj) {
	if (this != &obj) {
		buffer			= obj.buffer;
		deviceAddress	= obj.deviceAddress;
		size			= obj.size;
		sourceManager	= obj.sourceManager;
		assetID			= obj.assetID;

		obj.buffer		= VK_NULL_HANDLE;
		obj.sourceManager = nullptr;
		obj.size		= 0;
	}
	return *this;
}

VulkanBuffer::~VulkanBuffer() {
	if (buffer && sourceManager) {
		sourceManager->DiscardBuffer(*this);
	}
}

void VulkanBuffer::CopyData(void* data, size_t size) const {
	sourceManager->CopyData(*this, data, size);
}

void* VulkanBuffer::Map() const {
	return sourceManager->MapBuffer(*this);
}

void	VulkanBuffer::Unmap() const {
	sourceManager->UnmapBuffer(*this);
}