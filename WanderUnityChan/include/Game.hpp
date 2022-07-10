#pragma once

#include "SDL.h"
#include "glew.h"

class Game {
public:
	Game();
	~Game() {}

	bool Initialize();
	void RunLoop();
	void Shutdown();

private:
	void ProcessInput();
	void UpdateGame();
	void Draw();

	const int mWindowWidth;
	const int mWindowHeight;
	bool mIsRunning;

	SDL_Window* mWindow;
	// OpenGL context
	SDL_GLContext mContext;
	Uint32 mTicksCount;
};