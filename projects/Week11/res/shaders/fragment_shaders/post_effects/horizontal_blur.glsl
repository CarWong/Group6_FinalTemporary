#version 430

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec3 outColor;

uniform layout(binding = 0) sampler2D s_Image;

uniform float u_step;
uniform float u_weights[5];

void main()
{
	vec4 result = vec4(0.0);

	result += texture(s_Image, inUV) * u_weights[0];

	result += texture(s_Image, vec2((inUV.x + u_step), inUV.y)) * u_weights[1];
	result += texture(s_Image, vec2((inUV.x - u_step), inUV.y)) * u_weights[1];

	result += texture(s_Image, vec2((inUV.x + u_step * 2.0), inUV.y)) * u_weights[2];
	result += texture(s_Image, vec2((inUV.x - u_step * 2.0), inUV.y)) * u_weights[2];

	result += texture(s_Image, vec2((inUV.x + u_step * 3.0), inUV.y)) * u_weights[3];
	result += texture(s_Image, vec2((inUV.x - u_step * 3.0), inUV.y)) * u_weights[3];

	result += texture(s_Image, vec2((inUV.x + u_step * 4.0), inUV.y)) * u_weights[4];
	result += texture(s_Image, vec2((inUV.x - u_step * 4.0), inUV.y)) * u_weights[4];

	outColor = result.rgb;
}