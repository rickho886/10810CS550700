#include "../Include/Common.h"

//For GLUT to handle 
#define MENU_TIMER_START 1
#define MENU_TIMER_STOP 2
#define MENU_EXIT 3
#define MENU_HELLO 4
#define MENU_WORLD 5

//GLUT timer variable
float timer_cnt = 0;
bool timer_enabled = true;
unsigned int timer_speed = 16;

using namespace glm;
using namespace std;

// Default window size
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
// current window size
int screenWidth = WINDOW_WIDTH, screenHeight = WINDOW_HEIGHT;

int rotate_flag;
int resume_flag;
double waktu = 0;
size_t cur_frame = -1;

enum {
	TORSO, HEAD, L_EYE, R_EYE, L_EAR, R_EAR, MOUTH, L_SHOULDER, L1_ARM, L_ELBOW, L2_ARM, R_SHOULDER, R1_ARM, R_ELBOW, R2_ARM, L_HIP, L1_LEG, L_KNEE, L2_LEG, R_HIP, R1_LEG, R_KNEE, R2_LEG
};

vector<string> filenames; // .obj filename list
vector<mat4> M;
vector<mat4> T;
vector<mat4> R;
vector<mat4> S;

typedef struct {
	GLuint diffuseTexture;
} PhongMaterial;

typedef struct {
	GLuint vao;
	GLuint vbo;
	GLuint vboTex;
	GLuint ebo;
	GLuint p_color;
	int vertex_count;
	GLuint p_normal;
	GLuint p_texCoord;
	PhongMaterial material;
	int indexCount;
} Shape;

struct model {
	glm::vec3 position = glm::vec3(0, 0, 0);
	glm::vec3 scale = glm::vec3(1, 1, 1);
	glm::vec3 rotation = glm::vec3(0, 0, 0);	// Euler form

	std::vector<Shape> shapes;
};

vector<model> models;

struct camera {
	vec3 position;
	vec3 center;
	vec3 up_vector;
};
camera main_camera;

struct project_setting {
	GLfloat nearClip, farClip;
	GLfloat fovy;
	GLfloat aspect;
};
project_setting proj;

vector<string> model_list{ "../TextureModels/body.obj", "../TextureModels/head.obj", "../TextureModels/eyes.obj", "../TextureModels/eyes.obj", "../TextureModels/sphere.obj", "../TextureModels/sphere.obj", "../TextureModels/mouth.obj", "../TextureModels/cube.obj", "../TextureModels/arm_1.obj", "../TextureModels/cylinder.obj", "../TextureModels/arm_2.obj", "../TextureModels/cube.obj", "../TextureModels/arm_1.obj", "../TextureModels/cylinder.obj", "../TextureModels/arm_2.obj", "../TextureModels/cylinder.obj", "../TextureModels/cube.obj", "../TextureModels/cylinder.obj", "../TextureModels/cube.obj", "../TextureModels/cylinder.obj", "../TextureModels/cube.obj", "../TextureModels/cylinder.obj", "../TextureModels/cube.obj" };

GLuint program;

GLuint iLocP;
GLuint iLocV;
GLuint iLocM;

// Load shader file to program
char* textFileRead(const char* fn) {


	FILE* fp;
	char* content = NULL;

	int count = 0;

	if (fn != NULL) {
		fp = fopen(fn, "rt");

		if (fp != NULL) {
			fseek(fp, 0, SEEK_END);
			count = ftell(fp);
			rewind(fp);

			if (count > 0) {
				content = (char*)malloc(sizeof(char) * (count + 1));
				count = fread(content, sizeof(char), count, fp);
				content[count] = '\0';
			}
			fclose(fp);
		}
		else {
			printf("The file \"%s\" was not opened\n", fn);
		}
	}
	return content;
}

int textFileWrite(char* fn, char* s) {

	FILE* fp;
	int status = 0;

	if (fn != NULL) {
		fp = fopen(fn, "rt");
		if (fp != NULL) {
			if (fwrite(s, sizeof(char), strlen(s), fp) == strlen(s))
				status = 1;
			fclose(fp);
		}
		else {
			printf("The file \"%s\" was not opened\n", fn);
		}
	}
	return(status);
}

void translate_model(int id) {

	switch (id) {
	case TORSO:
		float x, y, z;
		x = models[id].position.x;
		y = models[id].position.y;
		z = models[id].position.z - 13;
		T[id] = translate(mat4(1.0f), vec3(x, y, z));
		break;
	case HEAD:
		T[id] = translate(mat4(1.0f), vec3(0, 3 + abs((float)sin(waktu)), 0));
		break;
	case L_EYE:
		T[id] = translate(mat4(1.0f), vec3(-0.5, 0.3, 1));
		break;
	case R_EYE:
		T[id] = translate(mat4(1.0f), vec3(0.5, 0.3, 1));
		break;
	case MOUTH:
		T[id] = translate(mat4(1.0f), vec3(0, -0.3, 0.75));
		break;
	case L_EAR:
		T[id] = translate(mat4(1.0f), vec3(-1, 0, 0));
		break;
	case R_EAR:
		T[id] = translate(mat4(1.0f), vec3(1, 0, 0));
		break;
	case L_SHOULDER:
		T[id] = translate(mat4(1.0f), vec3(3, 2, 0));
		break;
	case R_SHOULDER:
		T[id] = translate(mat4(1.0f), vec3(-3, 2, 0));
		break;
	case L_HIP:
		T[id] = translate(mat4(1.0f), vec3(-1, -2.3f, 0));
		break;
	case R_HIP:
		T[id] = translate(mat4(1.0f), vec3(1, -2.3f, 0));
		break;
	case L2_LEG:
	case R2_LEG:
		T[id] = translate(mat4(1.0f), vec3(0, -1.0, 0));
		break;
	default:
		T[id] = translate(mat4(1.0f), vec3(0, -1.5, 0));
		break;
	}
}

void rotate_model(int id) {
	switch (id) {
	case TORSO:
		if (rotate_flag) {
			GLfloat degree = (float)sin(waktu);
			vec3 rotate_axis = vec3(0.0, 1.0, 0.0);
			R[id] = rotate(mat4(1.0), degree, rotate_axis);
		}
		break;
	case L_SHOULDER:
	case R_SHOULDER:
		R[id] = rotate(mat4(1.0f), (float)sin(waktu) / 2, vec3(1.0f, 0.0f, 0.0f));
		break;
	case L_ELBOW:
	case R_ELBOW:
		R[id] = rotate(mat4(1.0f), (float)sin(waktu) / 2, vec3(1.0f, 2.0f, 0.0f));
		break;
	case L_HIP:
		R[id] = rotate(mat4(1.0f), (float)sin(waktu) / 2, vec3(1.0f, 0.0f, 0.0f));
		break;
	case R_HIP:
		R[id] = rotate(mat4(1.0f), (float)sin(waktu) / 2, vec3(-1.0f, 0.0f, 0.0f));
		break;
	case L_KNEE:
	case R_KNEE:
		R[id] = rotate(mat4(1.0f), (float)sin(waktu) / 2, vec3(1.0f, 0.0f, 0.0f));
		break;
	}
}

void scale_model(int id) {
	switch (id) {
	case TORSO:
		S[id] = scale(mat4(1.0f), vec3(2.0f, 2.0f, 1.0f));
		break;
	case HEAD:
		S[id] = scale(mat4(1.0f), vec3(1.0f, 1.0f, 1.0f));
		break;
	case L1_ARM:
		S[id] = scale(mat4(1.0f), vec3(0.5f, 1.0f, 0.5f));
		break;
	case R1_ARM:
		S[id] = scale(mat4(1.0f), vec3(0.5f, 1.0f, 0.5f));
		break;
	case L2_LEG:
		S[id] = scale(mat4(1.0f), vec3(1.0f, 0.3f, 1.0f));
		break;
	case R2_LEG:
		S[id] = scale(mat4(1.0f), vec3(1.0f, 0.3f, 1.0f));
		break;
	case L_EYE:
	case R_EYE:
		S[id] = scale(mat4(1.0f), vec3(0.2f, 0.2f, 0.2f));
		break;
	case MOUTH:
		S[id] = scale(mat4(1.0f), vec3(0.7f, 0.3f, 0.3f));
		break;
	case L_EAR:
	case R_EAR:
		S[id] = scale(mat4(1.0f), vec3(0.4f, 0.4f, 0.4f));
		break;
	}
}

void transform_model(int id) {
	switch (id) {
	case TORSO:
		M[id] = T[id] * R[id] * S[id];
		break;
	case HEAD:
	case L_SHOULDER:
	case R_SHOULDER:
	case L_HIP:
	case R_HIP:
		M[id] = M[TORSO] * inverse(S[TORSO]) * T[id] * R[id] * S[id];
		break;
	case L_EYE:
	case R_EYE:
	case L_EAR:
	case R_EAR:
	case MOUTH:
		M[id] = M[HEAD] * inverse(S[HEAD]) * T[id] * R[id] * S[id];
		break;
	default:
		M[id] = M[id - 1] * T[id] * R[id] * S[id];
		break;
	}
}

void transfer_uniform(mat4 M, mat4 V, mat4 P) {

}

void setShaders() {
	GLuint v, f, p;
	char* vs = NULL;
	char* fs = NULL;

	v = glCreateShader(GL_VERTEX_SHADER);
	f = glCreateShader(GL_FRAGMENT_SHADER);

	vs = textFileRead("shader.vs.glsl");
	fs = textFileRead("shader.fs.glsl");

	glShaderSource(v, 1, (const GLchar**)&vs, NULL);
	glShaderSource(f, 1, (const GLchar**)&fs, NULL);

	free(vs);
	free(fs);

	GLint success;
	char infoLog[1000];
	// compile vertex shader
	glCompileShader(v);
	// check for shader compile errors
	glGetShaderiv(v, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(v, 1000, NULL, infoLog);
		std::cout << "ERROR: VERTEX SHADER COMPILATION FAILED\n" << infoLog << std::endl;
	}

	// compile fragment shader
	glCompileShader(f);
	// check for shader compile errors
	glGetShaderiv(f, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(f, 1000, NULL, infoLog);
		std::cout << "ERROR: FRAGMENT SHADER COMPILATION FAILED\n" << infoLog << std::endl;
	}

	// create program object
	p = glCreateProgram();

	// attach shaders to program object
	glAttachShader(p, f);
	glAttachShader(p, v);

	// link program
	glLinkProgram(p);
	// check for linking errors
	glGetProgramiv(p, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(p, 1000, NULL, infoLog);
		std::cout << "ERROR: SHADER PROGRAM LINKING FAILED\n" << infoLog << std::endl;
	}

	glDeleteShader(v);
	glDeleteShader(f);

	if (success)
		glUseProgram(p);
	else
	{
		system("pause");
		exit(123);
	}

	program = p;
}

void normalization(tinyobj::attrib_t* attrib, vector<GLfloat>& vertices, vector<GLfloat>& colors, vector<GLfloat>& normals, vector<GLfloat>& textureCoords, vector<int>& material_id, tinyobj::shape_t* shape)
{
	vector<float> xVector, yVector, zVector;
	float minX = 10000, maxX = -10000, minY = 10000, maxY = -10000, minZ = 10000, maxZ = -10000;

	// find out min and max value of X, Y and Z axis
	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		//maxs = max(maxs, attrib->vertices.at(i));
		if (i % 3 == 0)
		{

			xVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minX)
			{
				minX = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxX)
			{
				maxX = attrib->vertices.at(i);
			}
		}
		else if (i % 3 == 1)
		{
			yVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minY)
			{
				minY = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxY)
			{
				maxY = attrib->vertices.at(i);
			}
		}
		else if (i % 3 == 2)
		{
			zVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minZ)
			{
				minZ = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxZ)
			{
				maxZ = attrib->vertices.at(i);
			}
		}
	}

	float offsetX = (maxX + minX) / 2;
	float offsetY = (maxY + minY) / 2;
	float offsetZ = (maxZ + minZ) / 2;

	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		if (offsetX != 0 && i % 3 == 0)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetX;
		}
		else if (offsetY != 0 && i % 3 == 1)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetY;
		}
		else if (offsetZ != 0 && i % 3 == 2)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetZ;
		}
	}

	float greatestAxis = maxX - minX;
	float distanceOfYAxis = maxY - minY;
	float distanceOfZAxis = maxZ - minZ;

	if (distanceOfYAxis > greatestAxis)
	{
		greatestAxis = distanceOfYAxis;
	}

	if (distanceOfZAxis > greatestAxis)
	{
		greatestAxis = distanceOfZAxis;
	}

	float scale = greatestAxis / 2;

	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		//std::cout << i << " = " << (double)(attrib.vertices.at(i) / greatestAxis) << std::endl;
		attrib->vertices.at(i) = attrib->vertices.at(i) / scale;
	}
	size_t index_offset = 0;
	for (size_t f = 0; f < shape->mesh.num_face_vertices.size(); f++) {
		int fv = shape->mesh.num_face_vertices[f];

		// Loop over vertices in the face.
		for (size_t v = 0; v < fv; v++) {
			// access to vertex
			tinyobj::index_t idx = shape->mesh.indices[index_offset + v];
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 0]);
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 1]);
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 2]);
			// Optional: vertex colors
			colors.push_back(attrib->colors[3 * idx.vertex_index + 0]);
			colors.push_back(attrib->colors[3 * idx.vertex_index + 1]);
			colors.push_back(attrib->colors[3 * idx.vertex_index + 2]);
			// Optional: vertex normals
			normals.push_back(attrib->normals[3 * idx.normal_index + 0]);
			normals.push_back(attrib->normals[3 * idx.normal_index + 1]);
			normals.push_back(attrib->normals[3 * idx.normal_index + 2]);
			// Optional: texture coordinate
			textureCoords.push_back(attrib->texcoords[2 * idx.texcoord_index + 0]);
			textureCoords.push_back(attrib->texcoords[2 * idx.texcoord_index + 1]);
			// The material of this vertex
			material_id.push_back(shape->mesh.material_ids[f]);
		}
		index_offset += fv;
	}
}

static string GetBaseDir(const string& filepath) {
	if (filepath.find_last_of("/\\") != std::string::npos)
		return filepath.substr(0, filepath.find_last_of("/\\"));
	return "";
}

GLuint LoadTextureImage(string image_path)
{
	int channel, width, height;
	int require_channel = 4;
	stbi_set_flip_vertically_on_load(true);
	stbi_uc* data = stbi_load(image_path.c_str(), &width, &height, &channel, require_channel);
	if (data != NULL)
	{
		GLuint tex;

		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		stbi_image_free(data);
		return tex;
	}
	else
	{
		cout << "LoadTextureImage: Cannot load image from " << image_path << endl;
		return -1;
	}
}

vector<Shape> SplitShapeByMaterial(vector<GLfloat>& vertices, vector<GLfloat>& colors, vector<GLfloat>& normals, vector<GLfloat>& textureCoords, vector<int>& material_id, vector<PhongMaterial>& materials)
{
	vector<Shape> res;
	for (int m = 0; m < materials.size(); m++)
	{
		vector<GLfloat> m_vertices, m_colors, m_normals, m_textureCoords;
		for (int v = 0; v < material_id.size(); v++)
		{
			// extract all vertices with same material id and create a new shape for it.
			if (material_id[v] == m)
			{
				m_vertices.push_back(vertices[v * 3 + 0]);
				m_vertices.push_back(vertices[v * 3 + 1]);
				m_vertices.push_back(vertices[v * 3 + 2]);

				m_colors.push_back(colors[v * 3 + 0]);
				m_colors.push_back(colors[v * 3 + 1]);
				m_colors.push_back(colors[v * 3 + 2]);

				m_normals.push_back(normals[v * 3 + 0]);
				m_normals.push_back(normals[v * 3 + 1]);
				m_normals.push_back(normals[v * 3 + 2]);

				m_textureCoords.push_back(textureCoords[v * 2 + 0]);
				m_textureCoords.push_back(textureCoords[v * 2 + 1]);
			}
		}

		if (!m_vertices.empty())
		{
			Shape tmp_shape;
			glGenVertexArrays(1, &tmp_shape.vao);
			glBindVertexArray(tmp_shape.vao);

			glGenBuffers(1, &tmp_shape.vbo);
			glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.vbo);
			glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(GL_FLOAT), &m_vertices.at(0), GL_STATIC_DRAW);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
			tmp_shape.vertex_count = m_vertices.size() / 3;

			glGenBuffers(1, &tmp_shape.p_color);
			glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_color);
			glBufferData(GL_ARRAY_BUFFER, m_colors.size() * sizeof(GL_FLOAT), &m_colors.at(0), GL_STATIC_DRAW);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

			glGenBuffers(1, &tmp_shape.p_normal);
			glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_normal);
			glBufferData(GL_ARRAY_BUFFER, m_normals.size() * sizeof(GL_FLOAT), &m_normals.at(0), GL_STATIC_DRAW);
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

			glGenBuffers(1, &tmp_shape.p_texCoord);
			glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_texCoord);
			glBufferData(GL_ARRAY_BUFFER, m_textureCoords.size() * sizeof(GL_FLOAT), &m_textureCoords.at(0), GL_STATIC_DRAW);
			glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 0, 0);

			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);
			glEnableVertexAttribArray(2);
			glEnableVertexAttribArray(3);

			tmp_shape.material = materials[m];
			res.push_back(tmp_shape);
		}
	}

	return res;
}

void LoadTexturedModels(string model_path) {

	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	tinyobj::attrib_t attrib;
	vector<GLfloat> vertices;
	vector<GLfloat> colors;
	vector<GLfloat> normals;
	vector<GLfloat> textureCoords;
	vector<int> material_id;

	string err;
	string warn;

	string base_dir;

	base_dir = GetBaseDir(model_path);

#ifdef _WIN32
	base_dir += "\\";
#else
	base_dir += "/";
#endif

	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_path.c_str(), base_dir.c_str());

	if (!warn.empty()) {
		cout << warn << std::endl;
	}

	if (!err.empty()) {
		cerr << err << std::endl;
	}

	if (!ret) {
		exit(1);
	}

	model tmp_model;

	vector<PhongMaterial> allMaterial;
	for (int i = 0; i < materials.size(); i++) {
		PhongMaterial material;
		material.diffuseTexture = LoadTextureImage(base_dir + string(materials[i].diffuse_texname));
		if (material.diffuseTexture == -1)
		{
			cout << "LoadTexturedModels: Fail to load model's material " << i << endl;
			system("pause");

		}

		allMaterial.push_back(material);
	}

	for (int i = 0; i < shapes.size(); i++)
	{

		vertices.clear();
		colors.clear();
		normals.clear();
		textureCoords.clear();
		material_id.clear();
		normalization(&attrib, vertices, colors, normals, textureCoords, material_id, &shapes[i]);

		vector<Shape> splitedShapeByMaterial = SplitShapeByMaterial(vertices, colors, normals, textureCoords, material_id, allMaterial);

		tmp_model.shapes.insert(tmp_model.shapes.end(), splitedShapeByMaterial.begin(), splitedShapeByMaterial.end());
	}

	shapes.clear();
	materials.clear();

	models.push_back(tmp_model);
}

void initParameter()
{
	proj.nearClip = 0.001;
	proj.farClip = 1000.0;
	proj.fovy = 80;
	proj.aspect = (float)(WINDOW_WIDTH) / (float)WINDOW_HEIGHT; // adjust width for side by side view

	main_camera.position = vec3(0.0f, 0.0f, 2.0f);
	main_camera.center = vec3(0.0f, 0.0f, 0.0f);
	main_camera.up_vector = vec3(0.0f, 1.0f, 0.0f);
}

void setUniformVariables()
{
	iLocP = glGetUniformLocation(program, "um4p");
	iLocV = glGetUniformLocation(program, "um4v");
	iLocM = glGetUniformLocation(program, "um4m");
}

// OpenGL initialization
void My_Init()
{
	glClearColor(0.2, 0.2, 0.2, 1.0);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	size_t i = 0;
	setShaders();
	initParameter();
	setUniformVariables();

	for (string model_path : model_list) {
		LoadTexturedModels(model_path);
	}

	while (i < models.size()) {
		T.emplace_back(mat4(1.0f));
		R.emplace_back(mat4(1.0f));
		S.emplace_back(mat4(1.0f));
		M.emplace_back(mat4(1.0f));
		translate_model(i);
		i++;
	}
}

// GLUT callback. Called to draw the scene.
void My_Display()
{
	// Clear display buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	mat4 project_matrix;
	// perspective(fov, aspect_ratio, near_plane_distance, far_plane_distance)
	// ps. fov = field of view, it represent how much range(degree) is this camera could see 
	project_matrix = perspective(deg2rad(proj.fovy), proj.aspect, proj.nearClip, proj.farClip);

	mat4 view_matrix;
	// lookAt(camera_position, camera_viewing_vector, up_vector)
	// up_vector represent the vector which define the direction of 'up'
	view_matrix = lookAt(main_camera.position, main_camera.center, main_camera.up_vector);

	translate_model(TORSO);
	translate_model(HEAD);

	for (size_t i = 0; i < models.size(); i++) {
		transform_model(i);
		if (resume_flag) rotate_model(i);
		scale_model(i);

		glUniformMatrix4fv(iLocM, 1, GL_FALSE, value_ptr(M[i]));
		glUniformMatrix4fv(iLocV, 1, GL_FALSE, value_ptr(view_matrix));
		glUniformMatrix4fv(iLocP, 1, GL_FALSE, value_ptr(project_matrix));

		for (int j = 0; j < models[i].shapes.size(); j++) {
			glBindVertexArray(models[i].shapes[j].vao);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, models[i].shapes[j].material.diffuseTexture);
			glDrawArrays(GL_TRIANGLES, 0, models[i].shapes[j].vertex_count);
		}
	}

	if (waktu > 360) {
		waktu = 0;
	}
	else {
		if (resume_flag) waktu += 0.0100;
	}

	glutSwapBuffers();
}

// Setting up viewing matrix
void My_Reshape(int width, int height)
{
	proj.aspect = (float)(width) / (float)height;
	screenWidth = width;
	screenHeight = height;
	printf("Size changed to %d %d\n", width, height);
	glViewport(0, 0, screenWidth, screenHeight);
	
}

void My_Timer(int val)
{
	timer_cnt += 0.0f;
	glutPostRedisplay();
	if (timer_enabled)
	{
		glutTimerFunc(timer_speed, My_Timer, val);
	}
}

void My_Keyboard(unsigned char key, int x, int y)
{
	if (timer_enabled) {
		printf("Key %c is pressed at (%d, %d)\n", key, x, y);
		if (key == 'd')
		{
			models[TORSO].position.x += 1;
		}
		else if (key == 'a')
		{
			models[TORSO].position.x -= 1;
		}
		else if (key == 'w')
		{
			models[TORSO].position.y += 1;
		}
		else if (key == 's')
		{
			models[TORSO].position.y -= 1;
		}
		else if (key == 'q')
		{
			models[TORSO].position.z += 1;
		}
		else if (key == 'e')
		{
			models[TORSO].position.z -= 1;
		}
	}
	else {
		printf("System paused, keyboard callback won't work\n");
	}
}

void My_Menu(int id)
{
	switch (id)
	{
	case MENU_TIMER_START:
		if (!timer_enabled)
		{
			timer_enabled = true;
			glutTimerFunc(timer_speed, My_Timer, 0);
		}
		break;
	case MENU_TIMER_STOP:
		timer_enabled = false;
		break;
	case MENU_EXIT:
		exit(0);
		break;
	default:
		break;
	}
}

void My_Mouse(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON)
	{
		if (state == GLUT_DOWN)
		{
			printf("Mouse %d is pressed at (%d, %d)\n", button, x, y);
		}
		else if (state == GLUT_UP)
		{
			printf("Mouse %d is released at (%d, %d)\n", button, x, y);
		}
	}
}

int main(int argc, char *argv[])
{
	rotate_flag = 1;
	resume_flag = 1;
#ifdef __APPLE__
	// Change working directory to source code path
	chdir(__FILEPATH__("/../Assets/"));
#endif
	// Initialize GLUT and GLEW, then create a window.
	glutInit(&argc, argv);
#ifdef _MSC_VER
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#else
	glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	glutCreateWindow("107062361 -- Assignment 1"); // You cannot use OpenGL functions before this line;
								  // The OpenGL context must be created first by glutCreateWindow()!
#ifdef _MSC_VER
	glewInit();
#endif
	dumpInfo();
	My_Init();

	int menu_main = glutCreateMenu(My_Menu);
	int menu_timer = glutCreateMenu(My_Menu);

	glutSetMenu(menu_main);
	glutAddSubMenu("Timer", menu_timer);
	glutAddMenuEntry("Exit", MENU_EXIT);

	glutSetMenu(menu_timer);
	glutAddMenuEntry("Start", MENU_TIMER_START);
	glutAddMenuEntry("Stop", MENU_TIMER_STOP);

	glutSetMenu(menu_main);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	glutDisplayFunc(My_Display);
	glutReshapeFunc(My_Reshape);
	glutKeyboardFunc(My_Keyboard);
	glutTimerFunc(timer_speed, My_Timer, 0);

	glutMouseFunc(My_Mouse);
	glutMainLoop();

	return 0;
}