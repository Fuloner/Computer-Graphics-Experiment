/// @brief 主程序 —— 基于PBF的3D流体模拟，使用屏幕空间流体渲染(SSFR)
/// <summary>
/// SSFR 渲染管线分为以下通道：
///   通道1（深度通道）：将每个粒子渲染为球面深度到大球点精灵，输出视图空间线性深度
///   通道2（厚度通道）：渲染相同粒子，加法混合累积视线穿过流体的总厚度
///   通道3（双边滤波）：对深度纹理做2次双边滤波，填充粒子间隙同时保持边缘
///   通道4（着色合成）：从平滑深度重建法线→Blinn-Phong光照→厚度吸收→与背景混合
///   通道5（水箱线框）：在水箱深度之上渲染半透明线框
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

#include <cstdio>
#include <iostream>
#include <vector>

// ======================== 全局变量 ========================

// 窗口尺寸（FBO 纹理大小与此一致）
const unsigned int SCR_WIDTH  = 800;
const unsigned int SCR_HEIGHT = 600;

// 摄像机
MyCamera camera(glm::vec3(0.0f, 1.0f, 5.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -12.0f);

// 鼠标状态
float  lastX             = SCR_WIDTH / 2.0f;
float  lastY             = SCR_HEIGHT / 2.0f;
bool   firstMouse        = true;
bool   leftButtonPressed = false;

// 时间
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

// ---- FBO 辅助函数 ----

/// <summary>
/// 创建一个 R32F 浮点纹理，用于 FBO 的颜色附件（存储深度或厚度）
/// </summary>
GLuint createFloatTexture(int width, int height)
{
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return tex;
}

/// <summary>
/// 创建深度渲染缓冲对象（RBO），用作 FBO 的深度附件
/// </summary>
GLuint createDepthRBO(int width, int height)
{
    GLuint rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    return rbo;
}

/// <summary>
/// 构建水箱（透明包围盒）的线框顶点数据
/// 12条棱 × 每条棱2个顶点 = 24个顶点，使用 GL_LINES 绘制
/// </summary>
std::vector<float> buildTankVertices(const glm::vec3& minBound, const glm::vec3& maxBound)
{
    float x0 = minBound.x, x1 = maxBound.x;
    float y0 = minBound.y, y1 = maxBound.y;
    float z0 = minBound.z, z1 = maxBound.z;

    // 12条棱的顶点对（每条棱2个顶点）
    std::vector<float> edges = {
        // 底面4条棱（y = y0）
        x0, y0, z0,  x1, y0, z0,
        x1, y0, z0,  x1, y0, z1,
        x1, y0, z1,  x0, y0, z1,
        x0, y0, z1,  x0, y0, z0,

        // 顶面4条棱（y = y1）
        x0, y1, z0,  x1, y1, z0,
        x1, y1, z0,  x1, y1, z1,
        x1, y1, z1,  x0, y1, z1,
        x0, y1, z1,  x0, y1, z0,

        // 4条立柱
        x0, y0, z0,  x0, y1, z0,
        x1, y0, z0,  x1, y1, z0,
        x1, y0, z1,  x1, y1, z1,
        x0, y0, z1,  x0, y1, z1,
    };
    return edges;
}

// ======================== 主函数 ========================

int main()
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    // ---- 一、初始化窗口 ----
    MyInit myInit(SCR_WIDTH, SCR_HEIGHT, "CG FinalWork — SSFR 流体渲染");

    myInit.SetFramebufferSizeCallback(framebuffer_size_callback);
    myInit.SetCursorPosCallback(mouse_callback);
    myInit.SetScrollCallback(scroll_callback);
    myInit.SetMouseButtonCallback(mouse_button_callback);
    myInit.EnableDepthTest();

    std::cout << "OpenGL Version:  " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version:    " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "Renderer:        " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "========================================" << std::endl;

    // ---- 二、初始化流体粒子系统 ----
    FluidSystem fluidSystem;

    float spacing = 0.1f;
    glm::vec3 blockMin(-0.3f, 0.3f, -0.3f);
    glm::vec3 blockMax( 0.3f, 0.9f,  0.3f);
    fluidSystem.initializeParticles(blockMin, blockMax, spacing);

    int particleCount = static_cast<int>(fluidSystem.getParticleCount());
    std::cout << "粒子总数: " << particleCount << std::endl;

    // ---- 三、加载着色器 ----

    // 深度通道着色器（粒子球面 → 视图空间线性深度）
    MyShader depthShader("src/ssfr_particle.vert", "src/ssfr_depth.frag");

    // 厚度通道着色器（粒子球体弦长 → 加法累积）
    MyShader thicknessShader("src/ssfr_particle.vert", "src/ssfr_thickness.frag");

    // 双边滤波着色器（全屏四边形 → 平滑深度纹理）
    MyShader bilateralShader("src/ssfr_quad.vert", "src/ssfr_bilateral.frag");

    // 最终着色合成着色器（全屏四边形 → 法线重建 + 光照 + 厚度吸收）
    MyShader shadeShader("src/ssfr_quad.vert", "src/ssfr_shade.frag");

    // 水箱线框着色器
    MyShader tankShader("src/tank.vert", "src/tank.frag");

    // ---- 四、创建 FBO 和纹理 ----

    // 深度通道 FBO（颜色附件: R32F 深度, 深度附件: RBO）
    GLuint depthTex = createFloatTexture(SCR_WIDTH, SCR_HEIGHT);
    GLuint depthRBO = createDepthRBO(SCR_WIDTH, SCR_HEIGHT);
    GLuint depthFBO;
    glGenFramebuffers(1, &depthFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, depthTex, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRBO);

    // 厚度通道 FBO（颜色附件: R32F 厚度, 无深度测试所以不需要深度附件）
    GLuint thicknessTex = createFloatTexture(SCR_WIDTH, SCR_HEIGHT);
    GLuint thicknessFBO;
    glGenFramebuffers(1, &thicknessFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, thicknessFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, thicknessTex, 0);

    // 双边滤波 ping-pong FBO（不需要深度附件，全屏四边形绘制）
    GLuint smoothTex1 = createFloatTexture(SCR_WIDTH, SCR_HEIGHT);
    GLuint smoothFBO1;
    glGenFramebuffers(1, &smoothFBO1);
    glBindFramebuffer(GL_FRAMEBUFFER, smoothFBO1);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, smoothTex1, 0);

    GLuint smoothTex2 = createFloatTexture(SCR_WIDTH, SCR_HEIGHT);
    GLuint smoothFBO2;
    glGenFramebuffers(1, &smoothFBO2);
    glBindFramebuffer(GL_FRAMEBUFFER, smoothFBO2);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, smoothTex2, 0);

    // 验证所有 FBO 的完整性
    glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR: Depth FBO 不完整！" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, thicknessFBO);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR: Thickness FBO 不完整！" << std::endl;

    // 恢复默认帧缓冲
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

    // ---- 六、创建水箱线框 VAO/VBO ----
    std::vector<float> tankVertices = buildTankVertices(fluidSystem.tankMin, fluidSystem.tankMax);

    unsigned int tankVAO, tankVBO;
    glGenVertexArrays(1, &tankVAO);
    glGenBuffers(1, &tankVBO);

    glBindVertexArray(tankVAO);
    glBindBuffer(GL_ARRAY_BUFFER, tankVBO);
    glBufferData(GL_ARRAY_BUFFER, tankVertices.size() * sizeof(float), tankVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // ---- 七、创建全屏四边形 VAO（空 VAO，顶点数据由 gl_VertexID 生成） ----
    unsigned int quadVAO;
    glGenVertexArrays(1, &quadVAO);

    // 启用点精灵程序尺寸控制
    glEnable(GL_PROGRAM_POINT_SIZE);

    std::cout << "========================================" << std::endl;
    std::cout << "操作说明：" << std::endl;
    std::cout << "  WASD      — 移动摄像机" << std::endl;
    std::cout << "  鼠标左键   — 旋转视角" << std::endl;
    std::cout << "  滚轮      — 缩放" << std::endl;
    std::cout << "  ESC       — 退出" << std::endl;
    std::cout << "========================================" << std::endl;

    // ---- 八、渲染循环 ----
    while (!glfwWindowShouldClose(myInit.window))
    {
        // 计算帧时间
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        totalTime += deltaTime;

        // 处理键盘
        processKeyboard(myInit.window);

        // ---- 更新物理模拟 ----
        if (totalTime > beginTime)
        {
            fluidSystem.update(fixedDt);
        }

        // ---- 更新粒子位置 VBO ----
        std::vector<float> newPosData = fluidSystem.getPositionData();
        glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, newPosData.size() * sizeof(float), newPosData.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // ---- 准备通用矩阵 ----
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 projection = glm::perspective(
            glm::radians(camera.Zoom),
            (float)SCR_WIDTH / (float)SCR_HEIGHT,
            0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();

        // 粒子渲染半径（取光滑核半径的90%，确保球面间有充分重叠投影）
        float particleRenderRadius = fluidSystem.h * 0.9f;

        // 点精灵投影缩放系数
        // gl_PointSize = pointScale * worldRadius / viewDepth
        // worldRadius 在 viewDepth 处的屏幕投影角度为 worldRadius/viewDepth 弧度
        // 屏幕Y方向像素密度 = screenHeight / (2*tan(fovY/2)) 像素/弧度
        // 点精灵直径 = 2 * worldRadius/viewDepth * screenHeight / (2*tan(fovY/2))
        //            = worldRadius/viewDepth * screenHeight / tan(fovY/2)
        float pointScale = SCR_HEIGHT / tan(glm::radians(camera.Zoom * 0.5f));

        // ============================================================
        // 通道1：深度渲染
        //   将粒子作为球面渲染到 depthFBO，输出视图空间线性深度
        //   同时写入硬件深度缓冲用于正确的粒子间遮挡
        // ============================================================
        glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        depthShader.use();
        depthShader.setMat4("model", model);
        depthShader.setMat4("view", view);
        depthShader.setMat4("projection", projection);
        depthShader.setFloat("particleWorldRadius", particleRenderRadius);
        depthShader.setFloat("pointScale", pointScale);
        depthShader.setFloat("nearPlane", 0.1f);
        depthShader.setFloat("farPlane", 100.0f);

        glBindVertexArray(particleVAO);
        glDrawArrays(GL_POINTS, 0, particleCount);

        // ============================================================
        // 通道2：厚度渲染
        //   加法混合累积每个像素处视线穿过流体的总厚度
        //   禁用深度测试——沿同一条视线的所有粒子都应累加厚度
        // ============================================================
        glBindFramebuffer(GL_FRAMEBUFFER, thicknessFBO);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);  // 加法混合

        thicknessShader.use();
        thicknessShader.setMat4("model", model);
        thicknessShader.setMat4("view", view);
        thicknessShader.setMat4("projection", projection);
        thicknessShader.setFloat("particleWorldRadius", particleRenderRadius);
        thicknessShader.setFloat("pointScale", pointScale);

        glDrawArrays(GL_POINTS, 0, particleCount);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_BLEND);

        // ============================================================
        // 通道3：双边滤波（4次迭代，粗→精渐进式平滑）
        //   迭代1 (粗):  大空间σ=4.0, 大深度σ=0.06  — 填充大间隙
        //   迭代2 (中粗): 中空间σ=2.5, 中深度σ=0.04  — 消除中等起伏
        //   迭代3 (中精): 小空间σ=1.5, 小深度σ=0.025 — 精细平滑
        //   迭代4 (精):  最小空间σ=1.0, 最小深度σ=0.015 — 最终抛光
        //   全部使用7×7核，在smoothTex1/smoothTex2之间乒乓切换
        // ============================================================
        int   filterRadius = 3;  // 7×7 核
        float texelW = 1.0f / SCR_WIDTH;
        float texelH = 1.0f / SCR_HEIGHT;

        bilateralShader.use();
        bilateralShader.setInt("kernelRadius", filterRadius);
        bilateralShader.setVec2("texelSize", texelW, texelH);

        glBindVertexArray(quadVAO);

        // --- 迭代1：原始深度 → smoothTex1（粗平滑，填大间隙）---
        glBindFramebuffer(GL_FRAMEBUFFER, smoothFBO1);
        glClear(GL_COLOR_BUFFER_BIT);
        bilateralShader.setFloat("sigmaSpatial", 4.0f);
        bilateralShader.setFloat("sigmaRange",   0.06f);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, depthTex);
        bilateralShader.setInt("depthTex", 0);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // --- 迭代2：smoothTex1 → smoothTex2（中粗平滑）---
        glBindFramebuffer(GL_FRAMEBUFFER, smoothFBO2);
        glClear(GL_COLOR_BUFFER_BIT);
        bilateralShader.setFloat("sigmaSpatial", 2.5f);
        bilateralShader.setFloat("sigmaRange",   0.04f);
        glBindTexture(GL_TEXTURE_2D, smoothTex1);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // --- 迭代3：smoothTex2 → smoothTex1（中精细平滑）---
        glBindFramebuffer(GL_FRAMEBUFFER, smoothFBO1);
        glClear(GL_COLOR_BUFFER_BIT);
        bilateralShader.setFloat("sigmaSpatial", 1.5f);
        bilateralShader.setFloat("sigmaRange",   0.025f);
        glBindTexture(GL_TEXTURE_2D, smoothTex2);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // --- 迭代4：smoothTex1 → smoothTex2（精平滑，最终抛光）---
        glBindFramebuffer(GL_FRAMEBUFFER, smoothFBO2);
        glClear(GL_COLOR_BUFFER_BIT);
        bilateralShader.setFloat("sigmaSpatial", 1.0f);
        bilateralShader.setFloat("sigmaRange",   0.015f);
        glBindTexture(GL_TEXTURE_2D, smoothTex1);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // ============================================================
        // 通道4：最终着色与合成
        //   从平滑深度重建世界位置 → dFdx/dFdy 计算法线 →
        //   Blinn-Phong 光照 → 厚度吸收（Beer-Lambert）→ 与背景混合
        // ============================================================
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(0.15f, 0.15f, 0.18f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);

        shadeShader.use();
        shadeShader.setMat4("invProj", glm::inverse(projection));
        shadeShader.setMat4("invView", glm::inverse(view));
        shadeShader.setVec3("viewPos", camera.Position);
        shadeShader.setVec3("lightDir", glm::normalize(glm::vec3(1.0f, 2.0f, 1.0f)));

        // 天空/环境颜色（用于Fresnel反射）
        shadeShader.setVec3("skyZenithColor",  glm::vec3(0.22f, 0.52f, 0.82f));  // 天顶深蓝
        shadeShader.setVec3("skyHorizonColor", glm::vec3(0.60f, 0.78f, 0.92f));  // 地平线浅蓝
        shadeShader.setVec3("sunColor",        glm::vec3(1.0f,  0.93f, 0.78f));  // 暖色太阳光

        // 水光学参数
        shadeShader.setFloat("absorptionCoeff", 2.0f);   // 吸收系数（与逐通道系数配合）
        shadeShader.setVec3("backgroundColor", glm::vec3(0.15f, 0.15f, 0.18f));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, smoothTex2);
        shadeShader.setInt("depthTex", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, thicknessTex);
        shadeShader.setInt("thicknessTex", 1);

        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // ============================================================
        // 通道5：水箱线框
        //   将深度缓冲从depthFBO拷贝到默认帧缓冲，使线框能被流体正确遮挡
        //   然后用半透明白色绘制水箱的12条棱
        // ============================================================
        glBindFramebuffer(GL_READ_FRAMEBUFFER, depthFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT,
                          0, 0, SCR_WIDTH, SCR_HEIGHT,
                          GL_DEPTH_BUFFER_BIT, GL_NEAREST);

        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        tankShader.use();
        tankShader.setMat4("model", glm::mat4(1.0f));
        tankShader.setMat4("view", view);
        tankShader.setMat4("projection", projection);
        tankShader.setVec3("lineColor", glm::vec3(0.7f, 0.85f, 1.0f));
        tankShader.setFloat("alpha", 0.35f);

        glBindVertexArray(tankVAO);
        glLineWidth(1.5f);
        glDrawArrays(GL_LINES, 0, 24);
        glLineWidth(1.0f);

        glDisable(GL_BLEND);

        // ---- 显示帧率到标题栏 ----
        float fps = 1.0f / (deltaTime + 0.0001f);
        char title[128];
        snprintf(title, sizeof(title), "CG FinalWork — SSFR | FPS: %.0f | 粒子数: %d",
                 fps, particleCount);
        glfwSetWindowTitle(myInit.window, title);

        // 交换缓冲
        glfwSwapBuffers(myInit.window);
        glfwPollEvents();
    }

    // ---- 九、清理资源 ----
    glDeleteVertexArrays(1, &particleVAO);
    glDeleteBuffers(1, &particleVBO);
    glDeleteVertexArrays(1, &tankVAO);
    glDeleteBuffers(1, &tankVBO);
    glDeleteVertexArrays(1, &quadVAO);

    glDeleteTextures(1, &depthTex);
    glDeleteTextures(1, &thicknessTex);
    glDeleteTextures(1, &smoothTex1);
    glDeleteTextures(1, &smoothTex2);

    glDeleteRenderbuffers(1, &depthRBO);

    glDeleteFramebuffers(1, &depthFBO);
    glDeleteFramebuffers(1, &thicknessFBO);
    glDeleteFramebuffers(1, &smoothFBO1);
    glDeleteFramebuffers(1, &smoothFBO2);

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
        leftButtonPressed = (action == GLFW_PRESS);
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
