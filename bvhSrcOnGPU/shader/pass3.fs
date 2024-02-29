#version 330 core

in vec2 screenCoord;
out vec4 FragColor;

uniform sampler2D texPass0;
uniform sampler2D texPass1;
uniform sampler2D texPass2;

vec3 toneMapping(in vec3 c, float limit) {
    float luminance = 0.3 * c.x + 0.6 * c.y + 0.1 * c.z;
    return c * 1.0 / (1.0 + luminance / limit);
}

void main()
{
	vec3 color = texture2D(texPass0, screenCoord).rgb;
	color = toneMapping(color, 1.5);
	color = pow(color, vec3(1.0 / 2.2));

	FragColor = vec4(color, 1.0);
}