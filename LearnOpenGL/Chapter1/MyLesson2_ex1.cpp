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
    float vertices[] = {
        // first triangle
        -0.9f, -0.5f, 0.0f,  // left 
        -0.0f, -0.5f, 0.0f,  // right
        -0.45f, 0.5f, 0.0f,  // top 
        // second triangle
         0.0f, -0.5f, 0.0f,  // left
         0.9f, -0.5f, 0.0f,  // right
         0.45f, 0.5f, 0.0f   // top 
    }; 

    //--创建--
    unsigned int VAO; //顶点数组对象 存储顶点属性配置的对象ID
    unsigned int VBO; //顶点缓冲对象 存储顶点数据的缓冲对象ID
    //unsigned int EBO; //元素缓冲对象 存储索引数据的缓冲对象ID
    
    //--生成--
    glGenVertexArrays(1, &VAO); //生成顶点数组对象 参数为 生成的顶点数组对象数量 和 存储生成的顶点数组对象ID的变量地址
    glGenBuffers(1, &VBO); //生成缓冲对象 参数为 生成的缓冲对象数量 和 存储生成的缓冲对象ID的变量地址
    //glGenBuffers(1, &EBO); //生成元素缓冲对象 参数同上
    
    //--绑定--
    glBindVertexArray(VAO); //绑定顶点数组对象 参数为要绑定的顶点数组对象ID 之后所有的顶点属性配置和调用都会作用在这个VAO上
    glBindBuffer(GL_ARRAY_BUFFER, VBO); //绑定缓冲对象 参数为缓冲对象类型 和 要绑定的缓冲对象ID 因为VBO类型是GL_ARRAY_BUFFER 所以参数为GL_ARRAY_BUFFER
                                        //之后 所有的GL_ARRAY_BUFFER类型的缓冲操作都会作用在这个VBO上
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); //将顶点数据复制到缓冲对象中 参数为缓冲对象类型、数据大小、数据指针（实际数据）和使用方式
                                                                               //GL_STATIC_DRAW 表示 数据不会或几乎不会改变
                                                                               //GL_DYNAMIC_DRAW 表示 数据会被改变很多次
                                                                               //GL_STREAM_DRAW 表示 数据每次绘制都会改变
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO); //绑定元素缓冲对象 参数为缓冲对象类型 和 要绑定的缓冲对象ID 因为EBO类型是GL_ELEMENT_ARRAY_BUFFER 所以参数为GL_ELEMENT_ARRAY_BUFFER
    //glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); //将索引数据复制到元素缓冲对象中 参数同上
    
    //--设置顶点属性指针--
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0); //设置顶点属性指针 参数为属性位置、属性大小、数据类型、是否归一化、步长和偏移量
                                                                                  //告诉OpenGL该如何解析顶点数据 以便在渲染时正确地使用这些数据
                                                                                  //属性位置是我们在顶点着色器中使用layout(location = 0)指定的属性位置 这里是0
                                                                                  //属性大小是每个顶点属性的组件数量 这里是3 因为每个顶点有3个坐标值
                                                                                  //数据类型是每个组件的数据类型 这里是GL_FLOAT 因为顶点坐标是浮点数
                                                                                  //是否归一化表示是否将整数数据归一化为[0,1]或[-1,1]之间的值 这里是GL_FALSE 因为顶点坐标已经是浮点数了 不需要归一化
                                                                                  //步长是连续顶点属性之间的字节偏移量 这里是3 * sizeof(float) 因为每个顶点有3个浮点数 每个浮点数占4字节 所以每个顶点占12字节
                                                                                  //偏移量是第一个顶点属性在缓冲中起始位置的字节偏移量 这里是0 因为顶点属性在数组的开头
    glEnableVertexAttribArray(0); //启用顶点属性数组 参数为属性位置 之前设置的属性位置是0 所以参数为0

    //--解绑--
    glBindVertexArray(0);  //解绑顶点数组对象 参数为 0 表示解绑当前绑定的顶点数组对象
    glBindBuffer(GL_ARRAY_BUFFER, 0); //解绑缓冲对象 参数为缓冲对象类型 和 0 表示解绑当前绑定的缓冲对象
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); //解绑元素缓冲对象 参数同上
    #pragma endregion
    
    while(!glfwWindowShouldClose(window)) //主循环 直到用户关闭窗口
    {
        processInput(window); //处理输入

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f); //设置清空屏幕所用的颜色
        glClear(GL_COLOR_BUFFER_BIT); //清空颜色缓冲区 用glClearColor设置的颜色来填充整个窗口 参数为要清空的缓冲区 这里只清空颜色缓冲区
                                      //FBO是容器 缓冲区是内容
        
        glUseProgram(shaderProgram); //激活着色器程序 参数为程序对象ID 之后所有的渲染调用都会使用这个程序对象
        glBindVertexArray(VAO); //绑定VAO 参数为要绑定的VAO的ID
        glDrawArrays(GL_TRIANGLES, 0, 6); //绘制图元 参数为图元类型、起始索引和顶点数量
        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); //设置线框模式 参数为要设置的面类型和模式 这里设置为线框模式 这样就可以看到绘制的三角形的边界线了
        //glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0); //绘制图元 参数为图元类型、索引数量、索引数据类型和索引数据的偏移量 因为EBO已经绑定了 所以索引数据的偏移量为0
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