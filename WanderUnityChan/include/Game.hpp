//#pragma once

#include "SDL.h"
#include "glew.h"
#include "Texture.hpp"
#include "Mesh.hpp"
#include "Shader.hpp"

class Game {
public:
	Game();
	~Game() {}

	bool Initialize();
	void RunLoop();
	void Shutdown();

private:
	enum PHASE {
		PHASE_IDLE,
		PHASE_MOVE,
		PHASE_MAX
	};
	void ProcessInput();
	void UpdateGame();
	void Draw();

	bool LoadData();
	void SetShaderLighting();

	const int mWindowWidth;
	const int mWindowHeight;
	bool mIsRunning;

	std::vector<Mesh*> mMeshes;

	Shader* mShadowMapShader;
	Shader* mShadowLightingShader;

	PHASE mPhase;

	// Camera
	glm::vec3 mCameraPos;
	glm::vec3 mCameraUP;
	glm::vec3 mCameraOrientation;
	float mMoveSpeed;
	float mMoveSensitivity;

	glm::vec3 mMousePos;

	SDL_Window* mWindow;
	// OpenGL context
	SDL_GLContext mContext;
	Uint32 mTicksCount;
};