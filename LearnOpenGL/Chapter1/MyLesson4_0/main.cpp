#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <learnopengl/shader_s.h>

#include <iostream>
#include <cmath>
#include <vector>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

//屏幕大小设置
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

int main()
{
    glfwInit(); //初始化GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); //使用glfwWindowHint函数来配置GLFW 主版本号
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); //次版本号
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //使用核心模式
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); //Mac OS X系统需要加上这行代码

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGLFirstExperiment", NULL, NULL); //创建一个窗口对象 参数为宽度、高度、标题
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window); //将刚刚创建的窗口设置为当前线程的主上下文
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback); //注册窗口大小变化的回调函数

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) //初始化GLAD 加载OpenGL函数指针
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }


    Shader ourShader("src/vertex_shader.vs", "src/fragment_shader.fs"); // 创建着色器对象

    #pragma region 顶点数据和缓冲配置
    //设置顶点数据和颜色
    float vertices[] =
    {
    //     ---- 位置 ----       ---- 颜色 ----     - 纹理坐标 -
        0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // 右上
        0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   // 右下
        -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // 左下
        -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f    // 左上
    };

    unsigned int indices[] =
    {  
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };

    //--创建--
    unsigned int VAO; //顶点数组对象 存储顶点属性配置的对象ID
    unsigned int VBO; //顶点缓冲对象 存储顶点数据的缓冲对象ID
    unsigned int EBO; //元素缓冲对象 存储索引数据的缓冲对象ID
    
    //--生成--
    glGenVertexArrays(1, &VAO); //生成顶点数组对象 参数为 生成的顶点数组对象数量 和 存储生成的顶点数组对象ID的变量地址
    glGenBuffers(1, &VBO); //生成缓冲对象 参数为 生成的缓冲对象数量 和 存储生成的缓冲对象ID的变量地址
    glGenBuffers(1, &EBO); //生成元素缓冲对象 参数为 生成的元素缓冲对象数量 和 存储生成的元素缓冲对象ID的变量地址
    
    //--绑定--
    glBindVertexArray(VAO); //绑定顶点数组对象 参数为要绑定的顶点数组对象ID 之后所有的顶点属性配置和调用都会作用在这个VAO上
    glBindBuffer(GL_ARRAY_BUFFER, VBO); //绑定缓冲对象 参数为缓冲对象类型 和 要绑定的缓冲对象ID 因为VBO类型是GL_ARRAY_BUFFER 所以参数为GL_ARRAY_BUFFER
                                        //之后 所有的GL_ARRAY_BUFFER类型的缓冲操作都会作用在这个VBO上
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); //将顶点数据复制到缓冲对象中 参数为缓冲对象类型、数据大小、数据指针（实际数据）和使用方式
                                                                               //GL_STATIC_DRAW 表示 数据不会或几乎不会改变
                                                                               //GL_DYNAMIC_DRAW 表示 数据会被改变很多次
                                                                               //GL_STREAM_DRAW 表示 数据每次绘制都会改变
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    //--设置顶点属性指针--
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    //--解绑--
    glBindVertexArray(0);  //解绑顶点数组对象 参数为 0 表示解绑当前绑定的顶点数组对象
    glBindBuffer(GL_ARRAY_BUFFER, 0); //解绑缓冲对象 参数为缓冲对象类型 和 0 表示解绑当前绑定的缓冲对象
    #pragma endregion


    #pragma region 加载并生成纹理
    unsigned int texture; //纹理对象ID
    glGenTextures(1, &texture); //生成纹理对象 参数为生成的纹理对象数量 和 存储生成的纹理对象ID的变量地址
    glBindTexture(GL_TEXTURE_2D, texture); //绑定纹理对象 参数为纹理类型 和 要绑定的纹理对象ID

    //设置纹理环绕参数
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    //设置纹理过滤参数
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    int width, height, nrChannels;
    unsigned char *data = stbi_load("texture/container.jpg", &width, &height, &nrChannels, 0);

    if(data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data); //生成纹理 参数为纹理类型、纹理级别、纹理内部格式、宽度、高度、边框、数据格式、数据类型和数据指针
                                                                                                  //纹理类型是要创建的纹理的类型 这里是GL_TEXTURE_2D
                                                                                                  //纹理级别是指定MIPMAP级别 这里是0 表示基本级别
                                                                                                  //纹理内部格式是指定OpenGL如何存储纹理数据 这里是GL_RGB 表示每个像素有3个颜色分量
                                                                                                  //宽度和高度是纹理的尺寸 这里使用stbi_load函数返回的宽度和高度
                                                                                                  //边框必须为0
                                                                                                  //数据格式是指定输入数据的格式 这里是GL_RGB 因为我们加载的图片有3个颜色分量
                                                                                                  //数据类型是指定输入数据的数据类型 这里是GL_UNSIGNED_BYTE 因为图片数据是无符号字节
                                                                                                  //数据指针是实际的图像数据 这里是stbi_load函数返回的数据指针
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data); //释放图像内存
    #pragma endregion
    

    while(!glfwWindowShouldClose(window)) //主循环 直到用户关闭窗口
    {
        processInput(window); //处理输入

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f); //设置清空屏幕所用的颜色
        glClear(GL_COLOR_BUFFER_BIT); //清空颜色缓冲区 用glClearColor设置的颜色来填充整个窗口 参数为要清空的缓冲区 这里只清空颜色缓冲区
                                      //FBO是容器 缓冲区是内容
        
        ourShader.use(); //激活着色器程序

        glBindTexture(GL_TEXTURE_2D, texture); //自动把纹理赋值给片段着色器的采样器

        glBindVertexArray(VAO); //绑定VAO 参数为要绑定的VAO的ID
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0); //解绑VAO 参数为 0 表示解绑当前绑定的VAO

        glfwSwapBuffers(window); //双缓冲 交换前后缓冲区 显示渲染结果
        glfwPollEvents(); //检查是否有事件发生 如键盘输入 鼠标移动等 如果有的话 就调用对应的回调函数
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    
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