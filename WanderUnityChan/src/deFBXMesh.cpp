#include "deFBXMesh.hpp"
#include "glad/glad.h"
#include "Game.hpp"
#include "UnityChan.hpp"
#include "Shader.hpp"
#include "GLUtil.hpp"
#include "Texture.hpp"
#include "NodeMesh.hpp"
#include <fstream>

deFBXMesh::deFBXMesh(class UnityChan* unitychan, bool setIsDrawArray, bool isSkeletal)
    :mIsDrawArray(setIsDrawArray)
    ,mIsSkeletal(isSkeletal)
    ,mUnityChan(unitychan)
{
    mPositions.resize(0);
    mNormals.resize(0);
    mTexCoords.resize(0);
    mIndices.resize(0);
}

deFBXMesh::~deFBXMesh()
{

}

static std::vector<int> pcount(6);

bool deFBXMesh::Load(std::string folderPath, std::string fileName)
{
    mMeshFileName = fileName;

    // マネージャを生成
    mManager = FbxManager::Create();

    // IOSettingを生成
    FbxIOSettings* ioSettings = FbxIOSettings::Create(mManager, IOSROOT);
    mManager->SetIOSettings(ioSettings);
    {
        FbxString lPath = FbxGetApplicationDirectory();
        mManager->LoadPluginsDirectory(lPath.Buffer());
    }
    FbxScene* scene = FbxScene::Create(mManager, "scene");


    // Importerを生成
    FbxImporter* importer = FbxImporter::Create(mManager, "");
    std::string filePath = folderPath + fileName + ".fbx";
    int lFileFormat = -1;
    if (!mManager->GetIOPluginRegistry()->DetectReaderFileFormat(filePath.c_str(), lFileFormat))
    {
        // Unrecognizable file format. Try to fall back to FbxImporter::eFBX_BINARY
        lFileFormat = mManager->GetIOPluginRegistry()->FindReaderIDByDescription("FBX binary (*.fbx)");;
    }

    // Initialize the importer by providing a filename.
    if (importer->Initialize(filePath.c_str(), lFileFormat) == true)
    {
        printf("Importing file...\n");
    }
    else
    {
        printf("Failed to open file: %s\n", fileName.c_str());
        return false;
    }


    // SceneオブジェクトにFBXファイル内の情報を流し込む
    importer->Import(scene);
    {
        // Check the scene integrity!
        FbxStatus status;
        FbxArray< FbxString*> details;
        FbxSceneCheckUtility sceneCheck(FbxCast<FbxScene>(scene), &status, &details);
        bool lNotify = (!sceneCheck.Validate(FbxSceneCheckUtility::eCkeckData) && details.GetCount() > 0) || (importer->GetStatus().GetCode() != FbxStatus::eSuccess);
        if (lNotify)
        {
            FBXSDK_printf("\n");
            FBXSDK_printf("********************************************************************************\n");
            if (details.GetCount())
            {
                FBXSDK_printf("Scene integrity verification failed with the following errors:\n");

                for (int i = 0; i < details.GetCount(); i++)
                    FBXSDK_printf("   %s\n", details[i]->Buffer());

                FbxArrayDelete<FbxString*>(details);
            }

            if (importer->GetStatus().GetCode() != FbxStatus::eSuccess)
            {
                FBXSDK_printf("\n");
                FBXSDK_printf("WARNING:\n");
                FBXSDK_printf("   The importer was able to read the file but with errors.\n");
                FBXSDK_printf("   Loaded scene may be incomplete.\n\n");
                FBXSDK_printf("   Last error message:'%s'\n", importer->GetStatus().GetErrorString());
            }

            FBXSDK_printf("********************************************************************************\n");
            FBXSDK_printf("\n");
        }
    }


    FbxAxisSystem SceneAxisSystem = scene->GetGlobalSettings().GetAxisSystem();
    FbxAxisSystem OurAxisSystem(FbxAxisSystem::eYAxis, FbxAxisSystem::eParityOdd, FbxAxisSystem::eRightHanded);
    if (SceneAxisSystem != OurAxisSystem)
    {
        OurAxisSystem.ConvertScene(scene);
    }

    // Convert Unit System to what is used in this example, if needed
    FbxSystemUnit SceneSystemUnit = scene->GetGlobalSettings().GetSystemUnit();
    if (SceneSystemUnit.GetScaleFactor() != 1.0)
    {
        //The unit in this example is centimeter.
        FbxSystemUnit::cm.ConvertScene(scene);
    }

    // 三角面化
    FbxGeometryConverter geometryConv = FbxGeometryConverter(mManager);
    geometryConv.Triangulate(scene, true);
    geometryConv.RemoveBadPolygonsFromMeshes(scene);
    //geometryConv.SplitMeshesPerMaterial(scene, true);

    // Scene解析
    // Texture 読み込み
    printf("=== textures ===\n");
    printf("texture num: %d\n", scene->GetTextureCount());
    for (int i = 0; i < scene->GetTextureCount(); i++) {
        LoadTexture(scene->GetTexture(i));
    }
    printf("=== material === \n");

    int materialCount = scene->GetMaterialCount();
    //for (int i = 0; i < materialCount; i++) {
    //    FbxSurfaceMaterial* material = scene->GetMaterial(i);
    //    LoadMaterial(material);
    //}

    FbxNode* rootNode = scene->GetRootNode();
    mRootNodeMesh = new NodeMesh(rootNode, this);

    // マテリアルとtextureとの対応関係を読み込む
    {
        std::string filePath = "./resources/" + fileName + "/Material.json";
        std::ifstream ifs(filePath.c_str());
        if (ifs.good())
        {
            ifs >> mMaterialJsonMap;
        }
        else {
            printf("error: failed to load material.josn\n");
        }
        ifs.close();
    }

    //if (rootNode == 0) {
    //    printf("error: cannont find root node: %s\n", fileName.c_str());
    //    return false;
    //}
    ////ShowNodeNames(rootNode, 0);
    //LoadNode(rootNode);

    //// Create VAO
    //if (!mIsDrawArray) {
    //    glGenVertexArrays(1, &mVertexArray);
    //    glBindVertexArray(mVertexArray);

    //    // Vertex Bufferの作成
    //    PopulateBuffers();

    //    // unbind cube vertex arrays
    //    glBindVertexArray(0);
    //    glBindBuffer(GL_ARRAY_BUFFER, 0);
    //    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //}

    //DrawArrayPB();

    importer->Destroy(); // シーンを流し込んだらImporterは解放してOK
    importer = NULL;
    // マネージャ解放
    // 関連するすべてのオブジェクトが解放される
     mManager->Destroy();

	return true;
}

void deFBXMesh::LoadTexture(FbxTexture* lTexture)
{
    FbxFileTexture* lFileTexture = FbxCast<FbxFileTexture>(lTexture);
    if (lFileTexture && !lFileTexture->GetUserDataPtr())
    {
        // Try to load the texture from absolute path
        std::string texturePathData = lFileTexture->GetFileName();
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
        std::string texFileName = split_list[split_list.size() - 1];
        Texture* tex = new Texture(texturePath);
        mTextures.emplace(texFileName, tex);
        printf("%s\n", texFileName.c_str());
    }
}

void deFBXMesh::BindTexture(std::string materialName)
{
    {
        auto iter = mMaterialJsonMap.find(materialName);
        if (iter == mMaterialJsonMap.end()) {
            return;
        }
    }

    std::string diffuseTexName = mMaterialJsonMap[materialName]["Diffuse"];

    {
        auto iter = mTextures.find(diffuseTexName);
        if (iter == mTextures.end()) {
            std::string texFilePath = "./resources/" + mMeshFileName + "/Textures/" + diffuseTexName;
            Texture* tex = new Texture(texFilePath);
            mTextures.emplace(diffuseTexName, tex);
            tex->BindTexture();
        }
        else {
            iter->second->BindTexture();
        }
    }
}

void deFBXMesh::UnBindTexture(std::string materialName)
{
    auto iter = mMaterialJsonMap.find(materialName);
    if (iter == mMaterialJsonMap.end()) {
        return;
    }
    std::string diffuseTexName = mMaterialJsonMap[materialName]["Diffuse"];
    Texture* diffuseTexture = mTextures[diffuseTexName];
    diffuseTexture->UnBindTexture();
}

void deFBXMesh::LoadMaterial(FbxSurfaceMaterial* material)
{
    Material* MaterialData = new Material;
    MaterialData->Name = material->GetName();
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
                std::string texFileName = split_list[split_list.size() - 1];
                Texture* tex = new Texture(texturePath);
                mTextures.emplace(texFileName, tex);
                MaterialData->Textures.push_back(tex);

                printf("tex name: %s\n", texFileName.c_str());
                //printf("%s\n", fileTex->GetFileName());
            }
        }
    }

    mMaterials.push_back(MaterialData);
}

void deFBXMesh::LoadNode(FbxNode* node)
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
            unsigned int vertexOffset;
            dedeMaterialVNT* NodeMeshes = nullptr;
            if (mIsDrawArray) {
                NodeMeshes = LoadMeshArray(pMesh, vertexOffset);
                if (!NodeMeshes) {
                    printf("error: failed to load LoadMeshArray\n");
                    exit(-1);
                }
            }
            else {
                LoadMeshElement(pMesh);
            }
            
            // マテリアルの読み込み
            if (mIsDrawArray) {
                int materialCount = node->GetMaterialCount();
                printf("material count: %d\n", materialCount);
                Material* MaterialData = nullptr;
                for (int materialIndex = 0; materialIndex < materialCount; materialIndex++) {
                    FbxSurfaceMaterial* material = node->GetMaterial(materialIndex);
                    LoadMaterial(material);
                    NodeMeshes[materialIndex].material = MaterialData;
                    mNodeMeshes.push_back(&NodeMeshes[materialIndex]);
                }
            }

            //MeshOffset mo;
            //mo.material = MaterialData;
            //mo.VNTOffset = vertexOffset;
            //mMeshOffsets.push_back(mo);
        }
    }

    int childCount = node->GetChildCount();
    for (int i = 0; i < childCount; ++i) {
        LoadNode(node->GetChild(i));
    }
}

deFBXMesh::dedeMaterialVNT* deFBXMesh::LoadMeshElement(FbxMesh* mesh)
{
    if (!mesh->GetNode())
        return nullptr;

    const int lPolygonCount = mesh->GetPolygonCount();
    std::vector<BasicMeshEntry*> mSubMeshes;
    std::vector<Material*> Materials;

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
                    if (Materials.size() < lMaterialIndex + 1)
                    {
                        Materials.resize(lMaterialIndex + 1);
                    }
                    if (Materials[lMaterialIndex] == NULL)
                    {
                        Materials[lMaterialIndex] = new Material;
                    }
                    Materials[lMaterialIndex]->TriangleCount += 1;
                }

                // Make sure we have no "holes" (NULL) in the mSubMeshes table. This can happen
                // if, in the loop above, we resized the mSubMeshes by more than one slot.
                for (int i = 0; i < Materials.size(); i++)
                {
                    if (Materials[i] == NULL)
                        Materials[i] = new Material;
                }

                // Record the offset (how many vertex)
                const int lMaterialCount = Materials.size();
                int lOffset = 0;
                for (int lIndex = 0; lIndex < lMaterialCount; ++lIndex)
                {
                    Materials[lIndex]->IndexOffset = lOffset;
                    lOffset += Materials[lIndex]->TriangleCount * 3;
                    // This will be used as counter in the following procedures, reset to zero
                    Materials[lIndex]->TriangleCount = 0;
                }
                FBX_ASSERT(lOffset == lPolygonCount * 3);
            }
        }
    }

    // All faces will use the same material.
    if (Materials.size() == 0)
    {
        Materials.resize(1);
        Materials[0] = new Material();
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

    dedeMaterialVNT* NodeMeshes = new dedeMaterialVNT[Materials.size()];
    // Allocate the array memory, by control point or by polygon vertex.
    int lPolygonVertexCount = mesh->GetControlPointsCount();
    if (!isAllByControlPoint)
    {
        lPolygonVertexCount = lPolygonCount * 3;
    }
    //float* lVertices = new float[lPolygonVertexCount * VERTEX_STRIDE];
    int currentPositionSize = mPositions.size();
    mPositions.resize(currentPositionSize + lPolygonVertexCount);
    NodeMeshes->Positions.resize(lPolygonVertexCount);
    //unsigned int* lIndices = new unsigned int[lPolygonCount * 3];
    int currentIndicesSize = mIndices.size();
    mIndices.resize(currentIndicesSize + lPolygonCount * 3);
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
    assert(isAllByControlPoint == false);   // このフラグは絶対falseなのでは？




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
        const int lIndexOffset = Materials[lMaterialIndex]->IndexOffset +
            Materials[lMaterialIndex]->TriangleCount * 3;
        for (int lVerticeIndex = 0; lVerticeIndex < 3; ++lVerticeIndex)
        {
            const int lControlPointIndex = mesh->GetPolygonVertex(lPolygonIndex, lVerticeIndex);
            // If the lControlPointIndex is -1, we probably have a corrupted mesh data. At this point,
            // it is not guaranteed that the cache will work as expected.
            if (lControlPointIndex >= 0)
            {
                if (isAllByControlPoint)
                {
                    //lIndices[lIndexOffset + lVerticeIndex] = static_cast<unsigned int>(lControlPointIndex);
                    mIndices[currentIndicesSize + lIndexOffset + lVerticeIndex] = static_cast<unsigned int>(lControlPointIndex);
                }
                // Populate the array with vertex attribute, if by polygon vertex.
                else
                {
                    //lIndices[lIndexOffset + lVerticeIndex] = static_cast<unsigned int>(lVertexCount);
                    mIndices[currentIndicesSize + lIndexOffset + lVerticeIndex] = static_cast<unsigned int>(lVertexCount);

                    lCurrentVertex = lControlPoints[lControlPointIndex];

                    mPositions[currentPositionSize + lVertexCount] = glm::vec3(
                        static_cast<float>(lCurrentVertex[0]),
                        static_cast<float>(lCurrentVertex[1]),
                        static_cast<float>(lCurrentVertex[2])
                    );
                    NodeMeshes->Positions[lVertexCount] = glm::vec3(
                        static_cast<float>(lCurrentVertex[0]),
                        static_cast<float>(lCurrentVertex[1]),
                        static_cast<float>(lCurrentVertex[2])
                    );
                    //lVertices[lVertexCount * VERTEX_STRIDE] = static_cast<float>(lCurrentVertex[0]);
                    //lVertices[lVertexCount * VERTEX_STRIDE + 1] = static_cast<float>(lCurrentVertex[1]);
                    //lVertices[lVertexCount * VERTEX_STRIDE + 2] = static_cast<float>(lCurrentVertex[2]);
                    //lVertices[lVertexCount * VERTEX_STRIDE + 3] = 1;

                    if (hasNormal)
                    {
                        mesh->GetPolygonVertexNormal(lPolygonIndex, lVerticeIndex, lCurrentNormal);
                        mNormals[currentNormalSize + lVertexCount] = glm::vec3(
                            static_cast<float>(lCurrentNormal[0]),
                            static_cast<float>(lCurrentNormal[1]),
                            static_cast<float>(lCurrentNormal[2])
                        );
                        //lNormals[lVertexCount * NORMAL_STRIDE] = static_cast<float>(lCurrentNormal[0]);
                        //lNormals[lVertexCount * NORMAL_STRIDE + 1] = static_cast<float>(lCurrentNormal[1]);
                        //lNormals[lVertexCount * NORMAL_STRIDE + 2] = static_cast<float>(lCurrentNormal[2]);
                    }

                    if (hasUV)
                    {
                        bool lUnmappedUV;
                        mesh->GetPolygonVertexUV(lPolygonIndex, lVerticeIndex, lUVName, lCurrentUV, lUnmappedUV);
                        mTexCoords[currentUVSize + lVertexCount] = glm::vec2(
                            static_cast<float>(lCurrentUV[0]),
                            static_cast<float>(lCurrentUV[1])
                        );
                        //lUVs[lVertexCount * UV_STRIDE] = static_cast<float>(lCurrentUV[0]);
                        //lUVs[lVertexCount * UV_STRIDE + 1] = static_cast<float>(lCurrentUV[1]);
                    }
                }
            }
            ++lVertexCount;
        }
        Materials[lMaterialIndex]->TriangleCount += 1;
    }

    for (auto material : Materials) {
        Material* materialData = new Material;
        materialData->IndexOffset = material->IndexOffset + currentIndicesSize;
        materialData->TriangleCount = material->TriangleCount;
        mMaterials.push_back(materialData);
    }


    return NodeMeshes;
}

deFBXMesh::dedeMaterialVNT* deFBXMesh::LoadMeshArray(FbxMesh* mesh, unsigned int& vertexOffset)
{
    if (!mesh->GetNode()) {
        return nullptr;
    }

    const int lPolygonCount = mesh->GetPolygonCount();
    std::vector<BasicMeshEntry*> mSubMeshes;

    struct VNTData
    {
        VNTData()
            :TriangleCount(0)
            ,VNTOffset(0)
            ,Positions(0)
            ,Normals(0)
            ,TexCoords(0)
        {
        }
        //glm::vec3* Positions;
        //glm::vec3* Normals;
        //glm::vec2* TexCoords;
        std::vector<glm::vec3> Positions;
        std::vector<glm::vec3> Normals;
        std::vector<glm::vec2> TexCoords;
        unsigned int TriangleCount;     // このマテリアルのポリゴンの数
        unsigned int VNTOffset;         // 頂点データのオフセット
    };
    std::vector<VNTData*> vntArray(0);

    // Count the polygon count of each material
    FbxLayerElementArrayTemplate<int>* lMaterialIndice = NULL;
    FbxGeometryElement::EMappingMode lMaterialMappingMode = FbxGeometryElement::eNone;
    assert(mesh->GetElementMaterial() != nullptr);
    lMaterialIndice = &mesh->GetElementMaterial()->GetIndexArray();
    lMaterialMappingMode = mesh->GetElementMaterial()->GetMappingMode();
    if (lMaterialIndice && lMaterialMappingMode == FbxGeometryElement::eByPolygon)
    {
        FBX_ASSERT(lMaterialIndice->GetCount() == lPolygonCount);
        if (lMaterialIndice->GetCount() == lPolygonCount)
        {
            // 各マテリアルのポリゴンの数を求める
            for (int lPolygonIndex = 0; lPolygonIndex < lPolygonCount; ++lPolygonIndex)
            {
                const int lMaterialIndex = lMaterialIndice->GetAt(lPolygonIndex);
                if (mSubMeshes.size() < lMaterialIndex + 1)
                {
                    mSubMeshes.resize(lMaterialIndex + 1);
                }
                if (vntArray.size() < lMaterialIndex + 1)
                {
                    vntArray.resize(lMaterialIndex + 1);
                }

                if (mSubMeshes[lMaterialIndex] == NULL)
                {
                    mSubMeshes[lMaterialIndex] = new BasicMeshEntry;
                }
                if (vntArray[lMaterialIndex] == NULL)
                {
                    vntArray[lMaterialIndex] = new VNTData();
                }
                vntArray[lMaterialIndex]->TriangleCount++;
                mSubMeshes[lMaterialIndex]->TriangleCount += 1;
            }

            // Make sure we have no "holes" (NULL) in the mSubMeshes table. This can happen
            // if, in the loop above, we resized the mSubMeshes by more than one slot.
            for (int i = 0; i < mSubMeshes.size(); i++)
            {
                if (mSubMeshes[i] == NULL)
                    mSubMeshes[i] = new BasicMeshEntry;
            }

            // 各マテリアルに対する頂点のオフセット導出
            const int lMaterialCount = mSubMeshes.size();
            int lOffset = 0;
            int vntOffset = 0;
            for (int lIndex = 0; lIndex < vntArray.size(); ++lIndex)
            {
                if (vntArray[lIndex] == NULL) {
                    vntArray[lIndex] = new VNTData();
                }
                mSubMeshes[lIndex]->IndexOffset = lOffset;
                vntArray[lIndex]->VNTOffset = vntOffset;
                lOffset += mSubMeshes[lIndex]->TriangleCount * 3;
                vntOffset += vntArray[lIndex]->TriangleCount * 3;


                vntArray[lIndex]->Positions.resize(vntArray[lIndex]->TriangleCount * 3);
                vntArray[lIndex]->Normals.resize(vntArray[lIndex]->TriangleCount * 3);
                vntArray[lIndex]->TexCoords.resize(vntArray[lIndex]->TriangleCount * 3);

                //vntArray[lIndex]->Positions = new glm::vec3[vntArray[lIndex]->TriangleCount * 3];
                //vntArray[lIndex]->Normals = new glm::vec3[vntArray[lIndex]->TriangleCount * 3];
                //vntArray[lIndex]->TexCoords = new glm::vec2[vntArray[lIndex]->TriangleCount * 3];

                vntArray[lIndex]->TriangleCount = 0;
                // This will be used as counter in the following procedures, reset to zero
                mSubMeshes[lIndex]->TriangleCount = 0;
            }
            FBX_ASSERT(lOffset == lPolygonCount * 3);
        }
    }

    assert(mesh->GetElementMaterial() != nullptr);

    // All faces will use the same material.
    if (mSubMeshes.size() == 0)
    {
        mSubMeshes.resize(1);
        mSubMeshes[0] = new BasicMeshEntry();
    }
    // マテリアルの指定がないとき
    if (vntArray.size() == 0)
    {
        vntArray.resize(1);
        vntArray[0] = new VNTData();
        vntArray[0]->Positions.resize(lPolygonCount * 3);
        vntArray[0]->Normals.resize(lPolygonCount * 3);
        vntArray[0]->TexCoords.resize(lPolygonCount * 3);

        //vntArray[0]->Positions.resize(lPolygonCount * 3);
        //vntArray[0]->Normals.resize(lPolygonCount * 3);
        //vntArray[0]->TexCoords.resize(lPolygonCount * 3);
    }


    // Congregate all the data of a mesh to be cached in VBOs.
    // If normal or UV is by polygon vertex, record all vertex attributes by polygon vertex.
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
    }
    if (hasUV)
    {
        lUVMappingMode = mesh->GetElementUV(0)->GetMappingMode();
        if (lUVMappingMode == FbxGeometryElement::eNone)
        {
            hasUV = false;
        }
    }

    // Allocate the array memory, by control point or by polygon vertex.
    int lPolygonVertexCount = mesh->GetControlPointsCount();
    lPolygonVertexCount = lPolygonCount * 3;


    //float* lVertices = new float[lPolygonVertexCount * VERTEX_STRIDE];

    //int currentPositionSize = mPositions.size();
    //mPositions.resize(currentPositionSize + lPolygonVertexCount);
    //unsigned int* lIndices = new unsigned int[lPolygonCount * 3];
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



    for (int lPolygonIndex = 0; lPolygonIndex < lPolygonCount; ++lPolygonIndex)
    {
        // The material for current face.
        int lMaterialIndex = 0;
        if (lMaterialIndice && lMaterialMappingMode == FbxGeometryElement::eByPolygon)
        {
            lMaterialIndex = lMaterialIndice->GetAt(lPolygonIndex);
        }

        if (vntArray.size() < lMaterialIndex + 1)
        {
            vntArray.resize(lMaterialIndex + 1);
        }

        //const int vntOffset = vntArray[lMaterialIndex]->VNTOffset +
        //    vntArray[lMaterialIndex]->TriangleCount * 3;
        const int vntOffset = vntArray[lMaterialIndex]->TriangleCount * 3;

        // Where should I save the vertex attribute index, according to the material
        for (int lVerticeIndex = 0; lVerticeIndex < 3; ++lVerticeIndex)
        {
            const int lControlPointIndex = mesh->GetPolygonVertex(lPolygonIndex, lVerticeIndex);
            // If the lControlPointIndex is -1, we probably have a corrupted mesh data. At this point,
            // it is not guaranteed that the cache will work as expected.
            if (lControlPointIndex >= 0)
            {
                lCurrentVertex = lControlPoints[lControlPointIndex];

                vntArray[lMaterialIndex]->Positions[vntOffset + lVerticeIndex] = glm::vec3(
                    static_cast<float>(lCurrentVertex[0]),
                    static_cast<float>(lCurrentVertex[1]),
                    static_cast<float>(lCurrentVertex[2])
                );

                if (hasNormal) {
                    mesh->GetPolygonVertexNormal(lPolygonIndex, lVerticeIndex, lCurrentNormal);
                    vntArray[lMaterialIndex]->Normals[vntOffset + lVerticeIndex] = glm::vec3(
                        static_cast<float>(lCurrentNormal[0]),
                        static_cast<float>(lCurrentNormal[1]),
                        static_cast<float>(lCurrentNormal[2])
                    );
                }

                if (hasUV) {
                    bool lUnmappedUV;
                    mesh->GetPolygonVertexUV(lPolygonIndex, lVerticeIndex, lUVName, lCurrentUV, lUnmappedUV);
                    vntArray[lMaterialIndex]->TexCoords[vntOffset + lVerticeIndex] = glm::vec2(
                        static_cast<float>(lCurrentUV[0]),
                        static_cast<float>(lCurrentUV[1])
                    );
                }

            }
        }
        vntArray[lMaterialIndex]->TriangleCount++;
    }

    // Position, Normal, Texcoordsの数はすべて同じはず


    // VAO作成
    dedeMaterialVNT* NodeMeshes = new dedeMaterialVNT[vntArray.size()];
    VAO* vao = nullptr;
    for (int i = 0; i < vntArray.size(); i++) {
        VNTData* vnt = vntArray[i];

        vao = new VAO();
        vao->SetVNT(vnt->Positions, vnt->Normals, vnt->TexCoords);
        vao->CreateVAO();
        mVAOs.push_back(vao);

        //assert((vnt->Normals.size() == vnt->Positions.size()) && (vnt->Normals.size() == vnt->TexCoords.size()));

        dedeMaterialVNT nm;
        glGenVertexArrays(1, &nm.VertexArray);
        glBindVertexArray(nm.VertexArray);

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

        const int positionNum = vnt->TriangleCount * 3;
        glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[POS_VB]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vnt->Positions[0])* positionNum, &vnt->Positions[0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

        if (vnt->Normals.size() != 0) {
            glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[NORMAL_VB]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vnt->Normals[0])* positionNum, &vnt->Normals[0], GL_STATIC_DRAW);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
        }

        // UV
        if (vnt->TexCoords.size() != 0) {
            glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[TEXCOORD_VB]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vnt->TexCoords[0]) * positionNum, &vnt->TexCoords[0], GL_STATIC_DRAW);
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
        }

        // unbind cube vertex arrays
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        //nm.VertexCount = vnt->Positions.size() * 3;
        nm.VertexCount = positionNum * 3;
        nm.VertexBuffers = m_Buffers;
        nm.Positions = vnt->Positions;
        nm.Normals = vnt->Normals;
        nm.TexCoords = vnt->TexCoords;

        NodeMeshes[i] = nm;
    }

    return NodeMeshes;
}

void deFBXMesh::LoadMesh(FbxMesh* mesh)
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

void deFBXMesh::LoadNormal(FbxLayerElementNormal* normalElem)
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

void deFBXMesh::LoadUV(FbxLayerElementUV* uvElement)
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

void deFBXMesh::PopulateBuffers()
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
    if (mNormals.size() != 0) {
        glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[NORMAL_VB]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(mNormals[0]) * mNormals.size(), &mNormals[0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
    }

    // UV
    if (mTexCoords.size() != 0) {
        glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[TEXCOORD_VB]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(mTexCoords[0]) * mTexCoords.size(), &mTexCoords[0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
    }

    // Index Buffer
    if (!mIsDrawArray) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Buffers[INDEX_BUFFER]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * mIndices.size(), &mIndices[0], GL_STATIC_DRAW);
    }
    mVertexBuffers = m_Buffers;
}

void deFBXMesh::DrawArrayPB()
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
    //unsigned int IndexBuffer;
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Buffers[INDEX_BUFFER]);
    //glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(mIndices[0]) * mIndices.size(), &mIndices[0], GL_STATIC_DRAW);


    // unbind cube vertex arrays
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void deFBXMesh::BindVertexArray()
{
    glBindVertexArray(mVertexArray);
}

void deFBXMesh::UnBindVertexArray()
{
    glBindVertexArray(0);
}

void deFBXMesh::Draw(Shader* shader)
{
    glm::vec3 mPos = glm::vec3(2.f, 2.f, 0.f);
    glm::mat4 mRotate = glm::rotate(glm::mat4(1.0f), (float)M_PI / 2.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    float mScale = 0.01f;

    glm::mat4 ScaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(mScale, mScale, mScale));
    glm::mat4 TranslateMat = glm::translate(glm::mat4(1.0f), mPos);
    glm::mat4 mWorldTransform = TranslateMat * mRotate * ScaleMat;

    //glDrawElementsBaseVertex(GL_TRIANGLES,
    //    mIndices.size(),
    //    GL_UNSIGNED_INT,
    //    (void*)(mIndices.data()),
    //    mPositions.size() * 3);
    if (mIsDrawArray) {
        //for (auto nm : mNodeMeshes) {
        //    glBindVertexArray(nm->VertexArray);
        //    glDrawArrays(
        //        GL_TRIANGLES,
        //        0,
        //        nm->VertexCount * 3
        //    );
        //}
        //mRootNodeMesh->Draw();
        //for (auto vao : mVAOs) {
        //    shader->UseProgram();
        //    shader->SetMatrixUniform("ModelTransform", mWorldTransform);

        //    vao->Bind();
        //    glDrawArrays(
        //        GL_POINTS,
        //        0,
        //        vao->GetVertexCount() * 3
        //    );
        //}

    }
    else {
        mRootNodeMesh->Draw(shader);
        //for (auto material : mMaterials) {
        //    GLsizei lOffset = material->IndexOffset * sizeof(unsigned int);
        //    glBindVertexArray(mVertexArray);
        //    glDrawElements(GL_TRIANGLES,
        //        material->TriangleCount * 3,
        //        GL_UNSIGNED_INT,
        //        (void*)(lOffset));
        //}
    }
}

void deFBXMesh::Update(float deltaTime)
{
    mRootNodeMesh->Update(deltaTime);
    mCurrentTicks = mUnityChan->GetGame()->GetCurrentTime();
}

void deFBXMesh::DrawArray()
{
    glBindVertexArray(mVertexArray);
    glDrawArrays(
        GL_TRIANGLES,
        0,
        mPositions.size() * 3
    );
}

void deFBXMesh::ShowNodeNames(FbxNode* node, int indent)
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

VAO::VAO()
    :mPositions(0)
    ,mNormals(0)
    ,mTexCoords(0)
{}

void VAO::CreateVAO()
{
    glGenVertexArrays(1, &mVertexArray);
    glBindVertexArray(mVertexArray);

    // Vertex Bufferの作成
    enum BUFFER_TYPE {
        INDEX_BUFFER = 0,
        POS_VB = 1,
        TEXCOORD_VB = 2,
        NORMAL_VB = 3,
        NUM_BUFFERS = 4,  // required only for instancing
    };
    //GLuint m_Buffers[NUM_BUFFERS] = { 0 };
    mVertexBuffers = new unsigned int[NUM_BUFFERS];
    for (int i = 0; i < NUM_BUFFERS; i++) {
        mVertexBuffers[i] = 0;
    }
    glGenBuffers(1, mVertexBuffers);

    //const int positionNum = vnt->TriangleCount * 3;
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffers[POS_VB]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(mPositions[0]) * mPositions.size(), &mPositions[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

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

    // unbind cube vertex arrays
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    //nm.VertexCount = vnt->Positions.size() * 3;
    //nm.Positions = vnt->Positions;
    //nm.Normals = vnt->Normals;
    //nm.TexCoords = vnt->TexCoords;

    //NodeMeshes[i] = nm;
}

void VAO::Bind()
{
    glBindVertexArray(mVertexArray);
}
