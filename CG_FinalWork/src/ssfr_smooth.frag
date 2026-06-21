#version 330 core

/// @brief SSFR 双边滤波深度平滑片元着色器
/// 参考 PositionBasedFluids 中 smoothDepth.comp 的实现
///
/// 支持两种模式（由 uHorizontal 控制）：
///   - 分离式（推荐）：水平 1D 滤波 + 垂直 1D 滤波，两次 pass 完成一轮平滑
///   - 非分离式：仅在 uHorizontal=1 时执行 2D 滤波
///
/// 双边滤波权重 = f_d(空间距离) × f_r(深度差异)
///   f_d = exp(-dist² / (2σ_d²))
///   f_r = exp(-Δdepth² / (2σ_r²))

in vec2 vTexCoord;
out float smoothedDepth;

uniform sampler2D depthTex;       // 输入深度纹理（NDC 深度 [0,1]）
uniform vec2 texelSize;           // 纹素大小 (1/width, 1/height)
uniform int uKernelRadius;        // 滤波核半径
uniform bool uHorizontal;         // true=水平 1D 滤波, false=垂直 1D 滤波
uniform int uSeparate;            // 0=非分离式(2D), 1=分离式(1D)

// 空间和深度高斯的标准差（与参考实现一致）
const float BILATERAL_SIGMA_D = 1.0;  // 空间 σ（像素）
const float BILATERAL_SIGMA_R = 1.0;  // 深度 σ（NDC 深度单位）

void main()
{
    float centerDepth = texture(depthTex, vTexCoord).r;

    // 无流体表面：跳过
    if (centerDepth < 0.001)
    {
        smoothedDepth = centerDepth;
        return;
    }

    float w_p = 0.0;
    float sum = 0.0;

    float rDenomInv = 1.0 / (2.0 * BILATERAL_SIGMA_R * BILATERAL_SIGMA_R);
    float dDenomInv = 1.0 / (2.0 * BILATERAL_SIGMA_D * BILATERAL_SIGMA_D);

    if (uSeparate == 0)
    {
        // ---- 非分离式：仅在水平模式时做完整 2D 滤波 ----
        if (uHorizontal)
        {
            for (int i = -uKernelRadius; i <= uKernelRadius; i++)
            {
                for (int j = -uKernelRadius; j <= uKernelRadius; j++)
                {
                    vec2 uv = vTexCoord + vec2(float(i), float(j)) * texelSize;
                    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0)
                        continue;

                    float d = texture(depthTex, uv).r;
                    if (d < 0.001)
                        continue;

                    float dist2 = float(i * i + j * j);
                    float f_d = exp(-dist2 * dDenomInv);
                    float diff = d - centerDepth;
                    float f_r = exp(-diff * diff * rDenomInv);

                    w_p += f_r * f_d;
                    sum += d * f_r * f_d;
                }
            }
        }
        // else: 非分离式垂直模式 → 不做处理，直接保留原值
    }
    else
    {
        // ---- 分离式：1D 滤波（水平或垂直） ----
        for (int i = -uKernelRadius; i <= uKernelRadius; i++)
        {
            vec2 uv = vTexCoord;
            if (uHorizontal)
                uv.x += float(i) * texelSize.x;
            else
                uv.y += float(i) * texelSize.y;

            if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0)
                continue;

            float d = texture(depthTex, uv).r;
            if (d < 0.001)
                continue;

            float dist2 = float(i * i);
            float f_d = exp(-dist2 * dDenomInv);
            float diff = d - centerDepth;
            float f_r = exp(-diff * diff * rDenomInv);

            w_p += f_r * f_d;
            sum += d * f_r * f_d;
        }
    }

    smoothedDepth = (w_p > 0.001) ? (sum / w_p) : centerDepth;
}
