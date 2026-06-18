#ifndef SPOTLIGHT_H
#define SPOTLIGHT_H

#include <glm/glm.hpp>

#include "MyShader.h"

#include <string>

class SpotLight
{
public:
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float cutOff;
    float outerCutOff;
    float constant;
    float linear;
    float quadratic;

    SpotLight(glm::vec3 position, glm::vec3 direction, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular,
              float cutOff, float outerCutOff, float constant, float linear, float quadratic)
        : position(position), direction(direction), ambient(ambient), diffuse(diffuse), specular(specular),
          cutOff(cutOff), outerCutOff(outerCutOff), constant(constant), linear(linear), quadratic(quadratic) {}
    
    void setUniform(MyShader& shader, const std::string& name) const
    {
        shader.setVec3(name + ".position", position);
        shader.setVec3(name + ".direction", direction);
        shader.setVec3(name + ".ambient", ambient);
        shader.setVec3(name + ".diffuse", diffuse);
        shader.setVec3(name + ".specular", specular);
        shader.setFloat(name + ".cutOff", cutOff);
        shader.setFloat(name + ".outerCutOff", outerCutOff);
        shader.setFloat(name + ".constant", constant);
        shader.setFloat(name + ".linear", linear);
        shader.setFloat(name + ".quadratic", quadratic);
    }

    // Getter 和 Setter 函数
    glm::vec3 getPosition() const { return position; }
    void setPosition(const glm::vec3& pos) { position = pos; }

    glm::vec3 getDirection() const { return direction; }
    void setDirection(const glm::vec3& dir) { direction = dir; }

    glm::vec3 getAmbient() const { return ambient; }
    void setAmbient(const glm::vec3& amb) { ambient = amb; }

    glm::vec3 getDiffuse() const { return diffuse; }
    void setDiffuse(const glm::vec3& diff) { diffuse = diff; }

    glm::vec3 getSpecular() const { return specular; }
    void setSpecular(const glm::vec3& spec) { specular = spec; }

    float getCutOff() const { return cutOff; }
    void setCutOff(float cut) { cutOff = cut; }

    float getOuterCutOff() const { return outerCutOff; }
    void setOuterCutOff(float outerCut) { outerCutOff = outerCut; }

    float getConstant() const { return constant; }
    void setConstant(float cons) { constant = cons; }

    float getLinear() const { return linear; }
    void setLinear(float lin) { linear = lin; }

    float getQuadratic() const { return quadratic; }
    void setQuadratic(float quad) { quadratic = quad; }
};

#endif