#version 330

in float age;
in vec2 texCoord0;

uniform sampler2D texture0;

out vec4 outColor;

void main()
{
	outColor = texture(texture0, gl_PointCoord);
	outColor.a = 1 - outColor.r * outColor.g * outColor.b;
	outColor.a *= age;
}