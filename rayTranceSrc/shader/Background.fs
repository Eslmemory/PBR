#version 330 core

in vec3 pos;
out vec4 FragColor;

uniform samplerCube environmentMap;

void main()
{
	vec3 color = texture(environmentMap, pos).rgb;
	FragColor = vec4(color, 1.0);
}