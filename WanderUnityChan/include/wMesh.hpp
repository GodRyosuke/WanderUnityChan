#pragma once
#include <string>

class wMesh {
public:
    wMesh();
    bool Load(std::string filePath);

private:
    std::string mMeshName;
};
