#version 330

// Uniforms: Transformation Matrices
uniform mat4 matrixProjection;
uniform mat4 matrixView;
uniform mat4 matrixModelView;

// Particle-specific Uniforms
uniform float particleLifetime; // Max Particle Lifetime
uniform float time; // Animation Time
uniform bool reflectedCam;

// Special Vertex Attributes
in float aStartTime; // Particle "birth" time
in vec3 aPos;

// Output Variable (sent to Fragment Shader)
out float age; // age of the particle (0..1)
out vec2 texCoord0;

vec3 gravity = vec3(0.0, -0.05, 0.0); // Gravity Acceleration in world coords

void main()
{
	float t = mod(time - aStartTime, particleLifetime);
	age = t / particleLifetime;

	vec3 pos = aPos + vec3(0, -1, 0) * t * 20;

	if (!reflectedCam) {
		if ((pos.x > -9.4f && pos.x < 9.7f) &&
		(pos.y < 10.2f) &&
		(pos.z > -6.3f && pos.z < 12.7f)) {
			age = 0;
		}
	}
	else {
		if (pos.z > -6.3f ||
			pos.x < 1.5f) {
			age = 0;
		}
		else {
			age /= 1.5;
		}
	}

	// calculate position (normal calculation not applicable here)
	vec4 position = matrixModelView * vec4(pos, 1.0);
	gl_Position = matrixProjection * position;
}