
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

/*| tanh_f2       |*/ #define tanh_f2(i) to_float2(_tanhf(i.x), _tanhf((i).y))

#define pi 3.14159f

__DEVICE__ float thc(float a, float b) {
    return _tanhf(a * _cosf(b)) / _tanhf(a);
}

__DEVICE__ float ths(float a, float b) {
    return _tanhf(a * _sinf(b)) / _tanhf(a);
}

__DEVICE__ float2 thc(float a, float2 b) {
    return tanh_f2(a * cos_f2(b)) / _tanhf(a);
}

__DEVICE__ float2 ths(float a, float2 b) {
    return tanh_f2(a * sin_f2(b)) / _tanhf(a);
}

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

__DEVICE__ float mlength(float3 p) {
    return _fmaxf(max(_fabs(p.x), _fabs(p.y)), _fabs(p.z));
}


// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


#define MAX_STEPS 100
#define MAX_DIST 10.0f
#define SURF_DIST 0.001f

#define S smoothstep
#define T iTime

__DEVICE__ mat2 Rot(float a) {
    float s=_sinf(a), c=_cosf(a);
    return to_mat2(c, -s, s, c);
}

__DEVICE__ float2 GetDist(float3 p, float T) {

    float m = 0.8f;

    float3 id = to_float3_s(1);
    float time = 0.15f * iTime;
    float n = 3.0f;
    for (float i = 0.0f; i < n; i+=1.0f) {
       // id 
        id *= 0.5f;
        id += to_float3(step(p.x,0.0f), step(p.y,0.0f), step(p.z,0.0f));
        time += 2.0f * pi / n;
        p = abs_f3(p) - m;
        swi2S(p,x,y, mul_f2_mat2(swi2(p,x,y) , Rot(time - 2.0f * pi / 3.0f)));
        swi2S(p,y,z, mul_f2_mat2(swi2(p,y,z) , Rot(time)));
        swi2S(p,z,x, mul_f2_mat2(swi2(p,z,x) , Rot(time + 2.0f * pi / 3.0f)));
        //float s = 0.5f + 0.5f * _cosf(time + 4.0f * 0.0f);//step(0.0f, _cosf(time + 2.0f * p.x));
        //m *= 0.6f * (1.0f-s) + s * 0.4f;
               
        m *= 0.5f;
       
        //m *= _mix(0.25f, 0.5f, s);
        //m *= 0.6f + 0.4f * thc(8.0f, 0.2f * iTime + time);
    }
    
    float i = h21(swi2(id,x,y));
    float j = h21(swi2(id,y,z));
    float k = h21(to_float2(i, j)); // need better 3d noise
    float k2 = 0.375f + 0.125f * thc(8.0f, 2.0f * pi * k + 0.0f * mlength(p) - 0.5f * iTime);
    
    float d = mlength(p) - 2.0f * m * k2;

    //d = length(p) - 2.0f * m;
    return to_float2(0.8f * d, k);
}

__DEVICE__ float RayMarch(float3 ro, float3 rd, float T) {
  float dO=0.0f;
    
    for(int i=0; i<MAX_STEPS; i++) {
      float3 p = ro + rd*dO;
        float dS = GetDist(p,T).x;
        dO += dS;
        if(dO>MAX_DIST || _fabs(dS)<SURF_DIST) break;
    }
    
    return dO;
}

__DEVICE__ float3 GetNormal(float3 p, float T) {
  float d = GetDist(p, T).x;
    float2 e = to_float2(0.001f, 0);
    
    float3 n = d - to_float3(
        GetDist(p-swi3(e,x,y,y),T).x,
        GetDist(p-swi3(e,y,x,y),T).x,
        GetDist(p-swi3(e,y,y,x),T).x);
    
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

__DEVICE__ float GetLight(float3 p, float3 lightPos, float T) {
   // float3 lightPos = to_float3(0, 5, 6);
    //swi2(lightPos,x,z) += to_float2(_sinf(iTime), _cosf(iTime))*2.0f;
    float3 l = normalize(lightPos-p);
    float3 n = GetNormal(p,T);
    
    float dif = clamp(dot(n, l), 0.0f, 1.0f);
    float d = RayMarch(p+n*SURF_DIST*2.0f, l, T);
    dif = 0.15f + 0.85f * dif * step(length(lightPos-p), d);
   // if(d<length(lightPos-p)) dif = 0.0f;
    
    return dif;
}

__KERNEL__ void D3DFractalCubesBuggyFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse)
{

  float2 uv = (fragCoord-0.5f*iResolution)/iResolution.y;
  float2 m = swi2(iMouse,x,y)/iResolution;

    float3 ro = to_float3(0, 3, -3);
    swi2S(ro,y,z, mul_f2_mat2(swi2(ro,y,z) , Rot(-m.y*3.14f+1.0f)));
    swi2S(ro,x,z, mul_f2_mat2(swi2(ro,x,z) , Rot(-m.x*6.2831f)));
    
    float3 rd = GetRayDir(uv, ro, to_float3(0,0.0f,0), 1.4f);
    float3 col = to_float3_s(0);
   
    float d = RayMarch(ro, rd, T);

    if(d<MAX_DIST) {
        float3 p = ro + rd * d;
        float3 n = GetNormal(p,T);
        float3 r = reflect(rd, n);

        float dif2 = GetLight(p,to_float3(4.0f, 2.0f, 4.0f),T);
        col = to_float3_s(dif2);

        float dif = dot(n, normalize(to_float3(1,2,3)))*0.5f+0.5f;
        col *= to_float3_s(dif);
        
        // VERY sloppy way of doing it
        float k = GetDist(p,T).y;
        //col *= 0.5f + 0.5f * thc(4.0f, 2.0f * pi * k + iTime);
        float3 e = to_float3_s(1.0f);
        col *= pal(k + 0.5f * pi * thc(8.0f, 2.0f * pi * k + 0.1f * iTime), 
                   e, e, e, 0.5f * to_float3(0.0f,0.33f,0.66f));
        //col *= 0.52f + 0.48f * _cosf(4.0f * length(p) - iTime);
    }
    
    col = pow_f3(col, to_float3_s(0.4545f));  // gamma correction
    
    fragColor = to_float4_aw(col,1.0f);


  SetFragmentShaderComputedColor(fragColor);
}