#version 410 core                                      
                                                           
// Per-vertex inputs                                   
layout(location = 0) in vec3 iv3vertex;
layout(location = 1) in vec2 iv2tex_coord;
layout(location = 2) in vec3 iv3normal;                                                          
                                                           
// Inputs from vertex shader                           
out VS_OUT                                             
{            
    vec4 shadow_coord;
	vec2 texcoord;
    vec3 N;                                            
    vec3 L;                                            
    vec3 V;                   
    vec3 normal;
    vec3 view;
} vs_out;                                   
                                                           
// Position of light                                   
uniform vec3 light_pos = vec3(-31.75, 26.05, -97.72);
uniform mat4 shadow_matrix;
uniform mat4 um4mv;
uniform mat4 um4p;
                                                           
void main(void)                                        
{                                                      
    // Calculate view-space coordinate                 
	vec4 P = um4mv * vec4(iv3vertex, 1.0);          
                                                           
    // Calculate normal in view-space                  
    vs_out.N = mat3(um4mv) * iv3normal;               
                                                           
    // Calculate light vector                          
    vs_out.L = light_pos - P.xyz;                      
                                                           
    // Calculate view vector                           
    vs_out.V = -P.xyz;   
    
    vs_out.shadow_coord = shadow_matrix * vec4(iv3vertex, 1.0);

	vs_out.texcoord = iv2tex_coord;
    
    vs_out.normal = mat3(um4mv) * iv3normal;
    vs_out.view = P.xyz;
                                                           
    // Calculate the clip-space position of each vertex
    gl_Position = um4p * P;                     
}                                                      
                                                           