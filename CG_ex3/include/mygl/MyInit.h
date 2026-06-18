#ifndef MYINIT_H
#define MYINIT_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>

class MyInit
{
public:
    GLFWwindow* window;
    MyInit(int width, int height, const char* title)
    {
        Init();
        CreateWindow(width, height, title);
    }

    void Init()
    {
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    }

    void CreateWindow(int width, int height, const char* title)
    {
        window = glfwCreateWindow(width, height, title, NULL, NULL);
        if (window == NULL)
        {
            std::cout << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return;
        }
        glfwMakeContextCurrent(window);
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            std::cout << "Failed to initialize GLAD" << std::endl;
            return;
        }
    }
    //--回调函数设置--
    void SetFramebufferSizeCallback(void (*callback)(GLFWwindow*, int, int))
    {
        glfwSetFramebufferSizeCallback(window, callback);
    }

    void SetCursorPosCallback(void (*callback)(GLFWwindow*, double, double))
    {
        glfwSetCursorPosCallback(window, callback);
    }

    void SetScrollCallback(void (*callback)(GLFWwindow*, double, double))
    {
        glfwSetScrollCallback(window, callback);
    }
    void SetMouseButtonCallback(void (*callback)(GLFWwindow*, int, int,int))
    {
        glfwSetMouseButtonCallback(window, callback);
    }

    //--其他设置--
    void SetInputMode(int mode, int value)
    {
        glfwSetInputMode(window, mode, value);
    }

    //--OpenGL全局设置--
    void EnableDepthTest()
    {
        glEnable(GL_DEPTH_TEST);
    }

    //析构函数
    ~MyInit()
    {
        glfwTerminate();
    }
};

#endif