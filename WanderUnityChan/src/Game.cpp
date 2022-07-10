#define _CRT_SECURE_NO_WARNINGS


#include "Game.hpp"

Game::Game()
	:mWindowWidth(1024),
	mWindowHeight(768),
	mIsRunning(true),
	mMoveSpeed(0.1),
	mMoveSensitivity(100.0f)
{
	mCameraPos = glm::vec3(-1.0f, 2.5f, 0.0f);
	mCameraOrientation = glm::vec3(0, 0.5f, 0);
	mCameraUP = glm::vec3(0.0, 0.0f, 1.0f);
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
	//Shaders.push_back(mSkinningShaderProgram);

	for (auto shader : Shaders) {
		shader->UseProgram();
		shader->SetVectorUniform("gEyeWorldPos", mCameraPos);
		shader->SetSamplerUniform("gShadowMap", 1);
		shader->SetSamplerUniform("gSampler", 0);
		shader->SetMatrixUniform("CameraView", glm::lookAt(mCameraPos, mCameraPos + mCameraOrientation, mCameraUP));
		shader->SetSamplerUniform("gNumSpotLights", 1);

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
	}
}

bool Game::LoadData()
{
	glm::mat4 CameraView = glm::lookAt(
		mCameraPos,
		mCameraPos + mCameraOrientation,
		mCameraUP);
	glm::mat4 CameraProj = glm::perspective(glm::radians(45.0f), (float)mWindowWidth / mWindowHeight, 0.1f, 100.0f);



	// Shader読み込み処理
	{
		// Shadow Map
		std::string vert_file = "./Shaders/Phong.vert";
		std::string frag_file = "./Shaders/ShadowMap.frag";
		mShadowMapShader = new Shader();
		if (!mShadowMapShader->CreateShaderProgram(vert_file, frag_file)) {
			return false;
		}
	}
	mShadowMapShader->UseProgram();
	mShadowMapShader->SetMatrixUniform("CameraView", CameraView);
	mShadowMapShader->SetMatrixUniform("CameraProj", CameraProj);

	{
		// Shadow Lighting
		std::string vert_file = "./Shaders/Phong.vert";
		std::string frag_file = "./Shaders/ShadowLighting.frag";
		mShadowLightingShader = new Shader();
		if (!mShadowLightingShader->CreateShaderProgram(vert_file, frag_file)) {
			return false;
		}
	}
	mShadowLightingShader->UseProgram();
	mShadowLightingShader->SetMatrixUniform("CameraView", CameraView);
	mShadowLightingShader->SetMatrixUniform("CameraProj", CameraProj);
	// light setting
	{
		glm::mat4 projection = glm::perspective(glm::radians(20.0f), (float)mWindowWidth / mWindowHeight, 0.1f, 100.0f);
		glm::mat4 view = glm::lookAt(
			glm::vec3(2.5f, 2.5f, 10.0f),	// position
			glm::vec3(0.0f, 0.0f, -1.0f),	// direction
			glm::vec3(0.0f, 0.0f, 1.0f)		// up
		);
		mShadowLightingShader->SetMatrixUniform("LightView", view);
		mShadowLightingShader->SetMatrixUniform("LightProj", projection);
	}



	// Model読み込み処理
	{
		// Treasure Box
		Mesh* mesh = new Mesh();
		if (mesh->Load("./resources/TreasureBox3/", "scene.gltf")) {
			mesh->SetMeshPos(glm::vec3(5.0f / 2.0f, 35.0f, 0.0f));
			mesh->SetMeshRotate(glm::mat4(1.0f));
			mesh->SetMeshScale(0.01f / 2.0f);
			mMeshes.push_back(mesh);
		}
	}
	// Unity Chan world


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
}

void Game::Draw()
{
	glClearColor(0, 0.5, 0.7, 1.0f);
	// Clear the color buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

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
