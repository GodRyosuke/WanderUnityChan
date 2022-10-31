#include "Mesh.hpp"
#include <iostream>
#include "GLUtil.hpp"


Mesh::Mesh()
{

}

bool Mesh::Load(std::string RootPath, std::string ObjFileName)
{
    this->ObjFileRoot = RootPath;
    this->ObjFileName = ObjFileName;
    std::string FilePath = RootPath + ObjFileName;

    m_pScene = m_Importer.ReadFile(FilePath.c_str(), ASSIMP_LOAD_FLAGS);
    //m_pScene = pScene;

    if (!m_pScene) {
        printf("Error parsing '%s': '%s'\n", FilePath.c_str(), m_Importer.GetErrorString());
        return false;
    }


    GetGlobalInvTrans();

    m_Meshes.resize(m_pScene->mNumMeshes);
    m_Materials.resize(m_pScene->mNumMaterials);

    mNumVertices = 0;
    mNumIndices = 0;

    // 頂点の総数などを読み込む
    for (unsigned int i = 0; i < m_Meshes.size(); i++) {
        m_Meshes[i].MaterialIndex = m_pScene->mMeshes[i]->mMaterialIndex; // MeshとMaterialの紐づけ
        m_Meshes[i].NumIndices = m_pScene->mMeshes[i]->mNumFaces * 3;
        m_Meshes[i].BaseVertex = mNumVertices;
        m_Meshes[i].BaseIndex = mNumIndices;

        mNumVertices += m_pScene->mMeshes[i]->mNumVertices;
        mNumIndices += m_Meshes[i].NumIndices;
    }

    ReserveVertexSpace();

    // Mesh(頂点情報など)の読み込み
    for (int meshIdx = 0; meshIdx < m_Meshes.size(); meshIdx++) {
        const aiMesh* paiMesh = m_pScene->mMeshes[meshIdx];
        LoadMesh(paiMesh, meshIdx);
    }


    // MaterialとTexture読み込み
    printf("Num materials: %d\n", m_pScene->mNumMaterials);

    // Materialの読み込み
    for (int materialIdx = 0; materialIdx < m_pScene->mNumMaterials; materialIdx++) {
        const aiMaterial* pMaterial = m_pScene->mMaterials[materialIdx];

        // Diffuse Textureを読み込む
        m_Materials[materialIdx].DiffuseTexture = NULL;

        if (pMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            aiString Path;

            if (pMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &Path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
                std::string texturePathData = Path.data;

                // 読み込んだTextureのPathの分解処理
                GLUtil glutil;
                char buffer[512];
                memset(buffer, 0, 512 * sizeof(char));
                memcpy(buffer, texturePathData.c_str(), sizeof(char) * 512);
                glutil.Replace('\\', '/', buffer);
                std::vector<std::string> split_list;
                std::string replace_file_name = buffer;
                // 「/」で分解
                glutil.Split('/', buffer, split_list);

                std::string texturePath = ObjFileRoot + "Textures/" + split_list[split_list.size() - 1];
                m_Materials[materialIdx].DiffuseTexture = new Texture(texturePath);
            }
        }

        // Diffuse Specular Ambientの読み込み
        aiColor3D AmbientColor(0.0f, 0.0f, 0.0f);
        glm::vec3 AllOnes = glm::vec3(1.0f, 1.0f, 1.0f);

        int ShadingModel = 0;
        if (pMaterial->Get(AI_MATKEY_SHADING_MODEL, ShadingModel) == AI_SUCCESS) {
            printf("Shading model %d\n", ShadingModel);
        }

        if (pMaterial->Get(AI_MATKEY_COLOR_AMBIENT, AmbientColor) == AI_SUCCESS) {
            printf("Loaded ambient color [%f %f %f]\n", AmbientColor.r, AmbientColor.g, AmbientColor.b);
            m_Materials[materialIdx].AmbientColor.r = AmbientColor.r;
            m_Materials[materialIdx].AmbientColor.g = AmbientColor.g;
            m_Materials[materialIdx].AmbientColor.b = AmbientColor.b;
        }
        else {
            m_Materials[materialIdx].AmbientColor = AllOnes;
        }

        aiColor3D DiffuseColor(0.0f, 0.0f, 0.0f);

        if (pMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, DiffuseColor) == AI_SUCCESS) {
            printf("Loaded diffuse color [%f %f %f]\n", DiffuseColor.r, DiffuseColor.g, DiffuseColor.b);
            m_Materials[materialIdx].DiffuseColor.r = DiffuseColor.r;
            m_Materials[materialIdx].DiffuseColor.g = DiffuseColor.g;
            m_Materials[materialIdx].DiffuseColor.b = DiffuseColor.b;
        }

        aiColor3D SpecularColor(0.0f, 0.0f, 0.0f);

        if (pMaterial->Get(AI_MATKEY_COLOR_SPECULAR, SpecularColor) == AI_SUCCESS) {
            printf("Loaded specular color [%f %f %f]\n", SpecularColor.r, SpecularColor.g, SpecularColor.b);
            m_Materials[materialIdx].SpecColor.r = SpecularColor.r;
            m_Materials[materialIdx].SpecColor.g = SpecularColor.g;
            m_Materials[materialIdx].SpecColor.b = SpecularColor.b;
        }

        ai_real Shiness(0.f);
        if (pMaterial->Get(AI_MATKEY_SHININESS, Shiness) == AI_SUCCESS) {
            m_Materials[materialIdx].SpecPower = Shiness;
        }
    }

    // Vertex Array Object作成
    unsigned int VertexArray;

    glGenVertexArrays(1, &VertexArray);
    glBindVertexArray(VertexArray);

    // Vertex Bufferの作成
    PopulateBuffers();


    // unbind cube vertex arrays
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    mVertexArray = VertexArray;


    bool checkErr = (glGetError() == GL_NO_ERROR);
    return true;
    return checkErr;
}

void Mesh::ReserveVertexSpace()
{
    m_Positions.reserve(mNumVertices);
    m_Normals.reserve(mNumVertices);
    m_TexCoords.reserve(mNumVertices);
    m_Indices.reserve(mNumIndices);
}

void Mesh::LoadMesh(const aiMesh* pMesh, unsigned int meshIdx)
{
    const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

    // Vertex, Normal, UV取得
    for (unsigned int i = 0; i < pMesh->mNumVertices; i++) {

        const aiVector3D& pPos = pMesh->mVertices[i];
        m_Positions.push_back(glm::vec3(pPos.x, pPos.y, pPos.z));

        if (pMesh->mNormals) {
            const aiVector3D& pNormal = pMesh->mNormals[i];
            m_Normals.push_back(glm::vec3(pNormal.x, pNormal.y, pNormal.z));
        }
        else {
            aiVector3D Normal(0.0f, 1.0f, 0.0f);
            m_Normals.push_back(glm::vec3(Normal.x, Normal.y, Normal.z));
        }

        const aiVector3D& pTexCoord = pMesh->HasTextureCoords(0) ? pMesh->mTextureCoords[0][i] : Zero3D;
        m_TexCoords.push_back(glm::vec2(pTexCoord.x, pTexCoord.y));
    }

    // Index 情報取得
    for (unsigned int i = 0; i < pMesh->mNumFaces; i++) {
        const aiFace& Face = pMesh->mFaces[i];
        //printf("num indices %d\n", Face.mNumIndices);
        assert(Face.mNumIndices == 3);
        m_Indices.push_back(Face.mIndices[0]);
        m_Indices.push_back(Face.mIndices[1]);
        m_Indices.push_back(Face.mIndices[2]);
    }
}

void Mesh::PopulateBuffers()
{
    enum BUFFER_TYPE {
        INDEX_BUFFER = 0,
        POS_VB = 1,
        TEXCOORD_VB = 2,
        NORMAL_VB = 3,
        NUM_BUFFERS = 4,  // required only for instancing
    };
    GLuint m_Buffers[NUM_BUFFERS] = { 0 };
    glGenBuffers(NUM_BUFFERS, m_Buffers);

    // Vertex Data
    glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[POS_VB]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(m_Positions[0]) * m_Positions.size(), &m_Positions[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // Normal Map
    glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[NORMAL_VB]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(m_Normals[0]) * m_Normals.size(), &m_Normals[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // UV
    glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[TEXCOORD_VB]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(m_TexCoords[0]) * m_TexCoords.size(), &m_TexCoords[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);


    // Indef Buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Buffers[INDEX_BUFFER]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(m_Indices[0]) * m_Indices.size(), &m_Indices[0], GL_STATIC_DRAW);
}



void Mesh::UpdateTransform(Shader* shader, float timeInSeconds)
{
    glm::mat4 ScaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(mMeshScale, mMeshScale, mMeshScale));
    glm::mat4 TranslateMat = glm::translate(glm::mat4(1.0f), mMeshPos);
    glm::mat4 TransformMat = TranslateMat * mMeshRotate * ScaleMat;

    shader->SetMatrixUniform("ModelTransform", TransformMat);
}

void Mesh::Draw(Shader* shader, float timeInSeconds)
{
    shader->UseProgram();
    UpdateTransform(shader, timeInSeconds);


    glBindVertexArray(mVertexArray);

    for (unsigned int i = 0; i < m_Meshes.size(); i++) {
        unsigned int MaterialIndex = m_Meshes[i].MaterialIndex;
        assert(MaterialIndex < m_Materials.size());

        shader->SetVectorUniform("uAmbientLight", m_Materials[MaterialIndex].AmbientColor);
        shader->SetVectorUniform("uDirLight.mDirection", glm::vec3(0, -0.707, -0.707));
        shader->SetVectorUniform("gMatDiffuseColor", m_Materials[MaterialIndex].DiffuseColor);
        shader->SetVectorUniform("uDirLight.mSpecColor", m_Materials[MaterialIndex].SpecColor);
        shader->SetFloatUniform("gMatSpecularPower", m_Materials[MaterialIndex].SpecPower);
        shader->SetVectorUniform("gMatSpecularColor", m_Materials[MaterialIndex].SpecColor);
        shader->SetFloatUniform("gMatSpecularIntensity", 1.f);
        //shader->SetFloatUniform("gMatSpecularIntensity", 1.0f);

        if (m_Materials[MaterialIndex].DiffuseTexture) {
            m_Materials[MaterialIndex].DiffuseTexture->BindTexture(GL_TEXTURE0);
        }

        //if (m_Materials[MaterialIndex].pSpecularExponent) {
        //    m_Materials[MaterialIndex].pSpecularExponent->Bind(SPECULAR_EXPONENT_UNIT);

        //    if (pRenderCallbacks) {
        //        pRenderCallbacks->ControlSpecularExponent(true);
        //    }
        //}
        //else {
        //    if (pRenderCallbacks) {
        //        pRenderCallbacks->ControlSpecularExponent(false);
        //    }
        //}

        //if (pRenderCallbacks) {
        //    pRenderCallbacks->DrawStartCB(i);
        //    pRenderCallbacks->SetMaterial(m_Materials[MaterialIndex]);
        //}

        glDrawElementsBaseVertex(GL_TRIANGLES,
            m_Meshes[i].NumIndices,
            GL_UNSIGNED_INT,
            (void*)(sizeof(unsigned int) * m_Meshes[i].BaseIndex),
            m_Meshes[i].BaseVertex);
    }

    glBindVertexArray(0);
}

glm::mat4 Mesh::GetWorldMat()
{
    glm::mat4 ScaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(mMeshScale, mMeshScale, mMeshScale));
    glm::mat4 TranslateMat = glm::translate(glm::mat4(1.0f), mMeshPos);
    glm::mat4 TransformMat = TranslateMat * mMeshRotate * ScaleMat;
    return TransformMat;
}