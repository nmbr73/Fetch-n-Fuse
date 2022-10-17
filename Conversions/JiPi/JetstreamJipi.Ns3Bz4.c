
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: RGBA Noise Medium' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// srtuss, 2015
//
// Volumetric cloud tunnel, a single light source, lightning and raindrops.
//
// The code is a bit messy, but in this case it's visuals that count. :P


#define pi 3.1415926535897932384626433832795f

struct ITSC
{
  float3 p;
  float dist;
  float3 n;
  float2 uv;
};

__DEVICE__ struct ITSC raycylh(float3 ro, float3 rd, float3 c, float r)
{

  struct ITSC i;
  i.dist = 1e38f;
  float3 e = ro - c;
  float a = dot(swi2(rd,x,y), swi2(rd,x,y));
  float b = 2.0f * dot(swi2(e,x,y), swi2(rd,x,y));
  float cc = dot(swi2(e,x,y), swi2(e,x,y)) - r;
  float f = b * b - 4.0f * a * cc;
  if(f > 0.0f)
  {
    f = _sqrtf(f);
    float t = (-b + f) / (2.0f * a);
    
    if(t > 0.001f)
    {
      i.dist = t;
      i.p = e + rd * t;
      i.n = -to_float3_aw(normalize(swi2(i.p,x,y)), 0.0f);
    }
  }
  return i;
}

__DEVICE__ void tPlane(inout struct ITSC *hit, float3 ro, float3 rd, float3 o, float3 n, float3 tg, float2 si)
{
    float2 uv;
    ro -= o;
    float t = -dot(ro, n) / dot(rd, n);
    if(t < 0.0f)
        return;
    float3 its = ro + rd * t;
    uv.x = dot(its, tg);
    uv.y = dot(its, cross(tg, n));
    if(_fabs(uv.x) > si.x || _fabs(uv.y) > si.y)
        return;
    
    //if(t < hit.dist)
    {
        (*hit).dist = t;
        (*hit).uv = uv;
    }
    return;
}


__DEVICE__ float hsh(float x)
{
    return fract(_sinf(x * 297.9712f) * 90872.2961f);
}

__DEVICE__ float nseI(float x)
{
    float fl = _floor(x);
    return _mix(hsh(fl), hsh(fl + 1.0f), smoothstep(0.0f, 1.0f, fract(x)));
}

__DEVICE__ float2 _rotate(float2 p, float a)
{
  return to_float2(p.x * _cosf(a) - p.y * _sinf(a), p.x * _sinf(a) + p.y * _cosf(a));
}

__DEVICE__ float nse3d(in float3 x, __TEXTURE2D__ iChannel0)
{
  float3 p = _floor(x);
  float3 f = fract_f3(x);
  f = f * f * (3.0f - 2.0f * f);
  float2 uv = (swi2(p,x,y) + to_float2(37.0f, 17.0f) * p.z) + swi2(f,x,y);
  float2 rg = swi2(texture( iChannel0, (uv+0.5f)/256.0f),y,x);//.yx;
  return _mix(rg.x, rg.y, f.z);
}

__DEVICE__ float nse(float2 p, __TEXTURE2D__ iChannel0)
{
    return _tex2DVecN(iChannel0,p.x,p.y,15).x;
}

__DEVICE__ float density2(float2 p, float z, float t, __TEXTURE2D__ iChannel0)
{
    float v = 0.0f;
    float fq = 1.0f, am = 0.5f, mvfd = 1.0f;
    float2 rnd = to_float2(0.3f, 0.7f);
    for(int i = 0; i < 7; i++)
    {
        rnd = fract_f2(sin_f2(rnd * 14.4982f) * 2987253.28612f);
        v += nse(p * fq + t * (rnd - 0.5f),iChannel0) * am;
        fq *= 2.0f;
        am *= 0.5f;
        mvfd *= 1.3f;
    }
    return v * _expf(z * z * -2.0f);
}



__DEVICE__ float fbm_f3(float3 p, float iTime, __TEXTURE2D__ iChannel0)
{
    float3 q = p;
    //swi2(q,x,y) = _rotate(swi2(p,x,y), iTime);
    
    p += (nse3d(p * 3.0f,iChannel0) - 0.5f) * 0.3f;
    
    //float v = nse3d(p) * 0.5f + nse3d(p * 2.0f) * 0.25f + nse3d(p * 4.0f) * 0.125f + nse3d(p * 8.0f) * 0.0625f;
    
    //p.y += iTime * 0.2f;
    
    float mtn = iTime * 0.15f;
    
    float v = 0.0f;
    float fq = 1.0f, am = 0.5f;
    for(int i = 0; i < 6; i++)
    {
        v += nse3d(p * fq + mtn * fq,iChannel0) * am;
        fq *= 2.0f;
        am *= 0.5f;
    }
    return v;
}

__DEVICE__ float fbmHQ(float3 p, float iTime, __TEXTURE2D__ iChannel0)
{
    float3 q = p;
    swi2S(q,x,y, _rotate(swi2(p,x,y), iTime));
    
    p += (nse3d(p * 3.0f,iChannel0) - 0.5f) * 0.4f;
    
    //float v = nse3d(p) * 0.5f + nse3d(p * 2.0f) * 0.25f + nse3d(p * 4.0f) * 0.125f + nse3d(p * 8.0f) * 0.0625f;
    
    //p.y += iTime * 0.2f;
    
    float mtn = iTime * 0.2f;
    
    float v = 0.0f;
    float fq = 1.0f, am = 0.5f;
    for(int i = 0; i < 9; i++)
    {
        v += nse3d(p * fq + mtn * fq,iChannel0) * am;
        fq *= 2.0f;
        am *= 0.5f;
    }
    return v;
}

__DEVICE__ float density(float3 p, float iTime, __TEXTURE2D__ iChannel0)
{
    float2 pol = to_float2(_atan2f(p.y, p.x), length(swi2(p,y,x)));
    
    float v = fbm_f3(p, iTime,iChannel0);
    
    float fo = (pol.y - 1.5f);//(densA + densB) * 0.5f);
    //fo *= (densB - densA);
    v *= _expf(fo * fo * -5.0f);

    float edg = 0.3f;
    return smoothstep(edg, edg + 0.1f, v);
}

__DEVICE__ float densityHQ(float3 p, float iTime, __TEXTURE2D__ iChannel0)
{
    float2 pol = to_float2(_atan2f(p.y, p.x), length(swi2(p,y,x)));
    
    float v = fbmHQ(p,iTime,iChannel0);
    
    float fo = (pol.y - 1.5f);//(densA + densB) * 0.5f);
    //fo *= (densB - densA);
    v *= _expf(fo * fo * -5.0f);
    
    float edg = 0.3f;
    return smoothstep(edg, edg + 0.1f, v);
}

__DEVICE__ float2 drop(inout float2 *p, float iTime)
{
    float2 mv = iTime * to_float2(0.5f, -1.0f) * 0.15f;
    
    float drh = 0.0f;
    float hl = 0.0f;
    
    float4 rnd = to_float4(0.1f, 0.2f, 0.3f, 0.4f);
    for(int i = 0; i < 20; i++)
    {
        rnd = fract_f4(sin_f4(rnd * 2.184972f) * 190723.58961f);
        float fd = fract(iTime * 0.2f + rnd.w);
        fd = _expf(fd * -4.0f);
        float r = 0.025f * (rnd.w * 1.5f + 1.0f);
        float sz = 0.35f;
        
        
        float2 q = (fract_f2((*p - mv) * sz + swi2(rnd,x,y)) - 0.5f) / sz;
        mv *= 1.06f;
        
        q.y *= -1.0f;
        float l = length(q + _powf(_fabs(dot(q, to_float2(1.0f, 0.4f))), 0.7f) * (fd * 0.2f + 0.1f));
        if(l < r)
        {
          float h = _sqrtf(r * r - l * l);
          drh = _fmaxf(drh, h * fd);
        }
        hl += _expf(length(q - to_float2(-0.02f, 0.01f)) * -30.0f) * 0.4f * fd;
    }
    *p += drh * 5.0f;
    return to_float2(drh, hl);
}


__DEVICE__ float hash1(float p)
{
  return fract(_sinf(p * 172.435f) * 29572.683f) - 0.5f;
}

__DEVICE__ float hash2(float2 p)
{
  float2 r = (456.789f * sin_f2(789.123f * swi2(p,x,y)));
  return fract(r.x * r.y * (1.0f + p.x));
}

__DEVICE__ float ns(float p)
{
  float fr = fract(p);
  float fl = _floor(p);
  return _mix(hash1(fl), hash1(fl + 1.0f), fr);
}

__DEVICE__ float fbm(float p)
{
  return (ns(p) * 0.4f + ns(p * 2.0f - 10.0f) * 0.125f + ns(p * 8.0f + 10.0f) * 0.025f);
}

__DEVICE__ float fbmd(float p)
{
  float h = 0.01f;
  return _atan2f(fbm(p + h) - fbm(p - h), h);
}

__DEVICE__ float arcsmp(float x, float seed)
{
  return fbm(x * 3.0f + seed * 1111.111f) * (1.0f - _expf(-x * 5.0f));
}

__DEVICE__ float arc(float2 p, float seed, float len)
{
  p *= len;
  //p = _rotate(p, iTime);
  float v = _fabs(p.y - arcsmp(p.x, seed));
  v += _expf((2.0f - p.x) * -4.0f);
  v = _expf(v * -60.0f) + _expf(v * -10.0f) * 0.6f;
  //v += _expf(p.x * -2.0f);
  v *= smoothstep(0.0f, 0.05f, p.x);
  return v;
}

__DEVICE__ float arcc(float2 p, float sd)
{
		
  float v = 0.0f;
  float rnd = fract(sd);
  float sp = 0.0f;
  v += arc(p, sd, 1.0f);
  for(int i = 0; i < 4; i ++)
  {
    sp = rnd + 0.01f;
    float2 mrk = to_float2(sp, arcsmp(sp, sd));
    v += arc(_rotate(p - mrk, fbmd(sp)), mrk.x, mrk.x * 0.4f + 1.5f);
    rnd = fract(_sinf(rnd * 195.2837f) * 1720.938f);
  }
  return v;
}

__KERNEL__ void JetstreamJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{

    float densA = 1.0f, densB = 2.0f;

    float2 uv = fragCoord / iResolution;
    
    uv = 2.0f * uv - 1.0f;
    uv.x *= iResolution.x / iResolution.y;
    
    float2 drh = drop(&uv, iTime);
    
    float camtm = iTime * 0.15f;
    float3 ro = to_float3(_cosf(camtm), 0.0f, camtm);
    float3 rd = normalize(to_float3_aw(uv, 1.2f));
    swi2S(rd,x,z, _rotate(swi2(rd,x,z), _sinf(camtm) * 0.4f));
    swi2S(rd,y,z, _rotate(swi2(rd,y,z), _sinf(camtm * 1.3f) * 0.4f));
    
    float3 sun = normalize(to_float3(0.2f, 1.0f, 0.1f));
    
    float sd = _sinf(fragCoord.x * 0.01f + fragCoord.y * 3.333333333f + iTime) * 1298729.146861f;
    
    float3 col;
    float dacc = 0.0f, lacc = 0.0f;
    
    float3 light = to_float3(_cosf(iTime * 8.0f) * 0.5f, _sinf(iTime * 4.0f) * 0.5f, ro.z + 4.0f + _sinf(iTime));

    struct ITSC tunRef;
    #define STP 15
    for(int i = 0; i < STP; i++)
    {
        struct ITSC itsc = raycylh(ro, rd, to_float3_s(0.0f), densB + (float)(i) * (densA - densB) / (float)(STP) + fract(sd) * 0.07f);
        float d = density(itsc.p,iTime,iChannel0);
        float3 tol = light - itsc.p;
        float dtol = length(tol);
        tol = tol * 0.1f / dtol;
        
        float dl = density(itsc.p + tol,iTime,iChannel0);
        lacc += _fmaxf(d - dl, 0.0f) * _expf(dtol * -0.2f);
        dacc += d;
        tunRef = itsc;
    }
    dacc /= (float)(STP);
    struct ITSC itsc = raycylh(ro, rd, to_float3_s(0.0f), 4.0f);
    float3 sky = to_float3(0.6f, 0.3f, 0.2f);
    sky *= 0.9f * _powf(fbmHQ(itsc.p, iTime,iChannel0), 2.0f);
    lacc = _fmaxf(lacc * 0.3f + 0.3f, 0.0f);
    float3 cloud = _powf(to_float3_s(lacc), to_float3(0.7f, 1.0f, 1.0f) * 1.0f);
    col = _mix(sky, cloud, dacc);
    col *= _expf(tunRef.dist * -0.1f);
    col += drh.y;
    
    float4 rnd = to_float4(0.1f, 0.2f, 0.3f, 0.4f);
    float arcv = 0.0f, arclight = 0.0f;
    for(int i = 0; i < 3; i++)
    {
        float v = 0.0f;
        rnd = fract_f4(sin_f4(rnd * 1.111111f) * 298729.258972f);
        float ts = rnd.z * 4.0f * 1.61803398875f + 1.0f;
        float arcfl = _floor(iTime / ts + rnd.y) * ts;
        float arcfr = fract(iTime / ts + rnd.y) * ts;
        
        struct ITSC arcits;
        arcits.dist = 1e38f;
        float arca = rnd.x + arcfl * 2.39996f;
        float arcz = ro.z + 1.0f + rnd.x * 12.0f;
        tPlane(&arcits, ro, rd, to_float3(0.0f, 0.0f, arcz), to_float3(0.0f, 0.0f, -1.0f), to_float3(_cosf(arca), _sinf(arca), 0.0f), to_float2_s(2.0f));

        float arcseed = _floor(iTime * 17.0f + rnd.y);
        if(arcits.dist < 20.0f)
        {
            arcits.uv *= 0.8f;
            v = arcc(to_float2(1.0f - _fabs(arcits.uv.x), arcits.uv.y * sign_f(arcits.uv.x)) * 1.4f, arcseed * 0.033333f);
        }
        float arcdur = rnd.x * 0.2f + 0.05f;
        float arcint = smoothstep(0.1f + arcdur, arcdur, arcfr);
        v *= arcint;
        arcv += v;
        arclight += _expf(_fabs(arcz - tunRef.p.z) * -0.3f) * fract(_sinf(arcseed) * 198721.6231f) * arcint;
    }
    float3 arccol = to_float3(0.9f, 0.7f, 0.7f);
    col += arclight * arccol * 0.5f;
    col = _mix(col, arccol, clamp(arcv, 0.0f, 1.0f));
    col = _powf(col, to_float3(1.0f, 0.8f, 0.5f) * 1.5f) * 1.5f;
    col = _powf(col, to_float3_s(1.0f / 2.2f));
    fragColor = to_float4_aw(col, 1.0f);


  SetFragmentShaderComputedColor(fragColor);
}