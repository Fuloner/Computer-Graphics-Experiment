#version 330 core

in vec3 vColor;
in vec3 vWorldPos;

out vec4 FragColor;

uniform vec3 lightDir;   // 光源方向（世界空间，归一化）
uniform vec3 viewPos;    // 摄像机位置（世界空间）

void main()
{
    // ---- 1. 圆形裁剪 ----
    vec2 center = gl_PointCoord - vec2(0.5);
    float dist = length(center);
    if (dist > 0.5) discard;

    // ---- 2. 球体法线近似（用点精灵坐标模拟球体） ----
    // 模拟一个半径为0.5的半球体，法线为 (x, y, z)，z = sqrt(0.25 - x² - y²)
    float z = sqrt(max(0.0, 0.25 - dist * dist));
    vec3 normal = normalize(vec3(center.x, center.y, z));

    // ---- 3. 光照计算 ----
    vec3 lightDirNorm = normalize(lightDir);
    float diff = max(dot(normal, lightDirNorm), 0.0);
    float ambient = 0.25;

    // 漫反射 + 环境光
    vec3 lighting = (ambient + diff * 0.75) * vColor;

    // ---- 4. 菲涅尔边缘光 ----
    vec3 viewDir = normalize(viewPos - vWorldPos);
    // 用世界空间中的视图方向（简化：将局部法线转换到世界空间？因为粒子是点，没有旋转，我们可以直接使用局部法线）
    // 但更好的效果：使用视角方向与法线的夹角
    float fresnel = pow(1.0 - max(dot(normal, viewDir), 0.0), 3.0);
    vec3 fresnelColor = vec3(0.3, 0.6, 0.9) * fresnel * 0.6;

    // ---- 5. 透明度（边缘半透） ----
    float alpha = smoothstep(0.5, 0.2, dist) * 0.9;

    // ---- 6. 最终颜色 ----
    vec3 finalColor = lighting + fresnelColor;
    FragColor = vec4(finalColor, alpha);
}