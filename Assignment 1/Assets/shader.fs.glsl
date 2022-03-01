#version 330

in vec3 model_color;
in vec2 texCoord;

out vec4 fragColor;

uniform sampler2D diffuseTex;

void main() {
    fragColor = vec4(texture(diffuseTex, texCoord.xy).rgb, 1);
    //fragColor = vec4(model_color, 1);
}
