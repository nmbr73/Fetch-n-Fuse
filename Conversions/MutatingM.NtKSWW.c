
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------

__DEVICE__ float3 abs_f3(float3 a) {return (to_float3(_fabs(a.x), _fabs(a.y),_fabs(a.z)));}
#define lpowf _powf
__DEVICE__ float3 pow_f3(float3 a, float3 b) {float3 r; r.x = lpowf(a.x,b.x); r.y = lpowf(a.y,b.y); r.z = lpowf(a.z,b.z); return r;}


__DEVICE__ float de(float3 p, inout float4 *o, float iTime) {
  float4 q = to_float4_aw(p, 1);
  float4 c = q;

  *o = to_float4_s(10000.0f);
  
  q = to_float4_aw(swi3(q,x,y,z) * _fabs(_cosf(iTime*0.1f)),q.w);
  for(int i = 0; i < 10; i++) {
    q = to_float4_aw( 2.0f*clamp(swi3(q,x,y,z), -1.0f, 1.0f) - swi3(q,x,y,z), q.w);
    q *= clamp(1.0f/dot(swi3(q,x,y,z), swi3(q,x,y,z)), 1.0f, 1.0f/0.5f);
    
    q = 3.0f*q - c;

    *o = _fminf(*o, to_float4_aw(abs_f3(swi3(q,x,z,y)), length(swi3(q,x,y,z))));
  }

  return _fminf(length(p) - 1.0f - smoothstep(-2.0f, -1.97f, -length(p))*(length(swi3(q,x,y,z))/q.w - 0.001f), p.y + 1.0f);
}

__DEVICE__ float3 render(float3 ro, float3 rd, inout float3 *pos, inout float3 *ref, float iTime) {
  float t = 0.0f;
  float4 orb;
  for(int i = 0; i < 200; i++) {
    float d = de(ro + rd*t, &orb, iTime);
    if(d < 0.0001f*t || t >= 10.0f) break;
    t += d*0.35f;
  }

  float3 col = to_float3_s(0.15f);
  if(t < 10.0f) {
    float4 tmp;
    *pos = ro + rd*t;
    float2 eps = to_float2(0.001f, 0.0f);
    float3 nor = normalize(to_float3(
                    de(*pos + swi3(eps,x,y,y), &tmp, iTime) - de(*pos - swi3(eps,x,y,y), &tmp, iTime),
                    de(*pos + swi3(eps,y,x,y), &tmp, iTime) - de(*pos - swi3(eps,y,x,y), &tmp, iTime),
                    de(*pos + swi3(eps,y,y,x), &tmp, iTime) - de(*pos - swi3(eps,y,y,x), &tmp, iTime)
    ));
    *ref = reflect(rd, nor);
    float3 key = normalize(to_float3(0.8f, 0.7f, -0.6f));
    
    float occ = 0.0f, w = 1.0f, s = 0.006f;
    for(int i = 0; i < 15; i++) {
      float d = de(*pos + nor*s, &tmp, iTime);
      occ += (s - d)*w;
      w *= 0.95f;
      s += s/(float(i) + 1.0f);
    }
    occ = 1.0f - clamp(occ, 0.0f, 1.0f);
    
    float sha = 1.0f; s = 0.002f;
    for(int i = 0; i < 50; i++) {
      float d = de(*pos + key*s, &tmp, iTime);
      s += d;
      sha = _fminf(sha, 8.0f*d/s);
      if(d < 0.0f || t >= 10.0f) break;
    }
        sha = clamp(sha, 0.0f, 1.0f);
    
    col = clamp(dot(nor, key), 0.0f, 1.0f)
      *to_float3(1.64f, 1.57f, 0.99f)
      *sha;
    
    col += clamp(0.5f + 0.5f*nor.y, 0.0f, 1.0f)
      *to_float3(0.16f, 0.20f, 0.28f)
      *occ;
    
    col += clamp(dot(nor, key*to_float3(-1, 0, -1)), 0.0f, 1.0f)
      *to_float3(0.40f, 0.28f, 0.20f)
      *occ;

    float3 mat; 
          
    if((*pos).y > -0.99f) {
      mat = _mix(to_float3(1.0f, 0.3f, 0.0f), to_float3(0.3f, 1.0f, 0.3f), orb.x);
      mat = _mix(mat, to_float3(0.3f, 0.3f, 1.0f), orb.y);
      mat = _mix(mat, to_float3(1.1f, 0.8f, 0.1f), orb.z);
    }
    else mat = 0.5f + 0.5f*to_float3_s(mod_f(_floor((*pos).x) + _floor((*pos).z), 2.0f));

    col *= 0.2f*mat;
    col += _powf(clamp(dot(key, *ref), 0.0f, 1.0f), 8.0f)*to_float3(1.00f, 0.95f, 0.5f)*mat*occ;
  }
float zzzzzzzzzzzzzzzzzzzzz;  
  return col;
}
//************************************************************************************************
__KERNEL__ void MutatingMFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{

  float2 p = -1.0f + 2.0f*fragCoord/iResolution;
  p.x *= iResolution.x/iResolution.y;

  float3 ro = 1.75f*to_float3(_cosf(iTime*0.4f), 0.5f, -_sinf(iTime*0.4f));
  float3 ww = normalize(-ro);
  float3 uu = normalize(cross(to_float3(0, 1, 0), ww));
  float3 vv = normalize(cross(ww, uu));
  float3 rd = normalize(p.x*uu + p.y*vv + 1.97f*ww);

  float3 td, tp, rp, re;
  float3 col = render(ro, rd, &tp, &td, iTime);

  float3 rcol = render(tp, td, &rp, &re, iTime);

  col = _mix(col, rcol, 0.2f);
  
  col = pow_f3(col, to_float3_s(1.0f/2.2f));
  
  fragColor = to_float4_aw(col, 1);

  SetFragmentShaderComputedColor(fragColor);
}