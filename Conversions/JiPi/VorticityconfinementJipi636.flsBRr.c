
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
   
   /* calculate s and _x */
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
   
   /* 3.0f return the sum of the four surflets */
   return dot(d, to_float4_s(52.0f));
}

/*****************************************************************************/


__DEVICE__ float2 pen(float t, float2 iResolution) {
    t *= 0.1f;
    return 5.0f * 0.5f * iResolution *
        to_float2(simplex3d(to_float3(t,0,0)) + 1.0f,
                  simplex3d(to_float3(0,t,0)) + 1.0f);
}


#define T(p) texture(iChannel0,(p)/iResolution)

//#define dt 0.15f
//#define K 0.1f
//#define nu 0.25f
//#define kappa 0.1f

__DEVICE__ float vorticity(float2 p, float2 R, __TEXTURE2D__ iChannel0) {
    float4 n = T(p + to_float2(0,1));
    float4 e = T(p + to_float2(1,0));
    float4 s = T(p - to_float2(0,1));
    float4 w = T(p - to_float2(1,0));
    float4 dx = (e - w)/2.0f;
    float4 dy = (n - s)/2.0f;
    return dx.y - dy.x;
}

__DEVICE__ float screendist2(float2 p, float2 q, float2 iResolution) {
    float2 r = mod_f2f2(p - q + iResolution/2.0f, iResolution) - iResolution/2.0f;
    return dot(r,r);
}

__KERNEL__ void VorticityconfinementJipi636Fuse__Buffer_A(float4 c, float2 p, float iTime, float2 iResolution, float iTimeDelta, int iFrame, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(Reset, 0);
    
    CONNECT_SLIDER1(dt, -10.0f, 10.0f, 0.15f);
    CONNECT_SLIDER2(K, -10.0f, 10.0f, 0.1f);
    CONNECT_SLIDER3(nu, -10.0f, 10.0f, 0.25f);
    CONNECT_SLIDER4(kappa, -10.0f, 10.0f, 0.10f);
    
   
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
    
    // MacCormack advection
    float2 q = p - dt*swi2(c,x,y);
    float2 r = q + dt*swi2(T(q),x,y);
    swi3S(c,x,y,w, swi3(T(q + (p - r)/2.0f),x,y,w));
    
    // semi-Langrangian advection
    //swi3(c,x,y,w) = T(q).xyw;
    
    // viscosity/diffusion
    swi3S(c,x,y,w, swi3(c,x,y,w) + dt * to_float3(nu,nu,kappa) * swi3(laplacian,x,y,w));
    
    // nullify divergence with pressure field gradient
    swi2S(c,x,y, swi2(c,x,y) - K * to_float2(dx.z,dy.z));

    // external source
    float2 m = pen(iTime, iResolution);
    float2 m0 = pen(iTime-0.015f, iResolution);
    float smoke = 100.0f * iTimeDelta * length(m - m0);
    swi3S(c,x,y,w, swi3(c,x,y,w) + dt * _expf(-screendist2(p,m, iResolution)/200.0f) * to_float3_aw(m - m0, smoke));
    
    // vorticity gradient
    float2 eta = to_float2(vorticity(p + to_float2(1,0),R,iChannel0) - vorticity(p - to_float2(1,0),R,iChannel0),
                           vorticity(p + to_float2(0,1),R,iChannel0) - vorticity(p - to_float2(0,1),R,iChannel0))/2.0f;
    if(length(eta) > 0.0f)
        swi2S(c,x,y, swi2(c,x,y) + dt * 3.0f * vorticity(p,R,iChannel0) * normalize(eta));
    
    // dissipation
    c.w -= dt * 0.1f * iTimeDelta;
    
    c = clamp(c, to_float4(-5,-5,0.5f,0), to_float4(5,5,3,1));

  SetFragmentShaderComputedColor(c);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void VorticityconfinementJipi636Fuse(float4 o, float2 p, float2 iResolution, sampler2D iChannel0)
{
    CONNECT_COLOR0(Color, 0.0f, 23.0f, 21.0f, 1.0f);
    CONNECT_SLIDER5(Par1, -10.0f, 10.0f, 0.10f);
    CONNECT_SLIDER6(Par2, -10.0f, 10.0f, 0.80f);
    CONNECT_SLIDER7(Par3, -10.0f, 10.0f, 5.0f);
    CONNECT_SLIDER8(Par4, -10.0f, 10.0f, 2.0f);
    //CONNECT_SLIDER9(Par2, -10.0f, 10.0f, 0.80f);
    
    CONNECT_SCREW0(Par5, -10.0f, 10.0f, 0.6f);
    CONNECT_SCREW1(Par6, -10.0f, 10.0f, 0.6f);
    CONNECT_SCREW2(Par7, -10.0f, 10.0f, 6.3f);
    CONNECT_SCREW3(Par8, -10.0f, 10.0f, 0.5f);

    float4 c = texture(iChannel0, p / iResolution);
//    float _z = (0.1f + 0.8f*c.w) * (1.0f + length(swi2(c,x,y))/5.0f)/2.0f;
    //o = to_float4_aw(_z * (0.6f + 0.6f * cos_f3(6.3f * (_z+0.5f) + to_float3(0,23,21))), 1.0f);
//    o = to_float4_aw(_z * (0.6f + 0.6f * cos_f3(6.3f * (_z+0.5f) + swi3(Color,x,y,z))), Color.w);
    
    float _z = (Par1 + Par2*c.w) * (1.0f + length(swi2(c,x,y))/Par3)/Par4;
    //o = to_float4_aw(_z * (0.6f + 0.6f * cos_f3(6.3f * (_z+0.5f) + to_float3(0,23,21))), 1.0f);
    o = to_float4_aw(_z * (Par5 + Par6 * cos_f3(Par7 * (_z+Par8) + swi3(Color,x,y,z))), Color.w);
    
    
    
  SetFragmentShaderComputedColor(o);
}