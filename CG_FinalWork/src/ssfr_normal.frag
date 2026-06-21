#version 330 core

/// @brief SSFR 法线计算片元着色器
/// 参考 PositionBasedFluids 中 computeFluidNormal.comp 的实现

in vec2 vTexCoord;
out vec4 fragNormal;  // RGBA: (nx, ny, nz, 1.0)，视图空间法线

uniform sampler2D depthTex;
uniform mat4 projInverse;
uniform vec2 uScreenSize;  // 屏幕分辨率 (width, height)，float 类型

vec3 getPosViewSpace(ivec2 tc)
{
    vec3 ndc = vec3(vec2(tc) / uScreenSize, texelFetch(depthTex, tc, 0).r);
    ndc = (ndc - 0.5) * 2.0;
    vec4 tmp = projInverse * vec4(ndc, 1.0);
    return tmp.xyz / tmp.w;
}

bool ok(vec3 n) { return !any(isnan(n)) && dot(n,n) > 1e-6; }

void main()
{
    ivec2 tc = ivec2(gl_FragCoord.xy);

    vec3 pc  = getPosViewSpace(tc);
    vec3 ppx = getPosViewSpace(tc + ivec2( 1, 0));
    vec3 pnx = getPosViewSpace(tc + ivec2(-1, 0));
    vec3 ppy = getPosViewSpace(tc + ivec2( 0, 1));
    vec3 pny = getPosViewSpace(tc + ivec2( 0,-1));

    vec3 tX = (abs(ppx.z - pc.z) < abs(pnx.z - pc.z)) ? (ppx - pc) : (pc - pnx);
    vec3 tY = (abs(ppy.z - pc.z) < abs(pny.z - pc.z)) ? (ppy - pc) : (pc - pny);

    vec3 normal = normalize(cross(tX, tY));

    if (!ok(normal))
    {
        vec3 sum = vec3(0.0);
        int cnt = 0;
        for (int dy = -1; dy <= 1; dy++)
        {
            for (int dx = -1; dx <= 1; dx++)
            {
                ivec2 nc = tc + ivec2(dx, dy);
                if (nc.x < 0 || nc.x >= int(uScreenSize.x) ||
                    nc.y < 0 || nc.y >= int(uScreenSize.y))
                    continue;

                vec3 np  = getPosViewSpace(nc);
                vec3 npx = getPosViewSpace(nc + ivec2( 1, 0));
                vec3 nnx = getPosViewSpace(nc + ivec2(-1, 0));
                vec3 npy = getPosViewSpace(nc + ivec2( 0, 1));
                vec3 nny = getPosViewSpace(nc + ivec2( 0,-1));

                vec3 tx = (abs(npx.z - np.z) < abs(nnx.z - np.z)) ? (npx - np) : (np - nnx);
                vec3 ty = (abs(npy.z - np.z) < abs(nny.z - np.z)) ? (npy - np) : (np - nny);
                vec3 nn = normalize(cross(tx, ty));
                if (ok(nn)) { sum += nn; cnt++; }
            }
        }
        normal = (cnt > 0) ? normalize(sum / float(cnt)) : vec3(0.0, 0.0, 1.0);
    }

    fragNormal = vec4(normal, 1.0);
}
