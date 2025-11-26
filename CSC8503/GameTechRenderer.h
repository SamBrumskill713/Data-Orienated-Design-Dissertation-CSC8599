#pragma once
#include "OGLRenderer.h"
#include "GameTechRendererInterface.h"

#include "OGLShader.h"

namespace NCL {
	namespace Rendering {
		class OGLMesh;
		class OGLShader;
		class OGLTexture;
		class OGLBuffer;
	};
	namespace CSC8503 {
		class RenderObject;
		class GameWorld;

		class GameTechRenderer : 
			public OGLRenderer,
			public NCL::CSC8503::GameTechRendererInterface	
		{
		public:
			GameTechRenderer(GameWorld& world);
			~GameTechRenderer();

			Mesh*		LoadMesh(const std::string& name)									override;
			Texture*	LoadTexture(const std::string& name)								override;
	
		protected:
			struct ObjectSortState {
				const RenderObject* object;
				float distanceFromCamera;
			};

			void RenderLines();
			void RenderText();
			void RenderTextures();

			void RenderFrame()	override;

			void BuildObjectLists();

			void RenderSkyboxPass();
			void RenderOpaquePass(std::vector<ObjectSortState>& list);
			void RenderTransparentPass(std::vector<ObjectSortState>& list);
			void RenderShadowMapPass(std::vector<ObjectSortState>& list);

			void LoadSkybox();

			void SetDebugStringBufferSizes(size_t newVertCount);
			void SetDebugLineBufferSizes(size_t newVertCount);

			std::vector<ObjectSortState> opaqueObjects;
			std::vector<ObjectSortState> transparentObjects;

			GameWorld&	gameWorld;

			OGLShader*	defaultShader;

			//Skybox pass data
			OGLShader*  skyboxShader;
			OGLMesh*	skyboxMesh;
			GLuint		skyboxTex;

			//shadow map pass data
			OGLShader*	shadowShader;
			GLuint		shadowTex;
			GLuint		shadowFBO;
			Matrix4     shadowMatrix;

			//Debug data
			OGLShader*  debugShader;
			OGLMesh*	debugTexMesh;

			std::vector<Vector3> debugLineData;

			std::vector<Vector3> debugTextPos;
			std::vector<Vector4> debugTextColours;
			std::vector<Vector2> debugTextUVs;

			GLuint lineVAO;
			GLuint lineVertVBO;
			size_t lineCount;

			GLuint textVAO;
			GLuint textVertVBO;
			GLuint textColourVBO;
			GLuint textTexVBO;
			size_t textCount;
		};
	}
}

