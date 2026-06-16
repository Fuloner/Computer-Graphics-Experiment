/// @brief 流体粒子系统 —— PBF算法核心
/// <summary>
/// 包含Particle结构体和FluidSystem类。
/// 第4步新增：Poly6密度核函数、Spiky核函数梯度、密度约束迭代求解器、
/// 人工压力项（s_corr）、初始扰动函数。
///
/// PBF核心算法参考：Macklin & Müller, "Position Based Fluids", SIGGRAPH 2013
/// </summary>
#ifndef FLUIDSYSTEM_H
#define FLUIDSYSTEM_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <climits>

#include "Particle.h"
#include "SpatialHashing.h"

/// ======================================================================
/// SPH光滑核函数（静态内联函数）
/// 这些核函数是PBF算法的数学基础，定义在类外部以便复用
/// ======================================================================

/// <summary>
/// Poly6核函数 —— 用于密度估算（4a）
/// W_poly6(r, h) = 315/(64π·h⁹) · (h² - r²)³    当 |r| ≤ h
///               = 0                                当 |r| > h
/// 参数r2为距离的平方（|r|²），避免开根号运算以提高性能
/// </summary>
inline float W_poly6(float r2, float h)
{
    if (r2 >= h * h || r2 < 0.0f)
        return 0.0f;

    float h2 = h * h;
    float diff = h2 - r2;
    // 常量 315/(64π) ≈ 1.566... 预计算以减少运算
    return (315.0f / (64.0f * 3.14159265358979323846f * std::pow(h, 9.0f)))
           * diff * diff * diff;
}

/// <summary>
/// Spiky核函数梯度 —— 用于位置修正（4b和4c）
/// ∇W_spiky(r, h) = -45/(π·h⁶) · (h - |r|)² · r̂    当 |r| ≤ h
///                = 0                                   当 |r| > h
/// 其中 r̂ = r/|r|（单位方向向量）
///
/// Spiky核的梯度是向外的排斥力：粒子越近，梯度越大，排斥力越强
/// 这是PBF算法中产生不可压缩性约束的核心机制
/// </summary>
/// <param name="r">从源粒子到目标粒子的位移向量（p_i - p_j）</param>
/// <param name="h">光滑核半径</param>
inline glm::vec3 gradW_spiky(const glm::vec3& r, float h)
{
    float rLen = glm::length(r);

    if (rLen <= 0.0f || rLen >= h)
        return glm::vec3(0.0f);

    float hMinusR = h - rLen;
    // -45/(π·h⁶) 为负值，与r方向相同产生排斥力
    float coeff = -45.0f / (3.14159265358979323846f * std::pow(h, 6.0f))
                  * hMinusR * hMinusR / rLen;

    return coeff * r;
}

/// <summary>
/// 流体系统类 —— 管理粒子集合，执行PBF密度约束求解
/// </summary>
class FluidSystem
{
public:
    std::vector<Particle> particles;
    std::vector<int>      neighborCounts;
    std::vector<float>    densities;       // 每个粒子的当前密度估计值ρ_i

    // ---- 可调物理参数 ----
    float particleRadius   = 0.05f;    // 粒子渲染半径
    float kernelRadius     = 0.15f;    // 光滑核半径h（决定邻居搜索范围和力的作用距离）
    float restDensity      = 1000.0f;  // 静止密度ρ₀（水的参考密度，kg/m³）
    int   solverIterations = 3;        // 密度约束求解的迭代次数（越多越精确，经典值3~5）
    float epsilon          = 1e-6f;    // 避免除零的小量

    // ---- s_corr人工压力项参数 ----
    // s_corr用于避免粒子异常聚集（"结块"），在粒子非常接近时产生额外排斥力
    float sCorrK = 0.1f;       // 人工压力强度系数
    float sCorrDeltaQ = 0.3f;  // 参考距离（以h的倍数表示，即0.3h）
    int   sCorrN = 4;          // 幂次（越大衰减越快）
    float sCorrWRef;           // W(Δq, h) —— 参考距离处的核函数值，在初始化时预计算

    FluidSystem()
    {
        // 预计算s_corr的归一化分母：W(0.3h, h)
        // 这避免了每次修正时重复计算
        float deltaQ2 = (sCorrDeltaQ * kernelRadius) * (sCorrDeltaQ * kernelRadius);
        sCorrWRef = W_poly6(deltaQ2, kernelRadius);
        if (sCorrWRef < 1e-10f) sCorrWRef = 1e-10f;
    }

    // ======================== 粒子初始化 ========================

    /// <summary>
    /// 在AABB包围盒内以立方晶格排列初始化粒子，所有粒子质量相等
    /// </summary>
    void initializeParticles(const glm::vec3& minBound, const glm::vec3& maxBound, float spacing)
    {
        particles.clear();

        int countX = static_cast<int>((maxBound.x - minBound.x) / spacing) + 1;
        int countY = static_cast<int>((maxBound.y - minBound.y) / spacing) + 1;
        int countZ = static_cast<int>((maxBound.z - minBound.z) / spacing) + 1;

        particles.reserve(countX * countY * countZ);

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
                    particles.emplace_back(pos, 1.0f);
                }
            }
        }

        particleRadius = kernelRadius * 0.5f;
        neighborCounts.resize(particles.size(), 0);
        densities.resize(particles.size(), 0.0f);
    }

    // ======================== 邻居搜索 ========================

    /// <summary>
    /// 使用空间哈希为每个粒子统计邻居数量
    /// </summary>
    /// <param name="verbose">是否在控制台输出统计信息</param>
    void updateNeighborSearch(bool verbose = false)
    {
        if (particles.empty()) return;

        SpatialHashing hashGrid;
        hashGrid.build(particles, kernelRadius);

        neighborCounts.resize(particles.size());
        int minNeighbors = INT_MAX, maxNeighbors = 0;
        long long totalNeighbors = 0;

        for (size_t i = 0; i < particles.size(); ++i)
        {
            auto neighbors = hashGrid.findNeighbors(particles[i].position, particles, kernelRadius);
            int count = static_cast<int>(neighbors.size()) - 1;  // 减自身
            neighborCounts[i] = count;

            if (count < minNeighbors) minNeighbors = count;
            if (count > maxNeighbors) maxNeighbors = count;
            totalNeighbors += count;
        }

        if (verbose)
        {
            float avg = static_cast<float>(totalNeighbors) / particles.size();
            std::cout << "===== 邻居搜索统计 =====" << std::endl;
            std::cout << "  粒子总数:     " << particles.size() << std::endl;
            std::cout << "  光滑核半径h:  " << kernelRadius << std::endl;
            std::cout << "  最少邻居数:   " << minNeighbors << std::endl;
            std::cout << "  最多邻居数:   " << maxNeighbors << std::endl;
            std::cout << "  平均邻居数:   " << avg << std::endl;
            std::cout << "========================" << std::endl;
        }
    }

    // ======================== 初始扰动（演示用） ========================

    /// <summary>
    /// 对粒子初始位置施加微小随机扰动，破坏完美的晶格排列
    /// 这样密度约束求解器才会产生可见的位置修正效果
    /// 后续步骤加入重力后，扰动不再需要，因为有外力破坏平衡
    /// </summary>
    /// <param name="amount">扰动的最大偏移量（世界单位）</param>
    void perturbInitialPositions(float amount)
    {
        // 仅扰动粒子块中间三分之一的粒子（保留边界粒子不动）
        // 这样块中心密度略高，求解器会将其向外扩张，产生可见的扩散效果
        for (auto& p : particles)
        {
            // 在包围盒的三分之一到三分之二范围内的粒子被扰动
            // 这里用粒子坐标做一个简单判断（对10×10×10晶格有效）
            // 简单起见，对所有粒子施加随机微扰
            float rx = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f * amount;
            float ry = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f * amount;
            float rz = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f * amount;
            p.position += glm::vec3(rx, ry, rz);
            p.predictedPosition = p.position;
        }

        std::cout << "粒子初始位置已施加扰动（幅度: ±" << amount << "），用于验证约束求解器" << std::endl;
        std::cout << "  求解器将通过密度约束将粒子向均匀密度方向调整" << std::endl;
    }

    // ======================== PBF密度约束求解（第4步核心） ========================

    /// <summary>
    /// PBF密度约束求解器 —— 通过迭代修正预测位置，迫使每个粒子的密度趋于ρ₀
    ///
    /// 算法流程（每次迭代）：
    /// 4a. 密度估算：用Poly6核计算每个粒子处的密度ρ_i
    /// 4b. 约束与λ：计算约束C_i = ρ_i/ρ₀ - 1，再计算拉格朗日乘子λ_i
    /// 4c. 位置修正：基于λ_i和λ_j计算Δp_i，施加到预测位置
    /// 4d. 重复：将4a-4c重复solverIterations次
    ///
    /// 注意：此函数操作 predictedPosition，不直接修改 position
    ///       position的更新在速度更新阶段进行（见第5步）
    /// </summary>
    void solveDensityConstraint()
    {
        size_t n = particles.size();
        if (n == 0) return;
        densities.resize(n, 0.0f);

        SpatialHashing hashGrid;
        float h = kernelRadius;
        float rho0 = restDensity;
        float eps = epsilon;

        // ---- PBF迭代求解循环 ----
        for (int iter = 0; iter < solverIterations; ++iter)
        {
            // ---- 每轮迭代重新构建空间哈希（因为预测位置会变化）----
            // 注意：build中使用predictedPosition而非position
            hashGrid.buildForPredicted(particles, h);

            // ============================================================
            // 4a. 密度估算 —— 对每个粒子i
            // ρ_i = Σ_j m_j · W_poly6(p_i* - p_j*, h)
            // 求和范围：所有邻居j（包括自身，因为W(0) > 0）
            // ============================================================
            for (size_t i = 0; i < n; ++i)
            {
                float rho = 0.0f;
                const auto& pi = particles[i].predictedPosition;

                auto neighbors = hashGrid.findNeighbors(pi, particles, h, true);
                for (int jIdx : neighbors)
                {
                    const auto& pj = particles[jIdx].predictedPosition;
                    glm::vec3 r = pi - pj;
                    float r2 = r.x * r.x + r.y * r.y + r.z * r.z;
                    rho += particles[jIdx].mass * W_poly6(r2, h);
                }
                densities[i] = rho;
            }

            // ============================================================
            // 4b. 计算拉格朗日乘子λ_i
            // C_i(p) = ρ_i / ρ₀ - 1
            // λ_i = -C_i / ( Σ_k ||∇_{p_k} C_i||² + ε )
            //
            // 约束梯度（使用Spiky核）：
            //   ∇_i C_i = (1/ρ₀) × Σ_{j≠i} ∇W_spiky(p_i - p_j)
            //   ∇_j C_i = -(1/ρ₀) × ∇W_spiky(p_i - p_j)    （j≠i）
            // ============================================================
            for (size_t i = 0; i < n; ++i)
            {
                float rho_i = densities[i];
                if (rho_i < 1e-6f) rho_i = 1e-6f;

                float C_i = rho_i / rho0 - 1.0f;

                // 计算分母：Σ_k ||∇_k C_i||²
                glm::vec3 sumGrad(0.0f);     // Σ ∇W_spiky，用于计算∇_i C_i
                float sumGradSqNeighbors = 0.0f;  // Σ ||∇W_spiky||²，用于邻居梯度贡献

                const auto& pi = particles[i].predictedPosition;
                auto neighbors = hashGrid.findNeighbors(pi, particles, h, true);

                for (int jIdx : neighbors)
                {
                    if (static_cast<size_t>(jIdx) == i) continue;

                    const auto& pj = particles[jIdx].predictedPosition;
                    glm::vec3 r = pi - pj;
                    float rLen = glm::length(r);
                    if (rLen < 1e-6f || rLen >= h) continue;

                    glm::vec3 gW = gradW_spiky(r, h);
                    sumGrad += gW;
                    sumGradSqNeighbors += glm::dot(gW, gW);
                }

                // ∇_i C_i = sumGrad / ρ₀
                // |∇_i C_i|² = ||sumGrad||² / ρ₀²
                // Σ_{j≠i} |∇_j C_i|² = sumGradSqNeighbors / ρ₀²
                float denom = (glm::dot(sumGrad, sumGrad) + sumGradSqNeighbors) / (rho0 * rho0) + eps;
                particles[i].lambda = -C_i / denom;
            }

            // ============================================================
            // 4c. 位置修正
            // Δp_i = (1/ρ₀) × Σ_{j≠i} (λ_i + λ_j + s_corr) × ∇W_spiky(p_i - p_j)
            //
            // s_corr（人工压力项）：
            //   s_corr = -k × [W_poly6(|p_i-p_j|) / W(Δq)]^n
            //   用于防止粒子过度聚集（"结块"现象）
            // ============================================================
            std::vector<glm::vec3> deltaP(n, glm::vec3(0.0f));

            for (size_t i = 0; i < n; ++i)
            {
                const auto& pi = particles[i].predictedPosition;
                float lambda_i = particles[i].lambda;
                auto neighbors = hashGrid.findNeighbors(pi, particles, h, true);

                for (int jIdx : neighbors)
                {
                    if (static_cast<size_t>(jIdx) == i) continue;

                    const auto& pj = particles[jIdx].predictedPosition;
                    glm::vec3 r = pi - pj;
                    float rLen = glm::length(r);
                    if (rLen < 1e-6f || rLen >= h) continue;

                    float lambda_j = particles[jIdx].lambda;

                    // 计算人工压力项 s_corr
                    float r2 = rLen * rLen;
                    float wVal = W_poly6(r2, h);
                    float ratio = wVal / sCorrWRef;
                    float sCorr = -sCorrK * std::pow(ratio, sCorrN);

                    glm::vec3 gW = gradW_spiky(r, h);
                    deltaP[i] += (lambda_i + lambda_j + sCorr) * gW;
                }

                deltaP[i] /= rho0;
            }

            // 将修正量施加到预测位置
            for (size_t i = 0; i < n; ++i)
            {
                particles[i].predictedPosition += deltaP[i];
            }
        }
    }

    // ======================== 颜色与位置数据获取 ========================

    /// <summary>
    /// 根据当前密度计算逐粒子颜色
    /// 密度接近ρ₀ → 白色（约束满足良好），密度偏离ρ₀ → 蓝色/品红（欠密/过密）
    /// 在视觉上直接反映密度约束的满足程度
    /// </summary>
    std::vector<glm::vec3> getColorData() const
    {
        glm::vec3 lowColor(0.2f, 0.3f, 0.9f);   // 低密度 → 深蓝
        glm::vec3 midColor(0.5f, 0.7f, 1.0f);   // 接近ρ₀ → 浅蓝/白蓝
        glm::vec3 highColor(0.9f, 0.3f, 0.7f);  // 高密度 → 品红

        std::vector<glm::vec3> colors(particles.size());

        if (densities.empty() || restDensity <= 0.0f)
        {
            for (auto& c : colors) c = midColor;
            return colors;
        }

        for (size_t i = 0; i < particles.size(); ++i)
        {
            float ratio = densities[i] / restDensity;  // ρ_i / ρ₀

            if (ratio <= 1.0f)
            {
                // ρ_i/ρ₀ ∈ [0, 1]：低密度到理想密度，蓝→浅蓝
                colors[i] = glm::mix(lowColor, midColor, ratio);
            }
            else
            {
                // ρ_i/ρ₀ ∈ (1, 2+]：过密，浅蓝→品红
                float t = std::min((ratio - 1.0f) / 1.0f, 1.0f);
                colors[i] = glm::mix(midColor, highColor, t);
            }
        }

        return colors;
    }

    /// <summary>
    /// 获取所有粒子当前位置的连续float数组（3N个float）
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

    size_t getParticleCount() const { return particles.size(); }

    /// <summary>
    /// 输出密度约束求解后的统计信息（用于验证算法效果）
    /// </summary>
    void printDensityStats() const
    {
        if (densities.empty()) return;

        float minRho = 1e30f, maxRho = 0.0f, sumRho = 0.0f;
        for (float rho : densities)
        {
            if (rho < minRho) minRho = rho;
            if (rho > maxRho) maxRho = rho;
            sumRho += rho;
        }
        float avgRho = sumRho / densities.size();

        // 计算方差，衡量密度均匀性
        float var = 0.0f;
        for (float rho : densities)
            var += (rho - avgRho) * (rho - avgRho);
        var /= densities.size();

        std::cout << "===== 密度统计 =====" << std::endl;
        std::cout << "  静止密度ρ₀: " << restDensity << std::endl;
        std::cout << "  最小密度:    " << minRho << "  (ρ/ρ₀=" << minRho/restDensity << ")" << std::endl;
        std::cout << "  最大密度:    " << maxRho << "  (ρ/ρ₀=" << maxRho/restDensity << ")" << std::endl;
        std::cout << "  平均密度:    " << avgRho << "  (ρ/ρ₀=" << avgRho/restDensity << ")" << std::endl;
        std::cout << "  密度方差:    " << var << std::endl;
        std::cout << "=====================" << std::endl;
    }

    // ======================== 每帧更新 ========================

    /// <summary>
    /// 每帧更新流体模拟状态
    ///
    /// 第4步的模拟循环：
    ///   1. 将最新位置设置为预测位置（因为第4步尚无外力和速度更新）
    ///   2. 执行密度约束迭代求解
    ///   3. 将求解后的预测位置写回位置（临时，第5步会用速度更新替换）
    /// </summary>
    /// <param name="deltaTime">上一帧耗时（秒），当前未使用</param>
    void update(float /*deltaTime*/)
    {
        if (particles.empty()) return;

        // 将当前位置拷贝到预测位置（第5步会改为 p* = p + v*dt + g*dt²）
        for (auto& p : particles)
            p.predictedPosition = p.position;

        // 执行密度约束求解器
        solveDensityConstraint();

        // 将求解后的预测位置写回position（第5步会改为 v = (p* - p)/dt 再 p = p*）
        for (auto& p : particles)
            p.position = p.predictedPosition;
    }
};

#endif
