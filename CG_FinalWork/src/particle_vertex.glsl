#version 330 core
// 粒子顶点着色器 —— 接收每个粒子的位置和颜色，变换到裁剪空间
// 同时根据距离计算点精灵的屏幕大小

layout (location = 0) in vec3 aPos;     // 粒子的世界空间位置
layout (location = 1) in vec3 aColor;   // 粒子的逐粒子颜色（由邻居数量决定）

uniform mat4 model;       // 模型矩阵（作用于所有粒子，通常为单位矩阵）
uniform mat4 view;        // 视图矩阵
uniform mat4 projection;  // 投影矩阵
uniform float pointScale; // 点精灵缩放因子（控制粒子在屏幕上的显示大小）

out vec3 vColor;          // 传递给片段着色器的颜色

void main()
{
    // 计算世界空间位置
    vec3 worldPos = vec3(model * vec4(aPos, 1.0));
    // 标准MVP变换到裁剪空间
    vec4 clipPos = projection * view * vec4(worldPos, 1.0);
    gl_Position = clipPos;

    // 点精灵大小随距离衰减，模拟近大远小的透视效果
    gl_PointSize = pointScale / (-clipPos.w);

    // 限制点精灵的像素尺寸范围
    gl_PointSize = clamp(gl_PointSize, 1.5, 40.0);

    // 将颜色透传给片段着色器
    vColor = aColor;
}
