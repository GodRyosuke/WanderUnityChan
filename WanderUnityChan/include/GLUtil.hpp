#pragma once

#include "glew.h"
#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtc/type_ptr.hpp"
#include <iostream>
#include <assimp/postprocess.h> // Post processing flags
#include <vector>

class GLUtil {
public:
	GLUtil() {}
	void GetErr() {
		GLenum err;
		while ((err = glGetError()) != GL_NO_ERROR)
		{
			switch (err) {
			case GL_INVALID_ENUM:
				std::cout << "GL_INVALID_ENUM\n";
				break;
			case GL_INVALID_VALUE:
				std::cout << "GL_INVALID_VALUE\n";
				break;
			case GL_INVALID_OPERATION:
				std::cout << "GL_INVALID_OPERATION\n";
				break;
			case GL_INVALID_FRAMEBUFFER_OPERATION:
				std::cout << "GL_INVALID_FRAMEBUFFER_OPERATION\n";
				break;
			case GL_OUT_OF_MEMORY:
				std::cout << "GL_OUT_OF_MEMORY\n";
				break;
			case GL_STACK_UNDERFLOW:
				std::cout << "GL_STACK_UNDERFLOW\n";
				break;
			case GL_STACK_OVERFLOW:
				std::cout << "GL_STACK_OVERFLOW\n";
				break;
			}
		}
	}

	void Split(char split_char, char* buffer, std::vector<std::string>& out)
	{
		int count = 0;
		if (buffer == nullptr)
		{
			return;
		}

		int start_point = 0;

		while (buffer[count] != '\0')
		{
			if (buffer[count] == split_char)
			{
				if (start_point != count)
				{
					char split_str[256] = { 0 };
					strncpy_s(split_str, 256, &buffer[start_point], count - start_point);
					out.emplace_back(split_str);
				}
				else
				{
					out.emplace_back("");
				}
				start_point = count + 1;
			}
			count++;
		}

		if (start_point != count)
		{
			char split_str[256] = { 0 };
			strncpy_s(split_str, 256, &buffer[start_point], count - start_point);
			out.emplace_back(split_str);
		}
	}

	void Replace(char search_char, char replace_char, char* buffer)
	{
		int len = (int)strlen(buffer);

		for (int i = 0; i < len; i++)
		{
			if (buffer[i] == search_char)
			{
				buffer[i] = replace_char;
			}
		}
	}

	glm::mat4 ToGlmMat4(aiMatrix4x4& aiMat)
	{
		return glm::transpose(glm::make_mat4(&aiMat.a1));
	}

	glm::mat4 ToGlmMat4(aiMatrix3x3& aiMat)
	{
		return glm::transpose(glm::make_mat3(&aiMat.a1));
	}




};


