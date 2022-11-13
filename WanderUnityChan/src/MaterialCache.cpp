#include "FBXLoader/MaterialCache.hpp"


namespace
{
    //const float ANGLE_TO_RADIAN = 3.1415926f / 180.f;
    const GLfloat BLACK_COLOR[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    const GLfloat GREEN_COLOR[] = { 0.0f, 1.0f, 0.0f, 1.0f };
    const GLfloat WHITE_COLOR[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    const GLfloat WIREFRAME_COLOR[] = { 0.5f, 0.5f, 0.5f, 1.0f };

    //const int TRIANGLE_VERTEX_COUNT = 3;

    //// Four floats for every position.
    //const int VERTEX_STRIDE = 4;
    //// Three floats for every normal.
    //const int NORMAL_STRIDE = 3;
    //// Two floats for every UV.
    //const int UV_STRIDE = 2;

    //const GLfloat DEFAULT_LIGHT_POSITION[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    //const GLfloat DEFAULT_DIRECTION_LIGHT_POSITION[] = { 0.0f, 0.0f, 1.0f, 0.0f };
    //const GLfloat DEFAULT_SPOT_LIGHT_DIRECTION[] = { 0.0f, 0.0f, -1.0f };
    //const GLfloat DEFAULT_LIGHT_COLOR[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    //const GLfloat DEFAULT_LIGHT_SPOT_CUTOFF = 180.0f;

    // Get specific property value and connected texture if any.
    // Value = Property value * Factor property value (if no factor property, multiply by 1).
    FbxDouble3 GetMaterialProperty(const FbxSurfaceMaterial* pMaterial,
        const char* pPropertyName,
        const char* pFactorPropertyName,
        GLuint& pTextureName)
    {
        FbxDouble3 lResult(0, 0, 0);
        const FbxProperty lProperty = pMaterial->FindProperty(pPropertyName);
        const FbxProperty lFactorProperty = pMaterial->FindProperty(pFactorPropertyName);
        if (lProperty.IsValid() && lFactorProperty.IsValid())
        {
            lResult = lProperty.Get<FbxDouble3>();
            double lFactor = lFactorProperty.Get<FbxDouble>();
            if (lFactor != 1)
            {
                lResult[0] *= lFactor;
                lResult[1] *= lFactor;
                lResult[2] *= lFactor;
            }
        }

        if (lProperty.IsValid())
        {
            const int lTextureCount = lProperty.GetSrcObjectCount<FbxFileTexture>();
            if (lTextureCount)
            {
                const FbxFileTexture* lTexture = lProperty.GetSrcObject<FbxFileTexture>();
                std::string texFileName = lTexture->GetFileName();
                printf("tex name: %s\n", texFileName.c_str());
                if (lTexture && lTexture->GetUserDataPtr())
                {
                    pTextureName = *(static_cast<GLuint*>(lTexture->GetUserDataPtr()));
                }
            }
        }

        return lResult;
    }
}

MaterialCache::MaterialCache() : mShinness(0)
{
}

MaterialCache::~MaterialCache()
{

}

// Bake material properties.
bool MaterialCache::Initialize(const FbxSurfaceMaterial * pMaterial)
{
    const FbxDouble3 lEmissive = GetMaterialProperty(pMaterial,
        FbxSurfaceMaterial::sEmissive, FbxSurfaceMaterial::sEmissiveFactor, mEmissive.mTextureName);
    mEmissive.mColor[0] = static_cast<GLfloat>(lEmissive[0]);
    mEmissive.mColor[1] = static_cast<GLfloat>(lEmissive[1]);
    mEmissive.mColor[2] = static_cast<GLfloat>(lEmissive[2]);

    const FbxDouble3 lAmbient = GetMaterialProperty(pMaterial,
        FbxSurfaceMaterial::sAmbient, FbxSurfaceMaterial::sAmbientFactor, mAmbient.mTextureName);
    mAmbient.mColor[0] = static_cast<GLfloat>(lAmbient[0]);
    mAmbient.mColor[1] = static_cast<GLfloat>(lAmbient[1]);
    mAmbient.mColor[2] = static_cast<GLfloat>(lAmbient[2]);

    const FbxDouble3 lDiffuse = GetMaterialProperty(pMaterial,
        FbxSurfaceMaterial::sDiffuse, FbxSurfaceMaterial::sDiffuseFactor, mDiffuse.mTextureName);
    mDiffuse.mColor[0] = static_cast<GLfloat>(lDiffuse[0]);
    mDiffuse.mColor[1] = static_cast<GLfloat>(lDiffuse[1]);
    mDiffuse.mColor[2] = static_cast<GLfloat>(lDiffuse[2]);

    const FbxDouble3 lSpecular = GetMaterialProperty(pMaterial,
        FbxSurfaceMaterial::sSpecular, FbxSurfaceMaterial::sSpecularFactor, mSpecular.mTextureName);
    mSpecular.mColor[0] = static_cast<GLfloat>(lSpecular[0]);
    mSpecular.mColor[1] = static_cast<GLfloat>(lSpecular[1]);
    mSpecular.mColor[2] = static_cast<GLfloat>(lSpecular[2]);

    FbxProperty lShininessProperty = pMaterial->FindProperty(FbxSurfaceMaterial::sShininess);
    if (lShininessProperty.IsValid())
    {
        double lShininess = lShininessProperty.Get<FbxDouble>();
        mShinness = static_cast<GLfloat>(lShininess);
    }

    return true;
}

void MaterialCache::SetCurrentMaterial() const
{
    //glMaterialfv(GL_FRONT, GL_EMISSION, mEmissive.mColor);
    //glMaterialfv(GL_FRONT, GL_AMBIENT, mAmbient.mColor);
    //glMaterialfv(GL_FRONT, GL_DIFFUSE, mDiffuse.mColor);
    //glMaterialfv(GL_FRONT, GL_SPECULAR, mSpecular.mColor);
    //glMaterialf(GL_FRONT, GL_SHININESS, mShinness);

    glBindTexture(GL_TEXTURE_2D, mDiffuse.mTextureName);
}

void MaterialCache::SetDefaultMaterial()
{
    //glMaterialfv(GL_FRONT, GL_EMISSION, BLACK_COLOR);
    //glMaterialfv(GL_FRONT, GL_AMBIENT, BLACK_COLOR);
    //glMaterialfv(GL_FRONT, GL_DIFFUSE, GREEN_COLOR);
    //glMaterialfv(GL_FRONT, GL_SPECULAR, BLACK_COLOR);
    //glMaterialf(GL_FRONT, GL_SHININESS, 0);

    glBindTexture(GL_TEXTURE_2D, 0);
}