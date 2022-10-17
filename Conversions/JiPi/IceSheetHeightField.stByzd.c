
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------

#define R    iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


// CC0: Ice sheet height field experimentation
//  Was tinkering with using recursive voronoi patterns to
//  generate something that could pass for ice breaking up
//  into smaller blocks
#define RESOLUTION  iResolution
#define TIME        iTime

// License: Unknown, author: Unknown, found: don't remember
__DEVICE__ float2 hash2(float2 p) {
  p = to_float2(dot (p, to_float2 (127.1f, 311.7f)), dot (p, to_float2 (269.5f, 183.3f)));
  return fract_f2(sin_f2(p)*43758.5453123f);
}

// From: https://www.shadertoy.com/view/MsScWz
// Originally from: https://www.shadertoy.com/view/ldl3W8
__DEVICE__ float3 voronoi(float2 x) {
  float2 n = _floor(x);
  float2 f = fract_f2(x);

  float2 mr;
  float2 mp;

  float md = 8.0f;
  for(int j=-1; j<=1; ++j)
  for(int i=-1; i<=1; ++i) {
    float2 g = to_float2((float)(i),(float)(j));
    float2 o = hash2(n + g);
    float2 r = g + o - f;
    float d = dot(r,r);

    if(d<md) {
      md = d;
      mr = r;
      mp = x+r;
    }
  }

  md = 8.0f;
  for(int j=-1; j<=1; ++j)
  for(int i=-1; i<=1; ++i) {
    float2 g = to_float2((float)(i),(float)(j));
    float2 o = hash2(n + g);
    float2 r = g + o - f;

    if(dot(mr-r,mr-r)>0.0001f) // skip the same cell
      md = _fminf(md, dot(0.5f*(mr+r), normalize(r-mr)));
  }

  return to_float3(md, mp.x, mp.y);
}

__DEVICE__ float height(float2 p, float Params[7]) {
  float2 vp = p;
  float vz = 1.0f;
  
  const float aa = 0.025f;

  float gh = 0.0f;
  float hh = 0.0f;

  const float hf = Params[4];//0.025f;

  // Recursive voronois
  {
    float3 c = voronoi(vp);
    //gh = _tanhf(_fmaxf(_fabs(0.35f*(c.y-2.0f*_sinf(0.25f*c.z)*_cosf(sqrt(0.1f)*c.z)))-0.4f, 0.0f));
    gh = _tanhf(_fmaxf(_fabs(Params[0]*(c.y-Params[1]*_sinf(Params[2]*c.z)*_cosf(sqrt(0.1f)*c.z)))-Params[3], 0.0f));
    
    hh = smoothstep(-aa, aa, c.x-2.0f*aa*smoothstep(1.0f, 0.75f, gh));
    if (gh > 0.75f) {    
      return hf*_tanhf(hh+1.0f*(gh-0.75f));
    }

    vz *= Params[5];//0.5f;
    vp = vp * Params[6];//2.0f;
  }

  {
    float3 c = voronoi(vp);
    hh = hh*smoothstep(-aa, aa, vz*c.x-3.0f*aa*smoothstep(1.0f, 0.5f, gh));
    if (gh > 0.5f) {
      return 0.75f*hf*hh;
    }

    vz *= Params[5];//]0.5f;
    vp = vp * Params[6];//2.0f;
  }

  {
    float3 c = voronoi(vp);
    hh = hh*smoothstep(-aa, aa, vz*c.x-2.0f*aa*smoothstep(0.9f, 0.25f, gh));
    if (gh > 0.25f) {
      return 0.5f*hf*hh;
    }

    vz *= Params[5];//0.5f;
    vp = vp * Params[6];//2.0f;
  }
  
  return 0.0f;
}

__DEVICE__ float3 normal(float2 p, float2 iResolution, float Params[7]) {
  float2 v;
  float2 w;
  float2 e = to_float2(4.0f/RESOLUTION.y, 0);
  
  float3 n;
  n.x = height(p + swi2(e,x,y), Params) - height(p - swi2(e,x,y), Params);
  n.y = 2.0f*e.x;
  n.z = height(p + swi2(e,y,x), Params) - height(p - swi2(e,y,x), Params);
  
  return normalize(n);
}

__KERNEL__ void IceSheetHeightFieldFuse(float4 fragColor, float2 fragCoord, float2 iResolution, float iTime, float2 iResolution)
{
CONNECT_CHECKBOX0(Specular, 0);

  CONNECT_SLIDER0(Pa1, -1.0f, 1.0f, 0.35f);
  CONNECT_SLIDER1(Pa2, -1.0f, 10.0f, 2.0f);
  CONNECT_SLIDER2(Pa3, -1.0f, 1.0f, 0.25f);
  CONNECT_SLIDER3(Pa4, -1.0f, 1.0f, 0.4f);
  CONNECT_SLIDER4(Pa5, -1.0f, 1.0f, 0.025f);
  
  CONNECT_SLIDER5(Pa6, -1.0f, 1.0f, 0.5f);
  CONNECT_SLIDER6(Pa7, -1.0f, 10.0f, 2.0f);

  CONNECT_SLIDER7(AlphaThres, 0.0f, 1.0f, 0.2f);

  CONNECT_POINT0(LampXY, 1.0f, 1.5f);
  CONNECT_SLIDER8(LampZ, -5.0f, 5.0f, -0.95f);
  CONNECT_SLIDER9(Spec, -15.0f, 15.0f, 10.0f);
  CONNECT_INTSLIDER0(Diff, -100, 400, 200);

  float Params[7] = {Pa1,Pa2,Pa3,Pa4,Pa5,Pa6,Pa7};


  float2 q = fragCoord/RESOLUTION;
  float2 p = -1.0f + 2.0f * q;
  p.x *= RESOLUTION.x/RESOLUTION.y;
  float aa = 2.0f/RESOLUTION.y;
  
  float z = _mix(0.2f, 0.5f, smoothstep(-0.5f, 0.5f, _sinf(0.5f*TIME)));

  float2 ip = p;
  ip /= z;
  ip.y += 0.5f*TIME;
  float h = height(ip, Params);
  float3 n  = normal(ip, iResolution, Params);
 
  float3 ro = to_float3(0.0f, -1.0f, 0.0f);
  //float3 lp = to_float3(1.0f, -0.95f, 1.5f);
  float3 lp = to_float3(LampXY.x, LampZ, LampXY.y);
  
  float3 pp = to_float3(p.x, h, p.y);;
  float3 rd = normalize(ro-pp);
  float3 ld = normalize(pp-lp);
  float3 ref= reflect(rd, n);
  
  float dif = _fmaxf(dot(n, ld), 0.0f)*_tanhf((float)Diff*h);
  
  float spe = _powf(_fmaxf(dot(ref, ld), 0.0f), Spec);
  
  if (Specular) spe = _powf(_fmaxf(dot(ref, rd), 0.0f), Spec);
 
  float3 col = to_float3_s(0.0f);
  //col = swi3(texture(iChannel0, q),x,y,z);

  col += dif;
 // col += spe;
 
  if (col.x < AlphaThres && col.y < AlphaThres && col.z < AlphaThres) col = swi3(texture(iChannel0, q),x,y,z);
 
 col += spe;
 
  fragColor = to_float4_aw(col, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}