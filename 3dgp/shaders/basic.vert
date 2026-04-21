#version 330

// Uniforms: Transformation Matrices
uniform mat4 matrixProjection;
uniform mat4 matrixView;
uniform mat4 matrixModelView;
uniform vec4 planeClip;

// Bone Transforms
#define MAX_BONES 100
uniform mat4 bones[MAX_BONES];

// Uniforms: Material Colours
uniform vec3 materialAmbient;
uniform vec3 materialDiffuse;
uniform vec3 materialSpecular;
uniform float shininess;

uniform mat4 matrixShadow;

in ivec4 aBoneId; // Bone Ids
in vec4 aBoneWeight; // Bone Weights

in vec3 aVertex;
in vec3 aNormal;
in vec2 aTexCoord;

out vec4 color;
out vec4 position;
out vec3 normal;
out vec2 texCoord0;

out vec4 shadowCoord;

// Light declarations
struct AMBIENT
{	
	vec3 color;
};
uniform AMBIENT lightAmbient, lightEmissive;

struct DIRECTIONAL
{	
	vec3 direction;
	vec3 diffuse;
};
uniform DIRECTIONAL lightDir;

vec4 AmbientLight(AMBIENT light)
{
	// Calculate Ambient Light
	return vec4(materialAmbient * light.color, 1);
}

vec4 DirectionalLight(DIRECTIONAL light)
{
	// Calculate Directional Light
	vec4 color = vec4(0, 0, 0, 0);
	vec3 L = normalize(mat3(matrixView) * light.direction);
	float NdotL = dot(normal, L);
	if (NdotL > 0)
		color += vec4(materialDiffuse * light.diffuse, 1) * NdotL;
	return color;
}

void main(void) 
{
	// calculate texture coordinate
	texCoord0 = aTexCoord;

	mat4 matrixBone;
	if (aBoneWeight[0] == 0.0)
		matrixBone = mat4(1);
	else
		matrixBone = (bones[aBoneId[0]] * aBoneWeight[0] +
					  bones[aBoneId[1]] * aBoneWeight[1] +
					  bones[aBoneId[2]] * aBoneWeight[2] +
					  bones[aBoneId[3]] * aBoneWeight[3]);

	// calculate position
	position = matrixModelView * matrixBone * vec4(aVertex, 1.0);
	gl_Position = matrixProjection * position;

	// calculate normal
	normal = normalize(mat3(matrixModelView) * mat3(matrixBone) * aNormal);

	// setup the clip distance
	gl_ClipDistance[0] = dot(inverse(matrixView) * position, planeClip);

	// calculate shadow coordinate – using the Shadow Matrix
	mat4 matrixModel = inverse(matrixView) * matrixModelView;
	shadowCoord = matrixShadow * matrixModel * vec4(aVertex + aNormal * 0.1, 1);

	// calculate light
	color = vec4(0, 0, 0, 1);
	color += AmbientLight(lightAmbient);
	color += AmbientLight(lightEmissive);
	color += DirectionalLight(lightDir);
}
