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
#include <iostream>

#include "Particle.h"
#include "SpatialHashing.h"

/// <summary>
/// 流体系统类 —— 管理所有流体粒子，负责初始化、更新和提供渲染所需数据
/// </summary>
class FluidSystem
{
public:
    std::vector<Particle> particles;  //所有粒子的数组

    float particleRadius = 0.05f;     //粒子渲染半径（约等于光滑核半径的一半）暂时未使用
    float restDensity    = 1000.0f;   //静止密度 ρ₀（水的参考密度，单位：kg/m³，用于约束求解）

    //水箱边界常量
    glm::vec3 tankMin = glm::vec3(-0.5f, -1.0f, -0.5f);
    glm::vec3 tankMax = glm::vec3( 0.5f,  1.0f,  0.5f);
    // glm::vec3 tankMin = glm::vec3(-1.0f, -1.0f, -1.0f);
    // glm::vec3 tankMax = glm::vec3( 1.0f,  1.0f,  1.0f);
    float restitution = 0.3f;  // 反弹系数

    //PBF 参数
    int   solverIterations = 3; //3~5次 迭代计算即可
    float epsilon = 10.0f; //CFM 正则化参数 一般设为 0.02~0.05 约束防止爆炸
    float h = 0.15f; //光滑核半径 h（PBF算法中用于计算密度和邻居搜索的范围） 略大于spacing 保证有5+个邻居

    //人工压力项参数
    float sCorrK = 0.1f;         // 强度系数
    float sCorrN = 4.0f;         // 幂次
    float sCorrDeltaQ = 0.3f;    // 相对于 h 的偏移比例

    FluidSystem() = default;

    /// <summary>
    /// 辅助函数：Poly6 核函数（用于 W(r) 计算）
    /// </summary>
    float W_poly6(float r2, float h)
    {
        float h2 = h * h;
        if (r2 >= h2 || r2 < 0.0f) return 0.0f;
        float diff = h2 - r2;
        float coeff = 315.0f / (64.0f * 3.141592653589793f * pow(h, 9));
        return coeff * diff * diff * diff;
    }

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
        particleRadius = h * 0.5f;
        //TEST
        // computeDensity();
        // float aveDensity = 0.0f;
        // float maxDensity = 0.0f;
        // for(auto & p : particles)
        // {
        //     aveDensity += p.density;
        //     if(p.density > maxDensity)
        //         maxDensity = p.density;
        // }
        // aveDensity = aveDensity / particles.size();
        // std::cout << "当前密度：" << aveDensity << std::endl;
        // std::cout << "最大密度：" << maxDensity << std::endl;
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
    /// 计算密度：用 Poly6 核在预测位置上累加邻居质量。
    /// 基于当前预测位置（predictedPosition）计算所有粒子的密度。
    /// 使用空间哈希加速邻居搜索，遍历所有邻居用 Poly6 核累加质量贡献。
    /// 结果存储于每个粒子的 density 成员。
    /// </summary>
    void computeDensity()
    {
        float h = this->h;
        float h2 = h * h;
        float poly6Coeff = 315.0f / (64.0f * 3.141592653589793f * pow(h, 9));

        // 重置密度
        for (auto& p : particles) p.density = 0.0f;

        // 构建空间哈希
        SpatialHashing hashGrid;
        hashGrid.buildForPredicted(particles, h);

        // 对每个粒子，获取邻居并累加密度
        for (size_t i = 0; i < particles.size(); ++i)
        {
            auto neighbors = hashGrid.findNeighbors(particles[i].predictedPosition, particles, h, true);
            float rho = 0.0f;
            for (int idx : neighbors)
            {
                const auto& pj = particles[idx];
                glm::vec3 r = particles[i].predictedPosition - pj.predictedPosition;
                float r2 = r.x*r.x + r.y*r.y + r.z*r.z;
                if (r2 >= h2) continue;
                float diff = h2 - r2;
                rho += pj.mass * poly6Coeff * diff * diff * diff;
            }
            particles[i].density = rho;
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
    /// 计算 lambda：根据密度误差和梯度范数，求解约束乘子。
    /// 根据密度约束计算每个粒子的拉格朗日乘子 λ。
    /// 约束函数 C_i = ρ_i/ρ₀ - 1，利用 CFM 正则化（epsilon）避免除零。
    /// 公式：λ_i = -C_i / ( Σ_k ||∇_k C_i||² + ε )
    /// 其中梯度使用 Spiky 核函数计算。
    /// 结果存储于每个粒子的 lambda 成员。
    /// </summary>
    void computeLambda()
    {
        float h = this->h;
        float h2 = h * h;
        float rho0 = restDensity;
        float eps = epsilon;
        float spikyCoeff = -45.0f / (3.141592653589793f * std::pow(h, 6));  // Spiky 梯度系数

        SpatialHashing hashGrid;
        hashGrid.buildForPredicted(particles, h);

        for (size_t i = 0; i < particles.size(); ++i)
        {
            float rho_i = particles[i].density;
            if (rho_i < 1e-6f) rho_i = 1e-6f;
            float C_i = rho_i / rho0 - 1.0f;

            // 计算梯度平方和
            glm::vec3 sumGrad(0.0f);
            float sumSq = 0.0f;
            auto neighbors = hashGrid.findNeighbors(particles[i].predictedPosition, particles, h, true);
            for (int idx : neighbors)
            {
                if (idx == (int)i) continue;
                const auto& pj = particles[idx];
                glm::vec3 r = particles[i].predictedPosition - pj.predictedPosition;
                float rLen = glm::length(r);
                if (rLen >= h) continue;
                // Spiky 梯度
                float hMinusR = h - rLen;
                glm::vec3 gW = spikyCoeff * (hMinusR * hMinusR / rLen) * r;
                sumGrad += gW;
                sumSq += glm::dot(gW, gW);
            }

            float denom = (glm::dot(sumGrad, sumGrad) + sumSq) / (rho0 * rho0) + eps;
            float lambda = -C_i / denom;
            lambda = std::clamp(lambda, -5.0f, 5.0f); // 钳制 lambda 防止爆炸
            particles[i].lambda = lambda;
        }
        //TEST
        // std::cout << "某粒子lambda：" << particles[0].lambda << std::endl;
    }

    /// <summary>
    /// 位置修正：用 λ 和 Spiky 梯度更新预测位置。
    /// 基于 λ 计算每个粒子的位置修正量 Δp，并更新预测位置。
    /// Δp_i = (1/ρ₀) * Σ_j (λ_i + λ_j) * ∇W_spiky(p_i* - p_j*)
    /// 其中 j 遍历所有邻居（包括自身，但实际 j≠i 贡献）。
    /// 修正后预测位置 += Δp_i。
    /// </summary>
    void computeDeltaP()
    {
        float h = this->h;
        float h2 = h * h;
        float rho0 = restDensity;
        float spikyCoeff = -45.0f / (3.141592653589793f * std::pow(h, 6));

        SpatialHashing hashGrid;
        hashGrid.buildForPredicted(particles, h);

        // 先计算每个粒子的总修正量
        std::vector<glm::vec3> deltaP(particles.size(), glm::vec3(0.0f));

        for (size_t i = 0; i < particles.size(); ++i)
        {
            float lambda_i = particles[i].lambda;
            auto neighbors = hashGrid.findNeighbors(particles[i].predictedPosition, particles, h, true);
            for (int idx : neighbors)
            {
                if (idx == (int)i) continue;
                const auto& pj = particles[idx];
                glm::vec3 r = particles[i].predictedPosition - pj.predictedPosition;
                float rLen = glm::length(r);
                if (rLen >= h) continue;
                // Spiky 梯度
                float hMinusR = h - rLen;
                glm::vec3 gW = spikyCoeff * (hMinusR * hMinusR / rLen) * r;

                //增加 scorr 项
                /*
                float r2 = rLen * rLen;
                float wVal = W_poly6(r2, h); //当前粒子的 Poly6 核值
                float wRef = W_poly6((sCorrDeltaQ * h) * (sCorrDeltaQ * h), h); // 参考值
                float ratio = wVal / (wRef + 1e-10f);
                float sCorr = -sCorrK * pow(ratio, sCorrN);
                */

                //deltaP[i] += (lambda_i + particles[idx].lambda + sCorr) * gW;
                deltaP[i] += (lambda_i + particles[idx].lambda) * gW;
            }
            deltaP[i] /= rho0;
            
            float maxCorrection = 0.002f; // 约束 防止爆炸 无sCorr设为0.002
            float len = glm::length(deltaP[i]);
            if (len > maxCorrection)
                deltaP[i] = deltaP[i] / len * maxCorrection;
        }

        // 统一更新预测位置
        for (size_t i = 0; i < particles.size(); ++i)
            particles[i].predictedPosition += deltaP[i];
        //TEST
        // std::cout << "某粒子deltaP：" << deltaP[0].x << " " << deltaP[0].y << " " << deltaP[0].z << std::endl;
    }

    /// <summary>
    /// 边界判断：将预测位置限制在水箱范围内。
    /// 将预测位置 p* 强制约束在水箱边界内部。
    /// 对六个面分别检查，若超出则把该轴坐标设置为边界值。
    /// 用于防止粒子在 PBF 迭代中穿出水箱。
    /// </summary>
    void applyBoundaryConstraints()
    {
        for (auto& p : particles)
        {
            if (p.predictedPosition.x < tankMin.x) p.predictedPosition.x = tankMin.x;
            if (p.predictedPosition.x > tankMax.x) p.predictedPosition.x = tankMax.x;
            if (p.predictedPosition.y < tankMin.y) p.predictedPosition.y = tankMin.y;
            if (p.predictedPosition.y > tankMax.y) p.predictedPosition.y = tankMax.y;
            if (p.predictedPosition.z < tankMin.z) p.predictedPosition.z = tankMin.z;
            if (p.predictedPosition.z > tankMax.z) p.predictedPosition.z = tankMax.z;
        }
    }

    /// <summary>
    /// 应用 XSPH 粘性，平滑速度场，使流体运动更连贯。
    /// </summary>
    /// <param name="xsphCoeff">粘性系数，通常取 0.01~0.03</param>
    void applyXSPH(float xsphCoeff)
    {
        SpatialHashing hashGrid;
        hashGrid.build(particles, h); //基于当前位置构建哈希

        std::vector<glm::vec3> newVelocities(particles.size());
        for (size_t i = 0; i < particles.size(); ++i)
        {
            newVelocities[i] = particles[i].velocity;
            auto neighbors = hashGrid.findNeighbors(particles[i].position, particles, h, false);
            for (int idx : neighbors)
            {
                if (idx == (int)i) continue;
                float r = glm::length(particles[i].position - particles[idx].position);
                float r2 = r * r;
                float w = W_poly6(r2, h);
                newVelocities[i] += xsphCoeff * (particles[idx].velocity - particles[i].velocity) * w;
            }
        }
        for (size_t i = 0; i < particles.size(); ++i)
            particles[i].velocity = newVelocities[i];
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
    /// 执行完整的一帧 PBF 模拟循环。
    /// 顺序：保存原始位置 → 施加重力 → 预测位置 → 初始边界碰撞 →
    ///       迭代求解（密度 → λ → 位置修正 → 边界碰撞）→ 反推速度 → 更新位置。
    /// </summary>
    /// <param name="deltaTime">上一帧耗时（秒）</param>
    void update(float deltaTime)
    {
        //初始化预测位置
        for (auto& p : particles)
        {
            p.predictedPosition = p.position;
        }

        const glm::vec3 gravity = glm::vec3(0.0f, -9.8f, 0.0f);

        //保存原始位置
        std::vector<glm::vec3> originalPositions(particles.size());
        for (size_t i = 0; i < particles.size(); ++i)
        {
            originalPositions[i] = particles[i].position;
        }
            
        //施加重力
        for (auto& p : particles)
        {
            p.velocity += gravity * deltaTime;
        }

        //预测位置
        for (auto& p : particles)
        {
            p.predictedPosition = p.position + p.velocity * deltaTime;
        }
        
        //初始边界碰撞
        applyBoundaryConstraints();

        //PBF 迭代求解
        for (int iter = 0; iter < solverIterations; ++iter)
        {
            //基于预测位置计算密度
            computeDensity();
            //计算 lambda
            computeLambda();
            //计算位置修正并更新预测位置
            computeDeltaP();
            //边界约束（将预测位置夹回边界）
            applyBoundaryConstraints();
        }

        //反推速度
        for (size_t i = 0; i < particles.size(); ++i)
        {
            particles[i].velocity = (particles[i].predictedPosition - originalPositions[i]) / deltaTime;
        }

        //施加XSPH
        //applyXSPH(0.0001f);

        //更新位置
        for (auto& p : particles)
        {
            p.position = p.predictedPosition;
        }
    }
};

#endif