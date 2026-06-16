/// @brief 第4步主程序 —— PBF核心算法：密度约束求解
/// <summary>
/// 本步骤实现PBF论文中最核心的"不可压缩性"约束求解：
/// 4a. Poly6核函数密度估算    —— ρ_i = Σ m_j · W_poly6(p_i - p_j, h)
/// 4b. 拉格朗日乘子λ_i      —— λ_i = -C_i / (Σ||∇C_i||² + ε)
/// 4c. 位置修正Δp_i         —— Δp_i = (1/ρ₀) Σ (λ_i+λ_j+s_corr) · ∇W_spiky
/// 4d. 迭代求解              —— 将4a-4c重复3次
///
/// 为演示求解器效果，初始化后对粒子位置施加微小随机扰动，
/// 求解器将通过密度约束将粒子向均匀密度方向调整。
/// 颜色含义：浅蓝=密度接近ρ₀（约束满足），深蓝=低密度，品红=过密。
/// </summary>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

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
#include <cstdlib>
#include <ctime>

// ======================== 全局变量 ========================

const unsigned int SCR_WIDTH  = 800;
const unsigned int SCR_HEIGHT = 600;

MyCamera camera(glm::vec3(0.0f, 0.5f, 3.5f));

float  lastX             = SCR_WIDTH / 2.0f;
float  lastY             = SCR_HEIGHT / 2.0f;
bool   firstMouse        = true;
bool   leftButtonPressed = false;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

// FPS计时器
int   frameCount   = 0;
float fpsTimer     = 0.0f;

// ======================== 函数声明 ========================

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void processKeyboard(GLFWwindow* window);

// ======================== 主函数 ========================

int main()
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    srand(static_cast<unsigned int>(time(nullptr)));

    // ---- 1. 初始化窗口和OpenGL上下文 ----
    MyInit myInit(SCR_WIDTH, SCR_HEIGHT, "CG FinalWork Step4 — PBF密度约束求解");

    myInit.SetFramebufferSizeCallback(framebuffer_size_callback);
    myInit.SetCursorPosCallback(mouse_callback);
    myInit.SetScrollCallback(scroll_callback);
    myInit.SetMouseButtonCallback(mouse_button_callback);
    myInit.EnableDepthTest();

    std::cout << "OpenGL Version:  " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version:    " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "========================================" << std::endl;

    // ---- 2. 初始化粒子系统 ----
    FluidSystem fluidSystem;

    float spacing = 0.1f;
    glm::vec3 blockMin(-0.45f, 0.05f, -0.45f);
    glm::vec3 blockMax( 0.45f, 0.95f,  0.45f);
    fluidSystem.initializeParticles(blockMin, blockMax, spacing);

    std::cout << "粒子总数:        " << fluidSystem.getParticleCount() << std::endl;
    std::cout << "  光滑核半径h:   " << fluidSystem.kernelRadius << std::endl;
    std::cout << "  静止密度ρ₀:    " << fluidSystem.restDensity << std::endl;
    std::cout << "  求解迭代次数:  " << fluidSystem.solverIterations << std::endl;
    std::cout << "========================================" << std::endl;

    // ---- 施加初始扰动（破坏完美晶格，演示求解器效果）----
    // 扰动幅度0.02世界单位，约为粒子间距的20%
    fluidSystem.perturbInitialPositions(0.02f);

    // ---- 执行首次求解并输出密度统计 ----
    fluidSystem.update(0.0f);
    fluidSystem.printDensityStats();
    std::cout << std::endl;

    std::cout << "操作说明：" << std::endl;
    std::cout << "  WASD      — 前后左右移动摄像机" << std::endl;
    std::cout << "  Q/E       — 升降摄像机" << std::endl;
    std::cout << "  鼠标左键拖拽 — 旋转视角" << std::endl;
    std::cout << "  滚轮      — 缩放（调整FOV）" << std::endl;
    std::cout << "  ESC       — 退出程序" << std::endl;
    std::cout << "  颜色：浅蓝=密度正常，深蓝=低密度，品红=过密度" << std::endl;
    std::cout << "  扰动后粒子方块边缘可能略微扩散或收缩" << std::endl;
    std::cout << "========================================" << std::endl;

    // ---- 3. 加载粒子着色器 ----
    MyShader particleShader("src/particle_vertex.glsl", "src/particle_fragment.glsl");

    // ---- 4. 创建VAO和VBO（位置 + 颜色，均为DYNAMIC_DRAW，每帧更新）----
    std::vector<float> positionData = fluidSystem.getPositionData();
    std::vector<glm::vec3> colorData = fluidSystem.getColorData();

    unsigned int particleVAO;
    unsigned int particleVBOs[2];
    glGenVertexArrays(1, &particleVAO);
    glGenBuffers(2, particleVBOs);

    glBindVertexArray(particleVAO);

    // VBO[0]：粒子位置（每帧更新，因为求解器持续修改位置）
    glBindBuffer(GL_ARRAY_BUFFER, particleVBOs[0]);
    glBufferData(GL_ARRAY_BUFFER, positionData.size() * sizeof(float),
                 positionData.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // VBO[1]：逐粒子颜色（每帧更新，反映密度变化）
    glBindBuffer(GL_ARRAY_BUFFER, particleVBOs[1]);
    glBufferData(GL_ARRAY_BUFFER, colorData.size() * sizeof(glm::vec3),
                 colorData.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // ---- 5. 渲染循环 ----
    while (!glfwWindowShouldClose(myInit.window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // FPS统计
        frameCount++;
        fpsTimer += deltaTime;
        if (fpsTimer >= 1.0f)
        {
            float fps = frameCount / fpsTimer;
            // 在窗口标题栏显示FPS
            std::string title = "CG FinalWork Step4 — PBF密度约束求解  [FPS: "
                              + std::to_string(static_cast<int>(fps))
                              + " | 粒子: " + std::to_string(fluidSystem.getParticleCount()) + "]";
            glfwSetWindowTitle(myInit.window, title.c_str());
            frameCount = 0;
            fpsTimer = 0.0f;
        }

        processKeyboard(myInit.window);

        // 执行PBF密度约束求解
        fluidSystem.update(deltaTime);

        // 更新位置VBO（粒子位置在求解中被修改）
        positionData = fluidSystem.getPositionData();
        glBindBuffer(GL_ARRAY_BUFFER, particleVBOs[0]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, positionData.size() * sizeof(float),
                        positionData.data());

        // 更新颜色VBO（颜色反映当前密度分布）
        colorData = fluidSystem.getColorData();
        glBindBuffer(GL_ARRAY_BUFFER, particleVBOs[1]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, colorData.size() * sizeof(glm::vec3),
                        colorData.data());

        // 清空缓冲
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

    // ---- 6. 清理 ----
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
        camera.ProcessMouseMovement(xoffset, yoffset);
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
