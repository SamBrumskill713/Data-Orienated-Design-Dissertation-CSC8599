#include "GameTechRendererDOD.h"
#include "TextureLoader.h"
#include "MshLoader.h"
#include "Debug.h"

#ifdef _WIN32
#include "Win32Window.h"
#include "glad/wgl.h"
#endif

using namespace NCL;
using namespace Rendering;
using namespace CSC8503;

#define SHADOWSIZE 4096

static Matrix4 biasMatrix = Matrix::Translation(Vector3(0.5f, 0.5f, 0.5f)) * Matrix::Scale(Vector3(0.5f, 0.5f, 0.5f));
static Matrix4 shadowMatrix;

void RendererSystemDOD::Initialise(Window* windowPtr) {
	window = windowPtr;
	resources.screenWidth = window->GetScreenSize().x;
	resources.screenHeight = window->GetScreenSize().y;

#ifdef _WIN32
	NCL::Win32Code::Win32Window* realWindow = (NCL::Win32Code::Win32Window*)windowPtr;
	deviceContext = GetDC(realWindow->GetHandle());
#endif

	glEnable(GL_DEPTH_TEST);

	// Loading Shaders
	resources.debugShader = new OGLShader("debug.vert", "debug.frag");
	resources.shadowShader = new OGLShader("shadow.vert", "shadow.frag");
	resources.defaultShader = new OGLShader("scene.vert", "scene.frag");

	// ShadowMap Texture
	glGenTextures(1, &resources.shadowTex);
	glBindTexture(GL_TEXTURE_2D, resources.shadowTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		SHADOWSIZE, SHADOWSIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Shadow FrameBuffer
	glGenFramebuffers(1, &resources.shadowFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, resources.shadowFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, resources.shadowTex, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "Shadow framebuffer is not complete!" << std::endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glClearColor(1, 1, 1, 1);

	// Skybox Shaders and Mesh
	resources.skyboxShader = new OGLShader("skybox.vert", "skybox.frag");
	resources.skyboxMesh = new OGLMesh();
	resources.skyboxMesh->SetVertexPositions({ Vector3(-1, 1, -1), Vector3(-1, -1, -1), Vector3(1, -1, -1), Vector3(1, 1, -1) });
	resources.skyboxMesh->SetVertexIndices({ 0, 1, 2, 2, 3, 0 });
	resources.skyboxMesh->UploadToGPU();

	// Skybox Textures
	std::string filenames[6] = {
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

	// Load all 6 cubemap faces
	for (int i = 0; i < 6; ++i) {
		TextureLoader::LoadTexture(filenames[i], texData[i], width[i], height[i], channels[i], flags[i]);
		if (i > 0 && (width[i] != width[0] || height[i] != height[0])) {
			std::cout << "Cubemap texture size mismatch!" << std::endl;
			return;
		}
	}

	glGenTextures(1, &resources.skyboxTex);
	glBindTexture(GL_TEXTURE_CUBE_MAP, resources.skyboxTex);

	GLenum type = channels[0] == 4 ? GL_RGBA : GL_RGB;

	// Upload all 6 faces to the cubemap
	for (int i = 0; i < 6; ++i) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width[i], height[i], 0, type, GL_UNSIGNED_BYTE, texData[i]);
	}

	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

Mesh* RendererSystemDOD::LoadMesh(const std::string& name) {
	OGLMesh* mesh = new OGLMesh();
	MshLoader::LoadMesh(name, *mesh);
	mesh->SetPrimitiveType(GeometryPrimitive::Triangles);
	mesh->UploadToGPU();
	return mesh;
}

Texture* RendererSystemDOD::LoadTexture(const std::string& name) {
	return OGLTexture::TextureFromFile(name).release();
}

void RendererSystemDOD::Destroy() {

	if (resources.defaultShader) {
		delete resources.defaultShader;
		resources.defaultShader = nullptr;
	}
	if (resources.shadowShader) {
		delete resources.shadowShader;
		resources.shadowShader = nullptr;
	}
	if (resources.skyboxShader) {
		delete resources.skyboxShader;
		resources.skyboxShader = nullptr;
	}
	if (resources.debugShader) {
		delete resources.debugShader;
		resources.debugShader = nullptr;
	}

	if (resources.skyboxMesh) {
		delete resources.skyboxMesh;
		resources.skyboxMesh = nullptr;
	}
	if (resources.debugTexMesh) {
		delete resources.debugTexMesh;
		resources.debugTexMesh = nullptr;
	}

	if (resources.shadowTex != 0) {
		glDeleteTextures(1, &resources.shadowTex);
		resources.shadowTex = 0;
	}
	if (resources.skyboxTex != 0) {
		glDeleteTextures(1, &resources.skyboxTex);
		resources.skyboxTex = 0;
	}

	if (resources.shadowFBO != 0) {
		glDeleteFramebuffers(1, &resources.shadowFBO);
		resources.shadowFBO = 0;
	}

	window = nullptr;
}

void RendererSystemDOD::swapBuffers()
{
#ifdef _WIN32
	if (deviceContext) {
		::SwapBuffers(deviceContext);
	}
#endif
}

void NCL::CSC8503::RendererSystemDOD::SetVerticalSync(int interval)
{
	static auto wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
	if (wglSwapIntervalEXT) {
		wglSwapIntervalEXT(interval);
	}
}

void RendererSystemDOD::BuildRenderFrame(GameWorldDOD& world, GameTechRendererData& frameData) {
	frameData.opaqueObjectIndices.clear();
	frameData.transparentObjectIndices.clear();

	Vector3 camPos = frameData.cameraPos;

	auto& objects = world.gameObjects.GetObjectArray();
	std::vector<std::pair<size_t, float>> objectDistances; 
	objectDistances.reserve(objects.size());

	for (size_t i = 0; i < objects.size(); ++i) {
		const GameObjectDOD& obj = objects[i];
		if (!obj.isActive) continue;

		float distSq = Vector::LengthSquared(camPos - obj.transform.position);
		objectDistances.emplace_back(i, distSq);

		frameData.opaqueObjectIndices.push_back(i);
	}

	std::sort(frameData.opaqueObjectIndices.begin(), frameData.opaqueObjectIndices.end(),
		[&](size_t a, size_t b) {
			float distA = Vector::LengthSquared(camPos - objects[a].transform.position);
			float distB = Vector::LengthSquared(camPos - objects[b].transform.position);
			return distA < distB;
		}
	);

	std::sort(frameData.transparentObjectIndices.rbegin(), frameData.transparentObjectIndices.rend(),
		[&](size_t a, size_t b) {
			float distA = Vector::LengthSquared(camPos - objects[a].transform.position);
			float distB = Vector::LengthSquared(camPos - objects[b].transform.position);
			return distA < distB;
		}
	);
}

void RendererSystemDOD::RenderSkyboxPass(GameTechRendererData& frameData) {
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	glUseProgram(resources.skyboxShader->GetProgramID());

	int projLocation = glGetUniformLocation(resources.skyboxShader->GetProgramID(), "projMatrix");
	int viewLocation = glGetUniformLocation(resources.skyboxShader->GetProgramID(), "viewMatrix");
	int texLocation = glGetUniformLocation(resources.skyboxShader->GetProgramID(), "cubeTex");

	glUniformMatrix4fv(projLocation, 1, false, (float*)&frameData.projMatrix);
	glUniformMatrix4fv(viewLocation, 1, false, (float*)&frameData.viewMatrix);

	// Set texture uniform BEFORE binding texture
	glUniform1i(texLocation, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, resources.skyboxTex);

	glBindVertexArray(resources.skyboxMesh->GetVAO());
	glDrawElements(GL_TRIANGLES,resources.skyboxMesh->GetIndexCount(), GL_UNSIGNED_INT, 0);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
}

void RendererSystemDOD::RenderShadowMapPass(GameWorldDOD& world, GameTechRendererData& frameData) {
	glBindFramebuffer(GL_FRAMEBUFFER, resources.shadowFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glViewport(0, 0, SHADOWSIZE, SHADOWSIZE);
	glCullFace(GL_FRONT);

	glUseProgram(resources.shadowShader->GetProgramID());
	int mvpLocation = glGetUniformLocation(resources.shadowShader->GetProgramID(), "mvpMatrix");

	Vector3 sunPos = world.GetSunPosition();
	Matrix4 shadowViewMatrix = Matrix::View(sunPos, Vector3(0, 0, 0), Vector3(0, 1, 0));
	Matrix4 shadowProjMatrix = Matrix::Perspective(100.0f, 500.0f, 1.0f, 45.0f);

	Matrix4 mvMatrix = shadowProjMatrix * shadowViewMatrix;
	shadowMatrix = biasMatrix * mvMatrix; // Store for use in opaque/transparent passes

	auto& objects = world.gameObjects.GetObjectArray();
	for (size_t idx : frameData.opaqueObjectIndices) {
		const GameObjectDOD& obj = objects[idx];
		Matrix4 modelMatrix = obj.transform.matrix;
		Matrix4 mvpMatrix = mvMatrix * modelMatrix;

		glUniformMatrix4fv(mvpLocation, 1, false, (float*)&mvpMatrix);
		glBindVertexArray(((Rendering::OGLMesh*)obj.render.mesh)->GetVAO());
		glDrawElements(GL_TRIANGLES, ((Rendering::OGLMesh*)obj.render.mesh)->GetIndexCount(), GL_UNSIGNED_INT, 0);
	}

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glViewport(0, 0, resources.screenWidth, resources.screenHeight);
	glCullFace(GL_BACK);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RendererSystemDOD::RenderOpaquePass(GameWorldDOD& world, GameTechRendererData& frameData) {
	glDisable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);

	glUseProgram(resources.defaultShader->GetProgramID());

	int projLocation = glGetUniformLocation(resources.defaultShader->GetProgramID(), "projMatrix");
	int viewLocation = glGetUniformLocation(resources.defaultShader->GetProgramID(), "viewMatrix");
	int modelLocation = glGetUniformLocation(resources.defaultShader->GetProgramID(), "modelMatrix");
	int colourLocation = glGetUniformLocation(resources.defaultShader->GetProgramID(), "objectColour");
	int hasVColLocation = glGetUniformLocation(resources.defaultShader->GetProgramID(), "hasVertexColours");
	int hasTexLocation = glGetUniformLocation(resources.defaultShader->GetProgramID(), "hasTexture");

	int lightPosLocation = glGetUniformLocation(resources.defaultShader->GetProgramID(), "sunPos");
	int lightColourLocation = glGetUniformLocation(resources.defaultShader->GetProgramID(), "sunColour");
	int lightRadiusLocation = glGetUniformLocation(resources.defaultShader->GetProgramID(), "sunRadius");

	int cameraLocation = glGetUniformLocation(resources.defaultShader->GetProgramID(), "cameraPos");
	int shadowTexLocation = glGetUniformLocation(resources.defaultShader->GetProgramID(), "shadowTex");
	int shadowLocation = glGetUniformLocation(resources.defaultShader->GetProgramID(), "shadowMatrix");

	glUniformMatrix4fv(projLocation, 1, false, (float*)&frameData.projMatrix);
	glUniformMatrix4fv(viewLocation, 1, false, (float*)&frameData.viewMatrix);

	Vector3 camPos = frameData.cameraPos;
	glUniform3fv(cameraLocation, 1, &camPos.x);

	Vector3 sunPos = world.GetSunPosition();
	Vector3 sunCol = world.GetSunColour();
	float sunRadius = 10000.0f;
	glUniform3fv(lightPosLocation, 1, (float*)&sunPos);
	glUniform3fv(lightColourLocation, 1, (float*)&sunCol);
	glUniform1f(lightRadiusLocation, sunRadius);

	glActiveTexture(GL_TEXTURE0 + 1);
	glBindTexture(GL_TEXTURE_2D, resources.shadowTex);
	glUniform1i(shadowTexLocation, 1);

	auto& objects = world.gameObjects.GetObjectArray();
	for (size_t idx : frameData.opaqueObjectIndices) {
		const GameObjectDOD& obj = objects[idx];
		OGLTexture* diffuseTex = (OGLTexture*)obj.render.material.diffuseTex;

		if (diffuseTex) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, diffuseTex->GetObjectID());
			glUniform1i(glGetUniformLocation(resources.defaultShader->GetProgramID(), "mainTex"), 0);
		}

		Matrix4 modelMatrix = obj.transform.matrix;
		glUniformMatrix4fv(modelLocation, 1, false, (float*)&modelMatrix);

		Matrix4 fullShadowMat = shadowMatrix * modelMatrix;
		glUniformMatrix4fv(shadowLocation, 1, false, (float*)&fullShadowMat);

		glUniform4fv(colourLocation, 1, (float*)&obj.render.colour);
		glUniform1i(hasVColLocation, 0); 
		glUniform1i(hasTexLocation, diffuseTex ? 1 : 0);

		glBindVertexArray(((Rendering::OGLMesh*)obj.render.mesh)->GetVAO());
		GLuint indexCount = ((Rendering::OGLMesh*)obj.render.mesh)->GetIndexCount();
		glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
	}
}

void RendererSystemDOD::RenderTransparentPass(GameWorldDOD& world, GameTechRendererData& frameData) {
	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glUseProgram(resources.defaultShader->GetProgramID());

	int projLocation = glGetUniformLocation(resources.defaultShader->GetProgramID(), "projMatrix");
	int viewLocation = glGetUniformLocation(resources.defaultShader->GetProgramID(), "viewMatrix");
	int modelLocation = glGetUniformLocation(resources.defaultShader->GetProgramID(), "modelMatrix");
	int colourLocation = glGetUniformLocation(resources.defaultShader->GetProgramID(), "objectColour");
	int hasVColLocation = glGetUniformLocation(resources.defaultShader->GetProgramID(), "hasVertexColours");
	int hasTexLocation = glGetUniformLocation(resources.defaultShader->GetProgramID(), "hasTexture");

	int lightPosLocation = glGetUniformLocation(resources.defaultShader->GetProgramID(), "sunPos");
	int lightColourLocation = glGetUniformLocation(resources.defaultShader->GetProgramID(), "sunColour");
	int lightRadiusLocation = glGetUniformLocation(resources.defaultShader->GetProgramID(), "sunRadius");

	int cameraLocation = glGetUniformLocation(resources.defaultShader->GetProgramID(), "cameraPos");
	int shadowTexLocation = glGetUniformLocation(resources.defaultShader->GetProgramID(), "shadowTex");
	int shadowLocation = glGetUniformLocation(resources.defaultShader->GetProgramID(), "shadowMatrix");

	glUniformMatrix4fv(projLocation, 1, false, (float*)&frameData.projMatrix);
	glUniformMatrix4fv(viewLocation, 1, false, (float*)&frameData.viewMatrix);

	Vector3 camPos = frameData.cameraPos;
	glUniform3fv(cameraLocation, 1, &camPos.x);

	Vector3 sunPos = world.GetSunPosition();
	Vector3 sunCol = world.GetSunColour();
	float sunRadius = 10000.0f;
	glUniform3fv(lightPosLocation, 1, (float*)&sunPos);
	glUniform3fv(lightColourLocation, 1, (float*)&sunCol);
	glUniform1f(lightRadiusLocation, sunRadius);

	glActiveTexture(GL_TEXTURE0 + 1);
	glBindTexture(GL_TEXTURE_2D, resources.shadowTex);
	glUniform1i(shadowTexLocation, 1);

	auto& objects = world.gameObjects.GetObjectArray();
	for (size_t idx : frameData.transparentObjectIndices) {
		const GameObjectDOD& obj = objects[idx];
		OGLTexture* diffuseTex = (OGLTexture*)obj.render.material.diffuseTex;

		if (diffuseTex) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, diffuseTex->GetObjectID());
			glUniform1i(glGetUniformLocation(resources.defaultShader->GetProgramID(), "mainTex"), 0);
		}

		Matrix4 modelMatrix = obj.transform.matrix;
		glUniformMatrix4fv(modelLocation, 1, false, (float*)&modelMatrix);

		Matrix4 fullShadowMat = shadowMatrix * modelMatrix;
		glUniformMatrix4fv(shadowLocation, 1, false, (float*)&fullShadowMat);

		glUniform4fv(colourLocation, 1, (float*)&obj.render.colour);
		glUniform1i(hasVColLocation, 0);
		glUniform1i(hasTexLocation, diffuseTex ? 1 : 0);

		glBindVertexArray(((Rendering::OGLMesh*)obj.render.mesh)->GetVAO());
		GLuint indexCount = ((Rendering::OGLMesh*)obj.render.mesh)->GetIndexCount();
		glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
	}

	glDisable(GL_BLEND);
}

void RendererSystemDOD::RenderFrame(GameWorldDOD& world, GameTechRendererData& frameData) {
	glEnable(GL_CULL_FACE);
	glClearColor(1, 1, 1, 1);

	glViewport(0, 0, resources.screenWidth, resources.screenHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	BuildRenderFrame(world, frameData);

	RenderShadowMapPass(world, frameData);
	RenderSkyboxPass(frameData);
	RenderOpaquePass(world, frameData);
	RenderTransparentPass(world, frameData);
}