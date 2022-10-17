
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: RGBA Noise Small' to iChannel1
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0


/******** 3d simplex noise from https://www.shadertoy.com/view/XsX3zB ********/

/* discontinuous pseudorandom uniformly distributed in [-0.5f, +0.5]^3 */
__DEVICE__ float3 random3(float3 c) {
  float j = 4096.0f*_sinf(dot(c,to_float3(17.0f, 59.4f, 15.0f)));
  float3 r;
  r.z = fract(512.0f*j);
  j *= 0.125f;
  r.x = fract(512.0f*j);
  j *= 0.125f;
  r.y = fract(512.0f*j);
  return r-0.5f;
}



/* 3d simplex noise */
__DEVICE__ float simplex3d(float3 p) {
   /* 1.0f find current tetrahedron T and it's four vertices */
   /* s, s+i1, s+i2, s+1.0f - absolute skewed (integer) coordinates of T vertices */
   /* x, x1, x2, x3 - unskewed coordinates of p relative to each of T vertices*/
   
   /* skew constants for 3d simplex functions */
   const float F3 =  0.3333333f;
   const float G3 =  0.1666667f;
   
   
   /* calculate s and x */
   float3 s = _floor(p + dot(p, to_float3_s(F3)));
   float3 x = p - s + dot(s, to_float3_s(G3));
   
   /* calculate i1 and i2 */
   float3 e = step(to_float3_s(0.0f), x - swi3(x,y,z,x));
   float3 i1 = e*(1.0f - swi3(e,z,x,y));
   float3 i2 = 1.0f - swi3(e,z,x,y)*(1.0f - e);
     
   /* x1, x2, x3 */
   float3 x1 = x - i1 + G3;
   float3 x2 = x - i2 + 2.0f*G3;
   float3 x3 = x - 1.0f + 3.0f*G3;
   
   /* 2.0f find four surflets and store them in d */
   float4 w, d;
   
   /* calculate surflet weights */
   w.x = dot(x, x);
   w.y = dot(x1, x1);
   w.z = dot(x2, x2);
   w.w = dot(x3, x3);
   
   /* w fades from 0.6f at the center of the surflet to 0.0f at the margin */
   w = _fmaxf(to_float4_s(0.6f) - w, to_float4_s(0.0f));
   
   /* calculate surflet components */
   d.x = dot(random3(s), x);
   d.y = dot(random3(s + i1), x1);
   d.z = dot(random3(s + i2), x2);
   d.w = dot(random3(s + 1.0f), x3);

   /* multiply d by w^4 */
   w *= w;
   w *= w;
   d *= w;
   
   /* 3.0f return the sum of the four surflets */
   return dot(d, to_float4_s(52.0f));
}

/*****************************************************************************/


__DEVICE__ float2 pen(float t,float2 iResolution) {
    //t *= 0.05f;
    return 0.5f * iResolution *
        to_float2(simplex3d(to_float3(t,0,0)) + 1.0f,
                  simplex3d(to_float3(0,t,0)) + 1.0f);
}

__DEVICE__ float4 sample_tex(float2 loc,float2 iResolution, __TEXTURE2D__ iChannel0)
{
    float2 uv = swi2(loc,x,y) / iResolution;
    uv = clamp(uv, 0.0f, 1.0f);
    
    return _tex2DVecN(iChannel0,uv.x,uv.y,15);
}


//#define T(p) texture(iChannel0, clamp((p)/iResolution, 0.0f, 1.0f))
#define length2(p) dot(p,p)

#define dt 0.15
#define K 0.2
#define nu 0.5
#define kappa 0.1

#define swi4S(a,b,c,d,e,f) {float4 tmp = f; (a).b = tmp.x; (a).c = tmp.y; (a).d = tmp.z; (a).e = tmp.w;}

__KERNEL__ void PollutionFuse__Buffer_A(float4 c, float2 p, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{


    CONNECT_CHECKBOX0(Clear, 0.0f); //Clear all
    CONNECT_CHECKBOX1(InvertYps, 0.0f); //Invert
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_BUTTON0(Modus, 1, Icks, Yps, Zet, Weh, PunchOut);

  if (Clear)
  {
      SetFragmentShaderComputedColor(to_float4_s(0.0f));
      return;
  }    
    


    p+=0.5f;
    const float FlowMapNormalStrength = 0.5f;
    const float DisturbStrength = 1000.0f;

    float2 uv = p / iResolution;
    
    c = sample_tex(p,iResolution,iChannel0);
    
    float4 n = sample_tex(p + to_float2(0,1),iResolution,iChannel0);
    float4 e = sample_tex(p + to_float2(1,0),iResolution,iChannel0);
    float4 s = sample_tex(p - to_float2(0,1),iResolution,iChannel0);
    float4 w = sample_tex(p - to_float2(1,0),iResolution,iChannel0);
    
    float4 laplacian = (n + e + s + w - 4.0f*c);
    
    float4 dx = (e - w)/2.0f;
    float4 dy = (n - s)/2.0f;
    
    // velocity field divergence
    float div = dx.x + dy.y;
    
    // mass conservation, Euler method step
    c.z -= dt*(dx.z * c.x + dy.z * c.y + div * c.z);
    
    // semi-Langrangian advection
    swi3S(c,x,y,w, swi3(sample_tex(p - dt*swi2(c,x,y),iResolution,iChannel0),x,y,w));
    
    // viscosity/diffusion
    swi3S(c,x,y,w, swi3(c,x,y,w) + dt * to_float3(nu,nu,kappa) * swi3(laplacian,x,y,w));
    
    // nullify divergence with pressure field gradient
    swi2S(c,x,y, swi2(c,x,y) - K * to_float2(dx.z,dy.z));

    // external source
    if (iMouse.z > 0.0f)
    {
        float2 m = swi2(iMouse,x,y);
        float2 disturb = normalize(swi2(_tex2DVecN(iChannel1,uv.x,uv.y,15),x,y) - 0.5f) * DisturbStrength;
        float2 random = pen(iTime,iResolution);
        disturb = _mix(random, disturb, FlowMapNormalStrength);
        swi3S(c,x,y,w, swi3(c,x,y,w) + dt * _expf(-length2(p - m)/50.0f) * to_float3_aw(m - disturb, 1));
    }
  
  // TExturblending  
  if (Blend1>0.0f)
  {
    float4 tex = _tex2DVecN(iChannel2,uv.x,uv.y,15);
    
    if (tex.w != 0.0f)    
    {
      tex = tex*10.0 - 5.0f;
      if ((int)Modus & 2)  c.x = _mix(c.x,tex.x,Blend1);
      if ((int)Modus & 4 && InvertYps == 0)  c.y = _mix(c.y,tex.y,Blend1);
      if ((int)Modus & 8)  c.z = _mix(c.z,(tex.x+5.0f)/3.0f,Blend1);
      if ((int)Modus & 16) c.w = _mix(c.w,(tex.x+5.0f)/2.0f,Blend1);
      if ((int)Modus & 32) c = to_float4(0.0f,0.0f,0.0f,0.0f);
    }
    else
    { 
      if ((int)Modus & 4 && InvertYps == 1) c.w = _mix(c.w,5.0f,Blend1);//c = _mix(c,tex,Blend1);
    }
    
  } 
    

    // dissipation
    c.w -= dt*0.0005f;
    
    //swi4S(c,x,y,z,w, clamp(swi4(c,x,y,z,w), to_float4(-5,-5,0.5f,0), to_float4(5,5,3,5)));
    c = clamp(c, to_float4(-5,-5,0.5f,0), to_float4(5,5,3,5));

  SetFragmentShaderComputedColor(c);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: RGBA Noise Small' to iChannel1
// Connect Image 'Previsualization: Buffer A' to iChannel0


#define PI 3.141592653589793

__DEVICE__ float3 desaturate(float3 color)
{
  float3 lum = to_float3(0.299f, 0.587f, 0.114f);
  return to_float3_s(dot(lum, color));
}

__KERNEL__ void PollutionFuse(float4 o, float2 p, float2 iResolution, sampler2D iChannel0)
{
  
    p+=0.5f;
    float4 c = _tex2DVecN(iChannel0, p.x / iResolution.x, p.y / iResolution.y, 15);
    float3 _o = 0.6f + 0.6f * cos_f3(6.3f * _atan2f(c.y,c.x)/(2.0f*PI) + to_float3(0,23,21)); // velocity
    _o *= c.w/5.0f; // ink
    _o += clamp(c.z - 1.0f, 0.0f, 1.0f)/10.0f; // local fluid density
    o = to_float4_aw(desaturate(_o), 1.0f);
    
  SetFragmentShaderComputedColor(o);
}