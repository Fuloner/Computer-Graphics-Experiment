#version 330 core
// 片段着色器 —— 接收一个统一的颜色值并输出，用于第1步的简单渲染验证

in vec3 WorldPos;           // 从顶点着色器接收的世界空间位置

out vec4 FragColor;         // 最终输出的片段颜色

uniform vec3 objectColor;   // 物体颜色（由C++端设置）

void main()
{
    FragColor = vec4(objectColor, 1.0);
}
