
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution

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
   float3 _x = p - s + dot(s, to_float3_s(G3));
   
   /* calculate i1 and i2 */
   float3 e = step(to_float3_s(0.0f), _x - swi3(_x,y,z,x));
   float3 i1 = e*(1.0f - swi3(e,z,x,y));
   float3 i2 = 1.0f - swi3(e,z,x,y)*(1.0f - e);
     
   /* x1, x2, x3 */
   float3 x1 = _x - i1 + G3;
   float3 x2 = _x - i2 + 2.0f*G3;
   float3 x3 = _x - 1.0f + 3.0f*G3;
   
   /* 2.0f find four surflets and store them in d */
   float4 w, d;
   
   /* calculate surflet weights */
   w.x = dot(_x, _x);
   w.y = dot(x1, x1);
   w.z = dot(x2, x2);
   w.w = dot(x3, x3);
   
   /* w fades from 0.6f at the center of the surflet to 0.0f at the margin */
   w = _fmaxf(to_float4_s(0.6f) - w, to_float4_s(0.0f));
   
   /* calculate surflet components */
   d.x = dot(random3(s), _x);
   d.y = dot(random3(s + i1), x1);
   d.z = dot(random3(s + i2), x2);
   d.w = dot(random3(s + 1.0f), x3);
   
   /* multiply d by w^4 */
   w *= w;
   w *= w;
   d *= w;
float tttttttttttttttttt;   
   /* 3.0f return the sum of the four surflets */
   return dot(d, to_float4_s(52.0f));
}

/*****************************************************************************/


__DEVICE__ float2 pen(float t, float2 iResolution) {
    t *= 0.05f;
    return 0.5f * iResolution *
        to_float2(simplex3d(to_float3(t,0,0)) + 1.0f,
                  simplex3d(to_float3(0,t,0)) + 1.0f);
}


#define T(p) texture(iChannel0,(p)/iResolution)
#define length2(p) dot(p,p)

//#define dt 0.15f
//#define K 0.2f
//#define nu 0.5f
//#define kappa 0.1f

__KERNEL__ void FluidsolverJipi721Fuse__Buffer_A(float4 c, float2 p, float iTime, float2 iResolution, int iFrame, float4 iMouse, sampler2D iChannel0)
{
  CONNECT_CHECKBOX0(Reset, 0);

    CONNECT_SLIDER1(dt, -10.0f, 10.0f, 0.15f);
    CONNECT_SLIDER2(K, -10.0f, 10.0f, 0.1f);
    CONNECT_SLIDER3(nu, -10.0f, 10.0f, 0.25f);
    CONNECT_SLIDER4(kappa, -10.0f, 10.0f, 0.10f);

  CONNECT_SCREW0(Penthickness, 0.0f, 30.0f, 10.0f);
  CONNECT_SCREW1(Par1, 0.0f, 100.0f, 50.0f);

  
    p+=0.5f;

    if(iFrame < 10 || Reset) {
        c = to_float4(0,0,1,0);
        SetFragmentShaderComputedColor(c);
        return;
    }
    
    c = T(p);

    float4 n = T(p + to_float2(0,1));
    float4 e = T(p + to_float2(1,0));
    float4 s = T(p - to_float2(0,1));
    float4 w = T(p - to_float2(1,0));
    
    float4 laplacian = (n + e + s + w - 4.0f*c);
    
    float4 dx = (e - w)/2.0f;
    float4 dy = (n - s)/2.0f;
    
    // velocity field divergence
    float div = dx.x + dy.y;
    
    // mass conservation, Euler method step
    c.z -= dt*(dx.z * c.x + dy.z * c.y + div * c.z);
    
    // semi-Langrangian advection
    //swi3(c,x,y,w) = T(p - dt*swi2(c,x,y)).xyw;
    c.x = T(p - dt*swi2(c,x,y)).x;
    c.y = T(p - dt*swi2(c,x,y)).y;
    c.w = T(p - dt*swi2(c,x,y)).w;
    
    
    // viscosity/diffusion
    //swi3(c,x,y,w) += dt * to_float3(nu,nu,kappa) * swi3(laplacian,x,y,w);
    c.x += dt * nu * laplacian.x;
    c.y += dt * nu * laplacian.y;
    c.w += dt * kappa * laplacian.w;
    
    // nullify divergence with pressure field gradient
    //swi2(c,x,y) -= K * to_float2(dx.z,dy.z);
    c.x -= K * dx.z;
    c.y -= K * dy.z;
    
    // external source
    float2 m = pen(iTime, iResolution);
    swi3S(c,x,y,w, swi3(c,x,y,w) + dt * _expf(-length2(p - m)/Par1) * to_float3_aw(m - pen(iTime-0.1, iResolution), 1));
    
#ifdef XXX    
    if (iMouse.z > 0.0f) {
      float2 d = p - swi2(iMouse,x,y);
      float m = _expf(-length(d) / 50.0f);
      //du += 0.2f * m * normz(d);
      c.w = m;
    }
#endif

    if(iMouse.z>0.0f && distance_f2(p,swi2(iMouse,x,y))<Penthickness) swi3S(c,x,y,w, swi3(c,x,y,w) + to_float3_aw(swi2(random3(to_float3_aw(p,0)),x,y), 1));   //c = to_float4_s(1.0f);    
    
    
    // dissipation
    c.w -= dt*0.0005f;
    
    c = clamp(c, to_float4(-5,-5,0.5f,0), to_float4(5,5,3,5));


  SetFragmentShaderComputedColor(c);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


#define PI 3.141592653589793

__KERNEL__ void FluidsolverJipi721Fuse(float4 o, float2 p, float2 iResolution, sampler2D iChannel0)
{
    CONNECT_COLOR0(Color, 0.0f, 23.0f, 21.0f, 1.0f);
    CONNECT_SLIDER5(Par1, -10.0f, 10.0f, 0.60f);
    CONNECT_SLIDER6(Par2, -10.0f, 10.0f, 0.60f);
    CONNECT_SLIDER7(Par3, -10.0f, 10.0f, 6.3f);
    CONNECT_SLIDER8(Par4, -10.0f, 10.0f, 5.0f);
    CONNECT_SLIDER9(Par5, -10.0f, 20.0f, 10.0f);
  
  
    p+=0.5f;

    float4 c = texture(iChannel0, p / iResolution);
    
    //float3 oxyz = 0.6f + 0.6f * cos_f3(6.3f * _atan2f(c.y,c.x)/(2.0f*PI) + to_float3(0,23,21)); // velocity
    float3 oxyz = Par1 + Par2 * cos_f3(Par3 * _atan2f(c.y,c.x)/(2.0f*PI) + swi3(Color,x,y,z)); // velocity
    
    
    oxyz *= c.w/Par4;//5.0f; // ink
    oxyz += clamp(c.z - 1.0f, 0.0f, 1.0f)/Par5;//10.0f; // local fluid density
    o = to_float4_aw(oxyz,Color.w);

  SetFragmentShaderComputedColor(o);
}