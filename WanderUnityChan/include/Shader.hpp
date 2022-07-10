#pragma once

#include "glew.h"
#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtc/type_ptr.hpp"
#include <string>

class Shader {
public:
	Shader();
	bool CreateShaderProgram(std::string vertFilePath, std::string fragFilePath);
	void UseProgram();
	void SetMatrixUniform(std::string uniformName, glm::mat4 mat);
	void SetVectorUniform(std::string uniformName, glm::vec3 vec);
	void SetFloatUniform(std::string uniformName, float data);
	void SetSamplerUniform(std::string uniformName, GLuint tex);

private:
	GLuint mShaderProgram;

	int mNumIndices;
};