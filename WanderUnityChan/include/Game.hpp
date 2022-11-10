#pragma once

#include "glad/glad.h"
#include "SDL.h"
//#include "glew.h"
#include "TextureShadowMap.hpp"
#include "Skinning.hpp"
#include "UnityChan.hpp"
#include <map>

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
	struct SpotLight {
		glm::vec3 Position;
		glm::vec3 Direction;
		glm::vec3 Up;
	};

	void ProcessInput();
	void UpdateGame();
	void Draw();

	bool LoadData();
	void SetShaderLighting();

	const int mWindowWidth;
	const int mWindowHeight;
	bool mIsRunning;

	SpotLight mSpotLight;

	struct MeshData {
		MeshData(Mesh* mesh, bool IsShadow)
			:mesh(mesh),
			IsShadow(IsShadow)
		{
		}
		Mesh* mesh;
		bool IsShadow;
	};
	std::vector<MeshData> mMeshes;
	std::vector<SkinMesh*> mSkinMeshes;
	class UnityChan* mUnityChan;
	SkinMesh* mRunAnim;
	UnityChan* mAnimUnityChan;

	TextureShadowMap* mTextureShadowMapFBO;

	//Shader* mShadowMapShader;
	//Shader* mShadowLightingShader;
	//Shader* mSkinShadowMapShader;
	//Shader* mSkinShadowLightingShader;
	//Shader* mUnityChanShader;

	std::map<std::string, class Shader*> mShaders;

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