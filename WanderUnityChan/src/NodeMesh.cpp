#include "NodeMesh.hpp"
#include "glad/glad.h"
//#include "glew.h"
#include "GLUtil.hpp"
#include "Texture.hpp"
#include "deFBXMesh.hpp"
#include "Shader.hpp"
#include "FBXSkeleton.hpp"

namespace
{
    //const float ANGLE_TO_RADIAN = 3.1415926f / 180.f;
    const GLfloat BLACK_COLOR[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    const GLfloat GREEN_COLOR[] = { 0.0f, 1.0f, 0.0f, 1.0f };
    const GLfloat WHITE_COLOR[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    const GLfloat WIREFRAME_COLOR[] = { 0.5f, 0.5f, 0.5f, 1.0f };

 
}

NodeMesh::NodeMesh(FbxNode* node, deFBXMesh* fbxmesh)
    :mOwnerMesh(fbxmesh)
    ,mFBXSkeleton(nullptr)
{
    mVertexArray = 0;
    //mVertexBuffers = nullptr;
    mPositions.resize(0);
    mNormals.resize(0);
    mTexCoords.resize(0);
    
    std::string NodeName = node->GetName();
    mIsMesh = false;
    //printf("NodeName: %s\n", NodeName.c_str());
    int attrCount = node->GetNodeAttributeCount();
    for (int i = 0; i < attrCount; ++i) {
        FbxNodeAttribute* attr = node->GetNodeAttributeByIndex(i);
        FbxNodeAttribute::EType type = attr->GetAttributeType();
        if (type == FbxNodeAttribute::EType::eMesh) {   // Mesh Nodeなら
            mIsMesh = true;
            FbxMesh* pMesh = static_cast<FbxMesh*>(attr);
            LoadMesh(pMesh);
            //unsigned int vertexOffset;
            //NodeMesh* NodeMeshes = nullptr;
            //if (mIsDrawArray) {
            //    NodeMeshes = LoadMeshArray(pMesh, vertexOffset);
            //    if (!NodeMeshes) {
            //        printf("error: failed to load LoadMeshArray\n");
            //        exit(-1);
            //    }
            //}
            //else {
            //    LoadMeshElement(pMesh);
            //}

            // マテリアルの読み込み
            int materialCount = node->GetMaterialCount();
            //printf("material count: %d\n", materialCount);
            //Material* MaterialData = nullptr;
            for (int materialIndex = 0; materialIndex < materialCount; materialIndex++) {
                if (mSubMeshes.size() <= materialIndex) {
                    break;
                }
                FbxSurfaceMaterial* material = node->GetMaterial(materialIndex);
                mSubMeshes[materialIndex]->Material = LoadMaterial(material);
                mSubMeshes[materialIndex]->MaterialName = material->GetName();
                //MaterialData = LoadMaterial(material);
                //NodeMeshes[materialIndex].material = MaterialData;
                //mNodeMeshes.push_back(&NodeMeshes[materialIndex]);
            }

            //MeshOffset mo;
            //mo.material = MaterialData;
            //mo.VNTOffset = vertexOffset;
            //mMeshOffsets.push_back(mo);
        }
    }

    int childCount = node->GetChildCount();
    NodeMesh* nodeMesh = nullptr;
    mChilds.resize(childCount);
    for (int nodeIdx = 0; nodeIdx < childCount; ++nodeIdx) {
        nodeMesh = new NodeMesh(node->GetChild(nodeIdx), fbxmesh);
        mChilds[nodeIdx] = nodeMesh;
        //LoadNode(node->GetChild(nodeIdx));
    }

	mNumChild = node->GetChildCount();
}


//
//bool NodeMesh::LoadMesh(FbxMesh* mesh)
//{
//    FbxLayerElementArrayTemplate<int>* lMaterialIndice = NULL;
//    lMaterialIndice = &mesh->GetElementMaterial()->GetIndexArray();
//    std::vector<int> mSubMeshes(0);
//    for (int lPolygonIndex = 0; lPolygonIndex < mesh->GetPolygonCount(); lPolygonIndex++) {
//        const int lMaterialIndex = lMaterialIndice->GetAt(lPolygonIndex);
//        if (mSubMeshes.size() < lMaterialIndex + 1)
//        {
//            mSubMeshes.resize(lMaterialIndex + 1);
//        }
//    }
//    printf("material num: %d\n", mSubMeshes.size());
//
//
//    return true;
//}

bool NodeMesh::LoadMesh(FbxMesh* mesh)
{
    if (!mesh->GetNode()) {
        return false;
    }
    const int lPolygonCount = mesh->GetPolygonCount();

    // Count the polygon count of each material
    FbxLayerElementArrayTemplate<int>* lMaterialIndice = NULL;
    FbxGeometryElement::EMappingMode lMaterialMappingMode = FbxGeometryElement::eNone;
    if (mesh->GetElementMaterial())
    {
        lMaterialIndice = &mesh->GetElementMaterial()->GetIndexArray();
        lMaterialMappingMode = mesh->GetElementMaterial()->GetMappingMode();
        if (lMaterialIndice && lMaterialMappingMode == FbxGeometryElement::eByPolygon)
        {
            FBX_ASSERT(lMaterialIndice->GetCount() == lPolygonCount);
            if (lMaterialIndice->GetCount() == lPolygonCount)
            {
                // Count the faces of each material
                for (int lPolygonIndex = 0; lPolygonIndex < lPolygonCount; ++lPolygonIndex)
                {
                    const int lMaterialIndex = lMaterialIndice->GetAt(lPolygonIndex);
                    if (mSubMeshes.size() < lMaterialIndex + 1)
                    {
                        mSubMeshes.resize(lMaterialIndex + 1);
                    }
                    if (mSubMeshes[lMaterialIndex] == NULL)
                    {
                        mSubMeshes[lMaterialIndex] = new SubMesh;
                    }
                    mSubMeshes[lMaterialIndex]->TriangleCount += 1;
                }

                // Make sure we have no "holes" (NULL) in the mSubMeshes table. This can happen
                // if, in the loop above, we resized the mSubMeshes by more than one slot.
                for (int i = 0; i < mSubMeshes.size(); i++)
                {
                    if (mSubMeshes[i] == NULL)
                        mSubMeshes[i] = new SubMesh;
                }

                // Record the offset (how many vertex)
                const int lMaterialCount = mSubMeshes.size();
                int lOffset = 0;
                for (int lIndex = 0; lIndex < lMaterialCount; ++lIndex)
                {
                    mSubMeshes[lIndex]->IndexOffset = lOffset;
                    lOffset += mSubMeshes[lIndex]->TriangleCount * 3;
                    // This will be used as counter in the following procedures, reset to zero
                    mSubMeshes[lIndex]->TriangleCount = 0;
                }
                FBX_ASSERT(lOffset == lPolygonCount * 3);
            }
        }
    }

    // All faces will use the same material.
    if (mSubMeshes.size() == 0)
    {
        mSubMeshes.resize(1);
        mSubMeshes[0] = new SubMesh();
    }

    bool hasNormal = mesh->GetElementNormalCount() > 0;
    bool hasUV = mesh->GetElementUVCount() > 0;
    bool allByControlPoint = true;
    FbxGeometryElement::EMappingMode lNormalMappingMode = FbxGeometryElement::eNone;
    FbxGeometryElement::EMappingMode lUVMappingMode = FbxGeometryElement::eNone;
    if (hasNormal)
    {
        lNormalMappingMode = mesh->GetElementNormal(0)->GetMappingMode();
        if (lNormalMappingMode == FbxGeometryElement::eNone)
        {
            hasNormal = false;
        }
        if (hasNormal && lNormalMappingMode != FbxGeometryElement::eByControlPoint)
        {
            allByControlPoint = false;
        }
    }
    if (hasUV)
    {
        lUVMappingMode = mesh->GetElementUV(0)->GetMappingMode();
        if (lUVMappingMode == FbxGeometryElement::eNone)
        {
            hasUV = false;
        }
        if (hasNormal && lUVMappingMode != FbxGeometryElement::eByControlPoint)
        {
            allByControlPoint = false;
        }
    }

    assert(allByControlPoint == false);


    mIndices.resize(lPolygonCount * 3);

    mPositions.resize(lPolygonCount * 3);
    mNormals.resize(lPolygonCount * 3);
    mTexCoords.resize(lPolygonCount * 3);

    unsigned int* lIndices = new unsigned int[lPolygonCount * 3];
    float* lVertices = new float[lPolygonCount * 3 * 3];
    float* lNormals = nullptr;

    if (hasNormal) {
        lNormals = new float[lPolygonCount * 3 * 3];
    }

    float* lUVs = nullptr;
    FbxStringList lUVNames;
    mesh->GetUVSetNames(lUVNames);
    const char* lUVName = NULL;
    if (hasUV && lUVNames.GetCount())
    {
        mTexCoords.resize(lPolygonCount * 3);
        lUVs = new float[lPolygonCount * 3 * 2];

        lUVName = lUVNames[0];
    }

    const FbxVector4* lControlPoints = mesh->GetControlPoints();
    FbxVector4 lCurrentVertex;
    FbxVector4 lCurrentNormal;
    FbxVector2 lCurrentUV;

    int lVertexCount = 0;
    for (int lPolygonIndex = 0; lPolygonIndex < lPolygonCount; ++lPolygonIndex)
    {
        int lMaterialIndex = 0;
        if (lMaterialIndice && lMaterialMappingMode == FbxGeometryElement::eByPolygon)
        {
            lMaterialIndex = lMaterialIndice->GetAt(lPolygonIndex);
        }

        const int lIndexOffset = mSubMeshes[lMaterialIndex]->IndexOffset +
            mSubMeshes[lMaterialIndex]->TriangleCount * 3;

        for (int lVerticeIndex = 0; lVerticeIndex < 3; ++lVerticeIndex)
        {
            const int lControlPointIndex = mesh->GetPolygonVertex(lPolygonIndex, lVerticeIndex);

            if (lControlPointIndex < 0) {
                lVertexCount++;
                continue;
            }

            // If the lControlPointIndex is -1, we probably have a corrupted mesh data. At this point,
            // it is not guaranteed that the cache will work as expected.
            lIndices[lIndexOffset + lVerticeIndex] = static_cast<unsigned int>(lVertexCount);

            lCurrentVertex = lControlPoints[lControlPointIndex];
            lVertices[lVertexCount * 3] = static_cast<float>(lCurrentVertex[0]);
            lVertices[lVertexCount * 3+ 1] = static_cast<float>(lCurrentVertex[1]);
            lVertices[lVertexCount * 3+ 2] = static_cast<float>(lCurrentVertex[2]);

            if (hasNormal)
            {
                mesh->GetPolygonVertexNormal(lPolygonIndex, lVerticeIndex, lCurrentNormal);
                lNormals[lVertexCount * 3] = static_cast<float>(lCurrentNormal[0]);
                lNormals[lVertexCount * 3+ 1] = static_cast<float>(lCurrentNormal[1]);
                lNormals[lVertexCount * 3+ 2] = static_cast<float>(lCurrentNormal[2]);
            }

            if (hasUV)
            {
                bool lUnmappedUV;
                mesh->GetPolygonVertexUV(lPolygonIndex, lVerticeIndex, lUVName, lCurrentUV, lUnmappedUV);
                lUVs[lVertexCount * 2] = static_cast<float>(lCurrentUV[0]);
                lUVs[lVertexCount * 2+ 1] = static_cast<float>(lCurrentUV[1]);
            }


            mIndices[lPolygonIndex * 3 + lVerticeIndex] = lPolygonIndex * 3 + lVerticeIndex;
            lCurrentVertex = lControlPoints[lControlPointIndex];

            FbxVector4 position = mesh->GetControlPointAt(lControlPointIndex);
            mPositions[lPolygonIndex * 3 + lVerticeIndex] = glm::vec3(
                static_cast<float>(position[0]),
                static_cast<float>(position[1]),
                static_cast<float>(position[2])
            );

            if (hasNormal) {
                mesh->GetPolygonVertexNormal(lPolygonIndex, lVerticeIndex, lCurrentNormal);
                mNormals[lPolygonIndex * 3 + lVerticeIndex] = glm::vec3(
                    static_cast<float>(lCurrentNormal[0]),
                    static_cast<float>(lCurrentNormal[1]),
                    static_cast<float>(lCurrentNormal[2])
                );
            }

            if (hasUV) {
                //lCurrentUV[0] = 0.f; lCurrentUV[1] = 0.f;
                bool lUnmappedUV = false;
                //lUVName = lUVNames.GetStringAt(lPolygonIndex);
                mesh->GetPolygonVertexUV(lPolygonIndex, lVerticeIndex, lUVName, lCurrentUV, lUnmappedUV);
                mTexCoords[lPolygonIndex * 3 + lVerticeIndex] = glm::vec2(
                    static_cast<float>(lCurrentUV[0]),
                    static_cast<float>(lCurrentUV[1])
                );
            }

            lVertexCount++;
        }
        mSubMeshes[lMaterialIndex]->TriangleCount += 1;
    }


    //CreateVAO(lPolygonCount,
    //    lIndices,
    //    lVertices,
    //    lNormals,
    //    lUVs,
    //    hasNormal,
    //    hasUV);

    //delete[] lIndices;
    //delete[] lVertices;
    //if (hasNormal) {
    //    delete[] lNormals;
    //}
    //if (hasUV) {
    //    delete[] lUVs;
    //}

    if (mOwnerMesh->GetIsSkinMesh()) {
        mFBXSkeleton = new FBXSkeleton();
        if (!mFBXSkeleton->Load(mesh)) {
            printf("error: failed to load fbx skeleton\n");
            return false;
        }
    }

    CreateVAO();
    mPositions.clear();
    mNormals.clear();
    mTexCoords.clear();
    
    if (mOwnerMesh->GetIsSkinMesh()) {
        mFBXSkeleton->DeleteBoneData();
    }

    const bool lHasVertexCache = mesh->GetDeformerCount(FbxDeformer::eVertexCache) &&
        (static_cast<FbxVertexCacheDeformer*>(mesh->GetDeformer(0, FbxDeformer::eVertexCache)))->Active.Get();
    assert(lHasVertexCache == false);
    const bool lHasShape = mesh->GetShapeCount() > 0;





    //assert(mesh->GetElementPolygonGroupCount() == 3);
    //printf("vertex color num: %d\n", mesh->GetElementVertexColorCount());

    return true;
}

void NodeMesh::CreateVAO(
    const int lPolygonCount,
    unsigned int* lIndices,
    float* lVertices,
    float* lNormals,
    float* lUVs,
    bool hasNormal,
    bool hasUV
)
{
    // Create VBOs
    glGenVertexArrays(1, &mVertexArray);
    glBindVertexArray(mVertexArray);
    glGenBuffers(NUM_BUFFERS, mVertexBuffers);

    // Save vertex attributes into GPU
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffers[POS_VB]);
    glBufferData(GL_ARRAY_BUFFER, lPolygonCount * 3 * 3 * sizeof(float), lVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);


    if (hasNormal)
    {
        glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffers[NORMAL_VB]);
        glBufferData(GL_ARRAY_BUFFER, lPolygonCount * 3 * 3* sizeof(float), lNormals, GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
    }

    if (hasUV)
    {
        glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffers[TEXCOORD_VB]);
        glBufferData(GL_ARRAY_BUFFER, lPolygonCount * 3 * 2 * sizeof(float), lUVs, GL_STATIC_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVertexBuffers[INDEX_BUFFER]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, lPolygonCount * 3 * sizeof(unsigned int), lIndices, GL_STATIC_DRAW);
}

void NodeMesh::CreateVAO()
{
    GLUtil glutil;
    glGenVertexArrays(1, &mVertexArray);
    glBindVertexArray(mVertexArray);
    // Vertex Bufferの作成

    //GLuint mVertexBuffers[NUM_BUFFERS] = { 0 };
    //mVertexBuffers = new GLuint[NUM_BUFFERS];
    for (int i = 0; i < NUM_BUFFERS; i++) {
        mVertexBuffers[i] = 0;
    }
    //GLuint vertexBuffer;
    glGenBuffers(NUM_BUFFERS, mVertexBuffers);

    //const int positionNum = vnt->TriangleCount * 3;
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffers[POS_VB]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(mPositions[0]) * mPositions.size(), &mPositions[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glutil.GetErr();

    if (mNormals.size() != 0) {
        glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffers[NORMAL_VB]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(mNormals[0]) * mNormals.size(), &mNormals[0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
    }

    // UV
    if (mTexCoords.size() != 0) {
        glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffers[TEXCOORD_VB]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(mTexCoords[0]) * mTexCoords.size(), &mTexCoords[0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
    }

    // Index Buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVertexBuffers[INDEX_BUFFER]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * mIndices.size(), &mIndices[0], GL_STATIC_DRAW);

    // BoneIdx
    if (mOwnerMesh->GetIsSkinMesh()) {
        std::vector<glm::ivec4> BoneIndices;
        std::vector<glm::vec4> BoneWeights;
        mFBXSkeleton->GetBoneIdexWeightArray(BoneIndices, BoneWeights);

        if ((BoneIndices.size() != 0) && (BoneWeights.size() != 0)) {
            // Bind Buffer
            GLuint BoneBuffers[2] = { 0 };
            glGenBuffers(2, BoneBuffers);

            glBindBuffer(GL_ARRAY_BUFFER, BoneBuffers[0]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(glm::ivec4) * BoneIndices.size(), &BoneIndices[0], GL_STATIC_DRAW);
            glEnableVertexAttribArray(3);
            glVertexAttribIPointer(3, 4, GL_INT, 0, 0);

            glBindBuffer(GL_ARRAY_BUFFER, BoneBuffers[1]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * BoneWeights.size(), &BoneWeights[0], GL_STATIC_DRAW);
            glEnableVertexAttribArray(4);
            glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 0, 0);
        }
    }

    // unbind cube vertex arrays
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

NodeMesh::Material* NodeMesh::LoadMaterial(FbxSurfaceMaterial* material)
{
    //const FbxDouble3 lEmissive = GetMaterialProperty(pMaterial,
    //    FbxSurfaceMaterial::sEmissive, FbxSurfaceMaterial::sEmissiveFactor, mEmissive.mTextureName);
    //mEmissive.mColor[0] = static_cast<GLfloat>(lEmissive[0]);
    //mEmissive.mColor[1] = static_cast<GLfloat>(lEmissive[1]);
    //mEmissive.mColor[2] = static_cast<GLfloat>(lEmissive[2]);
    printf("%s\n", material->GetName());

    std::string AmbientTexName = "";
    std::string DiffuseTexName = "";
    std::string SpecTexName = "";
    Material* materialData = new Material;
    const FbxDouble3 lAmbient = GetMaterialProperty(material,
        FbxSurfaceMaterial::sAmbient, FbxSurfaceMaterial::sAmbientFactor, AmbientTexName);
    materialData->AmbientColor[0] = static_cast<float>(lAmbient[0]);
    materialData->AmbientColor[1] = static_cast<float>(lAmbient[1]);
    materialData->AmbientColor[2] = static_cast<float>(lAmbient[2]);

    const FbxDouble3 lDiffuse = GetMaterialProperty(material,
        FbxSurfaceMaterial::sDiffuse, FbxSurfaceMaterial::sDiffuseFactor, DiffuseTexName);
    materialData->DiffuseColor[0] = static_cast<GLfloat>(lDiffuse[0]);
    materialData->DiffuseColor[1] = static_cast<GLfloat>(lDiffuse[1]);
    materialData->DiffuseColor[2] = static_cast<GLfloat>(lDiffuse[2]);

    const FbxDouble3 lSpecular = GetMaterialProperty(material,
        FbxSurfaceMaterial::sSpecular, FbxSurfaceMaterial::sSpecularFactor, SpecTexName);
    materialData->SpecColor[0] = static_cast<GLfloat>(lSpecular[0]);
    materialData->SpecColor[1] = static_cast<GLfloat>(lSpecular[1]);
    materialData->SpecColor[2] = static_cast<GLfloat>(lSpecular[2]);

    FbxProperty lShininessProperty = material->FindProperty(FbxSurfaceMaterial::sShininess);
    if (lShininessProperty.IsValid())
    {
        double lShininess = lShininessProperty.Get<FbxDouble>();
        materialData->Shinness = static_cast<GLfloat>(lShininess);
    }

    //printf("ambient tex name: %s\n", AmbientTexName.c_str());
    //printf("diffuse tex name: %s\n", DiffuseTexName.c_str());
    //printf("spec tex name:    %s\n", SpecTexName.c_str());

    return materialData;
}

FbxDouble3 NodeMesh::GetMaterialProperty(const FbxSurfaceMaterial* pMaterial,
    const char* pPropertyName,
    const char* pFactorPropertyName,
    std::string& textureName)
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

    const int lTextureCount = lProperty.GetSrcObjectCount<FbxFileTexture>();
    if (lProperty.IsValid())
    {
        if (lTextureCount)
        {
            assert(lTextureCount == 1);

            const FbxFileTexture* lFileTexture = lProperty.GetSrcObject<FbxFileTexture>();
            if (lFileTexture) {
                const FbxString lFileName = lFileTexture->GetFileName();
                std::string texFileName = lFileName.Buffer();

                GLUtil glutil;
                char buffer[512];
                memset(buffer, 0, 512 * sizeof(char));
                memcpy(buffer, texFileName.c_str(), sizeof(char) * 512);
                glutil.Replace('\\', '/', buffer);
                std::vector<std::string> split_list;
                std::string replace_file_name = buffer;
                // 「/」で分解
                glutil.Split('/', buffer, split_list);
                textureName = split_list[split_list.size() - 1];
                printf("\ttexture name: %s\n", textureName.c_str());

            //    std::string texturePath = "./resources/" + mMeshName +"/Textures/" + split_list[split_list.size() - 1];
            //    Texture* textureData = new Texture(texturePath);
            //    tex = textureData;
            }

            //if (lTexture && lTexture->GetUserDataPtr())
            //{
            //    pTextureName = *(static_cast<GLuint*>(lTexture->GetUserDataPtr()));
            //}
        }
    }

    return lResult;
}

void NodeMesh::Draw(Shader* shader)
{
    unsigned int currentEelementCount = 0;
    if (mIsMesh) {
        glBindVertexArray(mVertexArray);
        //glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffers[POS_VB]);
        //glEnableClientState(GL_VERTEX_ARRAY);
        //glEnableVertexAttribArray(0);
        //glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

        for (int materialIdx = 0; materialIdx < mSubMeshes.size(); materialIdx++) {
            shader->UseProgram();
            //materialIdx = 2;
            // Bind Texture
            std::string materialName = mSubMeshes[materialIdx]->MaterialName;
            mOwnerMesh->BindTexture(materialName);
            //  Bind Material
            Material* material = mSubMeshes[materialIdx]->Material;
            if (material->AmbientColor != glm::vec3(0.f)) {
                shader->SetVectorUniform("matAmbientColor", material->AmbientColor);
            }
            else {
                shader->SetVectorUniform("matAmbientColor", glm::vec3(1.f));
            }
            if (material->DiffuseColor != glm::vec3(0.f)) {
                shader->SetVectorUniform("matDiffuseColor", material->DiffuseColor);
            }
            else {
                shader->SetVectorUniform("matDiffuseColor", glm::vec3(1.f));
            }
            if (material->SpecColor != glm::vec3(0.f)) {
                shader->SetVectorUniform("matSpecColor", material->SpecColor);
            }
            else {
                shader->SetVectorUniform("matSpecColor", glm::vec3(1.f));
            }


            GLsizei lOffset = mSubMeshes[materialIdx]->IndexOffset * sizeof(unsigned int);
            const GLsizei lElementCount = mSubMeshes[materialIdx]->TriangleCount * 3;

            glDrawElements(GL_TRIANGLES,
                lElementCount,
                GL_UNSIGNED_INT,
                reinterpret_cast<const GLvoid*>(lOffset));
            //glDrawElementsBaseVertex(GL_TRIANGLES,
            //    lElementCount,
            //    GL_UNSIGNED_INT,
            //    reinterpret_cast<const GLvoid*>(lOffset),
            //    currentEelementCount);
            currentEelementCount += lElementCount;
            //glDrawArrays(
            //    GL_TRIANGLES,
            //    0,
            //    mPositions.size()
            //);
            mOwnerMesh->UnBindTexture(materialName);
            //break;
        }


        glBindVertexArray(0);
    }

    for (auto child : mChilds) {
        child->Draw(shader);
    }
}

//
//bool NodeMesh::LoadMesh(FbxMesh* mesh)
//{
//    if (!mesh->GetNode()) {
//        return false;
//    }
//
//    const int lPolygonCount = mesh->GetPolygonCount();
//
//    int lPolygonVertexCount = lPolygonCount * 3;
//
//    FbxStringList lUVNames;
//    mesh->GetUVSetNames(lUVNames);
//    const char* lUVName = NULL;
//    if (hasUV && lUVNames.GetCount())
//    {
//        mTexCoords.resize(lPolygonVertexCount);
//        lUVName = lUVNames[0];
//    }
//
//    // Count the polygon count of each material
//    FbxLayerElementArrayTemplate<int>* lMaterialIndice = NULL;
//    FbxGeometryElement::EMappingMode lMaterialMappingMode = FbxGeometryElement::eNone;
//    assert(mesh->GetElementMaterial() != nullptr);
//    lMaterialIndice = &mesh->GetElementMaterial()->GetIndexArray();
//    lMaterialMappingMode = mesh->GetElementMaterial()->GetMappingMode();
//    if (lMaterialIndice && lMaterialMappingMode == FbxGeometryElement::eByPolygon)
//    {
//        FBX_ASSERT(lMaterialIndice->GetCount() == lPolygonCount);
//        if (lMaterialIndice->GetCount() == lPolygonCount)
//        {
//            // 各マテリアルのポリゴンの数を求める
//            for (int lPolygonIndex = 0; lPolygonIndex < lPolygonCount; ++lPolygonIndex)
//            {
//                const int lMaterialIndex = lMaterialIndice->GetAt(lPolygonIndex);
//                if (mSubMeshes.size() < lMaterialIndex + 1)
//                {
//                    mSubMeshes.resize(lMaterialIndex + 1);
//                }
//                if (vntArray.size() < lMaterialIndex + 1)
//                {
//                    vntArray.resize(lMaterialIndex + 1);
//                }
//
//                if (mSubMeshes[lMaterialIndex] == NULL)
//                {
//                    mSubMeshes[lMaterialIndex] = new BasicMeshEntry;
//                }
//                if (vntArray[lMaterialIndex] == NULL)
//                {
//                    vntArray[lMaterialIndex] = new VNTData();
//                }
//                vntArray[lMaterialIndex]->TriangleCount++;
//                mSubMeshes[lMaterialIndex]->TriangleCount += 1;
//            }
//
//            // Make sure we have no "holes" (NULL) in the mSubMeshes table. This can happen
//            // if, in the loop above, we resized the mSubMeshes by more than one slot.
//            for (int i = 0; i < mSubMeshes.size(); i++)
//            {
//                if (mSubMeshes[i] == NULL)
//                    mSubMeshes[i] = new BasicMeshEntry;
//            }
//
//            // 各マテリアルに対する頂点のオフセット導出
//            const int lMaterialCount = mSubMeshes.size();
//            int lOffset = 0;
//            int vntOffset = 0;
//            for (int lIndex = 0; lIndex < vntArray.size(); ++lIndex)
//            {
//                if (vntArray[lIndex] == NULL) {
//                    vntArray[lIndex] = new VNTData();
//                }
//                mSubMeshes[lIndex]->IndexOffset = lOffset;
//                vntArray[lIndex]->VNTOffset = vntOffset;
//                lOffset += mSubMeshes[lIndex]->TriangleCount * 3;
//                vntOffset += vntArray[lIndex]->TriangleCount * 3;
//
//
//                vntArray[lIndex]->Positions.resize(vntArray[lIndex]->TriangleCount * 3);
//                vntArray[lIndex]->Normals.resize(vntArray[lIndex]->TriangleCount * 3);
//                vntArray[lIndex]->TexCoords.resize(vntArray[lIndex]->TriangleCount * 3);
//
//                //vntArray[lIndex]->Positions = new glm::vec3[vntArray[lIndex]->TriangleCount * 3];
//                //vntArray[lIndex]->Normals = new glm::vec3[vntArray[lIndex]->TriangleCount * 3];
//                //vntArray[lIndex]->TexCoords = new glm::vec2[vntArray[lIndex]->TriangleCount * 3];
//
//                vntArray[lIndex]->TriangleCount = 0;
//                // This will be used as counter in the following procedures, reset to zero
//                mSubMeshes[lIndex]->TriangleCount = 0;
//            }
//            FBX_ASSERT(lOffset == lPolygonCount * 3);
//        }
//    }
//
//    assert(mesh->GetElementMaterial() != nullptr);
//
//    // All faces will use the same material.
//    if (mSubMeshes.size() == 0)
//    {
//        mSubMeshes.resize(1);
//        mSubMeshes[0] = new BasicMeshEntry();
//    }
//    // マテリアルの指定がないとき
//    if (vntArray.size() == 0)
//    {
//        vntArray.resize(1);
//        vntArray[0] = new VNTData();
//        vntArray[0]->Positions.resize(lPolygonCount * 3);
//        vntArray[0]->Normals.resize(lPolygonCount * 3);
//        vntArray[0]->TexCoords.resize(lPolygonCount * 3);
//
//        //vntArray[0]->Positions.resize(lPolygonCount * 3);
//        //vntArray[0]->Normals.resize(lPolygonCount * 3);
//        //vntArray[0]->TexCoords.resize(lPolygonCount * 3);
//    }
//
//
//    // Congregate all the data of a mesh to be cached in VBOs.
//    // If normal or UV is by polygon vertex, record all vertex attributes by polygon vertex.
//    bool hasNormal = mesh->GetElementNormalCount() > 0;
//    bool hasUV = mesh->GetElementUVCount() > 0;
//    FbxGeometryElement::EMappingMode lNormalMappingMode = FbxGeometryElement::eNone;
//    FbxGeometryElement::EMappingMode lUVMappingMode = FbxGeometryElement::eNone;
//    if (hasNormal)
//    {
//        lNormalMappingMode = mesh->GetElementNormal(0)->GetMappingMode();
//        if (lNormalMappingMode == FbxGeometryElement::eNone)
//        {
//            hasNormal = false;
//        }
//    }
//    if (hasUV)
//    {
//        lUVMappingMode = mesh->GetElementUV(0)->GetMappingMode();
//        if (lUVMappingMode == FbxGeometryElement::eNone)
//        {
//            hasUV = false;
//        }
//    }
//
//    // Allocate the array memory, by control point or by polygon vertex.
//    int lPolygonVertexCount = mesh->GetControlPointsCount();
//    lPolygonVertexCount = lPolygonCount * 3;
//
//
//    //float* lVertices = new float[lPolygonVertexCount * VERTEX_STRIDE];
//
//    //int currentPositionSize = mPositions.size();
//    //mPositions.resize(currentPositionSize + lPolygonVertexCount);
//    //unsigned int* lIndices = new unsigned int[lPolygonCount * 3];
//    float* lNormals = NULL;
//    int currentNormalSize = mNormals.size();
//    if (hasNormal)
//    {
//        //lNormals = new float[lPolygonVertexCount * NORMAL_STRIDE];
//        mNormals.resize(currentNormalSize + lPolygonVertexCount);
//    }
//    int currentUVSize = mTexCoords.size();
//    FbxStringList lUVNames;
//    mesh->GetUVSetNames(lUVNames);
//    const char* lUVName = NULL;
//    if (hasUV && lUVNames.GetCount())
//    {
//        mTexCoords.resize(currentUVSize + lPolygonVertexCount);
//        lUVName = lUVNames[0];
//    }
//
//    // Populate the array with vertex attribute, if by control point.
//    const FbxVector4* lControlPoints = mesh->GetControlPoints();
//    FbxVector4 lCurrentVertex;
//    FbxVector4 lCurrentNormal;
//    FbxVector2 lCurrentUV;
//
//
//
//    for (int lPolygonIndex = 0; lPolygonIndex < lPolygonCount; ++lPolygonIndex)
//    {
//        // The material for current face.
//        int lMaterialIndex = 0;
//        if (lMaterialIndice && lMaterialMappingMode == FbxGeometryElement::eByPolygon)
//        {
//            lMaterialIndex = lMaterialIndice->GetAt(lPolygonIndex);
//        }
//
//        if (vntArray.size() < lMaterialIndex + 1)
//        {
//            vntArray.resize(lMaterialIndex + 1);
//        }
//
//        //const int vntOffset = vntArray[lMaterialIndex]->VNTOffset +
//        //    vntArray[lMaterialIndex]->TriangleCount * 3;
//        const int vntOffset = vntArray[lMaterialIndex]->TriangleCount * 3;
//
//        // Where should I save the vertex attribute index, according to the material
//        for (int lVerticeIndex = 0; lVerticeIndex < 3; ++lVerticeIndex)
//        {
//            const int lControlPointIndex = mesh->GetPolygonVertex(lPolygonIndex, lVerticeIndex);
//            // If the lControlPointIndex is -1, we probably have a corrupted mesh data. At this point,
//            // it is not guaranteed that the cache will work as expected.
//            if (lControlPointIndex >= 0)
//            {
//                lCurrentVertex = lControlPoints[lControlPointIndex];
//
//                vntArray[lMaterialIndex]->Positions[vntOffset + lVerticeIndex] = glm::vec3(
//                    static_cast<float>(lCurrentVertex[0]),
//                    static_cast<float>(lCurrentVertex[1]),
//                    static_cast<float>(lCurrentVertex[2])
//                );
//
//                if (hasNormal) {
//                    mesh->GetPolygonVertexNormal(lPolygonIndex, lVerticeIndex, lCurrentNormal);
//                    vntArray[lMaterialIndex]->Normals[vntOffset + lVerticeIndex] = glm::vec3(
//                        static_cast<float>(lCurrentNormal[0]),
//                        static_cast<float>(lCurrentNormal[1]),
//                        static_cast<float>(lCurrentNormal[2])
//                    );
//                }
//
//                if (hasUV) {
//                    bool lUnmappedUV;
//                    mesh->GetPolygonVertexUV(lPolygonIndex, lVerticeIndex, lUVName, lCurrentUV, lUnmappedUV);
//                    vntArray[lMaterialIndex]->TexCoords[vntOffset + lVerticeIndex] = glm::vec2(
//                        static_cast<float>(lCurrentUV[0]),
//                        static_cast<float>(lCurrentUV[1])
//                    );
//                }
//
//            }
//        }
//        vntArray[lMaterialIndex]->TriangleCount++;
//    }
//
//    // Position, Normal, Texcoordsの数はすべて同じはず
//
//
//    // VAO作成
//    dedeMaterialVNT* NodeMeshes = new dedeMaterialVNT[vntArray.size()];
//    VAO* vao = nullptr;
//    for (int i = 0; i < vntArray.size(); i++) {
//        VNTData* vnt = vntArray[i];
//
//        vao = new VAO();
//        vao->SetVNT(vnt->Positions, vnt->Normals, vnt->TexCoords);
//        vao->CreateVAO();
//        mVAOs.push_back(vao);
//
//        //assert((vnt->Normals.size() == vnt->Positions.size()) && (vnt->Normals.size() == vnt->TexCoords.size()));
//
//        dedeMaterialVNT nm;
//        glGenVertexArrays(1, &nm.VertexArray);
//        glBindVertexArray(nm.VertexArray);
//
//        // Vertex Bufferの作成
//        enum BUFFER_TYPE {
//            INDEX_BUFFER = 0,
//            POS_VB = 1,
//            TEXCOORD_VB = 2,
//            NORMAL_VB = 3,
//            NUM_BUFFERS = 4,  // required only for instancing
//        };
//        GLuint m_Buffers[NUM_BUFFERS] = { 0 };
//        glGenBuffers(1, m_Buffers);
//
//        const int positionNum = vnt->TriangleCount * 3;
//        glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[POS_VB]);
//        glBufferData(GL_ARRAY_BUFFER, sizeof(vnt->Positions[0]) * positionNum, &vnt->Positions[0], GL_STATIC_DRAW);
//        glEnableVertexAttribArray(0);
//        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
//
//        if (vnt->Normals.size() != 0) {
//            glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[NORMAL_VB]);
//            glBufferData(GL_ARRAY_BUFFER, sizeof(vnt->Normals[0]) * positionNum, &vnt->Normals[0], GL_STATIC_DRAW);
//            glEnableVertexAttribArray(1);
//            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
//        }
//
//        // UV
//        if (vnt->TexCoords.size() != 0) {
//            glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[TEXCOORD_VB]);
//            glBufferData(GL_ARRAY_BUFFER, sizeof(vnt->TexCoords[0]) * positionNum, &vnt->TexCoords[0], GL_STATIC_DRAW);
//            glEnableVertexAttribArray(2);
//            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
//        }
//
//        // unbind cube vertex arrays
//        glBindVertexArray(0);
//        glBindBuffer(GL_ARRAY_BUFFER, 0);
//        //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
//
//        //nm.VertexCount = vnt->Positions.size() * 3;
//        nm.VertexCount = positionNum * 3;
//        nm.VertexBuffers = m_Buffers;
//        nm.Positions = vnt->Positions;
//        nm.Normals = vnt->Normals;
//        nm.TexCoords = vnt->TexCoords;
//
//        NodeMeshes[i] = nm;
//    }
//
//    return NodeMeshes;
//}
