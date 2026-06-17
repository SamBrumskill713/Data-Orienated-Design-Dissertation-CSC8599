#include "GameTechRendererSOA.h"

using namespace NCL;
using namespace Rendering;
using namespace CSC8503;

#define SHADOWSIZE 4096

static Matrix4 biasMatrix = Matrix::Translation(Vector3(0.5f, 0.5f, 0.5f)) * Matrix::Scale(Vector3(0.5f, 0.5f, 0.5f));
static Matrix4 shadowMatrix;

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
	glBindTexture(GL_TEXTURE_2D, 0);

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

void NCL::CSC8503::RendererSystemSOA::BuildRenderFrame(GameWorldSOA& world, GameTechRendererDataSOA& frameData){
	frameData.opaqueObjectIndices.clear();
	frameData.transparentObjectIndices.clear();
	Vector3 camPos = frameData.cameraPos;

	int count = world.GetObjectCount();
	auto& objects = world.gameObjects;

	std::vector<std::pair<size_t, float>> objectDistances;
	objectDistances.reserve(count);

	for (int i = 0; i < count; ++i) {
		if (!objects.isActive[i]) continue;
		float distSq = Vector::LengthSquared(camPos - objects.transforms.positions[i]);
		objectDistances.emplace_back(i, distSq);
		frameData.opaqueObjectIndices.push_back(i);
	}

	std::sort(frameData.opaqueObjectIndices.begin(), frameData.opaqueObjectIndices.end(), [&](size_t a, size_t b) {
		float distA = Vector::LengthSquared(camPos - objects.transforms.positions[a]);
		float distB = Vector::LengthSquared(camPos - objects.transforms.positions[b]);
		return distA < distB;
	});

	std::sort(frameData.transparentObjectIndices.begin(), frameData.transparentObjectIndices.end(), [&](size_t a, size_t b) {
		float distA = Vector::LengthSquared(camPos - objects.transforms.positions[a]);
		float distB = Vector::LengthSquared(camPos - objects.transforms.positions[b]);
		return distA < distB;
	});
}

void NCL::CSC8503::RendererSystemSOA::RenderSkyBoxPass(GameTechRendererDataSOA& frameData){
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	glUseProgram(SOAResources.skyboxShader->GetProgramID());

	int projLocation = glGetUniformLocation(SOAResources.skyboxShader->GetProgramID(), "projMatrix");
	int viewLocation = glGetUniformLocation(SOAResources.skyboxShader->GetProgramID(), "viewMatrix");
	int texLocation = glGetUniformLocation(SOAResources.skyboxShader->GetProgramID(), "cubeTex");

	glUniformMatrix4fv(projLocation, 1, false, (float*)&frameData.projMatrix);
	glUniformMatrix4fv(viewLocation, 1, false, (float*)&frameData.viewMatrix);

	// Set texture uniform BEFORE binding texture
	glUniform1i(texLocation, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, SOAResources.skyboxTex);

	glBindVertexArray(SOAResources.skyboxMesh->GetVAO());
	glDrawElements(GL_TRIANGLES, SOAResources.skyboxMesh->GetIndexCount(), GL_UNSIGNED_INT, 0);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
}

void NCL::CSC8503::RendererSystemSOA::RenderOpaquePass(GameWorldSOA& world, GameTechRendererDataSOA& frameData){
	glDisable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);

	glUseProgram(SOAResources.defaultShader->GetProgramID());

	int projLocation = glGetUniformLocation(SOAResources.defaultShader->GetProgramID(), "projMatrix");
	int viewLocation = glGetUniformLocation(SOAResources.defaultShader->GetProgramID(), "viewMatrix");
	int modelLocation = glGetUniformLocation(SOAResources.defaultShader->GetProgramID(), "modelMatrix");
	int colourLocation = glGetUniformLocation(SOAResources.defaultShader->GetProgramID(), "objectColour");
	int hasVColLocation = glGetUniformLocation(SOAResources.defaultShader->GetProgramID(), "hasVertexColours");
	int hasTexLocation = glGetUniformLocation(SOAResources.defaultShader->GetProgramID(), "hasTexture");

	int lightPosLocation = glGetUniformLocation(SOAResources.defaultShader->GetProgramID(), "sunPos");
	int lightColourLocation = glGetUniformLocation(SOAResources.defaultShader->GetProgramID(), "sunColour");
	int lightRadiusLocation = glGetUniformLocation(SOAResources.defaultShader->GetProgramID(), "sunRadius");

	int cameraLocation = glGetUniformLocation(SOAResources.defaultShader->GetProgramID(), "cameraPos");
	int shadowTexLocation = glGetUniformLocation(SOAResources.defaultShader->GetProgramID(), "shadowTex");
	int shadowLocation = glGetUniformLocation(SOAResources.defaultShader->GetProgramID(), "shadowMatrix");

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
	glBindTexture(GL_TEXTURE_2D, SOAResources.shadowTex);
	glUniform1i(shadowTexLocation, 1);

	auto& objects = world.gameObjects;
	for (size_t idx : frameData.opaqueObjectIndices) {
		OGLTexture* diffuseTex = (OGLTexture*)objects.render.diffuseTextures[idx];

		if (diffuseTex) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, diffuseTex->GetObjectID());
			glUniform1i(glGetUniformLocation(SOAResources.defaultShader->GetProgramID(), "mainTex"), 0);
		}

		Matrix4 modelMatrix = objects.transforms.matrices[idx];
		glUniformMatrix4fv(modelLocation, 1, false, (float*)&modelMatrix);

		Matrix4 fullShadowMat = shadowMatrix * modelMatrix;
		glUniformMatrix4fv(shadowLocation, 1, false, (float*)&fullShadowMat);

		glUniform4fv(colourLocation, 1, (float*)&objects.render.colours[idx]);
		glUniform1i(hasVColLocation, 0);
		glUniform1i(hasTexLocation, diffuseTex ? 1 : 0);

		glBindVertexArray(((Rendering::OGLMesh*)objects.render.meshes[idx])->GetVAO());
		GLuint indexCount = ((Rendering::OGLMesh*)objects.render.meshes[idx])->GetIndexCount();
		glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
	}
}

void NCL::CSC8503::RendererSystemSOA::RenderTransparenetPass(GameWorldSOA& world, GameTechRendererDataSOA& frameData){
	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glUseProgram(SOAResources.defaultShader->GetProgramID());

	int projLocation = glGetUniformLocation(SOAResources.defaultShader->GetProgramID(), "projMatrix");
	int viewLocation = glGetUniformLocation(SOAResources.defaultShader->GetProgramID(), "viewMatrix");
	int modelLocation = glGetUniformLocation(SOAResources.defaultShader->GetProgramID(), "modelMatrix");
	int colourLocation = glGetUniformLocation(SOAResources.defaultShader->GetProgramID(), "objectColour");
	int hasVColLocation = glGetUniformLocation(SOAResources.defaultShader->GetProgramID(), "hasVertexColours");
	int hasTexLocation = glGetUniformLocation(SOAResources.defaultShader->GetProgramID(), "hasTexture");

	int lightPosLocation = glGetUniformLocation(SOAResources.defaultShader->GetProgramID(), "sunPos");
	int lightColourLocation = glGetUniformLocation(SOAResources.defaultShader->GetProgramID(), "sunColour");
	int lightRadiusLocation = glGetUniformLocation(SOAResources.defaultShader->GetProgramID(), "sunRadius");

	int cameraLocation = glGetUniformLocation(SOAResources.defaultShader->GetProgramID(), "cameraPos");
	int shadowTexLocation = glGetUniformLocation(SOAResources.defaultShader->GetProgramID(), "shadowTex");
	int shadowLocation = glGetUniformLocation(SOAResources.defaultShader->GetProgramID(), "shadowMatrix");

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
	glBindTexture(GL_TEXTURE_2D, SOAResources.shadowTex);
	glUniform1i(shadowTexLocation, 1);

	auto& objects = world.gameObjects;
	for (size_t idx : frameData.transparentObjectIndices) {
		OGLTexture* diffuseTex = (OGLTexture*)objects.render.diffuseTextures[idx];

		if (diffuseTex) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, diffuseTex->GetObjectID());
			glUniform1i(glGetUniformLocation(SOAResources.defaultShader->GetProgramID(), "mainTex"), 0);
		}

		Matrix4 modelMatrix = objects.transforms.matrices[idx];
		glUniformMatrix4fv(modelLocation, 1, false, (float*)&modelMatrix);

		Matrix4 fullShadowMat = shadowMatrix * modelMatrix;
		glUniformMatrix4fv(shadowLocation, 1, false, (float*)&fullShadowMat);

		glUniform4fv(colourLocation, 1, (float*)&objects.render.colours[idx]);
		glUniform1i(hasVColLocation, 0);
		glUniform1i(hasTexLocation, diffuseTex ? 1 : 0);

		glBindVertexArray(((Rendering::OGLMesh*)objects.render.meshes[idx])->GetVAO());
		GLuint indexCount = ((Rendering::OGLMesh*)objects.render.meshes[idx])->GetIndexCount();
		glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
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
	int mvpLocation = glGetUniformLocation(SOAResources.shadowShader->GetProgramID(), "mvpMatrix");

	Vector3 sunPos = world.GetSunPosition();
	Matrix4 shadowViewMatrix = Matrix::View(sunPos, Vector3(0, 0, 0), Vector3(0, 1, 0));
	Matrix4 shadowProjMatrix = Matrix::Perspective(100.0f, 500.0f, 1.0f, 45.0f);

	Matrix4 mvMatrix = shadowProjMatrix * shadowViewMatrix;
	// Store the biased shadow matrix for later use in opaque/transparent passes
	shadowMatrix = biasMatrix * mvMatrix;

	auto& objects = world.gameObjects;
	for (size_t idx : frameData.opaqueObjectIndices) {
		Matrix4 modelMatrix = objects.transforms.matrices[idx];
		Matrix4 mvpMatrix = mvMatrix * modelMatrix;

		glUniformMatrix4fv(mvpLocation, 1, false, (float*)&mvpMatrix);
		glBindVertexArray(((Rendering::OGLMesh*)objects.render.meshes[idx])->GetVAO());
		glDrawElements(GL_TRIANGLES, ((Rendering::OGLMesh*)objects.render.meshes[idx])->GetIndexCount(), GL_UNSIGNED_INT, 0);
	}

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glViewport(0, 0, SOAResources.screenWidth, SOAResources.screenHeight);
	glCullFace(GL_BACK);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void NCL::CSC8503::RendererSystemSOA::RenderFrame(GameWorldSOA& world, GameTechRendererDataSOA& frameData){
	glEnable(GL_CULL_FACE);
	glClearColor(1, 1, 1, 1);

	glViewport(0, 0, SOAResources.screenWidth, SOAResources.screenHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	BuildRenderFrame(world, frameData);

	RenderShadowMapPass(world, frameData);
	RenderSkyBoxPass(frameData);
	RenderOpaquePass(world, frameData);
	RenderTransparenetPass(world, frameData);
}
