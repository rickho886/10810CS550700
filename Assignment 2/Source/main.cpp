#include "../Externals/Include/Common.h"

#define MAX_T 2
#define MAX_NP 3

GLubyte timer_cnt = 0;
bool timer_enabled = true;
unsigned int timer_speed = 16;

using namespace glm;
using namespace std;

GLuint program, program2, texture_location;
GLuint window_vao;
GLuint window_buffer;
GLint um4mv, um4p;
GLuint FBO;
GLuint mode_location;
GLuint FBODataTexture;
GLuint depthRBO;
GLuint noiseTexture;


struct Shape {
	GLuint vao;
	GLuint vbo_position;
	GLuint vbo_normal;
	GLuint vbo_texcoord;
	GLuint ibo;
	int drawCount;
	int materialID;
};

struct Material {
	GLuint diffuse_tex;
};

vector<Material> v_material;
vector<Shape> v_shape;

vec3 eye = vec3(-20.0f, 50.0f, 0.0f);
vec3 center = vec3(30.0f, 50.0f, 0.0f);;
vec3 up = vec3(0.0f, 50.0f, 0.0f);
vec3 direction;
mat4 mv, projection, view, model;

float viewportAspect;
float ya = -90.0f;
float pit = 0.0f;
float lastX = 600.0 / 2.0;
float lastY = 600.0 / 2.0;
bool firstMouse = true;

static const GLfloat window_vertex[] =
{
	1.0f,-1.0f,1.0f,0.0f,
	-1.0f,-1.0f,0.0f,0.0f,
	-1.0f,1.0f,0.0f,1.0f,
	1.0f,1.0f,1.0f,1.0f
};

int mode = 6;
int W;
int H;

char** loadShaderSource(const char* file)
{
	FILE* fp = fopen(file, "rb");
	fseek(fp, 0, SEEK_END);
	long sz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char* src = new char[sz + 1];
	fread(src, sizeof(char), sz, fp);
	src[sz] = '\0';
	char** srcp = new char* [1];
	srcp[0] = src;
	return srcp;
}

void freeShaderSource(char** srcp)
{
	delete[] srcp[0];
	delete[] srcp;
}

void loadModel()
{
	const aiScene* scene = aiImportFile("sponza.obj", aiProcessPreset_TargetRealtime_MaxQuality);
	unsigned int i = 0;
	while (i < scene->mNumMaterials) {
		aiMaterial* material = scene->mMaterials[i];
		Material Material;
		aiString texturePath;
		if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == aiReturn_SUCCESS) {
			texture_data tex = loadImg(texturePath.C_Str());
			int tex_width = tex.width;
			int tex_height = tex.height;
			glGenTextures(1, &Material.diffuse_tex);
			glBindTexture(GL_TEXTURE_2D, Material.diffuse_tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tex_width, tex_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex.data);
			glGenerateMipmap(GL_TEXTURE_2D);
		}

		v_material.push_back(Material);
		i++;
	}

	i = 0;
	while(i < scene->mNumMeshes) {
		aiMesh* mesh = scene->mMeshes[i];
		Shape shape;
		glGenVertexArrays(1, &shape.vao);
		glBindVertexArray(shape.vao);
		glGenBuffers(1, &shape.vbo_position);
		glGenBuffers(1, &shape.vbo_normal);
		glGenBuffers(1, &shape.vbo_texcoord);
		glGenBuffers(1, &shape.ibo);
		float* normal = new float[mesh->mNumVertices * MAX_NP];
		float* position = new float[mesh->mNumVertices * MAX_NP];
		float* texcoord = new float[mesh->mNumVertices * MAX_T];
		int x = 2, y = 1;
		for (unsigned int v = 0; v < mesh->mNumVertices; ++v, y += 2, x += 3)
		{
			position[x - 2] = mesh->mVertices[v][0];
			position[x - 1] = mesh->mVertices[v][1];
			position[x] = mesh->mVertices[v][2];
			normal[x - 2] = mesh->mNormals[v][0];
			normal[x - 1] = mesh->mNormals[v][1];
			normal[x] = mesh->mNormals[v][2];
			texcoord[y - 1] = mesh->mTextureCoords[0][v][0];
			texcoord[y] = mesh->mTextureCoords[0][v][1];
		}

		glBindBuffer(GL_ARRAY_BUFFER, shape.vbo_position);
		glBufferData(GL_ARRAY_BUFFER, mesh->mNumVertices * sizeof(float) * 3, position, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);


		glBindBuffer(GL_ARRAY_BUFFER, shape.vbo_normal);
		glBufferData(GL_ARRAY_BUFFER, mesh->mNumVertices * sizeof(float) * 3, normal, GL_STATIC_DRAW);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);

		glBindBuffer(GL_ARRAY_BUFFER, shape.vbo_texcoord);
		glBufferData(GL_ARRAY_BUFFER, mesh->mNumVertices * sizeof(float) * 2, texcoord, GL_STATIC_DRAW);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		unsigned int* index = new unsigned int[mesh->mNumFaces * sizeof(unsigned int) * 3];
		int z = 2;
		for (unsigned int f = 0; f < mesh->mNumFaces; ++f, z += 3) {
			index[z] = mesh->mFaces[f].mIndices[2];
			index[z - 1] = mesh->mFaces[f].mIndices[1];
			index[z - 2] = mesh->mFaces[f].mIndices[0];
		}
		glBindBuffer(GL_ARRAY_BUFFER, shape.ibo);
		glBufferData(GL_ARRAY_BUFFER, mesh->mNumFaces * sizeof(unsigned int) * 3, index, GL_STATIC_DRAW);


		shape.materialID = mesh->mMaterialIndex;
		shape.drawCount = mesh->mNumFaces * 3;

		v_shape.push_back(shape);
		i++;
	}

	aiReleaseImport(scene);
}

void My_Init()
{
	glClearColor(0.0f, 0.6f, 0.0f, 1.0f);
	printf("my_init\n");
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	// Create Shader Program
	program = glCreateProgram();
	printf("create program1\n");

	// Create customize shader by tell openGL specify shader type
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	printf("Succesfully create fragment shader\n");

	// Load shader file
	char** vertexShaderSource = loadShaderSource("vertex.vs.glsl");
	glShaderSource(vertexShader, 1, vertexShaderSource, NULL);
	freeShaderSource(vertexShaderSource);
	glCompileShader(vertexShader);
	shaderLog(vertexShader);
	glAttachShader(program, vertexShader);
	printf("Succesfully load vertex.vs.glsl\n");

	char** fragmentShaderSource = loadShaderSource("fragment.fs.glsl");
	glShaderSource(fragmentShader, 1, fragmentShaderSource, NULL);
	freeShaderSource(fragmentShaderSource);
	glCompileShader(fragmentShader);
	shaderLog(fragmentShader);
	glAttachShader(program, fragmentShader);
	printf("Succesfully load fragment.vs.glsl\n");

	glLinkProgram(program);

	texture_location = glGetUniformLocation(program, "tex");
	um4mv = glGetUniformLocation(program, "um4mv");
	um4p = glGetUniformLocation(program, "um4p");

	glUseProgram(program);

	loadModel();

	program2 = glCreateProgram();
	printf("create program2\n");

	GLuint vs2 = glCreateShader(GL_VERTEX_SHADER);
	GLuint fs2 = glCreateShader(GL_FRAGMENT_SHADER);

	char** vs2_source = loadShaderSource("post_effect.vs.glsl");
	char** fs2_source = loadShaderSource("post_effect.fs.glsl");
	printf("Succesfully load shader2\n");

	glShaderSource(vs2, 1, vs2_source, NULL);
	glShaderSource(fs2, 1, fs2_source, NULL);
	printf("Succesfully load shader2\n");
	glCompileShader(fs2);
	glCompileShader(vs2);
	printf("compile shader2\n");
	shaderLog(vs2);
	shaderLog(fs2);

	glAttachShader(program2, vs2);
	glAttachShader(program2, fs2);
	printf("attach shader2\n");
	glLinkProgram(program2);
	//printf("link program shader2\n");

	glGenVertexArrays(1, &window_vao);
	glBindVertexArray(window_vao);

	glGenBuffers(1, &window_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, window_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(window_vertex), window_vertex, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * 4, 0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * 4, (const GLvoid*)(sizeof(GL_FLOAT) * 2));

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glGenFramebuffers(1, &FBO);

	glutWarpPointer(300, 300);

	mode_location = glGetUniformLocation(program2, "mode");

	texture_data noise_tex = loadImg("noise_map.png");
	int noise_width = noise_tex.width;
	int noise_height = noise_tex.height;
	glGenTextures(1, &noiseTexture);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, noise_width, noise_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, noise_tex.data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void My_Display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FBO);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	glUseProgram(program);

	static const GLfloat white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	static const GLfloat one = 1.0f;

	glClearBufferfv(GL_COLOR, 0, white);
	glClearBufferfv(GL_DEPTH, 0, &one);

	vec3 realcenter = eye + center;
	view = lookAt(eye, realcenter, up);

	model = mat4(1.0f);
	mv = projection * view * model;

	glUseProgram(program);
	GLuint texture_location = glGetUniformLocation(program, "tex");
	GLuint noise_location = glGetUniformLocation(program, "noiseMap");

	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(mv));
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(model));
	glUniform1i(glGetUniformLocation(program, "mode"), mode);

	for (int i = 0; i < v_shape.size(); i++)
	{
		glBindVertexArray(v_shape[i].vao);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, v_shape[i].ibo);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, v_material[v_shape[i].materialID].diffuse_tex);
		glUniform1i(texture_location, 0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, noiseTexture);
		glUniform1i(noise_location, 1);
		glDrawElements(GL_TRIANGLES, v_shape[i].drawCount, GL_UNSIGNED_INT, 0);
	}



	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, FBODataTexture);

	glBindVertexArray(window_vao);
	glUseProgram(program2);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	float t = glutGet(GLUT_ELAPSED_TIME);
	glUniform1f(glGetUniformLocation(program2, "time"), t / 1000);
	glUniform1f(glGetUniformLocation(program2, "width"), W);
	glUniform1i(mode_location, mode);
	glUniform1f(glGetUniformLocation(program2, "height"), H);



	glutSwapBuffers();
}

void My_Reshape(int width, int height)
{
	W = width;
	H = height;
	glViewport(0, 0, width, height);
	viewportAspect = (float)width / (float)height;

	projection = perspective(radians(45.0f), viewportAspect, 0.1f, 3000.f);

	// If the windows is reshaped, we need to reset some settings of framebuffer
	glDeleteRenderbuffers(1, &depthRBO);
	glDeleteTextures(1, &FBODataTexture);
	printf(" delete FBO textured\n");
	glGenRenderbuffers(1, &depthRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, depthRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, width, height);
	printf(" gen depth buffer\n");

	glGenTextures(1, &FBODataTexture);
	printf(" FBO gen\n");
	glBindTexture(GL_TEXTURE_2D, FBODataTexture);
	printf(" FBO bind\n");

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	printf(" FBO mirror\n");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	printf(" FBO textured\n");

	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FBODataTexture, 0);
}

void My_Timer(int val)
{
	glutPostRedisplay();
	glutTimerFunc(timer_speed, My_Timer, val);
}

void My_Mouse(int button, int state, int x, int y)
{
	if (state == GLUT_DOWN)
	{
		glUniform2f(glGetUniformLocation(program2, "resolution"), (float)W, (float)H);
		glUniform2f(glGetUniformLocation(program2, "mouse"), ((float)x / (float)W), ((float)y / (float)H));

	}
}

void mouse_callback(int xpos, int ypos) {
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;
	lastX = xpos;
	lastY = ypos;

	float sensitivity = 0.4f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	ya += xoffset;
	pit += yoffset;

	if (pit > 89.0f)
		pit = 89.0f;
	if (pit < -89.0f)
		pit = -89.0f;

	glm::vec3 direction;
	direction.x = cos(glm::radians(ya)) * cos(glm::radians(pit));
	direction.y = sin(glm::radians(pit));
	direction.z = sin(glm::radians(ya)) * cos(glm::radians(pit));
	center = glm::normalize(direction);
}


void My_Keyboard(unsigned char key, int x, int y)
{
	float cameraSpeed = 50.0f;
	if (key == 'w' || key == 'W') {
		eye += cameraSpeed * center;
	}
	if (key == 'a' || key == 'A') {
		eye -= normalize(cross(center, up)) * cameraSpeed;
	}
	if (key == 's' || key == 'S') {
		eye -= cameraSpeed * center;
	}
	if (key == 'd' || key == 'D') {
		eye += normalize(cross(center, up)) * cameraSpeed;
	}
	if (key == 'z' || key == 'Z') {
		eye += up;
	}
	if (key == 'x' || key == 'X') {
		eye -= up;
	}
}

void My_Menu(int id)
{
	mode = id;
}

int main(int argc, char* argv[])
{
#ifdef __APPLE__
	// Change working directory to source code path
	chdir(__FILEPATH__("/../Assets/"));
#endif
	// Initialize GLUT and GLEW, then create a window.
	////////////////////
	glutInit(&argc, argv);
#ifdef _MSC_VER
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#else
	glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(600, 600);
	glutCreateWindow("AS2_Framework"); // You cannot use OpenGL functions before this line; The OpenGL context must be created first by glutCreateWindow()!
#ifdef _MSC_VER
	glewInit();
#endif
	dumpInfo();
	My_Init();


	glutSetMenu(glutCreateMenu(My_Menu));
	glutAddMenuEntry("Image Abstraction", 0);
	glutAddMenuEntry("Water Color", 1);
	glutAddMenuEntry("Magnifier", 2);
	glutAddMenuEntry("Bloom Effect", 3);
	glutAddMenuEntry("Pixelation", 4);
	glutAddMenuEntry("Sin Wave", 5);
	glutAddMenuEntry("Diffuse Mapped", 6);
	glutAddMenuEntry("Output normal as color", 7);


	glutAttachMenu(GLUT_RIGHT_BUTTON);

	// Register GLUT callback functions.
	glutDisplayFunc(My_Display);
	glutReshapeFunc(My_Reshape);
	glutKeyboardFunc(My_Keyboard);
	glutMouseFunc(My_Mouse);
	glutPassiveMotionFunc(mouse_callback);
	glutTimerFunc(timer_speed, My_Timer, 0);

	// Enter main event loop.
	glutMainLoop();

	return 0;
}
