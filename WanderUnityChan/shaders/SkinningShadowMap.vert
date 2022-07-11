#version 330 core

// ïÅí ÇÃMeshÇï`âÊÇ∑ÇÈ

layout (location = 0) in vec3 Position;                                             
layout (location = 1) in vec3 Normal;                                               
layout (location = 2) in vec2 TexCoord;  
layout (location = 3) in ivec4 BoneIDs;
layout (location = 4) in vec4 Weights;


uniform mat4 ModelTransform;
uniform mat4 LightView;
uniform mat4 CameraProj;
// Matrix Pallete
uniform mat4 uMatrixPalette[96];

out vec2 TexCoord0;                                                                 

void main()
{
	mat4 BoneTransform = uMatrixPalette[BoneIDs[0]] * Weights[0];
		BoneTransform += uMatrixPalette[BoneIDs[1]] * Weights[1];
		BoneTransform += uMatrixPalette[BoneIDs[2]] * Weights[2];
		BoneTransform += uMatrixPalette[BoneIDs[3]] * Weights[3];
	vec4 PosL = BoneTransform * vec4(Position, 1.0);

	gl_Position = CameraProj * LightView * ModelTransform * PosL;
	TexCoord0 = TexCoord;
}