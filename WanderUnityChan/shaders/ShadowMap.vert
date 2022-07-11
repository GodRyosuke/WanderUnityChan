#version 330 core

// ïÅí ÇÃMeshÇï`âÊÇ∑ÇÈ

layout (location = 0) in vec3 Position;                                             
layout (location = 1) in vec3 Normal;                                               
layout (location = 2) in vec2 TexCoord;  


uniform mat4 ModelTransform;
uniform mat4 LightView;
uniform mat4 CameraProj;

out vec2 TexCoord0;                                                                 

void main()
{
	gl_Position = CameraProj * LightView * ModelTransform * vec4(Position, 1.0);
	TexCoord0 = TexCoord;
}