#include "TextureShadowMap.hpp"

bool TextureShadowMap::Load(int width, int height)
{
    this->width = width;
    this->height = height;

    // Create the FBO
    glGenFramebuffers(1, &m_fbo);

    // Create the depth buffer
    glGenTextures(1, &m_shadowMap);
    glBindTexture(GL_TEXTURE_2D, m_shadowMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_shadowMap, 0);

    // Disable writes to the color buffer
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    if (Status != GL_FRAMEBUFFER_COMPLETE) {
        printf("FB error, status: 0x%x\n", Status);
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return true;
}

void TextureShadowMap::WriteBind()
{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, width, height);  // set the width/height of the shadow map!
}

void TextureShadowMap::BindTexture(GLenum TextureUnit)
{
    glActiveTexture(TextureUnit);
    glBindTexture(GL_TEXTURE_2D, m_shadowMap);
}
