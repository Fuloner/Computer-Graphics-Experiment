/// @brief 流体粒子系统 —— 定义粒子数据结构与流体系统的初始化和管理
/// <summary>
/// 包含Particle结构体（PBF算法中的单个流体粒子）和FluidSystem类（管理所有粒子的集合）。
/// 第2步仅实现粒子的初始化排列和位置数据导出，不包含物理计算。
/// </summary>
#ifndef FLUIDSYSTEM_H
#define FLUIDSYSTEM_H

#include <glm/glm.hpp>
#include <vector>
#include <algorithm>

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
    Particle(const glm::vec3& pos, float m = 1.0f)
        : position(pos), velocity(0.0f), predictedPosition(pos), mass(m) {}
};

/// <summary>
/// 流体系统类 —— 管理所有流体粒子，负责初始化、更新和提供渲染所需数据
/// </summary>
class FluidSystem
{
public:
    std::vector<Particle> particles;  // 所有粒子的数组

    float particleRadius = 0.05f;     // 粒子渲染半径（约等于光滑核半径的一半）
    float kernelRadius   = 0.1f;      // 光滑核半径 h（PBF算法中用于计算密度和邻居搜索的范围）
    float restDensity    = 1000.0f;   // 静止密度 ρ₀（水的参考密度，单位：kg/m³，用于约束求解）

    FluidSystem() = default;

    /// <summary>
    /// 在指定的轴对齐包围盒（AABB）内以立方晶格排列初始化粒子
    /// </summary>
    /// <param name="minBound">包围盒的最小角点坐标（世界空间）</param>
    /// <param name="maxBound">包围盒的最大角点坐标（世界空间）</param>
    /// <param name="spacing">相邻粒子之间的间距（控制粒子数量和分辨率）</param>
    void initializeParticles(const glm::vec3& minBound, const glm::vec3& maxBound, float spacing)
    {
        particles.clear();

        // 计算三个维度上各有多少个粒子位置（+1是因为两端都包含）
        int countX = static_cast<int>((maxBound.x - minBound.x) / spacing) + 1;
        int countY = static_cast<int>((maxBound.y - minBound.y) / spacing) + 1;
        int countZ = static_cast<int>((maxBound.z - minBound.z) / spacing) + 1;

        // 预分配内存，避免频繁重新分配
        particles.reserve(countX * countY * countZ);

        // 三层循环生成立方晶格排列的粒子
        for (int i = 0; i < countX; ++i)
        {
            for (int j = 0; j < countY; ++j)
            {
                for (int k = 0; k < countZ; ++k)
                {
                    glm::vec3 pos;
                    pos.x = minBound.x + i * spacing;
                    pos.y = minBound.y + j * spacing;
                    pos.z = minBound.z + k * spacing;
                    particles.emplace_back(pos, 1.0f);  // 质量统一为1.0
                }
            }
        }

        // 更新粒子渲染半径（约为光滑核半径的一半，视觉上相邻粒子刚好接触）
        particleRadius = kernelRadius * 0.5f;
    }

    /// <summary>
    /// 获取所有粒子当前的位置数据，返回连续的float数组（每个粒子3个float：x, y, z）
    /// 用于上传到OpenGL的VBO中
    /// </summary>
    std::vector<float> getPositionData() const
    {
        std::vector<float> data;
        data.reserve(particles.size() * 3);
        for (const auto& p : particles)
        {
            data.push_back(p.position.x);
            data.push_back(p.position.y);
            data.push_back(p.position.z);
        }
        return data;
    }

    /// <summary>
    /// 返回粒子总数
    /// </summary>
    size_t getParticleCount() const
    {
        return particles.size();
    }

    /// <summary>
    /// 更新流体模拟（第2步暂不实现物理计算，仅作为占位）
    /// </summary>
    /// <param name="deltaTime">上一帧耗时（秒）</param>
    void update(float /*deltaTime*/)
    {
        // 第2步：不做任何物理计算，粒子保持在初始位置不动
        // 后续步骤将在此实现PBF算法的主循环
    }
};

#endif
