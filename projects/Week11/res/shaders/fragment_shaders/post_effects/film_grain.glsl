#version 430

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec3 outColor;

uniform layout(binding = 0) sampler2D s_Image;

uniform float u_amount;

//from: https://simonharris.co/making-a-noise-film-grain-post-processing-effect-from-scratch-in-threejs/
float random(vec2 fg)
{
	 vec2 k1 = vec2( 23.14069263277926, // e^pi (Gelfond's constant)
    2.665144142690225 // 2^sqrt(2) (Gelfond–Schneider constant)
  );
   return fract(cos(dot(fg, k1) ) * 12345.6789);
}

void main()
{
	vec3 Colour = texture(s_Image, inUV).rgb;

	vec2 intensity = inUV;
	intensity.y *= random(vec2(intensity.y, u_amount));

	Colour += random(intensity) * 0.25;

	outColor = Colour;
}