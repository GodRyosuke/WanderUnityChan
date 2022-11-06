#include "FBXMesh.hpp"
#include "glew.h"
#include "Shader.hpp"
#include "GLUtil.hpp"
#include "Texture.hpp"

FBXMesh::FBXMesh()
{
    mPositions.resize(0);
    mNormals.resize(0);
    mTexCoords.resize(0);
    mIndices.resize(0);
}

FBXMesh::~FBXMesh()
{

}

static std::vector<int> pcount(6);

bool FBXMesh::Load(std::string fileName)
{
    mMeshFileName = fileName;

    // マネージャを生成
    mManager = FbxManager::Create();

    // IOSettingを生成
    FbxIOSettings* ioSettings = FbxIOSettings::Create(mManager, IOSROOT);

    // Importerを生成
    FbxImporter* importer = FbxImporter::Create(mManager, "");
    std::string filePath = "./resources/" + fileName + "/" + fileName + ".fbx";
    if (importer->Initialize(filePath.c_str(), -1, mManager->GetIOSettings()) == false) {
        // インポートエラー
        return -1;
    }

    // SceneオブジェクトにFBXファイル内の情報を流し込む
    FbxScene* scene = FbxScene::Create(mManager, "scene");
    importer->Import(scene);
    importer->Destroy(); // シーンを流し込んだらImporterは解放してOK

    // 三角面化
    FbxGeometryConverter geometryConv = FbxGeometryConverter(mManager);
    geometryConv.Triangulate(scene, true);

    // Scene解析
    // Material 読み込み
    int materialCount = scene->GetMaterialCount();
    for (int i = 0; i < materialCount; i++) {
        FbxSurfaceMaterial* material = scene->GetMaterial(i);
        LoadMaterial(material);
    }

    FbxNode* rootNode = scene->GetRootNode();
    if (rootNode == 0) {
        printf("error: cannont find root node: %s\n", fileName.c_str());
        return false;
    }
    //ShowNodeNames(rootNode, 0);
    LoadNode(rootNode);

    // Create VAO
    glGenVertexArrays(1, &mVertexArray);
    glBindVertexArray(mVertexArray);

    // Vertex Bufferの作成
    PopulateBuffers();

    // unbind cube vertex arrays
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    DrawArrayPB();

    // マネージャ解放
    // 関連するすべてのオブジェクトが解放される
    mManager->Destroy();

	return true;
}

void FBXMesh::LoadMaterial(FbxSurfaceMaterial* material)
{
    Material MaterialData;
    MaterialData.Name = material->GetName();
    printf("material name: %s\n", material->GetName());
    FbxProperty fbxProperty = material->FindProperty(FbxSurfaceMaterial::sDiffuse);
    int textureCount = fbxProperty.GetSrcObjectCount<FbxFileTexture>();

    const FbxImplementation* fbxImp = GetImplementation(
        material,
        FBXSDK_IMPLEMENTATION_CGFX
    );
    if (fbxImp) {
        const FbxBindingTable* rootTable = fbxImp->GetRootTable();
        int entryCount = static_cast<int>(rootTable->GetEntryCount());
        for (int i = 0; i < entryCount; i++) {
            FbxBindingTableEntry const& entry = rootTable->GetEntry(i);
            FbxProperty hierachical = material->RootProperty.FindHierarchical(
                entry.GetSource()
            );
            int fileTexCount = hierachical.GetSrcObjectCount<FbxFileTexture>();

            for (int texIdx = 0; texIdx < fileTexCount; texIdx++) {
                FbxFileTexture* fileTex = hierachical.GetSrcObject<FbxFileTexture>(texIdx);
                std::string texturePathData = fileTex->GetFileName();
                GLUtil glutil;
                char buffer[512];
                memset(buffer, 0, 512 * sizeof(char));
                memcpy(buffer, texturePathData.c_str(), sizeof(char) * 512);
                glutil.Replace('\\', '/', buffer);
                std::vector<std::string> split_list;
                std::string replace_file_name = buffer;
                // 「/」で分解
                glutil.Split('/', buffer, split_list);

                std::string texturePath = "./resources/" + mMeshFileName + "/Textures/" + split_list[split_list.size() - 1];
                Texture* tex = new Texture(texturePath);
                MaterialData.Textures.push_back(tex);
                //printf("%s\n", fileTex->GetFileName());
            }
        }
    }

    mMaterials.push_back(MaterialData);
}

void FBXMesh::LoadNode(FbxNode* node)
{
    const char* typeNames[] = {
        "eUnknown", "eNull", "eMarker", "eSkeleton", "eMesh", "eNurbs",
        "ePatch", "eCamera", "eCameraStereo", "eCameraSwitcher", "eLight",
        "eOpticalReference", "eOpticalMarker", "eNurbsCurve", "eTrimNurbsSurface",
        "eBoundary", "eNurbsSurface", "eShape", "eLODGroup", "eSubDiv",
        "eCachedEffect", "eLine"
    };
    std::string NodeName = node->GetName();
    //printf("NodeName: %s\n", NodeName.c_str());
    int attrCount = node->GetNodeAttributeCount();
    for (int i = 0; i < attrCount; ++i) {
        FbxNodeAttribute* attr = node->GetNodeAttributeByIndex(i);
        FbxNodeAttribute::EType type = attr->GetAttributeType();
        if (type == FbxNodeAttribute::EType::eMesh) {   // Mesh Nodeなら
            FbxMesh* pMesh = static_cast<FbxMesh*>(attr);
            //LoadMesh(pMesh);
            LoadMeshElement(pMesh);
            
            int materialCount = node->GetMaterialCount();
            for (int i = 0; i < materialCount; i++) {
                FbxSurfaceMaterial* material = node->GetMaterial(i);
                LoadMaterial(material);
            }
        }
    }

    int childCount = node->GetChildCount();
    for (int i = 0; i < childCount; ++i) {
        LoadNode(node->GetChild(i));
    }
}

bool FBXMesh::LoadMeshElement(FbxMesh* mesh)
{
    if (!mesh->GetNode())
        return false;

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
                        mSubMeshes[lMaterialIndex] = new BasicMeshEntry;
                    }
                    mSubMeshes[lMaterialIndex]->TriangleCount += 1;
                }

                // Make sure we have no "holes" (NULL) in the mSubMeshes table. This can happen
                // if, in the loop above, we resized the mSubMeshes by more than one slot.
                for (int i = 0; i < mSubMeshes.size(); i++)
                {
                    if (mSubMeshes[i] == NULL)
                        mSubMeshes[i] = new BasicMeshEntry;
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
        mSubMeshes[0] = new BasicMeshEntry();
    }

    // Congregate all the data of a mesh to be cached in VBOs.
    // If normal or UV is by polygon vertex, record all vertex attributes by polygon vertex.
    bool isAllByControlPoint = true;
    bool hasNormal = mesh->GetElementNormalCount() > 0;
    bool hasUV = mesh->GetElementUVCount() > 0;
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
            isAllByControlPoint = false;
        }
    }
    if (hasUV)
    {
        lUVMappingMode = mesh->GetElementUV(0)->GetMappingMode();
        if (lUVMappingMode == FbxGeometryElement::eNone)
        {
            hasUV = false;
        }
        if (hasUV && lUVMappingMode != FbxGeometryElement::eByControlPoint)
        {
            isAllByControlPoint = false;
        }
    }

    // Allocate the array memory, by control point or by polygon vertex.
    int lPolygonVertexCount = mesh->GetControlPointsCount();
    if (!isAllByControlPoint)
    {
        lPolygonVertexCount = lPolygonCount * 3;
    }
    //float* lVertices = new float[lPolygonVertexCount * VERTEX_STRIDE];
    int currentPositionSize = mPositions.size();
    mPositions.resize(currentPositionSize + lPolygonVertexCount);
    unsigned int* lIndices = new unsigned int[lPolygonCount * 3];
    float* lNormals = NULL;
    int currentNormalSize = mNormals.size();
    if (hasNormal)
    {
        //lNormals = new float[lPolygonVertexCount * NORMAL_STRIDE];
        mNormals.resize(currentNormalSize + lPolygonVertexCount);
    }
    int currentUVSize = mTexCoords.size();
    float* lUVs = NULL;
    FbxStringList lUVNames;
    mesh->GetUVSetNames(lUVNames);
    const char* lUVName = NULL;
    if (hasUV && lUVNames.GetCount())
    {
        mTexCoords.resize(currentUVSize + lPolygonVertexCount);
        //lUVs = new float[lPolygonVertexCount * UV_STRIDE];
        lUVName = lUVNames[0];
    }

    // Populate the array with vertex attribute, if by control point.
    const FbxVector4* lControlPoints = mesh->GetControlPoints();
    FbxVector4 lCurrentVertex;
    FbxVector4 lCurrentNormal;
    FbxVector2 lCurrentUV;
    if (isAllByControlPoint)
    {
        const FbxGeometryElementNormal* lNormalElement = NULL;
        const FbxGeometryElementUV* lUVElement = NULL;
        if (hasNormal)
        {
            lNormalElement = mesh->GetElementNormal(0);
        }
        if (hasUV)
        {
            lUVElement = mesh->GetElementUV(0);
        }


        for (int lIndex = 0; lIndex < lPolygonVertexCount; ++lIndex)
        {
            // Save the vertex position.
            lCurrentVertex = lControlPoints[lIndex];
            
            mPositions[currentPositionSize + lIndex] = glm::vec3(
                static_cast<float>(lCurrentVertex[0]),
                static_cast<float>(lCurrentVertex[1]),
                static_cast<float>(lCurrentVertex[2])
            );

            //lVertices[lIndex * VERTEX_STRIDE] = static_cast<float>(lCurrentVertex[0]);
            //lVertices[lIndex * VERTEX_STRIDE + 1] = static_cast<float>(lCurrentVertex[1]);
            //lVertices[lIndex * VERTEX_STRIDE + 2] = static_cast<float>(lCurrentVertex[2]);
            //lVertices[lIndex * VERTEX_STRIDE + 3] = 1;

            // Save the normal.
            if (hasNormal)
            {
                int lNormalIndex = lIndex;
                if (lNormalElement->GetReferenceMode() == FbxLayerElement::eIndexToDirect)
                {
                    lNormalIndex = lNormalElement->GetIndexArray().GetAt(lIndex);
                }
                lCurrentNormal = lNormalElement->GetDirectArray().GetAt(lNormalIndex);
                mNormals[currentNormalSize + lIndex] = glm::vec3(
                    static_cast<float>(lCurrentNormal[0]),
                    static_cast<float>(lCurrentNormal[1]),
                    static_cast<float>(lCurrentNormal[2])
                );
                //lNormals[lIndex * NORMAL_STRIDE] = static_cast<float>(lCurrentNormal[0]);
                //lNormals[lIndex * NORMAL_STRIDE + 1] = static_cast<float>(lCurrentNormal[1]);
                //lNormals[lIndex * NORMAL_STRIDE + 2] = static_cast<float>(lCurrentNormal[2]);
            }

            // Save the UV.
            if (hasUV)
            {
                int lUVIndex = lIndex;
                if (lUVElement->GetReferenceMode() == FbxLayerElement::eIndexToDirect)
                {
                    lUVIndex = lUVElement->GetIndexArray().GetAt(lIndex);
                }
                lCurrentUV = lUVElement->GetDirectArray().GetAt(lUVIndex);
                mTexCoords[currentUVSize + lIndex] = glm::vec2(
                    static_cast<float>(lCurrentUV[0]),
                    static_cast<float>(lCurrentUV[1])
                );
                //lUVs[lIndex * UV_STRIDE] = static_cast<float>(lCurrentUV[0]);
                //lUVs[lIndex * UV_STRIDE + 1] = static_cast<float>(lCurrentUV[1]);
            }
        }
    }


    int lVertexCount = 0;
    for (int lPolygonIndex = 0; lPolygonIndex < lPolygonCount; ++lPolygonIndex)
    {
        // The material for current face.
        int lMaterialIndex = 0;
        if (lMaterialIndice && lMaterialMappingMode == FbxGeometryElement::eByPolygon)
        {
            lMaterialIndex = lMaterialIndice->GetAt(lPolygonIndex);
        }

        // Where should I save the vertex attribute index, according to the material
        const int lIndexOffset = mSubMeshes[lMaterialIndex]->IndexOffset +
            mSubMeshes[lMaterialIndex]->TriangleCount * 3;
        for (int lVerticeIndex = 0; lVerticeIndex < 3; ++lVerticeIndex)
        {
            const int lControlPointIndex = mesh->GetPolygonVertex(lPolygonIndex, lVerticeIndex);
            // If the lControlPointIndex is -1, we probably have a corrupted mesh data. At this point,
            // it is not guaranteed that the cache will work as expected.
            if (lControlPointIndex >= 0)
            {
                if (isAllByControlPoint)
                {
                    lIndices[lIndexOffset + lVerticeIndex] = static_cast<unsigned int>(lControlPointIndex);
                }
                // Populate the array with vertex attribute, if by polygon vertex.
                else
                {
                    lIndices[lIndexOffset + lVerticeIndex] = static_cast<unsigned int>(lVertexCount);

                    lCurrentVertex = lControlPoints[lControlPointIndex];
                    lVertices[lVertexCount * VERTEX_STRIDE] = static_cast<float>(lCurrentVertex[0]);
                    lVertices[lVertexCount * VERTEX_STRIDE + 1] = static_cast<float>(lCurrentVertex[1]);
                    lVertices[lVertexCount * VERTEX_STRIDE + 2] = static_cast<float>(lCurrentVertex[2]);
                    lVertices[lVertexCount * VERTEX_STRIDE + 3] = 1;

                    if (mHasNormal)
                    {
                        pMesh->GetPolygonVertexNormal(lPolygonIndex, lVerticeIndex, lCurrentNormal);
                        lNormals[lVertexCount * NORMAL_STRIDE] = static_cast<float>(lCurrentNormal[0]);
                        lNormals[lVertexCount * NORMAL_STRIDE + 1] = static_cast<float>(lCurrentNormal[1]);
                        lNormals[lVertexCount * NORMAL_STRIDE + 2] = static_cast<float>(lCurrentNormal[2]);
                    }

                    if (mHasUV)
                    {
                        bool lUnmappedUV;
                        pMesh->GetPolygonVertexUV(lPolygonIndex, lVerticeIndex, lUVName, lCurrentUV, lUnmappedUV);
                        lUVs[lVertexCount * UV_STRIDE] = static_cast<float>(lCurrentUV[0]);
                        lUVs[lVertexCount * UV_STRIDE + 1] = static_cast<float>(lCurrentUV[1]);
                    }
                }
            }
            ++lVertexCount;
        }
        mSubMeshes[lMaterialIndex]->TriangleCount += 1;
    }
}

void FBXMesh::LoadMesh(FbxMesh* mesh)
{
    const int PolygonNum = mesh->GetPolygonCount();


    // Mesh Nodeの読み込み処理

    int PolygonVertexNum = mesh->GetPolygonVertexCount();
    int* IndexAry = mesh->GetPolygonVertices();

    int currentPosSize = mPositions.size();
    mPositions.resize(currentPosSize + PolygonNum * 3);  // 3角形ポリゴンと仮定
    int currentNormalSize = mNormals.size();
    mNormals.resize(currentNormalSize + PolygonNum * 3);
    int currentUVSize = mTexCoords.size();
    mTexCoords.resize(currentUVSize + PolygonNum * 3);
    int currentIndicesSize = mIndices.size();
    mIndices.resize(currentIndicesSize + PolygonNum * 3);
    for (int p = 0; p < PolygonNum; p++) {
        //mIndices.push_back(p * 3);
        //mIndices.push_back(p * 3 + 1);
        //mIndices.push_back(p * 3 + 2);
        int IndexNumInPolygon = mesh->GetPolygonSize(p);  // p番目のポリゴンの頂点数
        for (int n = 0; n < IndexNumInPolygon; n++) {
            mIndices.push_back((p * 3 + n));
            // ポリゴンpを構成するn番目の頂点のインデックス番号
            int IndexNumber = mesh->GetPolygonVertex(p, n);
            //mIndices.push_back(IndexNumber);

            // 頂点を読みだす
            FbxVector4 position = mesh->GetControlPointAt(IndexNumber);
            mPositions[currentPosSize + p * 3 + n] = glm::vec3(
                static_cast<float>(position[0]),
                static_cast<float>(position[1]),
                static_cast<float>(position[2])
            );

            // 法線情報の取得
            FbxVector4 normal;
            mesh->GetPolygonVertexNormal(p, n, normal);
            mNormals[currentNormalSize + p * 3 + n] = glm::vec3(
                static_cast<float>(normal[0]),
                static_cast<float>(normal[1]),
                static_cast<float>(normal[2])
            );

            // UV情報の取得
            FbxStringList uvSetNameList;
            mesh->GetUVSetNames(uvSetNameList);
            const char* uvSetName = uvSetNameList.GetStringAt(p);
            bool unMapped;
            FbxVector2 uv;
            mesh->GetPolygonVertexUV(
                p,
                n,
                uvSetName,
                uv,
                unMapped);
            mTexCoords[currentUVSize + p * 3 + n] = glm::vec2(
                static_cast<float>(uv[0]),
                static_cast<float>(uv[1])
            );
        }
    }



    // 頂点データの取得
    int controlNum = mesh->GetControlPointsCount();   // 頂点数
    FbxVector4* src = mesh->GetControlPoints();    // 頂点座標配列
    //for (int i = 0; i < controlNum; ++i) {
    //    mPositions.push_back(glm::vec3(src[i][0], src[i][1], src[i][2]));
    //}

    int layerNum = mesh->GetLayerCount();
    for (int i = 0; i < layerNum; ++i) {
        FbxLayer* layer = mesh->GetLayer(i);

        // 法線情報の取得
        FbxLayerElementNormal* normalElem = layer->GetNormals();
        if (normalElem != 0) {
            //LoadNormal(normalElem);
        }
        // UV情報を取得
        FbxLayerElementUV* uvElem = layer->GetUVs();
        if (uvElem != 0) {
            //LoadUV(uvElem);
        }
    }
}

void FBXMesh::LoadNormal(FbxLayerElementNormal* normalElem)
{
    // 法線の数・インデックス
    int normalNum = normalElem->GetDirectArray().GetCount();
    int indexNum = normalElem->GetIndexArray().GetCount();

    // マッピングモード・リファレンスモード取得
    FbxLayerElement::EMappingMode mappingMode = normalElem->GetMappingMode();
    FbxLayerElement::EReferenceMode refMode = normalElem->GetReferenceMode();
    assert(refMode == FbxLayerElement::eDirect);    // eDirectじゃないと対応できない
    if (mappingMode == FbxLayerElement::eByPolygonVertex) {
        if (refMode == FbxLayerElement::eDirect) {
            // 直接取得
            for (int i = 0; i < normalNum; ++i) {
                mNormals.push_back(glm::vec3(
                    static_cast<float>(normalElem->GetDirectArray().GetAt(i)[0]),
                    static_cast<float>(normalElem->GetDirectArray().GetAt(i)[1]),
                    static_cast<float>(normalElem->GetDirectArray().GetAt(i)[2])
                ));
            }
        }
    }
    else if (mappingMode == FbxLayerElement::eByControlPoint) {
        if (refMode == FbxLayerElement::eDirect) {
            // 直接取得
            for (int i = 0; i < normalNum; ++i) {
                mNormals.push_back(glm::vec3(
                    static_cast<float>(normalElem->GetDirectArray().GetAt(i)[0]),
                    static_cast<float>(normalElem->GetDirectArray().GetAt(i)[1]),
                    static_cast<float>(normalElem->GetDirectArray().GetAt(i)[2])
                ));
            }
        }
    }
}

void FBXMesh::LoadUV(FbxLayerElementUV* uvElement)
{
    // UVの数・インデックス
    int UVNum = uvElement->GetDirectArray().GetCount();
    int indexNum = uvElement->GetIndexArray().GetCount();
    int size = UVNum > indexNum ? UVNum : indexNum;

    // マッピングモード・リファレンスモード別にUV取得
    FbxLayerElementUV::EMappingMode mappingMode = uvElement->GetMappingMode();
    FbxLayerElementUV::EReferenceMode refMode = uvElement->GetReferenceMode();
    if ((mappingMode == FbxLayerElementUV::eByPolygonVertex) || (mappingMode == FbxLayerElementUV::eByControlPoint)) {
        if (refMode == FbxLayerElementUV::eDirect) {
            // 直接取得
            for (int i = 0; i < size; ++i) {
                mTexCoords.push_back(glm::vec2(
                    static_cast<float>(uvElement->GetDirectArray().GetAt(i)[0]),
                    static_cast<float>(uvElement->GetDirectArray().GetAt(i)[1])
                ));
            }
        }
        else if (refMode == FbxLayerElementUV::eIndexToDirect) {
            // インデックスから取得
            for (int i = 0; i < size; ++i) {
                int index = uvElement->GetIndexArray().GetAt(i);
                mTexCoords.push_back(glm::vec2(
                    static_cast<float>((uvElement->GetDirectArray().GetAt(index)[0])),
                    static_cast<float>((uvElement->GetDirectArray().GetAt(index)[1]))
                ));
            }
        }
    }
}

void FBXMesh::PopulateBuffers()
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
    glBufferData(GL_ARRAY_BUFFER, sizeof(mPositions[0]) * mPositions.size(), &mPositions[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // Normal Map
    glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[NORMAL_VB]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(mNormals[0]) * mNormals.size(), &mNormals[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // UV
    glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[TEXCOORD_VB]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(mTexCoords[0]) * mTexCoords.size(), &mTexCoords[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

    // Index Buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Buffers[INDEX_BUFFER]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(mIndices[0]) * mIndices.size(), &mIndices[0], GL_STATIC_DRAW);
}

void FBXMesh::DrawArrayPB()
{
    glGenVertexArrays(1, &mDrawArrayVAO);
    glBindVertexArray(mDrawArrayVAO);

    // Vertex Bufferの作成
    enum BUFFER_TYPE {
        INDEX_BUFFER = 0,
        POS_VB = 1,
        TEXCOORD_VB = 2,
        NORMAL_VB = 3,
        NUM_BUFFERS = 4,  // required only for instancing
    };
    GLuint m_Buffers[NUM_BUFFERS] = { 0 };
    glGenBuffers(1, m_Buffers);

    glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[POS_VB]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(mPositions[0]) * mPositions.size(), &mPositions[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[NORMAL_VB]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(mNormals[0])* mNormals.size(), &mNormals[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // Indef Buffer
    unsigned int IndexBuffer;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Buffers[INDEX_BUFFER]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(mIndices[0]) * mIndices.size(), &mIndices[0], GL_STATIC_DRAW);


    // unbind cube vertex arrays
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void FBXMesh::BindVertexArray()
{
    glBindVertexArray(mVertexArray);
}

void FBXMesh::UnBindVertexArray()
{
    glBindVertexArray(0);
}

void FBXMesh::Draw(Shader* shader)
{
    //glDrawElementsBaseVertex(GL_TRIANGLES,
    //    mIndices.size(),
    //    GL_UNSIGNED_INT,
    //    (void*)(mIndices.data()),
    //    mPositions.size() * 3);

    glBindVertexArray(mVertexArray);
    glDrawElements(GL_TRIANGLES,
        mIndices.size() * 3,
        GL_UNSIGNED_INT,
        (void*)(0));
}

void FBXMesh::DrawArray()
{
    glBindVertexArray(mVertexArray);
    glDrawArrays(
        GL_TRIANGLES,
        0,
        mPositions.size() * 3
    );
}

void FBXMesh::ShowNodeNames(FbxNode* node, int indent)
{
    // indentの数だけ空白を描画
    for (int i = 0; i < indent; i++) {
        printf(" ");
    }

    const char* typeNames[] = {
        "eUnknown", "eNull", "eMarker", "eSkeleton", "eMesh", "eNurbs",
        "ePatch", "eCamera", "eCameraStereo", "eCameraSwitcher", "eLight",
        "eOpticalReference", "eOpticalMarker", "eNurbsCurve", "eTrimNurbsSurface",
        "eBoundary", "eNurbsSurface", "eShape", "eLODGroup", "eSubDiv",
        "eCachedEffect", "eLine"
    };
    const char* name = node->GetName();
    int attrCount = node->GetNodeAttributeCount();
    if (attrCount == 0) {
        printf("%s\n", name);
    }
    else {
        printf("%s (", name);
    }
    for (int i = 0; i < attrCount; ++i) {
        FbxNodeAttribute* attr = node->GetNodeAttributeByIndex(i);
        FbxNodeAttribute::EType type = attr->GetAttributeType();
        printf("%s", typeNames[type]);
        if (i + 1 == attrCount) {
            printf(")\n");
        }
        else {
            printf(", ");
        }
    }

    int childCount = node->GetChildCount();
    for (int i = 0; i < childCount; ++i) {
        ShowNodeNames(node->GetChild(i), indent + 1);
    }
}

