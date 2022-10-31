#version 330                                                                        
                                                                                    
const int MAX_POINT_LIGHTS = 2;                                                     
const int MAX_SPOT_LIGHTS = 2;                                                      
                                                                                    
in vec4 LightSpacePos;                                                              
in vec2 TexCoord0;                                                                  
in vec3 Normal0;                                                                    
in vec3 WorldPos0;                                                                  
                                                                                    
out vec4 FragColor;

void main()
{
    FragColor.r = 0.8;
    FragColor.g = 0.8;
    FragColor.b = 0.8;
}