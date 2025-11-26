/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanPipeline.h"
#include "VulkanShaderModule.h"
#include "VulkanUtils.h"
#include "SmartTypes.h"

namespace NCL::Rendering::Vulkan {
	class VulkanRenderer;
	class VulkanShader;

	struct VulkanVertexSpecification;

	template <class T, class P>
	class PipelineBuilderBase	{
	public:

		T& WithLayout(vk::PipelineLayout pipeLayout) {
			m_layout = pipeLayout;
			m_pipelineCreate.setLayout(pipeLayout);
			return (T&)*this;
		}

		T& WithDescriptorSetLayout(uint32_t setIndex, vk::DescriptorSetLayout m_layout) {
			assert(setIndex < 32);
			if (setIndex >= m_userLayouts.size()) {
				vk::DescriptorSetLayout nullLayout = Vulkan::GetNullDescriptor(m_sourceDevice);
				while (m_userLayouts.size() <= setIndex) {
					m_userLayouts.push_back(nullLayout);
				}
			}
			m_userLayouts[setIndex] = m_layout;
			return (T&)*this;
		}

		T& WithDescriptorSetLayout(uint32_t setIndex, const vk::UniqueDescriptorSetLayout& m_layout) {
			return WithDescriptorSetLayout(setIndex, *m_layout);
		}

		T& WithDescriptorBuffers() {
			m_pipelineCreate.flags |= vk::PipelineCreateFlagBits::eDescriptorBufferEXT;
			return (T&)*this;
		}

		P& GetCreateInfo() {
			return m_pipelineCreate;
		}

	protected:
		PipelineBuilderBase(vk::Device device) {
			m_sourceDevice = device;
		}
		~PipelineBuilderBase() {}

		void FillShaderState(VulkanPipeline& output) {
			for (int i = 0; i < m_usedModules.size(); ++i) {
				vk::PipelineShaderStageCreateInfo stageInfo;

				stageInfo.pName = m_moduleEntryPoints[i].c_str();
				stageInfo.stage = m_usedModules[i]->m_shaderStage;
				stageInfo.module = *m_usedModules[i]->m_shaderModule;

				m_shaderStages.push_back(stageInfo);
			}

			m_pipelineCreate.setStageCount(m_shaderStages.size());
			m_pipelineCreate.setPStages(m_shaderStages.data());

			if (m_externalLayout) {
				m_pipelineCreate.setLayout(m_externalLayout);
			}
			else {
				vk::PipelineLayoutCreateInfo pipeLayoutCreate = vk::PipelineLayoutCreateInfo();

				for (auto& module : m_usedModules) {
					module->CombineLayoutBindings(output.m_allLayoutsBindings);
					module->CombinePushConstantRanges(output.m_pushConstants);
				}
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

				pipeLayoutCreate.setSetLayouts(output.m_allLayouts);
				pipeLayoutCreate.setPushConstantRanges(output.m_pushConstants);

				output.layout = m_sourceDevice.createPipelineLayoutUnique(pipeLayoutCreate);
				m_pipelineCreate.setLayout(*output.layout);
			}
		}

	protected:
		P m_pipelineCreate;
		vk::PipelineLayout	m_layout;
		vk::Device			m_sourceDevice;

		vk::PipelineLayout						m_externalLayout;

		std::vector< vk::DescriptorSetLayout>	m_userLayouts;

		std::vector<vk::PipelineShaderStageCreateInfo>	m_shaderStages;
		std::vector<const VulkanShaderModule*>			m_usedModules;
		std::vector<UniqueVulkanShaderModule>			m_loadedShaderModules;
		std::vector<std::string>						m_moduleEntryPoints;
	};
}