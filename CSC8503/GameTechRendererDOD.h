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
			Rendering::OGLMesh* skyboxMesh = nullptr;
			Rendering::OGLMesh* debugTexMesh = nullptr;

			Rendering::OGLShader* defaultShader = nullptr;
			Rendering::OGLShader* skyboxShader = nullptr;
			Rendering::OGLShader* shadowShader = nullptr;
			Rendering::OGLShader* debugShader = nullptr;

			std::vector<Vector4> debugTextColours;
			std::vector<Vector3> debugLineData;
			std::vector<Vector3> debugTextPos;
			std::vector<Vector2> debugTextUVs;

			Matrix4 shadowMatrix;

			size_t lineCount = 0;
			size_t textCount = 0;

			GLuint skyboxTex = 0;
			GLuint shadowTex = 0;
			GLuint shadowFBO = 0;
			GLuint lineVAO = 0;
			GLuint lineVertVBO = 0;
			GLuint textVAO = 0;
			GLuint textVertVBO = 0;
			GLuint textColourVBO = 0;
			GLuint textTexVBO = 0;

			int screenWidth = 0;
			int screenHeight = 0;
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

			void SetDebugStringBufferSizes(size_t newVertCount);
			void RenderText();

			void BuildRenderFrame(GameWorldDOD& world, GameTechRendererData& frameData);
			void RenderSkyboxPass(GameTechRendererData& frameData);
			void RenderOpaquePass(GameWorldDOD& world, GameTechRendererData& frameData);
			void RenderTransparentPass(GameWorldDOD& world, GameTechRendererData& frameData);
			void RenderShadowMapPass(GameWorldDOD& world, GameTechRendererData& frameData);
			void RenderFrame(GameWorldDOD& world, GameTechRendererData& frameData);
		};
	}
}