/// @brief 颜色
//--openGL库--
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//--自定义工具类--
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>

//--C++标准库--
#include <iostream>
#include <cmath>
#include <vector>

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

//--光--
glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

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

    //--着色器配置--
    Shader cubeShader("src/cube.vs", "src/cube.fs");
    Shader lightCubeShader("src/light_cube.vs", "src/light_cube.fs");


    //--顶点数据和缓冲配置--
    float vertices[] =
    {
        -0.5f, -0.5f, -0.5f, 
         0.5f, -0.5f, -0.5f,  
         0.5f,  0.5f, -0.5f,  
         0.5f,  0.5f, -0.5f,  
        -0.5f,  0.5f, -0.5f, 
        -0.5f, -0.5f, -0.5f, 

        -0.5f, -0.5f,  0.5f, 
         0.5f, -0.5f,  0.5f,  
         0.5f,  0.5f,  0.5f,  
         0.5f,  0.5f,  0.5f,  
        -0.5f,  0.5f,  0.5f, 
        -0.5f, -0.5f,  0.5f, 

        -0.5f,  0.5f,  0.5f, 
        -0.5f,  0.5f, -0.5f, 
        -0.5f, -0.5f, -0.5f, 
        -0.5f, -0.5f, -0.5f, 
        -0.5f, -0.5f,  0.5f, 
        -0.5f,  0.5f,  0.5f, 

         0.5f,  0.5f,  0.5f,  
         0.5f,  0.5f, -0.5f,  
         0.5f, -0.5f, -0.5f,  
         0.5f, -0.5f, -0.5f,  
         0.5f, -0.5f,  0.5f,  
         0.5f,  0.5f,  0.5f,  

        -0.5f, -0.5f, -0.5f, 
         0.5f, -0.5f, -0.5f,  
         0.5f, -0.5f,  0.5f,  
         0.5f, -0.5f,  0.5f,  
        -0.5f, -0.5f,  0.5f, 
        -0.5f, -0.5f, -0.5f, 

        -0.5f,  0.5f, -0.5f, 
         0.5f,  0.5f, -0.5f,  
         0.5f,  0.5f,  0.5f,  
         0.5f,  0.5f,  0.5f,  
        -0.5f,  0.5f,  0.5f, 
        -0.5f,  0.5f, -0.5f, 
    };

    //--创建--
    unsigned int cubeVAO;
    unsigned int VBO;
    
    //--生成--
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &VBO);
    
    //--绑定--
    glBindVertexArray(cubeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    //--设置顶点属性指针--
    //位置属性
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    //--解绑--
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    unsigned int lightVAO; //灯的VAO
    glGenVertexArrays(1, &lightVAO);
    glBindVertexArray(lightVAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO); //只需要绑定VBO不用再次设置VBO的数据，因为箱子的VBO数据中已经包含了正确的立方体顶点数据
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0); //设置灯立方体的顶点属性
    glEnableVertexAttribArray(0);


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
        cubeShader.use();
        cubeShader.setVec3("objectColor", 1.0f, 0.5f, 0.31f);
        cubeShader.setVec3("lightColor",  1.0f, 1.0f, 1.0f);

        //--设置投影矩阵--
        glm::mat4 projection = glm::perspective(glm::radians(myCamera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        cubeShader.setMat4("projection", projection);

        //--设置视图矩阵--
        glm::mat4 view = myCamera.GetViewMatrix();
        cubeShader.setMat4("view", view);

        //--设置模型矩阵--
        glm::mat4 model = glm::mat4(1.0f);
        cubeShader.setMat4("model", model);

        //--绘制物体--
        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);

        //--激活灯立方体的着色器--
        lightCubeShader.use();

        //--设置投影矩阵--
        lightCubeShader.setMat4("projection", projection);

        //--设置视图矩阵--
        lightCubeShader.setMat4("view", view);

        //--设置模型矩阵--
        model = glm::mat4(1.0f);
        model = glm::translate(model, lightPos);
        model = glm::scale(model, glm::vec3(0.2f));
        lightCubeShader.setMat4("model", model);

        //--绘制灯立方体--
        glBindVertexArray(lightVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);

        //--交换缓冲区 与 轮询事件--
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    //--清理资源--
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteVertexArrays(1, &lightVAO);
    glDeleteBuffers(1, &VBO);
    
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

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    myCamera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    myCamera.ProcessMouseScroll(yoffset);
}