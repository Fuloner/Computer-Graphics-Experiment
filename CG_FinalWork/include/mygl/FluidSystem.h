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

#include "Particle.h"

/// <summary>
/// 流体系统类 —— 管理所有流体粒子，负责初始化、更新和提供渲染所需数据
/// </summary>
class FluidSystem
{
public:
    std::vector<Particle> particles;  // 所有粒子的数组

    float particleRadius = 0.05f;     // 粒子渲染半径（约等于光滑核半径的一半）
    float kernelRadius   = 1.2f;      // 光滑核半径 h（PBF算法中用于计算密度和邻居搜索的范围）
    float restDensity    = 1000.0f;   // 静止密度 ρ₀（水的参考密度，单位：kg/m³，用于约束求解）

    //水箱边界常量
    glm::vec3 tankMin = glm::vec3(-0.5f, -1.0f, -0.5f);
    glm::vec3 tankMax = glm::vec3( 0.5f,  1.0f,  0.5f);
    float restitution = 0.3f;  // 反弹系数

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
    /// 计算密度
    /// </summary>
    void computeDensity()
    {
        float h = kernelRadius;
        float h2 = h * h;
        float poly6Coeff = 315.0f / (64.0f * 3.141592653589793f * pow(h, 9));

        // 重置密度
        for (auto& p : particles) p.density = 0.0f;

        // 暴力遍历所有粒子对（包括自身）
        for (size_t i = 0; i < particles.size(); ++i)
        {
            for (size_t j = 0; j < particles.size(); ++j)
            {
                glm::vec3 r = particles[i].position - particles[j].position;
                float r2 = r.x*r.x + r.y*r.y + r.z*r.z;
                if (r2 >= h2) continue;
                float diff = h2 - r2;
                float w = poly6Coeff * diff * diff * diff;
                particles[i].density += particles[j].mass * w;
            }
        }
    }

    /// <summary>
    /// 获取颜色
    /// </summary>
    std::vector<glm::vec3> getColorData() const
    {
        float maxDensity = 0.0f, minDensity = FLT_MAX;
        for (const auto& p : particles) {
            if (p.density > maxDensity) maxDensity = p.density;
            if (p.density < minDensity) minDensity = p.density;
        }
        if (maxDensity < 1e-6f) maxDensity = 1.0f;

        std::vector<glm::vec3> colors;
        colors.reserve(particles.size());
        for (const auto& p : particles) {
            float normalized = (p.density - minDensity) / (maxDensity - minDensity + 1e-6f);
            // 低密度→蓝色，高密度→红色
            colors.push_back(glm::mix(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), normalized));
        }
        return colors;
    }

    /// <summary>
    /// 返回粒子总数
    /// </summary>
    size_t getParticleCount() const
    {
        return particles.size();
    }

    /// <summary>
    /// 更新流体模拟
    /// </summary>
    /// <param name="deltaTime">上一帧耗时（秒）</param>
    void update(float deltaTime)
    {
        computeDensity();

        // 在此实现PBF算法的主循环
        const glm::vec3 gravity = glm::vec3(0.0f, -9.8f, 0.0f);

        for(auto& p : particles)
        {
            //施加重力
            p.velocity += gravity * deltaTime;
            //更新位置
            p.position += p.velocity * deltaTime;

            //边界碰撞
            if (p.position.x < tankMin.x) { p.position.x = tankMin.x; p.velocity.x = -p.velocity.x * restitution; }
            if (p.position.x > tankMax.x) { p.position.x = tankMax.x; p.velocity.x = -p.velocity.x * restitution; }
            if (p.position.y < tankMin.y) { p.position.y = tankMin.y; p.velocity.y = -p.velocity.y * restitution; }
            if (p.position.y > tankMax.y) { p.position.y = tankMax.y; p.velocity.y = -p.velocity.y * restitution; }
            if (p.position.z < tankMin.z) { p.position.z = tankMin.z; p.velocity.z = -p.velocity.z * restitution; }
            if (p.position.z > tankMax.z) { p.position.z = tankMax.z; p.velocity.z = -p.velocity.z * restitution; }
        }
    }
};

#endif