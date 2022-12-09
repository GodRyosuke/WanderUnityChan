#define _CRT_SECURE_NO_WARNINGS

#include "Game.hpp"
#include "deFBXMesh.hpp"
#include "Shader.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtc/type_ptr.hpp"
#include "gtx/rotate_vector.hpp"
#include "gtx/vector_angle.hpp"
#include "UnityChan.hpp"
#include "Actor.hpp"
#include "ActorUnityChan.hpp"
#include "MeshCompoinent.hpp"
#include "wMesh.hpp"

#define STB_IMAGE_IMPLEMENTATION

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
	gladLoadGL();
	//glewExperimental = GL_TRUE;
	//if (glewInit() != GLEW_OK)
	//{
	//	printf("Failed to initialize GLEW.");
	//	return false;
	//}

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
	Shaders.push_back(mShaders["ShadowLighting"]);
	Shaders.push_back(mShaders["SkinShadowLighting"]);
	Shaders.push_back(mShaders["UnityChanShader"]);

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


	// Shader�ǂݍ��ݏ���
	// ���ʂ�Mesh
	Shader* shader = nullptr;
	{
		// Shadow Map
		std::string vert_file = "./Shaders/ShadowMap.vert";
		std::string frag_file = "./Shaders/ShadowMap.frag";
		shader = new Shader();
		if (!shader->CreateShaderProgram(vert_file, frag_file)) {
			return false;
		}
		mShaders.emplace("ShadowMap", shader);
	}

	{
		// Shadow Lighting
		std::string vert_file = "./Shaders/ShadowLighting.vert";
		std::string frag_file = "./Shaders/ShadowLighting.frag";
		shader = new Shader();
		if (!shader->CreateShaderProgram(vert_file, frag_file)) {
			return false;
		}
		mShaders.emplace("ShadowLighting", shader);
	}

	// SkinMesh
	{
		// Shadow Map
		std::string vert_file = "./Shaders/SkinningShadowMap.vert";
		std::string frag_file = "./Shaders/ShadowMap.frag";
		shader = new Shader();
		if (!shader->CreateShaderProgram(vert_file, frag_file)) {
			return false;
		}
		mShaders.emplace("SkinShadowMap", shader);
	}

	{
		// Shadow Lighting
		std::string vert_file = "./Shaders/SkinningShadowLighting.vert";
		std::string frag_file = "./Shaders/ShadowLighting.frag";
		shader = new Shader();
		if (!shader->CreateShaderProgram(vert_file, frag_file)) {
			return false;
		}
		mShaders.emplace("SkinShadowLighting", shader);
	}

	// Unity Chan Shadow Lighting 
	{
		// Shadow Lighting
		std::string vert_file = "./Shaders/SkinningShadowLighting.vert";
		std::string frag_file = "./Shaders/UnityChan.frag";
		shader = new Shader();
		if (!shader->CreateShaderProgram(vert_file, frag_file)) {
			return false;
		}
		mShaders.emplace("UnityChanShader", shader);
	}

	// Load TestShader
	{
		// Shadow Lighting
		std::string vert_file = "./Shaders/test.vert";
		std::string frag_file = "./Shaders/test.frag";
		shader = new Shader();
		if (!shader->CreateShaderProgram(vert_file, frag_file)) {
			return false;
		}
		mShaders.emplace("TestShader", shader);
	}

	// View Projection Matrixを設定
	for (auto iter : mShaders) {
		Shader* shader = iter.second;
		shader->UseProgram();
		shader->SetMatrixUniform("CameraView", CameraView);
		shader->SetMatrixUniform("CameraProj", CameraProj);
		shader->SetMatrixUniform("LightView", SpotLightView);
		shader->SetSamplerUniform("gShadowMap", 1);
	}

	// light setting
	SetShaderLighting();


	// Load Models
    Actor* a = nullptr;
    // UnityChan Loader改良版
    a = new ActorUnityChan(this);


	{
		// Treasure Box
		Mesh* mesh = new Mesh();
        if (mesh->Load("./resources/UnityChan/", "UnityChan_fbx7binary.fbx")) {
        //if (mesh->Load("./resources/UnityChan/", "unitychan_RUN00_F.fbx")) {
        //if (mesh->Load("./resources/SchoolDesk/", "SchoolDesk.fbx")) {
			mesh->SetMeshPos(glm::vec3(4.0f, 5.0f / 2.0f, 0.0f));
			mesh->SetMeshRotate(glm::mat4(1.0f));
			mesh->SetMeshScale(0.01f / 2.0f);
			mdeMeshes.push_back(MeshData(mesh, true));
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
			mdeMeshes.push_back(MeshData(mesh, false));
		}
	}

    // Treasure Chest(Move)
    //{
    //    // Treasure Box
    //    SkinMesh* mesh = new SkinMesh();
    //    if (mesh->Load("./resources/TreasureBox3/", "scene.gltf")) {
    //        mesh->SetMeshPos(glm::vec3(4.0f, 4.0f, 0.0f));
    //        glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), (float)M_PI, glm::vec3(1.0f, 0.0f, 0.0f));
    //        mesh->SetMeshRotate(rotate);
    //        mesh->SetMeshScale(0.01f / 2.0f);
    //        mSkinMeshes.push_back(mesh);
    //    }
    //}

    // Unity Chan Skin 
    {
        // Treasure Box
        SkinMesh* mesh = new SkinMesh();
        //if (mesh->Load("./resources/UnityChan/", "unitychan_RUN00_F_fbx7binary.fbx")) {
        if (mesh->Load("./resources/UnityChan/", "UnityChan_fbx7binary.fbx")) {
        //if (mesh->Load("./resources/UnityChan/", "running.fbx")) {
            mesh->SetMeshPos(glm::vec3(4.0f, 4.0f, 0.0f));
            glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), (float)M_PI / 2.f, glm::vec3(1.0f, 0.0f, 0.0f));
            mesh->SetMeshRotate(rotate);
            mesh->SetMeshScale(0.01f / 2.0f);
            mSkinMeshes.push_back(mesh);
        }
    }



	// Bob mesh clean
	{
		// Treasure Box
		//SkinMesh* mesh = new SkinMesh();
		//if (mesh->Load("./resources/boblampclean/", "boblampclean.md5mesh")) {
		//	mesh->SetMeshPos(glm::vec3(4.0f, 1.5, 0.0f));
		//	glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), (float)M_PI, glm::vec3(1.0f, 0.0f, 0.0f));
		//	mesh->SetMeshRotate(glm::mat4(1.0f));
		//	mesh->SetMeshScale(0.01f);
		//	mSkinMeshes.push_back(mesh);
		//}
	}


	// Load Unity Chan!!
	//{
	//	// Treasure Box
	//	SkinMesh* mesh = new SkinMesh();
	//	if (mesh->Load("./resources/UnityChan/", "unitychansetup.fbx")) {
	//		mesh->SetMeshPos(glm::vec3(6.0f, 4.0f, 0.0f));
	//		glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), (float)M_PI, glm::vec3(1.0f, 0.0f, 0.0f));
	//		mesh->SetMeshRotate(rotate);
	//		mesh->SetMeshScale(0.1f);
	//		mSkinMeshes.push_back(mesh);
	//	}
	//}
	//{
	//	// Unity Chan
	//	Mesh* mesh = new Mesh();
	//	if (mesh->Load("./resources/UnityChan/", "unitychan2.fbx")) {
	//		mesh->SetMeshPos(glm::vec3(6.0f, 4.0f, 0.0f));
	//		glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), (float)M_PI / 2.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	//		mesh->SetMeshRotate(rotate);
	//		mesh->SetMeshScale(0.01);
	//		mUnityChan = mesh;
	//	}
	//}

	//{
	//	// Running Animation
	//	SkinMesh* mesh = new SkinMesh();
	//	if (mesh->Load("./resources/UnityChan/", "running.fbx")) {
	//		mesh->SetMeshPos(glm::vec3(6.0f, 4.0f, 0.0f));
	//		glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), (float)M_PI / 2.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	//		mesh->SetMeshRotate(rotate);
	//		mesh->SetMeshScale(0.01);
	//		mRunAnim = mesh;
	//	}
	//}

	{
		//UnityChan* unitychan = new UnityChan();
		//std::vector<std::string> animFillePaths;
		//animFillePaths.push_back("unitychan_RUN00_F.fbx");
		//if (unitychan->Load("./resources/UnityChan/", "unitychan.fbx", animFillePaths)) {
		//	unitychan->SetMeshPos(glm::vec3(6.0f, 4.0f, 0.0f));
		//	glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), (float)M_PI / 2.0f, glm::vec3(1.0f, 0.0f, 0.0f));
		//	unitychan->SetMeshRotate(rotate);
		//	unitychan->SetMeshScale(0.01);
		//	mAnimUnityChan = unitychan;
		//}
		//else {
		//	delete unitychan;
		//	mAnimUnityChan = nullptr;
		//	return false;
		//}
	}

	// FBXSDKを使ってUnityChanを読み込む
	mUnityChan = new UnityChan(this);



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
		case SDL_MOUSEBUTTONDOWN:	// �}�E�X�̃{�^���������ꂽ��
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
		case SDL_MOUSEBUTTONUP:		// �}�E�X�𗣂�����
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
	if (keyState[SDL_SCANCODE_ESCAPE] || keyState[SDL_SCANCODE_Q])	// escape�L�[����������ƏI��
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

    for (auto actor : mActors)
    {
        actor->ProcessInput(keyState);
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

	mUnityChan->Update(deltaTime);
    for (auto actor : mActors) {
        actor->Update(deltaTime);
    }
    for (auto skinMesh : mSkinMeshes) {
        skinMesh->Update(mTicksCount / 1000.0f);
    }

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

	// �X�V���ꂽ�J�����̈ʒu��Shader�ɔ��f
	std::vector<Shader*> Shaders;
	Shaders.push_back(mShaders["ShadowLighting"]);
	Shaders.push_back(mShaders["SkinShadowLighting"]);
	Shaders.push_back(mShaders["UnityChanShader"]);
	Shaders.push_back(mShaders["TestShader"]);
	for (auto shader : Shaders) {
		shader->UseProgram();
		shader->SetVectorUniform("gEyeWorldPos", mCameraPos);
		shader->SetMatrixUniform("CameraView", glm::lookAt(mCameraPos, mCameraPos + mCameraOrientation, mCameraUP));
	}



}

void Game::TestDraw()
{
	glClearColor(0, 0.5, 0.7, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	mUnityChan->Draw(nullptr);
	SDL_GL_SwapWindow(mWindow);
}

void Game::Draw()
{
	// Frame Buffer�ɕ`��
	mTextureShadowMapFBO->WriteBind();

	glClear(GL_DEPTH_BUFFER_BIT);

	mShaders["ShadowMap"]->UseProgram();

	//{
	//	// Spot Light��View Projection��ݒ�
	//	glm::mat4 projection = glm::perspective(glm::radians(20.0f), (float)mWindowWidth / mWindowHeight, 0.1f, 100.0f);
	//	glm::mat4 view = glm::lookAt(
	//		mSpotLight.Position,
	//		mSpotLight.Direction,
	//		mSpotLight.Up
	//	);
	//	mShadowMapShader->SetMatrixUniform("LightView", view);
	//	mShadowMapShader->SetMatrixUniform("LightProj", projection);
	//}
	for (auto mesh : mdeMeshes) {
		if (mesh.IsShadow) {
			mesh.mesh->Draw(mShaders["ShadowMap"], mTicksCount / 1000.0f);
		}
	}
	for (auto skinmesh : mSkinMeshes) {
		skinmesh->Draw(mShaders["SkinShadowMap"], mTicksCount / 1000.0f);
	}

	//mUnityChan->Draw(mShadowMapShader);
	//mAnimUnityChan->Draw(mSkinShadowMapShader, mTicksCount / 1000.0f);


	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// ---------------------------------
	glEnable(GL_DEPTH_TEST);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glClearColor(0, 0.5, 0.7, 1.0f);
	// Clear the color buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	mShaders["ShadowLighting"]->UseProgram();
	mTextureShadowMapFBO->BindTexture(GL_TEXTURE1);
	for (auto mesh : mdeMeshes) {
		mesh.mesh->Draw(mShaders["ShadowLighting"], mTicksCount / 1000.0f);
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	for (auto skinmesh : mSkinMeshes) {
        //skinmesh->Draw(mShaders["SkinShadowLighting"], mTicksCount / 1000.0f);
        skinmesh->Draw(mShaders["TestShader"], mTicksCount / 1000.0f);
	}
    for (auto mc : mMeshComps) {
        mc->Draw(mShaders["TestShader"]);
    }
	mUnityChan->Draw(mShaders["TestShader"]);
	//mAnimUnityChan->Draw(mUnityChanShader, mTicksCount / 1000.0f);
    //for (auto meshcomp : mMeshComps) {
    //    meshcomp->Draw(mShaders["TestShader"]);
    //}



	SDL_GL_SwapWindow(mWindow);
}

void Game::RunLoop()
{
	while (mIsRunning)
	{
		ProcessInput();
		UpdateGame();
		//TestDraw();
		Draw();
	}
}

void Game::Shutdown()
{

}


wMesh* Game::GetMesh(std::string fileName)
{
    wMesh* m = nullptr;
    auto iter = mMeshes.find(fileName);
    if (iter != mMeshes.end())
    {
        m = iter->second;
    }
    else
    {
        m = new wMesh();
        if (m->Load(fileName))
        {
            mMeshes.emplace(fileName, m);
        }
        else
        {
            delete m;
            m = nullptr;
        }
    }
    return m;
}

void Game::AddMeshComp(MeshComponent* meshcomp)
{
    if (meshcomp->GetIsSkeletal()) {
        //SkinMeshComponent* skin = static_cast<SkinMeshComponent*>(meshcomp);
        //mSkinMeshComps.push_back(skin);
    }
    else {
        mMeshComps.push_back(meshcomp);
    }
}

void Game::RemoveActor(Actor* actor)
{
    // Is it in actors?
    auto iter = std::find(mActors.begin(), mActors.end(), actor);
    if (iter != mActors.end())
    {
        // Swap to end of vector and pop off (avoid erase copies)
        std::iter_swap(iter, mActors.end() - 1);
        mActors.pop_back();
    }
}

void Game::RemoveMeshComp(MeshComponent* meshcomp)
{
    if (meshcomp->GetIsSkeletal())
    {
        //SkinMeshComponent* sk = static_cast<SkinMeshComponent*>(meshcomp);
        //auto iter = std::find(mSkinMeshComps.begin(), mSkinMeshComps.end(), sk);
        //mSkinMeshComps.erase(iter);
    }
    else
    {
        auto iter = std::find(mMeshComps.begin(), mMeshComps.end(), meshcomp);
        mMeshComps.erase(iter);
    }
}
