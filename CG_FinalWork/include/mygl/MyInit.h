/// @brief 窗口与OpenGL初始化封装类
/// <summary>
/// 封装GLFW窗口创建、OpenGL上下文初始化（通过GLAD加载函数指针）、
/// 各类回调函数的注册以及OpenGL全局状态设置。
/// </summary>
#ifndef MYINIT_H
#define MYINIT_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>

class MyInit
{
public:
    GLFWwindow* window = nullptr; // GLFW窗口句柄（初始化为空指针，构造时赋值）

    /// <summary>
    /// 构造时自动初始化GLFW并创建窗口，同时加载OpenGL函数
    /// </summary>
    /// <param name="width">窗口宽度（像素）</param>
    /// <param name="height">窗口高度（像素）</param>
    /// <param name="title">窗口标题</param>
    MyInit(int width, int height, const char* title)
    {
        Init();
        CreateGLFWWindow(width, height, title);
    }

    /// <summary>
    /// 初始化GLFW库，设置OpenGL版本为3.3核心模式
    /// </summary>
    void Init()
    {
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    }

    /// <summary>
    /// 创建指定尺寸和标题的窗口，并设置当前OpenGL上下文
    /// 同时通过GLAD加载所有OpenGL函数指针
    /// </summary>
    void CreateGLFWWindow(int width, int height, const char* title)
    {
        window = glfwCreateWindow(width, height, title, NULL, NULL);
        if (window == NULL)
        {
            std::cout << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return;
        }
        glfwMakeContextCurrent(window);

        // 加载OpenGL函数指针（GLAD必须在有GL上下文后调用）
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            std::cout << "Failed to initialize GLAD" << std::endl;
            return;
        }
    }

    // ======================== 回调函数注册 ========================

    /// <summary>
    /// 注册窗口尺寸变化回调 —— 当用户拖拽窗口边缘时触发
    /// </summary>
    void SetFramebufferSizeCallback(void (*callback)(GLFWwindow*, int, int))
    {
        glfwSetFramebufferSizeCallback(window, callback);
    }

    /// <summary>
    /// 注册鼠标移动回调 —— 每当鼠标在窗口内移动时触发
    /// </summary>
    void SetCursorPosCallback(void (*callback)(GLFWwindow*, double, double))
    {
        glfwSetCursorPosCallback(window, callback);
    }

    /// <summary>
    /// 注册鼠标滚轮回调 —— 当用户滚动滚轮时触发
    /// </summary>
    void SetScrollCallback(void (*callback)(GLFWwindow*, double, double))
    {
        glfwSetScrollCallback(window, callback);
    }

    /// <summary>
    /// 注册鼠标按键回调 —— 当鼠标按键被按下或释放时触发
    /// </summary>
    void SetMouseButtonCallback(void (*callback)(GLFWwindow*, int, int, int))
    {
        glfwSetMouseButtonCallback(window, callback);
    }

    // ======================== 其他工具方法 ========================

    /// <summary>
    /// 设置输入模式（如光标隐藏/锁定等）
    /// 用法示例：SetInputMode(GLFW_CURSOR, GLFW_CURSOR_DISABLED)
    /// </summary>
    void SetInputMode(int mode, int value)
    {
        glfwSetInputMode(window, mode, value);
    }

    /// <summary>
    /// 开启深度测试 —— 使OpenGL正确渲染3D物体的前后遮挡关系
    /// </summary>
    void EnableDepthTest()
    {
        glEnable(GL_DEPTH_TEST);
    }

    // 析构函数 —— 清理GLFW资源
    ~MyInit()
    {
        glfwTerminate();
    }
};

#endif
