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
//#include "mygl/MyVBO.h"
//#include "mygl/MyVAO.h"
//#include "mygl/MyTexture.h"
#include "mygl/MyMesh.h"
#include "mygl/MyModel.h"
#include "mygl/DirLight.h"
#include "mygl/PointLight.h"
#include "mygl/SpotLight.h"

//--C++标准库--
#include <iostream>
#include <cmath>
#include <vector>

//--函数声明--
//回调函数
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
//其他函数
void processInput(GLFWwindow *window);
void LightUI(DirLight& dirLight, std::vector<PointLight>& pointLights, std::vector<SpotLight>& spotLights, int& pointLightNum, int& spotLightNum);

//--全局设置--
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;
int moveMode = 1;
int shadeMode = 0;

//--摄像机设置--
MyCamera myCamera(glm::vec3(0.0f, 0.5f, 3.0f));
float lastX = SCR_WIDTH / 2.0;
float lastY = SCR_HEIGHT / 2.0;
bool firstMouse = true;

//--时间设置--
float deltaTime = 0.0f;
float lastFrame = 0.0f;

//--主函数--
int main()
{
	//--初始化--
	MyInit myInit(SCR_WIDTH, SCR_HEIGHT, "First Experiment");
	myInit.SetFramebufferSizeCallback(framebuffer_size_callback);
	myInit.SetCursorPosCallback(mouse_callback);
	myInit.SetScrollCallback(scroll_callback);
	myInit.SetInputMode(GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	myInit.EnableDepthTest();

	//--着色器--
	MyShader myShader("src/vertex_shader.glsl", "src/fragment_shader.glsl");

	//--模型--
	MyModel myModel("model/batman/batman.obj");

	//--光源--
    int dirLightNum = 1;
    int pointLightNum = 0;
    int spotLightNum = 0;
    DirLight dirLight(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.05f, 0.05f, 0.05f), glm::vec3(0.4f, 0.4f, 0.4f), glm::vec3(0.5f, 0.5f, 0.5f));
    std::vector<PointLight> pointLights;
    std::vector<SpotLight> spotLights;
    for(int i = 0; i < 16; i++)
    {
        pointLights.emplace_back(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.1f, 0.1f, 0.1f), glm::vec3(0.8f, 0.8f, 0.8f), glm::vec3(1.0f, 1.0f, 1.0f), 1.0f, 0.09f, 0.032f);
        spotLights.emplace_back(glm::vec3(0.0f, 3.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.1f, 0.1f, 0.1f), glm::vec3(0.8f, 0.8f, 0.8f), glm::vec3(1.0f, 1.0f, 1.0f), glm::cos(glm::radians(12.5f)), glm::cos(glm::radians(15.0f)), 1.0f, 0.09f, 0.032f);
    }

    //--imGui设置--
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    
    ImGui::StyleColorsDark();

    //字体设置
    io.Fonts->AddFontFromFileTTF("Fonts/simsun.ttc", 14.0f, nullptr, io.Fonts->GetGlyphRangesChineseFull());
    
    ImGui_ImplGlfw_InitForOpenGL(myInit.window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

	//--渲染循环--
	while (!glfwWindowShouldClose(myInit.window))
	{
		//--时间逻辑--
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		//--输入处理--
		processInput(myInit.window);

		//--渲染指令--
		//-背景渲染-
		glClearColor(0.3f, 0.8f, 0.8f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //--移动模式--
        if (moveMode == 1)
        {
            myInit.SetInputMode(GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        else if (moveMode == 0)
        {
            myInit.SetInputMode(GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }

		//-物体渲染-
		//基本设置
		myShader.use();
        myShader.setVec3("viewPos", myCamera.Position);
        myShader.setInt("shadeMode", shadeMode);

        //光照设置
        myShader.setInt("dirLightNum", dirLightNum);
        dirLight.setUniform(myShader, "dirLights[0]");
        
        myShader.setInt("pointLightNum", pointLightNum);
        for(int i = 0; i < pointLightNum; i++)
        {
            pointLights[i].setUniform(myShader, "pointLights[" + std::to_string(i) + "]");
        }
        //pointLight.setUniform(myShader, "pointLights[0]");
        //pointLight2.setUniform(myShader, "pointLights[1]");

        myShader.setInt("spotLightNum", spotLightNum);
        for(int i = 0; i < spotLightNum; i++)
        {
            spotLights[i].setUniform(myShader, "spotLights[" + std::to_string(i) + "]");
        }
        //spotLight.setUniform(myShader, "spotLights[0]");
        
        //设置投影和视图矩阵
        glm::mat4 projection = glm::perspective(glm::radians(myCamera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = myCamera.GetViewMatrix();
        myShader.setMat4("projection", projection);
        myShader.setMat4("view", view);

        //设置模型矩阵
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
        myShader.setMat4("model", model);
        myModel.Draw(myShader);

        //--imGui渲染--
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(300, 600), ImGuiCond_Always);

        ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
        ImGui::Text("按 Left Ctrl/Alt 以切换鼠标控制模式");
        const char* items[] = { "法线渲染模式", "漫反射渲染模式", "法线贴图模式", "纯色模式", "法线纯色模式", "光照模式" };
        ImGui::Combo("着色模型", &shadeMode, items, IM_ARRAYSIZE(items));

        LightUI(dirLight, pointLights, spotLights, pointLightNum, spotLightNum);

        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		//--交换缓冲区和轮询IO事件--
		glfwSwapBuffers(myInit.window);
		glfwPollEvents();
	}

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

	//--资源释放--
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
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        myCamera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        myCamera.ProcessKeyboard(DOWN, deltaTime);
    
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        moveMode = 1;
    if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS)
        moveMode = 0;
}


void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
    {
        return;
    }

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

    if(moveMode == 0)
    {
        myCamera.ProcessMouseMovement(xoffset, yoffset);
    }
    else if(moveMode == 1 && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        myCamera.ProcessMouseLeftDrag(xoffset, yoffset);
    }
    else if(moveMode == 1 && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
    {
        myCamera.ProcessMouseRightDrag(xoffset, yoffset);
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
    {
        return;
    }
    myCamera.ProcessMouseScroll(yoffset);
}

void LightUI(DirLight& dirLight, std::vector<PointLight>& pointLights, std::vector<SpotLight>& spotLights, int& pointLightNum, int& spotLightNum)
{
    if (shadeMode == 5)
    {
        ImGui::Separator();
        ImGui::Text("光源设置");

        // 方向光
        if (ImGui::CollapsingHeader("方向光"))
        {
            glm::vec3 dir = dirLight.getDirection();
            if (ImGui::SliderFloat3("方向", glm::value_ptr(dir), -1.0f, 1.0f))
                dirLight.setDirection(dir);
            glm::vec3 ambient = dirLight.getAmbient();
            if (ImGui::ColorEdit3("环境光", glm::value_ptr(ambient)))
                dirLight.setAmbient(ambient);
            glm::vec3 diffuse = dirLight.getDiffuse();
            if (ImGui::ColorEdit3("漫反射", glm::value_ptr(diffuse)))
                dirLight.setDiffuse(diffuse);
            glm::vec3 specular = dirLight.getSpecular();
            if (ImGui::ColorEdit3("镜面反射", glm::value_ptr(specular)))
                dirLight.setSpecular(specular);
        }

        // 点光源
        if (ImGui::CollapsingHeader("点光源"))
        {
            ImGui::SliderInt("点光源数量", &pointLightNum, 0, (int)pointLights.size());
            for (int i = 0; i < pointLightNum; ++i)
            {
                if (ImGui::TreeNode((void*)(intptr_t)i, "点光源 %d", i + 1))
                {
                    PointLight& pl = pointLights[i];
                    glm::vec3 pos = pl.getPosition();
                    if (ImGui::DragFloat3("位置", glm::value_ptr(pos), 0.1f))
                        pl.setPosition(pos);
                    glm::vec3 ambient = pl.getAmbient();
                    if (ImGui::ColorEdit3("环境光", glm::value_ptr(ambient)))
                        pl.setAmbient(ambient);
                    glm::vec3 diffuse = pl.getDiffuse();
                    if (ImGui::ColorEdit3("漫反射", glm::value_ptr(diffuse)))
                        pl.setDiffuse(diffuse);
                    glm::vec3 specular = pl.getSpecular();
                    if (ImGui::ColorEdit3("镜面反射", glm::value_ptr(specular)))
                        pl.setSpecular(specular);
                    float constant = pl.getConstant(), linear = pl.getLinear(), quadratic = pl.getQuadratic();
                    if (ImGui::DragFloat("常数衰减", &constant, 0.01f, 0.0f, 10.0f)) pl.setConstant(constant);
                    if (ImGui::DragFloat("线性衰减", &linear, 0.01f, 0.0f, 10.0f)) pl.setLinear(linear);
                    if (ImGui::DragFloat("二次衰减", &quadratic, 0.01f, 0.0f, 10.0f)) pl.setQuadratic(quadratic);
                    ImGui::TreePop();
                }
            }
        }

        // 聚光源
        if (ImGui::CollapsingHeader("聚光源"))
        {
            ImGui::SliderInt("聚光源数量", &spotLightNum, 0, (int)spotLights.size());
            for (int i = 0; i < spotLightNum; ++i)
            {
                if (ImGui::TreeNode((void*)(intptr_t)i, "聚光源 %d", i + 1))
                {
                    SpotLight& sl = spotLights[i];
                    glm::vec3 pos = sl.getPosition();
                    if (ImGui::DragFloat3("位置", glm::value_ptr(pos), 0.1f))
                        sl.setPosition(pos);
                    glm::vec3 dir = sl.getDirection();
                    if (ImGui::DragFloat3("方向", glm::value_ptr(dir), 0.05f, -1.0f, 1.0f))
                        sl.setDirection(glm::normalize(dir));
                    float cutOff = glm::degrees(acos(sl.getCutOff()));
                    float outerCutOff = glm::degrees(acos(sl.getOuterCutOff()));
                    if (ImGui::SliderFloat("内切角", &cutOff, 0.0f, 90.0f))
                        sl.setCutOff(glm::cos(glm::radians(cutOff)));
                    if (ImGui::SliderFloat("外切角", &outerCutOff, 0.0f, 90.0f))
                        sl.setOuterCutOff(glm::cos(glm::radians(outerCutOff)));
                    // 颜色和衰减同点光源...
                    glm::vec3 ambient = sl.getAmbient();
                    if (ImGui::ColorEdit3("环境光", glm::value_ptr(ambient)))
                        sl.setAmbient(ambient);
                    glm::vec3 diffuse = sl.getDiffuse();
                    if (ImGui::ColorEdit3("漫反射", glm::value_ptr(diffuse)))
                        sl.setDiffuse(diffuse);
                    glm::vec3 specular = sl.getSpecular();
                    if (ImGui::ColorEdit3("镜面反射", glm::value_ptr(specular)))
                        sl.setSpecular(specular);
                    float constant = sl.getConstant(), linear = sl.getLinear(), quadratic = sl.getQuadratic();
                    if (ImGui::DragFloat("常数衰减", &constant, 0.01f, 0.0f, 10.0f)) sl.setConstant(constant);
                    if (ImGui::DragFloat("线性衰减", &linear, 0.01f, 0.0f, 10.0f)) sl.setLinear(linear);
                    if (ImGui::DragFloat("二次衰减", &quadratic, 0.01f, 0.0f, 10.0f)) sl.setQuadratic(quadratic);
                    ImGui::TreePop();
                }
            }
        }
    }
}