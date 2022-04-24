#version 430

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec3 outColor;

uniform layout(binding = 0) sampler2D s_Image;

uniform float u_pixels;

void main()
{
	float dx = 12.0 * (1 / u_pixels);
	float dy = 10.0 * (1/ u_pixels);

	vec2 newUV = vec2(dx * floor(inUV.x / dx), dy * floor(inUV.y / dy));
	vec3 Colour = texture(s_Image, newUV).rgb;

	outColor = Colour;
}