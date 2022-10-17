
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

__DEVICE__ float mlength_f2(float2 uv) {
    return _fmaxf(_fabs(uv.x), _fabs(uv.y));
}

__DEVICE__ float mlength_f3(float3 uv) {
    return _fmaxf(_fmaxf(_fabs(uv.x), _fabs(uv.y)), _fabs(uv.z));
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
#define MAX_DIST 50.0f
#define SURF_DIST 0.001f

//nabbed from blacklemori
__DEVICE__ float3 erot(float3 p, float3 ax, float rot) {
  return _mix(dot(ax, p)*ax, p, _cosf(rot)) + cross(ax,p)*_sinf(rot);
}

__DEVICE__ mat2 Rot(float a) {
    float s=_sinf(a), c=_cosf(a);
    return to_mat2(c, -s, s, c);
}

__DEVICE__ float sdBox(float3 p, float3 s) {
    p = abs_f3(p)-s;
  return length(_fmaxf(p, to_float3_s(0.0f)))+_fminf(_fmaxf(p.x, _fmaxf(p.y, p.z)), 0.0f);
}

__DEVICE__ float3 distort(float3 p, float iTime) {
    float time = 0.5f * iTime;
    
    float3 q = p;
    float m = 3.5f + 3.0f * _cosf( 0.4f * length(p) -  0.5f * iTime);
    float th = 0.2f * iTime;
    for (float i = 0.0f; i < 9.0f; i+=1.0f) {
        th += -0.1f * iTime;
        swi2S(q,x,y, mul_f2_mat2(swi2(q,x,y) , Rot(th)));
        swi2S(q,z,y, mul_f2_mat2(swi2(q,z,y) , Rot(th)));
        q = abs_f3(q) - m; //sabs cool too
        m *= 0.28f + 0.1f * i;
    }
    
    float spd = 0.01f;
    //float time = iTime;
    float cx = _cosf(time);
    float cy = _cosf(time + 2.0f * pi / 3.0f);
    float cz = _cosf(time - 2.0f * pi / 3.0f);
    q = erot(q, normalize(to_float3(cx,cy,cz)), iTime);
    return cross(p, q);
}

__DEVICE__ float GetDist(float3 p, float iTime) {
   
    
    float sd = length(p - to_float3(0, 3.0f, -3.5f)) - 2.2f;
    
    //p = _mix(sabs(p) - 0.0f, sabs(p) - 1.0f, 0.5f + 0.5f * thc(4.0f, iTime));
    
    p = distort(p, iTime);
    //swi2(p,x,z) *= Rot(4.0f * p.y + iTime);
   // p = sabs(p) - 0.25f;
    float d = length(p) - 1.0f; // was 0.8
    d *= 0.05f; //smaller than I'd ike it to be
    d = -smin(-d, sd); 
    
    return d;
}

__DEVICE__ float RayMarch(float3 ro, float3 rd, float z, float iTime) {
  float dO=0.0f;
    
    for(int i=0; i<MAX_STEPS; i++) {
      float3 p = ro + rd*dO;
        float dS = z * GetDist(p, iTime);
        dO += dS;
        if(dO>MAX_DIST || _fabs(dS)<SURF_DIST) break;
    }
    
    return dO;
}

__DEVICE__ float3 GetNormal(float3 p, float iTime) {
  float d = GetDist(p, iTime);
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

__KERNEL__ void ScigglewFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse)
{

    float2 uv = (fragCoord-0.5f*iResolution)/iResolution.y;
    float2 m = swi2(iMouse,x,y)/iResolution;

    float3 ro = to_float3(0, 3, -3.5f);
    swi2S(ro,y,z, mul_f2_mat2(swi2(ro,y,z) , Rot(-m.y*3.14f+1.0f)));
    swi2S(ro,x,z, mul_f2_mat2(swi2(ro,x,z) , Rot(-m.x*6.2831f)));
    
    float3 rd = GetRayDir(uv, ro, to_float3(0,0.0f,0), 0.5f);
    float3 col = to_float3_s(0);
   
    float d = RayMarch(ro, rd, 1.0f, iTime);

    float IOR = 1.5f;
    if(d<MAX_DIST) {
        float3 p = ro + rd * d;
        float3 n = GetNormal(p,iTime);
        //vec3 r = reflect(rd, n);
        /*
        float3 rdIn = refract(rd, n, 1.0f/IOR);
        float3 pIn = p - 30.0f * SURF_DIST * n;
        float dIn = RayMarch(pIn, rdIn, -1.0f,iTime);

        float3 pExit = pIn + dIn * rdIn;
        float3 nExit = GetNormal(pExit,iTime);
        */
        float dif = dot(n, normalize(to_float3(1,2,3)))*0.5f+0.5f;
       // col = to_float3_s(dif);
        
        //float fresnel = 1.0f-_powf(1.0f+dot(rd, n), 1.0f);
       // col = 1.0f * to_float3(fresnel);
        
        col = _mix(to_float3_s(dif), 0.5f + 0.5f * n, _expf(-0.2f * length(p)));
        //p = distort(p,iTime);
       
        col *= 1.0f + 0.6f * thc(3.0f, 4.0f * n.y - 0.0f * iTime);
        col = clamp(col, 0.0f, 1.0f);
       // col *= 1.0f-_expf(-0.5f - 0.5f * p.y);
        
        float3 e = to_float3_s(1.0f);
        col *= pal(length(p) * 0.1f + -0.05f, e, e, e, 0.4f * to_float3(0,1,2)/3.0f);
        col = clamp(col, 0.0f, 1.0f);
        col *= 2.0f * _expf(-0.2f * length(p));
        col *= 0.8f + 0.2f * n.y;
       
        //col += dif;
    }
    
    col = pow_f3(col, to_float3_s(0.4545f));  // gamma correction
    
    fragColor = to_float4_aw(col,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}