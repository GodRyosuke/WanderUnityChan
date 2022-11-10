#include "Shader.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

Shader::Shader()
{

}


static bool CompileShader(std::string filepath, GLenum shaderType, GLuint& shader)
{
	std::ifstream shaderFile(filepath);
	if (shaderFile.is_open())
	{
		// Read all the text into a string
		std::stringstream sstream;
		sstream << shaderFile.rdbuf();
		std::string contents = sstream.str();
		const char* contentsChar = contents.c_str();

		// Create a shader of the specified type
		shader = glCreateShader(shaderType);
		// Set the source characters and try to compile
		glShaderSource(shader, 1, &(contentsChar), nullptr);
		glCompileShader(shader);

		GLint status;
		// Query the compile status
		glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

		if (status != GL_TRUE)
		{
			char buffer[512];
			memset(buffer, 0, 512);
			glGetShaderInfoLog(shader, 511, nullptr, buffer);
			printf("GLSL Compile Failed:\n%s", buffer);
			return false;
		}
	}
	else
	{
		printf("Shader file not found: %s", filepath.c_str());
		return false;
	}
	return true;
}



bool Shader::CreateShaderProgram(std::string vertFilePath, std::string fragFilePath)
{
	GLuint VertexShader;
	GLuint FragmentShader;
	// Compile vertex and pixel shaders
	if (!CompileShader(vertFilePath,
		GL_VERTEX_SHADER,
		VertexShader) ||
		!CompileShader(fragFilePath,
			GL_FRAGMENT_SHADER,
			FragmentShader))
	{
		return false;
	}

	// Now create a shader program that
	// links together the vertex/frag shaders
	mShaderProgram = glCreateProgram();
	glAttachShader(mShaderProgram, VertexShader);
	glAttachShader(mShaderProgram, FragmentShader);
	glLinkProgram(mShaderProgram);

	// Verify that the program linked successfully
	GLint status;
	// Query the link status
	glGetProgramiv(mShaderProgram, GL_LINK_STATUS, &status);
	if (status != GL_TRUE)
	{
		char buffer[512];
		memset(buffer, 0, 512);
		glGetProgramInfoLog(mShaderProgram, 511, nullptr, buffer);
		std::string err_msg = buffer;
		std::cout << "GLSL Link Status: " << err_msg << std::endl;
		return false;
	}

	glUseProgram(mShaderProgram);
}

void Shader::UseProgram()
{
	glUseProgram(mShaderProgram);
}

void Shader::SetMatrixUniform(std::string uniformName, glm::mat4 mat)
{
	GLuint loc = glGetUniformLocation(mShaderProgram, uniformName.c_str());
	glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::SetVectorUniform(std::string uniformName, glm::vec3 vec)
{
	GLuint loc = glGetUniformLocation(mShaderProgram, uniformName.c_str());
	glUniform3fv(loc, 1, glm::value_ptr(vec));
}

void Shader::SetFloatUniform(std::string uniformName, float data)
{
	glUniform1f(glGetUniformLocation(mShaderProgram, uniformName.c_str()), data);
}

void Shader::SetSamplerUniform(std::string uniformName, GLuint tex)
{
	glUniform1i(glGetUniformLocation(mShaderProgram, uniformName.c_str()), tex);
}
