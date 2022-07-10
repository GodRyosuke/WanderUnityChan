//#pragma once

#include "SDL.h"
#include "glew.h"
#include "Texture.hpp"
#include "Mesh.hpp"

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

	bool LoadData();

	const int mWindowWidth;
	const int mWindowHeight;
	bool mIsRunning;

	std::vector<Mesh*> mMeshes;

	SDL_Window* mWindow;
	// OpenGL context
	SDL_GLContext mContext;
	Uint32 mTicksCount;
};