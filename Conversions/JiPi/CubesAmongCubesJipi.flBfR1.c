
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image '/media/a/27012b4eadd0c3ce12498b867058e4f717ce79e10a99568cca461682d84a4b04.bin' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


struct Material {
    float3 ka;
    float3 kd;
    float3 ks;
    float a;
};

__DEVICE__ mat2 rot(float a){
    float c = _cosf(a);
    float s = _sinf(a);
    return to_mat2(c, -s, s, c);
}

__DEVICE__ float mdot(float3 a, float3 b){
    return clamp(dot(a, b), 0.0f, 1.0f);
}

__DEVICE__ float rand(float2 u){
    u = 50.0f*fract_f2(u);
    return fract(u.x*u.y*(u.x+u.y));
}

__DEVICE__ float udbox(float3 p, float3 a, float r){
    return length(_fmaxf(abs_f3(p) - a, to_float3_s(0.0f))) - r;
}

__DEVICE__ float sdcone(float3 p, float3 u, float a){
    // u : direction of the cone
    float _y = dot(u, p);
    float _x = length(p - _y*u);
    return _x*_cosf(a) - _y*_sinf(a);
}

__DEVICE__ float noise(float3 p, __TEXTURE2D__ iChannel0){
    float3 i = _floor(p);
    float3 f = fract_f3(p);
    f = smoothstep(to_float3_s(0.0f), to_float3_s(0.0f), f);
    p = i + f;
    return texture(iChannel0, (p+0.5f)/32.0f).x;
}

__DEVICE__ float fbm(float3 p, __TEXTURE2D__ iChannel0){
    float f;
    f = 0.5f    * noise(p * 1.0f,iChannel0);
    f += 0.25f  * noise(p * 2.0f,iChannel0);
    f += 0.125f * noise(p * 4.0f,iChannel0);
    return f;
}






__DEVICE__ void updateMovingCubes(float3 positions[7], float iTime){
  float zzzzzzzzzzzzzzzzzzzzzz;
    const float2 _h = to_float2(-1.0f, 1.0f);
    const float3 path[] = {
      swi3(_h,x,x,x), swi3(_h,x,y,x), swi3(_h,y,y,x), swi3(_h,y,x,x), swi3(_h,y,x,y), swi3(_h,y,y,y), swi3(_h,x,y,y), swi3(_h,x,x,y)
      };
  
    // Updates the positions of the 6 moving cubes
    float t0 = 8.0f*iTime/6.0f;
    float h = 1.0f/6.0f;
    int ki = 0;
    float kf = 0.0f;
    for(; ki < 6; ki++){
        float t = mod_f(t0 + kf*h, 8.0f);
        int i = (int)(_floor(t));
        float f = _fminf(6.0f*fract(t), 1.0f);
        float3 p1 = path[(i+ki)%8];
        float3 p2 = path[(i+ki+1)%8];
        float3 p = _mix(p1, p2, smoothstep(0.0f, 1.0f, f));
        positions[ki] = p;
        kf++;
    }
}

__DEVICE__ float cuboidDst(float3 p, float3 positions[7], float iTime){
       
    // Returns the distance to the moving cubes
    swi2S(p,x,z, mul_f2_mat2(swi2(p,x,z) , rot(0.8f*iTime)));
    swi2S(p,y,z, mul_f2_mat2(swi2(p,y,z) , rot(1.1f*iTime+_sinf(iTime))));
    
    float a = 0.8f;
    
    //float t = mod_f(10.0f*iTime, 22.0f);
    float sp = 1.0f;// + 0.25f*smoothstep(0.0f, 1.0f, t)*smoothstep(10.0f, 9.0f, t);
    
    float r = 0.2f;
    float d = 10000.0f;
    for(int k = 0; k < 6; k++){
        float3 pos = sp*positions[k];
        d = _fminf(d, udbox(p - pos, to_float3_s(a), r));
    }
    
    return d;
}








__DEVICE__ float3 cubeLocal(float3 p, float2 o, float3 cuboidPos, float iTime){
    // Returns the local space of a ground block
    float3 q;
    //swi2(q,x,z) = swi2(p,x,z) - o;
    q.x = p.x - o.x;
    q.z = p.z - o.y;

    float d2c = length(o - swi2(cuboidPos,x,z));
    float att = smoothstep(-2.0f, -4.0f, d2c) + smoothstep(2.0f, 4.0f, d2c) - 0.3f;
    float w = 80.0f;
    float t = iTime;
    float f = (0.8f+0.5f*_fabs(_sinf(w*o.y+t)+_cosf(w*o.x-t)))*att;

    q.y = p.y - f - 0.3f;

    return q;
}

__DEVICE__ float gridCubeDst(float3 p, float2 c, float2 offset, out float2 *closestO, float curD, float3 cuboidPos, float iTime){
    // Returns the distance to a single ground block
    float2 o = c + offset;
    float3 q = cubeLocal(p, o, cuboidPos, iTime);

    float dst = udbox(q+to_float3(0.0f, 1.5f, 0.0f), to_float3(0.3f, 2.0f, 0.3f), 0.1f);

    float sd = step(0.0f, dst - curD);
    *closestO = sd* *closestO + (1.0f-sd)*o;
    
    return dst;
}


__DEVICE__ float sdf(float3 p, out Material *mat, float3 positions[7], float iTime, __TEXTURE2D__ iChannel0, float3 cuboidPos){
    
    
    Material defBoxesMat = {
            to_float3(0.08f, 0.0f, 0.0f),
            to_float3(0.3f, 0.2f, 0.1f),
            to_float3(0.3f, 0.3f, 0.3f),
            100.0f
            };

    Material cuboidMat = {
            to_float3(0.05f, 0.0f, 0.0f),
            to_float3(0.05f,0.0f,0.05f),
            to_float3_s(1.0f),
            10.0f
            };
    
    
    float d = 10000.0f;
    
    // Evaluate distance to the closest ground block by
    // checking its neighbors (thus simulating an infinite grid)
    // It's not very efficient but I don't know if it can be
    // improved
    float2 c = _floor(swi2(p,x,z)) + 0.5f;
    float2 closestO = to_float2_s(0.0f);
    float3 h = to_float3(1.0f, 0.0f, -1.0f);
    for(float x = -1.0f; x <= 1.0f; x++){
        for(float y = -1.0f; y <= 1.0f; y++){
            d = _fminf(d, gridCubeDst(p, c, to_float2(x, y), &closestO, d, cuboidPos, iTime));
        }
    }
float uuuuuuuuuuuuuuuuuuuuuuuu;    
    // Select the color of the material based on the
    // closest ground block
    *mat = defBoxesMat;
    float3 col = sin_f3((closestO.x*closestO.y) + to_float3(0.0f, 0.4f, 0.8f));
    (*mat).kd = defBoxesMat.kd + 0.15f * col;
    (*mat).kd = clamp(1.3f*(*mat).kd, 0.0f, 1.0f);
    
    // Adding noise to a block
    float3 q = cubeLocal(p, closestO, cuboidPos, iTime);
    d += 0.01f*fbm(10.0f*q + 10.0f*rand(closestO),iChannel0);
    
    
    // Distance and material to the moving cubes
    q = p-cuboidPos;
    float dc = cuboidDst(q/0.3f, positions, iTime)*0.3f;
    if(dc < d){
        *mat = cuboidMat;
    }
    d = _fminf(d, dc);

    return d;
}

__DEVICE__ float shadow(float3 ro, float3 rd, float tmin, float tmax, float3 positions[7], float iTime, __TEXTURE2D__ iChannel0, float3 cuboidPos){
    Material _;
    float t = tmin;
    float res = 1.0f;
    for(int i = 0; i < 50 && t < tmax; i++)
    {
        float d = sdf(ro + rd*t, &_, positions, iTime,iChannel0, cuboidPos);
        if(d < 0.0001f)
            return 0.0f;
        res = _fminf(res, 32.0f*d/t);
        t += d;
    }
    return res;
}

__DEVICE__ float2 raymarch(float3 ro, float3 rd, float tmin, float tmax, out Material *mat, float3 positions[7], float iTime, __TEXTURE2D__ iChannel0, float3 cuboidPos){
    float maxSteps = 100.0f;
    float t = tmin;
    for(float i = 0.0f; i < maxSteps && t < tmax; i++){
        float d = sdf(ro + rd * t, mat, positions, iTime,iChannel0,cuboidPos);
        if(d < 0.001f){
            return to_float2(t, i/maxSteps);
        }
        t += d*0.8f;
    }
    
    return to_float2(-1.0f, -1.0f);
}

__DEVICE__ float3 normal(float3 p, float3 positions[7], float iTime, __TEXTURE2D__ iChannel0, float3 cuboidPos){
    float2 h = to_float2(1.0f, 0.0f) * 0.0001f;
    Material _;
    return normalize(to_float3(
        sdf(p+swi3(h,x,y,y), &_, positions,iTime,iChannel0,cuboidPos) - sdf(p-swi3(h,x,y,y), &_, positions,iTime,iChannel0,cuboidPos),
        sdf(p+swi3(h,y,x,y), &_, positions,iTime,iChannel0,cuboidPos) - sdf(p-swi3(h,y,x,y), &_, positions,iTime,iChannel0,cuboidPos),
        sdf(p+swi3(h,y,y,x), &_, positions,iTime,iChannel0,cuboidPos) - sdf(p-swi3(h,y,y,x), &_, positions,iTime,iChannel0,cuboidPos)
    ));
}

__DEVICE__ float3 lighting(float3 p, float3 n, float3 ro, Material mat, float3 positions[7], float iTime, __TEXTURE2D__ iChannel0, float3 cuboidPos, float3 spotDir, float3 spotPos, float spotAngle, float3 lightCol){
    float3 lightDir = -spotDir;
    
    float shad = shadow(p, lightDir, 0.0f, 10.0f, positions,iTime,iChannel0,cuboidPos);

    float3 v = normalize(ro - p);
    float3 r = reflect(-lightDir, n);
    float3 h = normalize(lightDir + v);
    
    float3 amb = to_float3_s(0.0f);
    float3 dif = to_float3_s(0.0f);
    float3 spc = to_float3_s(0.0f);
    
    float3 q = p;
    q -= spotPos;
    float spd = sdcone(q, spotDir, spotAngle);
    
    float3 col = to_float3_s(0.0f);
    
    amb = mat.ka * lightCol;
    dif = mat.kd * lightCol * mdot(n, lightDir);
    spc = mat.ks * lightCol * _powf(mdot(n, h), mat.a);

    float lig = clamp(-spd, 0.0f, 1.5f);
    col = (amb + (dif + spc) * shad) * lig;
    
    return clamp(col, 0.0f, 1.0f);
}


__KERNEL__ void CubesAmongCubesJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
    float3 positions[7];
    
    float3 spotPos = to_float3(5.0f, 10.0f, 5.0f);
    float3 spotDir = normalize(-1.0f*to_float3(5.0f, 10.0f, 5.0f));
    float spotAngle = 0.4f;

    float3 lightCol = to_float3(1.0f, 1.0f, 1.0f);

    float3 cuboidPos = to_float3(0.0f, 1.8f, 0.0f);

    float2 moveDir = 0.5f * to_float2(-1.0f, -1.0f);
float IIIIIIIIIIIIIIIIIIIIII;    

    float3 col = to_float3_s(0.0f);
    
    float2 uv = fragCoord/iResolution;
    uv -= 0.5f;
    uv.x *= iResolution.x/iResolution.y;
    
    float3 ro = to_float3(-1.0f, 4.0f, -5.0f);
    float3 rd = normalize(to_float3_aw(uv, 1.0f));
    
    swi2S(rd,y,z, mul_f2_mat2(swi2(rd,y,z) , rot(0.5f)));
    mat2 rr = rot(iTime*0.1f+2.0f - iMouse.x*0.01f);
    swi2S(ro,x,z, mul_f2_mat2(swi2(ro,x,z) , rr));
    swi2S(rd,x,z, mul_f2_mat2(swi2(rd,x,z) , rr));
    
    float2 disp = moveDir * iTime - iMouse.y * 0.01f;
    //swi2(ro,x,z) += disp;
    ro.x += disp.x;
    ro.z += disp.y;
    //swi2(cuboidPos,x,z) += disp;
    cuboidPos.x += disp.x;
    cuboidPos.z += disp.y;
    //swi2(spotPos,x,z) += disp;
    spotPos.x += disp.x;
    spotPos.z += disp.y;
    
    updateMovingCubes(positions, iTime);
    
    Material mat;
    float2 hit = raymarch(ro, rd, 0.0f, 20.0f, &mat, positions,iTime,iChannel0,cuboidPos);
    float t = hit.x;
    
    if(t > 0.0f){
        float3 p = ro + rd * t;
        float3 n = normal(p, positions,iTime,iChannel0,cuboidPos);
        p += n * 0.01f;
        col = lighting(p, n, ro, mat, positions,iTime,iChannel0,cuboidPos,spotDir,spotPos,spotAngle,lightCol);
    }
    fragColor = to_float4_aw(col,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}