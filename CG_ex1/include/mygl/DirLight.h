#ifndef DIRLIGHT_H
#define DIRLIGHT_H

#include <glm/glm.hpp>

#include "MyShader.h"

#include <string>

class DirLight
{
public:
    glm::vec3 direction;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    DirLight(glm::vec3 direction, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular)
        : direction(direction), ambient(ambient), diffuse(diffuse), specular(specular) {}
    
    void setUniform(MyShader& shader, const std::string& name) const
    {
        shader.setVec3(name + ".direction", direction);
        shader.setVec3(name + ".ambient", ambient);
        shader.setVec3(name + ".diffuse", diffuse);
        shader.setVec3(name + ".specular", specular);
    }

    // Getter 和 Setter
    glm::vec3 getDirection() const { return direction; }
    void setDirection(const glm::vec3& dir) { direction = dir; }

    glm::vec3 getAmbient() const { return ambient; }
    void setAmbient(const glm::vec3& amb) { ambient = amb; }

    glm::vec3 getDiffuse() const { return diffuse; }
    void setDiffuse(const glm::vec3& diff) { diffuse = diff; }

    glm::vec3 getSpecular() const { return specular; }
    void setSpecular(const glm::vec3& spec) { specular = spec; }
};

#endif