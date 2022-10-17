
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


//cambios por jorge flores p.---->jorge2017a2
//21-feb-2022
///referencia y fork
//https://www.shadertoy.com/view/MtlfRs.....by zackpudil in 2017-12-11

__DEVICE__ float hash(float2 n) {
  return fract(dot(to_float2(_sinf(n.x*2343.34f), _cosf(n.y*30934.0f)), to_float2(_sinf(n.y*309392.34f), _cosf(n.x*3991.0f))));
}

// Minkowski operators, can be seen at http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.90.803&rep=rep1&type=pdf
// It's a paper on generalized distance functions, but it intros on minkowski operators (generalizing them).
__DEVICE__ float len(float3 p, float l) {
  p = pow_f3(abs_f3(p), to_float3_s(l));
  return _powf(p.x + p.y + p.z, 1.0f/l);
}

__DEVICE__ float2 GetDist(float3 p  )
{
  float3 q = p;
  float2 c = _floor((swi2(p,x,z) + 3.0f)/6.0f);
  
  swi2S(q,x,z, mod_f2(swi2(q,x,z) + 3.0f, 6.0f) - 3.0f);
  q.y -= 0.5f;
    // use random value to produce different shape.
    //return to_float2(_fminf(len(q, 1.5f + 9.0f*hash(c)) - 1.5f, p.y + 1.0f),1.0f);
    float d1=len(q, 1.5f + 9.0f*hash(c)) - 1.5f;
    float d2=p.y + 1.0f;
  
    return to_float2(_fminf(d1, d2),1.0f);
}

// basic trace, with some LOD
__DEVICE__ float RayMarch(float3 o, float3 d, float m) {
  
  float t = 0.0f;
  for(int i = 0; i < 200; i++) {
    float d = GetDist(o + d*t).x;
    if(d < (0.001f + 0.0001f*t) || t >= m) break;
        t += d*0.67f;
  }
  return t;
}

// basic normal.
__DEVICE__ float3 normal(float3 p) {
  float2 h = to_float2(0.001f, 0.0f);
  float3 n = to_float3(
    GetDist(p + swi3(h,x,y,y)).x - GetDist(p - swi3(h,x,y,y)).x,
    GetDist(p + swi3(h,y,x,y)).x - GetDist(p - swi3(h,y,x,y)).x,
    GetDist(p + swi3(h,y,y,x)).x - GetDist(p - swi3(h,y,y,x)).x
  );
  return normalize(n);
}

__DEVICE__ float getSoftShadow(float3 p, float3 lightPos)
{   float res = 1.0f;
    float dist = 0.01f;
    float lightSize = 0.03f;
    for (int i = 0; i < 15; i++) {
        float hit = GetDist(p + lightPos * dist).x;
        res = _fminf(res, hit / (dist * lightSize));
        dist += hit;
        if (hit < 0.0001f || dist > 60.0f) break;
    }
    return clamp(res, 0.0f, 1.0f);
}

__DEVICE__ float ambOcclusion(float3 pos, float3 nor)
{   float sca = 2.0f, occ = 0.0f;
    for(int i = 0; i < 10; i++) {
        float hr = 0.01f + (float)(i) * 0.5f / 4.0f;
        float dd = GetDist(nor * hr + pos).x;
        occ += (hr - dd)*sca;
        sca *= 0.6f;
    }
    return clamp( 1.0f - occ, 0.0f, 1.0f );
}

//euclidean cameras.
__DEVICE__ mat3 camera(float3 o, float3 l) 
{  float3 w = normalize(l - o);
  float3 u = normalize(cross(to_float3(0, 1, 0), w));
  float3 v = normalize(cross(w, u));  
  return to_mat3_f3(u, v, w);
}

__DEVICE__ float3 iluminacion(float3 pos,float3 rd,float3 nor, float3 ref, float3 lig)
{
    float3 rcol =to_float3_s(0.0f);    
    // occlusion and shadows
    float occ = ambOcclusion(pos, nor);
  //float sha = step(5.0f, trace(pos + nor*0.001f, lig, 5.0f));
    float sha = getSoftShadow(pos, normalize(lig));
      
    // lighting ambient + diffuse + fresnel + specular
  rcol += 0.2f*occ;
  rcol += clamp(dot(lig, nor), 0.0f, 1.0f)*occ*sha;
  rcol += _powf(clamp(1.0f + dot(rd, nor), 0.0f, 1.0f), 2.0f)*occ;
  rcol += 2.0f*_powf(clamp(dot(ref, lig), 0.0f, 1.0f), 30.0f)*occ;
      
    // simple material.
  if(pos.y > -0.99f)
        rcol *= to_float3(1.2f, 0.7f, 0.7f);
        //rcol *= to_float3(0.92f, 0.27f, 0.57f);
  else
    {
        rcol *= 0.2f + 0.5f*mod_f(_floor(pos.x) + _floor(pos.z), 2.0f);
        //rcol*=to_float3_s(0.5f);
     }
        
  return rcol;    
}        

__DEVICE__ float3 render(float3 ro, float3 rd) 
{
    float3 col = to_float3(0.45f, 0.8f, 1.0f);
    float3 lig = normalize(to_float3(0.8f, 0.7f, -0.6f));
    float t;
    
    for(int i = 0; i < 3; i++) {
     t = RayMarch(ro, rd, 50.0f);
     if(t < 50.0f) 
      {
      float3 rcol = to_float3_s(0);
            // geometry, hit position, normal, reflect
      float3 pos = ro + rd*t;
      float3 nor = normal(pos);
      float3 ref = reflect(rd, nor);
             rcol=iluminacion(pos,rd, nor, ref, lig);
            // set up the ray orgin and direction for reflection.
      ro = pos + nor*0.001f;
      rd = ref;
            // sky fog.
      rcol = _mix(rcol, to_float3(0.45f, 0.8f, 1.0f), 1.0f - _expf(-0.00715f*t));
            // lighten intensity on each successive reflect.
      if(i == 0)
               col = rcol;
      else
               col *= _mix(rcol, to_float3_s(1), 1.0f - _expf(-0.8f*(float)(i)));
    }
  }
    return col;
}


__DEVICE__ float3 linear2srgb(float3 c) 
{ return mix_f3(12.92f * c,1.055f * pow_f3(c, to_float3_s(1.0f/1.8f)) - 0.055f, step(to_float3_s(0.0031308f), c)); }

__DEVICE__ float3 exposureToneMapping(float exposure, float3 hdrColor) 
{ return to_float3_s(1.0f) - exp_f3(-hdrColor * exposure); }


__DEVICE__ float3 ACESFilm(float3 x)
{   float a,b,c,d,e;
    a = 2.51f; b = 0.03f; c = 2.43f; 
    d = 0.59f; e = 0.14f;
    return (x*(a*x+b))/(x*(c*x+d)+e);
}

__KERNEL__ void ReflectionChangesJipi360Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{

  float2 p = (-iResolution + 2.0f*fragCoord)/iResolution.y;
  
  float3 ro = to_float3(2.7f + _cosf(iTime*0.3f), 0, iTime);
  float3 rd = mul_mat3_f3(camera(ro, to_float3(2.5f + 0.9f*_cosf(iTime*0.3f + 0.3f), 0, iTime + 0.2f)),normalize(to_float3_aw(p, 1.97f)));
  
  float3 col= render(ro, rd);
        
  // tone mapping and gamma correction.
  //col = 1.0f - _expf(-0.5f*col);
  //col = _powf(_fabs(col), to_float3_aw(1.0f/2.2f));
  col= (ACESFilm(col)+linear2srgb(col)+col+ exposureToneMapping(3.0f, col))/4.0f ;
  fragColor = to_float4_aw(col, 1);


  SetFragmentShaderComputedColor(fragColor);
}