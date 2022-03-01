#version 420 core

layout (binding = 1) uniform sampler2D sobj_tex;
layout (binding = 2) uniform sampler2D snoobj_tex;
layout (binding = 3) uniform sampler2D sb_tex;

out vec4 color;

in VS_OUT
{
	vec2 texcoord;
} fs_in;


void main()
{
	vec4 sobj = texture(sobj_tex, fs_in.texcoord);
	vec4 snoobj = texture(snoobj_tex, fs_in.texcoord);
	vec4 sb = texture(sb_tex, fs_in.texcoord);
	color = sb * (sobj / snoobj);
}