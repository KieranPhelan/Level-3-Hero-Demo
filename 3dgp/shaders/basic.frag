#version 330

// Input Variables (received from Vertex Shader)
in vec4 color;
in vec4 position;
in vec3 normal;
in vec2 texCoord0;

// Materials
uniform vec3 materialAmbient;
uniform vec3 materialDiffuse;
uniform vec3 materialSpecular;
uniform float shininess;

// View Matrix
uniform mat4 matrixView;

// Uniform: The Texture
uniform sampler2D texture0;

uniform sampler2DShadow shadowMap;
in vec4 shadowCoord;

// Output Variable (sent down through the Pipeline)
out vec4 outColor;

struct POINT
{
	vec3 position;
	vec3 diffuse;
	vec3 specular;
};
uniform POINT lightPoint1, lightPoint2, lightPoint3, lightPoint4;

vec4 PointLight(POINT light)
{
	// Calculate Point Light
	vec4 color = vec4(0, 0, 0, 1);
	
	vec3 L = normalize((matrixView * vec4(light.position, 1)) - position).xyz;
	float NdotL = dot(normal, L);
	if (NdotL > 0)
		color += vec4(light.diffuse * materialDiffuse, 1) * NdotL;

	vec3 V = normalize(-position.xyz);
	vec3 R = reflect(-L, normal);
	float RdotV = dot(R, V);
	if (NdotL > 0 && RdotV > 0)
		color += vec4(light.specular * materialSpecular * pow(RdotV, shininess), 1);

	float dist = length(matrixView * vec4(light.position, 1) - position);
	float att = 1 / (0.05 * dist * dist);
	color *= att;
	
	return color;
}

void main(void) 
{
	outColor = color;
	outColor += PointLight(lightPoint1);
	outColor += PointLight(lightPoint2);
	outColor += PointLight(lightPoint3);
	outColor += PointLight(lightPoint4);

	// Calculation of the shadow
	float shadow = 1.0;
	if (shadowCoord.w > 0) // if shadowCoord.w < 0 fragment is out of the Light POV
		shadow = 0.5 + 0.5 * textureProj(shadowMap, shadowCoord);

	outColor *= shadow;

	outColor *= texture(texture0, texCoord0);
}
