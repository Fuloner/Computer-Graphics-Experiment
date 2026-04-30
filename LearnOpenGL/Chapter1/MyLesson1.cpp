#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
 
int main()
{
    glfwInit(); //初始化GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); //使用glfwWindowHint函数来配置GLFW 主版本号
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); //次版本号
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //使用核心模式
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); //Mac OS X系统需要加上这行代码

    GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGLFirstExperiment", NULL, NULL); //创建一个窗口对象 参数为宽度、高度、标题
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window); //将刚刚创建的窗口设置为当前线程的主上下文

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) //初始化GLAD 加载OpenGL函数指针
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback); //注册窗口大小变化的回调函数

    while(!glfwWindowShouldClose(window)) //主循环 直到用户关闭窗口
    {
        processInput(window); //处理输入

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f); //设置清空屏幕所用的颜色
        glClear(GL_COLOR_BUFFER_BIT); //清空颜色缓冲区 用glClearColor设置的颜色来填充整个窗口 参数为要清空的缓冲区 这里只清空颜色缓冲区
        //FBO是容器 缓冲区是内容

        glfwSwapBuffers(window); //双缓冲 交换前后缓冲区 显示渲染结果
        glfwPollEvents(); //检查是否有事件发生 如键盘输入 鼠标移动等 如果有的话 就调用对应的回调函数
    }
    
    glfwTerminate(); //销毁窗口 释放资源
    return 0;
}

/// @brief 当窗口大小改变时 调整视口大小
/// @param window 窗口对象
/// @param width 新的宽度
/// @param height 新的高度
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);//设置视口大小 参数为视口左下角的坐标和宽度、高度
    //实际上也可以将视口的维度设置为比GLFW的维度小
    //之后所有的OpenGL渲染将会在一个更小的窗口中显示
    //这样的话就可以将一些其它元素显示在OpenGL视口之外。
}

/// @brief 处理输入
/// @param window 窗口对象
void processInput(GLFWwindow *window)
{
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}