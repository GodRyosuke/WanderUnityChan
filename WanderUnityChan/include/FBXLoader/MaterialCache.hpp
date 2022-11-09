#pragma once

#include <fbxsdk.h>
#include <glew.h>

// Cache for FBX material
class MaterialCache
{
public:
    MaterialCache();
    ~MaterialCache();

    bool Initialize(const FbxSurfaceMaterial* pMaterial);

    // Set material colors and binding diffuse texture if exists.
    void SetCurrentMaterial() const;

    bool HasTexture() const { return mDiffuse.mTextureName != 0; }

    // Set default green color.
    static void SetDefaultMaterial();

private:
    struct ColorChannel
    {
        ColorChannel() : mTextureName(0)
        {
            mColor[0] = 0.0f;
            mColor[1] = 0.0f;
            mColor[2] = 0.0f;
            mColor[3] = 1.0f;
        }

        GLuint mTextureName;
        GLfloat mColor[4];
    };




    ColorChannel mEmissive;
    ColorChannel mAmbient;
    ColorChannel mDiffuse;
    ColorChannel mSpecular;
    GLfloat mShinness;
};