#version 330 core

/// @brief SSFR 最终着色/合成片元着色器（真实水效果版）
///
/// 从平滑深度和厚度纹理出发，在单个着色器中实现照片级水的渲染：
///
/// 1. 深度→世界位置重建（利用逆投影/逆视图矩阵）
/// 2. 屏幕空间法线计算（dFdx/dFdy叉积）
/// 3. Schlick Fresnel —— 决定反射/折射的混合比例
///    正面看水→透射（看到水下），侧面看水→反射（看到天空）
/// 4. 天空渐变环境反射 —— 基于反射向量的Y分量混合天顶/地平线颜色
/// 5. 逐通道Beer-Lambert吸收 —— 红/绿/蓝光在水中衰减速度不同
///    红光衰减最快→深水呈深蓝色，浅水呈青色
/// 6. 锐利镜面高光 —— 水的特征是窄而亮的高光
/// 7. 次表面散射近似 —— 薄水区（边缘）产生内部光散射发光
/// 8. 边缘泡沫 —— 极薄区域呈现白色泡沫

in vec2 vTexCoord;
out vec4 FragColor;

// ---- 输入纹理 ----
uniform sampler2D depthTex;
uniform sampler2D thicknessTex;

// ---- 矩阵 ----
uniform mat4 invProj;
uniform mat4 invView;

// ---- 摄像机与光照 ----
uniform vec3 viewPos;
uniform vec3 lightDir;

// ---- 水/环境参数 ----
uniform vec3  skyZenithColor;     // 天顶颜色（深蓝），默认 (0.25, 0.55, 0.85)
uniform vec3  skyHorizonColor;    // 地平线颜色（浅蓝/白），默认 (0.65, 0.82, 0.95)
uniform vec3  sunColor;           // 太阳光颜色，默认 (1.0, 0.95, 0.8)
uniform float absorptionCoeff;    // 吸收系数缩放，默认 2.0
uniform vec3  backgroundColor;    // 场景背景色

// ===== 水光学常数（硬编码，物理基准值） =====
const float waterIOR   = 1.333;   // 水的折射率
const float R0_water   = 0.02;    // 正入射Fresnel反射率 = ((1-1.333)/(1+1.333))²
// 逐通道吸收系数（红光衰减最快 → 深水蓝，蓝光衰减最慢 → 浅水青）
const vec3  absorbRGB  = vec3(3.5, 1.8, 0.6);


// ---- 辅助函数：从屏幕UV + 视图空间线性深度重建世界位置 ----
vec3 reconstructWorldPos(vec2 uv, float eyeDepth)
{
    vec4 ndcNear = vec4(uv * 2.0 - 1.0, -1.0, 1.0);
    vec4 viewNear = invProj * ndcNear;
    viewNear /= viewNear.w;
    float near = -viewNear.z;
    vec3 viewPosAtDepth = viewNear.xyz * (eyeDepth / near);
    vec4 worldPos4 = invView * vec4(viewPosAtDepth, 1.0);
    return worldPos4.xyz;
}


void main()
{
    // ================================================================
    // 第1步：深度读取与位置重建
    // ================================================================
    float eyeDepth = texture(depthTex, vTexCoord).r;

    if (eyeDepth < 0.001)
    {
        FragColor = vec4(backgroundColor, 1.0);
        return;
    }

    vec3 worldPos = reconstructWorldPos(vTexCoord, eyeDepth);

    // ================================================================
    // 第2步：屏幕空间法线 + 视线方向
    // ================================================================
    vec3 normal = normalize(cross(dFdx(worldPos), dFdy(worldPos)));
    vec3 V = normalize(viewPos - worldPos);
    if (dot(normal, V) < 0.0) normal = -normal;

    float NdotV = abs(dot(normal, V));

    // ================================================================
    // 第3步：Schlick 菲涅尔反射
    // ================================================================
    // F = R0 + (1 - R0) * (1 - cosθ)^5
    // 正面看 (NdotV≈1) → fresnel≈R0=0.02 → 98%透射，2%反射（看穿水面）
    // 侧面看 (NdotV≈0) → fresnel≈1.0 → 100%反射（看到天空倒影）
    float fresnel = R0_water + (1.0 - R0_water) * pow(1.0 - NdotV, 5.0);

    // ================================================================
    // 第4步：天空/环境反射
    // ================================================================
    // 计算反射方向，用其Y分量决定反射天空的哪个部分
    vec3 reflDir = reflect(-V, normal);
    float reflUp  = reflDir.y;  // [-1, 1]: 反射指向天空上方 → 正值

    // 天顶→地平线渐变：反射指向上方时看到深蓝色天顶，指向水平时看到亮色地平线
    vec3 skyReflection = mix(skyHorizonColor, skyZenithColor, max(0.0, reflUp));

    // 太阳光斑：反射方向接近光源方向时产生明亮高光
    vec3 L = normalize(lightDir);
    float sunDotRefl = max(0.0, dot(reflDir, L));
    float sunGlint = pow(sunDotRefl, 512.0) * 0.6;  // 极窄的太阳反射光斑

    // ================================================================
    // 第5步：透射/折射 —— 逐通道 Beer-Lambert 吸收
    // ================================================================
    float thickness = texture(thicknessTex, vTexCoord).r;

    // 每通道独立吸收：红色衰减最快 → 深水蓝，蓝色衰减最慢 → 浅水青
    vec3 transmittance = exp(-absorbRGB * thickness * absorptionCoeff);

    // 背景被水体染色：厚水区背景完全被水自身颜色取代
    vec3 waterBodyColor = vec3(0.02, 0.12, 0.35);  // 深水色（深海蓝）
    vec3 refractedColor = backgroundColor * transmittance
                        + waterBodyColor * (1.0 - transmittance);

    // ================================================================
    // 第6步：镜面高光（水的特征：窄而亮）
    // ================================================================
    vec3 H = normalize(L + V);
    float NdotH = max(dot(normal, H), 0.0);
    float NdotL = max(dot(normal, L), 0.0);

    // 两个高光瓣：非常窄的主高光 + 中等宽度的扩散高光
    float specSharp = pow(NdotH, 512.0);   // 极锐利的光斑
    float specWide  = pow(NdotH, 64.0);    // 较宽的光晕

    // 高光强度受Fresnel调制：掠射角处高光更强
    vec3 specular = sunColor * (specSharp * 1.0 + specWide * 0.25) * fresnel;

    // ================================================================
    // 第7步：次表面散射近似
    // ================================================================
    // 薄水区（边缘）光穿透表面后在内部散射，产生发光效果
    float sss = exp(-thickness * 3.0);
    // 散射光在有光照射的一面更明显
    vec3 sssColor = vec3(0.15, 0.45, 0.75) * sss * 0.3 * (0.5 + 0.5 * NdotL);

    // ================================================================
    // 第8步：边缘泡沫
    // ================================================================
    // 极薄区域（thickness < ~0.2）呈现白色泡沫
    float foam = exp(-thickness * 10.0);
    vec3 foamColor = vec3(0.85, 0.92, 0.98) * foam * 0.35;

    // ================================================================
    // 第9步：组合所有效果
    // ================================================================
    // 核心合成：Fresnel 驱动的反射/折射混合
    //   fresnel=0 (正面) → 看透水（折射色为主）
    //   fresnel=1 (侧面) → 看反射（天空色为主）
    vec3 waterColor = mix(refractedColor, skyReflection, fresnel);

    // 反射方向上的太阳光斑（叠加在反射之上）
    waterColor += sunGlint * fresnel;

    // 镜面高光（叠加）
    waterColor += specular;

    // 次表面散射（叠加，让薄水区发光）
    waterColor += sssColor;

    // 边缘泡沫（叠加，薄水区变白）
    waterColor += foamColor;

    // 微弱的漫反射模拟（水自身几乎无漫反射，但有微小贡献）
    waterColor *= (1.0 + NdotL * 0.06);

    FragColor = vec4(waterColor, 1.0);
}
