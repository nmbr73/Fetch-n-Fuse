
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


#define AUTO  //Comment this line to gain mouse controll.

#define MAX_STEPS 300
#define SURF_DIST 1e-3
#define MAX_DIST 100.0f
//#define float2 float2
//#define float3 float3
//#define float4 float4
#define lerp _mix

#define saturate(v) clamp(v,0.0f,0.1f)
//#define fmod _fmod
__DEVICE__ float hash21(float2 p) {
    p = fract_f2(p * to_float2(233.34f, 851.74f));
    p += dot(p, p + 23.45f);
    return fract(p.x * p.y);
}
__DEVICE__ float2 hash22(float2 p) {
    float k = hash21(p);
    return to_float2(k, hash21(p + k));
}
__DEVICE__ float sdSphere(float3 p, float s)
{
    return length(p) - s;
}

__DEVICE__ mat3 rotateY(float theta) {
    float c = _cosf(theta);
    float s = _sinf(theta);

    return to_mat3_f3(
                  to_float3(c, 0, s),
                  to_float3(0, 1, 0),
                  to_float3(-s, 0, c)
    );
}
__DEVICE__ float opSmoothUnion(float d1, float d2, float k) {
    float h = clamp(0.5f + 0.5f * (d2 - d1) / k, 0.0f, 1.0f);
    return lerp(d2, d1, h) - k * h * (1.0f - h);
}
__DEVICE__ float opSmoothSubtraction(float d1, float d2, float k) {
    float h = clamp(0.5f - 0.5f * (d2 + d1) / k, 0.0f, 1.0f);
    return lerp(d2, -d1, h) + k * h * (1.0f - h);
}
__DEVICE__ float sdPlane(float3 p, float4 n)
{
    return dot(p, swi3(n,x,y,z)) + n.w;
}
__DEVICE__ float sdRoundBox(float3 p, float3 b, float r)
{
    float3 q = abs_f3(p) - b;
    return length(_fmaxf(q, to_float3_s(0.0f))) + _fminf(_fmaxf(q.x, _fmaxf(q.y, q.z)), 0.0f) - r;
}

__DEVICE__ float sdPig(float3 p,float jump, float iTime) {
    p*= 1.0f + to_float3(-0.2f,0.2f,-0.2f)*(0.5f+0.5f*_sinf(iTime*10.0f+3.5f));
    float3 j = to_float3(0.0f, -jump, 0.0f);
    p.x = _fabs(p.x);
    float g = opSmoothUnion(sdRoundBox(p+j, to_float3_s(1.0f), 0.1f), sdSphere(p + j, 1.2f), 1.0f); //Main Body
    g = _fminf(g,
            opSmoothUnion(
                          sdRoundBox(p - to_float3(0, -0.25f, 0.9f) + j, to_float3(0.4f, 0.3f, 0.5f), 0.1f),
                          sdSphere(p - to_float3(0, -0.25f, 0.9f) + j, 0.5f), 0.5f) //nose
                         );
    float s = sdRoundBox(p - to_float3(0.2f, -0.25f, 1.5f) + j, to_float3(0.03f, 0.13f, 0.2f), 0.05f); //nostrile
    s = _fminf(s, sdRoundBox(p - to_float3(0.4f, 0.5f, 1.3f) + j, to_float3(0.05f, 0.2f, 0.05f), 0.05f)); //eye
    return opSmoothSubtraction(s, g, 0.02f);
}

__DEVICE__ float sdBridge(float3 p, float t) {
    float gap = 2.4f;
    float tread = _fminf(_fmod(t, 3.141529f * 2.0f) / 3.141529f, 1.0f) * gap;
    float backScale = smoothstep(3.141529f * 2.0f, 3.141529f, _fmod(t, 3.141529f * 2.0f));
    float frontScale = smoothstep(0.0f, 3.141529f, _fmod(t, 3.141529f * 2.0f));
    float g = _fminf(
                    sdRoundBox(p - to_float3(0.0f, -2.3f - ((1.0f - backScale) * 3.0f), gap * -1.0f - tread), to_float3_s(backScale), 0.1f),
                    sdRoundBox(p - to_float3(0.0f, -2.3f, 0.0f - tread), to_float3_s(1.0f), 0.1f)
                    );
    g = _fminf(g, sdRoundBox(p - to_float3(0.0f, -2.3f, gap - tread), to_float3_s(1.0f), 0.1f));
    float alternate = _fmod(_floor(t / (3.141529f * 2.0f)), 2.0f);
    p = (mul_mat3_f3(rotateY(alternate > 0.5f ? (frontScale - 1.0f) : (1.0f - frontScale)) , p));
    return _fminf(g, sdRoundBox(p - to_float3(0.0f, -2.3f, gap * 2.0f - tread), to_float3_s(frontScale), 0.1f));
}
      
__DEVICE__ float GetDist(float3 p, float iTime) {
float zzzzzzzzzzzzzzzzzzzz;    
    float t = iTime * 10.0f;
    //float2 id = _floor(swi2(p,x,z) * 0.2f);
    //swi2(p,x,z) = fract(swi2(p,x,z) * 0.2f) *5.0f - 2.5f;
    //float2 h = hash22(id);
    float g = sdPig(p, _fmaxf(_sinf(iTime * 10.0f /*+ h.x * 3.141529f * 2.0f*/), 0.0f), iTime);
    //g = _fminf(g, sdPlane(p-float3(0,-1.3f,0), float4(0, 1, 0, 0)));
    g = _fminf(g, sdBridge(p, t));
    
    return g;
}
__DEVICE__ float CalculateAO(float3 p, float3 n, float iTime) {
    float d = 0.6f;
    return smoothstep(0.0f,d,GetDist(p + n*d, iTime));
}
__DEVICE__ float Raymarch(float3 ro, float3 rd, float iTime) {
    float dO = 0.0f;
    float dS;
    for (int i = 0; i < MAX_STEPS; i+=1) {
        float3 p = ro + rd * dO;
        dS = GetDist(p, iTime);
        dO += dS;
        if (dS<SURF_DIST || dO>MAX_DIST) break;
    }
    return dO;
}

__DEVICE__ float3 GetNormal(float3 p, float iTime) {
    float2 e = to_float2(1e-2, 0.0f);

    float3 n = GetDist(p,iTime) - to_float3(
                                  GetDist(p-swi3(e,x,y,y),iTime),
                                  GetDist(p-swi3(e,y,x,y),iTime),
                                  GetDist(p-swi3(e,y,y,x),iTime)
                                  );

    return normalize(n);
}

__DEVICE__ float4 scene (float3 ro,float3 rd, float iTime, float3 Color[5], out bool *alpha)
{

    float3 col = to_float3_s(0);

    float d = Raymarch(ro, rd, iTime);
    float3 light1Dir = normalize(to_float3(0.8f, 1, 0.2f));
    float3 light1Color = to_float3(1, 0.9f, 0.9f);

    if (d < MAX_DIST) {
        float3 p = ro + d * rd;
        float3 n = GetNormal(p, iTime);
        float ground = smoothstep(-1.18f, -1.19f, p.y);
        //col = lerp(to_float3(1, 0.7f, 0.8f), to_float3(0.5f, 0.6f, 0.9f), ground);
        col = lerp(Color[0], Color[1], ground);
        col += _powf(saturate(dot(reflect(rd, n), light1Dir)), 0.6f) * light1Color * 0.3f;
        col += n * 0.15f;
        col *= CalculateAO(p, n, iTime) * 0.4f + 0.6f;
        *alpha = false;
    }
    else
    {
        float3 bg = lerp(Color[2], Color[3], rd.x);
        bg = lerp(bg, Color[4], rd.y);
        col = bg;
        *alpha = true;
    }
    return to_float4_aw(col, 0.0f);
}

__DEVICE__ mat2 Rot(float a) {
    float s = _sinf(a);
    float c = _cosf(a);
    return to_mat2(c, -s, s, c);
}

__DEVICE__ float3 GetRayDir(float2 uv, float3 p, float3 l, float z) {
    float3 f = normalize(l-p),
        r = normalize(cross(to_float3(0,1,0), f)),
        u = cross(f,r),
        c = p+f*z,
        i = c + uv.x*r + uv.y*u,
        d = normalize(i-p);
    return d;
}


__KERNEL__ void PigSquadJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse)
{
    CONNECT_COLOR0(Color1, 1.0f, 0.7f, 0.8f, 1.0f);
    CONNECT_COLOR1(Color2, 0.5f, 0.6f, 0.9f, 1.0f);
    CONNECT_COLOR2(Color3, 1.0f, 0.7f, 0.8f, 1.0f);
    CONNECT_COLOR3(Color4, 0.5f, 0.6f, 0.9f, 1.0f);
    CONNECT_COLOR4(Color5, 0.8f, 0.5f, 0.8f, 1.0f);

    float3 Color[] = {swi3(Color1,x,y,z),swi3(Color2,x,y,z),swi3(Color3,x,y,z),swi3(Color4,x,y,z),swi3(Color5,x,y,z)};

    bool alphasignal; 
    float alpha = 1.0f;

    float2 uv = (fragCoord-0.5f*iResolution)/iResolution.y;
    float2 m = swi2(iMouse,x,y)/iResolution;
    
    float3 col = to_float3_s(0);

    float3 ro = to_float3(0, 5, -5);
    if(iMouse.w<0.5f){
      swi2S(ro,y,z, mul_f2_mat2(swi2(ro,y,z) , Rot(-0.4f)));
      swi2S(ro,x,z, mul_f2_mat2(swi2(ro,x,z) , Rot(iTime*0.5f+2.0f)));
    }else{
      swi2S(ro,y,z, mul_f2_mat2(swi2(ro,y,z) , Rot(-m.y*3.14f+1.0f)));
      swi2S(ro,x,z, mul_f2_mat2(swi2(ro,x,z) , Rot(-m.x*6.2831f)));
    }
    float3 rd = GetRayDir(uv, ro, to_float3_s(0), 1.0f);
    
    fragColor = scene(ro,rd, iTime, Color, &alphasignal);

    if (!alphasignal) alpha = Color1.w; 
    else              alpha = Color3.w;
    
    fragColor.w = alpha;

  SetFragmentShaderComputedColor(fragColor);
}