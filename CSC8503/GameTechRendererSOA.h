#pragma once

#include "Window.h"
#include "GameWorldSOA.h"
#include "OGLShader.h"
#include "OGLMesh.h"
#include "OGLTexture.h"
#include "GameTechRendererDOD.h"
#include "TextureLoader.h"
#include "MshLoader.h"
#include "Debug.h"
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include "Win32Window.h"
#include "glad/wgl.h"
#endif

namespace NCL {
	namespace CSC8503{
		struct GameTechRendererDataSOA {
			std::vector<size_t> opaqueObjectIndices;
			std::vector<size_t> transparentObjectIndices;

			Matrix4 viewMatrix;
			Matrix4 projMatrix;
			Vector3 cameraPos;
		};

		struct GameTechRendererResourcesSOA {
			Rendering::OGLMesh* skyboxMesh;
			Rendering::OGLMesh* debugTextMesh;

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
			GLuint lineColourVBO;
			GLuint textTexVBO;

			int screenWidth;
			int screenHeight;
		};

		struct RendererSystemSOA {
			GameTechRendererResourcesSOA SOAResources;
			Window* window;
#ifdef _WIN32
			HDC deviceContext = nullptr;
#else
			void* deviceContext = nullptr;
#endif
			void Initialise(Window* windowPtr);
			void Destroy();
			void SwapBuffers();
			void SetVerticalSync(int interval);

			Mesh* LoadMesh(const std::string& name);
			Texture* LoadTexture(const std::string& name);

			void BuildRenderFrame(GameWorldSOA& world, GameTechRendererDataSOA& frameData);
			void RenderSkyBoxPass(GameTechRendererDataSOA& frameData);
			void RenderOpaquePass(GameWorldSOA& world, GameTechRendererDataSOA& frameData);
			void RenderTransparenetPass(GameWorldSOA& world, GameTechRendererDataSOA& frameData);
			void RenderShadowMapPass(GameWorldSOA& world, GameTechRendererDataSOA& frameData);
			void RenderFrame(GameWorldSOA& world, GameTechRendererDataSOA& frameData);
		};
	}
}