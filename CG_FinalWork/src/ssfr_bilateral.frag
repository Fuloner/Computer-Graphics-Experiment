#version 330 core

/// @brief 双边滤波片元着色器 —— 平滑深度纹理同时保持边缘清晰
/// 双边滤波（Bilateral Filter）是屏幕空间流体渲染的核心步骤。
/// 它解决了"粒子之间有间隙导致表面不连续"的问题：
///   - 空间权重（Gaussian）：距离当前像素越远的邻居，影响越小
///   - 范围权重（Range）：深度值与当前像素相差越大的邻居，影响越小
/// 范围权重确保了不同深度层次的粒子（如水花飞溅与水面之间）不会被错误地混合在一起，
/// 实现了"该光滑的地方光滑，该锐利的地方保持锐利"的效果。
///
/// 本着色器在 7×7 邻域内执行完整的二维双边滤波。

in vec2 vTexCoord;

uniform sampler2D depthTex;       // 输入的深度纹理（视图空间线性深度，R32F）
uniform vec2 texelSize;           // 单个纹素的大小 = vec2(1.0/width, 1.0/height)
uniform float sigmaSpatial;       // 空间高斯核的 σ（像素单位），默认约 2.0
uniform float sigmaRange;         // 深度范围 σ（深度单位），默认约 0.03
uniform int kernelRadius;         // 核半径（7×7 核则 radius=3）

layout (location = 0) out float smoothedDepth;  // 输出平滑后的深度

void main()
{
    float centerDepth = texture(depthTex, vTexCoord).r;

    // 如果当前像素没有流体表面（深度接近0），直接输出0，不做平滑
    // 这不影响周围的流体像素——它们会在自己的片元着色器中被独立处理
    if (centerDepth < 0.001)
    {
        smoothedDepth = centerDepth;
        return;
    }

    float sum = 0.0;          // 加权深度累加
    float totalWeight = 0.0;  // 权重归一化因子

    // 预计算高斯核的常数因子（省略 1/(2πσ²)，因为最终会归一化）
    float invTwoSigmaSpatial2 = 1.0 / (2.0 * sigmaSpatial * sigmaSpatial);
    float invTwoSigmaRange2   = 1.0 / (2.0 * sigmaRange   * sigmaRange);

    // 遍历 kernelRadius × kernelRadius 的正方形邻域
    for (int dx = -kernelRadius; dx <= kernelRadius; ++dx)
    {
        for (int dy = -kernelRadius; dy <= kernelRadius; ++dy)
        {
            vec2 uv = vTexCoord + vec2(float(dx), float(dy)) * texelSize;

            // 边界检查（防止采样到纹理外部）
            if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0)
                continue;

            float neighborDepth = texture(depthTex, uv).r;

            // 跳过没有流体表面的像素
            if (neighborDepth < 0.001)
                continue;

            // ---- 空间权重：二维高斯核 ----
            float spatialDist2 = float(dx * dx + dy * dy);
            float wSpatial = exp(-spatialDist2 * invTwoSigmaSpatial2);

            // ---- 范围权重：基于深度差的边缘感知 ----
            float depthDiff = neighborDepth - centerDepth;
            float wRange = exp(-depthDiff * depthDiff * invTwoSigmaRange2);

            float w = wSpatial * wRange;
            sum += neighborDepth * w;
            totalWeight += w;
        }
    }

    // 归一化：加权平均
    smoothedDepth = (totalWeight > 0.001) ? (sum / totalWeight) : centerDepth;
}
