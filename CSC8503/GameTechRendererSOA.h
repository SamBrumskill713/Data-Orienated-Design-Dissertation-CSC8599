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

		struct ShaderUniformCache {
			// Default shader uniforms
			GLint defaultShader_proj = -1;
			GLint defaultShader_view = -1;
			GLint defaultShader_model = -1;
			GLint defaultShader_colour = -1;
			GLint defaultShader_hasVertexColours = -1;
			GLint defaultShader_hasTexture = -1;
			GLint defaultShader_sunPos = -1;
			GLint defaultShader_sunColour = -1;
			GLint defaultShader_sunRadius = -1;
			GLint defaultShader_cameraPos = -1;
			GLint defaultShader_shadowTex = -1;
			GLint defaultShader_shadowMatrix = -1;
			GLint defaultShader_mainTex = -1;

			// Skybox shader uniforms
			GLint skyboxShader_proj = -1;
			GLint skyboxShader_view = -1;
			GLint skyboxShader_cubeTex = -1;

			// Shadow shader uniforms
			GLint shadowShader_mvp = -1;
		};

		struct GameTechRendererDataSOA {
			std::vector<size_t> opaqueObjectIndices;
			std::vector<size_t> transparentObjectIndices;
			std::vector<OGLMesh*> cachedMeshPtrs;
			bool meshCacheDirty = true;

			std::unordered_map<size_t, std::vector<size_t>> textureToObjectIndices;
			bool textureBatchDirty = true;

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

			ShaderUniformCache uniformCache;
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

			void CacheUniformLocations();
			void UpdateMeshCache(GameWorldSOA& world, GameTechRendererDataSOA& frameData);
			void BuildTextureBatches(GameWorldSOA& world, GameTechRendererDataSOA& frameData);
			void BuildRenderFrame(GameWorldSOA& world, GameTechRendererDataSOA& frameData);
			void RenderSkyBoxPass(GameTechRendererDataSOA& frameData);
			void RenderOpaquePass(GameWorldSOA& world, GameTechRendererDataSOA& frameData);
			void RenderTransparenetPass(GameWorldSOA& world, GameTechRendererDataSOA& frameData);
			void RenderShadowMapPass(GameWorldSOA& world, GameTechRendererDataSOA& frameData);
			void RenderFrame(GameWorldSOA& world, GameTechRendererDataSOA& frameData);
		};
	}
}