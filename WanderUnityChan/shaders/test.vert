#version 330 core

// ˆÊ’u‚¾‚¯Žó‚¯Žæ‚Á‚Ä•`‰æ

layout (location = 0) in vec4 Position;                                              
layout (location = 1) in vec3 Normal; 

uniform mat4 ModelTransform;
uniform mat4 CameraView;
uniform mat4 CameraProj;
// SpotLight—p
uniform mat4 LightView;
// uniform mat4 LightProj;

out vec4 LightSpacePos;                                                             
out vec2 TexCoord0;                                                                 
out vec3 Normal0;                                                                   
out vec3 WorldPos0; 


void main()
{
	gl_Position = CameraProj * CameraView * ModelTransform * vec4(Position.xyz, 1.0);
	// gl_Position = vec4(Position, 1.0);
	// LightSpacePos = CameraProj * LightView * ModelTransform * vec4(Position, 1.0);                                 
    // WorldPos0 = (ModelTransform * vec4(Position, 1.0)).xyz;                          
	Normal0 = (ModelTransform * vec4(Normal, 0.0)).xyz;                            
	// TexCoord0 = TexCoord;
}