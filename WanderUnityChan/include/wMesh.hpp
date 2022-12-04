#pragma once
#include <string>
#include <unordered_map>
#include "fbxsdk.h"
#include "json.hpp"

namespace nl = nlohmann;

class wMesh {
public:
    wMesh();
    bool Load(std::string filePath);

private:
    void LoadNode(FbxNode* node);
    void LoadTexture(FbxTexture* lTexture);

    FbxManager* mManager;
    FbxScene* mScene;
    std::string mMeshName;
    std::unordered_map<std::string, class Texture*> mTextures;
    nl::json mMaterialJsonMap;


};
