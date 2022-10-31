#include "FBXMesh.hpp"
#include "glew.h"

FBXMesh::FBXMesh()
{
    mPositions.resize(0);
    mNormals.resize(0);
    mTexCoords.resize(0);
    mIndices.resize(0);

    mNormals.push_back(glm::vec3());
    mTexCoords.push_back(glm::vec2());
}

FBXMesh::~FBXMesh()
{

}

static std::vector<int> pcount(6);

bool FBXMesh::Load(std::string fileName)
{
    // �}�l�[�W���𐶐�
    mManager = FbxManager::Create();

    // IOSetting�𐶐�
    FbxIOSettings* ioSettings = FbxIOSettings::Create(mManager, IOSROOT);

    // Importer�𐶐�
    FbxImporter* importer = FbxImporter::Create(mManager, "");
    std::string filePath = "./resources/" + fileName + "/" + fileName + ".fbx";
    if (importer->Initialize(filePath.c_str(), -1, mManager->GetIOSettings()) == false) {
        // �C���|�[�g�G���[
        return -1;
    }

    // Scene�I�u�W�F�N�g��FBX�t�@�C�����̏��𗬂�����
    FbxScene* scene = FbxScene::Create(mManager, "scene");
    importer->Import(scene);
    importer->Destroy(); // �V�[���𗬂����񂾂�Importer�͉������OK

    // Scene���
    FbxNode* rootNode = scene->GetRootNode();
    if (rootNode == 0) {
        printf("error: cannont find root node: %s\n", fileName.c_str());
        return false;
    }
    ShowNodeNames(rootNode, 0);
    LoadNode(rootNode);

    // Create VAO
    glGenVertexArrays(1, &mVertexArray);
    glBindVertexArray(mVertexArray);

    // Vertex Buffer�̍쐬
    PopulateBuffers();


    // unbind cube vertex arrays
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // ...

    // �}�l�[�W�����
    // �֘A���邷�ׂẴI�u�W�F�N�g����������
    mManager->Destroy();

	return true;
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
    const char* name = node->GetName();
    int attrCount = node->GetNodeAttributeCount();
    for (int i = 0; i < attrCount; ++i) {
        FbxNodeAttribute* attr = node->GetNodeAttributeByIndex(i);
        FbxNodeAttribute::EType type = attr->GetAttributeType();
        if (type == FbxNodeAttribute::EType::eMesh) {   // Mesh Node�Ȃ�
            FbxMesh* pMesh = static_cast<FbxMesh*>(attr);
            LoadMesh(pMesh);
        }
    }

    int childCount = node->GetChildCount();
    for (int i = 0; i < childCount; ++i) {
        LoadNode(node->GetChild(i));
    }
}

void FBXMesh::LoadMesh(FbxMesh* mesh)
{
    // Mesh Node�̓ǂݍ��ݏ���
    int PolygonNum = mesh->GetPolygonCount();
    int PolygonVertexNum = mesh->GetPolygonVertexCount();
    int* IndexAry = mesh->GetPolygonVertices();


    for (int p = 0; p < PolygonNum; p++) {
        int IndexNumInPolygon = mesh->GetPolygonSize(p);  // p�Ԗڂ̃|���S���̒��_��
        for (int n = 0; n < IndexNumInPolygon; n++) {
            // �|���S��p���\������n�Ԗڂ̒��_�̃C���f�b�N�X�ԍ�
            int IndexNumber = mesh->GetPolygonVertex(p, n);
            mIndices.push_back(IndexNumber);
        }
    }

    int controlNum = mesh->GetControlPointsCount();   // ���_��
    FbxVector4* src = mesh->GetControlPoints();    // ���_���W�z��

    // �R�s�[
    for (int i = 0; i < controlNum; ++i) {
        mPositions.push_back(glm::vec3(src[i][0], src[i][1], src[i][2]));
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


    // Indef Buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Buffers[INDEX_BUFFER]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(mIndices[0]) * mIndices.size(), &mIndices[0], GL_STATIC_DRAW);
}

void FBXMesh::BindVertexArray()
{
    glBindVertexArray(mVertexArray);
}

void FBXMesh::UnBindVertexArray()
{
    glBindVertexArray(0);
}

void FBXMesh::ShowNodeNames(FbxNode* node, int indent)
{
    // indent�̐������󔒂�`��
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

