#include "wMesh.hpp"
#include "Texture.hpp"
#include "GLUtil.hpp"
#include <fstream>

wMesh::wMesh()
{

}

bool wMesh::Load(std::string filePath)
{
    // Mesh Nnameの取り出し
    {
        std::vector<std::string> wordArray;
        GLUtil::Split('/', filePath, wordArray);
        std::string meshFileName = wordArray[wordArray.size() - 1];
        wordArray.clear();
        GLUtil::Split('.', meshFileName, wordArray);
        mMeshName = wordArray[wordArray.size() - 2];
    }

    // マネージャを生成
    mManager = FbxManager::Create();

    // IOSettingを生成
    FbxIOSettings* ioSettings = FbxIOSettings::Create(mManager, IOSROOT);
    mManager->SetIOSettings(ioSettings);
    {
        FbxString lPath = FbxGetApplicationDirectory();
        mManager->LoadPluginsDirectory(lPath.Buffer());
    }
    mScene = FbxScene::Create(mManager, "scene");


    // Importerを生成
    FbxImporter* importer = FbxImporter::Create(mManager, "");
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
        printf("Failed to open file: %s\n", filePath.c_str());
        return false;
    }


    // SceneオブジェクトにFBXファイル内の情報を流し込む
    importer->Import(mScene);
    {
        // Check the scene integrity!
        FbxStatus status;
        FbxArray< FbxString*> details;
        FbxSceneCheckUtility sceneCheck(FbxCast<FbxScene>(mScene), &status, &details);
        bool lNotify = (!sceneCheck.Validate(FbxSceneCheckUtility::eCkeckData) && details.GetCount() > 0) || (importer->GetStatus().GetCode() != FbxStatus::eSuccess);
        assert(!lNotify);
    }


    FbxAxisSystem SceneAxisSystem = mScene->GetGlobalSettings().GetAxisSystem();
    FbxAxisSystem OurAxisSystem(FbxAxisSystem::eZAxis, FbxAxisSystem::eParityOdd, FbxAxisSystem::eRightHanded);
    if (SceneAxisSystem != OurAxisSystem)
    {
        OurAxisSystem.ConvertScene(mScene);
    }

    // Convert Unit System to what is used in this example, if needed
    FbxSystemUnit SceneSystemUnit = mScene->GetGlobalSettings().GetSystemUnit();
    if (SceneSystemUnit.GetScaleFactor() != 1.0)
    {
        //The unit in this example is centimeter.
        FbxSystemUnit::cm.ConvertScene(mScene);
    }

    // 三角面化
    FbxGeometryConverter geometryConv = FbxGeometryConverter(mManager);
    geometryConv.Triangulate(mScene, true);
    geometryConv.RemoveBadPolygonsFromMeshes(mScene);
    //geometryConv.SplitMeshesPerMaterial(scene, true);

    // Scene解析
    // Texture 読み込み
    printf("=== textures ===\n");
    printf("texture num: %d\n", mScene->GetTextureCount());
    for (int i = 0; i < mScene->GetTextureCount(); i++) {
        LoadTexture(mScene->GetTexture(i));
    }
    printf("=== material === \n");

    int materialCount = mScene->GetMaterialCount();
    //for (int i = 0; i < materialCount; i++) {
    //    FbxSurfaceMaterial* material = scene->GetMaterial(i);
    //    LoadMaterial(material);
    //}

    // Meshファイルの読み出し
    FbxNode* rootNode = mScene->GetRootNode();
    LoadNode(rootNode);
    //mRootNodeMesh = new NodeMesh(rootNode, nullptr, this);
    //if (!mIsAnimMesh) {
    //    // skeletonのないmesh transformを埋める
    //    for (auto iter : mMeshSkeletonNameMap) {
    //        if (iter.second.mSkeletonNodeName.size() == 0) {
    //            printf("**** %s\n", iter.first.c_str());
    //        }
    //    }
    //}

    // マテリアルとtextureとの対応関係を読み込む
    {
        std::string filePath = "./resources/" + mMeshName + "/Material.json";
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


    importer->Destroy(); // シーンを流し込んだらImporterは解放してOK
    importer = NULL;
    // マネージャ解放
    // 関連するすべてのオブジェクトが解放される
     //mManager->Destroy();


    return true;
}

void wMesh::LoadTexture(FbxTexture* lTexture)
{
    FbxFileTexture* lFileTexture = FbxCast<FbxFileTexture>(lTexture);
    if (lFileTexture && !lFileTexture->GetUserDataPtr())
    {
        std::string texturePathData = lFileTexture->GetFileName();
        std::vector<std::string> nameArray;
        GLUtil::Split('/', texturePathData, nameArray);
        std::string texName = nameArray[nameArray.size() - 1];
        std::string texFilePath = "./resources/" + mMeshName + "/Textures/" + texName;
        Texture* tex = new Texture(texFilePath);
        printf("load tex: %s\n", texFilePath.c_str());
        mTextures.emplace(texName, tex);
    }
}

void wMesh::LoadNode(FbxNode* node)
{

}
