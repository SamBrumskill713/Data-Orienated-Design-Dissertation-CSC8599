#include "GameTechRenderer.h"
#include "GameObject.h"
#include "GameWorld.h"
#include "RenderObject.h"
#include "Camera.h"
#include "TextureLoader.h"
#include "MshLoader.h"

#include "Debug.h"

#include "OGLRenderer.h"
#include "OGLShader.h"
#include "OGLTexture.h"
#include "OGLMesh.h"

using namespace NCL;
using namespace Rendering;
using namespace CSC8503;

#define SHADOWSIZE 4096

Matrix4 biasMatrix = Matrix::Translation(Vector3(0.5f, 0.5f, 0.5f)) * Matrix::Scale(Vector3(0.5f, 0.5f, 0.5f));

GameTechRenderer::GameTechRenderer(GameWorld& world) : OGLRenderer(*Window::GetWindow()), gameWorld(world)	{
	glEnable(GL_DEPTH_TEST);

	debugShader  = new OGLShader("debug.vert", "debug.frag");
	shadowShader = new OGLShader("shadow.vert", "shadow.frag");
	defaultShader = new OGLShader("scene.vert", "scene.frag");

	glGenTextures(1, &shadowTex);
	glBindTexture(GL_TEXTURE_2D, shadowTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
			     SHADOWSIZE, SHADOWSIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffers(1, &shadowFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D, shadowTex, 0);
	glDrawBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glClearColor(1, 1, 1, 1);

	////Set up the light properties
	//lightColour = Vector4(0.8f, 0.8f, 0.5f, 1.0f);
	//lightRadius = 1000.0f;
	//lightPosition = Vector3(-200.0f, 60.0f, -200.0f);

	//Skybox!
	skyboxShader = new OGLShader("skybox.vert", "skybox.frag");
	skyboxMesh = new OGLMesh();
	skyboxMesh->SetVertexPositions({Vector3(-1, 1,-1), Vector3(-1,-1,-1) , Vector3(1,-1,-1) , Vector3(1,1,-1) });
	skyboxMesh->SetVertexIndices({ 0,1,2,2,3,0 });
	skyboxMesh->UploadToGPU();

	LoadSkybox();

	glGenVertexArrays(1, &lineVAO);
	glGenVertexArrays(1, &textVAO);

	glGenBuffers(1, &lineVertVBO);
	glGenBuffers(1, &textVertVBO);
	glGenBuffers(1, &textColourVBO);
	glGenBuffers(1, &textTexVBO);

	Debug::CreateDebugFont("PressStart2P.fnt", *LoadTexture("PressStart2P.png"));

	//Debug quad for drawing tex
	debugTexMesh = new OGLMesh();
	debugTexMesh->SetVertexPositions({ Vector3(-1, 1,0), Vector3(-1,-1,0) , Vector3(1,-1,0) , Vector3(1,1,0) });
	debugTexMesh->SetVertexTextureCoords({ Vector2(0, 1), Vector2(0,0) , Vector2(1,0) , Vector2(1,1) });
	debugTexMesh->SetVertexIndices({ 0,1,2,2,3,0 });
	debugTexMesh->UploadToGPU();


	SetDebugStringBufferSizes(10000);
	SetDebugLineBufferSizes(1000);
}

GameTechRenderer::~GameTechRenderer()	{
	glDeleteTextures(1, &shadowTex);
	glDeleteFramebuffers(1, &shadowFBO);
}

void GameTechRenderer::LoadSkybox() {
	std::string filenames[6] = {
		"/Cubemap/skyrender0004.png",
		"/Cubemap/skyrender0001.png",
		"/Cubemap/skyrender0003.png",
		"/Cubemap/skyrender0006.png",
		"/Cubemap/skyrender0002.png",
		"/Cubemap/skyrender0005.png"
	};

	uint32_t width[6]	 = { 0 };
	uint32_t height[6]	 = { 0 };
	uint32_t channels[6] = { 0 };
	uint32_t flags[6]	 = { 0 };

	vector<char*> texData(6, nullptr);

	for (int i = 0; i < 6; ++i) {
		TextureLoader::LoadTexture(filenames[i], texData[i], width[i], height[i], channels[i], flags[i]);
		if (i > 0 && (width[i] != width[0] || height[0] != height[0])) {
			std::cout << __FUNCTION__ << " cubemap input textures don't match in size?\n";
			return;
		}
	}
	glGenTextures(1, &skyboxTex);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTex);

	GLenum type = channels[0] == 4 ? GL_RGBA : GL_RGB;

	for (int i = 0; i < 6; ++i) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width[i], height[i], 0, type, GL_UNSIGNED_BYTE, texData[i]);
	}

	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

Mesh* GameTechRenderer::LoadMesh(const std::string& name) {
	OGLMesh* mesh = new OGLMesh();
	MshLoader::LoadMesh(name, *mesh);
	mesh->SetPrimitiveType(GeometryPrimitive::Triangles);
	mesh->UploadToGPU();
	return mesh;
}

Texture* GameTechRenderer::LoadTexture(const std::string& name) {
	return OGLTexture::TextureFromFile(name).release();
}

void GameTechRenderer::RenderFrame() {
	glEnable(GL_CULL_FACE);
	glClearColor(1, 1, 1, 1);
	BuildObjectLists();
	
	{
		OGLDebugScope scope("Shadow map pass");
		RenderShadowMapPass(opaqueObjects);
	}
	{
		OGLDebugScope scope("Skybox pass");
		RenderSkyboxPass();
	}
	{
		OGLDebugScope scope("Opaque pass");
		RenderOpaquePass(opaqueObjects);
	}
	{
		OGLDebugScope scope("Transparent pass");
		RenderTransparentPass(transparentObjects);
	}

	{
		OGLDebugScope scope("Debug pass");
		glDisable(GL_CULL_FACE); //Todo - text indices are going the wrong way...
		glDisable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		RenderLines();
		RenderTextures();
		RenderText();
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
}

void GameTechRenderer::BuildObjectLists() {
	opaqueObjects.clear();
	transparentObjects.clear();

	Vector3 camPos = gameWorld.GetMainCamera().GetPosition();

	gameWorld.OperateOnContents(
		[&](GameObject* o) {
			if (o->IsActive()) {
				const RenderObject* g = o->GetRenderObject();
				if (g) {
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
}

void GameTechRenderer::RenderShadowMapPass(std::vector<ObjectSortState>& list) {
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glViewport(0, 0, SHADOWSIZE, SHADOWSIZE);

	glCullFace(GL_FRONT);

	UseShader(*shadowShader);
	int mvpLocation = glGetUniformLocation(shadowShader->GetProgramID(), "mvpMatrix");

	Matrix4 shadowViewMatrix = Matrix::View(gameWorld.GetSunPosition(), Vector3(0, 0, 0), Vector3(0, 1, 0));
	Matrix4 shadowProjMatrix = Matrix::Perspective(100.0f, 500.0f, 1.0f, 45.0f);

	Matrix4 mvMatrix = shadowProjMatrix * shadowViewMatrix;

	shadowMatrix = biasMatrix * mvMatrix; //we'll use this one later on

	for (const auto&i : list) {
		const RenderObject* o = i.object;

		Matrix4 modelMatrix = o->GetTransform().GetMatrix();
		Matrix4 mvpMatrix	= mvMatrix * modelMatrix;
		glUniformMatrix4fv(mvpLocation, 1, false, (float*)&mvpMatrix);
		BindMesh((OGLMesh&)*o->GetMesh());
		size_t layerCount = o->GetMesh()->GetSubMeshCount();
		for (size_t i = 0; i < layerCount; ++i) {
			DrawBoundMesh((uint32_t)i);
		}
	}

	glViewport(0, 0, windowSize.x, windowSize.y);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glCullFace(GL_BACK);
}

void GameTechRenderer::RenderSkyboxPass() {
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	Matrix4 viewMatrix = gameWorld.GetMainCamera().BuildViewMatrix();
	Matrix4 projMatrix = gameWorld.GetMainCamera().BuildProjectionMatrix(hostWindow.GetScreenAspect());

	UseShader(*skyboxShader);

	int projLocation = glGetUniformLocation(skyboxShader->GetProgramID(), "projMatrix");
	int viewLocation = glGetUniformLocation(skyboxShader->GetProgramID(), "viewMatrix");
	int texLocation  = glGetUniformLocation(skyboxShader->GetProgramID(), "cubeTex");

	glUniformMatrix4fv(projLocation, 1, false, (float*)&projMatrix);
	glUniformMatrix4fv(viewLocation, 1, false, (float*)&viewMatrix);

	glUniform1i(texLocation, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTex);

	BindMesh(*skyboxMesh);
	DrawBoundMesh();

	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
}

void GameTechRenderer::RenderOpaquePass(std::vector<ObjectSortState>& list) {
	glDisable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	
	UseShader(*defaultShader);

	int projLocation		= glGetUniformLocation(activeShader->GetProgramID(), "projMatrix");
	int viewLocation		= glGetUniformLocation(activeShader->GetProgramID(), "viewMatrix");
	int modelLocation		= glGetUniformLocation(activeShader->GetProgramID(), "modelMatrix");
	int colourLocation		= glGetUniformLocation(activeShader->GetProgramID(), "objectColour");
	int hasVColLocation		= glGetUniformLocation(activeShader->GetProgramID(), "hasVertexColours");
	int hasTexLocation		= glGetUniformLocation(activeShader->GetProgramID(), "hasTexture");
	
	int lightPosLocation	= glGetUniformLocation(activeShader->GetProgramID(), "sunPos");
	int lightColourLocation = glGetUniformLocation(activeShader->GetProgramID(), "sunColour");
	int lightRadiusLocation = glGetUniformLocation(activeShader->GetProgramID(), "sunRadius");

	int cameraLocation		= glGetUniformLocation(activeShader->GetProgramID(), "cameraPos");
	int shadowTexLocation	= glGetUniformLocation(activeShader->GetProgramID(), "shadowTex");
	int shadowLocation		= glGetUniformLocation(activeShader->GetProgramID(), "shadowMatrix");

	Matrix4 viewMatrix = gameWorld.GetMainCamera().BuildViewMatrix();
	Matrix4 projMatrix = gameWorld.GetMainCamera().BuildProjectionMatrix(hostWindow.GetScreenAspect());
	glUniformMatrix4fv(projLocation, 1, false, (float*)&projMatrix);
	glUniformMatrix4fv(viewLocation, 1, false, (float*)&viewMatrix);

	Vector3 camPos = gameWorld.GetMainCamera().GetPosition();
	glUniform3fv(cameraLocation, 1, &camPos.x);

	Vector3 sunPos		= gameWorld.GetSunPosition();
	Vector3 sunCol		= gameWorld.GetSunColour();
	float	sunRadius	= 10000.0f;
	glUniform3fv(lightPosLocation, 1, (float*)&sunPos);
	glUniform3fv(lightColourLocation, 1, (float*)&sunCol);
	glUniform1f(lightRadiusLocation, sunRadius);

	//TODO - PUT IN FUNCTION
	glActiveTexture(GL_TEXTURE0 + 1);
	glBindTexture(GL_TEXTURE_2D, shadowTex);
	glUniform1i(shadowTexLocation, 1);

	for (const auto& i : list) {
		const RenderObject* o = i.object;
		OGLTexture* diffuseTex = (OGLTexture*)o->GetMaterial().diffuseTex;

		if (diffuseTex) {
			BindTextureToShader(*diffuseTex, "mainTex", 0);
		}
		Matrix4 modelMatrix = o->GetTransform().GetMatrix();
		glUniformMatrix4fv(modelLocation, 1, false, (float*)&modelMatrix);

		Matrix4 fullShadowMat = shadowMatrix * modelMatrix;
		glUniformMatrix4fv(shadowLocation, 1, false, (float*)&fullShadowMat);

		Vector4 colour = o->GetColour();
		glUniform4fv(colourLocation, 1, &colour.x);

		glUniform1i(hasVColLocation, !o->GetMesh()->GetColourData().empty());

		glUniform1i(hasTexLocation, diffuseTex ? 1 : 0);

		BindMesh((OGLMesh&)*o->GetMesh());
		size_t layerCount = o->GetMesh()->GetSubMeshCount();
		for (size_t i = 0; i < layerCount; ++i) {
			DrawBoundMesh((uint32_t)i);
		}
	}
}

void GameTechRenderer::RenderTransparentPass(std::vector<ObjectSortState>& list) {
	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);

	UseShader(*defaultShader);

	int projLocation		= glGetUniformLocation(activeShader->GetProgramID(), "projMatrix");
	int viewLocation		= glGetUniformLocation(activeShader->GetProgramID(), "viewMatrix");
	int modelLocation		= glGetUniformLocation(activeShader->GetProgramID(), "modelMatrix");
	int colourLocation		= glGetUniformLocation(activeShader->GetProgramID(), "objectColour");
	int hasVColLocation		= glGetUniformLocation(activeShader->GetProgramID(), "hasVertexColours");
	int hasTexLocation		= glGetUniformLocation(activeShader->GetProgramID(), "hasTexture");

	int lightPosLocation	= glGetUniformLocation(activeShader->GetProgramID(), "sunPos");
	int lightColourLocation = glGetUniformLocation(activeShader->GetProgramID(), "sunColour");
	int lightRadiusLocation = glGetUniformLocation(activeShader->GetProgramID(), "sunRadius");

	int cameraLocation		= glGetUniformLocation(activeShader->GetProgramID(), "cameraPos");
	int shadowTexLocation	= glGetUniformLocation(activeShader->GetProgramID(), "shadowTex");
	int shadowLocation		= glGetUniformLocation(activeShader->GetProgramID(), "shadowMatrix");

	Matrix4 viewMatrix = gameWorld.GetMainCamera().BuildViewMatrix();
	Matrix4 projMatrix = gameWorld.GetMainCamera().BuildProjectionMatrix(hostWindow.GetScreenAspect());
	glUniformMatrix4fv(projLocation, 1, false, (float*)&projMatrix);
	glUniformMatrix4fv(viewLocation, 1, false, (float*)&viewMatrix);

	Vector3 camPos = gameWorld.GetMainCamera().GetPosition();
	glUniform3fv(cameraLocation, 1, &camPos.x);

	Vector3 sunPos		= gameWorld.GetSunPosition();
	Vector3 sunCol		= gameWorld.GetSunColour();
	float	sunRadius	= 10000.0f;
	glUniform3fv(lightPosLocation, 1, (float*)&sunPos);
	glUniform3fv(lightColourLocation, 1, (float*)&sunCol);
	glUniform1f(lightRadiusLocation, sunRadius);

	//TODO - PUT IN FUNCTION
	glActiveTexture(GL_TEXTURE0 + 1);
	glBindTexture(GL_TEXTURE_2D, shadowTex);
	glUniform1i(shadowTexLocation, 1);

	for (const auto& i : list) {
		const RenderObject* o = i.object;
		OGLTexture* diffuseTex = (OGLTexture*)o->GetMaterial().diffuseTex;

		if (diffuseTex) {
			BindTextureToShader(*diffuseTex, "mainTex", 0);
		}
		Matrix4 modelMatrix = o->GetTransform().GetMatrix();
		glUniformMatrix4fv(modelLocation, 1, false, (float*)&modelMatrix);

		Matrix4 fullShadowMat = shadowMatrix * modelMatrix;
		glUniformMatrix4fv(shadowLocation, 1, false, (float*)&fullShadowMat);

		Vector4 colour = o->GetColour();
		glUniform4fv(colourLocation, 1, &colour.x);

		glUniform1i(hasVColLocation, !o->GetMesh()->GetColourData().empty());

		glUniform1i(hasTexLocation, diffuseTex ? 1 : 0);
	
		BindMesh((OGLMesh&)*o->GetMesh());
			
		size_t layerCount = o->GetMesh()->GetSubMeshCount();

		glCullFace(GL_FRONT);
		for (size_t i = 0; i < layerCount; ++i) {
			DrawBoundMesh((uint32_t)i);
		}
		glCullFace(GL_BACK);
		for (size_t i = 0; i < layerCount; ++i) {
			DrawBoundMesh((uint32_t)i);
		}
	}
	glDisable(GL_BLEND);
}


void GameTechRenderer::RenderLines() {
	const std::vector<Debug::DebugLineEntry>& lines = Debug::GetDebugLines();
	if (lines.empty()) {
		return;
	}

	Matrix4 viewMatrix = gameWorld.GetMainCamera().BuildViewMatrix();
	Matrix4 projMatrix = gameWorld.GetMainCamera().BuildProjectionMatrix(hostWindow.GetScreenAspect());
	
	Matrix4 viewProj  = projMatrix * viewMatrix;

	UseShader(*debugShader);
	int matSlot = glGetUniformLocation(debugShader->GetProgramID(), "viewProjMatrix");
	GLuint texSlot = glGetUniformLocation(debugShader->GetProgramID(), "useTexture");
	glUniform1i(texSlot, 0);

	glUniformMatrix4fv(matSlot, 1, false, (float*)viewProj.array);

	debugLineData.clear();

	size_t frameLineCount = lines.size() * 2;

	SetDebugLineBufferSizes(frameLineCount);

	glBindBuffer(GL_ARRAY_BUFFER, lineVertVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, lines.size() * sizeof(Debug::DebugLineEntry), lines.data());
	
	glBindVertexArray(lineVAO);
	glDrawArrays(GL_LINES, 0, (GLsizei)frameLineCount);
	glBindVertexArray(0);
}

void GameTechRenderer::RenderText() {
	const std::vector<Debug::DebugStringEntry>& strings = Debug::GetDebugStrings();
	if (strings.empty()) {
		return;
	}

	UseShader(*debugShader);

	OGLTexture* t = (OGLTexture*)Debug::GetDebugFont()->GetTexture();

	if (t) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, t->GetObjectID());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);	
		BindTextureToShader(*t, "mainTex", 0);
	}

	Matrix4 proj = Matrix::Orthographic(0.0f, 100.0f, 100.0f, 0.0f, -1.0f, 1.0f);

	int matSlot = glGetUniformLocation(debugShader->GetProgramID(), "viewProjMatrix");
	glUniformMatrix4fv(matSlot, 1, false, (float*)proj.array);

	GLuint texSlot = glGetUniformLocation(debugShader->GetProgramID(), "useTexture");
	glUniform1i(texSlot, 1);

	debugTextPos.clear();
	debugTextColours.clear();
	debugTextUVs.clear();

	int frameVertCount = 0;
	for (const auto& s : strings) {
		frameVertCount += Debug::GetDebugFont()->GetVertexCountForString(s.data);
	}
	SetDebugStringBufferSizes(frameVertCount);

	for (const auto& s : strings) {
		float size = 20.0f;
		Debug::GetDebugFont()->BuildVerticesForString(s.data, s.position, s.colour, size, debugTextPos, debugTextUVs, debugTextColours);
	}

	glBindBuffer(GL_ARRAY_BUFFER, textVertVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, frameVertCount * sizeof(Vector3), debugTextPos.data());
	glBindBuffer(GL_ARRAY_BUFFER, textColourVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, frameVertCount * sizeof(Vector4), debugTextColours.data());
	glBindBuffer(GL_ARRAY_BUFFER, textTexVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, frameVertCount * sizeof(Vector2), debugTextUVs.data());

	glBindVertexArray(textVAO);
	glDrawArrays(GL_TRIANGLES, 0, frameVertCount);
	glBindVertexArray(0);
}

void GameTechRenderer::RenderTextures() {
	const std::vector<Debug::DebugTexEntry>& texEntries = Debug::GetDebugTex();
	if (texEntries.empty()) {
		return;
	}
	UseShader(*debugShader);

	Matrix4 proj = Matrix::Orthographic(0.0f, 100.0f, 100.0f, 0.0f, -1.0f, 1.0f);

	int matSlot = glGetUniformLocation(debugShader->GetProgramID(), "viewProjMatrix");
	glUniformMatrix4fv(matSlot, 1, false, (float*)proj.array);

	GLuint texSlot = glGetUniformLocation(debugShader->GetProgramID(), "useTexture");
	glUniform1i(texSlot, 2);

	GLuint useColourSlot = glGetUniformLocation(debugShader->GetProgramID(), "useColour");
	glUniform1i(useColourSlot, 1);

	GLuint colourSlot = glGetUniformLocation(debugShader->GetProgramID(), "texColour");

	BindMesh(*debugTexMesh);

	glActiveTexture(GL_TEXTURE0);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	
	for (const auto& tex : texEntries) {	
		OGLTexture* t = (OGLTexture*)tex.t;
		glBindTexture(GL_TEXTURE_2D, t->GetObjectID());	
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		BindTextureToShader(*t, "mainTex", 0);

		Matrix4 transform = Matrix::Translation(Vector3(tex.position.x, tex.position.y, 0)) * Matrix::Scale(Vector3(tex.scale.x, tex.scale.y, 1.0f));
		Matrix4 finalMatrix = proj * transform;

		glUniformMatrix4fv(matSlot, 1, false, (float*)finalMatrix.array);

		glUniform4f(colourSlot, tex.colour.x, tex.colour.y, tex.colour.z, tex.colour.w);

		DrawBoundMesh();
	}

	glUniform1i(useColourSlot, 0);
}
 
void GameTechRenderer::SetDebugStringBufferSizes(size_t newVertCount) {
	if (newVertCount > textCount) {
		textCount = newVertCount;

		glBindBuffer(GL_ARRAY_BUFFER, textVertVBO);
		glBufferData(GL_ARRAY_BUFFER, textCount * sizeof(Vector3), nullptr, GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, textColourVBO);
		glBufferData(GL_ARRAY_BUFFER, textCount * sizeof(Vector4), nullptr, GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, textTexVBO);
		glBufferData(GL_ARRAY_BUFFER, textCount * sizeof(Vector2), nullptr, GL_DYNAMIC_DRAW);

		debugTextPos.reserve(textCount);
		debugTextColours.reserve(textCount);
		debugTextUVs.reserve(textCount);

		glBindVertexArray(textVAO);

		glVertexAttribFormat(0, 3, GL_FLOAT, false, 0);
		glVertexAttribBinding(0, 0);
		glBindVertexBuffer(0, textVertVBO, 0, sizeof(Vector3));

		glVertexAttribFormat(1, 4, GL_FLOAT, false, 0);
		glVertexAttribBinding(1, 1);
		glBindVertexBuffer(1, textColourVBO, 0, sizeof(Vector4));

		glVertexAttribFormat(2, 2, GL_FLOAT, false, 0);
		glVertexAttribBinding(2, 2);
		glBindVertexBuffer(2, textTexVBO, 0, sizeof(Vector2));

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		glBindVertexArray(0);
	}
}

void GameTechRenderer::SetDebugLineBufferSizes(size_t newVertCount) {
	if (newVertCount > lineCount) {
		lineCount = newVertCount;

		glBindBuffer(GL_ARRAY_BUFFER, lineVertVBO);
		glBufferData(GL_ARRAY_BUFFER, lineCount * sizeof(Debug::DebugLineEntry), nullptr, GL_DYNAMIC_DRAW);

		debugLineData.reserve(lineCount);

		glBindVertexArray(lineVAO);

		int realStride = sizeof(Debug::DebugLineEntry) / 2;

		glVertexAttribFormat(0, 3, GL_FLOAT, false, offsetof(Debug::DebugLineEntry, start));
		glVertexAttribBinding(0, 0);
		glBindVertexBuffer(0, lineVertVBO, 0, realStride);

		glVertexAttribFormat(1, 4, GL_FLOAT, false, offsetof(Debug::DebugLineEntry, colourA));
		glVertexAttribBinding(1, 0);
		glBindVertexBuffer(1, lineVertVBO, sizeof(Vector4), realStride);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);

		glBindVertexArray(0);
	}
}