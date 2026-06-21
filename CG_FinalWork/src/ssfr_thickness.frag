#version 330 core

/// @brief SSFR 厚度准备通道片元着色器
/// 参考 PositionBasedFluids 中 fluidThicknessTexture.frag 的实现
///
/// 通过加法混合（GL_ONE, GL_ONE）累积视线方向上穿过流体的厚度。
///
/// 厚度贡献 = thicknessScaler × normalViewSpace.z
/// 其中 normalViewSpace.z = sqrt(1 - x² - y²) 是球面法线在视线方向（-Z）的分量。
/// 球心处 z=1（最大贡献），球边缘处 z=0（无贡献）。

in vec3 vCenterPosViewSpace;
in float vDensity;

out vec4 fThickness;

uniform float uPointSize;          // 粒子世界空间渲染半径
uniform float uThicknessScaler;    // 厚度缩放系数（参考值 0.05）
uniform float uMinimumDensity;     // 密度阈值

const vec2 POINT_CENTER = vec2(0.5, 0.5);

void main()
{
    if (vDensity < uMinimumDensity)
        discard;

    vec2 pointCoord = (gl_PointCoord - POINT_CENTER) * vec2(2.0, -2.0);
    float xySqLen = dot(pointCoord, pointCoord);

    if (xySqLen > 1.0)
        discard;

    vec3 normalViewSpace = vec3(pointCoord, sqrt(1.0 - xySqLen));

    // 厚度 = scaler × 视线方向分量
    // 写入 R 通道（加法混合自动累积多粒子贡献）
    fThickness = vec4(uThicknessScaler * normalViewSpace.z, 0.0, 0.0, 1.0);
}
