#version 330 core
out vec4 FragColor;

vec3 CalcDiffuseTexture();
vec3 CalcNormalTexture();
vec3 CalcColor();
vec3 CalcNormalColor();


in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

in mat3 TBN;

struct Light {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform sampler2D texture_normal1;

uniform Light pointLight;
uniform vec3 viewPos;

void main()
{
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(pointLight.position - FragPos);

    //法线值
    vec3 normalColor = texture(texture_normal1, TexCoords).rgb;
    norm = normalColor * 2.0 - 1.0; //将法线值从[0,1]范围转换到[-1,1]范围
    norm = TBN * norm; //切线空间 到 世界空间

    //环境光
    //vec3 ambient = pointLight.ambient * vec3(texture(texture_diffuse1, TexCoords));
    vec3 ambient = pointLight.ambient * vec3(texture(texture_normal1, TexCoords));
    //vec3 ambient = pointLight.ambient * vec3(1.0f);

    //漫反射
    float diff = max(dot(norm, lightDir), 0.0);
    //vec3 diffuse = pointLight.diffuse * diff * vec3(texture(texture_diffuse1, TexCoords));
    //vec3 diffuse = pointLight.diffuse * diff * vec3(texture(texture_normal1, TexCoords));
    vec3 diffuse = pointLight.diffuse * diff * vec3(1.0f , 0.0f , 0.0f);

    //镜面光
    vec3 reflectDir = reflect(-lightDir, norm);
    vec3 viewDir = normalize(viewPos - FragPos);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    //vec3 specular = pointLight.specular * spec * vec3(1.0f);
    vec3 specular = pointLight.specular * spec * vec3(texture(texture_normal1, TexCoords));

    //FragColor = vec4(diffuse + specular + ambient, 1.0);
    //FragColor = vec4( diffuse, 1.0);
    FragColor = vec4(CalcColor(), 1.0);
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

//纯色 + 法线贴图 无光照效果
vec3 CalcNormalColor()
{
    vec3 normalTex = texture(texture_normal1, TexCoords).rgb;
    vec3 norm = normalTex * 2.0 - 1.0; //将法线值从[0,1]范围转换到[-1,1]范围
    norm = TBN * norm; //切线空间 到 世界空间
    vec3 color = vec3(1.0f , 0.0f , 0.0f); //纯红色
    
    vec3 finalColor = color * norm; //将纯色与法线值相乘，得到最终颜色
    return finalColor;
}