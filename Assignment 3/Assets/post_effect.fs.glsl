#version 420 core                                                        
                                         							 
layout (binding = 0) uniform sampler2D tex;

out vec4 color;                                                              
                                                                            
in VS_OUT                                                                   
{                                                                            
  vec2 texcoord;   

} fs_in;

void main(void) {
    color = vec4(texture(tex,fs_in.texcoord).rgb, 1.0);
}