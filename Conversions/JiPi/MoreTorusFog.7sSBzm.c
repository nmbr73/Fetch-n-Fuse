
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


#define MAX_STEPS 100
#define MAX_DIST 50.0f
#define SURF_DIST 0.0001f

#define pi 3.14159f

__DEVICE__ float3 pal(in float t, in float3 a, in float3 b, in float3 c, in float3 d) {
    return a + b*cos_f3( 6.28318f*(c*t+d) );
}

__DEVICE__ mat2 Rot(float a) {
    float s=_sinf(a), c=_cosf(a);
    return to_mat2(c, -s, s, c);
}

__DEVICE__ float3 distort(float3 p, float iTime) {
    float o = 2.0f* pi / 3.0f;
    float t = 3.0f * length(p) - 0.5f * iTime;
   // p = _fabs(p) - 0.5f;
    swi2S(p,x,y, mul_f2_mat2(swi2(p,x,y) , Rot(t - o)));
    swi2S(p,y,z, mul_f2_mat2(swi2(p,y,z) , Rot(t)));
    swi2S(p,z,x, mul_f2_mat2(swi2(p,z,x) , Rot(t + o)));
    return fract_f3(0.8f * p) - 0.5f;
}

__DEVICE__ float GetDist(float3 p, float iTime) {
   
    p = distort(p,iTime); 
    float d = length(swi2(p,x,z)) - 0.5f;
    
    // lower k => more "fog"
    float k = 0.25f;
    //return length(p) -0.3f + SURF_DIST;
    return k * length(to_float2(d, p.y)) + 1.0f * SURF_DIST;
}

__DEVICE__ float RayMarch(float3 ro, float3 rd, float z, float iTime) {
  
    float dO=0.0f;
    
    for(int i=0; i<MAX_STEPS; i++) {
        float3 p = ro + rd*dO;
        float dS = GetDist(p,iTime);
        if(_fabs(dS)<SURF_DIST || dO>MAX_DIST) break;
        dO += dS*z; 
    }
    
    return _fminf(dO, MAX_DIST);
}

__DEVICE__ float3 GetNormal(float3 p, float iTime) {
    float d = GetDist(p,iTime);
    float2 e = to_float2(0.001f, 0);
    
    float3 n = d - to_float3(
        GetDist(p-swi3(e,x,y,y),iTime),
        GetDist(p-swi3(e,y,x,y),iTime),
        GetDist(p-swi3(e,y,y,x),iTime));
    
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

__KERNEL__ void MoreTorusFogFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{
    CONNECT_COLOR0(PalE, 1.0f,1.0f, 1.0f, 1.0f); 

    float2 uv = (fragCoord-0.5f*iResolution)/iResolution.y;

    float t = 0.125f * iTime, o = 2.0f * pi / 3.0f;
    float3 ro = 3.0f * to_float3(_cosf(t - o), _cosf(t), _cosf(t + o));

    float3 rd = GetRayDir(uv, ro, to_float3_s(0), 0.95f);
    float3 col = to_float3_s(0);
   
    float d = RayMarch(ro, rd, 1.0f,iTime);

    if(d<MAX_DIST) {
        float3 p = ro + rd * d;
        float3 n = GetNormal(p, iTime);

        float3 dp = distort(p,iTime);

        float dif = dot(n, normalize(to_float3(1,2,3)))*0.5f+0.5f;
       // col = to_float3_aw(step(0.0f, dif));
        
        // darken with distance from origin
        float v = _expf(-0.31f * length(p));
        
        // idk what this does
        v = smoothstep(0.0f, 1.0f, v);
        v *= v;
      
        // color + lighten
        float3 e = swi3(PalE,x,y,z);//to_float3_s(1);
        col = v * pal(0.77f + 0.15f * length(p), e, e, e, 0.8f * to_float3(0,1,2)/3.0f);    
        //col -= 0.1f;
    }
    
    fragColor = to_float4_aw(col, PalE.w);

  SetFragmentShaderComputedColor(fragColor);
}