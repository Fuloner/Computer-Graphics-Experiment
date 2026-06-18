#version 330 core

/// @brief SSFR深度通道片元着色器 —— 将每个粒子的球面深度写入颜色附件
/// 核心原理：点精灵是一个覆盖粒子球体投影的方形区域，片元着色器通过圆形裁剪
/// 丢弃球体外的像素。对于在球体内的像素，计算球面到摄像机的真实深度（考虑球面
/// 向摄像机方向隆起），输出为视图空间的线性深度（正数，单位与世界空间一致）。
/// 同时写入硬件深度缓冲（gl_FragDepth）以正确处理粒子之间的前后遮挡。

in vec3 vWorldPos;
in vec3 vViewPos;
in float vWorldRadius;

layout (location = 0) out float fragDepth;  // 输出到 GL_R32F 颜色纹理

uniform float nearPlane;  // 投影矩阵的近裁剪面距离
uniform float farPlane;   // 投影矩阵的远裁剪面距离

void main()
{
    // ---- 第1步：圆形裁剪 ----
    // gl_PointCoord 范围 [0,1]×[0,1]，中心在 (0.5, 0.5)
    vec2 offset = gl_PointCoord - 0.5;
    float dist = length(offset) * 2.0;  // 归一化距离：0=圆心, 1=圆边缘

    if (dist > 1.0)
        discard;  // 丢弃圆形之外的片段

    // ---- 第2步：计算球面深度 ----
    // 球体方程：对于距球心归一化距离为 dist 的点，球面向摄像机隆起的高度
    // height = sqrt(1 - dist²)，当 dist=0 时 height=1（球正面顶点），dist=1 时 height=0（球边缘）
    float height = sqrt(max(0.0, 1.0 - dist * dist));

    // 视图空间深度：球心深度减去球面向摄像机的隆起量
    // vViewPos.z < 0（摄像机看向-Z），-vViewPos.z 是到摄像机的正距离
    // 球面向摄像机隆起 => 表面更近 => 深度更小
    float viewSpaceDepth = -vViewPos.z - vWorldRadius * height;

    // 输出线性化的视图空间深度到颜色纹理（后续双边滤波和法线重建使用）
    fragDepth = max(viewSpaceDepth, 0.0);

    // ---- 第3步：设置硬件深度（用于正确的粒子间前后遮挡） ----
    // 将视图空间线性深度 d（正数，距摄像机距离）转换为 gl_FragDepth [0, 1]
    //
    // 对于 glm::perspective 投影矩阵，视图空间 z（负数，记为 viewZ）对应的 NDC z 为：
    //   ndcZ = (far+near)/(far-near) - 2*far*near/((far-near) * (-viewZ))
    // 令 d = -viewZ > 0（即 viewSpaceDepth），代入得：
    //   ndcZ = (far+near)/(far-near) - 2*far*near/((far-near) * d)
    //   ndcZ ∈ [-1, 1]：near面(d=near) → -1，far面(d=far) → +1
    // gl_FragDepth 使用 [0, 1] 范围（near=0, far=1），将 ndcZ 映射过去：
    float d = viewSpaceDepth;
    float ndcZ = (farPlane + nearPlane) / (farPlane - nearPlane)
               - (2.0 * farPlane * nearPlane) / ((farPlane - nearPlane) * d);
    gl_FragDepth = ndcZ * 0.5 + 0.5;
}
