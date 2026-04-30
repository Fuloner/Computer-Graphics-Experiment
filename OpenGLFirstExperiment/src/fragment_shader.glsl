#version 330 core
out vec4 FragColor;

//光源类型
struct DirLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;       
};

//函数声明
vec3 CalcNormal();
vec3 CalcDiffuseTexture();
vec3 CalcNormalTexture();
vec3 CalcColor();
vec3 CalcNormalColor();
vec3 CalcPhone(vec3 normal, vec3 fragPos, vec3 viewDir);

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

in mat3 TBN;

//贴图
uniform sampler2D texture_diffuse1;
//uniform sampler2D texture_specular1;
uniform sampler2D texture_normal1;

#define MAX_LIGHT_NUM 16
uniform int dirLightNum;
uniform int pointLightNum;
uniform int spotLightNum;
uniform DirLight dirLights[1];
uniform PointLight pointLights[MAX_LIGHT_NUM];
uniform SpotLight spotLights[MAX_LIGHT_NUM];

uniform vec3 viewPos;
uniform int shadeMode;

void main()
{
    vec3 normal = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 result;

    if(shadeMode == 0)
    {
        result = CalcNormal();
    }
    else if(shadeMode == 1)
    {
        result = CalcDiffuseTexture();
    }
    else if(shadeMode == 2)
    {
        result = CalcNormalTexture();
    }
    else if(shadeMode == 3)
    {
        result = CalcColor();
    }
    else if(shadeMode == 4)
    {
        result = CalcNormalColor();
    }
    else if(shadeMode == 5)
    {
        result = CalcPhone(normal, FragPos, viewDir);
    }

    FragColor = vec4(result, 1.0);
}

vec3 CalcNormal()
{
    return Normal * 0.5 + 0.5;
}

//漫反射贴图 无光照效果
vec3 CalcDiffuseTexture()
{
    return vec3(texture(texture_diffuse1, TexCoords));
}

//法线贴图 无光照效果
vec3 CalcNormalTexture()
{
    return vec3(texture(texture_normal1, TexCoords));
}

//纯色
vec3 CalcColor()
{
    return vec3(1.0f , 1.0f , 1.0f); //纯白色
}

//纯色 + 法线贴图 
vec3 CalcNormalColor()
{
    vec3 lightDir = normalize(vec3(1.0f) - FragPos);
    vec3 normal = texture(texture_normal1, TexCoords).rgb;
    normal = normal * 2.0 - 1.0; //将法线值从[0,1]范围转换到[-1,1]范围
    normal = TBN * normal; //切线空间 到 世界空间
    float normalValue = max(dot(normal, lightDir), 0.0);
    
    vec3 finalColor = normalValue * vec3(1.0f , 1.0f , 0.0f);
    return finalColor;
}


//单光源计算函数
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    // 漫反射着色
    float diff = max(dot(normal, lightDir), 0.0);
    // 镜面光着色
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0f);
    // 合并结果
    vec3 ambient  = light.ambient  * vec3(texture(texture_diffuse1, TexCoords));
    vec3 diffuse  = light.diffuse  * diff * vec3(texture(texture_diffuse1, TexCoords));
    vec3 specular = light.specular * spec * vec3(0.8f);
    return (ambient + diffuse + specular);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // 漫反射着色
    float diff = max(dot(normal, lightDir), 0.0);
    // 镜面光着色
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0f);
    // 衰减
    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    // 合并结果
    vec3 ambient  = light.ambient  * vec3(texture(texture_diffuse1, TexCoords));
    vec3 diffuse  = light.diffuse  * diff * vec3(texture(texture_diffuse1, TexCoords));
    vec3 specular = light.specular * spec * vec3(0.8f);
    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // 漫反射着色
    float diff = max(dot(normal, lightDir), 0.0);
    // 镜面光着色
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0f);
    // 衰减
    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    // 聚光强度
    float theta     = dot(lightDir, normalize(-light.direction)); 
    float epsilon   = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    // 合并结果
    vec3 ambient  = light.ambient  * vec3(texture(texture_diffuse1, TexCoords));
    vec3 diffuse  = light.diffuse  * diff * vec3(texture(texture_diffuse1, TexCoords));
    vec3 specular = light.specular * spec * vec3(0.8f);
    ambient  *= attenuation * intensity;
    diffuse  *= attenuation * intensity;
    specular *= attenuation * intensity;
    return (ambient + diffuse + specular);
}

//Phone光照模型
vec3 CalcPhone(vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 result = vec3(0.0);

    for (int i = 0; i < dirLightNum; ++i) {
        result += CalcDirLight(dirLights[i], normal, viewDir);
    }

    for (int i = 0; i < pointLightNum; ++i) {
        result += CalcPointLight(pointLights[i], normal, FragPos, viewDir);
    }

    for (int i = 0; i < spotLightNum; ++i) {
        result += CalcSpotLight(spotLights[i], normal, FragPos, viewDir);
    }

    return result;
}