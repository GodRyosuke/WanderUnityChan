#pragma once


#include <string>
#include "glad/glad.h"
//#include "glew.h"
#include "SDL.h"

#include <vector>
#define STB_IMAGE_IMPLEMENTATION

class Texture {
public:
	Texture();
	// Simple Load Texture
	Texture(std::string filePath);
	// For Cube Maps
	Texture(std::vector<std::string> filePaths);

	static bool LoadTextureFromFile(std::string fileName, GLuint& textureObj);
	virtual void BindTexture(GLenum TextureUnit = GL_TEXTURE0);
	void BindCubeMapTexture();
	void UnBindTexture();

	int getWidth() { return width; }
	int getHeight() { return height; }

protected:

	int width;
	int height;
	GLuint texture_data;


private:

};