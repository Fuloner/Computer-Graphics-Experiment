/// @brief 
//--openGL库--
#include <glad/glad.h>
#include <GLFW/glfw3.h>
//#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//--自定义工具类--
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/mesh.h>
#include <learnopengl/model.h>

//--C++标准库--
#include <iostream>
//#include <cmath>
//#include <vector>

//--函数声明--
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

//--屏幕设置--
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

//--摄像机设置--
Camera myCamera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX =  SCR_WIDTH / 2.0f;
float lastY =  SCR_HEIGHT / 2.0f;
bool firstMouse = true;

//--时间--
float deltaTime = 0.0f;
float lastFrame = 0.0f;

//--主函数--
int main()
{
    //--glfw初始化和配置--
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    //--创建窗口--
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    //--注册回调函数--
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    //--设置输入模式--
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    //--glad初始化--
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    //--配置全局OpenGL状态--
    glEnable(GL_DEPTH_TEST);

    //--stb_image加载纹理时y轴翻转--
    //stbi_set_flip_vertically_on_load(true);

    //--着色器--
    Shader ourShader("src/model_loading.vs", "src/model_loading.fs");

    //--加载模型--
    //Model ourModel("model/backpack/backpack.obj");
    Model ourModel("model/batman/batman.obj");

    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    //--渲染循环--
    while(!glfwWindowShouldClose(window))
    {
        //--每帧时间逻辑--
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        //--输入处理--
        processInput(window);

        //--渲染背景-- 
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //--激活被照立方体的着色器--
        //设置uniform变量
        ourShader.use();
        
        //光
        ourShader.setVec3("viewPos", myCamera.Position);
        ourShader.setVec3("pointLight.position", glm::vec3(1.2f, 1.0f, 2.0f));
        ourShader.setVec3("pointLight.ambient", glm::vec3(0.2f, 0.2f, 0.2f));
        ourShader.setVec3("pointLight.diffuse", glm::vec3(0.5f, 0.5f, 0.5f));
        ourShader.setVec3("pointLight.specular", glm::vec3(1.0f, 1.0f, 1.0f));

        //设置投影和视图矩阵
        glm::mat4 projection = glm::perspective(glm::radians(myCamera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = myCamera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        //设置模型矩阵
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
        ourShader.setMat4("model", model);
        ourModel.Draw(ourShader);

        //--交换缓冲区 与 轮询事件--
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window)
{
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        myCamera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        myCamera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        myCamera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        myCamera.ProcessKeyboard(RIGHT, deltaTime);
    //添加上下移动 方便控制
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
    {
        //myCamera.ProcessKeyboard(UP, deltaTime);
        //TODO
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
    {
        //myCamera.ProcessKeyboard(DOWN, deltaTime);
        //TODO
    }   
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    float xposf = static_cast<float>(xpos);
    float yposf = static_cast<float>(ypos);

    if(firstMouse)
    {
        lastX = xposf;
        lastY = yposf;
        firstMouse = false;
    }

    float xoffset = xposf - lastX;
    float yoffset = lastY - yposf;
    lastX = xposf;
    lastY = yposf;

    myCamera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    myCamera.ProcessMouseScroll(yoffset);
}