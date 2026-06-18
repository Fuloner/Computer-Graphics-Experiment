#version 330 core

/// @brief SSFR厚度通道片元着色器 —— 累积流体厚度
/// 对于每个粒子，计算视线穿过该粒子球体的弦长（即厚度贡献），
/// 所有粒子的贡献通过加法混合（GL_ONE, GL_ONE）累加到厚度纹理中。
/// 厚度纹理在最终着色时用于计算光吸收（Beer-Lambert 定律）。

in vec3 vWorldPos;
in vec3 vViewPos;
in float vWorldRadius;

layout (location = 0) out float fragThickness;  // 输出到 GL_R32F 颜色纹理

void main()
{
    // ---- 第1步：圆形裁剪（与深度通道一致） ----
    vec2 offset = gl_PointCoord - 0.5;
    float dist = length(offset) * 2.0;

    if (dist > 1.0)
        discard;

    // ---- 第2步：计算视线穿过球体的弦长 ----
    // 对于距球心归一化距离为 dist 的点，球体在该点的"厚度"（视线方向穿越距离）
    // thickness = 2 * R * sqrt(1 - dist²)
    // dist=0（球心）时厚度 = 2R（直径），dist=1（球边缘）时厚度 = 0
    float height = sqrt(max(0.0, 1.0 - dist * dist));
    float thickness = 2.0 * vWorldRadius * height;

    fragThickness = thickness;
}
