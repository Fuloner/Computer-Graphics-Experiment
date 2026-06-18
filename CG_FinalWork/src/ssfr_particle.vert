#version 330 core

/// @brief SSFR粒子顶点着色器 —— 将每个粒子渲染为覆盖其球体范围的大点精灵
/// 用于深度通道（输出球面深度）和厚度通道（输出射线穿越球体的厚度）
/// 点大小根据粒子世界空间半径和到摄像机的距离自动计算

layout (location = 0) in vec3 aPos;   // 粒子世界空间位置

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float pointScale;            // 点大小缩放系数（用于手动调节）
uniform float particleWorldRadius;   // 粒子的世界空间渲染半径（应略大于物理半径以保证粒子间有重叠）

out vec3 vWorldPos;       // 粒子中心的世界空间位置
out vec3 vViewPos;        // 粒子中心的视图空间位置
out float vWorldRadius;   // 世界空间渲染半径

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    vec4 viewPos  = view * worldPos;

    gl_Position = projection * viewPos;

    vWorldPos    = worldPos.xyz;
    vViewPos     = viewPos.xyz;
    vWorldRadius = particleWorldRadius;

    // 将世界空间半径投影到屏幕空间，计算所需的点精灵像素尺寸
    // 原理：在距摄像机 d 处，世界尺寸 w 对应的屏幕像素数 ≈ (screenHeight/2) * w / (d * tan(fovY/2))
    // 乘以 pointScale 允许手动调节以保证粒子之间有充分重叠
    float viewDepth = -viewPos.z;  // viewPos.z < 0 在摄像机前方，取反得到正的距离
    gl_PointSize = pointScale * vWorldRadius / max(viewDepth, 0.001);

    // 限制最小/最大点大小，防止极端情况
    gl_PointSize = clamp(gl_PointSize, 2.0, 200.0);
}
