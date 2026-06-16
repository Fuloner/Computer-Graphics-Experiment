#version 330 core
// 粒子片段着色器 —— 接收逐粒子颜色，将方形点精灵裁剪为圆形
// gl_PointCoord 是点精灵内部的标准化的纹理坐标，范围[0, 1]

in vec3 vColor;                  // 从顶点着色器接收的逐粒子颜色

out vec4 FragColor;              // 输出片段颜色

void main()
{
    // 计算当前片段到点精灵中心(0.5, 0.5)的距离
    float dist = length(gl_PointCoord - vec2(0.5));

    // 圆形裁剪：丢弃中心0.5半径以外的片段
    if (dist > 0.5)
        discard;

    // 在圆形边缘产生柔和过渡，消除锯齿
    float alpha = 1.0 - smoothstep(0.35, 0.5, dist);

    // 在圆形内部添加微妙的立体感：中心稍亮、边缘稍暗
    float brightness = 1.0 - dist * 0.3;

    FragColor = vec4(vColor * brightness, alpha);
}
