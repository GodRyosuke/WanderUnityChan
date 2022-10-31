#include "FBXMesh.hpp"
#include "glew.h"
#include "Shader.hpp"

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

    FbxGeometryConverter geometryConv = FbxGeometryConverter(mManager);
    geometryConv.Triangulate(scene, true);
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

    DrawArrayPB();

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
    std::vector<glm::vec3> positions(PolygonNum * 3);
    int currentSize = mPositions.size();
    mPositions.resize(currentSize + PolygonNum * 3);  // 3�p�`�|���S��
    for (int p = 0; p < PolygonNum; p++) {
        //mIndices.push_back(p * 3);
        //mIndices.push_back(p * 3 + 1);
        //mIndices.push_back(p * 3 + 2);
        int IndexNumInPolygon = mesh->GetPolygonSize(p);  // p�Ԗڂ̃|���S���̒��_��
        for (int n = 0; n < IndexNumInPolygon; n++) {
            // �|���S��p���\������n�Ԗڂ̒��_�̃C���f�b�N�X�ԍ�
            int IndexNumber = mesh->GetPolygonVertex(p, n);
            mIndices.push_back(IndexNumber);
            FbxVector4 vec = mesh->GetControlPointAt(IndexNumber);
            mPositions[currentSize + p * 3 + n] = glm::vec3(
                static_cast<float>(vec[0]),
                static_cast<float>(vec[1]),
                static_cast<float>(vec[2])
            );
        }
    }



    // ���_�f�[�^�̎擾
    int controlNum = mesh->GetControlPointsCount();   // ���_��
    FbxVector4* src = mesh->GetControlPoints();    // ���_���W�z��
    //for (int i = 0; i < controlNum; ++i) {
    //    mPositions.push_back(glm::vec3(src[i][0], src[i][1], src[i][2]));
    //}

    int layerNum = mesh->GetLayerCount();
    for (int i = 0; i < layerNum; ++i) {
        FbxLayer* layer = mesh->GetLayer(i);

        // �@�����̎擾
        FbxLayerElementNormal* normalElem = layer->GetNormals();
        if (normalElem != 0) {
            LoadNormal(normalElem);
        }
        // UV�����擾
        FbxLayerElementUV* uvElem = layer->GetUVs();
        if (uvElem != 0) {
            LoadUV(uvElem);
        }
    }
}

void FBXMesh::LoadNormal(FbxLayerElementNormal* normalElem)
{
    // �@���̐��E�C���f�b�N�X
    int normalNum = normalElem->GetDirectArray().GetCount();
    int indexNum = normalElem->GetIndexArray().GetCount();

    // �}�b�s���O���[�h�E���t�@�����X���[�h�擾
    FbxLayerElement::EMappingMode mappingMode = normalElem->GetMappingMode();
    FbxLayerElement::EReferenceMode refMode = normalElem->GetReferenceMode();
    assert(refMode == FbxLayerElement::eDirect);    // eDirect����Ȃ��ƑΉ��ł��Ȃ�
    if (mappingMode == FbxLayerElement::eByPolygonVertex) {
        if (refMode == FbxLayerElement::eDirect) {
            // ���ڎ擾
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
            // ���ڎ擾
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
    // UV�̐��E�C���f�b�N�X
    int UVNum = uvElement->GetDirectArray().GetCount();
    int indexNum = uvElement->GetIndexArray().GetCount();
    int size = UVNum > indexNum ? UVNum : indexNum;

    // �}�b�s���O���[�h�E���t�@�����X���[�h�ʂ�UV�擾
    FbxLayerElementUV::EMappingMode mappingMode = uvElement->GetMappingMode();
    FbxLayerElementUV::EReferenceMode refMode = uvElement->GetReferenceMode();
    if ((mappingMode == FbxLayerElementUV::eByPolygonVertex) || (mappingMode == FbxLayerElementUV::eByControlPoint)) {
        if (refMode == FbxLayerElementUV::eDirect) {
            // ���ڎ擾
            for (int i = 0; i < size; ++i) {
                mTexCoords.push_back(glm::vec2(
                    static_cast<float>(uvElement->GetDirectArray().GetAt(i)[0]),
                    static_cast<float>(uvElement->GetDirectArray().GetAt(i)[1])
                ));
            }
        }
        else if (refMode == FbxLayerElementUV::eIndexToDirect) {
            // �C���f�b�N�X����擾
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


    // Indef Buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Buffers[INDEX_BUFFER]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(mIndices[0]) * mIndices.size(), &mIndices[0], GL_STATIC_DRAW);
}

void FBXMesh::DrawArrayPB()
{
    glGenVertexArrays(1, &mDrawArrayVAO);
    glBindVertexArray(mDrawArrayVAO);

    // Vertex Buffer�̍쐬
    unsigned int VertexBuffer;
    glGenBuffers(1, &VertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(mPositions[0]) * mPositions.size(), &mPositions[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);


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

    glDrawElements(GL_TRIANGLES,
        mIndices.size(),
        GL_UNSIGNED_INT,
        (void*)(0));
}

void FBXMesh::DrawArray()
{
    glDrawArrays(
        GL_TRIANGLES,
        0,
        mPositions.size()
    );
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

