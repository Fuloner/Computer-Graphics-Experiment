/// @brief 粒子数据结构 —— PBF流体模拟中的单个粒子
/// <summary>
/// 独立定义Particle结构体，供FluidSystem和SpatialHashing共同引用，避免循环依赖。
/// </summary>
#ifndef PARTICLE_H
#define PARTICLE_H

#include <glm/glm.hpp>
#include <vector>

/// <summary>
/// 单个流体粒子的数据结构
/// 对应PBF论文中的粒子状态：位置、速度、预测位置、拉格朗日乘子
/// </summary>
struct Particle
{
    glm::vec3 position;           // 粒子的当前位置 p_i
    glm::vec3 velocity;           // 粒子的速度 v_i
    glm::vec3 predictedPosition;  // 预测位置 p_i*（PBF迭代的中间变量，初始等于position）
    float     lambda = 0.0f;      // 拉格朗日乘子 λ_i（密度约束的强度系数）
    float     mass = 1.0f;        // 粒子质量（所有粒子质量相同）

    Particle() = default;

    /// <summary>
    /// 用指定位置构造粒子，速度初始为零，预测位置等于当前位置
    /// </summary>
    /// <param name="pos">粒子的初始世界空间位置</param>
    /// <param name="m">粒子的质量（默认1.0）</param>
    Particle(const glm::vec3& pos, float m = 1.0f)
        : position(pos), velocity(0.0f), predictedPosition(pos), mass(m) {}
};

#endif
