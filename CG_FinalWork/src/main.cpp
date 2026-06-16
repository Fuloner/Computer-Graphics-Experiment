/// @brief 第3步主程序 —— 邻居搜索（空间哈希）
/// <summary>
/// 本步骤实现：
/// 1. SpatialHashing类 —— 空间哈希网格，将3D空间划分为立方体单元，O(n)级邻居搜索
/// 2. 集成到FluidSystem —— 每帧构建哈希表，统计每个粒子的邻居数量
/// 3. 逐粒子颜色映射 —— 邻居多的粒子显示为白色，邻居少的显示为蓝色
///    （内部粒子周围有约18个邻居，显示为亮白；边缘粒子邻居较少，偏蓝）
///
/// 当前无物理模拟，粒子保持静止，邻居数量每帧不变。
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
#include <vector>

// ======================== 全局变量 ========================

// 窗口尺寸
const unsigned int SCR_WIDTH  = 800;
const unsigned int SCR_HEIGHT = 600;

// 摄像机对象（位于水箱前方，稍微抬高以便看到粒子方块的全貌）
MyCamera camera(glm::vec3(0.0f, 0.5f, 3.5f));

// 鼠标状态
float  lastX             = SCR_WIDTH / 2.0f;
float  lastY             = SCR_HEIGHT / 2.0f;
bool   firstMouse        = true;
bool   leftButtonPressed = false;

// 时间
float deltaTime = 0.0f;
float lastFrame = 0.0f;

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

    // ---- 1. 初始化窗口和OpenGL上下文 ----
    MyInit myInit(SCR_WIDTH, SCR_HEIGHT, "CG FinalWork Step3 — 邻居搜索（空间哈希）");

    myInit.SetFramebufferSizeCallback(framebuffer_size_callback);
    myInit.SetCursorPosCallback(mouse_callback);
    myInit.SetScrollCallback(scroll_callback);
    myInit.SetMouseButtonCallback(mouse_button_callback);

    myInit.EnableDepthTest();

    std::cout << "OpenGL Version:  " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version:    " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "========================================" << std::endl;

    // ---- 2. 初始化流体粒子系统 ----
    FluidSystem fluidSystem;

    // 水箱尺寸：宽1.0 × 高2.0 × 深1.0，中心在原点（y ∈ [-1, 1]）
    // 粒子块放置在水箱上半部分（y ∈ [0.0, 1.0]），间距0.1，10×10×10 = 1000个粒子
    float spacing = 0.1f;
    glm::vec3 blockMin(-0.45f, 0.05f, -0.45f);
    glm::vec3 blockMax( 0.45f, 0.95f,  0.45f);

    fluidSystem.initializeParticles(blockMin, blockMax, spacing);

    std::cout << "粒子总数:        " << fluidSystem.getParticleCount() << std::endl;
    std::cout << "  粒子块范围: X[" << blockMin.x << ", " << blockMax.x << "]  "
              << "Y[" << blockMin.y << ", " << blockMax.y << "]  "
              << "Z[" << blockMin.z << ", " << blockMax.z << "]" << std::endl;
    std::cout << "  粒子间距:       " << spacing << std::endl;
    std::cout << "  光滑核半径 h:   " << fluidSystem.kernelRadius << std::endl;
    std::cout << "  粒子渲染半径:   " << fluidSystem.particleRadius << std::endl;
    std::cout << "  静止密度 ρ₀:    " << fluidSystem.restDensity << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "操作说明：" << std::endl;
    std::cout << "  WASD      — 前后左右移动摄像机" << std::endl;
    std::cout << "  Q/E       — 升降摄像机" << std::endl;
    std::cout << "  鼠标左键拖拽 — 旋转视角" << std::endl;
    std::cout << "  滚轮      — 缩放（调整FOV）" << std::endl;
    std::cout << "  ESC       — 退出程序" << std::endl;
    std::cout << "  颜色含义：白色=邻居多（内部粒子），深蓝=邻居少（边缘/角落）" << std::endl;
    std::cout << "========================================" << std::endl;

    // ---- 执行首次邻居搜索（输出统计信息到控制台）----
    fluidSystem.updateNeighborSearch(true);

    // ---- 3. 加载粒子着色器 ----
    MyShader particleShader("src/particle_vertex.glsl", "src/particle_fragment.glsl");

    // ---- 4. 创建粒子VAO和VBO（位置 + 颜色）----
    std::vector<float> positionData = fluidSystem.getPositionData();
    std::vector<glm::vec3> colorData = fluidSystem.getColorData();

    unsigned int particleVAO;
    unsigned int particleVBOs[2];  // VBO[0]=位置, VBO[1]=颜色
    glGenVertexArrays(1, &particleVAO);
    glGenBuffers(2, particleVBOs);

    glBindVertexArray(particleVAO);

    // VBO[0]：粒子位置数据（location = 0）
    glBindBuffer(GL_ARRAY_BUFFER, particleVBOs[0]);
    glBufferData(GL_ARRAY_BUFFER, positionData.size() * sizeof(float),
                 positionData.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // VBO[1]：逐粒子颜色数据（location = 1），由邻居数量决定
    glBindBuffer(GL_ARRAY_BUFFER, particleVBOs[1]);
    glBufferData(GL_ARRAY_BUFFER, colorData.size() * sizeof(glm::vec3),
                 colorData.data(), GL_DYNAMIC_DRAW);  // DYNAMIC_DRAW：后续帧中会更新
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // 启用点精灵程序尺寸控制（允许顶点着色器中的gl_PointSize生效）
    glEnable(GL_PROGRAM_POINT_SIZE);

    // 启用混合渲染，用于粒子圆形的柔和边缘
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

        // 更新流体模拟（当前执行邻居搜索，粒子不动但为后续步骤做准备）
        fluidSystem.update(deltaTime);

        // 更新颜色VBO（因为后续步骤中粒子会移动，邻居关系会变化）
        colorData = fluidSystem.getColorData();
        glBindBuffer(GL_ARRAY_BUFFER, particleVBOs[1]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, colorData.size() * sizeof(glm::vec3),
                        colorData.data());

        // 清空颜色缓冲和深度缓冲
        glClearColor(0.15f, 0.15f, 0.18f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ---- 渲染粒子 ----
        particleShader.use();

        glm::mat4 model = glm::mat4(1.0f);

        glm::mat4 projection = glm::perspective(
            glm::radians(camera.Zoom),
            (float)SCR_WIDTH / (float)SCR_HEIGHT,
            0.1f, 100.0f);

        glm::mat4 view = camera.GetViewMatrix();

        particleShader.setMat4("model", model);
        particleShader.setMat4("view", view);
        particleShader.setMat4("projection", projection);

        float pointScale = fluidSystem.particleRadius * SCR_HEIGHT
                         / tan(glm::radians(camera.Zoom * 0.5f));
        particleShader.setFloat("pointScale", pointScale);

        glBindVertexArray(particleVAO);
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(fluidSystem.getParticleCount()));
        glBindVertexArray(0);

        glfwSwapBuffers(myInit.window);
        glfwPollEvents();
    }

    // ---- 6. 清理资源 ----
    glDeleteVertexArrays(1, &particleVAO);
    glDeleteBuffers(2, particleVBOs);

    return 0;
}

// ======================== 回调函数实现 ========================

void framebuffer_size_callback(GLFWwindow* /*window*/, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* /*window*/, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

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

    if (leftButtonPressed)
    {
        camera.ProcessMouseMovement(xoffset, yoffset);
    }
}

void scroll_callback(GLFWwindow* /*window*/, double /*xoffset*/, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void mouse_button_callback(GLFWwindow* /*window*/, int button, int action, int /*mods*/)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
            leftButtonPressed = true;
        else if (action == GLFW_RELEASE)
            leftButtonPressed = false;
    }
}

void processKeyboard(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, deltaTime);
}
