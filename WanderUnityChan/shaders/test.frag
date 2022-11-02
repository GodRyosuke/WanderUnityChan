#version 330                                                                        
                                                                                    
const int MAX_POINT_LIGHTS = 2;                                                     
const int MAX_SPOT_LIGHTS = 2;                                                      
                                                                                    
in vec4 LightSpacePos;                                                              
in vec2 TexCoord0;                                                                  
in vec3 Normal0;                                                                    
in vec3 WorldPos0;                                                                  
                         
uniform vec3 gEyeWorldPos;
						 
out vec4 FragColor;

void main()
{
	vec3 Direction = vec3(1.0f, 0.0f, -1.0f);
	vec3 AmbientLight = vec3(0.2, 0.2, 0.2);
	vec3 DiffuseColor = vec3(0.5, 0.5, 0.5) * 2.f;
	vec3 SpecColor = vec3(0.5f, 0.5f, 0.5f) * 0.3f;
	float SpecPower = 0.2f;


    // Surface normal
	vec3 N = normalize(Normal0);
	// Vector from surface to light
	vec3 L = normalize(-Direction);
	// Vector from surface to camera
	vec3 V = normalize(gEyeWorldPos - WorldPos0);
	// Reflection of -L about N
	vec3 R = normalize(reflect(-L, N));

	// Compute phong reflection
	vec3 Phong = AmbientLight;
	float NdotL = dot(N, L);
	if (NdotL > 0)
	{
		vec3 Diffuse = DiffuseColor * NdotL;
		vec3 Specular = SpecColor * pow(max(0.0, dot(R, V)), SpecPower);
		Phong += Diffuse + Specular;
	}
    FragColor = vec4(Phong, 1.0f);
	// FragColor = vec4(1.0f);
}

