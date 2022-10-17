
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


#define pi 3.14159f

#define thc(a,b) _tanhf(a*_cosf(b))/_tanhf(a)
#define ths(a,b) _tanhf(a*_sinf(b))/_tanhf(a)
#define sabs(x) _sqrtf(x*x+1e-2)

__DEVICE__ float3 pal( in float t, in float3 a, in float3 b, in float3 c, in float3 d )
{
    return a + b*cos_f3( 6.28318f*(c*t+d) );
}

__DEVICE__ float h21 (float2 a) {
    return fract(_sinf(dot(swi2(a,x,y), to_float2(12.9898f, 78.233f))) * 43758.5453123f);
}

__DEVICE__ float mlength(float2 uv) {
    return _fmaxf(_fabs(uv.x), _fabs(uv.y));
}

__DEVICE__ float mlength(float3 uv) {
    return _fmaxf(max(_fabs(uv.x), _fabs(uv.y)), _fabs(uv.z));
}

// (SdSmoothMin) stolen from here: https://www.shadertoy.com/view/MsfBzB
__DEVICE__ float smin(float a, float b)
{
    float k = 0.12f;
    float h = clamp(0.5f + 0.5f * (b-a) / k, 0.0f, 1.0f);
    return _mix(b, a, h) - k * h * (1.0f - h);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


#define MAX_STEPS 400
#define MAX_DIST 10.0f
#define SURF_DIST 0.001f 

//nabbed from blacklemori
__DEVICE__ float3 erot(float3 p, float3 ax, float rot) {
  return _mix(dot(ax, p)*ax, p, _cosf(rot)) + cross(ax,p)*_sinf(rot);
}

__DEVICE__ mat2 Rot(float a) {
    float s=_sinf(a), c=_cosf(a);
    return to_mat2(c, -s, s, c);
}

__DEVICE__ float3 distort(float3 p, float iTime) {
    float time = 0.25f * iTime;
    
    float sc = 1.5f;//_expf(-0.5f * length(p));// + 0.5f * _cosf(0.5f * length(p) - iTime);
    
    float tp = 2.0f*pi/3.0f;
    //float val = length(p);//.;//_cosf(time)*p.x + _cosf(time + tp)*p.y + _cosf(time-tp)*p.z;
    
    float c  = _cosf(time + sc * smin(p.y, p.z) + pi * _cosf(0.1f * iTime));
    float c2 = _cosf(time + sc * smin(p.z, p.x) + pi * _cosf(0.1f * iTime + tp));
    float c3 = _cosf(time + sc * smin(p.x, p.y) + pi * _cosf(0.1f * iTime - tp));

    float3 q = erot(normalize(p), normalize(to_float3(c,c2,c3)), pi + 0.3f * _cosf(2.0f * length(p) - iTime));
    //q = cross(q, to_float3(c3,c,c2));
    return cross(p, q);
}

__DEVICE__ float GetDist(float3 p, float iTime) {
   
    float sd = length(p - to_float3(0, 2, -4)) - 1.2f;
    
    //p = _mix(sabs(p) - 0.0f, sabs(p) - 1.0f, 0.5f + 0.5f * thc(4.0f, iTime));
       
    swi2S(p,x,z, mul_f2_mat2(swi2(p,x,z) , Rot(0.2f * iTime)));
    p = distort(p,iTime);
    
    //swi2(p,x,z) *= Rot(4.0f * p.y + iTime);
    //p = sabs(p) - 0.25f;
    
    float d = length(p) - 0.5f;
    d *= 0.05f; // MUCH smaller than I'd like it to be
       
    // looks okayish with torus
    /*
    float r1 = 2.0f;
    float r2 = 0.2f;
    float d1 = length(swi2(p,x,z)) - r1;
    
    float a = _atan2f(p.x ,p.z);
    float2 u = to_float2(d1, p.y);  
    u *= Rot(1.5f * a);
    u.x = _fabs(u.x) - r2;
    float d2 = length(u) - r2;
    d2 *= 0.05f;
    */
    
    d = -smin(-d, sd); 
    
    return d;
}

__DEVICE__ float RayMarch(float3 ro, float3 rd, float z, float iTime) {
    float dO=0.0f;
    
    for(int i=0; i<MAX_STEPS; i++) {
        float3 p = ro + rd*dO;
        float dS = z * GetDist(p,iTime);
        dO += dS;
        if(dO>MAX_DIST || _fabs(dS)<SURF_DIST) break;
    }
    
    return dO;
}

__DEVICE__ float3 GetNormal(float3 p, float iTime) {
    float d = GetDist(p, iTime);
    float2 e = to_float2(0.001f, 0);
    
    float3 n = d - to_float3(
        GetDist(p-swi3(e,x,y,y), iTime),
        GetDist(p-swi3(e,y,x,y), iTime),
        GetDist(p-swi3(e,y,y,x), iTime));
    
    return normalize(n);
}

__DEVICE__ float3 GetRayDir(float2 uv, float3 p, float3 l, float z) {
    float3 f = normalize(l-p),
        r = normalize(cross(to_float3(0,1,0), f)),
        u = cross(f,r),
        c = f*z,
        i = c + uv.x*r + uv.y*u,
        d = normalize(i);
    return d;
}

__KERNEL__ void WeirdObjectFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse)
{

    float2 uv = (fragCoord-0.5f*iResolution)/iResolution.y;
    float2 m = swi2(iMouse,x,y)/iResolution;

    float3 ro = to_float3(0, 2, -4);
    swi2S(ro,y,z, mul_f2_mat2(swi2(ro,y,z) , Rot(-m.y*3.14f+1.0f)));
    swi2S(ro,x,z, mul_f2_mat2(swi2(ro,x,z) , Rot(-m.x*6.2831f)));
    
    float3 rd = GetRayDir(uv, ro, to_float3(0,0.0f,0), 1.0f);
    float3 col = to_float3_s(0);
   
    float d = RayMarch(ro, rd, 1.0f, iTime);

    float IOR = 1.5f;
    if(d<MAX_DIST) {
        float3 p = ro + rd * d;
        float3 n = GetNormal(p, iTime);

        // float dif = dot(n, normalize(to_float3(1,2,3)))*0.5f+0.5f;
        // col = to_float3_aw(dif);
        
        float fresnel = _powf(1.0f+dot(rd, n), 2.0f);
        col = to_float3_s(fresnel);
        
        // p = distort(p);
        col *= 2.2f + 1.8f * thc(14.0f, 24.0f * length(p) - 1.0f * iTime);
        col = clamp(col, 0.0f, 1.0f);
        col *= 1.0f-_expf(-1.1f - 0.5f * p.y);
        col *= 0.9f + 0.5f * n.y;
        float3 e = to_float3_s(1.0f);
        col *= pal(length(p) * 0.2f - 0.08f * iTime, e, e, e, 0.5f * to_float3(0,1,2)/3.0f);
        col = clamp(col, 0.0f, 1.0f);
        col *= 2.8f * _expf(-0.8f * length(p));
    }
    
    col = pow_f3(col, to_float3_s(0.4545f));  // gamma correction
    
    fragColor = to_float4_aw(col,1.0f);


  SetFragmentShaderComputedColor(fragColor);
}