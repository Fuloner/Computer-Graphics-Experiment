/// @brief 空间哈希网格 —— 用于高效邻居搜索
/// <summary>
/// 将3D空间划分为边长为cellSize的立方体网格单元，每个单元存储其中粒子的索引。
/// 查询邻居时只需检查周围3×3×3=27个相邻单元，将朴素O(n²)的邻居搜索降低到O(n)。
/// 这是PBF/SPH等粒子流体算法中不可或缺的加速结构。
/// </summary>
#ifndef SPATIALHASHING_H
#define SPATIALHASHING_H

#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include <cmath>

#include "Particle.h"  // 需要完整的Particle定义以访问粒子位置

class SpatialHashing
{
public:
    /// <summary>
    /// 哈希表：键 = 网格单元的哈希值，值 = 该单元内所有粒子的索引列表
    /// </summary>
    std::unordered_map<unsigned int, std::vector<int>> table;

    float cellSize;  // 网格单元边长（通常等于光滑核半径h）

    SpatialHashing() = default;

    /// <summary>
    /// 用指定网格单元边长构造空间哈希对象
    /// </summary>
    /// <param name="cs">网格单元边长</param>
    explicit SpatialHashing(float cs) : cellSize(cs) {}

    /// <summary>
    /// 根据3D世界坐标计算所属网格单元的哈希值
    /// 哈希算法：将三个维度的整数坐标分别乘以大质数后异或，减少碰撞
    /// </summary>
    /// <param name="pos">世界空间位置</param>
    /// <returns>网格单元的哈希键</returns>
    unsigned int hashPosition(const glm::vec3& pos) const
    {
        // 将世界坐标转换为整数网格坐标（向下取整）
        int cx = static_cast<int>(std::floor(pos.x / cellSize));
        int cy = static_cast<int>(std::floor(pos.y / cellSize));
        int cz = static_cast<int>(std::floor(pos.z / cellSize));
        return hashCell(cx, cy, cz);
    }

    /// <summary>
    /// 遍历所有粒子，将粒子索引插入对应的网格单元哈希桶中
    /// 每次物理帧开始前需要调用一次，因为粒子位置会变化
    /// </summary>
    /// <param name="particles">所有粒子的数组</param>
    /// <param name="radius">光滑核半径（必须与cellSize一致）</param>
    void build(const std::vector<Particle>& particles, float radius)
    {
        cellSize = radius;
        table.clear();

        // 遍历所有粒子，将其索引插入对应的空间哈希桶
        for (size_t i = 0; i < particles.size(); ++i)
        {
            unsigned int h = hashPosition(particles[i].position);
            table[h].push_back(static_cast<int>(i));
        }
    }

    /// <summary>
    /// 基于预测位置（predictedPosition）构建空间哈希网格
    /// PBF密度约束求解中，粒子预测位置会不断变化，需要每轮迭代重建哈希表
    /// </summary>
    void buildForPredicted(const std::vector<Particle>& particles, float radius)
    {
        cellSize = radius;
        table.clear();

        for (size_t i = 0; i < particles.size(); ++i)
        {
            unsigned int h = hashPosition(particles[i].predictedPosition);
            table[h].push_back(static_cast<int>(i));
        }
    }

    /// <summary>
    /// 查找给定位置周围所有距离小于cellSize(=h)的粒子索引（即邻居列表）
    /// 查询范围为该位置所在网格及其3×3×3=27邻域内的所有粒子
    /// </summary>
    /// <param name="pos">查询位置（通常是某个粒子的位置）</param>
    /// <param name="particles">所有粒子的数组（用于计算精确距离）</param>
    /// <param name="radius">光滑核半径h，只有距离< h的粒子才被返回</param>
    /// <param name="usePredicted">若为true，使用particles[idx].predictedPosition计算距离；
    /// 否则使用particles[idx].position。PBF密度约束求解时需设为true</param>
    std::vector<int> findNeighbors(const glm::vec3& pos,
                                   const std::vector<Particle>& particles,
                                   float radius, bool usePredicted = false) const
    {
        std::vector<int> neighbors;
        float radiusSq = radius * radius;

        int cx = static_cast<int>(std::floor(pos.x / cellSize));
        int cy = static_cast<int>(std::floor(pos.y / cellSize));
        int cz = static_cast<int>(std::floor(pos.z / cellSize));

        // 遍历周围3×3×3=27个相邻网格单元
        for (int dx = -1; dx <= 1; ++dx)
        {
            for (int dy = -1; dy <= 1; ++dy)
            {
                for (int dz = -1; dz <= 1; ++dz)
                {
                    unsigned int h = hashCell(cx + dx, cy + dy, cz + dz);
                    auto it = table.find(h);
                    if (it == table.end())
                        continue;

                    for (int idx : it->second)
                    {
                        // 根据uPsePredicted标志选择使用实际位置还是预测位置
                        const glm::vec3& p = usePredicted
                            ? particles[idx].predictedPosition
                            : particles[idx].position;
                        glm::vec3 diff = pos - p;
                        float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

                        if (distSq < radiusSq)
                            neighbors.push_back(idx);
                    }
                }
            }
        }

        return neighbors;
    }

private:
    /// <summary>
    /// 将三个维度的整数网格坐标组合为一个unsigned int哈希值
    /// 使用大质数乘法+异或，使得邻近网格的哈希值分布均匀，减少哈希表冲突
    /// </summary>
    unsigned int hashCell(int cx, int cy, int cz) const
    {
        return (static_cast<unsigned int>(cx) * 73856093u) ^
               (static_cast<unsigned int>(cy) * 19349663u) ^
               (static_cast<unsigned int>(cz) * 83492791u);
    }
};

#endif
