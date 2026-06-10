#pragma once
#include "Window.h"
#include "GameWorldDOD.h"
#include "OGLShader.h"
#include "OGLMesh.h"
#include "OGLTexture.h"
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif


namespace NCL {
	namespace Rendering {
		class OGLMesh;
		class OGLShader;
		class OGLTexture;
	}
	namespace CSC8503 {
		struct GameTechRendererData {
			std::vector<size_t> opaqueObjectIndices;
			std::vector<size_t> transparentObjectIndices;

			Matrix4 viewMatrix;
			Matrix4 projMatrix;
			Vector3 cameraPos;
		};

		struct GameTechRendererResources {
			Rendering::OGLMesh* skyboxMesh;
			Rendering::OGLMesh* debugTexMesh;

			Rendering::OGLShader* defaultShader;
			Rendering::OGLShader* skyboxShader;
			Rendering::OGLShader* shadowShader;
			Rendering::OGLShader* debugShader;

			std::vector<Vector4> debugTextColours;
			std::vector<Vector3> debugLineData;
			std::vector<Vector3> debugTextPos;
			std::vector<Vector2> debugTextUVs;

			Matrix4 shadowMatrix;

			size_t lineCount;
			size_t textCount;

			GLuint skyboxTex;
			GLuint shadowTex;
			GLuint shadowFBO;
			GLuint lineVAO;
			GLuint lineVertVBO;
			GLuint textVAO;
			GLuint textVertVBO;
			GLuint textColourVBO;
			GLuint textTexVBO;

			int screenWidth;
			int screenHeight;
		};

		struct RendererSystemDOD {
			GameTechRendererResources resources;
			Window* window;
#ifdef _WIN32
			HDC deviceContext = nullptr;
#else
			void* deviceContext = nullptr;
#endif

			void Initialise(Window* windowPtr);
			void Destroy();
			void swapBuffers();
			void SetVerticalSync(int interval);

			Mesh* LoadMesh(const std::string& name);
			Texture* LoadTexture(const std::string& name);

			void BuildRenderFrame(GameWorldDOD& world, GameTechRendererData& frameData);
			void RenderSkyboxPass(GameTechRendererData& frameData);
			void RenderOpaquePass(GameWorldDOD& world, GameTechRendererData& frameData);
			void RenderTransparentPass(GameWorldDOD& world, GameTechRendererData& frameData);
			void RenderShadowMapPass(GameWorldDOD& world, GameTechRendererData& frameData);
			void RenderFrame(GameWorldDOD& world, GameTechRendererData& frameData);
		};
	}
}