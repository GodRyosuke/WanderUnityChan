#pragma once

#include "Texture.hpp"

class TextureShadowMap : public Texture {
public:
	TextureShadowMap() : Texture() {}
	~TextureShadowMap() {}

	bool Load(int width, int height);
	void BindTexture(GLenum TextureUnit) override;
	void WriteBind();

private:
	GLuint m_fbo;
	GLuint m_shadowMap;
};