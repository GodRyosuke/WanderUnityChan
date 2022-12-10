#pragma once

#include <vector>
#include <glm.hpp>

class VertexArray
{
public:
    // Different supported vertex layouts
    enum Layout
    {
        PosNormTex,
        PosNormSkinTex
    };

    VertexArray(std::vector<glm::vec3>positions, std::vector<glm::vec3> normals, std::vector<glm::vec2> texcoords,
        std::vector<unsigned int> indices, Layout layout);
    ~VertexArray();

    void SetActive();
    //unsigned int GetNumIndices() const { return mNumIndices; }
    //unsigned int GetNumVerts() const { return mNumVerts; }
    //const int GetNumSubMeshes() const { return mSubMeshes.size(); }

private:
    //struct SubMesh {
    //    unsigned int NumIndices;
    //    unsigned int BaseVertex;
    //    unsigned int BaseIndex;
    //    unsigned int MaterialIndex;
    //};

    //std::vector<SubMesh> mSubMeshes;
    //// How many vertices in the vertex buffer?
    //unsigned int mNumVerts;
    //// How many indices in the index buffer
    //unsigned int mNumIndices;
    // OpenGL ID of the vertex buffer
    unsigned int* mVertexBuffers;
    // OpenGL ID of the vertex array object
    unsigned int mVertexArray;
};
