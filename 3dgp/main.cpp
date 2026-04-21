#define _USE_MATH_DEFINES
#include <iostream>
#include <GL/glew.h>
#include <3dgl/3dgl.h>
#include <GL/glut.h>
#include <GL/freeglut_ext.h>

// Include GLM core features
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#pragma comment (lib, "glew32.lib")

using namespace std;
using namespace _3dgl;
using namespace glm;

// 3D Models
C3dglModel abe;
C3dglModel idle, run, turnRight, turnLeft, turnAround;

C3dglModel table;
C3dglModel vase;
C3dglModel chicken;
C3dglModel lamp;
C3dglModel walllamp;
C3dglModel window;
C3dglModel mirror;
C3dglModel door;
C3dglModel room;
unsigned nPyramidBuf = 0;

C3dglTerrain terrain, roadMap;
C3dglBitmap grass;
C3dglBitmap road;

// Textures
GLuint idTextile;
GLuint idTexWood;
GLuint idTexCloth;
GLuint idTexGrass;
GLuint idTexRoad;
GLuint idTexNone;

// GLSL Objects (Shader Program)
C3dglProgram program;
C3dglProgram programComp;
C3dglProgram programNorm;
C3dglProgram programParticle;

// Cloth-specific variables
const int X_PARTICLES = 60;
const int Y_PARTICLES = 90;

const float X_CLOTH_SIZE = 16.6f;
const float Y_CLOTH_SIZE = 25.0f;

bool bCompShaderSupported = false;	// Compute Shaders are only supported in OpenGL 4.3 and higher

GLuint idBufPos[2], idBufVel[2];	// Storage Buffers: for Position and Velocity (double vectors)
GLuint idBufNormal;					// Normal Buffer
GLuint idBufTexCoords;				// Texture Coordinate Buffer
GLuint idBufIndex;					// Index Buffer
GLuint idVAOCloat = 0;				// VAO (Vertex Array Object)
GLuint idTexShadowMap;
GLuint idFBO;

// The View Matrix
mat4 matrixView;

// Camera & navigation
float maxspeed = 20.f;	// camera max speed
float accel = 20.f;		// camera acceleration
vec3 _acc(0), _vel(0);	// camera acceleration and velocity vectors
float _fov = 60.f;		// field of view (zoom)

// Particle System
GLuint idTexParticle;
GLuint idBufferVelocity;
GLuint idBufferStartTime;
GLuint idBufferPos;

// Particle System Params
const float PERIOD = 0.00015f;
const float LIFETIME = 3;
const int NPARTICLES = (int)(LIFETIME / PERIOD);

const float GRAVITY = 0.0f; // Disable gravity

// light switches
float fAmbient = 1, fDir = 1, fPoint1 = 1, fPoint2 = 1, fPoint3 = 1, fPoint4 = 1;

float angleFrame = 142.0f;
float angleMirror = 0.0f;
mat4 matrixReflection;

float directionTimer = 0.0;
float directionChangeY = 0.0;
float directionChangeX = 0.0;
float windForce = 1.0;

vec3 lightSource = vec3(-0.95f, 4.24f, 1.0f);

float abeDirection = 90.0f;
vec3 abePosition = vec3(-5.0f, 0.0f, 6.0f);
vec3 abeVel = vec3(0.0f, 0.0f, 0.0f);
vec3 abeAcc = vec3(0.0f, 0.0f, 0.0f);
float abeMaxSpeed = 0.5f;
float abeAccel = 0.1f;
bool stop = false;
int newAnim = -1;
float initTime = 0.0f;
double animationDuration = 0;
bool once = true;
float stopTimer = 0;

vector<vec3> waypoints = {
	vec3(4.3f, 0.0f, 6.0f),
	vec3(4.3f, 0.0f, -10.0f),
	vec3(-5.0f, 0.0f, -10.0f),
	vec3(4.3f, 0.0f, -10.0f),
	vec3(4.3f, 0.0f, 6.0f),
	vec3(-5.0f, 0.0f, 6.0f)
};

int waypointIndex = 0;

vector<mat4> transforms0;

bool init()
{
	// rendering states
	glEnable(GL_DEPTH_TEST);	// depth test is necessary for most 3D scenes
	glEnable(GL_NORMALIZE);		// normalization is needed by AssImp library models
	glShadeModel(GL_SMOOTH);	// smooth shading mode is the default one; try GL_FLAT here!
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);	// this is the default one; try GL_LINE!

	// primitive restart: using 0xffffff as an index in index buffer will restart a primitive
	glEnable(GL_PRIMITIVE_RESTART);
	glPrimitiveRestartIndex(0xffffff);

	// Enable Alpha Blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Initialise Shaders
	C3dglShader vertexShader;
	C3dglShader fragmentShader;
	C3dglShader CompShader;

	// Initialise Shader - Particle
	if (!vertexShader.create(GL_VERTEX_SHADER)) return false;
	if (!vertexShader.loadFromFile("shaders/particles.vert")) return false;
	if (!vertexShader.compile()) return false;

	if (!fragmentShader.create(GL_FRAGMENT_SHADER)) return false;
	if (!fragmentShader.loadFromFile("shaders/particles.frag")) return false;
	if (!fragmentShader.compile()) return false;

	if (!programParticle.create()) return false;
	if (!programParticle.attach(vertexShader)) return false;
	if (!programParticle.attach(fragmentShader)) return false;
	if (!programParticle.link()) return false;
	if (!programParticle.use(true)) return false;

	// Initialise Shader - Basic
	if (!vertexShader.create(GL_VERTEX_SHADER)) return false;
	if (!vertexShader.loadFromFile("shaders/basic.vert")) return false;
	if (!vertexShader.compile()) return false;

	if (!fragmentShader.create(GL_FRAGMENT_SHADER)) return false;
	if (!fragmentShader.loadFromFile("shaders/basic.frag")) return false;
	if (!fragmentShader.compile()) return false;

	if (!program.create()) return false;
	if (!program.attach(vertexShader)) return false;
	if (!program.attach(fragmentShader)) return false;
	if (!program.link()) return false;
	if (!program.use(true)) return false;

	// Initialise Shader - Compute
	bCompShaderSupported = CompShader.create(GL_COMPUTE_SHADER);
	if (bCompShaderSupported)
	{
		if (!CompShader.loadFromFile("shaders/cloth.glsl")) return false;
		if (!CompShader.compile()) return false;

		if (!programComp.create()) return false;
		if (!programComp.attach(CompShader)) return false;
		if (!programComp.link()) return false;

		if (!CompShader.create(GL_COMPUTE_SHADER)) return false;
		if (!CompShader.loadFromFile("shaders/cloth_normal.glsl")) return false;
		if (!CompShader.compile()) return false;

		if (!programNorm.create()) return false;
		if (!programNorm.attach(CompShader)) return false;
		if (!programNorm.link()) return false;

		float dx = X_CLOTH_SIZE / (X_PARTICLES - 1);
		float dy = Y_CLOTH_SIZE / (Y_PARTICLES - 1);
		programComp.sendUniform("RestLengthHoriz", dx);
		programComp.sendUniform("RestLengthVert", dy);
		programComp.sendUniform("RestLengthDiag", sqrtf(dx * dx + dy * dy));
		programComp.sendUniform("RestLengthHorizBending", 2 * dx);
		programComp.sendUniform("RestLengthVertBending", 2 * dy);
	}
	else
	{
		cerr << "*** Compute Shaders require OpenGL Version 4.3 or higher ***" << endl;
		cerr << "Compute Shaders will be disabled." << endl;
	}

	// Prepare the particle buffers
	vector<float> bufferVelocity;
	vector<float> bufferStartTime;
	vector<float> bufferPos;
	float time = 0;

	for (int i = 0; i < NPARTICLES; i++)
	{
		float theta = pi<float>() / 6.f * (float)rand() / (float)RAND_MAX;
		float phi = pi<float>() * 2.f * (float)rand() / (float)RAND_MAX;
		float x = sin(theta) * cos(phi);
		float y = cos(theta);
		float z = sin(theta) * sin(phi);

		float v = 2 + 0.5f * (float)rand() / (float)RAND_MAX;
		vec3 velocity = vec3(x * v, y * v, z * v);

		float r1;
		float r2;

		do
		{
			r1 = 80.f * (float)rand() / (float)RAND_MAX - 40.0f;
			r2 = 80.f * (float)rand() / (float)RAND_MAX - 40.0f;
		} while (sqrt(r1 * r1 + r2 * r2) > 40.0f);

		bufferPos.push_back(r1);
		bufferPos.push_back(35.0f);
		bufferPos.push_back(r2);

		bufferVelocity.push_back(velocity.x);
		bufferVelocity.push_back(1.0f);
		bufferVelocity.push_back(velocity.z);

		bufferStartTime.push_back(time);
		time += PERIOD;
	}

	glGenBuffers(1, &idBufferVelocity);
	glBindBuffer(GL_ARRAY_BUFFER, idBufferVelocity);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * bufferVelocity.size(), &bufferVelocity[0], GL_STATIC_DRAW);

	glGenBuffers(1, &idBufferStartTime);
	glBindBuffer(GL_ARRAY_BUFFER, idBufferStartTime);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * bufferStartTime.size(), &bufferStartTime[0], GL_STATIC_DRAW);

	glGenBuffers(1, &idBufferPos);
	glBindBuffer(GL_ARRAY_BUFFER, idBufferPos);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * bufferPos.size(), &bufferPos[0], GL_STATIC_DRAW);

	// glut additional setup
	glutSetVertexAttribCoord3(program.getAttribLocation("aVertex"));
	glutSetVertexAttribNormal(program.getAttribLocation("aNormal"));

	// initialize buffers
	void initBuffers();
	initBuffers();

	program.use();

	// load your 3D models here!
	idle.load("models\\character\\idle.fbx");
	run.load("models\\character\\walking.fbx");
	turnRight.load("models\\character\\turn_right.fbx");
	turnLeft.load("models\\character\\turn_left.fbx");
	turnAround.load("models\\character\\turn_around.fbx");

	abe.load("models\\character\\abe.fbx");
	abe.loadMaterials("models\\character\\");
	abe.loadAnimations(&idle);
	abe.loadAnimations(&run);
	abe.loadAnimations(&turnRight);
	abe.loadAnimations(&turnLeft);
	abe.loadAnimations(&turnAround);
	cout << abe.getAnimationCount() << endl;
	abe.getAnimData(-1, time, transforms0);

	if (!table.load("models\\table.obj")) return false;
	if (!vase.load("models\\vase.obj")) return false;
	if (!chicken.load("models\\chicken.obj")) return false;
	if (!lamp.load("models\\lamp.obj")) return false;
	if (!window.load("models\\window.fbx")) return false;
	if (!mirror.load("models\\mirror\\mirror.obj")) return false;
	mirror.loadMaterials("models\\mirror\\");
	if (!door.load("models\\door1\\10057_wooden_door_v3_iterations-2.obj")) return false;
	door.loadMaterials("models\\door1\\");
	if (!room.load("models\\LivingRoomObj\\room.obj")) return false;
	room.loadMaterials("models\\LivingRoomObj\\");
	if (!walllamp.load("models\\platec-flamingo\\Platec_Flamingo_01.obj")) return false;
	walllamp.loadMaterials("models\\platec-flamingo\\");

	if (!terrain.load("models\\heightmap.bmp", 10)) return false;
	if (!roadMap.load("models\\roadmap.bmp", 10)) return false;

	// create pyramid
	float vermals[] = {
	  -4, 0,-4, 0, 4,-7, 4, 0,-4, 0, 4,-7, 0, 7, 0, 0, 4,-7,
	  -4, 0, 4, 0, 4, 7, 4, 0, 4, 0, 4, 7, 0, 7, 0, 0, 4, 7,
	  -4, 0,-4,-7, 4, 0,-4, 0, 4,-7, 4, 0, 0, 7, 0,-7, 4, 0,
	   4, 0,-4, 7, 4, 0, 4, 0, 4, 7, 4, 0, 0, 7, 0, 7, 4, 0,
	  -4, 0,-4, 0,-1, 0,-4, 0, 4, 0,-1, 0, 4, 0,-4, 0,-1, 0,
	   4, 0, 4, 0,-1, 0,-4, 0, 4, 0,-1, 0, 4, 0,-4, 0,-1, 0 };

	// Generate 1 buffer name
	glGenBuffers(1, &nPyramidBuf);
	// Bind (activate) the buffer
	glBindBuffer(GL_ARRAY_BUFFER, nPyramidBuf);
	// Send data to the buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(vermals), vermals, GL_STATIC_DRAW);

	// Setup Lights - note that diffuse and specular values are moved to onRender
	program.sendUniform("lightEmissive.color", vec3(0));
	program.sendUniform("lightDir.direction", vec3(1.0, 0.5, 1.0));
	program.sendUniform("lightPoint1.position", vec3(-0.95f, 4.24f, 1.0f));
	program.sendUniform("lightPoint2.position", vec3(-4.0f, 6.2f, 11.14f));
	program.sendUniform("lightPoint3.position", vec3(-0.2f, 6.1f, -7.56f));
	program.sendUniform("lightPoint4.position", vec3(-7.13f, 6.2f, -7.56f));

	// Setup materials
	program.sendUniform("materialAmbient", vec3(1.0, 1.0, 1.0));
	program.sendUniform("materialDiffuse", vec3(1.0, 1.0, 1.0));
	program.sendUniform("materialSpecular", vec3(0.0, 0.0, 0.0));
	program.sendUniform("shininess", 10.0);


	// create & load textures
	C3dglBitmap bm;
	glActiveTexture(GL_TEXTURE0);

	// textile texture
	bm.load("models/cloth.png", GL_RGBA);
	glGenTextures(1, &idTextile);
	glBindTexture(GL_TEXTURE_2D, idTextile);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm.getWidth(), bm.getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, bm.getBits());

	// cloth texture
	bm.load("models/cloth.png", GL_RGBA);
	if (!bm.getBits()) return false;
	glGenTextures(1, &idTexCloth);
	glBindTexture(GL_TEXTURE_2D, idTexCloth);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm.getWidth(), bm.getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, bm.getBits());

	// wood texture
	bm.load("models/oak.png", GL_RGBA);
	if (!bm.getBits()) return false;
	glGenTextures(1, &idTexWood);
	glBindTexture(GL_TEXTURE_2D, idTexWood);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm.getWidth(), bm.getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, bm.getBits());

	bm.load("models\\grass.png", GL_RGBA);
	if (!bm.getBits()) return false;
	glGenTextures(1, &idTexGrass);
	glBindTexture(GL_TEXTURE_2D, idTexGrass);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm.getWidth(), bm.getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, bm.getBits());

	bm.load("models\\road.png", GL_RGBA);
	if (!bm.getBits()) return false;
	glGenTextures(1, &idTexRoad);
	glBindTexture(GL_TEXTURE_2D, idTexRoad);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm.getWidth(), bm.getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, bm.getBits());

	bm.load("models/water.bmp", GL_RGBA);
	glGenTextures(1, &idTexParticle);
	glBindTexture(GL_TEXTURE_2D, idTexParticle);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm.getWidth(), bm.getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, bm.getBits());

	// none (simple-white) texture
	glGenTextures(1, &idTexNone);
	glBindTexture(GL_TEXTURE_2D, idTexNone);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	unsigned char bytes[] = { 255, 255, 255 };
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_BGR, GL_UNSIGNED_BYTE, &bytes);

	// Send the texture info to the shaders
	program.sendUniform("texture0", 0);
	programParticle.sendUniform("texture0", 0);

	// Setup the particle system
	programParticle.sendUniform("particleLifetime", LIFETIME);

	// Create shadow map texture
	glActiveTexture(GL_TEXTURE7);
	glGenTextures(1, &idTexShadowMap);
	glBindTexture(GL_TEXTURE_2D, idTexShadowMap);

	// Texture parameters - to get nice filtering & avoid artefact on the edges of the shadowmap
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LESS);

	// This will associate the texture with the depth component in the Z-buffer
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	int w = viewport[2], h = viewport[3];
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, w * 2, h * 2, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);

	// Send the texture info to the shaders
	program.sendUniform("shadowMap", 7);

	// revert to texture unit 0
	glActiveTexture(GL_TEXTURE0);

	// Create a framebuffer object (FBO)
	glGenFramebuffers(1, &idFBO);
	glBindFramebuffer(GL_FRAMEBUFFER_EXT, idFBO);

	// Instruct openGL that we won't bind a color texture with the currently binded FBO
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	// attach the texture to FBO depth attachment point
	glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, idTexShadowMap, 0);

	// switch back to window-system-provided framebuffer
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	// Initialise the View Matrix (initial position of the camera)
	matrixView = rotate(mat4(1), radians(12.f), vec3(1, 0, 0));
	matrixView *= lookAt(
		vec3(9.4f, 8.3f, 12.4f),
		vec3(-9.f, 2.0f, -5.2f),
		vec3(0.0, 1.0, 0.0));

	// setup the screen background colour
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

	cout << endl;
	cout << "Use:" << endl;
	cout << "  WASD or arrow key to navigate" << endl;
	cout << "  QE or PgUp/Dn to move the camera up and down" << endl;
	cout << "  Drag the mouse to look around" << endl;
	cout << endl;

	return true;
}


void initBuffers()
{
	// buffers
	vector<GLfloat> bufPos;
	vector<GLfloat> bufVel;
	vector<float> bufTexCoord;
	vector<float> bufNormal;
	vector<GLuint> bufIndex;

	// fill in positions, velocities, normals, texture coordinates and indices
	float dx = X_CLOTH_SIZE / (X_PARTICLES - 1);
	float dy = Y_CLOTH_SIZE / (Y_PARTICLES - 1);
	float ds = 1.0f / (X_PARTICLES - 1);
	float dt = 1.0f / (Y_PARTICLES - 1);
	float alpha = -90.0f;
	float sin_a = sin((float)M_PI * alpha / 180.0f);
	float cos_a = cos((float)M_PI * alpha / 180.0f);
	for (int i = 0; i < Y_PARTICLES; i++)
	{
		for (int j = 0; j < X_PARTICLES; j++)
		{
			bufPos.push_back((dy * i - Y_CLOTH_SIZE) * sin_a);
			bufPos.push_back((dy * i - Y_CLOTH_SIZE) * cos_a + Y_CLOTH_SIZE);
			if (i == Y_PARTICLES - 1 && (
				j == 0 ||
				j == X_PARTICLES * 1 / 4 ||
				j == X_PARTICLES * 2 / 4 ||
				j == X_PARTICLES * 3 / 4 ||
				j == X_PARTICLES - 1 ||
				false)) {
				bufPos.push_back(-dx * j * 0.9);
			}
			else {
				bufPos.push_back(-dx * j);
			}
			bufPos.push_back(1.0f);

			bufVel.push_back(0);
			bufVel.push_back(0);
			bufVel.push_back(0);
			bufVel.push_back(0);

			bufNormal.push_back(0);
			bufNormal.push_back(-sin_a);
			bufNormal.push_back(cos_a);
			bufNormal.push_back(0);

			bufTexCoord.push_back(ds * j);
			bufTexCoord.push_back(dt * i);

			if (i < Y_PARTICLES - 1)
			{
				bufIndex.push_back((i + 1) * X_PARTICLES + j);
				bufIndex.push_back((i)*X_PARTICLES + j);
			}
		}
		bufIndex.push_back(0xffffff);	// will restart the primitive rendering (triangle stripe)
	}

	// Generate buffers: 2 for positions, 2 for velocities, normal, texture coordinate, indices
	GLuint bufs[7];
	glGenBuffers(7, bufs);
	idBufPos[0] = bufs[0];
	idBufPos[1] = bufs[1];
	idBufVel[0] = bufs[2];
	idBufVel[1] = bufs[3];
	idBufNormal = bufs[4];
	idBufTexCoords = bufs[5];
	idBufIndex = bufs[6];

	if (bCompShaderSupported)
	{
		// The buffers for positions
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, idBufPos[0]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, X_PARTICLES * Y_PARTICLES * 4 * sizeof(GLfloat), &bufPos[0], GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, idBufPos[1]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, X_PARTICLES * Y_PARTICLES * 4 * sizeof(GLfloat), NULL, GL_DYNAMIC_DRAW);

		// Velocities
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, idBufVel[0]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, X_PARTICLES * Y_PARTICLES * 4 * sizeof(GLfloat), &bufVel[0], GL_DYNAMIC_COPY);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, idBufVel[1]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, X_PARTICLES * Y_PARTICLES * 4 * sizeof(GLfloat), NULL, GL_DYNAMIC_COPY);

		// Normal buffer
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, idBufNormal);
		glBufferData(GL_SHADER_STORAGE_BUFFER, bufNormal.size() * sizeof(GLfloat), &bufNormal[0], GL_DYNAMIC_COPY);
	}
	else
	{
		// The buffer for positions (just one)
		glBindBuffer(GL_ARRAY_BUFFER, idBufPos[0]);
		glBufferData(GL_ARRAY_BUFFER, X_PARTICLES * Y_PARTICLES * 4 * sizeof(GLfloat), &bufPos[0], GL_DYNAMIC_DRAW);

		// Normal buffer
		glBindBuffer(GL_ARRAY_BUFFER, idBufNormal);
		glBufferData(GL_ARRAY_BUFFER, bufNormal.size() * sizeof(GLfloat), &bufNormal[0], GL_DYNAMIC_COPY);
	}

	// Texture coordinates buffer
	glBindBuffer(GL_ARRAY_BUFFER, idBufTexCoords);
	glBufferData(GL_ARRAY_BUFFER, bufTexCoord.size() * sizeof(GLfloat), &bufTexCoord[0], GL_STATIC_DRAW);

	// Index buffer
	glBindBuffer(GL_ARRAY_BUFFER, idBufIndex);
	glBufferData(GL_ARRAY_BUFFER, bufIndex.size() * sizeof(GLuint), &bufIndex[0], GL_DYNAMIC_COPY);

	// Set up the VAO
	glGenVertexArrays(1, &idVAOCloat);
	glBindVertexArray(idVAOCloat);

	GLuint idAttrVertex = program.getAttribLocation("aVertex");
	GLuint idAttrNormal = program.getAttribLocation("aNormal");
	GLuint idAttrTexCoord = program.getAttribLocation("aTexCoord");

	glEnableVertexAttribArray(idAttrVertex);
	glEnableVertexAttribArray(idAttrNormal);
	glEnableVertexAttribArray(idAttrTexCoord);

	glBindBuffer(GL_ARRAY_BUFFER, idBufPos[0]);
	glVertexAttribPointer(idAttrVertex, 4, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, idBufNormal);
	glVertexAttribPointer(idAttrNormal, 4, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, idBufTexCoords);
	glVertexAttribPointer(idAttrTexCoord, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idBufIndex);
	glBindVertexArray(0);
}


// called before window opened or resized - to setup the Projection Matrix
void onReshape(int w, int h)
{
	float ratio = w * 1.0f / h;      // we hope that h is not zero
	glViewport(0, 0, w, h);

	mat4 m = perspective(radians(_fov), ratio, 0.02f, 1000.f);
	program.sendUniform("matrixProjection", m);
	programParticle.sendUniform("matrixProjection", m);
}


void ResetAbe(mat4& matrixView)
{
	waypointIndex = 0;
	abeDirection = 90.0f;
	abePosition = vec3(-5.0f, 0.0f, 6.0f);
	abeVel = vec3(0.0f, 0.0f, 0.0f);
	abeAcc = vec3(0.0f, 0.0f, 0.0f);
	stop = false;
	newAnim = -1;
	once = true;
	stopTimer = 0;

	mat4 m = matrixView;
	m = translate(m, abePosition);
	m = rotate(m, radians(abeDirection), vec3(0.f, 1.f, 0.f));
	m = scale(m, vec3(0.035, 0.035, 0.035));
	abe.render(m);

	program.sendUniform("bones", &transforms0[0], transforms0.size());
}


void abeController(mat4& matrixView, float time, float deltaTime)
{
	// Anim Data:
	// 0 = IDLE
	// 1 = WALK
	// 2 = TURN RIGHT
	// 3 = TURN LEFT
	// 4 = TURN AROUND

	mat4 m = matrixView;

	vec3 destination = waypoints[waypointIndex];
	vec3 distanceVec = destination - abePosition;
	float dist = distance(distanceVec[0], distanceVec[2]);

	float currentAccel = abeAccel;
	if (dist < 1.0f && currentAccel > 0)
		stop = true;

	if (stop)
		currentAccel = -abeAccel * 1.2;

	if (dist < 0.1f)
	{
		if (waypointIndex == 3 || waypointIndex == 4) {
			newAnim = 2;
		}
		else if (waypointIndex == 0 || waypointIndex == 1) {
			newAnim = 3;
		}
		else if (waypointIndex == 2 || waypointIndex == 5) {
			newAnim = 4;
		}

		waypointIndex++;
		if (waypointIndex > waypoints.size() - 1)
			waypointIndex = 0;
	}

	float angle = abeDirection / 180 * pi<float>();
	if ((int)abeDirection % 180 == 0)
	{
		abeAcc.z = currentAccel * cos(angle);

		if (cos(angle) == 1)
			abeVel = clamp(abeVel + abeAcc * deltaTime, vec3(0, 0, 0), vec3(0, 0, abeMaxSpeed));
		else
			abeVel = clamp(abeVel + abeAcc * deltaTime, vec3(0, 0, -abeMaxSpeed), vec3(0, 0, 0));
	}
	else if (((int)abeDirection - 90) % 180 == 0)
	{
		abeAcc.x = currentAccel * sin(angle);

		if (sin(angle) == 1)
			abeVel = clamp(abeVel + abeAcc * deltaTime, vec3(0, 0, 0), vec3(abeMaxSpeed, 0, 0));
		else
			abeVel = clamp(abeVel + abeAcc * deltaTime, vec3(-abeMaxSpeed, 0, 0), vec3(0, 0, 0));
	}

	abePosition += abeVel * deltaTime;
	float speed = distance(abeVel[0], abeVel[2]);

	vector<mat4> transforms1;
	vector<mat4> transforms2;

	if (speed == 0) {
		abe.getAnimData(0, time, transforms1);

		if (newAnim != -1)
		{
			if (once) {
				initTime = time;
				animationDuration = abe.getAnimation(newAnim)->getDuration() / abe.getAnimation(newAnim)->getTicksPerSecond();
				once = false;
			}

			abe.getAnimData(newAnim, time - initTime, transforms2);
			vector<mat4> transforms3(transforms1.size());

			float f = clamp(speed / 1.0f, 0.0f, 1.0f);

			for (size_t i = 0; i < transforms1.size(); i++)
			{
				transforms3[i] = (1.0f - f) * transforms1[i] + f * transforms2[i];
			}

			program.sendUniform("bones", &transforms2[0], transforms2.size());

			if (time - initTime >= animationDuration) {
				newAnim = -1;
				once = true;
				stop = false;

				if (waypointIndex == 1 || waypointIndex == 2)
					abeDirection += 90;
				else if (waypointIndex == 0 || waypointIndex == 3)
					abeDirection += 180;
				else if (waypointIndex == 4 || waypointIndex == 5)
					abeDirection -= 90;
			}
		}
		else {
			program.sendUniform("bones", &transforms1[0], transforms1.size());
		}

		stopTimer += deltaTime;
	}
	else if (speed > 1.0f) {
		stopTimer = 0;

		abe.getAnimData(1, time, transforms2);

		program.sendUniform("bones", &transforms2[0], transforms2.size());
	}
	else {
		stopTimer = 0;

		abe.getAnimData(0, time, transforms1);
		abe.getAnimData(1, time, transforms2);
		vector<mat4> transforms3(transforms1.size());

		float f = clamp(speed / 1.0f, 0.0f, 1.0f);

		for (size_t i = 0; i < transforms1.size(); i++)
		{
			transforms3[i] = (1.0f - f) * transforms1[i] + f * transforms2[i];
		}

		program.sendUniform("bones", &transforms3[0], transforms3.size());
	}

	m = translate(m, abePosition);
	m = rotate(m, radians(abeDirection), vec3(0.f, 1.f, 0.f));
	m = scale(m, vec3(0.035, 0.035, 0.035));
	abe.render(m);

	program.sendUniform("bones", &transforms0[0], transforms0.size());

	if (abePosition.x < -9.4f || abePosition.x > 9.7f ||
		abePosition.z < -14.6f || abePosition.z > 12.7f ||
		stopTimer > 10) {
		ResetAbe(matrixView);
	}
}


void renderScene(mat4& matrixView, float time, float deltaTime)
{
	mat4 m;

	// setup lights
	program.sendUniform("lightAmbient.color", fAmbient * vec3(0.02, 0.02, 0.02));
	program.sendUniform("lightPoint1.diffuse", fPoint1 * vec3(0.1, 0.1, 0.1));
	program.sendUniform("lightPoint1.specular", fPoint1 * vec3(0.5, 0.5, 0.5));
	program.sendUniform("lightPoint2.diffuse", fPoint2 * vec3(0.3, 0.3, 0.3));
	program.sendUniform("lightPoint2.specular", fPoint2 * vec3(0.5, 0.5, 0.5));
	program.sendUniform("lightPoint3.diffuse", fPoint3 * vec3(0.3, 0.3, 0.3));
	program.sendUniform("lightPoint3.specular", fPoint3 * vec3(0.5, 0.5, 0.5));
	program.sendUniform("lightPoint4.diffuse", fPoint4 * vec3(0.3, 0.3, 0.3));
	program.sendUniform("lightPoint4.specular", fPoint4 * vec3(0.5, 0.5, 0.5));

	// setup materials
	program.sendUniform("materialSpecular", vec3(0.0, 0.0, 0.0));
	program.sendUniform("materialDiffuse", vec3(1.0, 1.0, 1.0));
	program.sendUniform("materialAmbient", vec3(1.0, 1.0, 1.0));

	program.sendUniform("lightDir.diffuse", fDir * vec3(0.1, 0.1, 0.1));

	// render the terrain
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, idTexGrass);
	m = matrixView;
	m = translate(m, vec3(0, -154.5, 0));
	m = rotate(m, radians(90.f), vec3(0.f, 1.f, 0.f));
	m = scale(m, vec3(50.0f, 50.0f, 50.0f));
	terrain.render(m);

	// render the road
	glBindTexture(GL_TEXTURE_2D, idTexRoad);
	m = matrixView;
	m = translate(m, vec3(0, -154.5, 0));
	m = rotate(m, radians(90.f), vec3(0.f, 1.f, 0.f));
	m = scale(m, vec3(50.0f, 50.0f, 50.0f));
	m = translate(m, vec3(6.0f, 0.01f, 0.0f));
	roadMap.render(m);

	program.sendUniform("lightDir.diffuse", fDir * vec3(0.2, 0.2, 0.2));	  // dimmed white light

	// animated character
	abeController(matrixView, time, deltaTime);

	// room
	m = matrixView;
	m = scale(m, vec3(0.03f, 0.03f, 0.03f));
	room.render(m);

	// door
	m = matrixView;
	m = translate(m, vec3(4.8, 0, 12.5));
	m = rotate(m, radians(-90.f), vec3(1.f, 0.f, 0.f));
	m = rotate(m, radians(180.f), vec3(0.f, 0.f, 1.f));
	m = scale(m, vec3(0.035f, 0.035f, 0.035f));
	door.render(m);

	// window frame 1
	m = matrixView;
	m = translate(m, vec3(-10.4, 1.05, 8.08));
	m = rotate(m, radians(-90.f), vec3(0.f, 1.f, 0.f));
	m = scale(m, vec3(0.02f, 0.02f, 0.02f));
	window.render(m);

	// window frame 2
	m = matrixView;
	m = translate(m, vec3(-10.4, 1.05, 13.71));
	m = rotate(m, radians(-90.f), vec3(0.f, 1.f, 0.f));
	m = scale(m, vec3(0.02f, 0.02f, 0.02f));
	window.render(m);

	// window background 1
	m = matrixView;
	m = translate(m, vec3(-9.85, 1.4, 0.6));
	m = rotate(m, radians(-90.f), vec3(0.f, 1.f, 0.f));
	m = rotate(m, radians(-10.8f), vec3(1.f, 0.f, 0.f));
	m = scale(m, vec3(0.005f, 0.0037f, 0.004f));
	mirror.render(3, m);

	// window background 2
	m = matrixView;
	m = translate(m, vec3(-9.85, 1.4, 6.23));
	m = rotate(m, radians(-90.f), vec3(0.f, 1.f, 0.f));
	m = rotate(m, radians(-10.8f), vec3(1.f, 0.f, 0.f));
	m = scale(m, vec3(0.005f, 0.0037f, 0.004f));
	mirror.render(3, m);

	// mirror - stand
	m = matrixView;
	m = translate(m, vec3(-6.5, 0.2, 10));
	m = rotate(m, radians(angleFrame), vec3(0.f, 1.f, 0.f));
	m = scale(m, vec3(0.004f, 0.004f, 0.004f));
	mirror.render(1, m);

	// mirror - swivel
	m = translate(m, vec3(0, 3.67f / 0.004f, 0));
	m = rotate(m, radians(angleMirror - 10), vec3(1, 0, 0));
	m = translate(m, vec3(0, -3.67f / 0.004f, 0));
	mirror.render(2, m);

	// table & chairs
	glBindTexture(GL_TEXTURE_2D, idTexWood);
	m = matrixView;
	m = translate(m, vec3(-2, 0, 0));
	m = scale(m, vec3(0.004f, 0.004f, 0.004f));
	table.render(1, m);
	glBindTexture(GL_TEXTURE_2D, idTexCloth);
	table.render(0, m);
	m = rotate(m, radians(180.f), vec3(0.f, 1.f, 0.f));
	table.render(0, m);
	m = translate(m, vec3(250, 0, 0));
	m = rotate(m, radians(90.f), vec3(0.f, 1.f, 0.f));
	table.render(0, m);
	m = translate(m, vec3(0, 0, -500));
	m = rotate(m, radians(180.f), vec3(0.f, 1.f, 0.f));
	table.render(0, m);

	// vase
	program.sendUniform("materialDiffuse", vec3(0.2, 0.4, 0.8));
	program.sendUniform("materialAmbient", vec3(0.2, 0.4, 0.8));
	program.sendUniform("materialSpecular", vec3(1.0, 1.0, 1.0));
	glBindTexture(GL_TEXTURE_2D, idTexNone);
	m = matrixView;
	m = translate(m, vec3(-2.f, 3.f, 0.f));
	m = scale(m, vec3(0.12f, 0.12f, 0.12f));
	vase.render(m);

	// teapot
	program.sendUniform("materialDiffuse", vec3(0.1, 0.8, 0.3));
	program.sendUniform("materialAmbient", vec3(0.1, 0.8, 0.3));
	program.sendUniform("materialSpecular", vec3(1.0, 1.0, 1.0));
	m = matrixView;
	m = translate(m, vec3(-0.2f, 3.4f, 0.0f));
	program.sendUniform("matrixModelView", m);
	glutSolidTeapot(0.5);

	// pyramid
	program.sendUniform("materialDiffuse", vec3(1.0, 0.2, 0.2));
	program.sendUniform("materialAmbient", vec3(1.0, 0.2, 0.2));
	program.sendUniform("materialSpecular", vec3(0.0, 0.0, 0.0));
	m = matrixView;
	m = translate(m, vec3(-3.5f, 3.7f, 0.5f));
	m = rotate(m, radians(180.f), vec3(1, 0, 0));
	m = rotate(m, radians(-40 * time), vec3(0, 1, 0));
	m = scale(m, vec3(0.1f, 0.1f, 0.1f));
	program.sendUniform("matrixModelView", m);

	GLuint attribVertex = program.getAttribLocation("aVertex");
	GLuint attribNormal = program.getAttribLocation("aNormal");
	glBindBuffer(GL_ARRAY_BUFFER, nPyramidBuf);
	glEnableVertexAttribArray(attribVertex);
	glEnableVertexAttribArray(attribNormal);
	glVertexAttribPointer(attribVertex, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 0);
	glVertexAttribPointer(attribNormal, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glDrawArrays(GL_TRIANGLES, 0, 18);
	glDisableVertexAttribArray(GL_VERTEX_ARRAY);
	glDisableVertexAttribArray(GL_NORMAL_ARRAY);

	// chicken
	program.sendUniform("materialDiffuse", vec3(0.8, 0.8, 0.2));
	program.sendUniform("materialAmbient", vec3(0.8, 0.8, 0.2));
	program.sendUniform("materialSpecular", vec3(0.6, 0.6, 1.0));
	m = translate(m, vec3(-2, -5, 0));
	m = scale(m, vec3(0.2f, 0.2f, 0.2f));
	m = rotate(m, radians(180.f), vec3(1, 0, 0));
	chicken.render(m);

	// lamp 1 (desk)
	m = matrixView;
	m = translate(m, vec3(-0.2f, 3.075f, 1.0f));
	m = scale(m, vec3(0.02f, 0.02f, 0.02f));
	lamp.render(m);

	// lamp 2 (wall)
	m = matrixView;
	m = translate(m, vec3(-4.0f, 6.9f, 12.6f));
	m = rotate(m, radians(90.f), vec3(0.f, 1.f, 0.f));
	m = scale(m, vec3(0.005f, 0.005f, 0.005f));
	walllamp.render(m);

	// lamp 3 (outside left)
	m = matrixView;
	m = translate(m, vec3(-0.2f, 6.9f, -6.1f));
	m = rotate(m, radians(90.f), vec3(0.f, 1.f, 0.f));
	m = scale(m, vec3(0.005f, 0.005f, 0.005f));
	walllamp.render(m);

	// lamp 4 (outside right)
	m = matrixView;
	m = translate(m, vec3(-7.13f, 6.9f, -6.1f));
	m = rotate(m, radians(90.f), vec3(0.f, 1.f, 0.f));
	m = scale(m, vec3(0.005f, 0.005f, 0.005f));
	walllamp.render(m);

	// light bulb 1
	program.sendUniform("materialDiffuse", vec3(0.8, 0.8, 0.8));
	program.sendUniform("materialAmbient", vec3(1.0, 1.0, 1.0));
	program.sendUniform("lightEmissive.color", vec3(fPoint1));
	m = matrixView;
	m = translate(m, vec3(-0.95f, 4.24f, 1.0f));
	m = scale(m, vec3(0.1f, 0.1f, 0.1f));
	program.sendUniform("matrixModelView", m);
	glutSolidSphere(1, 32, 32);

	// light bulb 2
	program.sendUniform("materialDiffuse", vec3(0.8, 0.8, 0.8));
	program.sendUniform("materialAmbient", vec3(1.0, 1.0, 1.0));
	program.sendUniform("lightEmissive.color", vec3(fPoint2));
	m = matrixView;
	m = translate(m, vec3(-4.0f, 6.2f, 11.14f));
	m = scale(m, vec3(0.2f, 0.2f, 0.2f));
	program.sendUniform("matrixModelView", m);
	glutSolidSphere(1, 32, 32);

	// light bulb 3
	program.sendUniform("materialDiffuse", vec3(0.8, 0.8, 0.8));
	program.sendUniform("materialAmbient", vec3(1.0, 1.0, 1.0));
	program.sendUniform("lightEmissive.color", vec3(fPoint3));
	m = matrixView;
	m = translate(m, vec3(-0.2f, 6.2f, -7.56f));
	m = scale(m, vec3(0.2f, 0.2f, 0.2f));
	program.sendUniform("matrixModelView", m);
	glutSolidSphere(1, 32, 32);

	// light bulb 4
	program.sendUniform("materialDiffuse", vec3(0.8, 0.8, 0.8));
	program.sendUniform("materialAmbient", vec3(1.0, 1.0, 1.0));
	program.sendUniform("lightEmissive.color", vec3(fPoint4));
	m = matrixView;
	m = translate(m, vec3(-7.13f, 6.2f, -7.56f));
	m = scale(m, vec3(0.2f, 0.2f, 0.2f));
	program.sendUniform("matrixModelView", m);
	glutSolidSphere(1, 32, 32);

	program.sendUniform("lightEmissive.color", vec3(0));
	program.sendUniform("materialSpecular", vec3(0.0, 0.0, 0.0));

	programParticle.use();

	m = matrixView;
	programParticle.sendUniform("matrixModelView", m);

	// particles
	glDepthMask(GL_FALSE); // disable depth buffer updates
	glActiveTexture(GL_TEXTURE0); // choose the active texture
	glBindTexture(GL_TEXTURE_2D, idTexParticle); // bind the texture

	// render the buffer
	GLint aStartTime = programParticle.getAttribLocation("aStartTime");
	GLint aPos = programParticle.getAttribLocation("aPos");

	glEnableVertexAttribArray(aPos); // velocity
	glEnableVertexAttribArray(aStartTime); // start time
	glBindBuffer(GL_ARRAY_BUFFER, idBufferPos);
	glVertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, idBufferStartTime);
	glVertexAttribPointer(aStartTime, 1, GL_FLOAT, GL_FALSE, 0, 0);
	glDrawArrays(GL_POINTS, 0, NPARTICLES);
	glDisableVertexAttribArray(aPos);
	glDisableVertexAttribArray(aStartTime);
	glDepthMask(GL_TRUE);

	// setup the point size
	glEnable(GL_POINT_SPRITE);
	glPointSize(3);

	program.use();
}


void renderMirror(mat4& matrixView, float time, float deltaTime)
{
	// Draw only the mirror surface
	mat4 m = matrixView;
	m = translate(m, vec3(-6.5, 0.2, 10));
	m = rotate(m, radians(angleFrame), vec3(0, 1, 0));
	m = translate(m, vec3(0, 3.67f, 0));
	m = rotate(m, radians(angleMirror - 10), vec3(1, 0, 0));
	m = translate(m, vec3(0, -3.67f, 0));
	m = scale(m, vec3(0.004f, 0.004f, 0.004f));
	mirror.render(3, m);
}


// Creates a shadow map and stores in idFBO
// lightTransform - lookAt transform corresponding to the light position predominant direction
void createShadowMap(mat4 lightTransform, float time, float deltaTime)
{
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	// Store the current viewport in a safe place
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	int w = viewport[2], h = viewport[3];

	// setup the viewport to 2x2 the original and wide (120 degrees) FoV (Field of View)
	glViewport(0, 0, w * 2, h * 2);
	mat4 matrixProjection = perspective(radians(160.f), (float)w / (float)h, 0.5f, 50.0f);
	program.sendUniform("matrixProjection", matrixProjection);

	// prepare the camera
	mat4 matrixView = lightTransform;

	// send the View Matrix
	program.sendUniform("matrixView", matrixView);

	// Bind the Framebuffer
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, idFBO);

	// OFF-SCREEN RENDERING FROM NOW!
	// Clear previous frame values - depth buffer only!
	glClear(GL_DEPTH_BUFFER_BIT);

	// Disable color rendering, we only want to write to the Z-Buffer (this is to speed-up)
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	// Prepare and send the Shadow Matrix - this is matrix transform every coordinate x,y,z
	// x = x* 0.5 + 0.5 
	// y = y* 0.5 + 0.5 
	// z = z* 0.5 + 0.5 
	// Moving from unit cube [-1,1] to [0,1] 
	const mat4 bias = {
	{ 0.5, 0.0, 0.0, 0.0 },
	{ 0.0, 0.5, 0.0, 0.0 },
	{ 0.0, 0.0, 0.5, 0.0 },
	{ 0.5, 0.5, 0.5, 1.0 }
	};
	program.sendUniform("matrixShadow", bias * matrixProjection * matrixView);

	// Render all objects in the scene
	renderScene(matrixView, time, deltaTime);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDisable(GL_CULL_FACE);
	onReshape(w, h);
}


void onRender()
{
	// these variables control time & animation
	static float prev = 0;
	float time = glutGet(GLUT_ELAPSED_TIME) * 0.001f;	// time since start in seconds
	float deltaTime = time - prev;						// time since last frame
	prev = time;										// framerate is 1/deltaTime
	//cout << 1 / deltaTime << endl;

	programParticle.sendUniform("time", time);

	createShadowMap(lookAt(
		lightSource, // coordinates of the source of the light
		vec3(0.0f, 3.0f, 0.0f), // coordinates of a point within or behind the scene
		vec3(0.0f, 1.0f, 0.0f)), // a reasonable "Up" vector
		time, deltaTime);

	// Compute Cloth
	if (bCompShaderSupported)
	{
		programComp.use();

		if (directionTimer > 12 * pi<float>()) {
			directionTimer = 0;
		}
		directionChangeX = 10 * 25 * (sin(directionTimer * 5 / 2) + sin(directionTimer * 5 / 3)) / 953;
		directionChangeY = 0.1 + 150 * (sin(directionTimer * 5 / 2) + sin(directionTimer * 5 / 3)) / 953;

		windForce = 1.25 - 150 * (sin(directionTimer * 2 / 2) + sin(directionTimer * 2 / 3)) / 953;

		directionTimer += 0.01;

		programComp.sendUniform("directionChangeX", directionChangeX);
		programComp.sendUniform("directionChangeY", directionChangeY);
		programComp.sendUniform("windForce", windForce);

		const int NUM = 1000000 / 60 / 6;
		for (int i = 0; i < NUM; i++)
		{
			glDispatchCompute(X_PARTICLES / 10, Y_PARTICLES / 10, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

			// Swap buffers
			static GLuint readBuf = 0;
			readBuf = 1 - readBuf;
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, idBufPos[readBuf]);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, idBufPos[1 - readBuf]);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, idBufVel[readBuf]);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, idBufVel[1 - readBuf]);
		}

		// Compute the normals
		programNorm.use();
		glDispatchCompute(X_PARTICLES / 10, Y_PARTICLES / 10, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}

	// clear screen and buffers
	program.use();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// Prepare the stencil test
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_ALWAYS, 1, 1);
	glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);

	// Disable screen rendering
	glDisable(GL_DEPTH_TEST);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	// Create mirror stencil
	renderMirror(matrixView, time, deltaTime);

	// Use stencil test
	glStencilFunc(GL_EQUAL, 1, 1);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	// Enable screen rendering
	glEnable(GL_DEPTH_TEST);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	glEnable(GL_CLIP_PLANE0);

	// Find the reflection surface (point and normal)
	float ry = radians(angleFrame);
	float rx = -radians(angleMirror);
	vec3 p(-6.5f, 3.86f, 10.0f);
	vec3 n(sin(ry) * cos(rx), sin(rx), cos(ry) * cos(rx));

	// reflection matrix
	float a = n.x, b = n.y, c = n.z, d = -dot(p, n);
	program.sendUniform("planeClip", vec4(a, b, c, d));

	// parameters of the reflection plane: Ax + By + Cz + d = 0
	matrixReflection = mat4(
		1 - 2 * a * a, -2 * a * b, -2 * a * c, 0,
		-2 * a * b, 1 - 2 * b * b, -2 * b * c, 0,
		-2 * a * c, -2 * b * c, 1 - 2 * c * c, 0,
		-2 * a * d, -2 * b * d, -2 * c * d, 1);

	// Render the scene with the reflected camera
	matrixView *= matrixReflection;
	program.sendUniform("matrixView", matrixView);
	programParticle.sendUniform("reflectedCam", true);

	renderScene(matrixView, time, deltaTime);

	// setup the textile texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, idTextile);

	// setup mirrored cloth 1
	mat4 m;
	m = matrixView;
	m = translate(m, vec3(-8.85f, 2.25f, 1.8f));
	m = scale(m, vec3(0.2f, 0.2f, 0.2f));
	program.sendUniform("matrixModelView", m);

	// render mirrored cloth 1
	glBindVertexArray(idVAOCloat);
	glDrawElements(GL_TRIANGLE_STRIP, Y_PARTICLES * (X_PARTICLES * 2 + 1), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	// setup mirrored cloth 2
	m = matrixView;
	m = translate(m, vec3(-8.85f, 2.25f, 7.42f));
	m = scale(m, vec3(0.2f, 0.2f, 0.2f));
	program.sendUniform("matrixModelView", m);

	// render mirrored cloth 2
	glBindVertexArray(idVAOCloat);
	glDrawElements(GL_TRIANGLE_STRIP, Y_PARTICLES * (X_PARTICLES * 2 + 1), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	// Revert to the regular camera
	matrixView *= matrixReflection;
	program.sendUniform("matrixView", matrixView);

	// disable stencil test and clip plane
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_CLIP_PLANE0);

	mat4 tempMatrix = matrixView;

	// setup the View Matrix (camera)
	_vel = clamp(_vel + _acc * deltaTime, -vec3(maxspeed), vec3(maxspeed));
	float pitch = getPitch(matrixView);
	matrixView = rotate(translate(rotate(mat4(1),
		pitch, vec3(1, 0, 0)),	// switch the pitch off
		_vel * deltaTime),		// animate camera motion (controlled by WASD keys)
		-pitch, vec3(1, 0, 0))	// switch the pitch on
		* matrixView;

	// test if player camera is within bounds
	vec3 tempPos = getPos(matrixView);
	if ((tempPos.x < -9.2f || tempPos.x > 9.5f) ||
		(tempPos.y > 8.9f || tempPos.y < 0.0f) ||
		(tempPos.z < -13.7f || tempPos.z > 12.5f)) {
		matrixView = tempMatrix;
	}

	// setup View Matrix
	program.sendUniform("matrixView", matrixView);

	// setup Model-View Matrix
	program.sendUniform("matrixModelView", matrixView);

	// render the mirror with blocked color output
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	renderMirror(matrixView, time, deltaTime);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	programParticle.sendUniform("reflectedCam", false);

	// render the scene objects
	renderScene(matrixView, time, deltaTime);

	// setup the textile texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, idTextile);

	// setup cloth 1
	m = matrixView;
	m = translate(m, vec3(-8.85f, 2.25f, 1.8f));
	m = scale(m, vec3(0.2f, 0.2f, 0.2f));
	program.sendUniform("matrixModelView", m);

	// render cloth 1
	glBindVertexArray(idVAOCloat);
	glDrawElements(GL_TRIANGLE_STRIP, Y_PARTICLES * (X_PARTICLES * 2 + 1), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	// setup cloth 2
	m = matrixView;
	m = translate(m, vec3(-8.85f, 2.25f, 7.42f));
	m = scale(m, vec3(0.2f, 0.2f, 0.2f));
	program.sendUniform("matrixModelView", m);

	// render cloth 2
	glBindVertexArray(idVAOCloat);
	glDrawElements(GL_TRIANGLE_STRIP, Y_PARTICLES * (X_PARTICLES * 2 + 1), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	// essential for double-buffering technique
	glutSwapBuffers();

	// proceed the animation
	glutPostRedisplay();
}

// Handle WASDQE keys
void onKeyDown(unsigned char key, int x, int y)
{
	switch (tolower(key))
	{
	case 'w': _acc.z = accel; break;
	case 's': _acc.z = -accel; break;
	case 'a': _acc.x = accel; break;
	case 'd': _acc.x = -accel; break;
	case 'e': _acc.y = accel; break;
	case 'q': _acc.y = -accel; break;
	case '1': fPoint1 = 1 - fPoint1; break;
	case '2': fPoint2 = 1 - fPoint2; break;
	case '3': fPoint3 = 1 - fPoint3; break;
	case '4': fPoint4 = 1 - fPoint4; break;
	case '9': fDir = 1 - fDir; break;
	case '0': fAmbient = 1 - fAmbient; break;
	case 't': if (angleMirror < 20.0f) angleMirror += 0.2; break; // up
	case 'g': if (angleMirror > -20.0f) angleMirror -= 0.2; break; // down
	case 'f': if (angleFrame > 90.0f) angleFrame -= 0.2; break; // left
	case 'h': if (angleFrame < 200.0f) angleFrame += 0.2; break; // right
	}
}

// Handle WASDQE keys (key up)
void onKeyUp(unsigned char key, int x, int y)
{
	switch (tolower(key))
	{
	case 'w':
	case 's': _acc.z = _vel.z = 0; break;
	case 'a':
	case 'd': _acc.x = _vel.x = 0; break;
	case 'q':
	case 'e': _acc.y = _vel.y = 0; break;
	}
}

// Handle arrow keys and Alt+F4
void onSpecDown(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_F4:		if ((glutGetModifiers() & GLUT_ACTIVE_ALT) != 0) exit(0); break;
	case GLUT_KEY_UP:		onKeyDown('w', x, y); break;
	case GLUT_KEY_DOWN:		onKeyDown('s', x, y); break;
	case GLUT_KEY_LEFT:		onKeyDown('a', x, y); break;
	case GLUT_KEY_RIGHT:	onKeyDown('d', x, y); break;
	case GLUT_KEY_PAGE_UP:	onKeyDown('q', x, y); break;
	case GLUT_KEY_PAGE_DOWN:onKeyDown('e', x, y); break;
	case GLUT_KEY_F11:		glutFullScreenToggle();
	}
}

// Handle arrow keys (key up)
void onSpecUp(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_UP:		onKeyUp('w', x, y); break;
	case GLUT_KEY_DOWN:		onKeyUp('s', x, y); break;
	case GLUT_KEY_LEFT:		onKeyUp('a', x, y); break;
	case GLUT_KEY_RIGHT:	onKeyUp('d', x, y); break;
	case GLUT_KEY_PAGE_UP:	onKeyUp('q', x, y); break;
	case GLUT_KEY_PAGE_DOWN:onKeyUp('e', x, y); break;
	}
}

// Handle mouse click
void onMouse(int button, int state, int x, int y)
{
	glutSetCursor(state == GLUT_DOWN ? GLUT_CURSOR_CROSSHAIR : GLUT_CURSOR_INHERIT);
	glutWarpPointer(glutGet(GLUT_WINDOW_WIDTH) / 2, glutGet(GLUT_WINDOW_HEIGHT) / 2);
	if (button == 1)
	{
		_fov = 60.0f;
		onReshape(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
	}
}

// handle mouse move
void onMotion(int x, int y)
{
	glutWarpPointer(glutGet(GLUT_WINDOW_WIDTH) / 2, glutGet(GLUT_WINDOW_HEIGHT) / 2);

	// find delta (change to) pan & pitch
	float deltaYaw = 0.005f * (x - glutGet(GLUT_WINDOW_WIDTH) / 2);
	float deltaPitch = 0.005f * (y - glutGet(GLUT_WINDOW_HEIGHT) / 2);

	if (abs(deltaYaw) > 0.3f || abs(deltaPitch) > 0.3f)
		return;	// avoid warping side-effects

	// View = Pitch * DeltaPitch * DeltaYaw * Pitch^-1 * View;
	constexpr float maxPitch = radians(80.f);
	float pitch = getPitch(matrixView);
	float newPitch = clamp(pitch + deltaPitch, -maxPitch, maxPitch);
	matrixView = rotate(rotate(rotate(mat4(1.f),
		newPitch, vec3(1.f, 0.f, 0.f)),
		deltaYaw, vec3(0.f, 1.f, 0.f)),
		-pitch, vec3(1.f, 0.f, 0.f))
		* matrixView;
}

void onMouseWheel(int button, int dir, int x, int y)
{
	_fov = clamp(_fov - dir * 5.f, 5.0f, 175.f);
	onReshape(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
}

int main(int argc, char** argv)
{
	// init GLUT and create Window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(1280, 720);
	glutCreateWindow("3DGL Scene: Hero Demo");

	// init glew
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		C3dglLogger::log("GLEW Error {}", (const char*)glewGetErrorString(err));
		return 0;
	}
	C3dglLogger::log("Using GLEW {}", (const char*)glewGetString(GLEW_VERSION));

	// register callbacks
	glutDisplayFunc(onRender);
	glutReshapeFunc(onReshape);
	glutKeyboardFunc(onKeyDown);
	glutSpecialFunc(onSpecDown);
	glutKeyboardUpFunc(onKeyUp);
	glutSpecialUpFunc(onSpecUp);
	glutMouseFunc(onMouse);
	glutMotionFunc(onMotion);
	glutMouseWheelFunc(onMouseWheel);

	C3dglLogger::log("Vendor: {}", (const char*)glGetString(GL_VENDOR));
	C3dglLogger::log("Renderer: {}", (const char*)glGetString(GL_RENDERER));
	C3dglLogger::log("Version: {}", (const char*)glGetString(GL_VERSION));
	C3dglLogger::log("");

	// init light and everything – not a GLUT or callback function!
	if (!init())
	{
		C3dglLogger::log("Application failed to initialise\r\n");
		return 0;
	}

	// enter GLUT event processing cycle
	glutMainLoop();

	return 1;
}
