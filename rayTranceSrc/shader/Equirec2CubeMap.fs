#version 330 core

#define PI 3.1415926

in vec3 localPos;
out vec4 FragColor;

uniform sampler2D equirectangularMap;

vec2 SampleFromEquirec(vec3 pos)
{
	vec2 uv = vec2(atan(pos.z, pos.x), asin(pos.y));
	uv = uv / vec2(2 * PI, PI) + 0.5;
	return uv;
}

void main()
{
	vec2 uv = SampleFromEquirec(normalize(localPos));
	vec3 color = texture(equirectangularMap, uv).rgb;
	// color = pow(color, vec3(1.0 / 2.2));
	FragColor = vec4(color, 1.0);
}