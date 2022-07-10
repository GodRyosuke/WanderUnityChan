#pragma once
#define STB_IMAGE_IMPLEMENTATION

#include <string>
#include "glew.h"
#include "SDL.h"
#include "stb_image.h"
#include <vector>

class Texture {
public:
	Texture();
	// Simple Load Texture
	Texture(std::string filePath);
	// For Cube Maps
	Texture(std::vector<std::string> filePaths);


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