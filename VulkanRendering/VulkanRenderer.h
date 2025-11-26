/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "../NCLCoreClasses/RendererBase.h"
#include "../NCLCoreClasses/Maths.h"
#include "VulkanMemoryManager.h"
#include "VulkanPipeline.h"
#include "SmartTypes.h"
#include "vma/vk_mem_alloc.h"
using std::string;

namespace NCL::Rendering::Vulkan {
	namespace CommandType {
		enum Type : uint32_t {
			Graphics,
			AsyncCompute,
			Copy,
			Present,
			MAX_COMMAND_TYPES
		};
	};

	struct FrameContext {
		vk::Device			device;
		vk::DescriptorPool  descriptorPool;
		vk::CommandBuffer	cmdBuffer;		
		
		vk::CommandPool		commandPools[CommandType::Type::MAX_COMMAND_TYPES];
		vk::Queue			queues[CommandType::Type::MAX_COMMAND_TYPES];
		uint32_t			queueFamilies[CommandType::Type::MAX_COMMAND_TYPES];

		vk::Image			colourImage;
		vk::ImageView		colourView;
		vk::Format			colourFormat;

		vk::Image			depthImage;
		vk::ImageView		depthView;
		vk::Format			depthFormat;

		vk::Viewport		viewport;
		vk::Rect2D			screenRect;

		uint32_t			frameID = 0;
		uint32_t			cycleID = 0;
		uint64_t			waitID	= 0;
	};

	struct ChainState {
		vk::Framebuffer		frameBuffer;
		vk::Image			colourImage;
		vk::ImageView		colourView;
		vk::Format			colourFormat;

		vk::Semaphore		acquireSempaphore;
		vk::Fence			acquireFence;

		vk::Semaphore		presentSempaphore;
		vk::Fence			presentFence;

		vk::CommandBuffer	swapCmds;
	};

	struct VulkanInitialisation {
		vk::Format			depthStencilFormat	= vk::Format::eD32SfloatS8Uint;
		vk::PresentModeKHR  idealPresentMode	= vk::PresentModeKHR::eFifo;

		vk::PhysicalDeviceType idealGPU		= vk::PhysicalDeviceType::eDiscreteGpu;


		vk::DescriptorPoolCreateFlags	defaultDescriptorPoolFlags = {};

		uint32_t	defaultDescriptorPoolBufferCount	= 128;
		uint32_t	defaultDescriptorPoolImageCount		= 128;
		uint32_t	defaultDescriptorPoolSamplerCount	= 32;
		uint32_t	defaultDescriptorPoolAccelerationStructureCount	= 0;

		VmaAllocatorCreateFlags		vmaFlags = {};

		uint32_t	majorVersion	= 1;
		uint32_t	minorVersion	= 1;

		uint32_t	framesInFlight	= 1;

		bool				autoTransitionFrameBuffer	= true;
		bool				autoBeginDynamicRendering	= true;
		bool				useOpenGLCoordinates		= false;
		bool				skipDynamicState			= false;

		std::vector<void*>			features;

		std::vector<const char*>	instanceExtensions;
		std::vector<const char*>	instanceLayers;

		std::vector<const char*>	deviceExtensions;	
		std::vector<const char*>	deviceLayers;
	};

	class VulkanRenderer : public RendererBase {
		friend class VulkanMesh;
		friend class VulkanTexture;
	public:
		VulkanRenderer(Window& window, const VulkanInitialisation& vkInit);
		~VulkanRenderer();

		virtual bool HasInitialised() const { return m_device; }

		vk::Device GetDevice() const {
			return m_device;
		}

		vk::PhysicalDevice GetPhysicalDevice() const {
			return m_physicalDevice;
		}

		vk::Instance	GetVulkanInstance() const {
			return m_instance;
		}

		vk::Queue GetQueue(CommandType::Type type) const {
			return m_queues[type];
		}

		uint32_t GetQueueFamily(CommandType::Type type) const {
			return m_queueFamilies[type];
		}

		vk::CommandPool GetCommandPool(CommandType::Type type) const {
			return m_commandPools[type];
		}

		vk::DescriptorPool GetDescriptorPool() {
			return m_defaultDescriptorPool;
		}

		FrameContext const& GetFrameContext() const {
			return m_frameContexts[m_currentFrameContext];
		}

		bool SupportsAsyncCompute() const;
		bool SupportsAsyncCopy()	const;
		bool SupportsAsyncPresent() const;

		bool MemoryTypeFromPhysicalDeviceProps(vk::MemoryPropertyFlags requirements, uint32_t type, uint32_t& index);

		void WaitForGPUIdle();

		void	BeginDefaultRenderPass(vk::CommandBuffer cmds);
		void	BeginDefaultRendering(vk::CommandBuffer  cmds);

		void BeginFrame()		override;
		void RenderFrame()		override;
		void EndFrame()			override;
		void SwapBuffers()		override;

		void OnWindowResize(int w, int h)	override;
	protected:

		virtual void	CompleteResize();
		virtual void	InitDefaultRenderPass();
		virtual void	InitDefaultDescriptorPool();

		virtual void WaitForSwapImage();

	protected:	
		vk::Viewport			m_defaultViewport;
		vk::Rect2D				m_defaultScissor;	
		vk::Rect2D				m_defaultScreenRect;			
		vk::RenderPass			m_defaultRenderPass;
		vk::RenderPassBeginInfo m_defaultBeginInfo;
		
		vk::DescriptorPool		m_defaultDescriptorPool;	//descriptor sets come from here!

		vk::CommandPool			m_commandPools[CommandType::Type::MAX_COMMAND_TYPES];
		vk::Queue				m_queues[CommandType::Type::MAX_COMMAND_TYPES];
		uint32_t				m_queueFamilies[CommandType::Type::MAX_COMMAND_TYPES];

		vk::CommandBuffer		m_frameCmds;

		VulkanInitialisation	m_vkInit;
	private: 
		void	InitCommandPools();
		bool	InitInstance();
		bool	InitPhysicalDevice();
		bool	InitGPUDevice();
		bool	InitSurface();
		void	InitBufferChain(vk::CommandBuffer  m_cmdBuffer);

		void	InitFrameStates(uint32_t framesInFlight);

		void	CreateDepthBufer(uint32_t width, uint32_t height);

		static VkBool32 DebugCallbackFunction(
			vk::DebugUtilsMessageSeverityFlagBitsEXT			messageSeverity,
			vk::DebugUtilsMessageTypeFlagsEXT					messageTypes,
			const vk::DebugUtilsMessengerCallbackDataEXT*		pCallbackData,
			void*												pUserData);

		bool	InitDeviceQueueIndices();

		vk::Instance		m_instance;			//API Instance
		vk::PhysicalDevice	m_physicalDevice;	//GPU in use

		vk::PhysicalDeviceProperties		m_deviceProperties;
		vk::PhysicalDeviceMemoryProperties	m_deviceMemoryProperties;
		vk::DebugUtilsMessengerEXT			m_debugMessenger;

		vk::PipelineCache	m_pipelineCache;
		vk::Device			m_device;		//Device handle	

		vk::SwapchainKHR	m_swapChain;
		vk::SurfaceKHR		m_surface;
		vk::Format			m_surfaceFormat;
		vk::ColorSpaceKHR	m_surfaceSpace;
		//New depth buffer
		vk::Image			m_depthImage;
		vk::ImageView		m_depthView;
		vk::DeviceMemory	m_depthMemory;

		std::vector<FrameContext>	m_frameContexts;
		std::vector<ChainState>		m_swapStates;
		FrameContext*				m_currentContext	= nullptr;
		ChainState*					m_currentSwapState	= nullptr;

		uint32_t					m_currentFrameContext	= 0; //Frame context for this frame
		uint32_t					m_currentSwap			= 0; //To index our swapchain 
		uint64_t					m_globalFrameID			= 0;

		vk::Semaphore				m_flightSempaphore;
	};
}