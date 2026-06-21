/// @brief 主程序 —— 基于 PBF 的 3D 流体模拟，使用 SSFR 实现写实水面渲染
///
/// SSFR 渲染管线（参考 PositionBasedFluids 实现）：
///   通道1（深度准备）：GL_POINTS 渲染粒子球面 → NDC 深度纹理 + 硬件深度缓冲
///   通道2（双边平滑）：全屏四边形 × N轮迭代，分离式水平/垂直双边滤波
///   通道3（法线计算）：全屏四边形，从平滑深度重建视图空间法线（边缘感知）
///   通道4（厚度累积）：GL_POINTS 加法混合，累加视线方向流体厚度
///   通道5（流体着色）：全屏四边形，折射 + 天空反射 + Beer-Lambert 吸收

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

#include <cstdio>
#include <iostream>
#include <vector>

// ======================== 全局变量 ========================

const unsigned int SCR_WIDTH  = 800;
const unsigned int SCR_HEIGHT = 600;

MyCamera camera(glm::vec3(0.0f, 1.0f, 5.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -12.0f);

float  lastX             = SCR_WIDTH / 2.0f;
float  lastY             = SCR_HEIGHT / 2.0f;
bool   firstMouse        = true;
bool   leftButtonPressed = false;

float deltaTime = 0.0f;
float lastFrame = 0.0f;
float fixedDt   = 0.004f;
float totalTime = 0.0f;
float beginTime = 5.0f;

// ======================== 函数声明 ========================

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void processKeyboard(GLFWwindow* window);

// ---- FBO / 纹理工具 ----

GLuint createR32FTexture(int w, int h)
{
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, w, h, 0, GL_RED, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return tex;
}

GLuint createRGBA32FTexture(int w, int h)
{
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return tex;
}

GLuint createDepthRBO(int w, int h)
{
    GLuint rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
    return rbo;
}

// ======================== 主函数 ========================

int main()
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    // ---- 一、初始化窗口 ----
    MyInit myInit(SCR_WIDTH, SCR_HEIGHT, "CG FinalWork — SSFR");

    myInit.SetFramebufferSizeCallback(framebuffer_size_callback);
    myInit.SetCursorPosCallback(mouse_callback);
    myInit.SetScrollCallback(scroll_callback);
    myInit.SetMouseButtonCallback(mouse_button_callback);
    myInit.EnableDepthTest();

    std::cout << "OpenGL Version:  " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version:    " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "========================================" << std::endl;

    // ---- 二、初始化流体 ----
    FluidSystem fluidSystem;

    float spacing = 0.1f;
    glm::vec3 blockMin(-0.3f, 0.05f, -0.3f);
    glm::vec3 blockMax( 0.3f, 0.95f,  0.3f);
    //glm::vec3 blockMin(-0.45f, 0.05f, -0.45f);
    //glm::vec3 blockMax( 0.45f, 0.95f,  0.45f);
    fluidSystem.initializeParticles(blockMin, blockMax, spacing);

    int particleCount = static_cast<int>(fluidSystem.getParticleCount());
    std::cout << "粒子总数: " << particleCount << std::endl;
    std::cout << "========================================" << std::endl;

    // ---- 三、加载着色器 ----

    // 粒子着色器（深度准备 + 厚度准备共用顶点着色器）
    MyShader depthShader("src/ssfr_particle.vert", "src/ssfr_depth.frag");
    MyShader thicknessShader("src/ssfr_particle.vert", "src/ssfr_thickness.frag");

    // 后处理着色器（共用全屏四边形顶点着色器）
    MyShader smoothShader( "src/ssfr_quad.vert", "src/ssfr_smooth.frag");
    MyShader normalShader( "src/ssfr_quad.vert", "src/ssfr_normal.frag");
    MyShader shadeShader(  "src/ssfr_quad.vert", "src/ssfr_shade.frag");

    // ---- 四、创建纹理和 FBO ----

    // 深度通道
    GLuint depthTex      = createR32FTexture(SCR_WIDTH, SCR_HEIGHT);
    GLuint depthRBO      = createDepthRBO(SCR_WIDTH, SCR_HEIGHT);
    GLuint depthFBO;
    glGenFramebuffers(1, &depthFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, depthTex, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRBO);

    // 厚度通道（独立深度 RBO，因为在通道4中需要深度测试 + 禁止深度写入）
    GLuint thicknessTex  = createR32FTexture(SCR_WIDTH, SCR_HEIGHT);
    GLuint thicknessRBO  = createDepthRBO(SCR_WIDTH, SCR_HEIGHT);
    GLuint thicknessFBO;
    glGenFramebuffers(1, &thicknessFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, thicknessFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, thicknessTex, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, thicknessRBO);

    // 双边平滑 ping-pong（只需要颜色附件，全屏四边形不需要深度）
    GLuint smoothTexA = createR32FTexture(SCR_WIDTH, SCR_HEIGHT);
    GLuint smoothFBO_A;
    glGenFramebuffers(1, &smoothFBO_A);
    glBindFramebuffer(GL_FRAMEBUFFER, smoothFBO_A);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, smoothTexA, 0);

    GLuint smoothTexB = createR32FTexture(SCR_WIDTH, SCR_HEIGHT);
    GLuint smoothFBO_B;
    glGenFramebuffers(1, &smoothFBO_B);
    glBindFramebuffer(GL_FRAMEBUFFER, smoothFBO_B);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, smoothTexB, 0);

    // 法线通道
    GLuint normalTex = createRGBA32FTexture(SCR_WIDTH, SCR_HEIGHT);
    GLuint normalFBO;
    glGenFramebuffers(1, &normalFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, normalFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, normalTex, 0);

    // 验证 FBO 完整性
    GLenum fboStatus;
    glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
    fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (fboStatus != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR: depthFBO 不完整! 0x" << std::hex << fboStatus << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, thicknessFBO);
    fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (fboStatus != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR: thicknessFBO 不完整! 0x" << std::hex << fboStatus << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ---- 五、创建粒子 VAO/VBO ----
    std::vector<float> positionData = fluidSystem.getPositionData();

    unsigned int particleVAO, particleVBO;
    glGenVertexArrays(1, &particleVAO);
    glGenBuffers(1, &particleVBO);
    glBindVertexArray(particleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
    glBufferData(GL_ARRAY_BUFFER, positionData.size() * sizeof(float), positionData.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // ---- 六、创建全屏四边形 VAO（空 VAO，顶点由 gl_VertexID 生成） ----
    unsigned int quadVAO;
    glGenVertexArrays(1, &quadVAO);

    // 启用点精灵可编程大小
    glEnable(GL_PROGRAM_POINT_SIZE);

    std::cout << "========================================" << std::endl;
    std::cout << "SSFR 渲染管线就绪" << std::endl;
    std::cout << "========================================" << std::endl;

    // ---- 七、渲染循环 ----
    while (!glfwWindowShouldClose(myInit.window))
    {
        // 帧时间
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processKeyboard(myInit.window);

        // ---- 物理模拟 ----
        totalTime += deltaTime;
        if (totalTime > beginTime)
            fluidSystem.update(fixedDt);

        // ---- 上传粒子位置 ----
        std::vector<float> newPosData = fluidSystem.getPositionData();
        glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, newPosData.size() * sizeof(float), newPosData.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // ---- 矩阵 ----
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
                                                (float)SCR_WIDTH / (float)SCR_HEIGHT,
                                                0.1f, 100.0f);
        glm::mat4 view       = camera.GetViewMatrix();

        // 粒子渲染半径（世界空间）= PARTICLE_RADIUS * particleRadiusScaler
        // 参考实现中 particleRadiusScaler = 2.0，即渲染半径 = 物理半径 × 2
        float uPointSize = fluidSystem.h * 0.4f;   // 用光滑核半径作为渲染半径

        // 密度阈值：没有密度数据时设为 0，不做过滤
        float uMinimumDensity = 0.0f;

        // ================================================================
        // 通道1：深度准备
        //   渲染粒子球面 → 输出 NDC 深度到 depthTex，同时写入硬件深度缓冲
        // ================================================================
        glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        depthShader.use();
        depthShader.setMat4("uView", view);
        depthShader.setMat4("uProjection", projection);
        depthShader.setFloat("uPointSize", uPointSize);
        depthShader.setFloat("uMinimumDensity", uMinimumDensity);

        glBindVertexArray(particleVAO);
        glDrawArrays(GL_POINTS, 0, particleCount);

        // ================================================================
        // 通道2：双边滤波平滑深度
        //   分离式 1D 双边滤波（水平 + 垂直为一轮），共 6 轮，核半径递减
        //   核半径: 8 → 6 → 5 → 4 → 3 → 2（粗→细平滑）
        // ================================================================
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glDisable(GL_BLEND);

        smoothShader.use();
        smoothShader.setInt("uSeparate", 1);  // 分离式 (1D 水平/垂直)

        // 写死作为循环展开（GLSL 不支持动态数组作为 uniform）：
        // 每一轮 = 水平 pass + 垂直 pass，数据在 smoothTexA/B 之间乒乓
        const int smoothRounds = 6;
        const int kernels[smoothRounds] = {8, 6, 5, 4, 3, 2};

        // 源 = depthTex (raw), 目标 = smoothTexA
        GLuint srcTex = depthTex;
        GLuint dstFBO = smoothFBO_A;
        GLuint dstTex = smoothTexA;
        smoothShader.setInt("uSeparate", 1);

        for (int r = 0; r < smoothRounds; r++)
        {
            int kr = kernels[r];

            // --- 水平 pass ---
            glBindFramebuffer(GL_FRAMEBUFFER, dstFBO);
            glClear(GL_COLOR_BUFFER_BIT);

            smoothShader.setInt("uKernelRadius", kr);
            smoothShader.setBool("uHorizontal", true);
            smoothShader.setVec2("texelSize", 1.0f / SCR_WIDTH, 1.0f / SCR_HEIGHT);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, srcTex);
            smoothShader.setInt("depthTex", 0);

            glBindVertexArray(quadVAO);
            glDrawArrays(GL_TRIANGLES, 0, 3);

            // --- 垂直 pass ---
            // 乒乓切换：水平结果在 dstTex，垂直要写回 src 的另一侧
            GLuint midTex  = dstTex;   // 水平输出
            GLuint midFBO  = dstFBO;   // 水平 FBO
            GLuint outTex  = (r < smoothRounds - 1) ? ((dstTex == smoothTexA) ? smoothTexB : smoothTexA) : smoothTexB;
            GLuint outFBO  = (r < smoothRounds - 1) ? ((dstFBO == smoothFBO_A) ? smoothFBO_B : smoothFBO_A) : smoothFBO_B;

            glBindFramebuffer(GL_FRAMEBUFFER, outFBO);
            glClear(GL_COLOR_BUFFER_BIT);

            smoothShader.setInt("uKernelRadius", kr);
            smoothShader.setBool("uHorizontal", false);
            glBindTexture(GL_TEXTURE_2D, midTex);
            smoothShader.setInt("depthTex", 0);

            glDrawArrays(GL_TRIANGLES, 0, 3);

            // 下一轮的源 = 本轮垂直的输出
            srcTex = outTex;
            dstFBO = midFBO;  // 水平 FBO
            dstTex = midTex;  // 水平纹理
        }

        // 最终平滑结果在 smoothTexB
        GLuint finalSmoothedTex = smoothTexB;
        // (此时 srcTex == smoothTexB, 因为最后一轮垂直写入了 outTex == smoothTexB)

        // ================================================================
        // 通道3：法线计算
        //   从平滑深度重建视图空间位置 → 边缘感知切线选择 → 叉积求法线
        // ================================================================
        glBindFramebuffer(GL_FRAMEBUFFER, normalFBO);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        normalShader.use();
        normalShader.setMat4("projInverse", glm::inverse(projection));
        normalShader.setVec2("uScreenSize", glm::vec2(SCR_WIDTH, SCR_HEIGHT));  // ivec2 as vec2 for compatibility (will be cast in shader)
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, finalSmoothedTex);
        normalShader.setInt("depthTex", 0);

        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // ================================================================
        // 通道4：厚度累积
        //   加法混合（GL_ONE, GL_ONE）累加所有粒子贡献
        //   深度测试开启（用深度通道的深度做遮挡），禁止深度写入
        //   需要将深度通道的深度缓冲复制到厚度 FBO
        // ================================================================
        // 先将深度从 depthFBO 拷贝到 thicknessFBO
        glBindFramebuffer(GL_READ_FRAMEBUFFER, depthFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, thicknessFBO);
        glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT,
                          0, 0, SCR_WIDTH, SCR_HEIGHT,
                          GL_DEPTH_BUFFER_BIT, GL_NEAREST);

        glBindFramebuffer(GL_FRAMEBUFFER, thicknessFBO);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);  // 只清颜色，保留深度
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);        // 禁止写入深度
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);  // 加法混合

        thicknessShader.use();
        thicknessShader.setMat4("uView", view);
        thicknessShader.setMat4("uProjection", projection);
        thicknessShader.setFloat("uPointSize", uPointSize);
        thicknessShader.setFloat("uThicknessScaler", 0.05f);  // 参考值
        thicknessShader.setFloat("uMinimumDensity", uMinimumDensity);

        glBindVertexArray(particleVAO);
        glDrawArrays(GL_POINTS, 0, particleCount);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);

        // ================================================================
        // 通道5：流体着色
        //   全屏四边形：折射（背景） + 天空反射 + Beer-Lambert 吸收
        // ================================================================
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(0.15f, 0.15f, 0.18f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);

        shadeShader.use();
        shadeShader.setMat4("uViewInverse", glm::inverse(view));
        shadeShader.setMat4("uViewTranspose", glm::transpose(view));
        shadeShader.setMat4("uProjInverse", glm::inverse(projection));
        shadeShader.setVec2("uScreenSize", glm::vec2(SCR_WIDTH, SCR_HEIGHT));
        shadeShader.setVec3("uFluidColor", glm::vec3(0.15f, 0.45f, 0.75f));
        shadeShader.setVec3("uCameraPosition", camera.Position);
        shadeShader.setVec3("uSkyZenithColor", glm::vec3(0.22f, 0.52f, 0.82f));
        shadeShader.setVec3("uSkyHorizonColor", glm::vec3(0.60f, 0.78f, 0.92f));
        shadeShader.setVec3("uBackgroundColor", glm::vec3(0.15f, 0.15f, 0.18f));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, normalTex);
        shadeShader.setInt("uNormalViewSpaceTexture", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, thicknessTex);
        shadeShader.setInt("uThicknessTexture", 1);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, finalSmoothedTex);
        shadeShader.setInt("uSmoothedDepthTexture", 2);

        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // ---- FPS 显示 ----
        float fps = 1.0f / (deltaTime + 0.0001f);
        char title[128];
        snprintf(title, sizeof(title), "CG FinalWork | FPS: %.0f | Particles: %d", fps, particleCount);
        glfwSetWindowTitle(myInit.window, title);

        glfwSwapBuffers(myInit.window);
        glfwPollEvents();
    }

    // ---- 清理 ----
    glDeleteVertexArrays(1, &particleVAO);
    glDeleteBuffers(1, &particleVBO);
    glDeleteVertexArrays(1, &quadVAO);

    glDeleteTextures(1, &depthTex);
    glDeleteTextures(1, &thicknessTex);
    glDeleteTextures(1, &smoothTexA);
    glDeleteTextures(1, &smoothTexB);
    glDeleteTextures(1, &normalTex);

    glDeleteRenderbuffers(1, &depthRBO);
    glDeleteRenderbuffers(1, &thicknessRBO);

    glDeleteFramebuffers(1, &depthFBO);
    glDeleteFramebuffers(1, &thicknessFBO);
    glDeleteFramebuffers(1, &smoothFBO_A);
    glDeleteFramebuffers(1, &smoothFBO_B);
    glDeleteFramebuffers(1, &normalFBO);

    return 0;
}

// ======================== 回调函数 ========================

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
        leftButtonPressed = (action == GLFW_PRESS);
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
