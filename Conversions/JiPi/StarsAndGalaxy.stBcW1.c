
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


// License CC0: Stars and galaxy
// Bit of sunday tinkering lead to stars and a galaxy
// Didn't turn out as I envisioned but it turned out to something
// that I liked so sharing it.

// Controls how many layers of stars
#define LAYERS            5.0f

#define PI                3.141592654f
#define TAU               (2.0f*PI)
#define TIME              mod_f(iTime, 30.0f)
#define TTIME             (TAU*TIME)
#define RESOLUTION        iResolution
#define ROT(a)            to_mat2(_cosf(a), _sinf(a), -_sinf(a), _cosf(a))

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

// License: Unknown, author: Unknown, found: don't remember
__DEVICE__ float tanh_approx(float x) {
  //  Found this somewhere on the interwebs
  //  return _tanhf(x);
  float x2 = x*x;
  return clamp(x*(27.0f + x2)/(27.0f+9.0f*x2), -1.0f, 1.0f);
}


// License: WTFPL, author: sam hocevar, found: https://stackoverflow.com/a/17897228/418488

__DEVICE__ float3 hsv2rgb(float3 c) {
  const float4 hsv2rgb_K = to_float4(1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);

  float3 p = abs_f3(fract_f3(swi3(c,x,x,x) + swi3(hsv2rgb_K,x,y,z)) * 6.0f - swi3(hsv2rgb_K,w,w,w));
  return c.z * _mix(swi3(hsv2rgb_K,x,x,x), clamp(p - swi3(hsv2rgb_K,x,x,x), 0.0f, 1.0f), c.y);
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

__DEVICE__ float2 shash2(float2 p) {
  return -1.0f+2.0f*hash2(p);
}

__DEVICE__ float3 toSpherical(float3 p) {
  float r   = length(p);
  float t   = _acosf(p.z/r);
  float ph  = _atan2f(p.y, p.x);
  return to_float3(r, t, ph);
}


// License: CC BY-NC-SA 3.0f, author: Stephane Cuillerdier - Aiekick/2015 (twitter:@aiekick), found: https://www.shadertoy.com/view/Mt3GW2
__DEVICE__ float3 blackbody(float Temp) {
  float3 col = to_float3_s(255.0f);
  col.x = 56100000.0f * _powf(Temp,(-3.0f / 2.0f)) + 148.0f;
  col.y = 100.04f * _logf(Temp) - 623.6f;
  if (Temp > 6500.0f) col.y = 35200000.0f * _powf(Temp,(-3.0f / 2.0f)) + 184.0f;
  col.z = 194.18f * _logf(Temp) - 1448.6f;
  col = clamp(col, 0.0f, 255.0f)/255.0f;
  if (Temp < 1000.0f) col *= Temp/1000.0f;
  return col;
}


// License: MIT, author: Inigo Quilez, found: https://www.shadertoy.com/view/XslGRr
__DEVICE__ float noise(float2 p) {
  // Found at https://www.shadertoy.com/view/sdlXWX
  // Which then redirected to IQ shader
  float2 i = _floor(p);
  float2 f = fract_f2(p);
  float2 u = f*f*(3.0f-2.0f*f);
  
  float n =
         _mix( _mix( dot(shash2(i + to_float2(0.0f,0.0f) ), f - to_float2(0.0f,0.0f)), 
                     dot(shash2(i + to_float2(1.0f,0.0f) ), f - to_float2(1.0f,0.0f)), u.x),
               _mix( dot(shash2(i + to_float2(0.0f,1.0f) ), f - to_float2(0.0f,1.0f)), 
                     dot(shash2(i + to_float2(1.0f,1.0f) ), f - to_float2(1.0f,1.0f)), u.x), u.y);

  return 2.0f*n;              
}

__DEVICE__ float fbm(float2 p, float o, float s, int iters) {
  p *= s;
  p += o;

  const float aa = 0.5f;
  //const mat2 pp = mul_f_mat2(2.04f,ROT(1.0f));
  const mat2 pp = ROT(2.04f);
float zzzzzzzzzzzzzzzzzzzz;
  float h = 0.0f;
  float a = 1.0f;
  float d = 0.0f;
  for (int i = 0; i < iters; ++i) {
    d += a;
    h += a*noise(p);
    p += to_float2(10.7f, 8.3f);
    p = mul_f2_mat2(p,pp);
    a *= aa;
  }
  h /= d;
  
  return h;
}

__DEVICE__ float height(float2 p) {
  float h = fbm(p, 0.0f, 5.0f, 5);
  h *= 0.3f;
  h += 0.0f;
  return (h);
}

__DEVICE__ float3 stars(float3 ro, float3 rd, float2 sp, float hh) {
  float3 col = to_float3_s(0.0f);
  
  const float m = LAYERS;
  hh = tanh_approx(20.0f*hh);

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

    float3 scol = _mix(8.0f*h2, 0.25f*h2*h2, s)*blackbody(_mix(3000.0f, 22000.0f, h1*h1));

    float3 ccol = col + _expf(-(_mix(6000.0f, 2000.0f, hh)/_mix(2.0f, 0.25f, s))*_fmaxf(l-0.001f, 0.0f))*scol;
    col = h3 < y ? ccol : col;
  }
  
  return col;
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


__DEVICE__ float4 moon(float3 ro, float3 rd, float2 sp, float3 lp, float4 md) {
  float2 mi = raySphere(ro, rd, md);
  
  float3 p    = ro + mi.x*rd;
  float3 n    = normalize(p-swi3(md,x,y,z));
  float3 r    = reflect(rd, n);
  float3 ld   = normalize(lp - p);
  float fre = dot(n, rd)+1.0f;
  fre = _powf(fre, 15.0f);
  float dif = _fmaxf(dot(ld, n), 0.0f);
  float spe = _powf(_fmaxf(dot(ld, r), 0.0f), 8.0f);
  float i = 0.5f*tanh_approx(20.0f*fre*spe+0.05f*dif);
  float3 col = blackbody(1500.0f)*i+hsv2rgb(to_float3(0.6f, _mix(0.6f, 0.0f, i), i));

  float t = tanh_approx(0.25f*(mi.y-mi.x));
 
  return to_float4_aw(col, t);
}

__DEVICE__ float3 sky(float3 ro, float3 rd, float2 sp, float3 lp, out float *cf) {
  float ld = _fmaxf(dot(normalize(lp-ro), rd),0.0f);
  float y = -0.5f+sp.x/PI;
  y = _fmaxf(_fabs(y)-0.02f, 0.0f)+0.1f*smoothstep(0.5f, PI, _fabs(sp.y));
  float3 blue = hsv2rgb(to_float3(0.6f, 0.75f, 0.35f*_expf(-15.0f*y)));
  float ci = _powf(ld, 10.0f)*2.0f*_expf(-25.0f*y); 
  float3 yellow = blackbody(1500.0f)*ci;
  *cf = ci;
  return blue+yellow;
}

__DEVICE__ float3 galaxy(float3 ro, float3 rd, float2 sp, out float *sf) {
  float2 gp = sp;
  gp = mul_f2_mat2(gp,ROT(0.67f));
  gp += to_float2(-1.0f, 0.5f);
  float h1 = height(2.0f*sp);
  float gcc = dot(gp, gp);
  float gcx = _expf(-(_fabs(3.0f*(gp.x))));
  float gcy = _expf(-_fabs(10.0f*(gp.y)));
  float gh = gcy*gcx;
  float cf = smoothstep(0.05f, -0.2f, -h1);
  float3 col = to_float3_s(0.0f);
  col += blackbody(_mix(300.0f, 1500.0f, gcx*gcy))*gcy*gcx;
  col += hsv2rgb(to_float3(0.6f, 0.5f, 0.00125f/gcc));
  col *= _mix(_mix(0.15f, 1.0f, gcy*gcx), 1.0f, cf);
  *sf = gh*cf;
  return col;
}

__DEVICE__ float3 grid(float3 ro, float3 rd, float2 sp, float2 iResolution) {
  const float m = 1.0f;

  const float2 dim = to_float2_s(1.0f/8.0f*PI);
  float2 pp = sp;
  float2 np = mod2(&pp, dim);

  float3 col = to_float3_s(0.0f);

  float y = _sinf(sp.x);
  float d = _fminf(_fabs(pp.x), _fabs(pp.y*y));
  
  float aa = 2.0f/RESOLUTION.y;
  
  col += 2.0f*to_float3(0.5f, 0.5f, 1.0f)*_expf(-2000.0f*_fmaxf(d-0.00025f, 0.0f));
  
  return 0.25f*tanh_f3(col);
}

__DEVICE__ float3 color(float3 ro, float3 rd, float3 lp, float4 md, float2 iResolution, bool GridOn) {
  float2 sp = swi2(toSpherical(swi3(rd,x,z,y)),y,z);

  float sf = 0.0f;
  float cf = 0.0f;
  float3 col = to_float3_s(0.0f);

  float4 mcol = moon(ro, rd, sp, lp, md);

  col += stars(ro, rd, sp, sf)*(1.0f-tanh_approx(2.0f*cf));
  col += galaxy(ro, rd, sp, &sf);
  col = _mix(col, swi3(mcol,x,y,z), mcol.w);
  col += sky(ro, rd, sp, lp, &cf);
  if ( GridOn) col += grid(ro, rd, sp, iResolution);

  if (rd.y < 0.0f)
  {
    col = to_float3_s(0.0f);
  }

  return col;
}

__KERNEL__ void StarsAndGalaxyFuse(float4 fragColor, float2 fragCoord, float iTime, float4 iMouse, float2 iResolution)
{
  CONNECT_CHECKBOX0(GridOn, 1);

  CONNECT_SLIDER0(RoZ, -1.0f, 1.0f, 0.0f);
  
  CONNECT_POINT0(View, 0.0f, 0.0f);
  
  

  float2 q = fragCoord/iResolution;
  float2 p = -1.0f + 2.0f*q;
  p.x *= RESOLUTION.x/RESOLUTION.y;

  float3 ro = to_float3(0.0f, 0.0f, 0.0f);
  
  ro.x += iMouse.x/iResolution.x - 0.5f;
  ro.y += iMouse.y/iResolution.y - 0.5f;
  ro.z += RoZ;
  
  float3 lp = 500.0f*to_float3(1.0f, -0.25f, 0.0f);
  float4 md = 50.0f*to_float4(1.0f, 1.0f, -0.6f, 0.5f);
  float3 la = to_float3(1.0f, 0.5f, 0.0f);
  
  la.x += View.x;
  la.y += View.y;
  
  float3 up = to_float3(0.0f, 1.0f, 0.0f);
  swi2S(la,x,z, mul_f2_mat2(swi2(la,x,z) , ROT(TTIME/60.0f-PI/2.0f)));

  float3 ww = normalize(la - ro);
  float3 uu = normalize(cross(up, ww));
  float3 vv = normalize(cross(ww,uu));
  float3 rd = normalize(p.x*uu + p.y*vv + 2.0f*ww);
  float3 col= color(ro, rd, lp, md,iResolution, GridOn);
  
  col *= smoothstep(0.0f, 4.0f, TIME)*smoothstep(30.0f, 26.0f, TIME);
  col = aces_approx(col);
  col = sRGB(col);

  fragColor = to_float4_aw(col,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}