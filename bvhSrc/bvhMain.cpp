#ifdef bvhTEST
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <algorithm>

#include "Shader.h"

#define INF -100000

struct Vertex {
	glm::vec3 position;
};

struct Triangle {
	glm::vec3 p1, p2, p3;
	glm::vec3 centor;
	Triangle(glm::vec3 a, glm::vec3 b, glm::vec3 c)
		: p1(a), p2(b), p3(c)
	{
		centor = (a + b + c) / 3.0f;
	}
};

struct BVHNode {
	BVHNode* left = nullptr;
	BVHNode* right = nullptr;
	glm::vec3 AA, BB;
	int n, index;
};

struct Ray {
	glm::vec3 dir;
	glm::vec3 origin;
};

struct HitInfo {
	float distance;
	Triangle triangle;
	HitInfo(float d, Triangle tri)
		: distance(d), triangle(tri)
	{
	}
};

void Renderbunny(std::vector<Vertex>& vertices, int vertexCount);
void RenderBox(std::vector<glm::vec3>& box);
bool cmpx(const Triangle& t1, const Triangle& t2);
bool cmpy(const Triangle& t1, const Triangle& t2);
bool cmpz(const Triangle& t1, const Triangle& t2);
BVHNode* buildBVH(std::vector<Triangle>& triangles, int l, int r, int n);
float hitAABB(Ray r, glm::vec3 AA, glm::vec3 BB);
float hitTriangle(Triangle& triangle, Ray ray);
HitInfo hitTriangleArray(Ray ray, std::vector<Triangle>& triangles, int left, int right);
HitInfo hitBVH(BVHNode* node, std::vector<Triangle>& triangles, Ray ray);

std::vector<Triangle> triangle;
std::vector<glm::vec3> box;
unsigned int bunnyVAO, boxVAO;
int maxBoxNum = 0;
int indices[] = {
		0, 1, 1, 2, 2, 3, 3, 0,
		4, 5, 5, 6, 6, 7, 7, 4,
		2, 6, 6, 7, 7, 3, 3, 2,
		1, 5, 5, 4, 4, 0, 0, 1
};

std::vector<Vertex> loadModel(const std::string& file, int& count)
{
	std::ifstream in(file);
	std::string line = "";
	std::vector<Vertex> vertex;

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
			Vertex temp;
			temp.position = position[positionIndex[i][j]];
			vertex.push_back(temp);
			// std::cout << temp.position[0] << "  " << temp.position[1] << "  " << temp.position[2] << std::endl;
			count++;
		}
		triangle.push_back(Triangle(position[positionIndex[i][0]], position[positionIndex[i][1]], position[positionIndex[i][2]]));
	}

	return vertex;
}

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(1280, 640, "PBR", NULL, NULL);
	glfwMakeContextCurrent(window);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	// glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	int vertexCount = 0;
	const std::string modelFile = "D:/VisualStdio/VisualStdio2022/Project/PBR/PBR/Assets/model/bunny.obj";
	std::vector<Vertex> vertex = loadModel(modelFile, vertexCount);

	const char* vertexShader = "D:/VisualStdio/VisualStdio2022/Project/PBR/PBR/bvhSrc/shader/bvhVertex.vs";
	const char* fragmentShader = "D:/VisualStdio/VisualStdio2022/Project/PBR/PBR/bvhSrc/shader/bvhFragment.fs";
	Shader bvhShader(vertexShader, fragmentShader);

	const char* boxVertexShader = "D:/VisualStdio/VisualStdio2022/Project/PBR/PBR/bvhSrc/shader/boxVertex.vs";
	const char* boxFragmentShader = "D:/VisualStdio/VisualStdio2022/Project/PBR/PBR/bvhSrc/shader/boxFragment.fs";
	Shader boxShader(boxVertexShader, boxFragmentShader);

	glm::mat4 model = glm::mat4(1.0f);
	glm::mat4 view = glm::mat4(1.0f);
	model = glm::scale(model, glm::vec3(3.0f, 3.0f, 3.0f));
	view = glm::lookAt(glm::vec3(-0.2f, 0.2f, 0.5f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	// glm::mat4 projection = glm::perspective(glm::radians(90.0f), 0.5f, 0.1f, 10.0f);
	glm::mat4 projection = glm::ortho(-2.0f, 2.0f, -1.0f, 1.0f, -1.0f, 1.0f);
	buildBVH(triangle, 0, triangle.size() - 1, 100);

	while (!glfwWindowShouldClose(window))
	{
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		bvhShader.use();
		bvhShader.setMat4("model", model);
		bvhShader.setMat4("view", view);
		bvhShader.setMat4("projection", projection);
		Renderbunny(vertex, vertexCount);

		boxShader.use();
		boxShader.setMat4("model", model);
		boxShader.setMat4("view", view);
		boxShader.setMat4("projection", projection);
		RenderBox(box);
		
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

}

void Renderbunny(std::vector<Vertex>& vertices, int vertexCount)
{
	// initialize (if necessary)
	if (bunnyVAO == 0)
	{
		unsigned int bunnyVBO;
		glGenVertexArrays(1, &bunnyVAO);
		glGenBuffers(1, &bunnyVBO);
		// fill buffer
		glBindBuffer(GL_ARRAY_BUFFER, bunnyVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertexCount, &vertices[0], GL_STATIC_DRAW);
		// link vertex attributes
		glBindVertexArray(bunnyVAO);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)offsetof(Vertex, position));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	// render Cube
	glBindVertexArray(bunnyVAO);
	glDrawArrays(GL_POINTS, 0, vertexCount);
	glBindVertexArray(0);
}

void RenderBox(std::vector<glm::vec3>& box)
{
	if (boxVAO == 0)
	{
		unsigned int boxVBO;
		glGenVertexArrays(1, &boxVAO);
		glGenBuffers(1, &boxVBO);
		// fill buffer
		glBindBuffer(GL_ARRAY_BUFFER, boxVBO);
		glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(float) * box.size(), &box[0], GL_STATIC_DRAW);
		// link vertex attributes
		glBindVertexArray(boxVAO);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	// render Cube
	glBindVertexArray(boxVAO);
	glDrawArrays(GL_LINES, 0, 3 * box.size());
	glBindVertexArray(0);
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
	maxBoxNum++;
}

BVHNode* buildBVH(std::vector<Triangle>& triangle, int left, int right, int n)
{
	if (left > right)
		return 0;

	BVHNode* node = new BVHNode();
	node->AA = vec3(100000.0f, 100000.0f, 100000.0f);
	node->BB = vec3(-100000.0f, -100000.0f, -100000.0f);
	for (int i = left; i <= right; ++i)
	{
		float minx = std::min(triangle[i].p1.x, std::min(triangle[i].p2.x, triangle[i].p3.x));
		float miny = std::min(triangle[i].p1.y, std::min(triangle[i].p2.y, triangle[i].p3.y));
		float minz = std::min(triangle[i].p1.z, std::min(triangle[i].p2.z, triangle[i].p3.z));
		node->AA.x = std::min(node->AA.x, minx);
		node->AA.y = std::min(node->AA.y, miny);
		node->AA.z = std::min(node->AA.z, minz);

		float maxx = std::max(triangle[i].p1.x, std::max(triangle[i].p2.x, triangle[i].p3.x));
		float maxy = std::max(triangle[i].p1.y, std::max(triangle[i].p2.y, triangle[i].p3.y));
		float maxz = std::max(triangle[i].p1.z, std::max(triangle[i].p2.z, triangle[i].p3.z));
		node->BB.x = std::max(node->BB.x, maxx);
		node->BB.y = std::max(node->BB.y, maxy);
		node->BB.z = std::max(node->BB.z, maxz);
	}

	if ((right - left + 1) <= n)
	{
		buildBox(box, node->AA, node->BB);
		node->index = left;
		node->n = right - left + 1;
		return node;
	}

	float lenx = node->BB.x - node->AA.x;
	float leny = node->BB.y - node->AA.y;
	float lenz = node->BB.z - node->AA.z;

	if (lenx >= leny && lenx >= lenz)
		std::sort(triangle.begin() + left, triangle.begin() + right + 1, cmpx);
	if (leny >= lenx && leny >= lenz)
		std::sort(triangle.begin() + left, triangle.begin() + right + 1, cmpy);
	if (lenz >= lenx && lenz >= leny)
		std::sort(triangle.begin() + left, triangle.begin() + right + 1, cmpz);

	int mid = (left + right) / 2;
	node->left = buildBVH(triangle, left, mid, n);
	node->right = buildBVH(triangle, mid + 1, right, n);

	return node;
}


float hitAABB(Ray r, glm::vec3 AA, glm::vec3 BB)
{
	glm::vec3 invDir = glm::vec3(1.0 / r.dir.x, 1.0 / r.dir.y, 1.0 / r.dir.z);
	glm::vec3 in = (AA - r.origin)* invDir;
	glm::vec3 out = (BB - r.origin)* invDir;

	glm::vec3 tmax = max(in, out);
	glm::vec3 tmin = min(in, out);

	float t1 = std::min(tmax.x, std::min(tmax.y, tmax.z));
	float t0 = std::max(tmin.x, std::max(tmin.y, tmin.z));

	return (t1 >= t0) ? ((t0 > 0.0f) ? t0 : t1) : -1;
}


float hitTriangle(Triangle& triangle, Ray ray)
{
	glm::vec3 p1 = triangle.p1;
	glm::vec3 p2 = triangle.p2;
	glm::vec3 p3 = triangle.p3;

	glm::vec3 N = glm::normalize(glm::cross(p1 - p2, p2 - p3));
	if (glm::dot(N, ray.dir) < 0.0)
		N = -N;

	float t = glm::dot(p1 - ray.origin, N) / glm::dot(ray.dir, N);
	glm::vec3 hitPoint = ray.origin + t * ray.dir;

	glm::vec3 c1 = glm::cross(p2 - p1, hitPoint - p1);
	glm::vec3 c2 = glm::cross(p3 - p2, hitPoint - p2);
	glm::vec3 c3 = glm::cross(p1 - p3, hitPoint - p3);

	if (glm::dot(c1, N) > 0.0f && glm::dot(c2, N) > 0.0f && glm::dot(c3, N) > 0.0f) return t;
	if (glm::dot(c1, N) < 0.0f && glm::dot(c2, N) < 0.0f && glm::dot(c3, N) < 0.0f) return t;

	return INF;
}

HitInfo hitTriangleArray(Ray ray, std::vector<Triangle>& triangles, int left, int right)
{
	HitInfo hitinfo(INF, { glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f) });
	for (int i = left; i <= right; ++i)
	{
		float t = hitTriangle(triangles[i], ray);
		if (t != INF && t < hitinfo.distance)
		{
			hitinfo.distance = t;
			hitinfo.triangle = triangles[i];
		}
	}

	return hitinfo;
}

HitInfo hitBVH(BVHNode* node, std::vector<Triangle>& triangles, Ray ray)
{
	if (node == NULL) return HitInfo(INF, { glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f) });

	if (node->n != 0)
		return hitTriangleArray(ray, triangles, node->index, node->index + node->n - 1);

	float d1, d2;
	if (node->left)
		d1 = hitAABB(ray, node->left->AA, node->left->BB);
	if (node->right)
		d2 = hitAABB(ray, node->right->AA, node->right->BB);

	HitInfo h1(INF, { glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f) });
	HitInfo h2(INF, { glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f) });
	if (d1 != -1)
		h1 = hitBVH(node->left, triangles, ray);
	if (d2 != -1)
		h2 = hitBVH(node->right, triangles, ray);

	return (h1.distance < h2.distance) ? h1 : h2;
}



bool cmpx(const Triangle& t1, const Triangle& t2)
{
	return t1.centor.x < t2.centor.x;
}

bool cmpy(const Triangle& t1, const Triangle& t2)
{
	return t1.centor.y < t2.centor.y;
}

bool cmpz(const Triangle& t1, const Triangle& t2)
{
	return t1.centor.z < t2.centor.z;
}
#endif
