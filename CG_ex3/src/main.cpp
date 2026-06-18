//--OpenGL相关库--
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

//--自定义工具类--
#include "mygl/MyInit.h"
#include "mygl/MyCamera.h"
#include "mygl/MyShader.h"
#include "mygl/MyData.h"

//--C++标准库--
#include <iostream>
#include <cmath>
#include <vector>

//--场景数据结构--
struct Sphere
{
    glm::vec3 center;
    float radius;
    glm::vec3 color;
    int materialType; // 0:DIFFUSE_AND_GLOSSY, 1:REFLECTION_AND_REFRACTION, 2:REFLECTION
    float ior; //折射率
};

struct Triangle
{
    glm::vec3 v0, v1, v2;
    glm::vec3 color;
    int materialType;
    float ior;
};

struct Light
{
    glm::vec3 position;
    float intensity;
};

//--函数声明--
//回调函数
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
//其他函数
void processInput(GLFWwindow *window);

//--全局设置--
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

//--摄像机设置--

//--主函数--
int main(int argc, char *argv[])
{
    //--初始化--
    MyInit myInit(SCR_WIDTH, SCR_HEIGHT, "Third Experiment");
    myInit.SetFramebufferSizeCallback(framebuffer_size_callback);

    //--着色器--
    MyShader myShader("src/vertex_shader.glsl", "src/fragment_shader.glsl");

    //--准备场景数据--
    std::vector<Sphere> spheres;
    std::vector<Triangle> triangles;
    std::vector<Light> lights;

    //球体1
    Sphere sphere1;
    sphere1.center = glm::vec3(-1.0f, 0.0f, -12.0f);
    sphere1.radius = 2.0f;
    sphere1.color = glm::vec3(0.6f, 0.7f, 0.8f);
    sphere1.materialType = 0; //DIFFUSE_AND_GLOSSY
    sphere1.ior = 1.0f;
    spheres.push_back(sphere1);

    //球体2
    Sphere sphere2;
    sphere2.center = glm::vec3(0.5f, -0.5f, -8.0f);
    sphere2.radius = 1.5f;
    sphere2.color = glm::vec3(1.0f, 1.0f, 1.0f);
    sphere2.materialType = 1; //REFLECTION_AND_REFRACTION
    sphere2.ior = 1.5f;
    spheres.push_back(sphere2);

    //地面网格
    glm::vec3 verts[4] = 
    {
        {-5.0f, -3.0f, -6.0f},
        { 5.0f, -3.0f, -6.0f},
        { 5.0f, -3.0f,-16.0f},
        {-5.0f, -3.0f,-16.0f}
    };
    //三角形1
    Triangle triangle1;
    triangle1.v0 = verts[0];
    triangle1.v1 = verts[1];
    triangle1.v2 = verts[3];
    triangle1.color = glm::vec3(0.8f, 0.0f, 0.0f);
    triangle1.materialType = 0;   // DIFFUSE
    triangle1.ior = 1.0f;
    triangles.push_back(triangle1);
    // 三角形2
    Triangle triangle2;
    triangle2.v0 = verts[1];
    triangle2.v1 = verts[2];
    triangle2.v2 = verts[3];
    triangle2.color = glm::vec3(0.0f, 0.8f, 0.0f);
    triangle2.materialType = 0;
    triangle2.ior = 1.0f;
    triangles.push_back(triangle2);

    //光源
    Light light1, light2;
    light1.position = glm::vec3(-20.0f, 70.0f, 20.0f);
    light1.intensity = 0.5f;
    light2.position = glm::vec3(30.0f, 50.0f, -12.0f);
    light2.intensity = 0.5f;
    lights.push_back(light1);
    lights.push_back(light2);

    //--VAO/VBO数据准备--
    //覆盖全屏的四边形
    float vertices[] =
    {
        -1.0f, -1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,
        1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f,
        1.0f,  1.0f, 0.0f
    };

    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    //--场景数据同步--
    myShader.use();
    //传递unifor数组
    //球体
    for(int i = 0; i < spheres.size(); i++)
    {
        std::string base = "spheres[" + std::to_string(i) +"].";
        myShader.setVec3(base + "center", spheres[i].center);
        myShader.setFloat(base + "radius", spheres[i].radius);
        myShader.setVec3(base + "color", spheres[i].color);
        myShader.setInt(base + "materialType", spheres[i].materialType);
        myShader.setFloat(base + "ior", spheres[i].ior);
    }
    myShader.setInt("sphereCount", (int)spheres.size());

    //三角形
    for(int i = 0; i < triangles.size(); i++)
    {
        std::string base = "triangles[" + std::to_string(i) + "].";
        myShader.setVec3(base + "v0", triangles[i].v0);
        myShader.setVec3(base + "v1", triangles[i].v1);
        myShader.setVec3(base + "v2", triangles[i].v2);
        myShader.setVec3(base + "color", triangles[i].color);
        myShader.setInt(base + "materialType", triangles[i].materialType);
        myShader.setFloat(base + "ior", triangles[i].ior);
    }
    myShader.setInt("triangleCount", (int)triangles.size());

    //光源
    for (size_t i = 0; i < lights.size(); ++i) {
        std::string base = "lights[" + std::to_string(i) + "].";
        myShader.setVec3(base + "position", lights[i].position);
        myShader.setFloat(base + "intensity", lights[i].intensity);
    }
    myShader.setInt("lightCount", (int)lights.size());

    //摄像机参数
    glm::vec3 cameraPos(0.0f, 0.0f, 0.0f);
    myShader.setVec3("cameraPos", cameraPos);
    myShader.setFloat("fov", 90.0f);
    myShader.setInt("width", SCR_WIDTH);
    myShader.setInt("height", SCR_HEIGHT);

    //--渲染主循环--
    while(!glfwWindowShouldClose(myInit.window))
    {
        //--输入处理--
        processInput(myInit.window);

        //--渲染指令--
        //-背景渲染-
        glClearColor(0.3f, 0.8f, 0.8f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        //-画面渲染-
        myShader.use();
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        //--交换缓冲区和轮询IO事件--
		glfwSwapBuffers(myInit.window);
		glfwPollEvents();
    }

    //--资源释放--
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    return 0;
}

//--回调函数--
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window)
{
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}