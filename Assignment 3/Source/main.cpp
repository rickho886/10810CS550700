#include "../Externals/Include/Common.h"

#define MAX_T 2
#define MAX_NP 3
#define SHADOW_MAP_SIZE 4096

GLubyte timer_cnt = 0;
bool timer_enabled = true;
float timer_speed = 0;

using namespace glm;
using namespace std;

GLuint program, program2, depthProg, skybox_prog;
GLuint window_vao;
GLuint shadow_location;
GLuint skybox_vao;
GLuint tex_envmap;
GLuint window_buffer;
GLint um4mv, um4p;
GLuint FBO;
GLuint FBODataTexture;
GLuint depthRBO;


struct Shape {
	GLuint vao;
	GLuint vbo_position;
	GLuint vbo_normal;
	GLuint vbo_texcoord;
	GLuint ibo;
	int drawCount;
	int materialID;
};

typedef struct
{
	GLuint vao;			// vertex array object
	GLuint vbo;			// vertex buffer object

	int materialId;
	int vertexCount;
	GLuint m_texture;
} Shape_test;

Shape_test m_shape;
Shape_test m_shape2;

struct
{
	struct
	{
		GLint inv_vp_matrix;
		GLint eye;
	} skybox;
} uniforms;

struct
{
	struct
	{
		GLint   mvp;
	} light;
	struct
	{
		GLuint  shadow_tex;
		GLint   mv_matrix;
		GLint   proj_matrix;
		GLint   shadow_matrix;
		GLint   full_shading;
		GLint   light_matrix;
	} view;
} uniform;

struct
{
	GLuint fbo;
	GLuint depthMap;
} shadowBuffer;

struct
{
	GLuint fbo;
	GLuint rbo;
	GLuint texture;
	GLuint uniform_tex;
} Sobj;

struct
{
	GLuint fbo;
	GLuint rbo;
	GLuint texture;
	GLuint uniform_tex;
} Snoobj;

struct
{
	GLuint fbo;
	GLuint rbo;
	GLuint texture;
	GLuint uniform_tex;
} Sb;

struct Material {
	GLuint diffuse_tex;
};

vector<Material> v_material;
vector<Shape> v_shape;
vector<Material> q_material;
vector<Shape> q_shape;
const char* model_list[] = { "right.png", "left.png", "top.png", "bottom.png", "back.png", "front.png" };

vec3 eye = vec3(0.0f, 0.0f, 0.0f);
vec3 center = vec3(3.0f, 5.0f, 0.0f);;
vec3 up = vec3(0.0f, 1.0f, 0.0f);
vec3 direction;
mat4 mv, projection, view, model_matrix;

int index;
int mode;
float viewportAspect;
float ya = -90.0f;
float pit = 0.0f;
float lastX = 600.0 / 2.0;
float lastY = 600.0 / 2.0;
bool firstMouse = true;

static const GLfloat window_positions[] =
{
	1.0f,-1.0f,1.0f,0.0f,
	-1.0f,-1.0f,0.0f,0.0f,
	-1.0f,1.0f,0.0f,1.0f,
	1.0f,1.0f,1.0f,1.0f
};

int W = 1440;
int H = 900;

char** loadShaderSource(const char* file)
{
	FILE* fp = fopen(file, "rb");
	fseek(fp, 0, SEEK_END);
	long sz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char* src = new char[sz + 1];
	fread(src, sizeof(char), sz, fp);
	src[sz] = '\0';
	char** srcp = new char*[1];
	srcp[0] = src;
	return srcp;
}

void freeShaderSource(char** srcp)
{
	delete[] srcp[0];
	delete[] srcp;
}

void My_LoadModels()
{
	tinyobj::attrib_t attrib;
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	string warn;
	string err;
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, "nanosuit.obj");
	if (!warn.empty()) {
		cout << warn << endl;
	}
	if (!err.empty()) {
		cout << err << endl;
	}
	if (!ret) {
		exit(1);
	}

	vector<float> vertices, texcoords, normals;  // if OBJ preserves vertex order, you can use element array buffer for memory efficiency
	for (int s = 0; s < shapes.size(); ++s) {  // for 'ladybug.obj', there is only one object
		int index_offset = 0;
		for (int f = 0; f < shapes[s].mesh.num_face_vertices.size(); ++f) {
			int fv = shapes[s].mesh.num_face_vertices[f];
			for (int v = 0; v < fv; ++v) {
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
				vertices.push_back(attrib.vertices[3 * idx.vertex_index + 0]);
				vertices.push_back(attrib.vertices[3 * idx.vertex_index + 1]);
				vertices.push_back(attrib.vertices[3 * idx.vertex_index + 2]);
				texcoords.push_back(attrib.texcoords[2 * idx.texcoord_index + 0]);
				texcoords.push_back(attrib.texcoords[2 * idx.texcoord_index + 1]);
				normals.push_back(attrib.normals[3 * idx.normal_index + 0]);
				normals.push_back(attrib.normals[3 * idx.normal_index + 1]);
				normals.push_back(attrib.normals[3 * idx.normal_index + 2]);
			}
			index_offset += fv;
			m_shape.vertexCount += fv;
		}
	}

	glGenVertexArrays(1, &m_shape.vao);
	glBindVertexArray(m_shape.vao);

	glGenBuffers(1, &m_shape.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_shape.vbo);

	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float) + texcoords.size() * sizeof(float) + normals.size() * sizeof(float), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());
	glBufferSubData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), texcoords.size() * sizeof(float), texcoords.data());
	glBufferSubData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float) + texcoords.size() * sizeof(float), normals.size() * sizeof(float), normals.data());

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(vertices.size() * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(vertices.size() * sizeof(float) + texcoords.size() * sizeof(float)));
	glEnableVertexAttribArray(2);

	shapes.clear();
	shapes.shrink_to_fit();
	materials.clear();
	materials.shrink_to_fit();
	vertices.clear();
	vertices.shrink_to_fit();
	texcoords.clear();
	texcoords.shrink_to_fit();
	normals.clear();
	normals.shrink_to_fit();

	cout << "Load " << m_shape.vertexCount << " vertices" << endl;
}

void My_LoadModels2()
{
	tinyobj::attrib_t attrib;
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	string warn;
	string err;
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, "Plane.obj");
	if (!warn.empty()) {
		cout << warn << endl;
	}
	if (!err.empty()) {
		cout << err << endl;
	}
	if (!ret) {
		exit(1);
	}

	vector<float> vertices, texcoords, normals;  // if OBJ preserves vertex order, you can use element array buffer for memory efficiency
	for (int s = 0; s < shapes.size(); ++s) {  // for 'ladybug.obj', there is only one object
		int index_offset = 0;
		for (int f = 0; f < shapes[s].mesh.num_face_vertices.size(); ++f) {
			int fv = shapes[s].mesh.num_face_vertices[f];
			for (int v = 0; v < fv; ++v) {
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
				vertices.push_back(attrib.vertices[3 * idx.vertex_index + 0]);
				vertices.push_back(attrib.vertices[3 * idx.vertex_index + 1]);
				vertices.push_back(attrib.vertices[3 * idx.vertex_index + 2]);
				texcoords.push_back(attrib.texcoords[2 * idx.texcoord_index + 0]);
				texcoords.push_back(attrib.texcoords[2 * idx.texcoord_index + 1]);
				normals.push_back(attrib.normals[3 * idx.normal_index + 0]);
				normals.push_back(attrib.normals[3 * idx.normal_index + 1]);
				normals.push_back(attrib.normals[3 * idx.normal_index + 2]);
			}
			index_offset += fv;
			m_shape2.vertexCount += fv;
		}
	}

	glGenVertexArrays(1, &m_shape2.vao);
	glBindVertexArray(m_shape2.vao);

	glGenBuffers(1, &m_shape2.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_shape2.vbo);

	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float) + texcoords.size() * sizeof(float) + normals.size() * sizeof(float), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());
	glBufferSubData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), texcoords.size() * sizeof(float), texcoords.data());
	glBufferSubData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float) + texcoords.size() * sizeof(float), normals.size() * sizeof(float), normals.data());

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(vertices.size() * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(vertices.size() * sizeof(float) + texcoords.size() * sizeof(float)));
	glEnableVertexAttribArray(2);

	shapes.clear();
	shapes.shrink_to_fit();
	materials.clear();
	materials.shrink_to_fit();
	vertices.clear();
	vertices.shrink_to_fit();
	texcoords.clear();
	texcoords.shrink_to_fit();
	normals.clear();
	normals.shrink_to_fit();

	cout << "Load " << m_shape2.vertexCount << " vertices" << endl;
}


void My_Init()
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	printf("my_init\n");
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	// ----- Begin Initialize Depth Shader Program -----------------------------------------------------------------------------------------------------------------------
	depthProg = glCreateProgram();
	GLuint shadow_vs;
	GLuint shadow_fs;
	shadow_vs = glCreateShader(GL_VERTEX_SHADER);
	char** shadow_vs_Source = loadShaderSource("depth.vs.glsl");
	glShaderSource(shadow_vs, 1, shadow_vs_Source, NULL);
	freeShaderSource(shadow_vs_Source);
	glCompileShader(shadow_vs);
	shaderLog(shadow_vs);
	glAttachShader(depthProg, shadow_vs);

	shadow_fs = glCreateShader(GL_FRAGMENT_SHADER);
	char** shadow_fs_Source = loadShaderSource("depth.fs.glsl");
	glShaderSource(shadow_fs, 1, shadow_fs_Source, NULL);
	freeShaderSource(shadow_fs_Source);
	glCompileShader(shadow_fs);
	shaderLog(shadow_fs);
	glAttachShader(depthProg, shadow_fs);

	glLinkProgram(depthProg);
	uniform.light.mvp = glGetUniformLocation(depthProg, "mvp");
	// ----- End Initialize Depth Shader Program -----------------------------------------------------------------------------------------------------------------------


	//skybox------------------------------------------------------------------------------------------------------------------
	skybox_prog = glCreateProgram();
	GLuint skybox_fs = glCreateShader(GL_FRAGMENT_SHADER);
	char** skybox_fs_Source = loadShaderSource("skybox.fs.glsl");
	glShaderSource(skybox_fs, 1, skybox_fs_Source, NULL);
	freeShaderSource(skybox_fs_Source);
	glCompileShader(skybox_fs);
	shaderLog(skybox_fs);
	glAttachShader(skybox_prog, skybox_fs);

	GLuint skybox_vs = glCreateShader(GL_VERTEX_SHADER);
	char** skybox_vs_Source = loadShaderSource("skybox.vs.glsl");
	glShaderSource(skybox_vs, 1, skybox_vs_Source, NULL);
	freeShaderSource(skybox_vs_Source);
	glCompileShader(skybox_vs);
	shaderLog(skybox_vs);
	glAttachShader(skybox_prog, skybox_vs);

	glLinkProgram(skybox_prog);

	uniforms.skybox.inv_vp_matrix = glGetUniformLocation(skybox_prog, "inv_vp_matrix");
	uniforms.skybox.eye = glGetUniformLocation(skybox_prog, "eye");

	texture_data envmap_data;
	glGenTextures(1, &tex_envmap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, tex_envmap);
	for (int i = 0; i < 6; ++i)
	{
		envmap_data = loadImg(model_list[i]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
			0,
			GL_RGBA,
			envmap_data.width,
			envmap_data.height,
			0,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			envmap_data.data);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	delete[] envmap_data.data;

	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	glGenVertexArrays(1, &skybox_vao);

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

	um4mv = glGetUniformLocation(program, "um4mv");
	um4p = glGetUniformLocation(program, "um4p");
	shadow_location = glGetUniformLocation(program, "index");
	uniform.view.shadow_matrix = glGetUniformLocation(program, "shadow_matrix");
	uniform.view.shadow_tex = glGetUniformLocation(program, "shadow_tex");
	Sobj.uniform_tex = glGetUniformLocation(program, "sobj_tex");
	Snoobj.uniform_tex = glGetUniformLocation(program, "snoobj_tex");
	Sb.uniform_tex = glGetUniformLocation(program, "sb_tex");

	My_LoadModels();
	My_LoadModels2();

	program2 = glCreateProgram();

	char** FB_vertexShaderSource = loadShaderSource("FB_vertex.vs.glsl");
	char** FB_fragmentShaderSource = loadShaderSource("FB_fragment.fs.glsl");

	GLuint vs2 = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs2, 1, FB_vertexShaderSource, NULL);
	freeShaderSource(FB_vertexShaderSource);
	glCompileShader(vs2);
	shaderLog(vs2);

	GLuint fs2 = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs2, 1, FB_fragmentShaderSource, NULL);
	freeShaderSource(FB_fragmentShaderSource);
	glCompileShader(fs2);
	shaderLog(fs2);

	glAttachShader(program2, vs2);
	glAttachShader(program2, fs2);
	glLinkProgram(program2);


	glGenVertexArrays(1, &window_vao);
	glBindVertexArray(window_vao);

	glGenBuffers(1, &window_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, window_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(window_positions), window_positions, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * 4, 0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * 4, (const GLvoid*)(sizeof(GL_FLOAT) * 2));

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	// ----- Begin Initialize Shadow Framebuffer Object -----
	glGenFramebuffers(1, &shadowBuffer.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowBuffer.fbo);

	glGenTextures(1, &shadowBuffer.depthMap);
	glBindTexture(GL_TEXTURE_2D, shadowBuffer.depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);


	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowBuffer.depthMap, 0);

	glGenFramebuffers(1, &Sobj.fbo);

	glGenFramebuffers(1, &Snoobj.fbo);

	glGenFramebuffers(1, &Sb.fbo);

}
float val_y = 0.0f;
void My_Display()
{
	timer_speed += 0.05;
	mat4 scale_bias_matrix = translate(mat4(1.0f), vec3(0.5f, 0.5f, 0.5f));
	scale_bias_matrix = scale(scale_bias_matrix, vec3(0.5f, 0.5f, 0.5f));

	model_matrix = translate(mat4(1.0), vec3());
	model_matrix = translate(model_matrix, vec3(-10.0f, -13.0f, -8.0f));
	model_matrix = scale(model_matrix, vec3(0.5f, 0.35f, 0.5f));
	model_matrix = rotate(model_matrix, val_y, vec3(0.0f, 0.25f, 0.0f));

	mat4 quad_matrix = translate(mat4(1.0), vec3());
	quad_matrix = translate(quad_matrix, vec3(-10.0f, -13.0f, 0.0f));
	quad_matrix = scale(quad_matrix, vec3(3.2f, 3.2f, 3.2f));
	//quad_matrix = translate(quad_matrix, vec3(-10.0f, -13.0f, -8.0f));
	//quad_matrix = scale(quad_matrix, vec3(0.0f, 0.0f, 0.0f));
	//quad_matrix = rotate(quad_matrix, 90.f, vec3(0.0f, 1.0f, 0.0f));

	static const GLfloat gray[] = { 0.2f, 0.2f, 0.2f, 1.0f };
	static const GLfloat ones[] = { 1.0f };

	// ----- Begin Shadow Map Pass -----
	const float shadow_range = 15.0f;
	mat4 light_proj_matrix = ortho(-shadow_range, shadow_range, -shadow_range, shadow_range, 0.0f, 100.0f);
	mat4 light_view_matrix = lookAt(vec3(-31.75, 26.05, -97.72), vec3(0.0f, 0.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f));

	mat4 light_vp_matrix = light_proj_matrix * light_view_matrix;
	mat4 shadow_sbpv_matrix = scale_bias_matrix * light_vp_matrix;

	mat4 shadow_matrix;

	if (mode == 0 || mode == 1) {
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, Sobj.fbo);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);

		glEnable(GL_STENCIL_TEST);
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
		glStencilFunc(GL_ALWAYS, 1, 0xFF);
		glStencilMask(0xFF);

		glBindFramebuffer(GL_FRAMEBUFFER, shadowBuffer.fbo);
		glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(4.0f, 4.0f);

		glUseProgram(depthProg);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		index = 0;
		glUniform1i(shadow_location, index);
		glUniformMatrix4fv(uniform.light.mvp, 1, GL_FALSE, value_ptr(light_vp_matrix * model_matrix));
		glBindVertexArray(m_shape.vao);
		glDrawArrays(GL_TRIANGLES, 0, m_shape.vertexCount);

		index = 1;
		glUniform1i(shadow_location, index);
		glUniformMatrix4fv(uniform.light.mvp, 1, GL_FALSE, value_ptr(light_vp_matrix * quad_matrix));
		glBindVertexArray(m_shape2.vao);
		glDrawArrays(GL_TRIANGLES, 0, m_shape2.vertexCount);

		glDisable(GL_POLYGON_OFFSET_FILL);
		// ----- End Shadow Map Pass -----

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, W, H);

		view = lookAt(eye, vec3(-1.0f, -1.0f, 0.0f), up);

		//draw model and quad
		glUseProgram(program);

		glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, shadowBuffer.depthMap);
		glUniform1i(uniform.view.shadow_tex, 0);

		//render model

		shadow_matrix = shadow_sbpv_matrix * model_matrix;
		index = 0;
		glUniform1i(shadow_location, index);
		glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view * model_matrix));
		glUniformMatrix4fv(uniform.view.shadow_matrix, 1, GL_FALSE, value_ptr(shadow_matrix));
		glBindVertexArray(m_shape.vao);
		glDrawArrays(GL_TRIANGLES, 0, m_shape.vertexCount);

		glStencilMask(0x00);
		glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
		glClear(GL_COLOR_BUFFER_BIT);

		//render quad
		shadow_matrix = shadow_sbpv_matrix * quad_matrix;
		index = 1;
		glUniform1i(shadow_location, index);
		glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view * quad_matrix));
		glUniformMatrix4fv(uniform.view.shadow_matrix, 1, GL_FALSE, value_ptr(shadow_matrix));
		glBindVertexArray(m_shape2.vao);
		glDrawArrays(GL_TRIANGLES, 0, m_shape2.vertexCount);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, Sobj.texture);
		glUniform1i(Sobj.uniform_tex, 1);
	}
	// --- Snoobj
	if (mode == 1) {
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, Snoobj.fbo);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);

		//glEnable(GL_STENCIL_TEST);
		//glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

		glBindFramebuffer(GL_FRAMEBUFFER, shadowBuffer.fbo);
		glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(4.0f, 4.0f);

		glUseProgram(depthProg);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		index = 1;
		glUniform1i(shadow_location, index);
		glUniformMatrix4fv(uniform.light.mvp, 1, GL_FALSE, value_ptr(light_vp_matrix * quad_matrix));


		glDisable(GL_POLYGON_OFFSET_FILL);
		// ----- End Shadow Map Pass -----

		glBindFramebuffer(GL_FRAMEBUFFER, 0);


		//skybox
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, W, H);

		view = lookAt(eye, vec3(-1.0f, -1.0f, 0.0f), up);



		//draw model and quad
		glUseProgram(program);

		glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, shadowBuffer.depthMap);
		glUniform1i(uniform.view.shadow_tex, 0);

		//glStencilMask(0x00);
		//glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
		//glClear(GL_COLOR_BUFFER_BIT);

		//render quad
		shadow_matrix = shadow_sbpv_matrix * quad_matrix;
		index = 1;
		glUniform1i(shadow_location, index);
		glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view * quad_matrix));
		glUniformMatrix4fv(uniform.view.shadow_matrix, 1, GL_FALSE, value_ptr(shadow_matrix));
		glBindVertexArray(m_shape2.vao);
		glDrawArrays(GL_TRIANGLES, 0, m_shape2.vertexCount);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, Snoobj.texture);
		glUniform1i(Snoobj.uniform_tex, 2);
	}
	// --- Sb ---
	else if (mode == 2) {
		glStencilMask(0xFF);
		glClear(GL_STENCIL_BUFFER_BIT);
		glStencilMask(0x00);
		glStencilFunc(GL_NOTEQUAL, 1, 0xFF);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, Sb.fbo);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);

		glEnable(GL_DEPTH_TEST);

		glBindFramebuffer(GL_FRAMEBUFFER, shadowBuffer.fbo);
		glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(4.0f, 4.0f);

		glUseProgram(depthProg);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		index = 0;
		glUniform1i(shadow_location, index);
		glUniformMatrix4fv(uniform.light.mvp, 1, GL_FALSE, value_ptr(light_vp_matrix * model_matrix));
		glBindVertexArray(m_shape.vao);
		glDrawArrays(GL_TRIANGLES, 0, m_shape.vertexCount);




		glDisable(GL_POLYGON_OFFSET_FILL);
		// ----- End Shadow Map Pass -----

		glBindFramebuffer(GL_FRAMEBUFFER, 0);


		//skybox
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, W, H);

		glBindTexture(GL_TEXTURE_CUBE_MAP, tex_envmap);
		view = lookAt(eye, vec3(-1.0f, -1.0f, 0.0f), up);
		mat4 inv_vp_matrix = inverse(projection * view);

		glUseProgram(skybox_prog);
		glBindVertexArray(skybox_vao);

		glUniformMatrix4fv(uniforms.skybox.inv_vp_matrix, 1, GL_FALSE, &inv_vp_matrix[0][0]);
		glUniform3fv(uniforms.skybox.eye, 1, &eye[0]);

		glDisable(GL_DEPTH_TEST);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glEnable(GL_DEPTH_TEST);


		//draw model and quad
		glUseProgram(program);

		glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, shadowBuffer.depthMap);
		glUniform1i(uniform.view.shadow_tex, 0);

		shadow_matrix = shadow_sbpv_matrix * model_matrix;
		//printf("%d\n", shadow_matrix);

		//render model
		index = 0;
		glUniform1i(shadow_location, index);
		glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view * model_matrix));
		glUniformMatrix4fv(uniform.view.shadow_matrix, 1, GL_FALSE, value_ptr(shadow_matrix));
		glBindVertexArray(m_shape.vao);
		glDrawArrays(GL_TRIANGLES, 0, m_shape.vertexCount);

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, Sb.texture);
		glUniform1i(Sb.uniform_tex, 3);
	}
	//render quad

	/*glBindVertexArray(window_vao);
	glUseProgram(program2);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);*/

	glutSwapBuffers();
}

void My_Reshape(int width, int height)
{
	W = width;
	H = height;
	glViewport(0, 0, width, height);
	viewportAspect = (float)width / (float)height;

	projection = perspective(radians(80.0f), viewportAspect, 0.1f, 1000.f);

	// Sobj

	glDeleteRenderbuffers(1, &Sobj.rbo);
	glDeleteTextures(1, &Sobj.texture);
	glGenRenderbuffers(1, &Sobj.rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, Sobj.rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

	glGenTextures(1, &Sobj.texture);
	glBindTexture(GL_TEXTURE_2D, Sobj.texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindFramebuffer(GL_FRAMEBUFFER, Sobj.fbo);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, Sobj.rbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Sobj.texture, 0);

	// Snoobj

	glGenTextures(1, &Snoobj.texture);
	glBindTexture(GL_TEXTURE_2D, Snoobj.texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindFramebuffer(GL_FRAMEBUFFER, Snoobj.fbo);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, Sobj.rbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Snoobj.texture, 0);

	// Sb

	glDeleteRenderbuffers(1, &Sb.rbo);
	glDeleteTextures(1, &Sb.texture);
	glGenRenderbuffers(1, &Sb.rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, Sb.rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

	glGenTextures(1, &Sb.texture);
	glBindTexture(GL_TEXTURE_2D, Sb.texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindFramebuffer(GL_FRAMEBUFFER, Sb.fbo);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, Sb.rbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Sb.texture, 0);

}

void My_Timer(int val)
{
	glutPostRedisplay();
	glutTimerFunc(timer_speed, My_Timer, val);
}

void My_Mouse(int button, int state, int x, int y)
{

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
	/*float cameraSpeed = 5.0f;
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
	}*/

	if (key == 'i' || key == 'I') {
		if (mode == 2) {
			mode = 0;
		}
		else {
			mode += 1;
		}
	}
	if (key == 'q' || key == 'Q') {
		val_y += 1.0;
		glStencilMask(0xFF);
		glClear(GL_STENCIL_BUFFER_BIT);
		glStencilMask(0x00);
		glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
	if (key == 'e' || key == 'E') {
		val_y -= 1.0;
		glStencilMask(0xFF);
		glClear(GL_STENCIL_BUFFER_BIT);
		glStencilMask(0x00);
		glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
}

void My_Menu(int id)
{
	;
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
	glutInitWindowSize(1440, 900);
	glutCreateWindow("AS2_Framework"); // You cannot use OpenGL functions before this line; The OpenGL context must be created first by glutCreateWindow()!
#ifdef _MSC_VER
	glewInit();
#endif
	dumpInfo();
	mode = 0;
	My_Init();


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
