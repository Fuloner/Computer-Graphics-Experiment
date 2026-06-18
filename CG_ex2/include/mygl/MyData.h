#ifndef DATA_H
#define DATA_H

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <mygl/MyShader.h>

#include <string>
#include <vector>

class MyData
{
public:
    std::vector<glm::vec2> points;
    unsigned int VAO, VBO;

    MyData(std::vector<glm::vec2> points)
    {
        this->points = points;

        InitData();
    }

    void InitData()
    {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(glm::vec2), points.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);
        glEnableVertexAttribArray(0);
    }

    void updateData(const std::vector<glm::vec2>& points)
    {
        this->points = points;
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(glm::vec2), points.data(), GL_STATIC_DRAW);
    }

    void DrawPoint(MyShader &shader, float pointSize = 1.0f, glm::vec3 color = glm::vec3(1.0f))
    {
        shader.setVec3("color", color);
        glPointSize(pointSize);
        glBindVertexArray(VAO);
        glDrawArrays(GL_POINTS, 0, points.size());
        glPointSize(1.0f);
    }

    void DrawLine(MyShader &shader, glm::vec3 color = glm::vec3(1.0f))
    {
        shader.setVec3("color", color);
        glBindVertexArray(VAO);
        glDrawArrays(GL_LINE_STRIP, 0, points.size());
    }
};

#endif