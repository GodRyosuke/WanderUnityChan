#pragma once

#include "Texture.hpp"
#define STB_IMAGE_IMPLEMENTATION

class TextureShadowMap : public Texture {
public:
	TextureShadowMap();
	~TextureShadowMap() {}

	bool Load(int width, int height);
	void BindTexture(GLenum TextureUnit) override;
	void WriteBind();

private:
	GLuint m_fbo;
	GLuint m_shadowMap;
};