#include "GameTechRendererSOA.h"

using namespace NCL;
using namespace Rendering;
using namespace CSC8503;

#define SHADOWSIZE 4096

static Matrix4 biasMatrix = Matrix::Translation(Vector3(0.5f, 0.5f, 0.5f)) * Matrix::Scale(Vector3(0.5f, 0.5f, 0.5f));
static Matrix4 shadowMatrix;

void RendererSystemSOA::CacheUniformLocations() {
	// Cache default shader uniforms
	SOAResources.uniformCache.defaultShader_proj = glGetUniformLocation(SOAResources.defaultShader->GetProgramID(), "projMatrix");
	SOAResources.uniformCache.defaultShader_view = glGetUniformLocation(SOAResources.defaultShader->GetProgramID(), "viewMatrix");
	SOAResources.uniformCache.defaultShader_model = glGetUniformLocation(SOAResources.defaultShader->GetProgramID(), "modelMatrix");
	SOAResources.uniformCache.defaultShader_colour = glGetUniformLocation(SOAResources.defaultShader->GetProgramID(), "objectColour");
	SOAResources.uniformCache.defaultShader_hasVertexColours = glGetUniformLocation(SOAResources.defaultShader->GetProgramID(), "hasVertexColours");
	SOAResources.uniformCache.defaultShader_hasTexture = glGetUniformLocation(SOAResources.defaultShader->GetProgramID(), "hasTexture");
	SOAResources.uniformCache.defaultShader_sunPos = glGetUniformLocation(SOAResources.defaultShader->GetProgramID(), "sunPos");
	SOAResources.uniformCache.defaultShader_sunColour = glGetUniformLocation(SOAResources.defaultShader->GetProgramID(), "sunColour");
	SOAResources.uniformCache.defaultShader_sunRadius = glGetUniformLocation(SOAResources.defaultShader->GetProgramID(), "sunRadius");
	SOAResources.uniformCache.defaultShader_cameraPos = glGetUniformLocation(SOAResources.defaultShader->GetProgramID(), "cameraPos");
	SOAResources.uniformCache.defaultShader_shadowTex = glGetUniformLocation(SOAResources.defaultShader->GetProgramID(), "shadowTex");
	SOAResources.uniformCache.defaultShader_shadowMatrix = glGetUniformLocation(SOAResources.defaultShader->GetProgramID(), "shadowMatrix");
	SOAResources.uniformCache.defaultShader_mainTex = glGetUniformLocation(SOAResources.defaultShader->GetProgramID(), "mainTex");

	// Cache skybox shader uniforms
	SOAResources.uniformCache.skyboxShader_proj = glGetUniformLocation(SOAResources.skyboxShader->GetProgramID(), "projMatrix");
	SOAResources.uniformCache.skyboxShader_view = glGetUniformLocation(SOAResources.skyboxShader->GetProgramID(), "viewMatrix");
	SOAResources.uniformCache.skyboxShader_cubeTex = glGetUniformLocation(SOAResources.skyboxShader->GetProgramID(), "cubeTex");

	// Cache shadow shader uniforms
	SOAResources.uniformCache.shadowShader_mvp = glGetUniformLocation(SOAResources.shadowShader->GetProgramID(), "mvpMatrix");
}

void RendererSystemSOA::Initialise(Window* winPtr) {
	window = winPtr;
	SOAResources.screenWidth = window->GetScreenSize().x;
	SOAResources.screenHeight = window->GetScreenSize().y;

#ifdef _WIN32
	NCL::Win32Code::Win32Window* realWindow = (NCL::Win32Code::Win32Window*)winPtr;
	deviceContext = GetDC(realWindow->GetHandle());
#endif

	glEnable(GL_DEPTH_TEST);

	//Loading Shader
	SOAResources.debugShader = new OGLShader("debug.vert", "debug.frag");
	SOAResources.shadowShader = new OGLShader("shadow.vert", "shadow.frag");
	SOAResources.defaultShader = new OGLShader("scene.vert", "scene.frag");

	//shadow Texture
	glGenTextures(1, &SOAResources.shadowTex);
	glBindTexture(GL_TEXTURE_2D, SOAResources.shadowTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOWSIZE, SHADOWSIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
	glBindTexture(GL_TEXTURE_2D, 0);

	//shadowFBOs
	glGenFramebuffers(1, &SOAResources.shadowFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, SOAResources.shadowFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, SOAResources.shadowTex, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "Shadow framebuffer is not complete!" << std::endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glClearColor(1, 1, 1, 1);

	//skybox mesh and shaders
	SOAResources.skyboxShader = new OGLShader("skybox.vert", "skybox.frag");
	SOAResources.skyboxMesh = new OGLMesh();
	SOAResources.skyboxMesh->SetVertexPositions({ Vector3(-1, 1, -1), Vector3(-1, -1, -1), Vector3(1, -1, -1), Vector3(1, 1, -1) });
	SOAResources.skyboxMesh->SetVertexIndices({ 0, 1, 2, 2, 3, 0 });
	SOAResources.skyboxMesh->UploadToGPU();

	//skybox textures
	std::string fileNames[6] = {
		"/Cubemap/skyrender0004.png",
		"/Cubemap/skyrender0001.png",
		"/Cubemap/skyrender0003.png",
		"/Cubemap/skyrender0006.png",
		"/Cubemap/skyrender0002.png",
		"/Cubemap/skyrender0005.png"
	};

	uint32_t width[6] = { 0 };
	uint32_t height[6] = { 0 };
	uint32_t channels[6] = { 0 };
	uint32_t flags[6] = { 0 };

	std::vector<char*> texData(6, nullptr);

	//loads 6 faces
	for (int i = 0; i < 6; ++i) {
		TextureLoader::LoadTexture(fileNames[i], texData[i], width[i], height[i], channels[i], flags[i]);
		if (i > 0 && (width[i] != width[0] || height[i] != height[0])) {
			std::cout << "Cubemap texture size mismatch!" << std::endl;
			return;
		}
	}

	glGenTextures(1, &SOAResources.skyboxTex);
	glBindTexture(GL_TEXTURE_CUBE_MAP, SOAResources.skyboxTex);

	GLenum type = channels[0] == 4 ? GL_RGBA : GL_RGB;

	//upload 6 faces
	for (int i = 0; i < 6; ++i) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width[i], height[i], 0, type, GL_UNSIGNED_BYTE, texData[i]);
	}

	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	glGenVertexArrays(1, &SOAResources.textVAO);
	glGenBuffers(1, &SOAResources.textVertVBO);
	glGenBuffers(1, &SOAResources.textColourVBO);
	glGenBuffers(1, &SOAResources.textTexVBO);
	SetDebugStringBufferSizes(10000);

	Debug::CreateDebugFont("PressStart2P.fnt", *LoadTexture("PressStart2P.png"));

	CacheUniformLocations();
}

Mesh* RendererSystemSOA::LoadMesh(const std::string& name) {
	OGLMesh* mesh = new OGLMesh();
	MshLoader::LoadMesh(name, *mesh);
	mesh->SetPrimitiveType(GeometryPrimitive::Triangles);
	mesh->UploadToGPU();
	return mesh;
}

Texture* RendererSystemSOA::LoadTexture(const std::string& name) {
	return OGLTexture::TextureFromFile(name).release();
}

void NCL::CSC8503::RendererSystemSOA::UpdateMeshCache(GameWorldSOA& world, GameTechRendererDataSOA& frameData)
{
	auto& objects = world.gameObjects;

	if (frameData.meshCacheDirty || frameData.cachedMeshPtrs.size() != objects.render.meshes.size()) {
		frameData.cachedMeshPtrs.resize(objects.render.meshes.size());
		for (size_t i = 0; i < objects.render.meshes.size(); ++i) {
			frameData.cachedMeshPtrs[i] = (OGLMesh*)objects.render.meshes[i];
		}
		frameData.meshCacheDirty = false;
	}
}

void NCL::CSC8503::RendererSystemSOA::BuildTextureBatches(GameWorldSOA& world, GameTechRendererDataSOA& frameData)
{
	if (!frameData.textureBatchDirty) return;

	frameData.textureToObjectIndices.clear();
	auto& objects = world.gameObjects;

	// Group objects by texture
	for (size_t idx : frameData.opaqueObjectIndices) {
		OGLTexture* diffuseTex = (OGLTexture*)objects.render.diffuseTextures[idx];
		size_t texKey = reinterpret_cast<size_t>(diffuseTex);
		frameData.textureToObjectIndices[texKey].push_back(idx);
	}

	// Sort each texture group by mesh pointer to maximize mesh batching
	for (auto& [texKey, indices] : frameData.textureToObjectIndices) {
		std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
			return frameData.cachedMeshPtrs[a] < frameData.cachedMeshPtrs[b];
			});
	}

	frameData.textureBatchDirty = false;
}

void RendererSystemSOA::Destroy() {
	if (SOAResources.defaultShader) {
		delete SOAResources.defaultShader;
		SOAResources.defaultShader = nullptr;
	}
	if (SOAResources.shadowShader) {
		delete SOAResources.shadowShader;
		SOAResources.shadowShader = nullptr;
	}
	if (SOAResources.skyboxShader) {
		delete SOAResources.skyboxShader;
		SOAResources.skyboxShader = nullptr;
	}
	if (SOAResources.debugShader) {
		delete SOAResources.debugShader;
		SOAResources.debugShader = nullptr;
	}

	if (SOAResources.skyboxMesh) {
		delete SOAResources.skyboxMesh;
		SOAResources.skyboxMesh = nullptr;
	}

	if (SOAResources.shadowTex != 0) {
		glDeleteTextures(1, &SOAResources.shadowTex);
		SOAResources.shadowTex = 0;
	}
	if (SOAResources.skyboxTex != 0) {
		glDeleteTextures(1, &SOAResources.skyboxTex);
		SOAResources.skyboxTex = 0;
	}

	if (SOAResources.shadowFBO != 0) {
		glDeleteFramebuffers(1, &SOAResources.shadowFBO);
		SOAResources.shadowFBO = 0;
	}

	if (SOAResources.textTexVBO != 0) { glDeleteBuffers(1, &SOAResources.textTexVBO); SOAResources.textTexVBO = 0; }
	if (SOAResources.textColourVBO != 0) { glDeleteBuffers(1, &SOAResources.textColourVBO); SOAResources.textColourVBO = 0; }
	if (SOAResources.textVertVBO != 0) { glDeleteBuffers(1, &SOAResources.textVertVBO); SOAResources.textVertVBO = 0; }
	if (SOAResources.textVAO != 0) { glDeleteVertexArrays(1, &SOAResources.textVAO); SOAResources.textVAO = 0; }

	window = nullptr;
}

void RendererSystemSOA::SwapBuffers() {
#ifdef _WIN32
	if (deviceContext) {
		::SwapBuffers(deviceContext);
	}
#endif
}

void RendererSystemSOA::SetVerticalSync(int interval) {
	static auto wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
	if (wglSwapIntervalEXT) {
		wglSwapIntervalEXT(interval);
	}
}

void NCL::CSC8503::RendererSystemSOA::BuildRenderFrame(GameWorldSOA& world, GameTechRendererDataSOA& frameData) {
	frameData.opaqueObjectIndices.clear();
	frameData.transparentObjectIndices.clear();

	Vector3 camPos = frameData.cameraPos;
	int count = world.GetObjectCount();
	auto& objects = world.gameObjects;

	std::vector<float> objectDistances(count, 0.0f);

	for (int i = 0; i < count; ++i) {
		if (!objects.isActive[i]) {
			continue;
		}

		objectDistances[i] = Vector::LengthSquared(camPos - objects.transforms.positions[i]);

		if (objects.render.materialTypes[i] == MaterialType::Transparent) {
			frameData.transparentObjectIndices.push_back(i);
		}
		else {
			frameData.opaqueObjectIndices.push_back(i);
		}
	}

	std::sort(frameData.opaqueObjectIndices.begin(), frameData.opaqueObjectIndices.end(),
		[&](size_t a, size_t b) {
			return objectDistances[a] < objectDistances[b];
		});

	std::sort(frameData.transparentObjectIndices.begin(), frameData.transparentObjectIndices.end(),
		[&](size_t a, size_t b) {
			return objectDistances[a] > objectDistances[b];
		});

	frameData.textureBatchDirty = false;
}

void NCL::CSC8503::RendererSystemSOA::RenderSkyBoxPass(GameTechRendererDataSOA& frameData) {
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	glUseProgram(SOAResources.skyboxShader->GetProgramID());

	glUniformMatrix4fv(SOAResources.uniformCache.skyboxShader_proj, 1, false, (float*)&frameData.projMatrix);
	glUniformMatrix4fv(SOAResources.uniformCache.skyboxShader_view, 1, false, (float*)&frameData.viewMatrix);

	glUniform1i(SOAResources.uniformCache.skyboxShader_cubeTex, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, SOAResources.skyboxTex);

	glBindVertexArray(SOAResources.skyboxMesh->GetVAO());
	glDrawElements(GL_TRIANGLES, SOAResources.skyboxMesh->GetIndexCount(), GL_UNSIGNED_INT, 0);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
}

void NCL::CSC8503::RendererSystemSOA::RenderOpaquePass(GameWorldSOA& world, GameTechRendererDataSOA& frameData) {
	glDisable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);

	glUseProgram(SOAResources.defaultShader->GetProgramID());

	glUniformMatrix4fv(SOAResources.uniformCache.defaultShader_proj, 1, false, (float*)&frameData.projMatrix);
	glUniformMatrix4fv(SOAResources.uniformCache.defaultShader_view, 1, false, (float*)&frameData.viewMatrix);
	glUniform3fv(SOAResources.uniformCache.defaultShader_cameraPos, 1, &frameData.cameraPos.x);

	Vector3 sunPos = world.GetSunPosition();
	Vector3 sunCol = world.GetSunColour();
	glUniform3fv(SOAResources.uniformCache.defaultShader_sunPos, 1, (float*)&sunPos);
	glUniform3fv(SOAResources.uniformCache.defaultShader_sunColour, 1, (float*)&sunCol);
	glUniform1f(SOAResources.uniformCache.defaultShader_sunRadius, 10000.0f);

	glActiveTexture(GL_TEXTURE0 + 1);
	glBindTexture(GL_TEXTURE_2D, SOAResources.shadowTex);
	glUniform1i(SOAResources.uniformCache.defaultShader_shadowTex, 1);

	auto& objects = world.gameObjects;

	for (size_t idx : frameData.opaqueObjectIndices) {
		OGLTexture* diffuseTex = (OGLTexture*)objects.render.diffuseTextures[idx];
		OGLMesh* mesh = (OGLMesh*)objects.render.meshes[idx];

		if (diffuseTex) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, diffuseTex->GetObjectID());
			glUniform1i(SOAResources.uniformCache.defaultShader_mainTex, 0);
		}

		const Matrix4& modelMat = objects.transforms.matrices[idx];
		glUniformMatrix4fv(SOAResources.uniformCache.defaultShader_model, 1, false, (float*)&modelMat);

		Matrix4 fullShadowMat = SOAResources.shadowMatrix * objects.transforms.matrices[idx];
		glUniformMatrix4fv(SOAResources.uniformCache.defaultShader_shadowMatrix, 1, false, (float*)&fullShadowMat);

		glUniform4fv(SOAResources.uniformCache.defaultShader_colour, 1, (float*)&objects.render.colours[idx]);
		glUniform1i(SOAResources.uniformCache.defaultShader_hasVertexColours, 0);
		glUniform1i(SOAResources.uniformCache.defaultShader_hasTexture, diffuseTex ? 1 : 0);

		glBindVertexArray(mesh->GetVAO());
		glDrawElements(GL_TRIANGLES, mesh->GetIndexCount(), GL_UNSIGNED_INT, 0);
	}
}

void NCL::CSC8503::RendererSystemSOA::RenderTransparenetPass(GameWorldSOA& world, GameTechRendererDataSOA& frameData) {
	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);

	glUseProgram(SOAResources.defaultShader->GetProgramID());

	glUniformMatrix4fv(SOAResources.uniformCache.defaultShader_proj, 1, false, (float*)&frameData.projMatrix);
	glUniformMatrix4fv(SOAResources.uniformCache.defaultShader_view, 1, false, (float*)&frameData.viewMatrix);
	glUniform3fv(SOAResources.uniformCache.defaultShader_cameraPos, 1, &frameData.cameraPos.x);

	Vector3 sunPos = world.GetSunPosition();
	Vector3 sunCol = world.GetSunColour();
	glUniform3fv(SOAResources.uniformCache.defaultShader_sunPos, 1, (float*)&sunPos);
	glUniform3fv(SOAResources.uniformCache.defaultShader_sunColour, 1, (float*)&sunCol);
	glUniform1f(SOAResources.uniformCache.defaultShader_sunRadius, 10000.0f);

	glActiveTexture(GL_TEXTURE0 + 1);
	glBindTexture(GL_TEXTURE_2D, SOAResources.shadowTex);
	glUniform1i(SOAResources.uniformCache.defaultShader_shadowTex, 1);

	auto& objects = world.gameObjects;

	for (size_t idx : frameData.transparentObjectIndices) {
		OGLTexture* diffuseTex = (OGLTexture*)objects.render.diffuseTextures[idx];
		OGLMesh* mesh = (OGLMesh*)objects.render.meshes[idx];

		if (diffuseTex) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, diffuseTex->GetObjectID());
			glUniform1i(SOAResources.uniformCache.defaultShader_mainTex, 0);
		}

		const Matrix4& modelMat = objects.transforms.matrices[idx];
		glUniformMatrix4fv(SOAResources.uniformCache.defaultShader_model, 1, false, (float*)&modelMat);

		Matrix4 fullShadowMat = SOAResources.shadowMatrix * objects.transforms.matrices[idx];
		glUniformMatrix4fv(SOAResources.uniformCache.defaultShader_shadowMatrix, 1, false, (float*)&fullShadowMat);

		glUniform4fv(SOAResources.uniformCache.defaultShader_colour, 1, (float*)&objects.render.colours[idx]);
		glUniform1i(SOAResources.uniformCache.defaultShader_hasVertexColours, 0);
		glUniform1i(SOAResources.uniformCache.defaultShader_hasTexture, diffuseTex ? 1 : 0);

		glBindVertexArray(mesh->GetVAO());
		glDrawElements(GL_TRIANGLES, mesh->GetIndexCount(), GL_UNSIGNED_INT, 0);
	}

	glDisable(GL_BLEND);
}

void NCL::CSC8503::RendererSystemSOA::RenderShadowMapPass(GameWorldSOA& world, GameTechRendererDataSOA& frameData) {
	glBindFramebuffer(GL_FRAMEBUFFER, SOAResources.shadowFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glViewport(0, 0, SHADOWSIZE, SHADOWSIZE);
	glCullFace(GL_FRONT);

	glUseProgram(SOAResources.shadowShader->GetProgramID());

	Vector3 sunPos = world.GetSunPosition();
	Matrix4 shadowViewMatrix = Matrix::View(sunPos, Vector3(0, 0, 0), Vector3(0, 1, 0));
	Matrix4 shadowProjMatrix = Matrix::Perspective(100.0f, 500.0f, 1.0f, 45.0f);

	Matrix4 mvMatrix = shadowProjMatrix * shadowViewMatrix;
	// Store the biased shadow matrix for later use in opaque/transparent passes
	SOAResources.shadowMatrix = biasMatrix * mvMatrix;

	auto& objects = world.gameObjects;

	for (size_t idx : frameData.opaqueObjectIndices) {
		OGLMesh* mesh = (OGLMesh*)objects.render.meshes[idx];

		const Matrix4& modelMatrix = objects.transforms.matrices[idx];
		Matrix4 mvpMatrix = mvMatrix * modelMatrix;

		glUniformMatrix4fv(SOAResources.uniformCache.shadowShader_mvp, 1, false, (float*)&mvpMatrix);

		glBindVertexArray(mesh->GetVAO());
		glDrawElements(GL_TRIANGLES, mesh->GetIndexCount(), GL_UNSIGNED_INT, 0);
	}

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glViewport(0, 0, SOAResources.screenWidth, SOAResources.screenHeight);
	glCullFace(GL_BACK);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void NCL::CSC8503::RendererSystemSOA::RenderFrame(GameWorldSOA& world, GameTechRendererDataSOA& frameData) {
	UpdateMeshCache(world, frameData);
	glEnable(GL_CULL_FACE);
	glClearColor(1, 1, 1, 1);

	glViewport(0, 0, SOAResources.screenWidth, SOAResources.screenHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	BuildRenderFrame(world, frameData);

	RenderShadowMapPass(world, frameData);
	RenderSkyBoxPass(frameData);
	RenderOpaquePass(world, frameData);
	RenderTransparenetPass(world, frameData);
	RenderText();
}

void RendererSystemSOA::SetDebugStringBufferSizes(size_t newVertCount) {
	if (newVertCount <= SOAResources.textCount) {
		return;
	}

	SOAResources.textCount = newVertCount;

	glBindBuffer(GL_ARRAY_BUFFER, SOAResources.textVertVBO);
	glBufferData(GL_ARRAY_BUFFER, SOAResources.textCount * sizeof(Vector3), nullptr, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, SOAResources.textColourVBO);
	glBufferData(GL_ARRAY_BUFFER, SOAResources.textCount * sizeof(Vector4), nullptr, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, SOAResources.textTexVBO);
	glBufferData(GL_ARRAY_BUFFER, SOAResources.textCount * sizeof(Vector2), nullptr, GL_DYNAMIC_DRAW);

	SOAResources.debugTextPos.reserve(SOAResources.textCount);
	SOAResources.debugTextColours.reserve(SOAResources.textCount);
	SOAResources.debugTextUVs.reserve(SOAResources.textCount);

	glBindVertexArray(SOAResources.textVAO);

	glVertexAttribFormat(0, 3, GL_FLOAT, false, 0);
	glVertexAttribBinding(0, 0);
	glBindVertexBuffer(0, SOAResources.textVertVBO, 0, sizeof(Vector3));

	glVertexAttribFormat(1, 4, GL_FLOAT, false, 0);
	glVertexAttribBinding(1, 1);
	glBindVertexBuffer(1, SOAResources.textColourVBO, 0, sizeof(Vector4));

	glVertexAttribFormat(2, 2, GL_FLOAT, false, 0);
	glVertexAttribBinding(2, 2);
	glBindVertexBuffer(2, SOAResources.textTexVBO, 0, sizeof(Vector2));

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	glBindVertexArray(0);
}

void RendererSystemSOA::RenderText() {
	const std::vector<Debug::DebugStringEntry>& strings = Debug::GetDebugStrings();
	if (strings.empty() || Debug::GetDebugFont() == nullptr) {
		return;
	}

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(SOAResources.debugShader->GetProgramID());

	OGLTexture* fontTex = (OGLTexture*)Debug::GetDebugFont()->GetTexture();
	if (fontTex) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, fontTex->GetObjectID());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		GLuint mainTexSlot = glGetUniformLocation(SOAResources.debugShader->GetProgramID(), "mainTex");
		glUniform1i(mainTexSlot, 0);
	}

	Matrix4 proj = Matrix::Orthographic(0.0f, 100.0f, 100.0f, 0.0f, -1.0f, 1.0f);

	int matSlot = glGetUniformLocation(SOAResources.debugShader->GetProgramID(), "viewProjMatrix");
	glUniformMatrix4fv(matSlot, 1, false, (float*)proj.array);

	GLuint texSlot = glGetUniformLocation(SOAResources.debugShader->GetProgramID(), "useTexture");
	glUniform1i(texSlot, 1);

	SOAResources.debugTextPos.clear();
	SOAResources.debugTextColours.clear();
	SOAResources.debugTextUVs.clear();

	int frameVertCount = 0;
	for (const auto& s : strings) {
		frameVertCount += Debug::GetDebugFont()->GetVertexCountForString(s.data);
	}
	SetDebugStringBufferSizes(frameVertCount);

	for (const auto& s : strings) {
		Debug::GetDebugFont()->BuildVerticesForString(
			s.data, s.position, s.colour, 20.0f,
			SOAResources.debugTextPos, SOAResources.debugTextUVs, SOAResources.debugTextColours
		);
	}

	glBindBuffer(GL_ARRAY_BUFFER, SOAResources.textVertVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, frameVertCount * sizeof(Vector3), SOAResources.debugTextPos.data());
	glBindBuffer(GL_ARRAY_BUFFER, SOAResources.textColourVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, frameVertCount * sizeof(Vector4), SOAResources.debugTextColours.data());
	glBindBuffer(GL_ARRAY_BUFFER, SOAResources.textTexVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, frameVertCount * sizeof(Vector2), SOAResources.debugTextUVs.data());

	glBindVertexArray(SOAResources.textVAO);
	glDrawArrays(GL_TRIANGLES, 0, frameVertCount);
	glBindVertexArray(0);

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
}