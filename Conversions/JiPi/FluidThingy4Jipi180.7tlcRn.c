
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------



/* Linear Filtering Shim by Theron Tarigo, 2019.
   https://www.shadertoy.com/view/tssXWf

   The following codes may be used and copied freely,
   with or without attribution.  However, please do not
   remove the URL, so that others may find the explanation
   provided here, which may be expanded in future.
   This is not a legal requirement.
*/

#define LINEAR_FILTER_CONFIG  true,true,true,true

const struct LINEARFILTER_T {
  bool iChannel0, iChannel1, iChannel2, iChannel3;
} LINEARFILTER = LINEARFILTER_T{LINEAR_FILTER_CONFIG};

const struct SAMPLERINDEX_T {
  int iChannel0, iChannel1, iChannel2, iChannel3;
} SAMPLERINDEX = SAMPLERINDEX_T(0,1,2,3);

__DEVICE__ float4 textureLinearPix (sampler2D sampler, float2 U) {
  return _mix(mix(
      texelFetch(sampler,to_int2(U)+to_int2(0,0),0),   //funktioniert nicht :-(
      texelFetch(sampler,to_int2(U)+to_int2(1,0),0),
    fract((U).x)),_mix(
      texelFetch(sampler,to_int2(U)+to_int2(0,1),0),
      texelFetch(sampler,to_int2(U)+to_int2(1,1),0),
    fract((U).x)),fract((U).y));
}

#define textureLinear(sampler,P) textureLinearPix(sampler, \
    (P)*iChannelResolution[SAMPLERINDEX.sampler].xy-to_float2_s(0.5f))

#define _tex2DVecN(sampler,P.x,P.y,15) ( LINEARFILTER.iChannel0 ? \
    textureLinear(sampler,P) \
  : _tex2DVecN(sampler,P.x,P.y,15) )
// End of Linear Filtering Shim

#define R iResolution

// grayscale
#define g(c) (0.3f*c.x + 0.59f*c.y + 0.11f*c.z) 

// trigonometrics
__DEVICE__ float2 cs(float a) { return to_float2(_cosf(a), _sinf(a)); }

//code shortcuts
#define A(u) _tex2DVecN(iChannel0,u.x,u.y,15)
#define B(u) _tex2DVecN(iChannel1,u.x,u.y,15)
#define C(u) _tex2DVecN(iChannel2,u.x,u.y,15)
#define D(u) _tex2DVecN(iChannel3,u.x,u.y,15)

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: London' to iChannel1
// Connect Buffer A 'Preset: Keyboard' to iChannel2
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0


/*
** Based on this tutorial : http://wyattflanders.com/MeAndMyNeighborhood.pdf
*/

//
#define ORDER 

const float2 n = to_float2(+0.0f, +1.0f);
const float2 s = -n;
const float2 w = to_float2(+1.0f, +0.0f);
const float2 e = -w;

__DEVICE__ float4 F(float2 p)
{
  float2 r = A(p/R).xy;
    r = p - r;
    return A(r/R);
}

__KERNEL__ void FluidThingy4Jipi180Fuse__Buffer_A(float4 o, float2 f, float iTime, float iTimeDelta, int iFrame)
{

    float kb = C(to_float2(32.5f/256.0f, 0.25f)).x;
    if (kb > 0.5f || iFrame < 1) {o = to_float4(0.0f,0.0f,0.0f, 0.0f ); return;}

    o = F(f);
    float4 En = F(f+n);
    float4 Es = F(f+s);
    float4 Ew = F(f+w);
    float4 Ee = F(f+e);

    o.z = (En + Es + Ew + Ee).z * 0.12405f;// *_expf(1.0f-4.0f*o.z);

    // grayscale of camera texture
    float vgb = g( B(f/R).xyz );
    // Adding waves of different magnitudes
    swi2(o,x,y) += 15.0f * iTimeDelta*cs(50.0f*vgb + 1.0f*(length(swi2(o,x,y)) ) ) ;
    swi2(o,x,y) += -30.0f* iTimeDelta*cs(20.0f*vgb + 7.0f*(length(swi2(o,x,y)) )  - (length(to_float2(Ee.z - Ew.z, Es.z - En.z))));
    swi2(o,x,y) += 20.0f * iTimeDelta*cs(70.0f*vgb + 15.0f*(length(swi2(o,x,y)) ) + 5.0f*(length(to_float2(Ee.z - Ew.z, Es.z - En.z))));
    
    // experimental other sets of values, these replace the last wave
    /*
    swi2(o,x,y) += 150.0f*iTimeDelta*cs(1.0f*g( B(f/R).xyz )+1.0f*1.0f*(length(swi2(o,x,y)) ) ) ;
    swi2(o,x,y) += 150.0f*iTimeDelta*cs(10.0f*g( B(f/R).xyz )+3.0f*1.0f*(length(swi2(o,x,y)) ) ) ;
  swi2(o,x,y) += 20.0f*iTimeDelta*cs(61.0f*g( B(f/R).xyz )+1.0f*10.0f*(length(swi2(o,x,y)) )+1.0f*6.0f*(length(to_float2(Ee.z - Ew.z, Es.z - En.z)))) ;
    */
    /*
    swi2(o,x,y) *= 1.709333f; // This artifact is too cool to be killed just like that, I want to make something out of it
  */
    #ifdef ORDER
    swi2(o,x,y) += to_float2(Ee.z - Ew.z, Es.z - En.z) *0.25f; // ORDER
  #else
    swi2(o,x,y) += to_float2(Ee.z - Ew.z, Es.z - En.z) *0.0125f; // 50% more CHAOS
    #endif

    o.z += (Es.y - En.y + Ee.x - Ew.x) *-0.99f;

    o = clamp(o, -1.0f, 1.0f); // This should not be needed but idk for sure that it's not needed
    // I should probably also clamp from -PI to PI but I like it as is


  SetFragmentShaderComputedColor(o);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: London' to iChannel1
// Connect Image 'Previsualization: Buffer A' to iChannel0


/*
** License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.
** Created by bal-khan
**
** This is rendering code
**
** BufferA contain Fluid Code with a define to switch on/off
**
*/

__KERNEL__ void FluidThingy4Jipi180Fuse(float4 o, float2 f, sampler2D iChannel0)
{

    float4 ret = texture(iChannel0, f/R);
  o = to_float4(swi3(ret,z,z,z)*1e2, 1.0f); // mult value by bignum 
    o = clamp(o, 0.0f, 1.0f); // clamp bigvalue to be in [0-1] range
    o -= 0.25f; // offset value
    o = _sinf(B(f/R)*-2.0f+length(swi3(o,x,y,z))*3.14f+1.57f*1.0f+0.250f*to_float4(0.0f,1.04f,2.08f,0.0f) ); // sinus coloring


  SetFragmentShaderComputedColor(o);
}