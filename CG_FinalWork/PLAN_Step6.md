# PLAN_Step6.md —— 第6步：流体渲染（实例化球体 + Blinn-Phong光照）

## 概述

将当前简陋的**点精灵（GL_POINTS）**渲染替换为**实例化球体渲染**，每个粒子绘制一个带有法线信息的低多边形球体，在片元着色器中实现Blinn-Phong光照模型，使流体看起来像带有光泽的半透明水团。

当前状态：
- 粒子用 `GL_POINTS` + 圆形裁剪片元着色器渲染（`particle_vertex.glsl` / `particle_fragment.glsl`）
- 已有 `fluid_vertex.glsl` / `fluid_fragment.glsl` 骨架但未使用（输出纯青色调试色）
- 渲染从GPU读回数据到CPU，再上传VBO（GPU→CPU→GPU往返）

目标状态：
- 生成一个单位球体网格（低模UV球），每个粒子实例化渲染一个缩放后的球体
- 顶点着色器接收球体顶点+法线 + 逐实例的位置+颜色，输出世界空间位置/法线
- 片元着色器计算Blinn-Phong光照（环境光 + 漫反射 + 镜面高光），使用半透明蓝色+菲涅尔边缘效果
- 开启混合（GL_BLEND），使用适当的深度策略

---

## 6a：球体网格生成

**目标：** 在 `main.cpp` 中编写函数生成低模单位球体的顶点数据和索引数据。

- 编写 `generateUnitSphere(int sectors, int stacks)` 函数
  - 用经纬线方法（UV球）生成顶点：每顶点 = 位置(vec3) + 法线(vec3)，共6个float
  - 用三角形条带生成索引（`GL_TRIANGLES`）
  - 建议参数：sectors=12, stacks=8（低模适合小粒子，约117顶点/576三角形）
- 返回 `std::vector<float>`（顶点）和 `std::vector<unsigned int>`（索引）
- 法线就是单位球面上的归一化位置向量

**验证方式：** 编译通过即可（此时还没有用上球体网格）。

---

## 6b：实例化渲染着色器编写

**目标：** 重写 `fluid_vertex.glsl` 和 `fluid_fragment.glsl`，实现正确的实例化球体渲染+光照。

### 顶点着色器（fluid_vertex.glsl）

- 输入：
  - location=0: `vec3 aPos`（单位球体顶点位置，divisor=0）
  - location=1: `vec3 aNormal`（单位球体顶点法线，divisor=0）
  - location=2: `vec3 aInstancePos`（粒子世界位置，divisor=1）
  - location=3: `vec3 aInstanceColor`（粒子颜色，divisor=1）
- Uniforms：`model`, `view`, `projection`, `uParticleRadius`
- 逻辑：`worldPos = aPos * uParticleRadius + aInstancePos`，法线不变（均匀缩放）
- 输出：`WorldPos`, `WorldNormal`, `ParticleColor` 给片元着色器

### 片元着色器（fluid_fragment.glsl）

- 输入：`WorldPos`, `WorldNormal`, `ParticleColor`
- Uniforms：`uLightPos`, `uLightColor`, `uViewPos`, `uAmbientStrength`, `uSpecularStrength`, `uShininess`
- 光照计算：环境光 + 朗伯漫反射 + Blinn-Phong镜面高光
- 透明度：菲涅尔效果（边缘更不透明，中心更透明），alpha范围约0.3~0.8
- 最终颜色 = `ParticleColor * lighting`，alpha = 菲涅尔alpha

**验证方式：** 编译通过。

---

## 6c：渲染管线集成

**目标：** 修改 `main.cpp`，替换点精灵渲染为实例化球体渲染。

- 生成球体网格（调用6a中的函数）
- 创建新的VAO：
  - VBO[0]：球体顶点数据（位置+法线，divisor=0，GL_STATIC_DRAW）
  - EBO：球体索引数据（GL_STATIC_DRAW）
  - VBO[1]：实例位置（divisor=1，GL_DYNAMIC_DRAW，每帧更新）
  - VBO[2]：实例颜色（divisor=1，GL_DYNAMIC_DRAW，每帧更新）
- 使用 `glDrawElementsInstanced` 替代 `glDrawArrays(GL_POINTS, ...)`
- 移除 `glEnable(GL_PROGRAM_POINT_SIZE)`
- 调试：移除每帧打印粒子位置的代码（控制台输出过多）
- 混合设置保持：`glEnable(GL_BLEND)` + `GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA`
- 深度策略：渲染粒子时 `glDepthMask(GL_FALSE)` 允许透过粒子看到后方粒子

**验证方式：** 编译运行，看到带光照的半透明蓝色球体做流体动画。

---

## 6d：深度纹理 + 屏幕空间法线（可选增强）

**目标：** 先将所有粒子球体渲染到FBO的深度附件中，然后在全屏四边形上计算屏幕空间法线，实现更平滑的流体表面效果。

- 创建FBO + 颜色纹理 + 深度纹理
- Pass 1：渲染所有球体到FBO（记录深度和厚度）
- Pass 2：全屏四边形采样深度纹理，用Sobel算子计算屏幕空间法线，应用光照
- 此步骤较为复杂，视6c完成效果决定是否实施

**验证方式：** 编译运行，看到更平滑的流体表面。

---

## 开发顺序

按 **6a → 6b → 6c** 顺序实施，每完成一步编译验证。6d为可选增强，视情况决定。
