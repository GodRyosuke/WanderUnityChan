#define _CRT_SECURE_NO_WARNINGS


#include "Game.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtc/type_ptr.hpp"
#include "gtx/rotate_vector.hpp"
#include "gtx/vector_angle.hpp"

Game::Game()
	:mWindowWidth(1024),
	mWindowHeight(768),
	mIsRunning(true),
	mMoveSpeed(0.1),
	mMoveSensitivity(100.0f)
{
	mCameraPos = glm::vec3(-1.0f, 2.5f, 1.0f);
	mCameraUP = glm::vec3(0.0f, 0.0f, 1.0f);
	mCameraOrientation = glm::vec3(0.5f, 0, 0);

	mSpotLight.Position = glm::vec3(-1.0f, 2.5f, 10.0f);
	mSpotLight.Direction = glm::vec3(0.5f, 0.0f, -1.0f);
	mSpotLight.Up = glm::vec3(0.0f, 0.0f, 1.0f);
}


bool Game::Initialize()
{

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
	{
		printf("Unable to initialize SDL: %s", SDL_GetError());
		return false;
	}

	// Set OpenGL attributes
	// Use the core OpenGL profile
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	// Specify version 3.3
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	// Request a color buffer with 8-bits per RGBA channel
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	// Enable double buffering
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	// Force OpenGL to use hardware acceleration
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);


	mWindow = SDL_CreateWindow("Wander OpenGL Tutorial", 100, 100,
		mWindowWidth, mWindowHeight, SDL_WINDOW_OPENGL);
	if (!mWindow)
	{
		printf("Failed to create window: %s", SDL_GetError());
		return false;
	}

	// Create an OpenGL context
	mContext = SDL_GL_CreateContext(mWindow);

	// Initialize GLEW
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		printf("Failed to initialize GLEW.");
		return false;
	}

	auto error_code = glGetError();

	//if (!LoadShaders())
	//{
	//	SDL_Log("Failed to load shaders.");
	//	return false;
	//}


	////CreateSpriteVerts();

	if (!LoadData())
	{
		printf("error: Failed to Load Game Data\n");
		return false;
	}



	mTicksCount = SDL_GetTicks();

	return true;
}

void Game::SetShaderLighting()
{
	std::vector<Shader*> Shaders;
	Shaders.push_back(mShadowLightingShader);
	Shaders.push_back(mSkinShadowLightingShader);

	for (auto shader : Shaders) {
		shader->UseProgram();
		shader->SetVectorUniform("gEyeWorldPos", mCameraPos);
		shader->SetSamplerUniform("gShadowMap", 1);
		shader->SetSamplerUniform("gSampler", 0);
		shader->SetSamplerUniform("gNumSpotLights", 1);

		// spot light
		for (int i = 0; i < 1; i++) {
			std::string uniformName;
			std::string pD = std::to_string(i);
			uniformName = "gSpotLights[" + pD + "].Base.Base.Color";
			shader->SetVectorUniform(uniformName, glm::vec3(1.0f, 1.0f, 1.0f));
			uniformName = "gSpotLights[" + pD + "].Base.Base.AmbientIntensity";
			shader->SetFloatUniform(uniformName, 0.01f);
			uniformName = "gSpotLights[" + pD + "].Base.Position";
			shader->SetVectorUniform(uniformName, mSpotLight.Position);
			uniformName = "gSpotLights[" + pD + "].Direction";
			shader->SetVectorUniform(uniformName, mSpotLight.Direction);
			uniformName = "gSpotLights[" + pD + "].Cutoff";
			shader->SetFloatUniform(uniformName, cosf(20.0f * M_PI / 180.0f));
			uniformName = "gSpotLights[" + pD + "].Base.Base.DiffuseIntensity";
			shader->SetFloatUniform(uniformName, 0.05f);
			uniformName = "gSpotLights[" + pD + "].Base.Atten.Constant";
			uniformName = "gSpotLights[" + pD + "].Base.Atten.Linear";
			shader->SetFloatUniform(uniformName, 0.01f);
			uniformName = "gSpotLights[" + pD + "].Base.Atten.Exp";
		}

		 // directional light
		{
			std::string uniformName;
			uniformName = "gDirectionalLight.Base.Color";
			shader->SetVectorUniform(uniformName, glm::vec3(1.0f, 1.0f, 1.0f));
			uniformName = "gDirectionalLight.Base.AmbientIntensity";
			shader->SetFloatUniform(uniformName, 0.1f);
			uniformName = "gDirectionalLight.Base.DiffuseIntensity";
			shader->SetFloatUniform(uniformName, 0.9f);
			uniformName = "gDirectionalLight.Direction";
			shader->SetVectorUniform(uniformName, mSpotLight.Direction);
		}
	}
}

bool Game::LoadData()
{
	glm::mat4 CameraView = glm::lookAt(
		mCameraPos,
		mCameraPos + mCameraOrientation,
		mCameraUP);
	glm::mat4 CameraProj = glm::perspective(glm::radians(45.0f), (float)mWindowWidth / mWindowHeight, 0.1f, 100.0f);
	glm::mat4 SpotLightView = glm::lookAt(
		mSpotLight.Position,
		mSpotLight.Direction,
		mSpotLight.Up
	);


	// Shader読み込み処理
	// 普通のMesh
	{
		// Shadow Map
		std::string vert_file = "./Shaders/ShadowMap.vert";
		std::string frag_file = "./Shaders/ShadowMap.frag";
		mShadowMapShader = new Shader();
		if (!mShadowMapShader->CreateShaderProgram(vert_file, frag_file)) {
			return false;
		}
	}
	mShadowMapShader->UseProgram();
	mShadowMapShader->SetMatrixUniform("LightView", SpotLightView);
	mShadowMapShader->SetMatrixUniform("CameraProj", CameraProj);
	mShadowMapShader->SetSamplerUniform("gShadowMap", 1);

	{
		// Shadow Lighting
		std::string vert_file = "./Shaders/ShadowLighting.vert";
		std::string frag_file = "./Shaders/ShadowLighting.frag";
		mShadowLightingShader = new Shader();
		if (!mShadowLightingShader->CreateShaderProgram(vert_file, frag_file)) {
			return false;
		}
	}
	mShadowLightingShader->UseProgram();
	mShadowLightingShader->SetMatrixUniform("CameraView", CameraView);
	mShadowLightingShader->SetMatrixUniform("CameraProj", CameraProj);
	mShadowLightingShader->SetMatrixUniform("LightView", SpotLightView);
	mShadowLightingShader->SetSamplerUniform("gShadowMap", 1);

	// SkinMesh
	{
		// Shadow Map
		std::string vert_file = "./Shaders/SkinningShadowMap.vert";
		std::string frag_file = "./Shaders/ShadowMap.frag";
		mSkinShadowMapShader = new Shader();
		if (!mSkinShadowMapShader->CreateShaderProgram(vert_file, frag_file)) {
			return false;
		}
	}
	mSkinShadowMapShader->UseProgram();
	mSkinShadowMapShader->SetMatrixUniform("LightView", SpotLightView);
	mSkinShadowMapShader->SetMatrixUniform("CameraProj", CameraProj);
	mSkinShadowMapShader->SetSamplerUniform("gShadowMap", 1);

	{
		// Shadow Lighting
		std::string vert_file = "./Shaders/SkinningShadowLighting.vert";
		std::string frag_file = "./Shaders/ShadowLighting.frag";
		mSkinShadowLightingShader = new Shader();
		if (!mSkinShadowLightingShader->CreateShaderProgram(vert_file, frag_file)) {
			return false;
		}
	}
	mSkinShadowLightingShader->UseProgram();
	mSkinShadowLightingShader->SetMatrixUniform("CameraView", CameraView);
	mSkinShadowLightingShader->SetMatrixUniform("CameraProj", CameraProj);
	mSkinShadowLightingShader->SetMatrixUniform("LightView", SpotLightView);
	mSkinShadowLightingShader->SetSamplerUniform("gShadowMap", 1);

	// light setting
	SetShaderLighting();


	// Model読み込み処理
	{
		// Treasure Box
		Mesh* mesh = new Mesh();
		if (mesh->Load("./resources/TreasureBox3/", "scene.gltf")) {
			mesh->SetMeshPos(glm::vec3(4.0f, 5.0f/2.0f, 0.0f));
			mesh->SetMeshRotate(glm::mat4(1.0f));
			mesh->SetMeshScale(0.01f / 2.0f);
			mMeshes.push_back(MeshData(mesh, true));
		}
	}
	// Unity Chan world
	{
		Mesh* mesh = new Mesh();
		if (mesh->Load("./resources/world/", "terrain.fbx")) {
			mesh->SetMeshPos(glm::vec3(0.0f));
			glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), (float)M_PI / 2.0f, glm::vec3(1.0f, 0.0f, 0.0f));
			//mesh->SetMeshRotate(rotate);
			mesh->SetMeshRotate(glm::mat4(1.0f));
			mesh->SetMeshScale(1.0f);
			mMeshes.push_back(MeshData(mesh, false));
		}
	}
	// Treasure Chest(Move)
	{
		// Treasure Box
		SkinMesh* mesh = new SkinMesh();
		if (mesh->Load("./resources/TreasureBox3/", "scene.gltf")) {
			mesh->SetMeshPos(glm::vec3(4.0f, 4.0f, 0.0f));
			glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), (float)M_PI, glm::vec3(1.0f, 0.0f, 0.0f));
			mesh->SetMeshRotate(rotate);
			mesh->SetMeshScale(0.01f / 2.0f);
			mSkinMeshes.push_back(mesh);
		}
	}
	// Load Unity Chan!!
	{
		// Treasure Box
		SkinMesh* mesh = new SkinMesh();
		if (mesh->Load("./resources/UnityChan/", "unitychan.fbx")) {
			mesh->SetMeshPos(glm::vec3(6.0f, 4.0f, 0.0f));
			glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), (float)M_PI, glm::vec3(1.0f, 0.0f, 0.0f));
			mesh->SetMeshRotate(rotate);
			mesh->SetMeshScale(0.1f);
			mSkinMeshes.push_back(mesh);
		}
	}


	// Load ShadowMap FBO
	mTextureShadowMapFBO = new TextureShadowMap();
	if (!mTextureShadowMapFBO->Load(mWindowWidth, mWindowHeight)) {
		printf("error: Failed to load shadwo map fbo\n");
		return false;
	}

	return true;
}


void Game::ProcessInput()
{
	SDL_Point mouse_position = { mWindowWidth / 2, mWindowHeight / 2 };
	SDL_GetMouseState(&mouse_position.x, &mouse_position.y);
	mMousePos.x = mouse_position.x;
	mMousePos.y = mouse_position.y;
	//printf("%d %d\n", mMousePos.x, mMousePos.y);

	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_QUIT:
			mIsRunning = false;
			break;
		case SDL_MOUSEBUTTONDOWN:	// マウスのボタンが押されたら
		{
			if (mPhase == PHASE_IDLE) {
				//mLastMousePos = mMousePos;
				//mSwipingDropPos = mMousePos / GRID_SIZE;
				mPhase = PHASE_MOVE;

				SDL_WarpMouseInWindow(mWindow, mWindowWidth / 2, mWindowHeight / 2);
				mMousePos.x = mWindowWidth / 2;
				mMousePos.y = mWindowHeight / 2;
				SDL_ShowCursor(SDL_DISABLE);
				//std::cout << "----------------------------------------------called\n";
			}
		}
		break;
		case SDL_MOUSEBUTTONUP:		// マウスを離したら
			if (mPhase == PHASE_MOVE) {
				mPhase = PHASE_IDLE;

				/*if (EraseDrops()) {
					phase = PHASE_ERASE;
				}
				else {
					phase = PHASE_IDLE;
				}*/
				SDL_ShowCursor(SDL_ENABLE);
			}
			break;
		}
	}

	const Uint8* keyState = SDL_GetKeyboardState(NULL);
	if (keyState[SDL_SCANCODE_ESCAPE] || keyState[SDL_SCANCODE_Q])	// escapeキーを押下すると終了
	{
		mIsRunning = false;
	}

	if (keyState[SDL_SCANCODE_W]) {
		mCameraPos += (float)mMoveSpeed * mCameraOrientation;
	}
	if (keyState[SDL_SCANCODE_S]) {
		mCameraPos -= (float)mMoveSpeed * mCameraOrientation;
	}
	if (keyState[SDL_SCANCODE_A]) {
		mCameraPos -= (float)mMoveSpeed * glm::normalize(glm::cross(mCameraOrientation, mCameraUP));
	}
	if (keyState[SDL_SCANCODE_D]) {
		mCameraPos += (float)mMoveSpeed * glm::normalize(glm::cross(mCameraOrientation, mCameraUP));
	}
}


void Game::UpdateGame()
{
	while (!SDL_TICKS_PASSED(SDL_GetTicks(), mTicksCount + 16))
		;

	float deltaTime = (SDL_GetTicks() - mTicksCount) / 1000.0f;
	if (deltaTime > 0.05f)
	{
		deltaTime = 0.05f;
	}

	mTicksCount = SDL_GetTicks();

	if (mPhase == PHASE_MOVE) {
		//printf("%d %d\n", mMousePos.x, mMousePos.y);

		float rotX = mMoveSensitivity * (float)((float)mMousePos.y - ((float)mWindowHeight / 2.0f)) / (float)mWindowHeight;
		float rotY = mMoveSensitivity * (float)((float)mMousePos.x - ((float)mWindowWidth / 2.0f)) / (float)mWindowWidth;
		//printf("rotX: %f rotY: %f\t", rotX, rotY);
		// Calculates upcoming vertical change in the Orientation
		glm::vec3 newOrientation = glm::rotate(mCameraOrientation, glm::radians(-rotX), glm::normalize(glm::cross(mCameraOrientation, mCameraUP)));

		// Decides whether or not the next vertical Orientation is legal or not
		int rad = abs(glm::angle(newOrientation, mCameraUP) - glm::radians(90.0f));
		//std::cout << rad * 180 / M_PI << std::endl;
		if (abs(glm::angle(newOrientation, mCameraUP) - glm::radians(90.0f)) <= glm::radians(85.0f))
		{
			mCameraOrientation = newOrientation;
		}

		// Rotates the Orientation left and right
		mCameraOrientation = glm::rotate(mCameraOrientation, glm::radians(-rotY), mCameraUP);

		if ((mMousePos.x != mWindowWidth / 2) || (mMousePos.y != mWindowHeight / 2)) {
			SDL_WarpMouseInWindow(mWindow, mWindowWidth / 2, mWindowHeight / 2);
		}
	}

	// 更新されたカメラの位置をShaderに反映
	std::vector<Shader*> Shaders;
	Shaders.push_back(mShadowLightingShader);
	Shaders.push_back(mSkinShadowLightingShader);
	for (auto shader : Shaders) {
		shader->UseProgram();
		shader->SetVectorUniform("gEyeWorldPos", mCameraPos);
		shader->SetMatrixUniform("CameraView", glm::lookAt(mCameraPos, mCameraPos + mCameraOrientation, mCameraUP));
	}



}

void Game::Draw()
{
	// Frame Bufferに描画
	mTextureShadowMapFBO->WriteBind();

	glClear(GL_DEPTH_BUFFER_BIT);

	mShadowMapShader->UseProgram();

	//{
	//	// Spot LightのView Projectionを設定
	//	glm::mat4 projection = glm::perspective(glm::radians(20.0f), (float)mWindowWidth / mWindowHeight, 0.1f, 100.0f);
	//	glm::mat4 view = glm::lookAt(
	//		mSpotLight.Position,
	//		mSpotLight.Direction,
	//		mSpotLight.Up
	//	);
	//	mShadowMapShader->SetMatrixUniform("LightView", view);
	//	mShadowMapShader->SetMatrixUniform("LightProj", projection);
	//}
	for (auto mesh : mMeshes) {
		if (mesh.IsShadow) {
			mesh.mesh->Draw(mShadowMapShader, mTicksCount / 1000.0f);
		}
	}
	for (auto skinmesh : mSkinMeshes) {
		skinmesh->Draw(mSkinShadowMapShader, mTicksCount / 1000.0f);
	}


	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// ---------------------------------
	glClearColor(0, 0.5, 0.7, 1.0f);
	// Clear the color buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	mShadowLightingShader->UseProgram();
	mTextureShadowMapFBO->BindTexture(GL_TEXTURE1);
	for (auto mesh : mMeshes) {
		mesh.mesh->Draw(mShadowLightingShader, mTicksCount / 1000.0f);
	}
	for (auto skinmesh : mSkinMeshes) {
		skinmesh->Draw(mSkinShadowLightingShader, mTicksCount / 1000.0f);
	}


	mShadowLightingShader->UseProgram();


	SDL_GL_SwapWindow(mWindow);
}

void Game::RunLoop()
{
	while (mIsRunning)
	{
		ProcessInput();
		UpdateGame();
		Draw();
	}
}

void Game::Shutdown()
{

}
