
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


// Licence CC0: Liquid Metal
// Some experimenting with warped FBM and very very fake lighting turned out ok 
    
#define PI  3.141592654f
#define TAU (2.0f*PI)

__DEVICE__ float2 rot(float2 p, float a) {
  float c = _cosf(a);
  float s = _sinf(a);
  p = to_float2(c*p.x + s*p.y, -s*p.x + c*p.y);
  
  return p;
}

__DEVICE__ float hash(in float2 co) {
  return fract(_sinf(dot(swi2(co,x,y) ,to_float2(12.9898f,58.233f))) * 13758.5453f);
}

__DEVICE__ float2 hash2(float2 p) {
  p = to_float2(dot(p,to_float2(127.1f,311.7f)), dot(p,to_float2(269.5f,183.3f)));
  return fract_f2(sin_f2(p)*18.5453f);
}

__DEVICE__ float psin(float a) {
  return 0.5f + 0.5f*_sinf(a);
}

__DEVICE__ float onoise(float2 x) {
  x *= 0.5f;
  float a = _sinf(x.x);
  float b = _sinf(x.y);
  float c = _mix(a, b, psin(TAU*_tanhf(a*b+a+b)));
  
  return c;
}

__DEVICE__ float vnoise(float2 x) {
  float2 i = _floor(x);
  float2 w = fract_f2(x);
    
#if 1
  // quintic interpolation
  float2 u = w*w*w*(w*(w*6.0f-15.0f)+10.0f);
#else
  // cubic interpolation
  float2 u = w*w*(3.0f-2.0f*w);
#endif    

  float a = hash(i+to_float2(0.0f,0.0f));
  float b = hash(i+to_float2(1.0f,0.0f));
  float c = hash(i+to_float2(0.0f,1.0f));
  float d = hash(i+to_float2(1.0f,1.0f));
    
  float k0 =   a;
  float k1 =   b - a;
  float k2 =   c - a;
  float k3 =   d - c + a - b;

  float aa = _mix(a, b, u.x);
  float bb = _mix(c, d, u.x);
  float cc = _mix(aa, bb, u.y);
  
  return k0 + k1*u.x + k2*u.y + k3*u.x*u.y;
}

__DEVICE__ float fbm1(float2 p) {
  float2 op = p;
  const float aa = 0.45f;
  const float pp = 2.03f;
  const float2 oo = -1.0f*to_float2(1.23f, 1.5f);
  const float rr = 1.2f;
  
  float h = 0.0f;
  float d = 0.0f;
  float a = 1.0f;
  
  for (int i = 0; i < 5; ++i) {
    h += a*onoise(p);
    d += (a);
    a *= aa;
    p += oo;
    p *= pp;
    p = rot(p, rr);
  }
  
  return _mix((h/d), -0.5f*(h/d), _powf(vnoise(0.9f*op), 0.25f));
}

__DEVICE__ float fbm2(float2 p) {
  float2 op = p;
  const float aa = 0.45f;
  const float pp = 2.03f;
  const float2 oo = -1.0f*to_float2(1.23f, 1.5f);
  const float rr = 1.2f;
  
  float h = 0.0f;
  float d = 0.0f;
  float a = 1.0f;
  
  for (int i = 0; i < 7; ++i) {
    h += a*onoise(p);
    d += (a);
    a *= aa;
    p += oo;
    p *= pp;
    p = rot(p, rr);
  }
  
  return _mix((h/d), -0.5f*(h/d), _powf(vnoise(0.9f*op), 0.25f));
}

__DEVICE__ float fbm3(float2 p) {
  float2 op = p;
  const float aa = 0.45f;
  const float pp = 2.03f;
  const float2 oo = -1.0f*to_float2(1.23f, 1.5f);
  const float rr = 1.2f;
  
  float h = 0.0f;
  float d = 0.0f;
  float a = 1.0f;
    
  for (int i = 0; i < 3; ++i) {
    h += a*onoise(p);
    d += (a);
    a *= aa;
    p += oo;
    p *= pp;
    p = rot(p, rr);
  }
  
  return _mix((h/d), -0.5f*(h/d), _powf(vnoise(0.9f*op), 0.25f));
}


__DEVICE__ float warp(float2 p, float iTime, float4 iMouse) {

  float bump = 5.4f;
  float mouseX = iMouse.x*0.005f;
  float mouseY = iMouse.y*0.005f;
    
  float2 v = to_float2(fbm1(p), fbm1(p+0.5f*to_float2(1.0f, 1.0f)));
  
  v = rot(v, 1.0f+iTime*1.1f+mouseX);
  float2 vv = to_float2(fbm2(p + 3.7f*v), fbm2(p + -2.7f*swi2(v,y,x)+0.7f*to_float2(1.0f, 1.0f)));
  vv = rot(vv, -1.0f+iTime*1.21315f+mouseY);
  
  return fbm3(p + bump *vv);
}

__DEVICE__ float height(float2 p, float iTime, float4 iMouse, float hpar[5], bool Textur, float2 uv, __TEXTURE2D__ iChannel0) {
#ifdef ORG
  float a = 0.005f*iTime;
  p += 5.0f*to_float2(_cosf(a), _sinf(a));
  p *= 2.0f;
  p += 13.0f;
  float h = warp(p, iTime, iMouse);
  float rs = 3.0f;
  return 0.35f*_tanhf(rs*h)/rs;
#endif  

  float a = 0.005f*iTime;
  p += hpar[0]*to_float2(_cosf(a), _sinf(a));
  p *= hpar[1];
  p += hpar[2];
  float h = warp(p, iTime, iMouse);
  float rs = hpar[3];
  
  if(Textur)
  {
    float4 tex = _tex2DVecN(iChannel0, uv.x, uv.y,15);
    
    if (tex.w > 0.0f)
    {
      return hpar[4]*_tanhf(rs*h)/rs;
    }
    else
      return 0.0f;
  }
  
  return hpar[4]*_tanhf(rs*h)/rs;
  
}

__DEVICE__ float3 normal(float2 p, float iTime, float4 iMouse, float2 iResolution, float hpar[5], bool Textur, float2 uv, __TEXTURE2D__ iChannel0) {
  // As suggested by IQ, thanks!
  float2 eps = -1.0f*to_float2(2.0f/iResolution.y, 0.0f);
  
  float3 n;
  
  n.x = height(p + swi2(eps,x,y), iTime, iMouse, hpar, Textur, uv, iChannel0) - height(p - swi2(eps,x,y), iTime, iMouse, hpar, Textur, uv, iChannel0);
  n.y = 2.0f*eps.x;
  n.z = height(p + swi2(eps,y,x), iTime, iMouse, hpar, Textur, uv, iChannel0) - height(p - swi2(eps,y,x), iTime, iMouse, hpar, Textur, uv, iChannel0);
  
  return normalize(n);
}

__DEVICE__ float3 postProcess(float3 col, float2 q)  {
  col = pow_f3(clamp(col,0.0f,1.0f),to_float3_s(0.85f)); 
  col = col*0.6f+0.4f*col*col*(3.0f-2.0f*col);                       // contrast
  col = _mix(col, to_float3_s(dot(col, to_float3_s(0.1f))), -1.0f);  // satuation
  col *= 0.5f+0.5f*_powf(100.0f*q.x*q.y*(1.0f-q.x)*(1.0f-q.y),0.7f); // vigneting
  return col;
}

__KERNEL__ void HdLiquidJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{

  CONNECT_CHECKBOX0(Textur, 0);
  CONNECT_COLOR0(BaseColor, 0.6f, 0.6f, 0.8f, 1.0f);
  
  CONNECT_POINT0(LightPoint1XY, 0.0f, 0.0f );
  CONNECT_SLIDER0(LightPoint1Z, -10.0f, 10.0f, 0.0f);
  CONNECT_POINT1(LightPoint2XY, 0.0f, 0.0f );
  CONNECT_SLIDER1(LightPoint2Z, -10.0f, 10.0f, 0.0f);

  CONNECT_SLIDER2(Height1, -10.0f, 10.0f, 5.0f);
  CONNECT_SLIDER3(Height2, -10.0f, 10.0f, 2.0f);
  CONNECT_SLIDER4(Height3, -50.0f, 50.0f, 13.0f);
  CONNECT_SLIDER5(Height4, -10.0f, 10.0f, 3.0f);
  CONNECT_SLIDER6(Height5, -10.0f, 10.0f, 0.35f);

float hpar[5] = {Height1,Height2,Height3,Height4,Height5};

  float2 q = fragCoord/iResolution;
  float2 p = -1.0f + 2.0f * q;
  p.x*=iResolution.x/iResolution.y;
 
  const float3 lp1 = to_float3(0.9f, -0.5f, 0.8f) + to_float3(LightPoint1XY.x,LightPoint1Z,LightPoint1XY.y);
  const float3 lp2 = to_float3(-0.9f, -1.5f, 0.9f)+ to_float3(LightPoint2XY.x,LightPoint2Z,LightPoint2XY.y);

  float h = height(p,iTime,iMouse, hpar, Textur, q, iChannel0);
  float3 pp = to_float3(p.x, h, p.y);
  float ll1 = length(swi2(lp1,x,z) - swi2(pp,x,z));
  float3 ld1 = normalize(lp1 - pp);
  float3 ld2 = normalize(lp2 - pp);
 
  float3 n = normal(p,iTime,iMouse,iResolution, hpar, Textur, q, iChannel0);
  float diff1 = _fmaxf(dot(ld1, n), -0.1f);
  float diff2 = _fmaxf(dot(ld2, n), 0.0f);
 
  float3 baseCol = swi3(BaseColor,x,y,z)  + 0.2f;//to_float3(0.6f, 0.6f, 0.8f)+0.2f;

  float oh = height(p + ll1*0.05f*normalize(swi2(ld1,x,z)),iTime,iMouse, hpar, Textur, q, iChannel0);
  const float level0 = 0.0f;
  const float level1 = 0.125f;
  // VERY VERY fake shadows + hilight
  float3 scol = baseCol*(smoothstep(level0, level1, h) - smoothstep(level0, level1, oh));

  float3 col = to_float3_s(0.0f);
  col += baseCol*_powf(diff1, 9.0f);      //hilights

  col += 0.1f*baseCol*_powf(diff1, 0.1f); // greys

  col += 0.15f*swi3(baseCol,z,y,x)*_powf(diff2, 2.0f);
  
  col += 0.015f*swi3(baseCol,z,y,x)*_powf(diff2, 2.0f);
  
  col += scol*0.5f;

  col = postProcess(col, q);
  
  fragColor = to_float4_aw(col, 2.0f);

  SetFragmentShaderComputedColor(fragColor);
}