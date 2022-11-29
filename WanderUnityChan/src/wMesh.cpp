#include "wMesh.hpp"
#include "GLUtil.hpp"

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
        mMeshName = wordArray[wordArray.size() - 1];
    }


    return true;
}
