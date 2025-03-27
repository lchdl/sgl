#version 330 core
layout(location = 0) out vec4 FragColor;
in vec2 TexCoord;
uniform sampler2D tex1;
uniform sampler2D tex2;
void main()
{
	FragColor = mix(texture(tex1, TexCoord), texture(tex2, TexCoord), 0.5);
}