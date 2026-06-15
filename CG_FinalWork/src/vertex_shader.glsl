#version 330 core
// 顶点着色器 —— 接受3D顶点位置，通过MVP矩阵变换到裁剪空间
// 同时也将世界空间位置传递给片段着色器，方便后续做光照等计算

layout (location = 0) in vec3 aPos;          // 顶点的局部空间位置

uniform mat4 model;       // 模型矩阵 —— 局部空间 → 世界空间
uniform mat4 view;        // 视图矩阵 —— 世界空间 → 观察空间
uniform mat4 projection;  // 投影矩阵 —— 观察空间 → 裁剪空间

out vec3 WorldPos;        // 传递给片段着色器的世界空间位置

void main()
{
    // 计算世界空间位置（不考虑非均匀缩放的情况）
    WorldPos = vec3(model * vec4(aPos, 1.0));
    // 标准MVP变换到裁剪空间
    gl_Position = projection * view * vec4(WorldPos, 1.0);
}
