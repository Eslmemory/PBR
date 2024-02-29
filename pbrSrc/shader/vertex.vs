#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 WorldPos;
out vec3 Normal;
out vec2 TexCoords;

void main()
{
	WorldPos = vec3(model * vec4(position, 1.0f));
	TexCoords = uv;
	Normal = mat3(model) * normal;

	gl_Position = projection * view * model * vec4(position, 1.0f);
}