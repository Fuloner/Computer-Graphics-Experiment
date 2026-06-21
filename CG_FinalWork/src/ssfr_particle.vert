#version 330 core

/// @brief SSFR 粒子顶点着色器 —— 所有粒子渲染通道共用
/// 参考 PositionBasedFluids 中 fluid.vert 的实现
///
/// 将粒子渲染为相机空间点精灵，点大小根据粒子半径和深度自动缩放。
/// POINT_SIZE_SCALER 是将世界空间半径映射到屏幕像素的经验系数，
/// 参考实现中使用 2000（针对 ~1000px 高度的窗口）。本项目针对 600px 调整。

layout (location = 0) in vec3 aPos;          // 粒子世界空间位置

uniform mat4 uView;          // 视图矩阵
uniform mat4 uProjection;    // 投影矩阵
uniform float uPointSize;    // 粒子世界空间渲染半径（= PARTICLE_RADIUS * scaler）

out vec3 vCenterPosViewSpace;  // 粒子球心在视图空间的坐标
out float vDensity;            // 预留：密度值（用于密度过滤，当前未使用）

// 将世界空间半径映射到屏幕空间点精灵像素数的缩放系数
// 屏幕Y方向像素密度 ≈ screenHeight / (2 * tan(fovY/2))
// 对于 600px 屏幕和 45° FOV: 600 / (2*0.414) ≈ 724 px/rad
// 点直径需要的像素 = 2 * radius * pxPerRad / depth
// 本系数取 2 * 724 ≈ 1448，保守取 2000（参考值，保证足够重叠）
const float POINT_SIZE_SCALER = 2000.0;

void main()
{
    vec4 viewPos = uView * vec4(aPos, 1.0);
    gl_Position = uProjection * viewPos;

    vCenterPosViewSpace = viewPos.xyz;

    // 根据深度缩放点大小：近大远小
    // viewPos.z < 0（相机看向 -Z），取反得到正距离
    gl_PointSize = POINT_SIZE_SCALER * uPointSize / (-vCenterPosViewSpace.z);

    vDensity = 1.0;  // 当前不使用密度过滤，所有粒子都参与渲染
}
