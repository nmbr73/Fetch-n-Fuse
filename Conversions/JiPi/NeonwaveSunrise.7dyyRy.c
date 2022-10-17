
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'https://soundcloud.com/didlybom/vangelis-memories-of-green' to iChannel0


// CC0 - Neonwave sunrise
//  Inspired by a tweet by I wanted to create something that looked
//  a bit like the tweet. This is the result.

#define RESOLUTION    iResolution
#define TIME          iTime
#define PI            3.141592654f
#define TAU           (2.0f*PI)

// License: WTFPL, author: sam hocevar, found: https://stackoverflow.com/a/17897228/418488

__DEVICE__ float3 hsv2rgb(float3 c, float4 hsv2rgb_K) {
  
  float3 p = abs_f3(fract_f3(swi3(c,x,x,x) + swi3(hsv2rgb_K,x,y,z)) * 6.0f - swi3(hsv2rgb_K,w,w,w));
  return c.z * _mix(swi3(hsv2rgb_K,x,x,x), clamp(p - swi3(hsv2rgb_K,x,x,x), 0.0f, 1.0f), c.y);
}
// License: WTFPL, author: sam hocevar, found: https://stackoverflow.com/a/17897228/418488
//  Macro version of above to enable compile-time constants
#define HSV2RGB(c)  (c.z * _mix(swi3(hsv2rgb_K,x,x,x), clamp(abs_f3(fract_f3(swi3(c,x,x,x) + swi3(hsv2rgb_K,x,y,z)) * 6.0f - swi3(hsv2rgb_K,w,w,w)) - swi3(hsv2rgb_K,x,x,x), 0.0f, 1.0f), c.y))

// License: Unknown, author: Unknown, found: don't remember
__DEVICE__ float4 alphaBlend(float4 back, float4 front) {
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

// License: Unknown, author: Unknown, found: don't remember
__DEVICE__ float hash(float2 p) {
  float a = dot (p, to_float2 (127.1f, 311.7f));
  return fract(_sinf(a)*43758.5453123f);
}

// Value noise: https://iquilezles.org/articles/morenoise
__DEVICE__ float vnoise(float2 p) {
  float2 i = _floor(p);
  float2 f = fract_f2(p);
    
  float2 u = f*f*(3.0f-2.0f*f);
//  float2 u = f;

  float a = hash(i + to_float2(0.0f,0.0f));
  float b = hash(i + to_float2(1.0f,0.0f));
  float c = hash(i + to_float2(0.0f,1.0f));
  float d = hash(i + to_float2(1.0f,1.0f));

  float m0 = _mix(a, b, u.x);
  float m1 = _mix(c, d, u.x);
  float m2 = _mix(m0, m1, u.y);
  
  return m2;
}

// License: MIT, author: Inigo Quilez, found: https://iquilezles.org/www/articles/spherefunctions/spherefunctions.htm
__DEVICE__ float2 raySphere(float3 ro, float3 rd, float4 sph) {
  float3 oc = ro - swi3(sph,x,y,z);
  float b = dot( oc, rd );
  float c = dot( oc, oc ) - sph.w*sph.w;
  float h = b*b - c;
  if( h<0.0f ) return to_float2_s(-1.0f);
  h = _sqrtf( h );
  return to_float2(-b - h, -b + h);
}

// License: MIT OR CC-BY-NC-4.0f, author: mercury, found: https://mercury.sexy/hg_sdf/
__DEVICE__ float2 mod2(inout float2 *p, float2 size) {
  float2 c = _floor((*p + size*0.5f)/size);
  *p = mod_f2f2(*p + size*0.5f,size) - size*0.5f;
  return c;
}

// License: Unknown, author: Unknown, found: don't remember
__DEVICE__ float2 hash2(float2 p) {
  p = to_float2(dot (p, to_float2 (127.1f, 311.7f)), dot (p, to_float2 (269.5f, 183.3f)));
  return fract_f2(sin_f2(p)*43758.5453123f);
}

__DEVICE__ float hifbm(float2 p) {
  const float aa = 0.5f;
  const float pp = 2.0f-0.0f;

  float sum = 0.0f;
  float a   = 1.0f;
  
  for (int i = 0; i < 5; ++i) {
    sum += a*vnoise(p);
    a *= aa;
    p *= pp;
  }
  
  return sum;
}

__DEVICE__ float lofbm(float2 p) {
  const float aa = 0.5f;
  const float pp = 2.0f-0.0f;

  float sum = 0.0f;
  float a   = 1.0f;
  
  for (int i = 0; i < 2; ++i) {
    sum += a*vnoise(p);
    a *= aa;
    p *= pp;
  }
  
  return sum;
}

__DEVICE__ float hiheight(float2 p) {
  return hifbm(p)-1.8f;
}

__DEVICE__ float loheight(float2 p) {
  return lofbm(p)-2.15f;
}

__DEVICE__ float4 plane(float3 ro, float3 rd, float3 pp, float3 npp, float3 off, float n, float4 hsv2rgb_K) {
  float h = hash(n);
  float s = _mix(0.05f, 0.25f, h);

  float3 hn;
  float2 p = swi2((pp-off*2.0f*to_float3(1.0f, 1.0f, 0.0f)),x,y);

  const float2 stp = to_float2(0.5f, 0.33f); 
  float he    = hiheight(to_float2(p.x, pp.z)*stp);
  float lohe  = loheight(to_float2(p.x, pp.z)*stp);

  float d = p.y-he;
  float lod = p.y - lohe;

  float aa = distance_f3(pp, npp)*_sqrtf(1.0f/3.0f);
  float t = smoothstep(aa, -aa, d);

  float df = _expf(-0.1f*(distance_f3(ro, pp)-2.0f));  
  float3 acol = hsv2rgb(to_float3(_mix(0.9f, 0.6f, df), 0.9f, _mix(1.0f, 0.0f, df)), hsv2rgb_K);
  float3 gcol = hsv2rgb(to_float3(0.6f, 0.5f, tanh_approx(_expf(-_mix(2.0f, 8.0f, df)*lod))), hsv2rgb_K);
  
  float3 col = to_float3_s(0.0f);
  col += acol;
  col += 0.5f*gcol;
  
  return to_float4_aw(col, t);
}

__DEVICE__ float3 stars(float2 sp, float hh, float4 hsv2rgb_K, float iTime) {
  const float3 scol0 = HSV2RGB(to_float3(0.85f, 0.8f, 1.0f));
  const float3 scol1 = HSV2RGB(to_float3(0.65f, 0.5f, 1.0f));
  float3 col = to_float3_s(0.0f);
  
  const float m = 6.0f;

  for (float i = 0.0f; i < m; ++i) {
    float2 pp = sp+0.5f*i;
    float s = i/(m-1.0f);
    float2 dim  = to_float2_s(_mix(0.05f, 0.003f, s)*PI);
    float2 np = mod2(&pp, dim);
    float2 h = hash2(np+127.0f+i);
    float2 o = -1.0f+2.0f*h;
    float y = _sinf(sp.x);
    pp += o*dim*0.5f;
    pp.y *= y;
    float l = length(pp);
  
    float h1 = fract(h.x*1667.0f);
    float h2 = fract(h.x*1887.0f);
    float h3 = fract(h.x*2997.0f);

    float3 scol = _mix(8.0f*h2, 0.25f*h2*h2, s)*_mix(scol0, scol1, h1*h1);

    float3 ccol = col + _expf(-(_mix(6000.0f, 2000.0f, hh)/_mix(2.0f, 0.25f, s))*_fmaxf(l-0.001f, 0.0f))*scol;
    ccol *= _mix(0.125f, 1.0f, smoothstep(1.0f, 0.99f, _sinf(0.25f*TIME+TAU*h.y)));
    col = h3 < y ? ccol : col;
  }
  
  return col;
}

__DEVICE__ float3 toSpherical(float3 p) {
  float r   = length(p);
  float t   = _acosf(p.z/r);
  float ph  = _atan2f(p.y, p.x);
  return to_float3(r, t, ph);
}

__DEVICE__ float3 skyColor(float3 ro, float3 rd, float4 hsv2rgb_K, float iTime) {
  const float3 acol   = HSV2RGB(to_float3(0.6f, 0.9f, 0.075f));
  const float3 lpos   = 1E6*to_float3(0.0f, -0.15f, 1.0f);
  const float3 ldir   = normalize(lpos);
  const float3 lcol   = HSV2RGB(to_float3(0.75f, 0.8f, 1.0f));

  const float4 mdim   = to_float4_aw(1E5*to_float3(0.0f, 0.4f, 1.0f), 20000.0f);
  const float3 mcol   = HSV2RGB(to_float3(0.75f, 0.7f, 1.0f));

  float2 sp     = swi2(toSpherical(swi3(rd,x,z,y)),y,z);

  float lf    = _powf(_fmaxf(dot(ldir, rd), 0.0f), 80.0f);
  float li    = 0.02f*_mix(1.0f, 10.0f, lf)/(_fabs((rd.y+0.055f))+0.025f);
  float lz    = step(-0.055f, rd.y);

  float2 md     = raySphere(ro, rd, mdim);
  float3 mpos   = ro + rd*md.x;
  float3 mnor   = normalize(mpos-swi3(mdim,x,y,z));
  float mdif    = _fmaxf(dot(ldir, mnor), 0.0f);
  float mf      = smoothstep(0.0f, 10000.0f, md.y - md.x);

  float3 col = to_float3_s(0.0f);
  col += stars(sp, 0.25f, hsv2rgb_K, iTime)*smoothstep(0.5f, 0.0f, li)*lz;  
  col = _mix(col, (mdif)*mcol*4.0f, mf);
  col += smoothstep(-0.4f, 0.0f, (sp.x-PI*0.5f))*acol;
  col += tanh_f3(lcol*li);
  return col;
}

__DEVICE__ float3 color(float3 ww, float3 uu, float3 vv, float3 ro, float2 p, float4 hsv2rgb_K, float iTime, float2 iResolution) {
  float lp = length(p);
  float2 np = p + 2.0f/RESOLUTION.y;
//  float rdd = (2.0f-1.0f*tanh_approx(lp));  // Playing around with rdd can give interesting distortions
  float rdd = 2.0f;
  float3 rd = normalize(p.x*uu + p.y*vv + rdd*ww);
  float3 nrd = normalize(np.x*uu + np.y*vv + rdd*ww);

  const float planeDist = 1.0f;
  const int furthest = 12;
  const int fadeFrom = _fmaxf(furthest-2, 0);

  const float fadeDist = planeDist*float(fadeFrom);
  const float maxDist  = planeDist*float(furthest);
  float nz = _floor(ro.z / planeDist);

  float3 skyCol = skyColor(ro, rd, hsv2rgb_K, iTime);


  float4 acol = to_float4_s(0.0f);
  const float cutOff = 0.95f;
  bool cutOut = false;

  // Steps from nearest to furthest plane and accumulates the color 
  for (int i = 1; i <= furthest; ++i) {
    float pz = planeDist*nz + planeDist*(float)(i);

    float pd = (pz - ro.z)/rd.z;

    float3 pp = ro + rd*pd;
    
    if (pp.y < 0.0f && pd > 0.0f && acol.w < cutOff) {
      float3 npp = ro + nrd*pd;

      float3 off = to_float3_s(0.0f);

      float4 pcol = plane(ro, rd, pp, npp, off, nz+(float)(i), hsv2rgb_K);

      float nz = pp.z-ro.z;
      float fadeIn = smoothstep(maxDist, fadeDist, pd);
      swi3S(pcol,x,y,z, _mix(skyCol, swi3(pcol,x,y,z), fadeIn));
//      pcol.w *= fadeOut;
      pcol = clamp(pcol, 0.0f, 1.0f);

      acol = alphaBlend(pcol, acol);
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

__DEVICE__ float3 effect(float2 p, float2 q, float4 hsv2rgb_K, float iTime, float2 iResolution, float3 View) {
  float tm= TIME*0.25f;
  float3 ro = to_float3(0.0f, 0.0f, tm) + View;
  float3 dro= normalize(to_float3(0.0f, 0.09f, 1.0f));  
  float3 ww = normalize(dro);
  float3 uu = normalize(cross(normalize(to_float3(0.0f,1.0f,0.0f)), ww));
  float3 vv = normalize(cross(ww, uu));

  float3 col = color(ww, uu, vv, ro, p, hsv2rgb_K, iTime, iResolution);
  
  return col;
}

// License: Unknown, author: nmz (twitter: @stormoid), found: https://www.shadertoy.com/view/NdfyRM
__DEVICE__ float sRGB(float t) { return _mix(1.055f*_powf(t, 1.0f/2.4f) - 0.055f, 12.92f*t, step(t, 0.0031308f)); }
// License: Unknown, author: nmz (twitter: @stormoid), found: https://www.shadertoy.com/view/NdfyRM
__DEVICE__ float3 sRGB(in float3 c) { return to_float3(sRGB(c.x), sRGB(c.y), sRGB(c.z)); }

// License: Unknown, author: Matt Taylor (https://github.com/64), found: https://64.github.io/tonemapping/
__DEVICE__ float3 aces_approx(float3 v) {
  v = _fmaxf(v, to_float3_s(0.0f));
  v *= 0.6f;
  float a = 2.51f;
  float b = 0.03f;
  float c = 2.43f;
  float d = 0.59f;
  float e = 0.14f;
  return clamp((v*(a*v+b))/(v*(c*v+d)+e), 0.0f, 1.0f);
}

__KERNEL__ void NeonwaveSunriseFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse)
{
  CONNECT_POINT0(ViewXY, 0.0f, 0.0f );
  CONNECT_SLIDER0(ViewZ, -10.0f, 10.0f, 0.0f);
  
  float3 View = to_float3_aw(ViewXY, ViewZ);

  const float4 hsv2rgb_K = to_float4(1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);

  float2 q = fragCoord/swi2(RESOLUTION,x,y);
  float2 p = -1.0f + 2.0f * q;
  p.x *= RESOLUTION.x/RESOLUTION.y;
  float3 col = to_float3_s(0.0f);
  col = effect(p, q, hsv2rgb_K, iTime, iResolution, View);
  col *= smoothstep(0.0f, 8.0f, TIME-_fabs(q.y));
  col = aces_approx(col);
  col = sRGB(col);
  fragColor = to_float4_aw(col, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}