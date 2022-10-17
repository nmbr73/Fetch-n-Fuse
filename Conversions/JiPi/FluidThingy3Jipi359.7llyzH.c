
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution



//code shortcuts
#define A(u) _tex2DVecN(iChannel0,(u).x,(u).y,15)
#define B(u) _tex2DVecN(iChannel1,(u).x,(u).y,15)
#define C(u) _tex2DVecN(iChannel2,(u).x,(u).y,15)
#define D(u) _tex2DVecN(iChannel3,(u).x,(u).y,15)

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Preset: Keyboard' to iChannel2
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1


/* Linear Filtering Shim by Theron Tarigo, 2019.
   https://www.shadertoy.com/view/tssXWf

   The following codes may be used and copied freely,
   with or without attribution.  However, please do not
   remove the URL, so that others may find the explanation
   provided here, which may be expanded in future.
   This is not a legal requirement.
*/


#ifdef XXX


#define LINEAR_FILTER_CONFIG  true,true,true,true

const struct LINEARFILTER_T {
  bool iChannel0, iChannel1, iChannel2, iChannel3;
} LINEARFILTER = LINEARFILTER_T(LINEAR_FILTER_CONFIG);

const struct SAMPLERINDEX_T {
  int iChannel0, iChannel1, iChannel2, iChannel3;
} SAMPLERINDEX = SAMPLERINDEX_T(0,1,2,3);

#ifdef ORG
__DEVICE__ float4 textureLinearPix (sampler2D sampler, float2 U) {
  return _mix(mix(
      texelFetch(sampler,to_int2(U)+to_int2(0,0),0),
      texelFetch(sampler,to_int2(U)+to_int2(1,0),0),
    fract((U).x)),_mix(
      texelFetch(sampler,to_int2(U)+to_int2(0,1),0),
      texelFetch(sampler,to_int2(U)+to_int2(1,1),0),
    fract((U).x)),fract((U).y));
}
#endif


__DEVICE__ float4 textureLinearPix (sampler2D sampler, float2 U) {
  return _mix(mix(
      texture(sampler,(to_float2(to_int2(U)+to_int2(0,0))+0.5f)/iR),
      texture(sampler,(to_float2(to_int2(U)+to_int2(1,0))+0.5f)/iR),
    fract((U).x)),_mix(
      texture(sampler,(to_float2(to_int2(U)+to_int2(0,1))+0.5f)/iR),
      texture(sampler,(to_float2(to_int2(U)+to_int2(1,1))+0.5f)/iR),
    fract((U).x)),fract((U).y));
}
#define textureLinear(sampler,P) textureLinearPix(sampler, \
    (P)*iChannelResolution[SAMPLERINDEX.sampler].xy-to_float2_s(0.5f))

#define _tex2DVecN(sampler,P.x,P.y,15) ( LINEARFILTER.iChannel0 ? \
    textureLinear(sampler,P) \
  : _tex2DVecN(sampler,P.x,P.y,15) )
// End of Linear Filtering Shim


#endif

/*
** This is fluid
** Following this tutorial : http://wyattflanders.com/MeAndMyNeighborhood.pdf
*/



__DEVICE__ float4 F(float2 p, float2 iResolution, __TEXTURE2D__ iChannel0)
{
    float2 r = swi2(A(p/R),x,y);
    r = p - r;
    return A(r/R);
}

__KERNEL__ void FluidThingy3Jipi359Fuse__Buffer_A(float4 o, float2 f, float iTime, float2 iResolution, float4 iMouse, float iTimeDelta, int iFrame, float3 iChannelResolution[], sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
    CONNECT_CHECKBOX0(Reset, 0); 
  
    f+=0.5f;
   
   
    const float2 n = to_float2(+0.0f, +1.0f);
    const float2 s = -n;
    const float2 w = to_float2(+1.0f, +0.0f);
    const float2 e = -w;

    if (f.x < 10.0f || f.y < 10.0f || -f.x+R.x < 10.0f || -f.y+R.y < 10.0f) {o = to_float4_s(0.0f);   SetFragmentShaderComputedColor(o); return;}

    //float kb = C(to_float2(32.5f/256.0f, 0.25f)).x;
    if (iFrame < 10 || Reset) {o = to_float4_s(0);   SetFragmentShaderComputedColor(o); return;}

    o = F(f,R,iChannel0);
    float4 En = F(f+n,R,iChannel0);
    float4 Es = F(f+s,R,iChannel0);
    float4 Ew = F(f+w,R,iChannel0);
    float4 Ee = F(f+e,R,iChannel0);

    o.z = (En + Es + Ew + Ee).z * 0.25f;//06125;

    swi2S(o,x,y, swi2(o,x,y) + to_float2(Ee.z - Ew.z, Es.z - En.z) * 0.25f);

    o.z += (Es.y - En.y + Ee.x - Ew.x) *0.25f;

    //swi2(o,x,y) += (B(f/R).xy -0.5f)/400.0f;
    //o.y += -o.w*_sinf(1.57f+1.0f*g(D(f/R)))/400.0f;//*iTimeDelta*1.0f;

    o.y += -o.w*iTimeDelta*0.75f;
    //    o.y += -o.w/400.0f;
    //swi2(o,x,y) += -o.w*(cs(length(o.w)*6.28f)/200.0f);
    o.w += B(f/R).y*iTimeDelta*1.0f;
    o.w += (Ee.x*Ee.w-Ew.x*Ew.w+Es.y*Es.w-En.y*En.w) * 0.25f;

    //swi2(o,x,y) += o.w*cs(o.w*50.0f*1.0f+g(B(f/R))*5000.0f )*0.505f;
    //swi2(o,x,y) += o.w*cs( (o.w*10.0f/(1.0001f+g(B(f/R)))) * 100.0f)*0.10501f;
    //swi2(o,x,y) += -NA(f/R).xy*1.0f/8.0f;
    //o.w += 0.001001f * (g(B(f/R))*2.0f-1.0f);

    //if (iMouse.z > 0.5f && length(f-swi2(iMouse,x,y)) < 100.0f) o.w = 0.5f;

    //if (f.x < 9.0f || f.y < 9.0f || -f.x+R.x < 9.0f || -f.y+R.y < 9.0f) o *= 0.0f;

    o = clamp(o, -1.0f, 1.0f);


  SetFragmentShaderComputedColor(o);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Preset: Keyboard' to iChannel2
// Connect Buffer B 'Previsualization: Buffer A' to iChannel1
// Connect Buffer B 'Previsualization: Buffer B' to iChannel0



/* Linear Filtering Shim by Theron Tarigo, 2019.
   https://www.shadertoy.com/view/tssXWf

   The following codes may be used and copied freely,
   with or without attribution.  However, please do not
   remove the URL, so that others may find the explanation
   provided here, which may be expanded in future.
   This is not a legal requirement.
*/

#ifdef XXX

#define LINEAR_FILTER_CONFIG  true,true,true,true
//#define LINEAR_FILTER_CONFIG  false,false,false,false

const struct LINEARFILTER_T {
  bool iChannel0, iChannel1, iChannel2, iChannel3;
} LINEARFILTER = LINEARFILTER_T(LINEAR_FILTER_CONFIG);

const struct SAMPLERINDEX_T {
  int iChannel0, iChannel1, iChannel2, iChannel3;
} SAMPLERINDEX = SAMPLERINDEX_T(0,1,2,3);


#ifdef ORG
__DEVICE__ float4 textureLinearPix (sampler2D sampler, float2 U) {
  return _mix(mix(
      texelFetch(sampler,to_int2(U)+to_int2(0,0),0),
      texelFetch(sampler,to_int2(U)+to_int2(1,0),0),
    fract((U).x)),_mix(
      texelFetch(sampler,to_int2(U)+to_int2(0,1),0),
      texelFetch(sampler,to_int2(U)+to_int2(1,1),0),
    fract((U).x)),fract((U).y));
}
#endif


__DEVICE__ float4 textureLinearPix (sampler2D sampler, float2 U) {
  return _mix(mix(
      texture(sampler,(to_float2(to_int2(U)+to_int2(0,0))+0.5f)/iR),
      texture(sampler,(to_float2(to_int2(U)+to_int2(1,0))+0.5f)/iR),
    fract((U).x)),_mix(
      texture(sampler,(to_float2(to_int2(U)+to_int2(0,1))+0.5f)/iR),
      texture(sampler,(to_float2(to_int2(U)+to_int2(1,1))+0.5f)/iR),
    fract((U).x)),fract((U).y));
}



#define textureLinear(sampler,P) textureLinearPix(sampler, \
    (P)*iChannelResolution[SAMPLERINDEX.sampler].xy-to_float2_s(0.5f))

#define _tex2DVecN(sampler,P.x,P.y,15) ( LINEARFILTER.iChannel0 ? \
    textureLinear(sampler,P) \
  : _tex2DVecN(sampler,P.x,P.y,15) )
// End of Linear Filtering Shim

#endif

/*
** This is reaction diffusion
** Taken from me at : https://www.shadertoy.com/view/XlKXDm
*/

#define FEED_DEFAULT (0.030550f + 0.0105f*bvar*0.0f  )
#define KILL_DEFAULT (0.0620f   + 0.060205f*bvar*0.0f)


__DEVICE__ float2  laplacian_convolution(float2 uv, float2 texsize, float2 iResolution, __TEXTURE2D__ iChannel0)
{
  float2  ret = to_float2_s(0.0f);
    
    if (uv.x == 0.0f || uv.y == 0.0f || uv.x== 1.0f || uv.y ==1.0f)
        return (ret);
    ret += swi2(texture(iChannel0, to_float2(uv.x , uv.y) ),x,y) * -1.0f;
    
    ret += swi2(texture(iChannel0, to_float2(uv.x -texsize.x, uv.y) ),x,y) * (0.2f);
    ret += swi2(texture(iChannel0, to_float2(uv.x +texsize.x, uv.y) ),x,y) * (0.2f);
    ret += swi2(texture(iChannel0, to_float2(uv.x , uv.y -texsize.y) ),x,y) * (0.2f);
    ret += swi2(texture(iChannel0, to_float2(uv.x , uv.y +texsize.y) ),x,y) * (0.2f);
    
    ret += swi2(texture(iChannel0, to_float2(uv.x -texsize.x, uv.y -texsize.y) ),x,y) * (0.05f);
    ret += swi2(texture(iChannel0, to_float2(uv.x +texsize.x, uv.y -texsize.y) ),x,y) * (0.05f);
    ret += swi2(texture(iChannel0, to_float2(uv.x +texsize.x, uv.y +texsize.y) ),x,y) * (0.05f);
    ret += swi2(texture(iChannel0, to_float2(uv.x -texsize.x, uv.y +texsize.y) ),x,y) * (0.05f);
    return (ret);
}

__KERNEL__ void FluidThingy3Jipi359Fuse__Buffer_B(float4 o, float2 f, float iTime, float2 iResolution, float4 iMouse, float iTimeDelta, float3 iChannelResolution[], sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
    CONNECT_CHECKBOX0(Reset, 0); 

    f+=0.5f;

    float2 uv = f / R;
    float2 texsize = 1.0f/R;
    float4  ret = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    float  bvar = B(uv).w;
    float2  ab = swi2(ret,x,y)+to_float2(0.0f, 1.0f)*bvar*iTimeDelta*1.0f;
    float2  mouse = swi2(iMouse,x,y) / iResolution;
    //float  kb = C(to_float2(32.5f/256.0f, 0.25f)).x;
    if (iMouse.z > 0.5f && length(f-swi2(iMouse,x,y)) < 50.0f) 
    {
      o.y = 1.0f;
        return;
  }
    if ( _fabs(iTime) <= 0.5f || Reset)
    {
        o.x = 1.0f;
        o.y = 0.0f;
        if (_sinf(uv.x) <0.5f && _sinf(uv.y-0.4f) < 0.5f && _sinf(uv.x) > 0.45f && _sinf(uv.y-0.4f) > 0.45f)
            o = to_float4(1.0f, 1.0f, 0.0f, 1.0f);
    }
    else
    {
     o.x = clamp(ab.x + (1.0f * (laplacian_convolution(uv,texsize,R,iChannel0).x) - ab.x * ab.y * ab.y + FEED_DEFAULT * (1.0f - ab.x) ) ,-1.0f,1.0f);
     o.y = clamp(ab.y + (0.5f * (laplacian_convolution(uv,texsize,R,iChannel0).y) + ab.x * ab.y * ab.y - (FEED_DEFAULT + KILL_DEFAULT) * ab.y ),-1.0f,1.0f);
    }


  SetFragmentShaderComputedColor(o);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: St Peters Basilica_0' to iChannel2
// Connect Image 'Previsualization: Buffer A' to iChannel1
// Connect Image 'Previsualization: Buffer B' to iChannel0



/*
** License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.
** Created by bal-khan
**
** This is rendering
** I took rendering from flockaroo at : https://www.shadertoy.com/view/WdVXWy
*/

#define FANCY_REFLECTIVE   // undef to see the reac-diff in 2 colors only, I like it too
#define MOTION_SICKNESS 1.0f // change to 0 to stop mooving the cubemap

__KERNEL__ void FluidThingy3Jipi359Fuse(float4 o, float2 f, float iTime, float2 iResolution, sampler2D iChannel0)
{

    f+=0.5f;

    float2 uv = f / swi2(R,x,y);
    float4 ret = _tex2DVecN(iChannel0,uv.x,uv.y,15);
  #ifdef FANCY_REFLECTIVE
    float2 d  = to_float2(1.0f/R.y, 0.0f);
    float2 gd = to_float2( (A(uv+swi2(d,x,y))-A(uv-swi2(d,x,y)) ).x , (A(uv+swi2(d,y,x))-A(uv-swi2(d,y,x))).x )/R.y;
    float3 n  = normalize( to_float3_aw(gd*500.0f, 1.0f) );
    float3 rd = normalize( to_float3_aw((f-R*0.5f-MOTION_SICKNESS*to_float2( (_sinf(iTime*0.25f)+0.5f)*30.0f,0.0f))/R, -0.25f) );
    rd = reflect(rd, n);
    float3 rf = swi3(C(swi3(rd,x,y,z)),x,y,z);
    //rf = _sinf(length(rf)*1.0f+0.0f*to_float3(0.0f, 1.04f, 2.08f));
    swi3S(ret,x,y,z, rf*ret.x);
  #endif 
    o = to_float4_aw(swi3(ret,x,y,z), 1.0f);

  SetFragmentShaderComputedColor(o);
}