#version 330 core
// 粒子片段着色器 —— 将方形点精灵裁剪为边缘柔和的圆形，模拟球形粒子外观
// gl_PointCoord 是点精灵内部的标准化的纹理坐标，范围[0, 1]，原点在左下角

out vec4 FragColor;              // 输出片段颜色

uniform vec3 particleColor;      // 粒子颜色（浅蓝色系，模拟水滴外观）

void main()
{
    // 计算当前片段到点精灵中心(0.5, 0.5)的距离
    float dist = length(gl_PointCoord - vec2(0.5));

    // 圆形裁剪：丢弃中心0.5半径以外的片段，使方形点变成圆形
    if (dist > 0.5)
        discard;

    // 使用smoothstep在圆形边缘产生柔和过渡，消除锯齿
    // 边缘从0.35到0.5之间平滑过渡，外侧透明、内侧不透明
    float alpha = 1.0 - smoothstep(0.35, 0.5, dist);

    // 在圆形内部添加微妙的立体感：中心稍亮、边缘稍暗
    float brightness = 1.0 - dist * 0.3;

    FragColor = vec4(particleColor * brightness, alpha);
}
