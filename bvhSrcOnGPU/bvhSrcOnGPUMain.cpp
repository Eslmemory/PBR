#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <algorithm>
#include <vector>

#include "imgui.h"
#include "imGui/imgui_impl_glfw.h"
#include "imGui/imgui_impl_opengl3.h"

#include "Shader.h"
#include "Texture.h"
#include "Camera.h"
#include "RenderPass.h"


struct Triangle {
	glm::vec3 p1, p2, p3;
	glm::vec3 n1, n2, n3;
	glm::vec3 emissive;
	glm::vec3 baseColor;
	glm::vec3 param1;
	glm::vec3 param2;
	glm::vec3 param3;
	glm::vec3 param4;
};

struct BVHNode {
	glm::vec3 child;         // left, right, ~
	glm::vec3 attribute;     // index, n, ~
	glm::vec3 AA, BB;
};

int buildBVH(std::vector<Triangle>& triangle, std::vector<BVHNode>& nodes, int left, int right, int n);
void AddTriangle(std::vector<Triangle>& triangle, std::vector<glm::vec3>& vertexArray);
void AddLight(std::vector<Triangle>& triangle, std::vector<glm::vec3>& vertexArray);
void SubmitTextureData(std::vector<Triangle>& triangle, std::vector<BVHNode>& bvhTree);
void ReDraw();

GLuint getTextureRGBA32F();

void RenderCanvas();

std::vector<glm::vec3> loadModel(const std::string& file, int& count);

bool cmpx(const Triangle& t1, const Triangle& t2);
bool cmpy(const Triangle& t1, const Triangle& t2);
bool cmpz(const Triangle& t1, const Triangle& t2);

int width = 1280, height = 640;
unsigned int dbo, tbo;
unsigned int BVHdbo, BVHtbo;
unsigned int canvasVAO;
int frameCounter = 0;

RenderPass pass1 = RenderPass();
RenderPass pass2 = RenderPass();
RenderPass pass3 = RenderPass();

int indices[] = {
		0, 1, 1, 2, 2, 3, 3, 0,
		4, 5, 5, 6, 6, 7, 7, 4,
		2, 6, 6, 7, 7, 3, 3, 2,
		1, 5, 5, 4, 4, 0, 0, 1
};

glm::vec3 lightArray[] = {

	//左
	glm::vec3(-0.5f,  0.5f, -0.5f),
	glm::vec3(-0.5f,  0.5f,  0.5f),
	glm::vec3(-0.5f, -0.5f, -0.5f),
	glm::vec3(-0.5f, -0.5f,  0.5f),
	glm::vec3(-0.5f, -0.5f, -0.5f),
	glm::vec3(-0.5f,  0.5f,  0.5f),

	//右
	glm::vec3(0.5f,  0.5f,  0.5f),
	glm::vec3(0.5f, -0.5f, -0.5f),
	glm::vec3(0.5f, -0.5f,  0.5f),
	glm::vec3(0.5f, -0.5f, -0.5f),
	glm::vec3(0.5f,  0.5f,  0.5f),
	glm::vec3(0.5f,  0.5f, -0.5f),

	//后
	glm::vec3(-0.5f, -0.5f, -0.5f),
	glm::vec3(0.5f, -0.5f, -0.5f),
	glm::vec3(-0.5f,  0.5f, -0.5f),
	glm::vec3(0.5f, -0.5f, -0.5f),
	glm::vec3(0.5f,  0.5f, -0.5f),
	glm::vec3(-0.5f,  0.5f, -0.5f),

	//下
	glm::vec3(-0.5f, -0.5f, -0.5f),
	glm::vec3(-0.5f, -0.5f,  0.5f),
	glm::vec3(0.5f, -0.5f,  0.5f),
	glm::vec3(0.5f, -0.5f,  0.5f),
	glm::vec3(0.5f, -0.5f, -0.5f),
	glm::vec3(-0.5f, -0.5f, -0.5f),

	//上
	glm::vec3(-0.5f,  0.5f, -0.5f),
	glm::vec3(0.5f,  0.5f,  0.5f),
	glm::vec3(-0.5f,  0.5f,  0.5f),
	glm::vec3(-0.5f,  0.5f, -0.5f),
	glm::vec3(0.5f,  0.5f, -0.5f),
	glm::vec3(0.5f,  0.5f,  0.5f),

	//light
	glm::vec3(-0.1f,  0.49f, -0.1f),
	glm::vec3( 0.1f,  0.49f,  0.1f),
	glm::vec3(-0.1f,  0.49f,  0.1f),
	glm::vec3(-0.1f,  0.49f, -0.1f),
	glm::vec3( 0.1f,  0.49f, -0.1f),
	glm::vec3( 0.1f,  0.49f,  0.1f)
};

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(width, height, "PBR", NULL, NULL);
	glfwMakeContextCurrent(window);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}
	glEnable(GL_DEPTH_TEST);

	Camera camera(width, height, 1.0);
	camera.SetTranslation(glm::vec3(0.0f, 0.0f, 1.0f));
	camera.SetRotation(0.0f, glm::vec3(1.0f, 0.0f, 0.0f));

	const std::string modelFile = "D:/VisualStdio/VisualStdio2022/Project/PBR/PBR/Assets/model/bunny.obj";
	std::vector<Triangle> triangleArray;
	std::vector<BVHNode> nodes;
	int vertexCount = 0;

	std::vector<glm::vec3> vertexArray = loadModel(modelFile, vertexCount);
	AddTriangle(triangleArray, vertexArray);

	std::vector<glm::vec3> lightVector;
	for (int i = 0; i < (sizeof(lightArray) / sizeof(glm::vec3)); ++i)  // (sizeof(lightArray) / sizeof(glm::vec3))
	{
		lightVector.push_back(lightArray[i]);
	}
	AddLight(triangleArray, lightVector);

	buildBVH(triangleArray, nodes, 0, triangleArray.size() - 1, 10);
	SubmitTextureData(triangleArray, nodes);

	const char* vertexShader = "D:/VisualStdio/VisualStdio2022/Project/PBR/PBR/bvhSrcOnGPU/shader/bvhSrcOnGPU.vs";
	const char* fragmentShader = "D:/VisualStdio/VisualStdio2022/Project/PBR/PBR/bvhSrcOnGPU/shader/bvhSrcOnGPU.fs";
	Shader pass1Shader(vertexShader, fragmentShader);

	const char* pass2FragmentShader = "D:/VisualStdio/VisualStdio2022/Project/PBR/PBR/bvhSrcOnGPU/shader/pass2.fs";
	Shader pass2Shader(vertexShader, pass2FragmentShader);
	
	const char* pass3FragmentShader = "D:/VisualStdio/VisualStdio2022/Project/PBR/PBR/bvhSrcOnGPU/shader/pass3.fs";
	Shader pass3Shader(vertexShader, pass3FragmentShader);

	pass1.setShader(pass1Shader);
	pass2.setShader(pass2Shader);
	pass3.setShader(pass3Shader);

	pass1.colorAttachment.push_back(getTextureRGBA32F());
	pass1.bindData(false);

	GLuint lastFrame = getTextureRGBA32F();
	pass2.colorAttachment.push_back(lastFrame);
	pass2.bindData(false);

	pass3.bindData(true);

	std::string hdrMapFile = "D:/VisualStdio/VisualStdio2022/Project/PBR/PBR/Assets/texture/circus_arena/studio_garden_4k.hdr";
	Texture hdrMap(hdrMapFile);

	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");
	
	float previous = glfwGetTime();
	float yaw = 0.0f;
	while (!glfwWindowShouldClose(window))
	{
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		frameCounter++;
		float now = glfwGetTime();
		float deltaTime = now - previous;
		previous = now;

		ImGui::Begin("fps");
		ImGui::Text("fps:%f", 1.0 / deltaTime);
		ImGui::End();

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		pass1.m_Shader.use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_BUFFER, tbo);
		pass1.m_Shader.setInt("triangleTexture", 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_BUFFER, BVHtbo);
		pass1.m_Shader.setInt("BVHTexture", 1);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, lastFrame);
		pass1.m_Shader.setInt("lastFrame", 2);

		hdrMap.bind(3);
		pass1.m_Shader.setInt("hdrMap", 3);

		pass1.m_Shader.setInt("triangleNum", (int)(vertexArray.size() / 3));
		pass1.m_Shader.setInt("NodeNum", (int)(nodes.size()));
		pass1.m_Shader.setInt("width", width);
		pass1.m_Shader.setInt("height", height);
		pass1.m_Shader.setInt("frameCounter", frameCounter);

		// pass1.m_Shader.setMat4("ViewProjection", camera.GetViewProjectionMatrix());
		pass1.m_Shader.setVec3("camera.bottomLeft", glm::vec3(-2.0f, -1.0f, -1.0f));
		pass1.m_Shader.setVec3("camera.width", glm::vec3(4.0f, 0.0f, 0.0f));
		pass1.m_Shader.setVec3("camera.height", glm::vec3(0.0f, 2.0f, 0.0f));
		pass1.m_Shader.setVec3("camera.origin", glm::vec3(0.0f, 0.0f, 1.0f));

		glm::mat4 rotation(1.0f);
		// rotation = glm::lookAt(glm::vec3(0.0f, 0.0f, 1.0f), cameraFront, glm::vec3(0.0f, 1.0f, 0.0f));
		// yaw += deltaTime * 1.0f;
		rotation = glm::rotate(rotation, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		pass1.m_Shader.setMat4("cameraRotation", rotation);

		pass1.draw();
		pass2.draw(pass1.colorAttachment);
		pass3.draw(pass2.colorAttachment);

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwPollEvents();
		glfwSwapBuffers(window);
	}
}

void RenderCanvas()
{
	if (canvasVAO == 0)
	{
		float vertices[] = {
				 1.0f,  1.0f, 0.0f,  // top right
				 1.0f, -1.0f, 0.0f,  // bottom right
				-1.0f, -1.0f, 0.0f,  // bottom left
				-1.0f,  1.0f, 0.0f   // top left 
		};
		unsigned int indices[] = {  // note that we start from 0!
			0, 1, 3,   // first triangle
			1, 2, 3    // second triangle
		};

		unsigned int VBO, EBO;
		glGenVertexArrays(1, &canvasVAO);
		glBindVertexArray(canvasVAO);

		glGenBuffers(1, &VBO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);

		glGenBuffers(1, &EBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), &indices, GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	}
	glBindVertexArray(canvasVAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}


void SubmitTextureData(std::vector<Triangle>& triangle, std::vector<BVHNode>& bvhTree)
{
	if (dbo == 0)
	{
		glGenBuffers(1, &dbo);
		glBindBuffer(GL_TEXTURE_BUFFER, dbo);
		glBufferData(GL_TEXTURE_BUFFER, sizeof(Triangle) * triangle.size(), &triangle[0], GL_STATIC_DRAW);

		glGenTextures(1, &tbo);
		glBindTexture(GL_TEXTURE_BUFFER, tbo);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, dbo);
	}
	
	if (BVHdbo == 0)
	{
		glGenBuffers(1, &BVHdbo);
		glBindBuffer(GL_TEXTURE_BUFFER, BVHdbo);
		glBufferData(GL_TEXTURE_BUFFER, sizeof(BVHNode) * bvhTree.size(), &bvhTree[0], GL_STATIC_DRAW);

		glGenTextures(1, &BVHtbo);
		glBindTexture(GL_TEXTURE_BUFFER, BVHtbo);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, BVHdbo);
	}
}

void AddTriangle(std::vector<Triangle>& triangle, std::vector<glm::vec3>& vertexArray)
{

	for (int i = 0; i < vertexArray.size(); i = i + 3)
	{
		Triangle t;
		t.p1 = vertexArray[i];
		t.p2 = vertexArray[i + 1];
		t.p3 = vertexArray[i + 2];

		t.n1 = glm::vec3(-1.0f, -1.0f, 0.0f);
		t.n2 = glm::vec3(1.0f, -1.0f, 0.0f);
		t.n3 = glm::vec3(0.0f, 1.0f, 0.0f);

		t.emissive = glm::vec3(0.0f, 0.0f, 0.0f);
		t.baseColor = glm::vec3(1.0f, 0.0f, 0.0f);

		t.param1 = glm::vec3(-1.0f, -1.0f, 0.0f);
		t.param2 = glm::vec3(-1.0f, -1.0f, 0.0f);
		t.param3 = glm::vec3(-1.0f, -1.0f, 0.0f);
		t.param4 = glm::vec3(-1.0f, -1.0f, 0.0f);

		triangle.push_back(t);
	}
}

void AddLight(std::vector<Triangle>& triangle, std::vector<glm::vec3>& vertexArray)
{
	for (int i = 0; i < vertexArray.size(); i = i + 3)
	{
		Triangle t;
		t.p1 = vertexArray[i];
		t.p2 = vertexArray[i + 1];
		t.p3 = vertexArray[i + 2];

		if (i >= 0 && i < 6)
		{
			// 左
			t.emissive = glm::vec3(0.0f, 0.0f, 0.0f);
			t.baseColor = glm::vec3(1.0f, 0.0f, 0.0f);
			t.n1 = glm::vec3(1.0f, 0.0f, 0.0f);
			t.n2 = glm::vec3(1.0f, 0.0f, 0.0f);
			t.n3 = glm::vec3(1.0f, 0.0f, 0.0f);
		}
		else if (i >= 6 && i < 12)
		{
			// 右
			t.emissive = glm::vec3(0.0f, 0.0f, 0.0f);
			t.baseColor = glm::vec3(0.0f, 1.0f, 0.0f);
			t.n1 = glm::vec3(-1.0f, 0.0f, 0.0f);
			t.n2 = glm::vec3(-1.0f, 0.0f, 0.0f);
			t.n3 = glm::vec3(-1.0f, 0.0f, 0.0f);
		}
		else if (i >= 12 && i < 18)
		{
			// 后
			t.emissive = glm::vec3(0.0f, 0.0f, 0.0f);
			t.baseColor = glm::vec3(1.0f, 1.0f, 1.0f);
			t.n1 = glm::vec3(0.0f, 0.0f, 1.0f);
			t.n2 = glm::vec3(0.0f, 0.0f, 1.0f);
			t.n3 = glm::vec3(0.0f, 0.0f, 1.0f);
		}
		else if (i >= 18 && i < 24)
		{
			// 下
			t.emissive = glm::vec3(0.0f, 0.0f, 0.0f);
			t.baseColor = glm::vec3(1.0f, 1.0f, 1.0f);
			t.n1 = glm::vec3(0.0f, 1.0f, 0.0f);
			t.n2 = glm::vec3(0.0f, 1.0f, 0.0f);
			t.n3 = glm::vec3(0.0f, 1.0f, 0.0f);
		}
		else if (i >= 24 && i < 30)
		{
			// 上
			t.emissive = glm::vec3(0.0f, 0.0f, 0.0f);
			t.baseColor = glm::vec3(1.0f, 1.0f, 1.0f);
			t.n1 = glm::vec3(0.0f, -1.0f, 0.0f);
			t.n2 = glm::vec3(0.0f, -1.0f, 0.0f);
			t.n3 = glm::vec3(0.0f, -1.0f, 0.0f);
		}
		else if (i >= 30)
		{
			// light
			t.emissive = glm::vec3(20.0f, 20.0f, 20.0f);
			t.baseColor = glm::vec3(1.0f, 1.0f, 1.0f);
			t.n1 = glm::vec3(0.0f, -1.0f, 0.0f);
			t.n2 = glm::vec3(0.0f, -1.0f, 0.0f);
			t.n3 = glm::vec3(0.0f, -1.0f, 0.0f);
		}

		t.param1 = glm::vec3(-1.0f, -1.0f, 0.0f);
		t.param2 = glm::vec3(-1.0f, -1.0f, 0.0f);
		t.param3 = glm::vec3(-1.0f, -1.0f, 0.0f);
		t.param4 = glm::vec3(-1.0f, -1.0f, 0.0f);

		triangle.push_back(t);
	}
}

std::vector<glm::vec3> loadModel(const std::string& file, int& count)
{
	std::ifstream in(file);
	std::string line = "";
	std::vector<glm::vec3> vertex;

	std::vector<glm::vec3> position;
	std::vector<glm::vec3> normal;
	std::vector<glm::vec2> uv;
	std::vector<glm::ivec3> positionIndex;
	std::vector<glm::ivec3> normalIndex;
	std::vector<glm::ivec3> uvIndex;

	if (!in.is_open())
	{
		std::cout << "文件打开失败" << std::endl;
		exit(-1);
	}

	while (std::getline(in, line))
	{
		std::stringstream ss(line);
		std::string type;
		float x, y, z;
		int v0, vt0, vn0;
		int v1, vt1, vn1;
		int v2, vt2, vn2;

		ss >> type;
		if (type == "v")
		{
			ss >> x >> y >> z;
			position.push_back(glm::vec3(x, y, z));
		}
		if (type == "f")
		{
			ss >> v0 >> v1 >> v2;
			positionIndex.push_back(glm::ivec3(v0 - 1, v1 - 1, v2 - 1));
		}
	}

	for (int i = 0; i < positionIndex.size(); i++)
	{
		for (int j = 0; j < 3; ++j)
		{
			glm::vec3 temp = position[positionIndex[i][j]];
			vertex.push_back(temp);
			count++;
		}
	}
	return vertex;
}

void buildBox(std::vector<glm::vec3>& box, glm::vec3& AA, glm::vec3& BB)
{
	if (true)  // 限制盒子数量
	{
		std::vector<glm::vec3> temp;
		temp.push_back(glm::vec3(AA.x, AA.y, AA.z));
		temp.push_back(glm::vec3(BB.x, AA.y, AA.z));
		temp.push_back(glm::vec3(BB.x, BB.y, AA.z));
		temp.push_back(glm::vec3(AA.x, BB.y, AA.z));
		temp.push_back(glm::vec3(AA.x, AA.y, BB.z));
		temp.push_back(glm::vec3(BB.x, AA.y, BB.z));
		temp.push_back(glm::vec3(BB.x, BB.y, BB.z));
		temp.push_back(glm::vec3(AA.x, BB.y, BB.z));
		for (int i = 0; i < sizeof(indices) / sizeof(int); ++i)
			box.push_back(temp[indices[i]]);
	}
}

int buildBVH(std::vector<Triangle>& triangle, std::vector<BVHNode>& nodes, int left, int right, int n)
{
	if (left > right)
		return 0;

	nodes.push_back(BVHNode());
	int id = nodes.size() - 1;

	nodes[id].AA = glm::vec3(10000.0f, 10000.0f, 10000.0f);
	nodes[id].BB = glm::vec3(-10000.0f, -10000.0f, -10000.0f);
	nodes[id].child = glm::vec3(-1.0f, -1.0f, -1.0f);
	nodes[id].attribute = glm::vec3(-1.0f, -1.0f, -1.0f);

	for (int i = left; i <= right; ++i)
	{
		float minx = std::min(triangle[i].p1.x, std::min(triangle[i].p2.x, triangle[i].p3.x));
		float miny = std::min(triangle[i].p1.y, std::min(triangle[i].p2.y, triangle[i].p3.y));
		float minz = std::min(triangle[i].p1.z, std::min(triangle[i].p2.z, triangle[i].p3.z));
		nodes[id].AA.x = std::min(nodes[id].AA.x, minx);
		nodes[id].AA.y = std::min(nodes[id].AA.y, miny);
		nodes[id].AA.z = std::min(nodes[id].AA.z, minz);

		float maxx = std::max(triangle[i].p1.x, std::max(triangle[i].p2.x, triangle[i].p3.x));
		float maxy = std::max(triangle[i].p1.y, std::max(triangle[i].p2.y, triangle[i].p3.y));
		float maxz = std::max(triangle[i].p1.z, std::max(triangle[i].p2.z, triangle[i].p3.z));
		nodes[id].BB.x = std::max(nodes[id].BB.x, maxx);
		nodes[id].BB.y = std::max(nodes[id].BB.y, maxy);
		nodes[id].BB.z = std::max(nodes[id].BB.z, maxz);
	}

	if ((right - left + 1) <= n)
	{
		std::vector<glm::vec3> box;
		// buildBox(box, nodes[id].AA, nodes[id].BB);
		nodes[id].attribute[0] = left;
		nodes[id].attribute[1] = right - left + 1;
		return id;
	}

	float lenx = nodes[id].BB.x - nodes[id].AA.x;
	float leny = nodes[id].BB.y - nodes[id].AA.y;
	float lenz = nodes[id].BB.z - nodes[id].AA.z;

	if (lenx >= leny && lenx >= lenz)
		std::sort(triangle.begin() + left, triangle.begin() + right + 1, cmpx);
	if (leny >= lenx && leny >= lenz)
		std::sort(triangle.begin() + left, triangle.begin() + right + 1, cmpy);
	if (lenz >= lenx && lenz >= leny)
		std::sort(triangle.begin() + left, triangle.begin() + right + 1, cmpz);

	int mid = (left + right) / 2;
	nodes[id].child[0] = buildBVH(triangle, nodes, left, mid, n);
	nodes[id].child[1] = buildBVH(triangle, nodes, mid + 1, right, n);

	return id;
}

GLuint getTextureRGBA32F()
{
	unsigned int tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 1280, 640, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	return tex;
}

bool cmpx(const Triangle& t1, const Triangle& t2)
{
	return (t1.p1.x + t1.p2.x + t1.p3.x) < (t2.p1.x + t2.p2.x + t2.p3.x);
}

bool cmpy(const Triangle& t1, const Triangle& t2)
{
	return (t1.p1.y + t1.p2.y + t1.p3.y) < (t2.p1.y + t2.p2.y + t2.p3.y);
}

bool cmpz(const Triangle& t1, const Triangle& t2)
{
	return (t1.p1.z + t1.p2.z + t1.p3.z) < (t2.p1.z + t2.p2.z + t2.p3.z);
}

void ReDraw()
{
	pass1.colorAttachment[0] = getTextureRGBA32F();
	pass1.bindData(false);

	GLuint lastFrame = getTextureRGBA32F();
	pass2.colorAttachment[0] = lastFrame;
	pass2.bindData(false);

	pass3.bindData(true);
	frameCounter = 0;
}


