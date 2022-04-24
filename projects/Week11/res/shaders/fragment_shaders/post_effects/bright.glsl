#version 430

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec3 outColor;

uniform layout(binding = 0) sampler2D s_Image;

uniform float u_threshold;

void main()
{
	vec4 colour = texture(s_Image, inUV);
	float lumience = 0.2989 * colour.r + 0.587 * colour.g + 0.114 * colour.b;
	if(lumience > u_threshold) outColor = colour.rgb; 
	else outColor = vec3(0.0);
}