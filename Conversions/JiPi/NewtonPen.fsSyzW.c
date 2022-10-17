
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: Uffizi Gallery_0' to iChannel0


#define MAX_STEPS 100
#define MAX_DIST 50.0f
#define SURF_DIST 0.005f

#define mat_support  1
#define mat_bar      2
#define mat_ball     3
#define mat_line     4


__DEVICE__ mat2 Rot(float a) {
    float s=_sinf(a), c=_cosf(a);
    return to_mat2(c, -s, s, c);
}

__DEVICE__ float SDFbox(float3 pos, float3 dim) {
    float3 p = abs_f3(pos) - dim;;
    return length(_fmaxf(p, to_float3_s(0.0f))) + _fminf(max(p.x, _fmaxf(p.y, p.z)), 0.0f);;
}

__DEVICE__ float SDFsphere(float3 pos, float r) {
    return length(pos) - r;
}

__DEVICE__ float SDF2box(float2 pos, float2 dim) {
    float2 p = abs_f2(pos) - dim;
    return length(_fmaxf(p, to_float2_s(0.0f))) + _fminf(max(p.x, p.y), 0.0f);;
}

__DEVICE__ float SDFring(float2 pos, float r) {
    return length(pos) - r;
}

__DEVICE__ float SDFseg(float3 pos, float3 a, float3 b) {
    float3 segDir = b - a;
    float t = _fmaxf(min(dot(pos - a, segDir)/dot(segDir, segDir), 1.0f), 0.0f);
    float3 q = a + t * (b - a);
    return length(pos - q);
}

__DEVICE__ float2 SDFball(float3 pos, float a) {
    
    pos.y -= 1.8f;
    swi2S(pos,y,x, mul_f2_mat2(swi2(pos,y,x) , Rot(a)));
    pos.y += 1.8f;
    
    float sphereDist = SDFsphere(pos - to_float3(0, 0.5f, 0), 0.2f);
    float ring = length(to_float2(SDFring(swi2(pos,y,x) - to_float2(0.7f, 0.0f), 0.03f), pos.z)) - 0.01f;
    
    float3 sim = pos;
    sim.z = _fabs(sim.z);
    float line = SDFseg(sim, to_float3(0, 0.7f, 0), to_float3(0.0f, 1.8f, 0.5f))-0.008f;
    
    sphereDist = _fminf(sphereDist, ring);
    
    float dist = _fminf(sphereDist, line); 
    
    return to_float2(dist, sphereDist == dist ? mat_ball : mat_line);
}

__DEVICE__ float2 mini(float2 a, float2 b) {
    return a.x < b.x ? a : b;
}

__DEVICE__ float2 GetDist(float3 pos, float iTime) {
    float4 sphere = to_float4(0, 1, 6, 1);
    
    float a = 0.78f * _cosf(iTime * 1.5f) * _fminf(10.0f / iTime, 1.0f);
    float aplus = _fminf(a, 0.0f);
    float amin = _fmaxf(a, 0.0f);
    
    float2 ball1 = SDFball(pos, 0.05f * a);
    float2 ball2 = SDFball(pos - to_float3(0.41f, 0.0f, 0), aplus * 0.02f + amin * 0.07f);
    float2 ball3 = SDFball(pos - to_float3(0.82f, 0.0f, 0), amin + 0.01f * aplus);
    float2 ball4 = SDFball(pos - to_float3(-0.41f, 0.0f, 0), aplus * 0.07f + amin * 0.02f);
    float2 ball5 = SDFball(pos - to_float3(-0.82f, 0.0f, 0), aplus + 0.01f * amin);
    
    float support = SDFbox(pos - to_float3(0, 0, 0), to_float3(1.5f, 0.1f, 0.7f)-0.05f) - 0.05f;
    support = _fmaxf(support, -pos.y);
    
    float bar = length(to_float2(SDF2box(swi2(pos,y,x), to_float2(1.8f, 1.3f)), _fabs(pos.z) - 0.5f)) - 0.05f;
    bar = _fmaxf(bar, -pos.y);
    
    float2 ball = mini(mini(mini(mini(ball1, ball2), ball3), ball4), ball5);
    
    float dist = _fminf(min(support, bar), ball.x);
    
    int mat = 0;
    
    if (dist == support) mat = mat_support;
    else if (dist == bar) mat = mat_bar;
    else if (dist == ball.x) mat = (int)(ball.y);
    
    return to_float2(dist, mat);
}

__DEVICE__ float2 RayMarch(float3 ro, float3 rd, float iTime) {
    

    float dO = 0.0f;
    float2 dS = to_float2_s(0.0f);
    for (int i = 0; i < MAX_STEPS; i++) {
      float3 pos = ro + rd * dO;
        dS = GetDist(pos,iTime);
        dO += dS.x;
        if(dO > MAX_DIST || dS.x < SURF_DIST) break;
    }
    
    return to_float2(dO, dS.y);
}

__DEVICE__ float3 GetNormal(float3 pos, float iTime) {
  float dist = GetDist(pos,iTime).x;
    float2 eps = to_float2(0.01f, 0);
    
    float3 norm = dist - to_float3(
        GetDist(pos - swi3(eps,x,y,y),iTime).x,
        GetDist(pos - swi3(eps,y,x,y),iTime).x,
        GetDist(pos - swi3(eps,y,y,x),iTime).x);
    
    return normalize(norm);
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

__DEVICE__ float3 Render(inout float3 *ro, inout float3 *rd, inout float3 *ref, bool last, float iTime, __TEXTURE2D__ iChannel0) {

   float3 col = swi3(decube_3f(iChannel0,(*rd).x,(*rd).y,(*rd).z),x,y,z);

   float2 d = RayMarch(*ro, *rd,iTime);

   if(d.x<MAX_DIST) {
      float3 p = *ro + *rd * d.x;
      float3 n = GetNormal(p,iTime);
      float3 r = reflect(*rd, n);
      
      float fresnel = _powf(1.0f - _fabs(dot(n, -*rd)), 5.0f);

      float dif = dot(n, normalize(to_float3(1,2,3)))*0.5f+0.5f;
       
      if ((int)(d.y) == mat_support) {
          *ref *= to_float3_s(_mix(0.01f, 0.5f, fresnel));
          col = to_float3_s(dif) * 0.1f;
      } else if ((int)(d.y) == mat_bar) {
          *ref *= to_float3_s(0.9f);
          col = to_float3_s(dif) * 0.1f;
      } else if ((int)(d.y) == mat_ball) {
          *ref *= to_float3(0.9f, 0.65f, 0.2f);
          col = to_float3_s(dif) * 0.1f;
      } else if ((int)(d.y) == mat_line) {
          *ref = to_float3_s(0.01f);
          col = to_float3_s(dif) * 0.1f;
      }
      
      if (last) col += swi3(decube_3f(iChannel0,r.x,r.y,r.z),x,y,z);
      
      *ro = p + 3.0f * n * SURF_DIST;
      *rd = r;
      
   } else *ref = to_float3_s(0.0f);
   
   return col;
}

__KERNEL__ void NewtonPenFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{

    float2 uv = (fragCoord-0.5f*iResolution)/iResolution.y;
    float2 m = swi2(iMouse,x,y)/iResolution;

    float3 ro = to_float3(0, 3, -3);
    swi2S(ro,y,z, mul_f2_mat2(swi2(ro,y,z) , Rot(-m.y*3.14f+1.0f)));
    swi2S(ro,x,z, mul_f2_mat2(swi2(ro,x,z) , Rot(-m.x*6.2831f)));
    
    float3 rd = GetRayDir(uv, ro, to_float3(0, 0.75f, 0), 1.6f);
    
    float3 ref = to_float3_s(1.0f);
    
    float3 col = Render(&ro, &rd, &ref, false, iTime,iChannel0);

    int NB_BOUNCE = 2;
    for (int i = 0; i < NB_BOUNCE; i++) {
        col += ref * Render(&ro, &rd, &ref, i + 1 == NB_BOUNCE, iTime,iChannel0);
    }
    
    col = pow_f3(col, to_float3_s(0.4545f));

    fragColor = to_float4_aw(col,1.0f);


  SetFragmentShaderComputedColor(fragColor);
}