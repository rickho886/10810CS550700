#version 420 core

// Output                                                                      
layout(location = 0) out vec4 color;
layout(binding = 0) uniform sampler2DShadow shadow_tex;
layout (binding = 1) uniform sampler2D sobj_tex;
layout (binding = 2) uniform sampler2D snoobj_tex;
layout (binding = 3) uniform sampler2D sb_tex;

// Input from vertex shader                                                    
in VS_OUT
{
    vec4 shadow_coord;
	vec2 texcoord;
    vec3 N;
    vec3 L;
    vec3 V;
    vec3 normal;
    vec3 view;
} fs_in;

// Material properties                                                         
uniform vec3 diffuse_albedo = vec3(0.35);
uniform vec3 specular_albedo = vec3(0.7);
uniform float specular_power = 200.0;
uniform int index;
uniform samplerCube tex_cubemap;

void main(void)
{
    // Normalize the incoming N, L and V vectors                               
    vec3 N = normalize(fs_in.N);
    vec3 L = normalize(fs_in.L);
    vec3 V = normalize(fs_in.V);
    vec3 H = normalize(L + V);

    // Compute the diffuse and specular components for each fragment           
    vec3 diffuse = max(dot(N, L), 0.0) * diffuse_albedo;
    vec3 specular = pow(max(dot(N, H), 0.0), specular_power) * specular_albedo;

    float shadow_factor = textureProj(shadow_tex, fs_in.shadow_coord);
    // Write final color to the framebuffer                                    
    vec4 blinn_color = vec4(diffuse + specular, 1.0);

    vec3 r = reflect(normalize(fs_in.view), normalize(fs_in.normal));
    vec4 environment_color = texture(tex_cubemap, r);

	//vec4 sobj = texture(sobj_tex, fs_in.texcoord);
	//vec4 snoobj = texture(snoobj_tex, fs_in.texcoord);
	//vec4 sb = texture(sb_tex, fs_in.texcoord);
    //color = 0.65 * blinn_color + 0.35 * environment_color;

    switch (index)
    {
    case 0:
        color = (0.65 * blinn_color + 0.35 * environment_color) * shadow_factor;
        break;
    case 1:
        if (shadow_factor > 0.5) {
            color = vec4(0.64, 0.57, 0.49, 1.0);
        }
        else {
            color = vec4(0.41, 0.36, 0.37, 1.0);
        }
        break;
    }



}
