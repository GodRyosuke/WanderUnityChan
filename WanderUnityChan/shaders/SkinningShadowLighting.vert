#version 330 core

layout (location = 0) in vec3 Position;                                             
layout (location = 1) in vec3 Normal;                                               
layout (location = 2) in vec2 TexCoord;  
layout (location = 3) in ivec4 BoneIDs;
layout (location = 4) in vec4 Weights;


out vec4 LightSpacePos;                                                             
out vec2 TexCoord0;                                                                 
out vec3 Normal0;                                                                   
out vec3 WorldPos0; 


// Inputs the matrices needed for 3D viewing with perspective
uniform mat4 ModelTransform;
uniform mat4 CameraView;
uniform mat4 CameraProj;
// SpotLight—p
uniform mat4 LightView;
// uniform mat4 LightProj;
// Matrix Pallete
uniform mat4 uMatrixPalette[96];

void main()
{
	mat4 BoneTransform = uMatrixPalette[BoneIDs[0]] * Weights[0];
		BoneTransform += uMatrixPalette[BoneIDs[1]] * Weights[1];
		BoneTransform += uMatrixPalette[BoneIDs[2]] * Weights[2];
		BoneTransform += uMatrixPalette[BoneIDs[3]] * Weights[3];
	
	vec4 PosL = BoneTransform * vec4(Position, 1.0);
	vec4 NormalL = BoneTransform * vec4(Normal, 0.0);

	gl_Position = CameraProj * CameraView * ModelTransform * PosL;
	LightSpacePos = CameraProj * LightView * ModelTransform * PosL;                                 
    WorldPos0 = (ModelTransform * PosL).xyz;                          
	Normal0 = (ModelTransform * NormalL).xyz;                            
	TexCoord0 = TexCoord;
}