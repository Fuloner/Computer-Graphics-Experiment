#version 330 core

/// @brief 全屏四边形顶点着色器 —— 使用 gl_VertexID 技巧无需 VBO
/// 用于所有后处理通道（深度平滑、法线计算、最终着色）

const vec2 positions[3] = vec2[](
    vec2(-1.0, -1.0),
    vec2( 3.0, -1.0),
    vec2(-1.0,  3.0)
);

out vec2 vTexCoord;

void main()
{
    vTexCoord = positions[gl_VertexID] * 0.5 + 0.5;
    gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
}
