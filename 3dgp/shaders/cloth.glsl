#version 430

layout (local_size_x = 10, local_size_y = 10) in;

uniform vec3 Gravity = vec3(0, -10, 0);
uniform float ParticleMass = 0.1;
uniform float ParticleInvMass = 1.0 / 0.1;
uniform float SpringK = 2000.0;
uniform float RestLengthHoriz;
uniform float RestLengthVert;
uniform float RestLengthDiag;
uniform float RestLengthHorizBending;
uniform float RestLengthVertBending;
uniform float DeltaT = 0.000005;
uniform float DampingConst = 0.1;

uniform vec3 velIn = vec3(0, 0, 0);

uniform float directionChangeX = 0.0;
uniform float directionChangeY = 0.0;
uniform float windForce = 1.0;
vec3 windVec = { 0.4, -0.2, 0.0 };

layout(std430, binding=0) buffer PosIn
{
	vec4 PositionIn[];
};
layout(std430, binding=1) buffer PosOut
{
	vec4 PositionOut[];
};
layout(std430, binding=2) buffer VelIn
{
	vec4 VelocityIn[];
};
layout(std430, binding=3) buffer VelOut
{
	vec4 VelocityOut[];
};
layout(std430, binding=4) buffer NormOut
{
	vec4 NormalIn[];
};

void main() 
{
	uvec3 nParticles = gl_NumWorkGroups * gl_WorkGroupSize;
	uint idx = gl_GlobalInvocationID.y * nParticles.x + gl_GlobalInvocationID.x;

	vec3 p = vec3(PositionIn[idx]);
	vec3 v = vec3(VelocityIn[idx]), r;

	// Gravitational acceleration
	vec3 force = Gravity * ParticleMass;

	// Spring forces from each neighboring particle - acc. to Hooke's Law: F = -k * dx
	if (gl_GlobalInvocationID.y < nParticles.y - 1)		// above
	{
		r = PositionIn[idx + nParticles.x].xyz - p;
		force += normalize(r) * SpringK * (length(r) - RestLengthVert);
	} 
	if (gl_GlobalInvocationID.y > 0)					// below
	{
		r = PositionIn[idx - nParticles.x].xyz - p;
		force += normalize(r) * SpringK * (length(r) - RestLengthVert);
	} 
	if (gl_GlobalInvocationID.x > 0)					// left
	{
		r = PositionIn[idx-1].xyz - p;
		force += normalize(r) * SpringK * (length(r) - RestLengthHoriz);
		
	} 
	if (gl_GlobalInvocationID.x < nParticles.x - 1)		// right
	{
		r = PositionIn[idx + 1].xyz - p;
		force += normalize(r) * SpringK * (length(r) - RestLengthHoriz);
		
	}
	if (gl_GlobalInvocationID.x > 0 && gl_GlobalInvocationID.y < nParticles.y - 1)						// diagonally: upper-left
	{
		r = PositionIn[idx + nParticles.x - 1].xyz - p;
		force += normalize(r) * SpringK * (length(r) - RestLengthDiag);
	}
	if (gl_GlobalInvocationID.x < nParticles.x - 1 && gl_GlobalInvocationID.y < nParticles.y - 1)		// diagonally: upper-right
	{
		r = PositionIn[idx + nParticles.x + 1].xyz - p;
		force += normalize(r) * SpringK * (length(r) - RestLengthDiag);
	}
	if (gl_GlobalInvocationID.x > 0 && gl_GlobalInvocationID.y > 0)										// diagonally: lower-left
	{
		r = PositionIn[idx - nParticles.x - 1].xyz - p;
		force += normalize(r) * SpringK * (length(r) - RestLengthDiag);
	}
	if (gl_GlobalInvocationID.x < nParticles.x - 1 && gl_GlobalInvocationID.y > 0)						// diagonally: lower-right
	{
		r = PositionIn[idx - nParticles.x + 1].xyz - p;
		force += normalize(r) * SpringK * (length(r) - RestLengthDiag);
	}

	// Bending Springs
	if (gl_GlobalInvocationID.x < nParticles.x - 2) {
		r = PositionIn[idx + 2].xyz - p;
		force += normalize(r) * SpringK * (length(r) - RestLengthHorizBending);
	}
	if (gl_GlobalInvocationID.x > 1) {
		r = PositionIn[idx - 2].xyz - p;
		force += normalize(r) * SpringK * (length(r) - RestLengthHorizBending);
	}
	if (gl_GlobalInvocationID.y < nParticles.y - 2) {
		r = PositionIn[idx + 2 * nParticles.x].xyz - p;
		force += normalize(r) * SpringK * (length(r) - RestLengthVertBending);
	}
	if (gl_GlobalInvocationID.y > 1) {
		r = PositionIn[idx - 2 * nParticles.x].xyz - p;
		force += normalize(r) * SpringK * (length(r) - RestLengthVertBending);
	}

	vec3 normal = NormalIn[idx].xyz;

	windVec.y += directionChangeY;
	windVec.z += directionChangeX;

	float dot = dot(windVec, normal);

	force += (windForce * dot) * windVec;
	force += -DampingConst * v;

	// Integrate over Time
	vec3 i = force * ParticleInvMass;
	PositionOut[idx] = vec4(p + v * DeltaT + 0.5 * i * DeltaT * DeltaT, 1.0);
	VelocityOut[idx] = vec4(velIn + v + i * DeltaT, 0.0);
	
	// Position and Velocity Constraints
	if (gl_GlobalInvocationID.y == nParticles.y - 1 && (
			gl_GlobalInvocationID.x == 0 || 
			gl_GlobalInvocationID.x == nParticles.x * 1 / 4 ||
			gl_GlobalInvocationID.x == nParticles.x * 2 / 4 ||
			gl_GlobalInvocationID.x == nParticles.x * 3 / 4 ||
			gl_GlobalInvocationID.x == nParticles.x - 1 ||
			false)) 
	{
		PositionOut[idx] = vec4(p, 1.0);
		VelocityOut[idx] = vec4(0.0, 0.0, 0.0, 0.0);
	}
}