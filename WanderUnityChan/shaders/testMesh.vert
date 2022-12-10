#version 330 core

// 位置だけ受け取って描画

layout (location = 0) in vec3 Position;                                              
layout (location = 1) in vec3 Normal; 
layout (location = 2) in vec2 TexCoord; 
layout (location = 3) in ivec4 BoneIDs;
layout (location = 4) in vec4 Weights;

uniform mat4 ModelTransform;
uniform mat4 CameraView;
uniform mat4 CameraProj;
// SpotLight用
uniform mat4 LightView;
// uniform mat4 LightProj;
uniform mat4 uMatrixPalette[150];

out vec4 LightSpacePos;                                                             
out vec2 TexCoord0;                                                                 
out vec3 Normal0;                                                                   
out vec3 WorldPos0; 


void main()
{
	gl_Position = CameraProj * CameraView * ModelTransform * vec4(Position, 1.0);
	// gl_Position = vec4(Position, 1.0);
	// LightSpacePos = CameraProj * LightView * ModelTransform * vec4(Position, 1.0);                                 
    WorldPos0 = (ModelTransform * vec4(Position, 1.0)).xyz;                          
	Normal0 = (ModelTransform * vec4(Normal, 0.0)).xyz;                            
	TexCoord0 = TexCoord;
}
