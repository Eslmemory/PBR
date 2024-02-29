#ifdef PBR_TEST

#include "Shader.h"
#include "Texture.h"

#include "imgui.h"
#include "imGui/imgui_impl_glfw.h"
#include "imGui/imgui_impl_opengl3.h"
#include <vector>

#define PI 3.1415926

void renderSphere();

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

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

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1280.0f / 640.f, 0.1f, 100.0f);
    glm::mat4 model(1.0f);
    glm::mat4 view(1.0f);
    view = glm::translate(view, glm::vec3(0.0f, 0.0f, -5.0f));

    const char* vertexShader = "D:/VisualStdio/VisualStdio2022/Project/PBR/PBR/pbrSrc/shader/vertex.vs";
    const char* fragmentShader = "D:/VisualStdio/VisualStdio2022/Project/PBR/PBR/pbrSrc/shader/fragment.fs";
    const std::string albedoMapFile = "D:/Unity/ModelSrc/Model/hair/Avatar_Girl_Sword_Keqing_Tex_Hair_Diffuse.png";
    const std::string normalMapFile = "D:/Unity/ModelSrc/Model/hair/Avatar_Girl_Sword_Keqing_Tex_Hair_Diffuse.png";
    const std::string metallicMapFile = "D:/Unity/ModelSrc/Model/hair/Avatar_Girl_Sword_Keqing_Tex_Hair_Diffuse.png";
    const std::string roughnessMapFile = "D:/Unity/ModelSrc/Model/hair/Avatar_Girl_Sword_Keqing_Tex_Hair_Diffuse.png";
    const std::string aoMapFile = "D:/Unity/ModelSrc/Model/hair/Avatar_Girl_Sword_Keqing_Tex_Hair_Diffuse.png";

    float roughness = 0.0f;
    float metallic = 0.0f;

    Texture albedoMap(albedoMapFile);
    albedoMap.bind(1);
    Texture normalMap(normalMapFile);
    albedoMap.bind(2);
    Texture metallicMap(metallicMapFile);
    albedoMap.bind(3);
    Texture roughnessMap(roughnessMapFile);
    albedoMap.bind(4);
    Texture aoMap(aoMapFile);
    albedoMap.bind(5);
     
	Shader shader(vertexShader, fragmentShader);
    shader.use();

    // texture slot
    shader.setInt("albedoMap", 1);
    shader.setInt("normalMap", 2);
    shader.setInt("metallicMap", 3);
    shader.setInt("roughnessMap", 4);
    shader.setInt("aoMap", 5);

    shader.setMat4("view", view);
    shader.setMat4("projection", projection);
    shader.setMat4("model", model);
    shader.setVec3("camPos", glm::vec3(0.0f, 0.0f, 5.0f));

    for (unsigned int i = 0; i < 1; ++i)
    {
        shader.setVec3("lightPosition[" + std::to_string(i) + "]", glm::vec3(10.0f, 10.0f, 10.0f));
        shader.setVec3("lightColor[" + std::to_string(i) + "]", glm::vec3(300.0f, 300.0f, 300.0f));
    }

    // imgui initialize
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    while (!glfwWindowShouldClose(window))
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui::Begin("param");
        ImGui::SliderFloat("roughness", &roughness, 0.0f, 1.0f);
        ImGui::SliderFloat("metallic", &metallic, 0.0f, 1.0f);
        ImGui::End();

        shader.setFloat("roughness", roughness);
        shader.setFloat("metallic", metallic);

        shader.use();
        renderSphere();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

unsigned int VAO;

void renderSphere()
{
    uint32_t XSegment = 64, YSegment = 64;
    
    std::vector<Vertex> data;
    
    static uint32_t indexCount = 0;
    
    if (VAO == 0)
    {
        for (uint32_t y = 0; y <= YSegment; ++y)
        {
            for (uint32_t x = 0; x <= XSegment; ++x)
            {
                float xsegment = float(x) / float(XSegment);
                float ysegment = float(y) / float(YSegment);
                float xpos = std::cos(xsegment * 2.0f * PI) * std::sin(ysegment * PI);
                float ypos = std::cos(ysegment * PI);
                float zpos = std::sin(xsegment * 2.0f * PI) * std::sin(ysegment * PI);

                glm::vec3 position(xpos, ypos, zpos);
                glm::vec3 normal(xpos, ypos, zpos);
                glm::vec2 uv(xsegment, ysegment);
            
                data.push_back(Vertex({ position, normal, uv }));
            }
        }

        bool inverse = false;
        std::vector<uint32_t> indices;
        for (uint32_t y = 0; y < YSegment; ++y)
        {
            if (!inverse)
            {
                for (int x = 0; x <= XSegment; ++x)
                {
                    indices.push_back(y * (XSegment + 1) + x);
                    indices.push_back((y + 1) * (XSegment + 1) + x);
                }
            }
            else
            {
                for (int x = XSegment; x >= 0; --x)
                {
                    indices.push_back((y + 1) * (XSegment + 1) + x);
                    indices.push_back(y * (XSegment + 1) + x);
                }
            }
            inverse = !inverse;
        }

        indexCount = indices.size();
        
        unsigned int VBO, EBO;
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(Vertex), &data[0], GL_STATIC_DRAW);
    
        glGenBuffers(1, &EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), &indices[0], GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), (void*)offsetof(Vertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(Vertex), (void*)offsetof(Vertex, uv));
        
    }

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
}

#endif