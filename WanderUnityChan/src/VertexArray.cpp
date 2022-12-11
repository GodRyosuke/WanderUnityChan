#include "VertexArray.hpp"
#include "glad/glad.h"
#include "Skeleton.hpp"

VertexArray::VertexArray(std::vector<glm::vec3>positions, std::vector<glm::vec3> normals, std::vector<glm::vec2> texcoords,
    std::vector<unsigned int> indices, Layout layout, const class Skeleton* skeleton)
{
    glGenVertexArrays(1, &mVertexArray);
    glBindVertexArray(mVertexArray);

    // Vertex Bufferの作成
    enum BUFFER_TYPE {
        INDEX_BUFFER = 0,
        POS_VB = 1,
        TEXCOORD_VB = 2,
        NORMAL_VB = 3,
        BONE_VB = 4,
        NUM_BUFFER_TYPE = 5,  // required only for instancing
    };

    int numBuffers = 4;
    if (layout == PosNormTexSkin) {
        numBuffers = 5;
    }

    mVertexBuffers = new unsigned int[numBuffers];
    //GLuint m_Buffers[numBuffers] = { 0 };
    glGenBuffers(numBuffers, mVertexBuffers);

    // Vertex Data
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffers[POS_VB]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(positions[0]) * positions.size(), &positions[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // Normal Map
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffers[NORMAL_VB]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(normals[0]) * normals.size(), &normals[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // UV
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffers[TEXCOORD_VB]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(texcoords[0]) * texcoords.size(), &texcoords[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

    if (layout == PosNormTexSkin) {
        // Born and Weights
        skeleton->PopulateBuffer(mVertexBuffers[BONE_VB]);
    }

    // Index Buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVertexBuffers[INDEX_BUFFER]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices[0]) * indices.size(), &indices[0], GL_STATIC_DRAW);


    // unbind cube vertex arrays
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

VertexArray::~VertexArray()
{
    glDeleteBuffers(1, mVertexBuffers);
    glDeleteVertexArrays(1, &mVertexArray);
}

void VertexArray::SetActive()
{
    glBindVertexArray(mVertexArray);
}
