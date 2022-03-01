#version 420 core                                                        
                                         							 
layout (binding = 0) uniform sampler2D tex;
layout (binding = 1) uniform sampler2D noiseMap;

uniform vec2 mouse = vec2(0.5f, 0.5f);
uniform vec2 resolution = vec2(600.0f, 600.0f);
uniform int mode;
uniform float width;
uniform float height;
uniform float time;

out vec4 color;                                                              
                                                                            
in VS_OUT                                                                   
{                                                                            
  vec2 texcoord;   

} fs_in;

vec4 Blur()
{
	vec4 color = vec4(0);
	int i, j;
	int n = 0;
	int half_size = 3;
	i = -half_size;
	for(; i <= half_size; ++i){
		j = -half_size;
		for(; j <= half_size; ++j){
			vec4 c = texture(tex, fs_in.texcoord + vec2(i, j) / vec2(width, height));
			color += c;
			n++;
		}
	}
	color /= n;
	return color;
}

vec4 SBlur() // second-pass blur
{
	vec4 color = vec4(0);
	int i, j;
	int n = 0;
	int half_size = 6;
	i = -half_size;
	for(;i <= half_size; ++i){
		j = -half_size;
		for(;j <= half_size; ++j){
			vec4 c = texture(tex, fs_in.texcoord + vec2(i, j) / vec2(width, height));
			color += c;
			n++;
		}
	}
	color /= n;
	return color;
}

vec3 Quantization()
{
	vec3 texture_color = texture2D(tex, fs_in.texcoord).rgb;
	texture_color = floor(texture_color * 8.0) / 8.0;
	return texture_color;
}

vec3 DoG(void)
{
	int i, j;
	int halfWidth = int(ceil(2.0 * 2.8f));
	vec2 img_size = vec2(1024, 768);
			
	vec2 sum = vec2(0);
	vec2 norm = vec2(0);
	i = -halfWidth;
	for(;i <= halfWidth; ++i){
		j = -halfWidth;
		for(;j <= halfWidth; ++j){
			float d = length(vec2(i, j));
			vec2 kernel = vec2(exp(-d * d / (2.0 * 2.0f * 2.0f)),
								exp(-d * d / (2.0 * 2.8f * 2.8f)));
			vec4 c = texture(tex, fs_in.texcoord + vec2(i, j) / img_size);
			vec2 L = vec2(0.299 * c.r + 0.587 * c.g + 0.114 * c.b);
			norm = norm + 2.0 * kernel;
			sum = sum + kernel * L;
		}
	}
	sum /= norm;
	float H = 100.0 * (sum.x - 0.99f * sum.y);
	float edge = (H > 0.0) ? 1.0 : 2.0 * smoothstep(-2.0, 2.0, 3.4f * H);
	return vec3(edge, edge, edge);
}

void main(void) {

  vec4 vec4Sum = vec4(0.0);
	 if (mode == 2) { 
		vec2 uv = fs_in.texcoord;
		vec2 mouse = mouse.xy;
		mouse.y = 1.0 - mouse.y;
		if (mouse == vec2(0.0))
			mouse = resolution.xy / 2.0;
    
		vec2 mouse_uv = mouse;
    
		//Distance to mouse
		float mouse_dist = distance(uv, mouse_uv);
    
		//Draw the texture
		vec4Sum = texture(tex, uv);
    
		//Draw the outline of the glass
		if (mouse_dist < 0.31)
			vec4Sum = vec4(0.1, 0.1, 0.1, 1.0);
    
		//Draw a zoomed-in version of the texture
		if (mouse_dist < 0.3)
			vec4Sum = texture(tex, (uv + mouse_uv) / 2.0);
	}
	else if(mode == 6 || mode == 7) {
		vec4Sum = vec4(texture(tex,fs_in.texcoord).rgb, 1.0);
	}
    
	else if (fs_in.texcoord.x < mouse.x - 0.002)
    {
		if(mode == 0){ // abstraction
			
			vec4Sum = Blur() * vec4(Quantization(), 1.0) * vec4(DoG(), 1.0);	
		}
		else if(mode == 1){ // water color

			float dx = 1.0f / resolution.x;
			float dy = 1.0f / resolution.y;
			int blurSize = 1;
			int strokeSize = 10;
			vec2 strokeDir = vec2(0.5, 0.5);
			int nbins = 8;


			vec2 uv = fs_in.texcoord + (1 - texture(noiseMap, fs_in.texcoord).r) * vec2(strokeDir.x * dx, strokeDir.y * dy) * strokeSize;

			vec3 sum = vec3(0, 0, 0);
			for(int i = -blurSize; i < blurSize; i++)
				for(int j = -blurSize; j < blurSize; j++)
					sum += texture(tex, uv + vec2(i * dx, j * dy)).rgb;

			vec3 color = sum / (4 * blurSize * blurSize);

			//Quantization
			float r = floor(color.r * float(nbins)) / float(nbins);
			float g = floor(color.g * float(nbins)) / float(nbins);
			float b = floor(color.b * float(nbins)) / float(nbins);

			vec4Sum = vec4(r, g, b, 1);
		}
		else if(mode == 3){ //bloom
			vec4 blur = SBlur();
			//vec4Sum = texture2D(tex, fs_in.texcoord) * 1.3 + blur * 55 / 256;
			vec4Sum = blur + texture2D(tex, fs_in.texcoord) * 0.55;
		}
		else if(mode == 4){ // pixelation
			float dx = 1.0f / resolution.x;
			float dy = 1.0f / resolution.y;
			int size = 6;

			vec2 baseUV = vec2(0, 0);
			vec2 uvSize = vec2(dx * size, dy * size);

			while(fs_in.texcoord.x >= baseUV.x) {
				baseUV.x += uvSize.x;
			}
			while(fs_in.texcoord.y >= baseUV.y) {
				baseUV.y += uvSize.y;
			}
			baseUV -= uvSize;

			vec3 sum = vec3(0, 0, 0);
			for(int i = 0; i < size; i++)
				for(int j = 0; j < size; j++)
					sum += texture(tex, baseUV + vec2(i * dx, j * dy)).rgb;

			vec4Sum = vec4(sum / (size * size), 1.0);

		}
		else if(mode == 5){ // Sin Wave
			vec2 uv = fs_in.texcoord;
			uv.x = uv.x + 0.05 * (sin(uv.y * 2 * 3.141592 + time));
			vec4Sum = texture2D(tex, uv);
		}
		
    }
    else if (fs_in.texcoord.x > mouse.x + 0.002)
    {
		vec4 color = texture2D(tex, fs_in.texcoord);
        vec4Sum = color;
    }
    else
    {
		vec4 color = vec4(1.0, 0.0, 0.0, 1.0);
        vec4Sum = color;
    }
    color = vec4Sum;
}