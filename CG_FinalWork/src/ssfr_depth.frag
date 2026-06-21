#version 330 core

/// @brief SSFR 深度准备通道片元着色器
/// 参考 PositionBasedFluids 中 fluidDepthTexture.frag 的实现
///
/// 将每个粒子渲染为一个球面的深度。
/// 核心思路：
///   1. 从 gl_PointCoord 推导球面局部法线（屏幕空间球体映射）
///   2. 用球面法线偏移球心位置：fragPos = centerPos + normal * radius
///   3. 投影到裁剪空间，计算 NDC 深度
///   4. 输出 NDC 深度（同时写入硬件深度缓冲用于遮挡）

in vec3 vCenterPosViewSpace;
in float vDensity;

layout (location = 0) out float fDepth;  // NDC 深度值 [0, 1]

uniform mat4 uProjection;     // 投影矩阵
uniform float uPointSize;     // 粒子世界空间渲染半径
uniform float uMinimumDensity;// 密度阈值（低于此值的粒子片段被丢弃）

const vec2 POINT_CENTER = vec2(0.5, 0.5);

void main()
{
    // ---- 密度过滤 ----
    if (vDensity < uMinimumDensity)
        discard;

    // ---- 第1步：将 gl_PointCoord 映射到以圆心为原点的坐标系 ----
    // gl_PointCoord 范围 [0,1]×[0,1]，参考实现中对 Y 取反以匹配 OpenGL 屏幕坐标
    vec2 pointCoord = (gl_PointCoord - POINT_CENTER) * vec2(2.0, -2.0);
    float xySqLen = dot(pointCoord, pointCoord);

    // 圆形裁剪：丢弃圆外片段
    if (xySqLen > 1.0)
        discard;

    // ---- 第2步：从 pointCoord 推导球面视图空间法线 ----
    // 对于半球体（摄像机看到的正面），z = sqrt(1 - x² - y²)
    vec3 normalViewSpace = vec3(pointCoord, sqrt(1.0 - xySqLen));

    // ---- 第3步：计算球面上该点的视图空间位置 ----
    // 球心 + 法线方向 × 半径
    vec3 fragPosViewSpace = vCenterPosViewSpace + normalViewSpace * uPointSize;

    // ---- 第4步：投影到裁剪空间，计算 NDC 深度 ----
    vec4 fragPosClipSpace = uProjection * vec4(fragPosViewSpace, 1.0);
    float ndcDepth = (fragPosClipSpace.z / fragPosClipSpace.w) * 0.5 + 0.5;

    gl_FragDepth = ndcDepth;
    fDepth = ndcDepth;
}
