#pragma once

#include "glad/glad.h"
//#include "glew.h"
#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtc/type_ptr.hpp"
#include <iostream>
#include <assimp/postprocess.h> // Post processing flags
#include <fbxsdk.h>
#include <vector>

#define SIZE_OF_SPLIT_STRING_BUFF 512

namespace GLUtil {
    void GetErr();
    void Split(char split_char, char* buffer, std::vector<std::string>& out);
    void Split(char split_char, std::string targetStr, std::vector<std::string>& out);
    void Replace(char search_char, char replace_char, char* buffer);
    void Split(char split_char, char* buffer, std::vector<std::string>& out);
    void Replace(char search_char, char replace_char, char* buffer);
    glm::mat4 ToGlmMat4(aiMatrix4x4& aiMat);
    glm::mat4 ToGlmMat4(aiMatrix3x3& aiMat);
    glm::mat4 ToGlmMat4(FbxAMatrix mat);
}


