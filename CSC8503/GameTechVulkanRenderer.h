#pragma once
#ifdef USEVULKAN
#include "VulkanRenderer.h"
#include "GameTechRendererInterface.h"
#include "VulkanMesh.h"
#include "SmartTypes.h"

#include "../GLTFLoader/GLTFLoader.h"

namespace NCL {
	class TextureBase;
	class ShaderBase;

	namespace Rendering::Vulkan {
		class VulkanMemoryManager;
	}

	namespace CSC8503 {
		class RenderObject;
		class GameWorld;

		class GameTechVulkanRenderer :
			public Vulkan::VulkanRenderer,
			public NCL::CSC8503::GameTechRendererInterface
		{
		public:
			GameTechVulkanRenderer(GameWorld& world);
			~GameTechVulkanRenderer();

			void	InitStructures();

			Mesh*	 LoadMesh(const string& name);
			Texture* LoadTexture(const string& name);

		protected:
			struct GlobalData {
				Matrix4 shadowMatrix;
				Matrix4 viewProjMatrix;
				Matrix4 viewMatrix;
				Matrix4 projMatrix;
				Matrix4 orthoMatrix;
				Vector3	lightPosition;
				float	lightRadius;
				Vector4	lightColour;
				Vector3	cameraPos;
			};

			struct ObjectState {
				Matrix4 modelMatrix;
				Vector4 colour;
				int		index[4];
			};

			struct ObjectData {
				Vulkan::VulkanTexture* cachedTex;
				int index;
			};

			struct ObjectSortState {
				const RenderObject* object;
				float distanceFromCamera;
			};

			struct FrameData {
				vk::UniqueFence fence;

				Vulkan::VulkanBuffer	dataBuffer;
				char* dataStart;					//Start of our buffer
				char* data;							//We're bumping this along as we write data

				size_t globalDataOffset		= 0;	//Where does the global data start in the buffer?
				size_t objectStateOffset	= 0;	//Where does the object states start?
				size_t debugLinesOffset		= 0;	//Where do the debug lines start?
				size_t debugTextOffset		= 0;	//Where do the debug text verts start?
				size_t debugTextureOffset	= 0;	

				size_t bytesWritten			= 0;

				size_t lineVertCount		= 0;
				size_t textVertCount		= 0;
				size_t textureDrawCount		= 0;

				vk::UniqueDescriptorSet dataDescriptorSet;

				template<typename T>
				void WriteData(T value) {
					memcpy(data, &value, sizeof(T));
					data += sizeof(T);
					bytesWritten += sizeof(T);
				}
				void WriteData(void* inData, size_t byteCount) {
					memcpy(data, inData, byteCount);
					data += byteCount;
					bytesWritten += byteCount;
				}

				void AlignDataPtr(size_t alignment) {
					char* oldData = data;
					data = (char*)((((uintptr_t)data + alignment - 1) / (uintptr_t)alignment) * (uintptr_t)alignment);
					bytesWritten += data - oldData;
				}
			};

			FrameData* allFrames;
			FrameData* currentFrame;

			void RenderFrame()	override;

			void BuildScenePipelines(Vulkan::VulkanMesh* m);
			void BuildDebugPipelines();

			void UpdateObjectList();
			void UpdateDebugData();

			void RenderSceneObjects(VulkanPipeline& pipe, vk::CommandBuffer cmds);

			void RenderOpaquePass(std::vector<ObjectSortState>& list, vk::CommandBuffer cmds);
			void RenderTransparentPass(std::vector<ObjectSortState>& list, vk::CommandBuffer cmds);


			void RenderSkybox(vk::CommandBuffer cmds);
			void RenderDebugLines(vk::CommandBuffer cmds);
			void RenderDebugText(vk::CommandBuffer cmds);
			void RenderDebugTextures(vk::CommandBuffer cmds);

			Vulkan::UniqueVulkanMesh GenerateQuad();

			Vulkan::UniqueVulkanTexture LoadCubemap(
				const std::string& negativeXFile, const std::string& positiveXFile,
				const std::string& negativeYFile, const std::string& positiveYFile,
				const std::string& negativeZFile, const std::string& positiveZFile,
				const std::string& debugName = "CubeMap");

			GameWorld& gameWorld;
			std::vector<const RenderObject*> activeObjects;

			std::vector<ObjectSortState> opaqueObjects;
			std::vector<ObjectSortState> transparentObjects;


			int currentFrameIndex;

			Vulkan::VulkanMemoryManager* m_memoryManager;

			VulkanPipeline	skyboxPipeline;
			VulkanPipeline	shadowPipeline;
			VulkanPipeline	scenePipeline;
			VulkanPipeline	debugLinePipeline;
			VulkanPipeline	debugTextPipeline;
			VulkanPipeline	debugTexturePipeline;

			Vulkan::UniqueVulkanMesh	quadMesh;
			Vulkan::UniqueVulkanTexture	cubeTex;

			Vulkan::UniqueVulkanTexture shadowMap;

			vk::UniqueDescriptorSet		objectTextureDescriptorSet;

			vk::UniqueSampler	defaultSampler;
			vk::UniqueSampler	textSampler;

			std::vector<Vulkan::UniqueVulkanTexture>	loadedTextures;
			std::vector<Mesh*>							loadedMeshes;

			//GLTF Interface
			GLTFScene meshBucket;
		};
	}
}
#endif

