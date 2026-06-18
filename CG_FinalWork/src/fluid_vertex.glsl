#version 330 core

layout (location = 0) in vec3 aPos;   // 粒子位置
layout (location = 1) in vec3 aColor; // 逐粒子颜色（密度映射）

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float pointScale;

out vec3 vColor;
out vec3 vWorldPos;   // 世界空间位置，用于光照

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    vWorldPos = worldPos.xyz;
    vec4 clipPos = projection * view * worldPos;
    gl_Position = clipPos;

    // 点大小固定（或可调），方便观察
    gl_PointSize = pointScale; // 由 CPU 控制
    // 限制尺寸
    gl_PointSize = clamp(gl_PointSize, 2.0, 60.0);

    // vColor = aColor;
    vColor = vec3(0.2f, 0.5f, 0.9f);
}