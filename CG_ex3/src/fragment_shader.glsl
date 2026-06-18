#version 330 core
out vec4 FragColor;

//场景数据结构
struct Sphere {
    vec3 center;
    float radius;
    vec3 color;
    int materialType;
    float ior;
};
struct Triangle {
    vec3 v0, v1, v2;
    vec3 color;
    int materialType;
    float ior;
};
struct Light {
    vec3 position;
    float intensity;
};

//uniform数组
uniform Sphere spheres[10];
uniform int sphereCount;
uniform Triangle triangles[10];
uniform int triangleCount;
uniform Light lights[10];
uniform int lightCount;

//摄像机参数
uniform vec3 cameraPos;
uniform float fov;
uniform int width;
uniform int height;

//射线结构
struct Ray {
    vec3 origin;
    vec3 direction;
};

//交点记录
struct HitRecord
{
    float t;
    vec3 point;
    vec3 normal;
    vec3 color;
    int materialType;
    float ior;
    bool hit;
};

//球体相交
bool intersectSphere(Ray ray, Sphere sph, out float t, out vec3 normal)
{
    vec3 L = ray.origin - sph.center;
    float a = dot(ray.direction, ray.direction);
    float b = 2.0 * dot(ray.direction, L);
    float c = dot(L, L) - sph.radius * sph.radius;
    float discriminant = b*b - 4.0*a*c;
    if (discriminant < 0.0) return false;
    float sqrtd = sqrt(discriminant);
    t = (-b - sqrtd) / (2.0*a);
    if (t < 0.001) {
        t = (-b + sqrtd) / (2.0*a);
        if (t < 0.001) return false;
    }
    vec3 hitPoint = ray.origin + ray.direction * t;
    normal = normalize(hitPoint - sph.center);
    return true;
}

//三角形相交
bool intersectTriangle(Ray ray, Triangle tri, out float t, out vec3 normal)
{
    vec3 e1 = tri.v1 - tri.v0;
    vec3 e2 = tri.v2 - tri.v0;
    vec3 p = cross(ray.direction, e2);
    float det = dot(e1, p);
    if (det > -1e-6 && det < 1e-6) return false;
    float inv_det = 1.0 / det;
    vec3 s = ray.origin - tri.v0;
    float u = dot(s, p) * inv_det;
    if (u < 0.0 || u > 1.0) return false;
    vec3 q = cross(s, e1);
    float v = dot(ray.direction, q) * inv_det;
    if (v < 0.0 || u+v > 1.0) return false;
    t = dot(e2, q) * inv_det;
    if (t < 0.001) return false;
    normal = normalize(cross(e1, e2));
    return true;
}

//场景求交 返回最近的交点
HitRecord intersectScene(Ray ray)
{
    HitRecord hitRecord;
    hitRecord.t = 1e20;
    hitRecord.hit = false;

    //与球体求交
    for (int i = 0; i < sphereCount; i++)
    {
        float t; vec3 n;
        if (intersectSphere(ray, spheres[i], t, n)) {
            if (t < hitRecord.t && t > 0.0) {
                hitRecord.t = t;
                hitRecord.point = ray.origin + ray.direction * t;
                hitRecord.normal = n;
                hitRecord.color = spheres[i].color;
                hitRecord.materialType = spheres[i].materialType;
                hitRecord.ior = spheres[i].ior;
                hitRecord.hit = true;
            }
        }
    }

    //与三角形求交
    for (int i = 0; i < triangleCount; i++)
    {
        float t; vec3 n;
        if (intersectTriangle(ray, triangles[i], t, n)) {
            if (t < hitRecord.t && t > 0.0) {
                hitRecord.t = t;
                hitRecord.point = ray.origin + ray.direction * t;
                hitRecord.normal = n;
                //hitRecord.color = triangles[i].color;
                float cx = floor(hitRecord.point.x / 1.0);
                float cz = floor(hitRecord.point.z / 1.0);
                bool even = (int(cx) + int(cz)) % 2 == 0;
                hitRecord.color = even ? vec3(0.8, 0.0, 0.0) : vec3(0.8, 0.8, 0.0);

                hitRecord.materialType = triangles[i].materialType;
                hitRecord.ior = triangles[i].ior;
                hitRecord.hit = true;
            }
        }
    }

    return hitRecord;
}

//简单漫反射光照
vec3 shade(HitRecord hit, Ray ray) {
    vec3 color = vec3(0.0);
    for (int i = 0; i < lightCount; i++) {
        vec3 lightDir = normalize(lights[i].position - hit.point);
        float diff = max(dot(hit.normal, lightDir), 0.0);
        //阴影测试
        Ray shadowRay;
        shadowRay.origin = hit.point + hit.normal * 0.001;
        shadowRay.direction = lightDir;
        HitRecord shadowHit = intersectScene(shadowRay);
        if (!shadowHit.hit || shadowHit.t > length(lights[i].position - hit.point)) {
            color += hit.color * diff * lights[i].intensity;
        }
    }
    //环境光
    color += hit.color * 0.2;
    return color;
}

//反射函数
vec3 reflect(vec3 I, vec3 N)
{
    return I - 2.0 * dot(I, N) * N;
}

//折射函数
vec3 refract(vec3 I, vec3 N, float ior)
{
    float cosi = clamp(dot(I, N), -1.0, 1.0);
    float etai = 1.0, etat = ior;
    vec3 n = N;
    if (cosi < 0.0) {
        cosi = -cosi;
    } else {
        float temp = etai; etai = etat; etat = temp;
        n = -N;
    }
    float eta = etai / etat;
    float k = 1.0 - eta * eta * (1.0 - cosi * cosi);
    if (k < 0.0) return vec3(0.0);
    return eta * I + (eta * cosi - sqrt(k)) * n;
}

//菲涅尔方程
float fresnel(vec3 I, vec3 N, float ior)
{
    float cosi = clamp(dot(I, N), -1.0, 1.0);
    float etai = 1.0, etat = ior;
    if (cosi > 0.0) {
        float temp = etai; etai = etat; etat = temp;
    }
    float sint = etai / etat * sqrt(max(0.0, 1.0 - cosi * cosi));
    if (sint >= 1.0) return 1.0;
    float cost = sqrt(max(0.0, 1.0 - sint * sint));
    cosi = abs(cosi);
    float Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost));
    float Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));
    return (Rs * Rs + Rp * Rp) * 0.5;
}

//主追踪函数
//使用栈来模拟递归
struct RayStack {
    Ray ray;
    float weight;
};
RayStack rayStack[16];
int stackSize;

void pushRay(Ray r, float w)
{
    if (stackSize < 32) {
        rayStack[stackSize].ray = r;
        rayStack[stackSize].weight = w;
        stackSize++;
    }
}
Ray popRay()
{
    stackSize--;
    return rayStack[stackSize].ray;
}
float popWeight()
{
    return rayStack[stackSize].weight;
}

vec3 trace(Ray ray)
{
    vec3 finalColor = vec3(0.0);
    stackSize = 0;
    pushRay(ray, 1.0);
    
    // 限制总迭代次数防止无限循环
    for (int iter = 0; iter < 100 && stackSize > 0; ++iter) {
        Ray currentRay = rayStack[stackSize-1].ray;
        float weight = rayStack[stackSize-1].weight;
        popRay();
        
        HitRecord hit = intersectScene(currentRay);
        if (!hit.hit)
        {
            //背景色
            finalColor += vec3(0.3, 0.8, 0.8) * weight;
            continue;
        }
        
        //漫反射材质
        if (hit.materialType == 0)
        {
            vec3 direct = shade(hit, currentRay);
            finalColor += direct * weight;
        }
        //纯镜面反射
        else if (hit.materialType == 2)
        {
            float kr = fresnel(currentRay.direction, hit.normal, hit.ior);
            vec3 reflectDir = reflect(currentRay.direction, hit.normal);
            Ray newRay;
            newRay.origin = dot(reflectDir, hit.normal) < 0.0 ? hit.point + hit.normal * 0.00001 : hit.point - hit.normal * 0.00001;
            newRay.direction = reflectDir;
            pushRay(newRay, weight * kr);
        }
        //反射+折射
        else if (hit.materialType == 1)
        {
            float kr = fresnel(currentRay.direction, hit.normal, hit.ior);
            //反射光线
            vec3 reflectDir = reflect(currentRay.direction, hit.normal);
            Ray reflectRay;
            reflectRay.origin = dot(reflectDir, hit.normal) < 0.0 ? hit.point + hit.normal * 0.00001 : hit.point - hit.normal * 0.00001;
            reflectRay.direction = reflectDir;
            pushRay(reflectRay, weight * kr);
            //折射光线
            if (kr < 0.9) {
                vec3 refractDir = refract(currentRay.direction, hit.normal, hit.ior);
                if (length(refractDir) > 0.0) {
                    Ray refractRay;
                    refractRay.origin = dot(refractDir, hit.normal) < 0.0 ? hit.point + hit.normal * 0.00001 : hit.point - hit.normal * 0.00001;
                    refractRay.direction = refractDir;
                    pushRay(refractRay, weight * (1.0 - kr));
                }
            }
        }
        //其他材质
        else
        {
            vec3 direct = shade(hit, currentRay);
            finalColor += direct * weight;
        }
    }
    return finalColor;
}

void main()
{
    //生成射线方向
    float aspect = float(width) / float(height);
    float scale = tan(radians(fov * 0.5));
    float x = (gl_FragCoord.x / width) * 2.0 - 1.0;
    float y = (gl_FragCoord.y / height) * 2.0 - 1.0;
    vec3 dir = normalize(vec3(x * scale * aspect, y * scale, -1.0));
    Ray ray;
    ray.origin = cameraPos;
    ray.direction = dir;

    vec3 finalColor = trace(ray);
    FragColor = vec4(finalColor, 1.0);
}