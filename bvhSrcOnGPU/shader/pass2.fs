#version 330 core

in vec2 screenCoord;

uniform sampler2D texPass0;
uniform sampler2D texPass1;
uniform sampler2D texPass2;

void main()
{
	gl_FragData[0] = vec4(texture2D(texPass0, screenCoord).rgb, 1.0);
}