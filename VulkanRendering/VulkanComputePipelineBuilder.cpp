/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "VulkanComputePipelineBuilder.h"
#include "VulkanUtils.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

ComputePipelineBuilder::ComputePipelineBuilder(vk::Device device) : PipelineBuilderBase(device){
	m_module = nullptr;
}

ComputePipelineBuilder& ComputePipelineBuilder::WithShaderBinary(const std::string& filename, const std::string& entrypoint) {
	m_loadedShaderModules.push_back(std::make_unique<VulkanShaderModule>(filename, vk::ShaderStageFlagBits::eCompute, m_sourceDevice));
	m_module		= m_loadedShaderModules.back().get();
	m_entryPoint	= entrypoint;
	return *this;
}

ComputePipelineBuilder& ComputePipelineBuilder::WithShaderModule(const VulkanShaderModule& module, const std::string& entrypoint) {
	m_module		= &module;
	m_entryPoint	= entrypoint;
	return *this;
}
//
//ComputePipelineBuilder& ComputePipelineBuilder::WithShaderBinary(const UniqueVulkanCompute& compute) {
//	//compute->FillShaderStageCreateInfo(m_pipelineCreate);
//	//compute->FillDescriptorSetLayouts(m_reflectionLayouts);
//	//compute->FillPushConstants(m_pushConstants);
//	return *this;
//}
//
//ComputePipelineBuilder& ComputePipelineBuilder::WithShaderModule(const VulkanCompute& compute) {
//	//compute.FillShaderStageCreateInfo(m_pipelineCreate);
//	//compute.FillDescriptorSetLayouts(m_reflectionLayouts);
//	//compute.FillPushConstants(m_pushConstants);
//	return *this;
//}

VulkanPipeline	ComputePipelineBuilder::Build(const std::string& debugName, vk::PipelineCache cache) {
	VulkanPipeline output;
	assert(m_module);

	m_module->CombineLayoutBindings(output.m_allLayoutsBindings);
	m_module->CombinePushConstantRanges(output.m_pushConstants);
	
	output.m_allLayouts.resize(output.m_allLayoutsBindings.size());

	for (int i = 0; i < output.m_allLayoutsBindings.size(); ++i) {
		if (i < m_userLayouts.size() && m_userLayouts[i]) {
			output.m_allLayouts[i] = m_userLayouts[i];
		}
		else {
			vk::DescriptorSetLayoutCreateInfo createInfo;
			createInfo.setBindings(output.m_allLayoutsBindings[i]);
			output.m_createdLayouts.push_back(m_sourceDevice.createDescriptorSetLayoutUnique(createInfo));
			output.m_allLayouts[i] = output.m_createdLayouts.back().get();
		}
	}

	vk::PipelineLayoutCreateInfo pipeLayoutCreate = vk::PipelineLayoutCreateInfo();
	pipeLayoutCreate.setSetLayouts(output.m_allLayouts);
	pipeLayoutCreate.setPushConstantRanges(output.m_pushConstants);

	output.layout = m_sourceDevice.createPipelineLayoutUnique(pipeLayoutCreate);

	vk::PipelineShaderStageCreateInfo	m_createInfo;
	m_createInfo.stage	= vk::ShaderStageFlagBits::eCompute;
	m_createInfo.module = *m_module->m_shaderModule;
	m_createInfo.pName	= m_entryPoint.c_str();

	m_pipelineCreate.setLayout(*output.layout);
	m_pipelineCreate.setStage(m_createInfo);

	output.pipeline = m_sourceDevice.createComputePipelineUnique(cache, m_pipelineCreate).value;

	if (!debugName.empty()) {
		SetDebugName(m_sourceDevice, vk::ObjectType::ePipeline, GetVulkanHandle(*output.pipeline), debugName);
	}

	return output;
}