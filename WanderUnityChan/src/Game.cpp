#define _CRT_SECURE_NO_WARNINGS


#include "Game.hpp"

Game::Game()
	:mWindowWidth(1024),
	mWindowHeight(768),
	mIsRunning(true)
{

}


bool Game::Initialize()
{

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
	{
		SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
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
		SDL_Log("Failed to create window: %s", SDL_GetError());
		return false;
	}

	// Create an OpenGL context
	mContext = SDL_GL_CreateContext(mWindow);

	// Initialize GLEW
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		SDL_Log("Failed to initialize GLEW.");
		return false;
	}

	auto error_code = glGetError();

	//if (!LoadShaders())
	//{
	//	SDL_Log("Failed to load shaders.");
	//	return false;
	//}


	////CreateSpriteVerts();

	//if (!LoadData())
	//{
	//	SDL_Log("Failed to load data.");
	//	return false;
	//}



	mTicksCount = SDL_GetTicks();

	return true;
}


void Game::ProcessInput()
{
	SDL_Point mouse_position = { mWindowWidth / 2, mWindowHeight / 2 };
	SDL_GetMouseState(&mouse_position.x, &mouse_position.y);
	//mMousePos.x = mouse_position.x;
	//mMousePos.y = mouse_position.y;
	//printf("%d %d\n", mMousePos.x, mMousePos.y);

	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_QUIT:
			mIsRunning = false;
			break;
		//case SDL_MOUSEBUTTONDOWN:	// マウスのボタンが押されたら
		//{
		//	if (mPhase == PHASE_IDLE) {
		//		//mLastMousePos = mMousePos;
		//		//mSwipingDropPos = mMousePos / GRID_SIZE;
		//		mPhase = PHASE_MOVE;

		//		SDL_WarpMouseInWindow(mWindow, mWindowWidth / 2, mWindowHeight / 2);
		//		mMousePos.x = mWindowWidth / 2;
		//		mMousePos.y = mWindowHeight / 2;
		//		SDL_ShowCursor(SDL_DISABLE);
		//		//std::cout << "----------------------------------------------called\n";
		//	}
		//}
		//break;
		//case SDL_MOUSEBUTTONUP:		// マウスを離したら
		//	if (mPhase == PHASE_MOVE) {
		//		mPhase = PHASE_IDLE;

		//		/*if (EraseDrops()) {
		//			phase = PHASE_ERASE;
		//		}
		//		else {
		//			phase = PHASE_IDLE;
		//		}*/
		//		SDL_ShowCursor(SDL_ENABLE);
		//	}
		//	break;
		}
	}

	const Uint8* keyState = SDL_GetKeyboardState(NULL);
	if (keyState[SDL_SCANCODE_ESCAPE] || keyState[SDL_SCANCODE_Q])	// escapeキーを押下すると終了
	{
		mIsRunning = false;
	}

	//if (keyState[SDL_SCANCODE_W]) {
	//	mCameraPos += (float)mMoveSpeed * mCameraOrientation;
	//}
	//if (keyState[SDL_SCANCODE_S]) {
	//	mCameraPos -= (float)mMoveSpeed * mCameraOrientation;
	//}
	//if (keyState[SDL_SCANCODE_A]) {
	//	mCameraPos -= (float)mMoveSpeed * glm::normalize(glm::cross(mCameraOrientation, mCameraUP));
	//}
	//if (keyState[SDL_SCANCODE_D]) {
	//	mCameraPos += (float)mMoveSpeed * glm::normalize(glm::cross(mCameraOrientation, mCameraUP));
	//}
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
