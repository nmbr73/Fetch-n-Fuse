
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Organic 2' to iChannel0
// Connect Image 'Cubemap: Forest_0' to iChannel1

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

__DEVICE__ float truc(float3 p) {
  float3 t;
  t.x = _cosf(p.x) + _sinf(p.y);
  t.y = _cosf(p.y) - _sinf(p.z);
  t.z = _cosf(p.z) + _sinf(p.x);
  return length(t) - 0.8f;
}

__DEVICE__ float displace(float3 p, __TEXTURE2D__ iChannel0) {
  p = swi3(texture(iChannel0, (swi2(p,x,z)) / 20.0f),x,y,z) / 20.0f;
  return p.x + p.y + p.z;
}

__DEVICE__ float map(float3 p, __TEXTURE2D__ iChannel0) {
  float d = 100.0f;
  d = truc(p);
  d += displace(p,iChannel0);
  return d;
}

__DEVICE__ float intersect(float3 ro, float3 rd, __TEXTURE2D__ iChannel0) {
  float t = 0.0f;
  for (int i = 0; i < 50; i++) {
  float d = map(ro + rd * t,iChannel0);
  if (d <= 0.01f) return t;
  t += d;
  }
  return 0.0f;
}

__DEVICE__ float3 normal(float3 p, __TEXTURE2D__ iChannel0) {
  float eps = 0.1f;
  return normalize(to_float3(
    map(p + to_float3(eps, 0, 0),iChannel0) - map(p - to_float3(eps, 0, 0),iChannel0),
    map(p + to_float3(0, eps, 0),iChannel0) - map(p - to_float3(0, eps, 0),iChannel0),
    map(p + to_float3(0, 0, eps),iChannel0) - map(p - to_float3(0, 0, eps),iChannel0)
  ));
}

__DEVICE__ float occ(float3 p, __TEXTURE2D__ iChannel0) {
  float ao = 0.0f;
  float eps = 0.5f;
  ao += map(p + to_float3(eps, eps, eps),iChannel0);
  ao += map(p + to_float3(eps, eps, -eps),iChannel0);
  ao += map(p + to_float3(eps,-eps, eps),iChannel0);
  ao += map(p + to_float3(eps, -eps, -eps),iChannel0);
  ao += map(p + to_float3(-eps, eps, eps),iChannel0);
  ao += map(p + to_float3(-eps, eps, -eps),iChannel0);
  ao += map(p + to_float3(-eps, -eps, eps),iChannel0);
  ao += map(p + to_float3(-eps, -eps, -eps),iChannel0);
  return ao / (8.0f * eps);
}


__KERNEL__ void Trig1Jipi478Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{

  CONNECT_CHECKBOX0(TimeInv, 0);

  float2 uv = fragCoord / iResolution - 0.5f;
  uv.x = uv.x * iResolution.x / iResolution.y;
  
  
  
  
  float t = 10.0f + iTime * 0.2f;
  if (TimeInv) t = 10.0f - iTime * 0.2f;
  
  float ty = 0.0, tx =0.0f;
  
  if (iMouse.z > 0.0f) {
    tx = iMouse.x / iResolution.x * 3.1416f * 0.50f;// + 3.14f;
    ty = iMouse.y / iResolution.y * 3.1416f * 0.50f;//- 0.1f;
  }

  //float3 ro = to_float3(_cosf(tx * 0.7f), ty, _sinf(tx * 0.7f));
  
  
  
  float cr = 32.0f;
  
  float3 ro = to_float3(_cosf(t+tx) * cr, ty, _sinf(t) * cr);
  float3 ta = to_float3(_cosf(t - 0.1f) * cr, 0.0f, _sinf(t - 0.1f) * cr);
  float3 ww = normalize(ta - ro);
  float3 vv = to_float3(0,1,0);
  float3 uu = normalize(cross(ww, vv));
  float3 l1 = normalize(to_float3(1, 1, 1));
  float3 l2 = normalize(to_float3(-1, -1, 1));
  
  float3 fcolor = to_float3_s(0.0f);
  for (int i = 0; i < 4; i++) {
  
    // Reinder dof ! thx
    
    const float fov = 1.0f;
    float3 er = normalize(to_float3_aw(swi2(uv,x,y), fov));
    float3 rd = er.x * uu + er.y * vv + er.z * ww;
    
    float rnd = fract(_sinf(iTime * 20322.1232f)) / 13211.123f;
    float3 go = 0.01f * to_float3_aw((rnd - to_float2_s(0.3f)) * 2.0f, 0.0f);
    float3 gd = normalize( er*4.0f - go );
    
    ro += go.x * uu + go.y * vv;
    rd += gd.x * uu + gd.y * vv;
    rd = normalize(rd);
    
    float d = intersect(ro, rd,iChannel0);
    float3 color = to_float3_s(0);
    
    if (d > 0.0f) {
      float3 pi = ro + rd * d;
      float3 ni = normal(pi,iChannel0);
      float dif = (dot(ni, l1) + dot(ni, l2)) / 2.0f;
      float spec1 = _powf(_fmaxf(0.5f, dot(reflect(l1, ni), rd)), 20.0f);
      float spec2 = _powf(_fmaxf(0.5f, dot(reflect(l2, ni), rd)), 20.0f);
      float3 peps = ro + rd * (d - 0.3f);
      float3 rn = reflect(rd, ni);
      float3 rf = swi3(_tex2DVecN(iChannel1,rn.x,rn.y,15),x,y,z);
      float ao = occ(pi,iChannel0);
      color = to_float3(1, 0, 0) * dif * ao + spec1 + spec2 + rf * 0.8f;
    }
    color *= _fminf(1.0f, 9.0f/d);
    fcolor += color - _powf(length(swi2(uv,x,y)), 6.0f) * 2.0f;
  }
  fragColor = to_float4_aw(fcolor / 4.0f,1.0f);


  SetFragmentShaderComputedColor(fragColor);
}