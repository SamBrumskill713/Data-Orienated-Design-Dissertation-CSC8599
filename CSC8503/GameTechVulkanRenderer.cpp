#ifdef USEVULKAN
#include "GameTechVulkanRenderer.h"
#include "VulkanVMAMemoryManager.h"

#include "GameObject.h"
#include "RenderObject.h"
#include "Camera.h"
#include "VulkanUtils.h"
#include "MshLoader.h"
#include "SimpleFont.h"
#include "GameWorld.h"

#include "Debug.h"

#include <filesystem>

using namespace NCL;
using namespace Rendering;
using namespace CSC8503;
using namespace Vulkan;

const uint32_t DEFAULT_TEXTURE		= 0;
const uint32_t DEBUGTEXT_TEXTURE	= 1;

const int _SHADOWSIZE = 8192;
const int _FRAMECOUNT = 2;

const int _TEXCOUNT = 128;

const size_t _LINE_STRIDE = sizeof(Vector4) + sizeof(Vector4);
const size_t _TEXT_STRIDE = sizeof(Vector2) + sizeof(Vector4) + sizeof(Vector2);

const size_t _TEXTURE_INSTANCE_STRIDE	= sizeof(Vector2) + sizeof(Vector4) + sizeof(int32_t);

static vk::PhysicalDeviceRobustness2FeaturesEXT robustness{
	.nullDescriptor = true
};

static vk::PhysicalDeviceSynchronization2Features syncFeatures{
	.synchronization2 = true
};

static vk::PhysicalDeviceDynamicRenderingFeaturesKHR dynamicRendering{
	.dynamicRendering = true
};

static vk::PhysicalDeviceTimelineSemaphoreFeatures timelineSemaphores{
	.timelineSemaphore = true
};

static vk::PhysicalDeviceScalarBlockLayoutFeatures scalarFeatures{
	.scalarBlockLayout = true
};

static VulkanInitialisation vkInitState = {
	.depthStencilFormat = vk::Format::eD32SfloatS8Uint,
	.idealPresentMode	= vk::PresentModeKHR::eMailbox,

	.majorVersion	= 1,
	.minorVersion	= 3,

	.framesInFlight = 1,

	.autoBeginDynamicRendering = false,
	
	.features{
		(void*)&robustness,
		(void*)&syncFeatures,
		(void*)&dynamicRendering,
		(void*)&timelineSemaphores,
		(void*)&scalarFeatures,
	},

	.instanceExtensions{
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
	},
	.instanceLayers{
		"VK_LAYER_KHRONOS_validation"
	},
	.deviceExtensions{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		"VK_KHR_dynamic_rendering",
		"VK_KHR_maintenance4",
		"VK_KHR_depth_stencil_resolve",
		"VK_KHR_create_renderpass2",
		"VK_KHR_synchronization2",
		VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
		"VK_EXT_robustness2"
	},	
	.deviceLayers{
		"VK_LAYER_LUNARG_standard_validation"
	},
};

GameTechVulkanRenderer::GameTechVulkanRenderer(GameWorld& world) : VulkanRenderer(*Window::GetWindow(), vkInitState), gameWorld(world)  {
	m_memoryManager = new VulkanVMAMemoryManager(GetDevice(), GetPhysicalDevice(), GetVulkanInstance(), m_vkInit);

	GLTFLoader::SetMeshConstructionFunction(
		[&]()-> std::shared_ptr<Mesh> {return std::shared_ptr<Mesh>(new VulkanMesh()); }
	);

	GLTFLoader::SetTextureConstructionFunction(
		[&](std::string& input) ->  SharedTexture {
			return std::shared_ptr<Texture>(nullptr); 
		}
	);

	InitStructures();
}

GameTechVulkanRenderer::~GameTechVulkanRenderer() {
	delete[] allFrames;
	for (Mesh* m : loadedMeshes) {
		delete m;
	}
}

void	GameTechVulkanRenderer::InitStructures() {
	FrameContext const& frameState = GetFrameContext();

	{//Setting up the data for the skybox
		quadMesh = GenerateQuad();

		cubeTex = LoadCubemap(
			"Cubemap/skyrender0004.png", "Cubemap/skyrender0001.png",
			"Cubemap/skyrender0003.png", "Cubemap/skyrender0006.png",
			"Cubemap/skyrender0002.png", "Cubemap/skyrender0005.png",
			"Cubemap Texture!"
		);

		skyboxPipeline = PipelineBuilder(frameState.device)
			.WithVertexInputState(quadMesh->GetVertexInputState())
			.WithTopology(quadMesh->GetVulkanTopology())
			.WithShaderBinary("skybox.vert.spv", vk::ShaderStageFlagBits::eVertex)
			.WithShaderBinary("skybox.frag.spv", vk::ShaderStageFlagBits::eFragment)
			.WithColourAttachment(frameState.colourFormat)
			.WithDepthAttachment(frameState.depthFormat, vk::CompareOp::eAlways, true, false)
			.Build("CubeMapRenderer Skybox Pipeline");
	}

	{//Setting up all the data we need for shadow maps!
	shadowMap = TextureBuilder(frameState.device, *m_memoryManager)
		.WithCommandBuffer(frameState.cmdBuffer)
		.WithDimension(_SHADOWSIZE, _SHADOWSIZE)
		.WithAspects(vk::ImageAspectFlagBits::eDepth)
		.WithFormat(vk::Format::eD32Sfloat)
		.WithLayout(vk::ImageLayout::eDepthAttachmentOptimal)
		.WithUsages(vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled)
		.WithMips(false)
		.Build("Shadowmap");
	}

	{//Texture stuff, including creating samplers, and our bindless buffer of texture descriptors
		defaultSampler = frameState.device.createSamplerUnique(
			vk::SamplerCreateInfo()
			.setAnisotropyEnable(true)
			.setMaxAnisotropy(16)
			.setMinFilter(vk::Filter::eLinear)
			.setMagFilter(vk::Filter::eLinear)
			.setMipmapMode(vk::SamplerMipmapMode::eLinear)
			.setMaxLod(80.0f)
		);
		textSampler = frameState.device.createSamplerUnique(
			vk::SamplerCreateInfo()
			.setAnisotropyEnable(false)
			.setMaxAnisotropy(1)
			.setMinFilter(vk::Filter::eNearest)
			.setMagFilter(vk::Filter::eNearest)
			.setMipmapMode(vk::SamplerMipmapMode::eNearest)
			.setMaxLod(0.0f)
		);
	}
}

//Defer making the scene pipelines until a mesh has been loaded
//We're assuming that all loaded meshes have the same vertex format...
void GameTechVulkanRenderer::BuildScenePipelines(VulkanMesh* m) {
	FrameContext const& context = GetFrameContext();

	scenePipeline = PipelineBuilder(context.device)
		.WithVertexInputState(m->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShaderBinary("scene.vert.spv", vk::ShaderStageFlagBits::eVertex)
		.WithShaderBinary("scene.frag.spv", vk::ShaderStageFlagBits::eFragment)
		.WithColourAttachment(context.colourFormat, vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha)
		.WithDepthAttachment(context.depthFormat, vk::CompareOp::eLessOrEqual, true, true)

		.WithRasterState(vk::CullModeFlagBits::eBack)
		.WithDynamicState(vk::DynamicState::eFrontFace)

		.Build("Main Scene Pipeline");

	shadowPipeline = PipelineBuilder(context.device)
		.WithVertexInputState(m->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithShaderBinary("Shadow.vert.spv", vk::ShaderStageFlagBits::eVertex)
		.WithShaderBinary("Shadow.frag.spv", vk::ShaderStageFlagBits::eFragment)
		.WithDepthAttachment(shadowMap->GetFormat(), vk::CompareOp::eLessOrEqual, true, true)
		.WithRasterState(vk::CullModeFlagBits::eFront)
		.Build("Shadow Map Pipeline");

	objectTextureDescriptorSet = CreateDescriptorSet(context.device, context.descriptorPool, scenePipeline.GetSetLayout(1));

	LoadTexture("Default.png");

	for (int i = 0; i < _TEXCOUNT; ++i) {
		WriteCombinedImageDescriptor(context.device, *objectTextureDescriptorSet, 0, i, loadedTextures[DEFAULT_TEXTURE]->GetDefaultView(), *defaultSampler);
	}

	{//Data structures used by the debug drawing
		Debug::CreateDebugFont("PressStart2P.fnt", *LoadTexture("PressStart2P.png"));
		BuildDebugPipelines();
		VulkanTexture* tex = (VulkanTexture*)Debug::GetDebugFont()->GetTexture();
		WriteCombinedImageDescriptor(context.device, *objectTextureDescriptorSet, 0, DEBUGTEXT_TEXTURE, loadedTextures[DEBUGTEXT_TEXTURE]->GetDefaultView(), *textSampler);
	}

	//We're going to have 3 frames in-flight at once, with their own per-frame data
	allFrames = new FrameData[_FRAMECOUNT];
	for (int i = 0; i < _FRAMECOUNT; ++i) {
		allFrames[i].fence = context.device.createFenceUnique({});

		{//We store scene object matrices in a big UBO
			allFrames[i].dataBuffer = m_memoryManager->CreateBuffer(
				{
					.size = 1024 * 1024 * 64,
					.usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eVertexBuffer
				},
				vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
				"Data Buffer"
			);

			allFrames[i].data = allFrames[i].dataBuffer.Map<char>();

			allFrames[i].dataStart = allFrames[i].data;

			allFrames[i].dataDescriptorSet = CreateDescriptorSet(context.device, context.descriptorPool, scenePipeline.GetSetLayout(0));

			WriteCombinedImageDescriptor(context.device, *allFrames[i].dataDescriptorSet, 2, 0, shadowMap->GetDefaultView(), *defaultSampler, vk::ImageLayout::eDepthStencilReadOnlyOptimal);
			WriteCombinedImageDescriptor(context.device, *allFrames[i].dataDescriptorSet, 3, 0, cubeTex->GetDefaultView(), *defaultSampler);
		}
	}
	currentFrameIndex = 0;
	currentFrame = &allFrames[currentFrameIndex];
}

void GameTechVulkanRenderer::BuildDebugPipelines() {
	FrameContext const& context = GetFrameContext();

	{
		vk::VertexInputAttributeDescription debugLineVertAttribs[] = {
			//Shader input, binding, format, offset
			{0,0,vk::Format::eR32G32B32A32Sfloat, 0},
			{1,1,vk::Format::eR32G32B32A32Sfloat, 0},
		};

		vk::VertexInputBindingDescription debugLineVertBindings[std::size(debugLineVertAttribs)] = {
			//Binding, stride, input rate
			{0, _LINE_STRIDE, vk::VertexInputRate::eVertex},//Positions
			{1, _LINE_STRIDE, vk::VertexInputRate::eVertex},//Colours
		};

		vk::PipelineVertexInputStateCreateInfo lineVertexState{
			.vertexBindingDescriptionCount	= std::size(debugLineVertBindings),
			.pVertexBindingDescriptions		= debugLineVertBindings,
			.vertexAttributeDescriptionCount	= std::size(debugLineVertAttribs),
			.pVertexAttributeDescriptions		= debugLineVertAttribs
		};

		debugLinePipeline = PipelineBuilder(context.device)
			.WithVertexInputState(lineVertexState)
			.WithTopology(vk::PrimitiveTopology::eLineList)

			.WithShaderBinary("Debug.vert.spv", vk::ShaderStageFlagBits::eVertex)
			.WithShaderBinary("Debug.frag.spv", vk::ShaderStageFlagBits::eFragment)

			.WithColourAttachment(context.colourFormat)
			.WithDepthAttachment(context.depthFormat)
			.Build("Debug Line Pipeline");
	}
	{
		vk::VertexInputAttributeDescription debugTextVertAttribs[] = {
			//Shader input, binding, format, offset
			{0,0,vk::Format::eR32G32Sfloat		, 0},
			{1,1,vk::Format::eR32G32Sfloat		, 0},
			{2,2,vk::Format::eR32G32B32A32Sfloat, 0},
		};

		vk::VertexInputBindingDescription debugTextVertBindings[std::size(debugTextVertAttribs)] = {
			//Binding, stride, input rate
			{0, _TEXT_STRIDE, vk::VertexInputRate::eVertex},//Positions
			{1, _TEXT_STRIDE, vk::VertexInputRate::eVertex},//Tex coords
			{2, _TEXT_STRIDE, vk::VertexInputRate::eVertex},//Colours
		};

		vk::PipelineVertexInputStateCreateInfo textVertexState{
			.vertexBindingDescriptionCount	= std::size(debugTextVertBindings),
			.pVertexBindingDescriptions		= debugTextVertBindings,
			.vertexAttributeDescriptionCount	= std::size(debugTextVertAttribs),
			.pVertexAttributeDescriptions		= debugTextVertAttribs
		};

		debugTextPipeline = PipelineBuilder(context.device)
			.WithVertexInputState(textVertexState)
			.WithTopology(vk::PrimitiveTopology::eTriangleList)

			.WithShaderBinary("DebugText.vert.spv", vk::ShaderStageFlagBits::eVertex)
			.WithShaderBinary("DebugText.frag.spv", vk::ShaderStageFlagBits::eFragment)

			.WithColourAttachment(context.colourFormat)
			.WithDepthAttachment(context.depthFormat)
			.Build("Debug Text Pipeline");
	}

	{
		vk::VertexInputAttributeDescription debugTextureVertAttribs[] = {
			//Shader input, binding, format, offset
			{0,0,vk::Format::eR32G32B32Sfloat	, 0},
			{1,1,vk::Format::eR32G32Sfloat		, 0},

			{2,2,vk::Format::eR32G32Sfloat		, 0},
			{3,3,vk::Format::eR32G32Sfloat		, 0},
			{4,4,vk::Format::eR32G32B32A32Sfloat, 0},
			{5,5,vk::Format::eR32Uint			, 0},
		};

		vk::VertexInputBindingDescription debugTextureVertBindings[std::size(debugTextureVertAttribs)] = {
			//Binding, stride, input rate
			{0, sizeof(Vector3), vk::VertexInputRate::eVertex},//Positions
			{1, sizeof(Vector2), vk::VertexInputRate::eVertex},//Tex coords

			{2, _TEXTURE_INSTANCE_STRIDE, vk::VertexInputRate::eInstance},//Instance Position
			{3, _TEXTURE_INSTANCE_STRIDE, vk::VertexInputRate::eInstance},//Instance Size
			{4, _TEXTURE_INSTANCE_STRIDE, vk::VertexInputRate::eInstance},//Instance Colour
			{5, _TEXTURE_INSTANCE_STRIDE, vk::VertexInputRate::eInstance},//Instance Colour
		};

		vk::PipelineVertexInputStateCreateInfo textureVertexState{
			.vertexBindingDescriptionCount		= std::size(debugTextureVertBindings),
			.pVertexBindingDescriptions			= debugTextureVertBindings,

			.vertexAttributeDescriptionCount	= std::size(debugTextureVertAttribs),
			.pVertexAttributeDescriptions		= debugTextureVertAttribs
		};

		debugTexturePipeline = PipelineBuilder(context.device)
			.WithVertexInputState(textureVertexState)
			.WithTopology(quadMesh->GetVulkanTopology())

			.WithShaderBinary("DebugTexture.vert.spv", vk::ShaderStageFlagBits::eVertex)
			.WithShaderBinary("DebugTexture.frag.spv", vk::ShaderStageFlagBits::eFragment)

			.WithColourAttachment(context.colourFormat)
			.WithDepthAttachment(context.depthFormat)
			.Build("Debug Texture Pipeline");
	}
}

void GameTechVulkanRenderer::RenderFrame() {
	FrameContext const& context = GetFrameContext();

	TransitionUndefinedToColour(context.cmdBuffer, context.colourImage);

	GlobalData frameData;
	frameData.lightColour	= Vector4(gameWorld.GetSunColour(), 1.0f);
	frameData.lightRadius	= 1000.0f;
	frameData.lightPosition = gameWorld.GetSunPosition();

	frameData.cameraPos		= gameWorld.GetMainCamera().GetPosition();

	frameData.viewMatrix	= gameWorld.GetMainCamera().BuildViewMatrix();
	frameData.projMatrix	= gameWorld.GetMainCamera().BuildProjectionMatrix(Window::GetWindow()->GetScreenAspect());
	frameData.orthoMatrix	= Matrix::Orthographic(0.0f, 100.0f, 100.0f, 0.0f, -1.0f, 1.0f);
	frameData.shadowMatrix  =	  Matrix::Perspective(50.0f, 5000.0f, 1, 45.0f) 
								* Matrix::View(frameData.lightPosition, Vector3(0, 0, 0), Vector3(0, 1, 0));

	currentFrame->data				= currentFrame->dataStart;
	currentFrame->bytesWritten		= 0;
	currentFrame->globalDataOffset	= 0;
	currentFrame->objectStateOffset = sizeof(GlobalData);
	currentFrame->debugLinesOffset	= currentFrame->objectStateOffset;

	currentFrame->WriteData<GlobalData>(frameData); //Let's start filling up our frame data!

	currentFrame->AlignDataPtr(128);
	currentFrame->objectStateOffset = currentFrame->bytesWritten;
	
	UpdateObjectList();

	currentFrame->AlignDataPtr(128);

	size_t objectSize = currentFrame->bytesWritten - currentFrame->objectStateOffset;

	WriteBufferDescriptor(context.device, *currentFrame->dataDescriptorSet, 0, vk::DescriptorType::eUniformBuffer, currentFrame->dataBuffer, 0, sizeof(GlobalData));
	WriteBufferDescriptor(context.device, *currentFrame->dataDescriptorSet, 1, vk::DescriptorType::eStorageBuffer, currentFrame->dataBuffer, currentFrame->objectStateOffset, objectSize);

	{//Render the shadow map for the frame
		TransitionSamplerToDepth(context.cmdBuffer, *shadowMap);
		DynamicRenderBuilder()
			.WithDepthAttachment(shadowMap->GetDefaultView())
			.WithRenderArea(vk::Offset2D(0, 0), vk::Extent2D(_SHADOWSIZE, _SHADOWSIZE))
			.BeginRendering(context.cmdBuffer);
	
		RenderSceneObjects(shadowPipeline, context.cmdBuffer); //Draw the shadow map

		context.cmdBuffer.endRendering();
		TransitionDepthToSampler(context.cmdBuffer, *shadowMap);
	}
	{//Now render the main scene view
		DynamicRenderBuilder()
			.WithColourAttachment(context.colourView)
			.WithDepthAttachment(context.depthView)
			.WithRenderArea(context.screenRect)
			.BeginRendering(context.cmdBuffer);

		RenderSkybox(context.cmdBuffer);
	
		RenderOpaquePass(opaqueObjects, context.cmdBuffer);
		RenderTransparentPass(transparentObjects, context.cmdBuffer);

		UpdateDebugData();
		RenderDebugLines(context.cmdBuffer);
		RenderDebugTextures(context.cmdBuffer);
		RenderDebugText(context.cmdBuffer);

		context.cmdBuffer.endRendering();
	}

	currentFrameIndex = (currentFrameIndex + 1) % _FRAMECOUNT;
	currentFrame = &allFrames[currentFrameIndex];
}

void GameTechVulkanRenderer::UpdateObjectList() {
	opaqueObjects.clear();
	transparentObjects.clear();

	Vector3 camPos = gameWorld.GetMainCamera().GetPosition();

	gameWorld.OperateOnContents(
		[&](GameObject* o) {
			if (o->IsActive()) {
				const RenderObject* g = o->GetRenderObject();
				if (g && g->GetMesh()) {
					GameTechMaterial mat = g->GetMaterial();

					ObjectSortState o;
					o.object = g;
					o.distanceFromCamera = Vector::LengthSquared(camPos - g->GetTransform().GetPosition());

					if (mat.type == MaterialType::Opaque) {
						opaqueObjects.emplace_back(o);
					}
					else if (mat.type == MaterialType::Transparent) {
						transparentObjects.emplace_back(o);
					}
				}
			}
		}
	);

	std::sort(opaqueObjects.begin(), opaqueObjects.end(),
		[](ObjectSortState& a, ObjectSortState& b) {
			return a.distanceFromCamera < b.distanceFromCamera;
		}
	);
	std::sort(transparentObjects.rbegin(), transparentObjects.rend(),
		[](ObjectSortState& a, ObjectSortState& b) {
			return a.distanceFromCamera < b.distanceFromCamera;
		}
	);

	auto objectWriter = [&](std::vector<ObjectSortState>& objects) {
		for (auto& o : objects) {
			ObjectState state;
			state.modelMatrix	= o.object->GetTransform().GetMatrix();
			state.colour		= o.object->GetColour();
			state.index[0]		= 0;

			GameTechMaterial mat = o.object->GetMaterial();

			if (mat.diffuseTex) {
				VulkanTexture* t = (VulkanTexture*)mat.diffuseTex;
				state.index[0] = t->GetAssetID();
			}
			currentFrame->WriteData<ObjectState>(state);
			currentFrame->debugLinesOffset += sizeof(ObjectState);
		}
	};

	objectWriter(opaqueObjects);
	objectWriter(transparentObjects);
}

void GameTechVulkanRenderer::RenderOpaquePass(std::vector<ObjectSortState>& list, vk::CommandBuffer cmds) {
	cmds.bindPipeline(vk::PipelineBindPoint::eGraphics, *scenePipeline.pipeline);
	cmds.setFrontFace(vk::FrontFace::eCounterClockwise);

	vk::DescriptorSet sets[] = {
		*currentFrame->dataDescriptorSet,
		*objectTextureDescriptorSet
	};

	cmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *scenePipeline.layout, 0, (uint32_t)std::size(sets), sets, 0, nullptr);

	VulkanMesh* prevMesh = (VulkanMesh*)opaqueObjects[0].object->GetMesh();

	uint32_t	startingIndex = 0;
	uint32_t	instanceCount = 0;

	for (int i = 0; i < list.size(); ++i) {
		VulkanMesh* objectMesh = (VulkanMesh*)list[i].object->GetMesh();

		//The new mesh is different than previous meshes, flush out the old list
		if (prevMesh != objectMesh) {
			cmds.pushConstants(*scenePipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(uint32_t), (void*)&startingIndex);
			prevMesh->Draw(cmds, instanceCount);
			prevMesh = objectMesh;
			instanceCount = 0;
			startingIndex = i;
		}
		if (i == list.size() - 1) {
			cmds.pushConstants(*scenePipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(uint32_t), (void*)&startingIndex);
			if (prevMesh == objectMesh) {
				instanceCount++;
			}
			objectMesh->Draw(cmds, instanceCount);
		}
		else {
			instanceCount++;
		}
	}
}

void GameTechVulkanRenderer::RenderTransparentPass(std::vector<ObjectSortState>& list, vk::CommandBuffer cmds) {
	cmds.bindPipeline(vk::PipelineBindPoint::eGraphics, *scenePipeline.pipeline);
	cmds.setFrontFace(vk::FrontFace::eCounterClockwise);

	vk::DescriptorSet sets[] = {
		*currentFrame->dataDescriptorSet,
		*objectTextureDescriptorSet
	};

	cmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *scenePipeline.layout, 0, (uint32_t)std::size(sets), sets, 0, nullptr);

	VulkanMesh* prevMesh = (VulkanMesh*)opaqueObjects[0].object->GetMesh();

	uint32_t	startingIndex = opaqueObjects.size();
	uint32_t	instanceCount = 0;

	for (int i = 0; i < list.size(); ++i) {
		uint32_t objectIndex = startingIndex + i;

		VulkanMesh* objectMesh = (VulkanMesh*)list[i].object->GetMesh();

		cmds.pushConstants(*scenePipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(uint32_t), (void*)&objectIndex);

		cmds.setFrontFace(vk::FrontFace::eClockwise);
		objectMesh->Draw(cmds, 1);
		cmds.setFrontFace(vk::FrontFace::eCounterClockwise);
		objectMesh->Draw(cmds, 1);
	}
}

void GameTechVulkanRenderer::RenderSceneObjects(VulkanPipeline& pipe, vk::CommandBuffer cmds) {
	if (opaqueObjects.empty() && transparentObjects.empty()) {
		return;
	}

	cmds.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipe.pipeline);

	vk::DescriptorSet sets[2] = {
		*currentFrame->dataDescriptorSet,
		*objectTextureDescriptorSet
	};

	cmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipe.layout, 0, (uint32_t)std::size(sets), sets, 0, nullptr);

	VulkanMesh* prevMesh = (VulkanMesh*)opaqueObjects[0].object->GetMesh();

	uint32_t	startingIndex = 0;
	uint32_t	instanceCount = 0;

	for (int i = 0; i < opaqueObjects.size(); ++i) {
		VulkanMesh* objectMesh = (VulkanMesh*)opaqueObjects[i].object->GetMesh();

		//The new mesh is different than previous meshes, flush out the old list
		if (prevMesh != objectMesh) {
			cmds.pushConstants(*pipe.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(uint32_t), (void*)&startingIndex);
			prevMesh->Draw(cmds, instanceCount);
			prevMesh = objectMesh;
			instanceCount = 0;
			startingIndex = i;
		}
		if (i == opaqueObjects.size() - 1) {
			cmds.pushConstants(*pipe.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(uint32_t), (void*)&startingIndex);
			if (prevMesh == objectMesh) {
				instanceCount++;
			}
			objectMesh->Draw(cmds, instanceCount);
		}
		else {
			instanceCount++;
		}
	}
}

void GameTechVulkanRenderer::RenderSkybox(vk::CommandBuffer cmds) {
	cmds.bindPipeline(vk::PipelineBindPoint::eGraphics, *skyboxPipeline.pipeline);
	cmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *skyboxPipeline.layout, 0, 1, &*currentFrame->dataDescriptorSet, 0, nullptr);
	quadMesh->Draw(cmds);
}

void GameTechVulkanRenderer::UpdateDebugData() {
	const std::vector<Debug::DebugStringEntry>& strings = Debug::GetDebugStrings();
	const std::vector<Debug::DebugLineEntry>&   lines	= Debug::GetDebugLines();
	const std::vector<Debug::DebugTexEntry>&	tex		= Debug::GetDebugTex();

	currentFrame->textVertCount		= 0;
	currentFrame->lineVertCount		= 0;
	currentFrame->textureDrawCount	= tex.size();

	for (const auto& s : strings) {
		currentFrame->textVertCount += Debug::GetDebugFont()->GetVertexCountForString(s.data);
	}
	currentFrame->lineVertCount = (int)lines.size() * 2;

	currentFrame->debugLinesOffset = currentFrame->bytesWritten;

	currentFrame->WriteData((void*)lines.data(), (size_t)currentFrame->lineVertCount * _LINE_STRIDE);

	currentFrame->debugTextOffset = currentFrame->bytesWritten;
	std::vector< NCL::Rendering::SimpleFont::InterleavedTextVertex> verts;

	for (const auto& s : strings) {
		float size = 20.0f;
		Debug::GetDebugFont()->BuildInterleavedVerticesForString(s.data, s.position, s.colour, size, verts);
		//can now copy to GPU visible mem
		size_t count = verts.size() * _TEXT_STRIDE;
		memcpy(currentFrame->data, verts.data(), count);
		currentFrame->data += count;
		currentFrame->bytesWritten += count;
		verts.clear();
	}

	currentFrame->debugTextureOffset = currentFrame->bytesWritten;

	for (const auto& t : tex) {
		currentFrame->WriteData<Vector2>(t.position);
		currentFrame->WriteData<Vector2>(t.scale);
		currentFrame->WriteData<Vector4>(t.colour);

		currentFrame->WriteData<uint32_t>(t.t->GetAssetID());
	}
}

void GameTechVulkanRenderer::RenderDebugLines(vk::CommandBuffer cmds) {
	if (currentFrame->lineVertCount == 0) { return; }
	cmds.bindPipeline(vk::PipelineBindPoint::eGraphics, *debugLinePipeline.pipeline);

	ScopedDebugArea scope(cmds, "Debug Lines");

	vk::DescriptorSet sets[2] = {
		*currentFrame->dataDescriptorSet,
		*objectTextureDescriptorSet
	};
	cmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *debugLinePipeline.layout, 0, (uint32_t)std::size(sets), sets, 0, nullptr);

	vk::Buffer attributeBuffers[] = {
		currentFrame->dataBuffer.buffer,	//Positions
		currentFrame->dataBuffer.buffer		//Colours
	};

	vk::DeviceSize attributeOffsets[std::size(attributeBuffers)] = {
		currentFrame->debugLinesOffset + 0,
		currentFrame->debugLinesOffset + sizeof(Vector4)
	};

	cmds.bindVertexBuffers(0, std::size(attributeBuffers), attributeBuffers, attributeOffsets);
	cmds.draw((uint32_t)currentFrame->lineVertCount, 1, 0, 0);
}

void GameTechVulkanRenderer::RenderDebugText(vk::CommandBuffer cmds) {
	if (currentFrame->textVertCount == 0) { return; }
	cmds.bindPipeline(vk::PipelineBindPoint::eGraphics, *debugTextPipeline.pipeline);

	ScopedDebugArea scope(cmds, "Debug Text");

	vk::DescriptorSet sets[2] = {
		*currentFrame->dataDescriptorSet,
		*objectTextureDescriptorSet
	};
	cmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *debugTextPipeline.layout, 0, (uint32_t)std::size(sets), sets, 0, nullptr);

	int texID = DEBUGTEXT_TEXTURE;
	cmds.pushConstants(*debugTextPipeline.layout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(uint32_t), (void*)&texID);

	vk::Buffer attributeBuffers[] = {
		currentFrame->dataBuffer.buffer,	//Positions
		currentFrame->dataBuffer.buffer,	//Tex Coords
		currentFrame->dataBuffer.buffer		//Colour
	};

	vk::DeviceSize attributeOffsets[std::size(attributeBuffers)] = {
		currentFrame->debugTextOffset + 0,
		currentFrame->debugTextOffset + sizeof(Vector2), //jump over the position
		currentFrame->debugTextOffset + sizeof(Vector2) + sizeof(Vector2) //jump over position and tex coord
	};

	cmds.bindVertexBuffers(0, std::size(attributeBuffers), attributeBuffers, attributeOffsets);	//Interleaved vertex buffer draw
	cmds.draw((uint32_t)currentFrame->textVertCount, 1, 0, 0);
}

void GameTechVulkanRenderer::RenderDebugTextures(vk::CommandBuffer cmds) {
	if (currentFrame->textureDrawCount == 0) { return; }
	cmds.bindPipeline(vk::PipelineBindPoint::eGraphics, *debugTexturePipeline.pipeline);

	ScopedDebugArea scope(cmds, "Debug Textures");

	vk::DescriptorSet sets[2] = {
	*currentFrame->dataDescriptorSet,
	*objectTextureDescriptorSet
	};
	cmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *debugTexturePipeline.layout, 0, (uint32_t)std::size(sets), sets, 0, nullptr);

	vk::Buffer attributeBuffers[] = {
		nullptr,	//Positions
		nullptr,	//Tex Coords

		currentFrame->dataBuffer.buffer,	//Instance Position
		currentFrame->dataBuffer.buffer,	//Instance Scale
		currentFrame->dataBuffer.buffer,		//Instance Colour
		currentFrame->dataBuffer.buffer		//Instance Texture
	};

	vk::DeviceSize attributeOffsets[std::size(attributeBuffers)] = {
		0,
		0,

		//Instance Offsets
		currentFrame->debugTextureOffset,
		currentFrame->debugTextureOffset + sizeof(Vector2), //jump over position
		currentFrame->debugTextureOffset + sizeof(Vector2) + sizeof(Vector2), //jump over position and scale

		currentFrame->debugTextureOffset + sizeof(Vector2) + sizeof(Vector2) + sizeof(Vector4) //jump over position, scale, colour
	};

	vk::Buffer	vBuffer[2];
	uint32_t	vOffset[2];
	uint32_t	vRange[2];
	vk::Format	vFormat[2];
	quadMesh->GetAttributeInformation(NCL::VertexAttribute::Positions	 , vBuffer[0], vOffset[0], vRange[0], vFormat[0]);
	quadMesh->GetAttributeInformation(NCL::VertexAttribute::TextureCoords, vBuffer[1], vOffset[1], vRange[1], vFormat[1]);

	attributeBuffers[0] = vBuffer[0];
	attributeBuffers[1] = vBuffer[1];

	attributeOffsets[0] = vOffset[0];
	attributeOffsets[1] = vOffset[1];

	cmds.bindVertexBuffers(0, std::size(attributeBuffers), attributeBuffers, attributeOffsets);	//Interleaved vertex buffer draw

	vk::Buffer		iBuffer;
	uint32_t		iOffset;
	uint32_t		iRange;
	vk::IndexType	iFormat;
	quadMesh->GetIndexInformation(iBuffer, iOffset, iRange, iFormat);

	cmds.bindIndexBuffer(iBuffer, iOffset, iFormat);

	cmds.drawIndexed((uint32_t)quadMesh->GetIndexCount(), currentFrame->textureDrawCount, 0, 0, 0);
}

Mesh* GameTechVulkanRenderer::LoadMesh(const string& name) {
	VulkanMesh* newMesh = nullptr;

	std::filesystem::path filepath = name;
	if (filepath.extension() == ".gltf") {
		GLTFLoader::Load(name, meshBucket);
		newMesh = (VulkanMesh*)meshBucket.meshes.back().get();
	}
	else if (filepath.extension() == ".msh") {
		newMesh = new VulkanMesh();
		MshLoader::LoadMesh(name, *newMesh);
	}

	newMesh->SetPrimitiveType(NCL::GeometryPrimitive::Triangles);
	newMesh->SetDebugName(name);

	FrameContext const& context = GetFrameContext();
	vk::UniqueCommandBuffer cmdBuffer = CmdBufferCreateBegin(context.device, context.commandPools[CommandType::Graphics], "VulkanMesh upload");

	newMesh->UploadToGPU(*cmdBuffer, m_memoryManager);

	CmdBufferEndSubmitWait(*cmdBuffer, context.device, context.queues[CommandType::Graphics]);

	loadedMeshes.push_back(newMesh);

	if (!scenePipeline.pipeline) {
		BuildScenePipelines(newMesh);
	}

	return newMesh;
}

Texture* GameTechVulkanRenderer::LoadTexture(const string& name) {
	uint32_t id = (uint32_t)loadedTextures.size();

	FrameContext const& context = GetFrameContext();
	vk::UniqueCommandBuffer cmdBuffer = CmdBufferCreateBegin(context.device, context.commandPools[CommandType::Graphics], "VulkanTexture upload");

	loadedTextures.emplace_back(TextureBuilder(context.device, *m_memoryManager)
		.WithCommandBuffer(*cmdBuffer)
		.BuildFromFile(name));
	
	loadedTextures.back()->SetAssetID(id);

	CmdBufferEndSubmitWait(*cmdBuffer, context.device, context.queues[CommandType::Graphics]);

	//Write the texture to our big descriptor set
	WriteCombinedImageDescriptor(GetDevice(), *objectTextureDescriptorSet, 0, id, loadedTextures.back()->GetDefaultView(), *defaultSampler);
	return &*loadedTextures.back();
}

UniqueVulkanMesh GameTechVulkanRenderer::GenerateQuad() {
	VulkanMesh* quadMesh = new VulkanMesh();
	quadMesh->SetVertexPositions({ Vector3(-1,-1,0), Vector3(1,-1,0), Vector3(1,1,0), Vector3(-1,1,0) });
	quadMesh->SetVertexTextureCoords({ Vector2(0,0), Vector2(1,0), Vector2(1, 1), Vector2(0, 1) });
	quadMesh->SetVertexIndices({ 0,1,3,2 });
	quadMesh->SetDebugName("Fullscreen Quad");
	quadMesh->SetPrimitiveType(NCL::GeometryPrimitive::TriangleStrip);

	FrameContext const& context = GetFrameContext();

	vk::UniqueCommandBuffer cmdBuffer = CmdBufferCreateBegin(context.device, context.commandPools[CommandType::Graphics], "VulkanMesh upload");

	quadMesh->UploadToGPU(*cmdBuffer, m_memoryManager);

	CmdBufferEndSubmitWait(*cmdBuffer, context.device, context.queues[CommandType::Graphics]);

	return UniqueVulkanMesh(quadMesh);
}


UniqueVulkanTexture GameTechVulkanRenderer::LoadCubemap(
	const std::string& negativeXFile, const std::string& positiveXFile,
	const std::string& negativeYFile, const std::string& positiveYFile,
	const std::string& negativeZFile, const std::string& positiveZFile,
	const std::string& debugName) {

	FrameContext const& context = GetFrameContext();
	vk::UniqueCommandBuffer cmdBuffer = CmdBufferCreateBegin(context.device, context.commandPools[CommandType::Graphics], "VulkanTexture upload");

	UniqueVulkanTexture tex = TextureBuilder(context.device, *m_memoryManager)
		.WithCommandBuffer(*cmdBuffer)
		.BuildCubemapFromFile(negativeXFile, positiveXFile,
			negativeYFile, positiveYFile,
			negativeZFile, positiveZFile,
			debugName
		);

	CmdBufferEndSubmitWait(*cmdBuffer, context.device, context.queues[CommandType::Graphics]);
	return tex;
}
#endif