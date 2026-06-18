#version 330 core

/// @brief 水箱线框顶点着色器 —— 渲染透明包围盒的线框

layout (location = 0) in vec3 aPos;  // 顶点的局部坐标（12条棱共24个顶点，用GL_LINES绘制）

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
