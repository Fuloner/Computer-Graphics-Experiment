#version 330 core

/// @brief 水箱线框片元着色器 —— 输出半透明白色线条

out vec4 FragColor;

uniform vec3 lineColor;   // 线条颜色
uniform float alpha;      // 透明度

void main()
{
    FragColor = vec4(lineColor, alpha);
}
