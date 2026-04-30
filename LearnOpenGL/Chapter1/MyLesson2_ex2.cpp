#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

//屏幕大小设置
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

//顶点着色器源码
const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
    "}\0";

const char *fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
    "}\n\0";
 
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


    #pragma region 着色器编译和程序链接
    //创建和编译顶点着色器
    unsigned int vertexShader; //顶点着色器对象ID
    vertexShader = glCreateShader(GL_VERTEX_SHADER); //创建一个顶点着色器对象 参数为着色器类型 创建顶点着色器的参数是GL_VERTEX_SHADER
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL); //将顶点着色器源码附加到着色器对象上 参数为着色器对象ID、源码字符串数量、源码字符串数组和每个字符串的长度（可以为NULL表示字符串以NULL结尾）
    glCompileShader(vertexShader); //编译着色器对象
    //检查编译是否成功
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    //创建和编译片段着色器
    unsigned int fragmentShader; //片段着色器对象ID
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER); //创建一个片段着色器对象 参数为着色器类型 创建片段着色器的参数是GL_FRAGMENT_SHADER
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL); //将片段着色器源码附加到着色器对象上
    glCompileShader(fragmentShader); //编译着色器对象
    //检查编译是否成功
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    //链接着色器程序
    unsigned int shaderProgram; //着色器程序对象ID
    shaderProgram = glCreateProgram(); //创建一个着色器程序对象 返回新创建程序对象的ID引用
    glAttachShader(shaderProgram, vertexShader); //将顶点着色器附加到程序对象上 参数为程序对象ID和着色器对象ID
    glAttachShader(shaderProgram, fragmentShader); //将片段着色器附加到程序对象上
    glLinkProgram(shaderProgram); //链接着色器程序 参数为程序对象ID
    //检查链接是否成功
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if(!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    //链接完成后 可以删除着色器对象 因为它们已经链接到程序对象上 不再需要了
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    #pragma endregion


    #pragma region 顶点数据和缓冲配置
    //--设置顶点数据--
    float firstTriangle[] = {
        -0.9f, -0.5f, 0.0f,  // left 
        -0.0f, -0.5f, 0.0f,  // right
        -0.45f, 0.5f, 0.0f,  // top 
    };
    float secondTriangle[] = {
        0.0f, -0.5f, 0.0f,  // left
        0.9f, -0.5f, 0.0f,  // right
        0.45f, 0.5f, 0.0f   // top 
    };

    //--创建--
    unsigned int VAO[2]; //顶点数组对象 存储顶点属性配置的对象ID
    unsigned int VBO[2]; //顶点缓冲对象 存储顶点数据的缓冲对象ID
    
    //--生成--
    glGenVertexArrays(2, VAO); //生成顶点数组对象 参数为 生成的顶点数组对象数量 和 存储生成的顶点数组对象ID的变量地址
    glGenBuffers(2, VBO); //生成缓冲对象 参数为 生成的缓冲对象数量 和 存储生成的缓冲对象ID的变量地址
    
    //--绑定--
    //第一个三角形
    glBindVertexArray(VAO[0]); //绑定VAO
    glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(firstTriangle), firstTriangle, GL_STATIC_DRAW);
    //设置顶点属性指针
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    //第二个三角形
    glBindVertexArray(VAO[1]); //绑定VAO
    glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(secondTriangle), secondTriangle, GL_STATIC_DRAW);
    //设置顶点属性指针
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    //--解绑--
    glBindVertexArray(0);  //解绑顶点数组对象 参数为 0 表示解绑当前绑定的顶点数组对象
    glBindBuffer(GL_ARRAY_BUFFER, 0); //解绑缓冲对象 参数为缓冲对象类型 和 0 表示解绑当前绑定的缓冲对象
    #pragma endregion
    
    while(!glfwWindowShouldClose(window)) //主循环 直到用户关闭窗口
    {
        processInput(window); //处理输入

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f); //设置清空屏幕所用的颜色
        glClear(GL_COLOR_BUFFER_BIT); //清空颜色缓冲区 用glClearColor设置的颜色来填充整个窗口 参数为要清空的缓冲区 这里只清空颜色缓冲区
                                      //FBO是容器 缓冲区是内容
        
        glUseProgram(shaderProgram); //激活着色器程序 参数为程序对象ID 之后所有的渲染调用都会使用这个程序对象
        glBindVertexArray(VAO[0]); //绑定VAO 参数为要绑定的VAO的ID
        glDrawArrays(GL_TRIANGLES, 0, 3); //绘制图元 参数为图元类型、起始索引和顶点数量
        glBindVertexArray(VAO[1]); //绑定VAO 参数为要绑定的VAO的ID
        glDrawArrays(GL_TRIANGLES, 0, 3); //绘制图元 参数为图元类型、起始索引和顶点数量
        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); //设置线框模式 参数为要设置的面类型和模式 这里设置为线框模式 这样就可以看到绘制的三角形的边界线了
        glBindVertexArray(0); //解绑VAO 参数为 0 表示解绑当前绑定的VAO

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