#version 330 core

/// @brief 全屏四边形顶点着色器 —— 使用无顶点缓冲区的技巧渲染覆盖全屏的三角形
/// 利用 gl_VertexID（内置顶点索引）生成三个顶点，形成一个刚好覆盖整个屏幕的大三角形。
/// 顶点位置硬编码在着色器中，无需 VBO/VAO 数据。
/// 用于所有后处理通道（双边滤波、最终着色合成）。

const vec2 positions[3] = vec2[](
    vec2(-1.0, -1.0),  // 左下
    vec2( 3.0, -1.0),  // 右下（超出屏幕右侧，GPU自动裁剪）
    vec2(-1.0,  3.0)   // 左上（超出屏幕顶部，GPU自动裁剪）
);

out vec2 vTexCoord;  // 屏幕空间纹理坐标，范围 [0,1]

void main()
{
    vTexCoord = positions[gl_VertexID] * 0.5 + 0.5;
    gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
}
