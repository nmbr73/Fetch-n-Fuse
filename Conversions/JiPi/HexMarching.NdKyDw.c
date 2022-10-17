
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// License CC0: Hex Marching
#define RESOLUTION  iResolution
#define TIME        iTime
#define PI          3.141592654f
#define TAU         (2.0f*PI)
#define ROT(a)      to_mat2(_cosf(a), _sinf(a), -_sinf(a), _cosf(a))
#define BPM         30.0f



// License: WTFPL, author: sam hocevar, found: https://stackoverflow.com/a/17897228/418488


__DEVICE__ float3 hsv2rgb(float3 c) {

  const float4 hsv2rgb_K = to_float4(1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);	
  	
  float3 p = abs_f3(fract_f3(swi3(c,x,x,x) + swi3(hsv2rgb_K,x,y,z)) * 6.0f - swi3(hsv2rgb_K,w,w,w));
  return c.z * _mix(swi3(hsv2rgb_K,x,x,x), clamp(p - swi3(hsv2rgb_K,x,x,x), 0.0f, 1.0f), c.y);
}
// License: WTFPL, author: sam hocevar, found: https://stackoverflow.com/a/17897228/418488
//  Macro version of above to enable compile-time constants
#define HSV2RGB(c)  (c.z * _mix(swi3(hsv2rgb_K,x,x,x), clamp(_fabs(fract(swi3(c,x,x,x) + swi3(hsv2rgb_K,x,y,z)) * 6.0f - swi3(hsv2rgb_K,w,w,w)) - swi3(hsv2rgb_K,x,x,x), 0.0f, 1.0f), c.y))


// License: Unknown, author: Unknown, found: don't remember
__DEVICE__ float4 alphaBlend_f4(float4 back, float4 front) {
  float w = front.w + back.w*(1.0f-front.w);
  float3 xyz = (swi3(front,x,y,z)*front.w + swi3(back,x,y,z)*back.w*(1.0f-front.w))/w;
  return w > 0.0f ? to_float4_aw(xyz, w) : to_float4_s(0.0f);
}

// License: Unknown, author: Unknown, found: don't remember
__DEVICE__ float3 alphaBlend(float3 back, float4 front) {
  return _mix(back, swi3(front,x,y,z), front.w);
}

// License: Unknown, author: Unknown, found: don't remember
__DEVICE__ float tanh_approx(float x) {
  //  Found this somewhere on the interwebs
  //  return _tanhf(x);
  float x2 = x*x;
  return clamp(x*(27.0f + x2)/(27.0f+9.0f*x2), -1.0f, 1.0f);
}

// License: Unknown, author: Unknown, found: don't remember
__DEVICE__ float hash(float co) {
  return fract(_sinf(co*12.9898f) * 13758.5453f);
}

__DEVICE__ float3 offset(float z) {
  float a = z;
  float2 p = -0.15f*(to_float2(_cosf(a), _sinf(a*_sqrtf(2.0f))) + to_float2(_cosf(a*_sqrtf(0.75f)), _sinf(a*_sqrtf(0.5f))));
  return to_float3_aw(p, z);
}

__DEVICE__ float3 doffset(float z) {
  float eps = 0.05f;
  return 0.5f*(offset(z + eps) - offset(z - eps))/(2.0f*eps);
}

__DEVICE__ float3 ddoffset(float z) {
  float eps = 0.05f;
  return 0.5f*(doffset(z + eps) - doffset(z - eps))/(2.0f*eps);
}

// License: CC0, author: Mårten Rånge, found: https://github.com/mrange/glsl-snippets
__DEVICE__ float2 toPolar(float2 p) {
  return to_float2(length(p), _atan2f(p.y, p.x));
}

// License: CC0, author: Mårten Rånge, found: https://github.com/mrange/glsl-snippets
__DEVICE__ float2 toRect(float2 p) {
  return to_float2(p.x*_cosf(p.y), p.x*_sinf(p.y));
}

// License: MIT OR CC-BY-NC-4.0f, author: mercury, found: https://mercury.sexy/hg_sdf/
__DEVICE__ float mod1(inout float *p, float size) {
  float halfsize = size*0.5f;
  float c = _floor((*p + halfsize)/size);
  *p = mod_f(*p + halfsize, size) - halfsize;
  return c;
}

__DEVICE__ float2 hextile(inout float2 *p) {
  // See Art of Code: Hexagonal Tiling Explained!
  // https://www.youtube.com/watch?v=VmrIDyYiJBA
  const float2 sz       = to_float2(1.0f, _sqrtf(3.0f));
  const float2 hsz      = 0.5f*sz;

  float2 p1 = mod_f2f2(*p, sz)-hsz;
  float2 p2 = mod_f2f2(*p - hsz, sz)-hsz;
  float2 p3 = dot(p1, p1) < dot(p2, p2) ? p1 : p2;
  float2 n = ((p3 - *p + hsz)/sz);
  *p = p3;

  n -= to_float2_s(0.5f);
  // Rounding to make hextile 0,0 well behaved
  return round(n*2.0f)*0.5f;
}

__DEVICE__ float4 effect(float2 p, float aa, float h, float iTime) {
	
  const float4 hsv2rgb_K = to_float4(1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);	
	
  float2 hhn = hextile(&p);
  const float w = 0.02f;
  float2 pp = toPolar(p);
  float a = pp.y;
  
  float par = pp.y;
  float hn = mod1(&par, TAU/6.0f);
  pp.y = par;
  
  float2 hp = toRect(pp);
  float hd = hp.x-(w*10.0f);
  
  float x = hp.x-0.5f*w;
  float n = mod1(&x, w);
  float d = _fabs(x)-(0.5f*w-aa);
  
  float h0 = hash(10.0f*(hhn.x+hhn.y)+2.0f*h+n);
  float h1 = fract(8667.0f*h0);
  float cut = _mix(-0.5f, 0.999f, 0.5f+0.5f*_sinf(TIME+TAU*h0));
  const float coln = 6.0f;
  float t = smoothstep(aa, -aa, d)*smoothstep(cut, cut-0.005f, _sinf(a+2.0f*(h1-0.5f)*TIME+h1*TAU))*_expf(-150.0f*_fabs(x));
  float3 col = hsv2rgb(to_float3(_floor(h0*coln)/coln, 0.8f, 1.0f))*t*1.75f;

  t = _mix(0.9f, 1.0f, t);
  t *= smoothstep(aa, -aa, -hd);
  if (hd < 0.0f) {
    col = to_float3_s(0.0f);
    t = 15.0f*dot(p, p);
  }
  return to_float4_aw(col, t);
}

__DEVICE__ float4 plane(float3 ro, float3 rd, float3 pp, float3 npp, float3 off, float n, float iTime) {
  float h0 = hash(n);
  float h1 = fract(8667.0f*h0);

  float3 hn;
  float2 p  = swi2((pp-off*to_float3(1.0f, 1.0f, 0.0f)),x,y);
  p = mul_f2_mat2(p,ROT(TAU*h0));
  p.x -= 0.25f*h1*(pp.z-ro.z);
  const float z = 1.0f;
  p /= z;
  float aa = distance_f3(pp,npp)*_sqrtf(1.0f/3.0f)/z;
  float4 col = effect(p, aa, h1, iTime);

  return col;
}

__DEVICE__ float3 skyColor(float3 ro, float3 rd) {
  return to_float3_s(0.0f);
}

__DEVICE__ float3 color(float3 ww, float3 uu, float3 vv, float3 ro, float2 p, float planeDist, float iTime, float2 iResolution) {
  float lp = length(p);
  float2 np = p + 2.0f/RESOLUTION.y;
  float rdd = (2.0f-0.5f*tanh_approx(lp));  // Playing around with rdd can give interesting distortions
//  float rdd = 2.0f;
  
  float3 rd = normalize(p.x*uu + p.y*vv + rdd*ww);
  float3 nrd = normalize(np.x*uu + np.y*vv + rdd*ww);

  const int furthest = 5;
  const int fadeFrom = _fmaxf(furthest-2, 0);

  const float fadeDist = planeDist*(float)(furthest - fadeFrom);
  float nz = _floor(ro.z / planeDist);

  float3 skyCol = skyColor(ro, rd);


  float4 acol = to_float4_s(0.0f);
  const float cutOff = 0.95f;
  bool cutOut = false;

  float maxpd = 0.0f;

  // Steps from nearest to furthest plane and accumulates the color 
  for (int i = 1; i <= furthest; ++i) {
    float pz = planeDist*nz + planeDist*(float)(i);

    float pd = (pz - ro.z)/rd.z;

    if (pd > 0.0f && acol.w < cutOff) {
      float3 pp = ro + rd*pd;
      maxpd = pd;
      float3 npp = ro + nrd*pd;

      float3 off = offset(pp.z);

      float4 pcol = plane(ro, rd, pp, npp, off, nz+(float)(i), iTime);

      float nz = pp.z-ro.z;
      float fadeIn = smoothstep(planeDist*(float)(furthest), planeDist*(float)(fadeFrom), nz);
      float fadeOut = smoothstep(0.0f, planeDist*0.1f, nz);
//      swi3(pcol,x,y,z) = _mix(skyCol, swi3(pcol,x,y,z), fadeIn);
      pcol.w *= fadeOut*fadeIn;
      pcol = clamp(pcol, 0.0f, 1.0f);

      acol = alphaBlend_f4(pcol, acol);
    } else {
      cutOut = true;
      acol.w = acol.w > cutOff ? 1.0f : acol.w;
      break;
    }

  }

  float3 col = alphaBlend(skyCol, acol);
// To debug cutouts due to transparency  
//  col += cutOut ? to_float3(1.0f, -1.0f, 0.0f) : to_float3_s(0.0f);
  return col;
}

__DEVICE__ float3 effectA(float2 p, float2 q, float planeDist, float iTime, float2 iResolution) {
  float tm  = planeDist*TIME*BPM/60.0f;
  float3 ro   = offset(tm);
  float3 dro  = doffset(tm);
  float3 ddro = ddoffset(tm);

  float3 ww = normalize(dro);
  float3 uu = normalize(cross(normalize(to_float3(0.0f,1.0f,0.0f)+ddro), ww));
  float3 vv = cross(ww, uu);

  float3 col = color(ww, uu, vv, ro, p, planeDist, iTime, iResolution);
  
  return col;
}

__KERNEL__ void HexMarchingFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{
  CONNECT_SLIDER0(planeDist, -10.0f, 10.0f, 0.8f);
  fragCoord+=0.5f;

  //const float planeDist = 1.0f-0.2f;

  float2 q = fragCoord/iResolution;
  float2 p = -1.0f + 2.0f * q;
  p.x *= RESOLUTION.x/RESOLUTION.y;

  float3 col = effectA(p, q, planeDist, iTime, iResolution);
  
  fragColor = to_float4_aw(col, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1



//  simplyfied version of Dave Hoskins blur
__DEVICE__ float3 dblur(float2 q,float rad, float2 iResolution, __TEXTURE2D__ iChannel1) {
  float3 acc=to_float3_s(0);
  const float m = 0.002f;
  float2 pixel=to_float2(m*RESOLUTION.y/RESOLUTION.x,m);
  float2 angle=to_float2(0,rad);
  rad=1.0f;
  const int iter = 30;
  for (int j=0; j<iter; ++j) {  
    rad += 1.0f/rad;
    angle = mul_f2_mat2(angle , ROT(2.399f));
    float4 col=texture(iChannel1,q+pixel*(rad-1.0f)*angle);
    acc+=swi3(col,x,y,z);
  }
  return acc*(1.0f/(float)(iter));
}

__KERNEL__ void HexMarchingFuse__Buffer_B(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

  fragCoord+=0.5f;

  float2 q = fragCoord/iResolution;
  float2 p = -1.0f+2.0f*q;
  float4 pcol = _tex2DVecN(iChannel0,q.x,q.y,15);
  float3 bcol = dblur(q, 0.75f, iResolution, iChannel1);
  
  float3 col = swi3(pcol,x,y,z);
  col += to_float3(0.9f, 0.8f, 1.2f)*_mix(0.5f, 0.66f, length(p))*(0.05f+bcol);
  
  fragColor = to_float4_aw(col, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer B' to iChannel0


// License CC0: Hex Marching
//  Results from saturday afternoon tinkering
#define TIME iTime
__KERNEL__ void HexMarchingFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{

  float2 q = fragCoord/iResolution;

  float4 pcol = _tex2DVecN(iChannel0,q.x,q.y,15);
  float3 col = swi3(pcol,x,y,z);
  col = clamp(col, 0.0f, 1.0f);
  col *= smoothstep(0.0f, 2.0f, TIME);
  col = sqrt_f3(col);
  fragColor = to_float4_aw(col, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}