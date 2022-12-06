#include "fbxsdk.h"
#include <vector>
#include <string>

#ifdef IOS_REF
#undef  IOS_REF
#define IOS_REF (*(pManager->GetIOSettings()))
#endif

void InitializeSdkObjects(FbxManager*& pManager, FbxScene*& pScene)
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


bool LoadScene(FbxManager* pManager, FbxDocument* pScene, const char* pFilename)
{
    int lFileMajor, lFileMinor, lFileRevision;
    int lSDKMajor, lSDKMinor, lSDKRevision;
    //int lFileFormat = -1;
    int lAnimStackCount;
    bool lStatus;
    char lPassword[1024];

    // Get the file version number generate by the FBX SDK.
    FbxManager::GetFileFormatVersion(lSDKMajor, lSDKMinor, lSDKRevision);

    // Create an importer.
    FbxImporter* lImporter = FbxImporter::Create(pManager, "");

    // Initialize the importer by providing a filename.
    const bool lImportStatus = lImporter->Initialize(pFilename, -1, pManager->GetIOSettings());
    lImporter->GetFileVersion(lFileMajor, lFileMinor, lFileRevision);

    if (!lImportStatus)
    {
        FbxString error = lImporter->GetStatus().GetErrorString();
        FBXSDK_printf("Call to FbxImporter::Initialize() failed.\n");
        FBXSDK_printf("Error returned: %s\n\n", error.Buffer());

        if (lImporter->GetStatus().GetCode() == FbxStatus::eInvalidFileVersion)
        {
            FBXSDK_printf("FBX file format version for this FBX SDK is %d.%d.%d\n", lSDKMajor, lSDKMinor, lSDKRevision);
            FBXSDK_printf("FBX file format version for file '%s' is %d.%d.%d\n\n", pFilename, lFileMajor, lFileMinor, lFileRevision);
        }

        return false;
    }

    FBXSDK_printf("FBX file format version for this FBX SDK is %d.%d.%d\n", lSDKMajor, lSDKMinor, lSDKRevision);

    if (lImporter->IsFBX())
    {
        FBXSDK_printf("FBX file format version for file '%s' is %d.%d.%d\n\n", pFilename, lFileMajor, lFileMinor, lFileRevision);

        // From this point, it is possible to access animation stack information without
        // the expense of loading the entire file.

        FBXSDK_printf("Animation Stack Information\n");

        lAnimStackCount = lImporter->GetAnimStackCount();

        FBXSDK_printf("    Number of Animation Stacks: %d\n", lAnimStackCount);
        FBXSDK_printf("    Current Animation Stack: \"%s\"\n", lImporter->GetActiveAnimStackName().Buffer());
        FBXSDK_printf("\n");

        for (int i = 0; i < lAnimStackCount; i++)
        {
            FbxTakeInfo* lTakeInfo = lImporter->GetTakeInfo(i);

            FBXSDK_printf("    Animation Stack %d\n", i);
            FBXSDK_printf("         Name: \"%s\"\n", lTakeInfo->mName.Buffer());
            FBXSDK_printf("         Description: \"%s\"\n", lTakeInfo->mDescription.Buffer());

            // Change the value of the import name if the animation stack should be imported 
            // under a different name.
            FBXSDK_printf("         Import Name: \"%s\"\n", lTakeInfo->mImportName.Buffer());

            // Set the value of the import state to false if the animation stack should be not
            // be imported. 
            FBXSDK_printf("         Import State: %s\n", lTakeInfo->mSelect ? "true" : "false");
            FBXSDK_printf("\n");
        }

        // Set the import states. By default, the import states are always set to 
        // true. The code below shows how to change these states.
        IOS_REF.SetBoolProp(IMP_FBX_MATERIAL, true);
        IOS_REF.SetBoolProp(IMP_FBX_TEXTURE, true);
        IOS_REF.SetBoolProp(IMP_FBX_LINK, true);
        IOS_REF.SetBoolProp(IMP_FBX_SHAPE, true);
        IOS_REF.SetBoolProp(IMP_FBX_GOBO, true);
        IOS_REF.SetBoolProp(IMP_FBX_ANIMATION, true);
        IOS_REF.SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);
    }

    // Import the scene.
    lStatus = lImporter->Import(pScene);
    if (lStatus == false && lImporter->GetStatus() == FbxStatus::ePasswordError)
    {
        FBXSDK_printf("Please enter password: ");

        lPassword[0] = '\0';

        FBXSDK_CRT_SECURE_NO_WARNING_BEGIN
            scanf("%s", lPassword);
        FBXSDK_CRT_SECURE_NO_WARNING_END

            FbxString lString(lPassword);

        IOS_REF.SetStringProp(IMP_FBX_PASSWORD, lString);
        IOS_REF.SetBoolProp(IMP_FBX_PASSWORD_ENABLE, true);

        lStatus = lImporter->Import(pScene);

        if (lStatus == false && lImporter->GetStatus() == FbxStatus::ePasswordError)
        {
            FBXSDK_printf("\nPassword is wrong, import aborted.\n");
        }
    }

    if (!lStatus || (lImporter->GetStatus() != FbxStatus::eSuccess))
    {
        FBXSDK_printf("********************************************************************************\n");
        if (lStatus)
        {
            FBXSDK_printf("WARNING:\n");
            FBXSDK_printf("   The importer was able to read the file but with errors.\n");
            FBXSDK_printf("   Loaded scene may be incomplete.\n\n");
        }
        else
        {
            FBXSDK_printf("Importer failed to load the file!\n\n");
        }

        if (lImporter->GetStatus() != FbxStatus::eSuccess)
            FBXSDK_printf("   Last error message: %s\n", lImporter->GetStatus().GetErrorString());

        FbxArray<FbxString*> history;
        lImporter->GetStatus().GetErrorStringHistory(history);
        if (history.GetCount() > 1)
        {
            FBXSDK_printf("   Error history stack:\n");
            for (int i = 0; i < history.GetCount(); i++)
            {
                FBXSDK_printf("      %s\n", history[i]->Buffer());
            }
        }
        FbxArrayDelete<FbxString*>(history);
        FBXSDK_printf("********************************************************************************\n");
    }

    // Destroy the importer.
    lImporter->Destroy();

    return lStatus;
}

void DestroySdkObjects(FbxManager* pManager, bool pExitStatus)
{
    //Delete the FBX Manager. All the objects that have been allocated using the FBX Manager and that haven't been explicitly destroyed are also automatically destroyed.
    if (pManager) pManager->Destroy();
    if (pExitStatus) FBXSDK_printf("Program Success!\n");
}

void Split(char split_char, char* buffer, std::vector<std::string>& out)
{
    int count = 0;
    if (buffer == nullptr)
    {
        return;
    }

    int start_point = 0;

    while (buffer[count] != '\0')
    {
        if (buffer[count] == split_char)
        {
            if (start_point != count)
            {
                char split_str[256] = { 0 };
                strncpy_s(split_str, 256, &buffer[start_point], count - start_point);
                out.emplace_back(split_str);
            }
            else
            {
                out.emplace_back("");
            }
            start_point = count + 1;
        }
        count++;
    }

    if (start_point != count)
    {
        char split_str[256] = { 0 };
        strncpy_s(split_str, 256, &buffer[start_point], count - start_point);
        out.emplace_back(split_str);
    }
}

void Split(char split_char, std::string targetStr, std::vector<std::string>& out)
{
    char buffer[512];
    memset(buffer, 0, 512 * sizeof(char));
    memcpy(buffer, targetStr.c_str(), sizeof(char) * 512);
    std::vector<std::string> splitList;
    std::string replace_file_name = buffer;
    // split_charで分解
    Split(split_char, buffer, splitList);
    out = splitList;
}

void Converter(std::string filePath)
{
    const char* lFileTypes[] =
    {
        "_dae.dae",            "Collada DAE (*.dae)",
        "_fbx7binary.fbx", "FBX binary (*.fbx)",
        "_fbx7ascii.fbx",  "FBX ascii (*.fbx)",
        "_fbx6binary.fbx", "FBX 6.0 binary (*.fbx)",
        "_fbx6ascii.fbx",  "FBX 6.0 ascii (*.fbx)",
        "_obj.obj",            "Alias OBJ (*.obj)",
        "_dxf.dxf",            "AutoCAD DXF (*.dxf)"
    };

    FbxManager* lSdkManager = NULL;
    FbxScene* lScene = NULL;
    std::string meshName;
    // Mesh Nnameの取り出し
    {
        std::vector<std::string> wordArray;
        Split('/', filePath, wordArray);
        std::string meshFileName = wordArray[wordArray.size() - 1];
        wordArray.clear();
        Split('.', meshFileName, wordArray);
        meshName = wordArray[wordArray.size() - 2];
    }

    // Prepare the FBX SDK.
    InitializeSdkObjects(lSdkManager, lScene);

    bool lResult = LoadScene(lSdkManager, lScene, filePath.c_str());
    if (lResult)
    {
        const size_t lFileTypeCount = sizeof(lFileTypes) / sizeof(lFileTypes[0]) / 2;

        for (size_t i = 0; i < lFileTypeCount; ++i)
        {
            // Retrieve the writer ID according to the description of file format.
            int lFormat = lSdkManager->GetIOPluginRegistry()->FindWriterIDByDescription(lFileTypes[i * 2 + 1]);

            // Construct the output file name.
            std::string newFileName = meshName + lFileTypes[i * 2];
            //FBXSDK_strcpy(lNewFileName + lFileNameLength - 4, 60, lFileTypes[i * 2]);

            // Create an exporter.
            FbxExporter* lExporter = FbxExporter::Create(lSdkManager, "");

            // Initialize the exporter.
            lResult = lExporter->Initialize(newFileName.c_str(), lFormat, lSdkManager->GetIOSettings());
            if (!lResult)
            {
                FBXSDK_printf("%s:\tCall to FbxExporter::Initialize() failed.\n", lFileTypes[i * 2 + 1]);
                FBXSDK_printf("Error returned: %s\n\n", lExporter->GetStatus().GetErrorString());
            }
            else
            {
                // Export the scene.
                lResult = lExporter->Export(lScene);
                if (!lResult)
                {
                    FBXSDK_printf("Call to FbxExporter::Export() failed.\n");
                }
            }

            // Destroy the exporter.
            lExporter->Destroy();
        }
        //delete[] lNewFileName;
    }
    else
    {
        FBXSDK_printf("Call to LoadScene() failed.\n");
    }

    // Delete the FBX SDK manager. All the objects that have been allocated 
    // using the FBX SDK manager and that haven't been explicitly destroyed 
    // are automatically destroyed at the same time.
    DestroySdkObjects(lSdkManager, lResult);
}

int main(int argc, char** argv)
{
    //Converter("UnityChan.fbx");   // 7.4.0
    Converter("unitychan_RUN00_F.fbx");   // 7.4.0
    //Converter("TreasureBox2.fbx");    // 7.4.0
    //Converter("Bush_1.fbx");    // 7.4.0
    //Converter("deSchoolDesk.fbx");    // 7.4.0

    return 0;
}
