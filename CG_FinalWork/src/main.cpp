/// @brief 第2步主程序 —— 粒子系统基础
/// <summary>
/// 本步骤实现：
/// 1. Particle结构体 —— 每个流体粒子的完整状态（位置、速度、预测位置、lambda）
/// 2. FluidSystem类  —— 管理所有粒子，初始化立方晶格排列，提供渲染数据
/// 3. 点精灵渲染    —— 使用GL_POINTS + 圆形裁剪片元着色器，将粒子渲染为蓝色圆点
///
/// 
/// </summary>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

// Windows控制台UTF-8编码支持（解决中文乱码问题）
#ifdef _WIN32
#include <windows.h>
#endif

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "mygl/MyInit.h"
#include "mygl/MyCamera.h"
#include "mygl/MyShader.h"
#include "mygl/FluidSystem.h"

#include <iostream>

// ======================== 全局变量 ========================

// 窗口尺寸
const unsigned int SCR_WIDTH  = 800;
const unsigned int SCR_HEIGHT = 600;

// 摄像机对象
MyCamera camera(glm::vec3(0.0f, 0.5f, 3.5f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -15.0f);

// 鼠标状态
float  lastX             = SCR_WIDTH / 2.0f;  // 上一帧鼠标X坐标
float  lastY             = SCR_HEIGHT / 2.0f;  // 上一帧鼠标Y坐标
bool   firstMouse        = true;               // 是否为首次鼠标输入
bool   leftButtonPressed = false;              // 鼠标左键是否被按住

// 时间
float deltaTime = 0.0f;  // 上一帧耗时（秒）
float lastFrame = 0.0f;  // 上一帧的时间点

// ======================== 函数声明 ========================

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void processKeyboard(GLFWwindow* window);

// ======================== 主函数 ========================

int main()
{
    // 将Windows控制台输出编码设置为UTF-8，解决中文乱码问题
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    // ---- 一、初始化窗口和OpenGL上下文 ----
    MyInit myInit(SCR_WIDTH, SCR_HEIGHT, "CG FinalWork Step2 — 粒子系统基础");

    // 注册回调函数
    myInit.SetFramebufferSizeCallback(framebuffer_size_callback);
    myInit.SetCursorPosCallback(mouse_callback);
    myInit.SetScrollCallback(scroll_callback);
    myInit.SetMouseButtonCallback(mouse_button_callback);

    // 开启深度测试，确保3D空间中粒子之间的前后遮挡正确
    myInit.EnableDepthTest();

    // 输出OpenGL版本信息
    std::cout << "OpenGL Version:  " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version:    " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "Renderer:        " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "========================================" << std::endl;

    // ---- 2. 初始化流体粒子系统 ----
    FluidSystem fluidSystem;

    // 水箱尺寸：宽1.0 × 高2.0 × 深1.0，中心在原点（y ∈ [-1, 1]）
    // 粒子块放置在水箱上半部分（y ∈ [0.0, 1.0]），XZ范围留出边距
    // 每个维度10个粒子，间距0.1，共10×10×10 = 1000个粒子
    float spacing = 0.08f;
    glm::vec3 blockMin(-0.45f, 0.05f, -0.45f);  // 粒子块最小角（水箱上半部分的底部）
    glm::vec3 blockMax( 0.45f, 0.95f,  0.45f);  // 粒子块最大角（水箱上半部分的顶部）
    //glm::vec3 blockMin(-0.3f, 0.3f, -0.3f);
    //glm::vec3 blockMax( 0.3f, 0.9f,  0.3f);

    fluidSystem.initializeParticles(blockMin, blockMax, spacing);

    std::cout << "粒子总数: " << fluidSystem.getParticleCount() << std::endl;
    std::cout << "  粒子块范围: X[" << blockMin.x << ", " << blockMax.x << "]  "
              << "Y[" << blockMin.y << ", " << blockMax.y << "]  "
              << "Z[" << blockMin.z << ", " << blockMax.z << "]" << std::endl;
    std::cout << "  粒子间距: " << spacing << std::endl;
    std::cout << "  光滑核半径 h: " << fluidSystem.kernelRadius << std::endl;
    std::cout << "  粒子渲染半径: " << fluidSystem.particleRadius << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "操作说明：" << std::endl;
    std::cout << "  WASD      — 前后左右移动摄像机" << std::endl;
    std::cout << "  Q/E       — 升降摄像机" << std::endl;
    std::cout << "  鼠标左键拖拽 — 旋转视角" << std::endl;
    std::cout << "  滚轮      — 缩放（调整FOV）" << std::endl;
    std::cout << "  ESC       — 退出程序" << std::endl;
    std::cout << "  当前为静态粒子方块，无物理模拟" << std::endl;
    std::cout << "========================================" << std::endl;

    // ---- 3. 加载粒子着色器 ----
    MyShader particleShader("src/particle_vertex.glsl", "src/particle_fragment.glsl");

    // ---- 4. 创建粒子VAO/VBO ----
    // 获取所有粒子的位置数据（连续的float数组）
    std::vector<float> positionData = fluidSystem.getPositionData();
    std::vector<glm::vec3> colorData = fluidSystem.getColorData();

    unsigned int particleVAO, particleVBO;
    glGenVertexArrays(1, &particleVAO);
    glGenBuffers(1, &particleVBO);

    glBindVertexArray(particleVAO);

    // 将粒子位置数据上传到VBO
    glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
    glBufferData(GL_ARRAY_BUFFER, positionData.size() * sizeof(float), positionData.data(), GL_STATIC_DRAW);

    // 设置顶点属性：位置（location = 0），3个float分量，紧密排列
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    unsigned int particleVBO_color;
    glGenBuffers(1, &particleVBO_color);

    glBindBuffer(GL_ARRAY_BUFFER, particleVBO_color);
    glBufferData(GL_ARRAY_BUFFER, colorData.size() * sizeof(glm::vec3), colorData.data(), GL_DYNAMIC_DRAW);
    
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // 启用点精灵程序尺寸控制（允许顶点着色器中的gl_PointSize生效）
    glEnable(GL_PROGRAM_POINT_SIZE);

    // 启用混合渲染，用于粒子圆形的柔和边缘（抗锯齿效果）
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // ---- 5. 渲染循环 ----
    while (!glfwWindowShouldClose(myInit.window))
    {
        // 计算帧时间差
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // 处理键盘输入
        processKeyboard(myInit.window);

        // 更新流体模拟
        deltaTime /= 3.0f;
        fluidSystem.update(deltaTime);

        // 更新粒子位置数据到VBO
        std::vector<float> newPosData = fluidSystem.getPositionData();
        glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, newPosData.size() * sizeof(float), newPosData.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // 
        std::vector<glm::vec3> newColorData = fluidSystem.getColorData();
        glBindBuffer(GL_ARRAY_BUFFER, particleVBO_color);
        glBufferSubData(GL_ARRAY_BUFFER, 0, newColorData.size() * sizeof(glm::vec3), newColorData.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // 清空颜色缓冲和深度缓冲
        glClearColor(0.15f, 0.15f, 0.18f, 1.0f);  // 深色背景
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ---- 渲染粒子 ----
        particleShader.use();

        // 变换矩阵
        glm::mat4 model = glm::mat4(1.0f);  // 粒子位置已经是世界坐标，不需要模型变换

        glm::mat4 projection = glm::perspective(
            glm::radians(camera.Zoom),
            (float)SCR_WIDTH / (float)SCR_HEIGHT,
            0.1f,    // 近裁剪面
            100.0f   // 远裁剪面
        );

        glm::mat4 view = camera.GetViewMatrix();

        // 传入uniform
        particleShader.setMat4("model", model);
        particleShader.setMat4("view", view);
        particleShader.setMat4("projection", projection);

        particleShader.setFloat("pointScale", 5.0f);

        // 设置粒子颜色（浅蓝色，模拟水滴外观） 暂时不需要
        particleShader.setVec3("particleColor", glm::vec3(0.2f, 0.5f, 0.9f));

        // 绘制所有粒子（使用GL_POINTS图元）
        glBindVertexArray(particleVAO);
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(fluidSystem.getParticleCount()));
        glBindVertexArray(0);

        // 交换前后缓冲并处理事件
        glfwSwapBuffers(myInit.window);
        glfwPollEvents();
    }

    // ---- 6. 清理资源 ----
    glDeleteVertexArrays(1, &particleVAO);
    glDeleteBuffers(1, &particleVBO);

    return 0;
}

// ======================== 回调函数实现 ========================

/// <summary>
/// 窗口尺寸变化回调 —— 确保视口始终与窗口大小匹配
/// </summary>
void framebuffer_size_callback(GLFWwindow* /*window*/, int width, int height)
{
    glViewport(0, 0, width, height);
}

/// <summary>
/// 鼠标移动回调 —— 按住左键拖拽时旋转摄像机视角
/// </summary>
void mouse_callback(GLFWwindow* /*window*/, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    // 首次进入时，直接将当前鼠标位置作为"上一帧位置"，避免跳帧
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    // 计算本次与上次鼠标位置的偏移量
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;  // Y轴反向：屏幕Y向下，摄像机俯仰角向上为正
    lastX = xpos;
    lastY = ypos;

    // 仅在左键被按住时旋转视角
    if (leftButtonPressed)
    {
        camera.ProcessMouseMovement(xoffset, yoffset);
    }
}

/// <summary>
/// 鼠标滚轮回调 —— 缩放（调整摄像机的FOV）
/// </summary>
void scroll_callback(GLFWwindow* /*window*/, double /*xoffset*/, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

/// <summary>
/// 鼠标按键回调 —— 跟踪左键是否被按下
/// </summary>
void mouse_button_callback(GLFWwindow* /*window*/, int button, int action, int /*mods*/)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            leftButtonPressed = true;
        }
        else if (action == GLFW_RELEASE)
        {
            leftButtonPressed = false;
        }
    }
}

/// <summary>
/// 处理键盘输入 —— WASD移动、QE升降、ESC退出
/// </summary>
void processKeyboard(GLFWwindow* window)
{
    // ESC退出
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // WASD —— 前后左右移动
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

    // Q/E —— 上升/下降
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, deltaTime);
}
