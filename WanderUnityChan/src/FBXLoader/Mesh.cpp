#include "FBXLoader/Mesh.hpp"
#include <fbxsdk.h>


FBXMesh::FBXMesh()
    :mSdkManager(nullptr)
    ,mImporter(nullptr)
    ,mScene(nullptr)
{}

bool FBXMesh::Load(std::string fileName)
{
    std::string filePath = "./resources/" + fileName + "/" + fileName + ".fbx";

    InitializeSdkObjects(mSdkManager, mScene);

    if (mSdkManager)
    {
        // Create the importer.
        int lFileFormat = -1;
        mImporter = FbxImporter::Create(mSdkManager, "");
        if (!mSdkManager->GetIOPluginRegistry()->DetectReaderFileFormat(filePath.c_str(), lFileFormat))
        {
            // Unrecognizable file format. Try to fall back to FbxImporter::eFBX_BINARY
            lFileFormat = mSdkManager->GetIOPluginRegistry()->FindReaderIDByDescription("FBX binary (*.fbx)");;
        }

        // Initialize the importer by providing a filename.
        if (mImporter->Initialize(filePath.c_str(), lFileFormat) == true)
        {
            printf("Importing file...\n");
        }
        else
        {
            printf("Failed to open file: %s\n", fileName.c_str());
            return false;
        }
    }
    else
    {
        printf("Unable to create the FBX SDK manager\n", fileName.c_str());
        return false;
    }

    mSdkManager->Destroy();

	return true;
}


void FBXMesh::InitializeSdkObjects(FbxManager*& pManager, FbxScene*& pScene)
{
    //The first thing to do is to create the FBX Manager which is the object allocator for almost all the classes in the SDK
    pManager = FbxManager::Create();
    if (!pManager)
    {
        FBXSDK_printf("Error: Unable to create FBX Manager!\n");
        exit(1);
    }
    else FBXSDK_printf("Autodesk FBX SDK version %s\n", pManager->GetVersion());

    //Create an IOSettings object. This object holds all import/export settings.
    FbxIOSettings* ios = FbxIOSettings::Create(pManager, IOSROOT);
    pManager->SetIOSettings(ios);

    //Load plugins from the executable directory (optional)
    FbxString lPath = FbxGetApplicationDirectory();
    pManager->LoadPluginsDirectory(lPath.Buffer());

    //Create an FBX scene. This object holds most objects imported/exported from/to files.
    pScene = FbxScene::Create(pManager, "My Scene");
    if (!pScene)
    {
        FBXSDK_printf("Error: Unable to create FBX scene!\n");
        exit(1);
    }
}