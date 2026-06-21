#version 330 core

/// @brief SSFR 流体最终着色片元着色器
/// 参考 PositionBasedFluids 中 fluid.frag 的实现

in vec2 vTexCoord;
out vec4 fColor;

uniform sampler2D uNormalViewSpaceTexture;
uniform sampler2D uThicknessTexture;
uniform sampler2D uSmoothedDepthTexture;

uniform mat4 uViewInverse;
uniform mat4 uViewTranspose;
uniform mat4 uProjInverse;
uniform vec2 uScreenSize;
uniform vec3 uFluidColor;
uniform vec3 uCameraPosition;
uniform vec3 uSkyZenithColor;
uniform vec3 uSkyHorizonColor;
uniform vec3 uBackgroundColor;

vec3 getWorldPos(vec2 uv, float depth)
{
    vec3 ndc = vec3(uv * 2.0 - 1.0, depth * 2.0 - 1.0);
    vec4 vp = uProjInverse * vec4(ndc, 1.0);
    vp /= vp.w;
    return (uViewInverse * vec4(vp.xyz, 1.0)).xyz;
}

void main()
{
    float depth = texture(uSmoothedDepthTexture, vTexCoord).r;
    if (depth < 0.001)
    {
        fColor = vec4(uBackgroundColor, 1.0);
        return;
    }

    vec3 N_vs = texture(uNormalViewSpaceTexture, vTexCoord).xyz;
    vec3 N = normalize((uViewTranspose * vec4(N_vs, 0.0)).xyz);
    vec3 P = getWorldPos(vTexCoord, depth);
    vec3 I = normalize(uCameraPosition - P);

    // === 折射 (Refraction) ===
    float thick = texture(uThicknessTexture, vTexCoord).r;
    vec3 atten = exp(-(vec3(1.0) - uFluidColor) * thick);
    float eta = 1.0 / 1.333;
    vec3 R_refract = refract(I, N, eta);
    float refractScaler = R_refract.y * 0.025;
    vec2 uvRefract = vTexCoord + N_vs.xy * refractScaler;
    vec3 colorRefract = uBackgroundColor * atten;

    // === 反射 (Reflection) ===
    vec3 R_reflect = normalize((uViewTranspose * vec4(reflect(-I, N), 0.0)).xyz);
    float up = R_reflect.y * 0.5 + 0.5;
    vec3 colorReflect = mix(uSkyHorizonColor, uSkyZenithColor, up);

    // === 合成 ===
    vec3 result = colorRefract + colorReflect * 0.75;
    fColor = vec4(result, 1.0);
}
