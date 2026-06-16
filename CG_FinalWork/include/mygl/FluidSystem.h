/// @brief 流体粒子系统 —— 定义粒子数据结构与流体系统的初始化和管理
/// <summary>
/// 包含Particle结构体（PBF算法中的单个流体粒子）和FluidSystem类（管理所有粒子的集合）。
/// 第3步：集成空间哈希邻居搜索，支持逐粒子邻居计数与颜色映射。
/// </summary>
#ifndef FLUIDSYSTEM_H
#define FLUIDSYSTEM_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <algorithm>
#include <iostream>

#include "Particle.h"
#include "SpatialHashing.h"

/// <summary>
/// 流体系统类 —— 管理所有流体粒子，负责初始化、邻居搜索、物理更新和提供渲染数据
/// </summary>
class FluidSystem
{
public:
    std::vector<Particle> particles;      // 所有粒子的数组
    std::vector<int>      neighborCounts; // 每个粒子的邻居数量（用于调试和可视化）

    float particleRadius = 0.05f;   // 粒子渲染半径（约等于光滑核半径的一半）
    float kernelRadius   = 0.15f;   // 光滑核半径 h（PBF算法中用于计算密度和邻居搜索的范围）
    float restDensity    = 1000.0f;  // 静止密度 ρ₀（水的参考密度，单位：kg/m³）

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

        // 初始化邻居计数数组
        neighborCounts.resize(particles.size(), 0);
    }

    /// <summary>
    /// 执行邻居搜索：使用空间哈希为每个粒子统计邻居数量
    /// </summary>
    /// <param name="verbose">是否在控制台输出详细统计信息（首次调用时为true）</param>
    void updateNeighborSearch(bool verbose = false)
    {
        if (particles.empty())
            return;

        // 构建空间哈希网格
        SpatialHashing hashGrid;
        hashGrid.build(particles, kernelRadius);

        // 为每个粒子查找邻居
        neighborCounts.resize(particles.size());
        int minNeighbors = INT_MAX;
        int maxNeighbors = 0;
        long long totalNeighbors = 0;

        for (size_t i = 0; i < particles.size(); ++i)
        {
            auto neighbors = hashGrid.findNeighbors(particles[i].position, particles, kernelRadius);
            // neighbors中包含粒子自身（距离0 < h），所以邻居数要减1
            int count = static_cast<int>(neighbors.size()) - 1;
            neighborCounts[i] = count;

            if (count < minNeighbors) minNeighbors = count;
            if (count > maxNeighbors) maxNeighbors = count;
            totalNeighbors += count;
        }

        float avgNeighbors = static_cast<float>(totalNeighbors) / particles.size();

        // 详细统计信息仅在首次调用时输出
        if (verbose)
        {
            std::cout << "===== 邻居搜索统计 =====" << std::endl;
            std::cout << "  粒子总数:     " << particles.size() << std::endl;
            std::cout << "  光滑核半径 h: " << kernelRadius << std::endl;
            std::cout << "  网格单元边长: " << hashGrid.cellSize << std::endl;
            std::cout << "  最少邻居数:   " << minNeighbors << std::endl;
            std::cout << "  最多邻居数:   " << maxNeighbors << std::endl;
            std::cout << "  平均邻居数:   " << avgNeighbors << std::endl;
            std::cout << "  哈希桶数量:   " << hashGrid.table.size() << std::endl;
            std::cout << "========================" << std::endl;
        }
    }

    /// <summary>
    /// 根据邻居数量计算每个粒子的颜色（用于可视化邻居分布）
    /// 邻居多的粒子偏白色，少的偏蓝色，便于从视觉上区分内部粒子和边缘粒子
    /// </summary>
    std::vector<glm::vec3> getColorData() const
    {
        glm::vec3 blueColor(0.2f, 0.45f, 0.85f);  // 低邻居数 → 深蓝
        glm::vec3 whiteColor(1.0f, 1.0f, 1.0f);    // 高邻居数 → 白色

        // 找到最大邻居数，用于归一化（避免除零）
        int maxCount = 0;
        for (int c : neighborCounts)
            maxCount = std::max(maxCount, c);
        if (maxCount == 0)
            maxCount = 1;

        std::vector<glm::vec3> colors(particles.size());
        for (size_t i = 0; i < particles.size(); ++i)
        {
            // 邻居数越多，颜色越接近白色
            float t = static_cast<float>(neighborCounts[i]) / maxCount;
            colors[i] = glm::mix(blueColor, whiteColor, t);
        }

        return colors;
    }

    /// <summary>
    /// 获取所有粒子当前的位置数据，返回连续的float数组（每个粒子3个float：x, y, z）
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
    /// 每帧更新流体模拟状态（第3步执行邻居搜索，静默模式不刷屏）
    /// </summary>
    /// <param name="deltaTime">上一帧耗时（秒），当前未使用</param>
    void update(float /*deltaTime*/)
    {
        // 每帧执行邻居搜索（静默模式），为后续物理步骤做准备
        updateNeighborSearch(false);
    }
};

#endif
