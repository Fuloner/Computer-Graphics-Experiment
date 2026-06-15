/// @brief 第1步主程序 —— 基础框架搭建验证
/// <summary>
/// 本程序验证以下三个模块的协作：
/// 1. MyInit.h   —— 窗口创建与OpenGL初始化
/// 2. MyCamera.h —— 第一人称自由摄像机（WASD移动 + 鼠标旋转 + 滚轮缩放）
/// 3. MyShader.h —— 着色器编译、链接与uniform设置
///
/// 渲染内容：一个以原点为中心的线框立方体，白色线条，深色背景
/// 按ESC退出程序
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

#include <iostream>

// ======================== 全局变量 ========================

// 窗口尺寸
const unsigned int SCR_WIDTH  = 800;
const unsigned int SCR_HEIGHT = 600;

// 摄像机对象（初始位置放在原点前方偏上的位置，便于观察立方体）
MyCamera camera(glm::vec3(0.0f, 0.5f, 3.0f));

// 鼠标状态
float  lastX      = SCR_WIDTH / 2.0f;  // 上一帧鼠标X坐标
float  lastY      = SCR_HEIGHT / 2.0f;  // 上一帧鼠标Y坐标
bool   firstMouse = true;              // 是否为首次鼠标输入
bool   leftButtonPressed = false;      // 鼠标左键是否被按住

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

    // ---- 1. 初始化窗口和OpenGL上下文（使用MyInit封装）----
    MyInit myInit(SCR_WIDTH, SCR_HEIGHT, "CG FinalWork Step1 — 基础框架");

    // 注册回调函数
    myInit.SetFramebufferSizeCallback(framebuffer_size_callback);
    myInit.SetCursorPosCallback(mouse_callback);
    myInit.SetScrollCallback(scroll_callback);
    myInit.SetMouseButtonCallback(mouse_button_callback);

    // 开启深度测试，确保3D物体前后遮挡正确
    myInit.EnableDepthTest();

    // 输出OpenGL版本信息，方便调试
    std::cout << "OpenGL Version:  " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version:    " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "Renderer:        " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "操作说明：" << std::endl;
    std::cout << "  WASD      — 前后左右移动摄像机" << std::endl;
    std::cout << "  Q/E       — 升降摄像机" << std::endl;
    std::cout << "  鼠标左键拖拽 — 旋转视角" << std::endl;
    std::cout << "  滚轮      — 缩放（调整FOV）" << std::endl;
    std::cout << "  ESC       — 退出程序" << std::endl;
    std::cout << "========================================" << std::endl;

    // ---- 2. 加载着色器（使用MyShader封装）----
    MyShader shader("src/vertex_shader.glsl", "src/fragment_shader.glsl");

    // ---- 3. 构建立方体线框的顶点和索引数据 ----
    // 立方体8个顶点的局部坐标（以原点为中心，边长为1的立方体）
    float cubeVertices[] = {
        // 位置坐标 (x, y, z)
        -0.5f, -0.5f, -0.5f,  // 0: 左-下-远
         0.5f, -0.5f, -0.5f,  // 1: 右-下-远
         0.5f,  0.5f, -0.5f,  // 2: 右-上-远
        -0.5f,  0.5f, -0.5f,  // 3: 左-上-远
        -0.5f, -0.5f,  0.5f,  // 4: 左-下-近
         0.5f, -0.5f,  0.5f,  // 5: 右-下-近
         0.5f,  0.5f,  0.5f,  // 6: 右-上-近
        -0.5f,  0.5f,  0.5f   // 7: 左-上-近
    };

    // 索引数组 —— 定义12条棱，每条棱由2个端点组成（共24个索引值）
    unsigned int cubeIndices[] = {
        // 后面（z = -0.5）
        0, 1,  1, 2,  2, 3,  3, 0,
        // 前面（z = +0.5）
        4, 5,  5, 6,  6, 7,  7, 4,
        // 四条竖直棱（连接前后面）
        0, 4,  1, 5,  2, 6,  3, 7
    };

    // 创建VAO（顶点数组对象）
    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    // 上传顶点数据到VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    // 上传索引数据到EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);

    // 设置顶点属性指针：位置属性（location = 0），3个float分量
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // 解绑VAO（好的习惯，防止后续意外修改）
    glBindVertexArray(0);

    // ---- 4. 渲染循环 ----
    while (!glfwWindowShouldClose(myInit.window))
    {
        // 计算帧时间差
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // 处理键盘输入
        processKeyboard(myInit.window);

        // 清空颜色缓冲和深度缓冲
        glClearColor(0.15f, 0.15f, 0.18f, 1.0f);  // 深色背景
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 激活着色器
        shader.use();

        // 设置变换矩阵
        glm::mat4 model = glm::mat4(1.0f);  // 模型矩阵（单位矩阵，立方体在原位）

        // 投影矩阵 —— 透视投影，使用摄像机的Zoom作为FOV
        glm::mat4 projection = glm::perspective(
            glm::radians(camera.Zoom),
            (float)SCR_WIDTH / (float)SCR_HEIGHT,
            0.1f,    // 近裁剪面
            100.0f   // 远裁剪面
        );

        // 视图矩阵 —— 由摄像机提供
        glm::mat4 view = camera.GetViewMatrix();

        // 将矩阵传入着色器
        shader.setMat4("model", model);
        shader.setMat4("view", view);
        shader.setMat4("projection", projection);

        // 设置线框颜色（亮白色）
        shader.setVec3("objectColor", glm::vec3(0.9f, 0.9f, 0.9f));

        // 绘制线框立方体（GL_LINES模式，每对索引构成一条线段）
        glBindVertexArray(VAO);
        glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // 交换前后缓冲并处理事件
        glfwSwapBuffers(myInit.window);
        glfwPollEvents();
    }

    // ---- 5. 清理资源 ----
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

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
