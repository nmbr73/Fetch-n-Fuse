
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

/*
Buffers B and C calculate fluid pressure using the two-pass Poisson solver described in
    "Fast Eulerian Fluid Simulation In Games Using Poisson Filters"
    https://www.shahinrabbani.ca/torch2pd.html

Solutions are initialised with a Gaussian blur of the previous solution
*/


__DEVICE__ float gaussian(float w, float s) {
    return _expf(-(w*w) / (2.0f*s*s)) / (s * _sqrtf(radians(360.0f)));
}


// Hash without Sine
// MIT License...
/* Copyright (c)2014 David Hoskins.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/

//----------------------------------------------------------------------------------------
//  1 out, 1 in...
__DEVICE__ float hash11(float p)
{
    p = fract(p * 0.1031f);
    p *= p + 33.33f;
    p *= p + p;
    return fract(p);
}

//----------------------------------------------------------------------------------------
//  1 out, 2 in...
__DEVICE__ float hash12(float2 p)
{
  float3 p3  = fract_f3((swi3(p,x,y,x)) * 0.1031f);
  p3 += dot(p3, swi3(p3,y,z,x) + 33.33f);
  return fract((p3.x + p3.y) * p3.z);
}

//----------------------------------------------------------------------------------------
//  1 out, 3 in...
__DEVICE__ float hash13(float3 p3)
{
  p3  = fract_f3(p3 * 0.1031f);
  p3 += dot(p3, swi3(p3,z,y,x) + 31.32f);
  return fract((p3.x + p3.y) * p3.z);
}

//----------------------------------------------------------------------------------------
//  2 out, 1 in...
__DEVICE__ float2 hash21(float p)
{
  float3 p3 = fract_f3(to_float3_s(p) * to_float3(0.1031f, 0.1030f, 0.0973f));
  p3 += dot(p3, swi3(p3,y,z,x) + 33.33f);
  return fract_f2((swi2(p3,x,x)+swi2(p3,y,z))*swi2(p3,z,y));
}

//----------------------------------------------------------------------------------------
///  2 out, 2 in...
__DEVICE__ float2 hash22(float2 p)
{
  float3 p3 = fract_f3((swi3(p,x,y,x)) * to_float3(0.1031f, 0.1030f, 0.0973f));
  p3 += dot(p3, swi3(p3,y,z,x)+33.33f);
  return fract_f2((swi2(p3,x,x)+swi2(p3,y,z))*swi2(p3,z,y));
}

//----------------------------------------------------------------------------------------
///  2 out, 3 in...
__DEVICE__ float2 hash23(float3 p3)
{
  p3 = fract_f3(p3 * to_float3(0.1031f, 0.1030f, 0.0973f));
  p3 += dot(p3, swi3(p3,y,z,x)+33.33f);
  return fract_f2((swi2(p3,x,x)+swi2(p3,y,z))*swi2(p3,z,y));
}

//----------------------------------------------------------------------------------------
//  3 out, 1 in...
__DEVICE__ float3 hash31(float p)
{
   float3 p3 = fract_f3(to_float3_s(p) * to_float3(0.1031f, 0.1030f, 0.0973f));
   p3 += dot(p3, swi3(p3,y,z,x)+33.33f);
   return fract_f3((swi3(p3,x,x,y)+swi3(p3,y,z,z))*swi3(p3,z,y,x)); 
}


//----------------------------------------------------------------------------------------
///  3 out, 2 in...
__DEVICE__ float3 hash32(float2 p)
{
  float3 p3 = fract_f3((swi3(p,x,y,x)) * to_float3(0.1031f, 0.1030f, 0.0973f));
  p3 += dot(p3, swi3(p3,y,x,z)+33.33f);
  return fract_f3((swi3(p3,x,x,y)+swi3(p3,y,z,z))*swi3(p3,z,y,x));
}

//----------------------------------------------------------------------------------------
///  3 out, 3 in...
__DEVICE__ float3 hash33(float3 p3)
{
  p3 = fract(p3 * to_float3(0.1031f, 0.1030f, 0.0973f));
  p3 += dot(p3, swi3(p3,y,x,z)+33.33f);
  return fract((swi3(p3,x,x,y) + swi3(p3,y,x,x))*swi3(p3,z,y,x));
}

//----------------------------------------------------------------------------------------
// 4 out, 1 in...
__DEVICE__ float4 hash41(float p)
{
  float4 p4 = fract_f4(to_float4_s(p) * to_float4(0.1031f, 0.1030f, 0.0973f, 0.1099f));
  p4 += dot(p4, swi4(p4,w,z,x,y)+33.33f);
  return fract_f4((swi4(p4,x,x,y,z)+swi4(p4,y,z,z,w))*swi4(p4,z,y,w,x));
}

//----------------------------------------------------------------------------------------
// 4 out, 2 in...
__DEVICE__ float4 hash42(float2 p)
{
  float4 p4 = fract_f4((swi4(p,x,y,x,y)) * to_float4(0.1031f, 0.1030f, 0.0973f, 0.1099f));
  p4 += dot(p4, swi4(p4,w,z,x,y)+33.33f);
  return fract_f4((swi4(p4,x,x,y,z)+swi4(p4,y,z,z,w))*swi4(p4,z,y,w,x));
}

//----------------------------------------------------------------------------------------
// 4 out, 3 in...
__DEVICE__ float4 hash43(float3 p)
{
  float4 p4 = fract_f4((swi4(p,x,y,z,x))  * to_float4(0.1031f, 0.1030f, 0.0973f, 0.1099f));
  p4 += dot(p4, swi4(p4,w,z,x,y)+33.33f);
  return fract_f4((swi4(p4,x,x,y,z)+swi4(p4,y,z,z,w))*swi4(p4,z,y,w,x));
}

//----------------------------------------------------------------------------------------
// 4 out, 4 in...
__DEVICE__ float4 hash44(float4 p4)
{
  p4 = fract_f4(p4  * to_float4(0.1031f, 0.1030f, 0.0973f, 0.1099f));
  p4 += dot(p4, swi4(p4,w,z,x,y)+33.33f);
  return fract_f4((swi4(p4,x,x,y,z)+swi4(p4,y,z,z,w))*swi4(p4,z,y,w,x));
}



/* https://www.shadertoy.com/view/XsX3zB
 *
 * The MIT License
 * Copyright © 2013 Nikita Miropolskiy
 * 
 * ( license has been changed from CCA-NC-SA 3.0f to MIT
 *
 *   but thanks for attributing your source code when deriving from this sample 
 *   with a following link: https://www.shadertoy.com/view/XsX3zB )
 *
 * ~
 * ~ if you're looking for procedural noise implementation examples you might 
 * ~ also want to look at the following shaders:
 * ~ 
 * ~ Noise Lab shader by candycat: https://www.shadertoy.com/view/4sc3z2
 * ~
 * ~ Noise shaders by iq:
 * ~     Value    Noise 2D, Derivatives: https://www.shadertoy.com/view/4dXBRH
 * ~     Gradient Noise 2D, Derivatives: https://www.shadertoy.com/view/XdXBRH
 * ~     Value    Noise 3D, Derivatives: https://www.shadertoy.com/view/XsXfRH
 * ~     Gradient Noise 3D, Derivatives: https://www.shadertoy.com/view/4dffRH
 * ~     Value    Noise 2D             : https://www.shadertoy.com/view/lsf3WH
 * ~     Value    Noise 3D             : https://www.shadertoy.com/view/4sfGzS
 * ~     Gradient Noise 2D             : https://www.shadertoy.com/view/XdXGW8
 * ~     Gradient Noise 3D             : https://www.shadertoy.com/view/Xsl3Dl
 * ~     Simplex  Noise 2D             : https://www.shadertoy.com/view/Msf3WH
 * ~     Voronoise: https://www.shadertoy.com/view/Xd23Dh
 * ~ 
 *
 */

/* skew constants for 3d simplex functions */


/* 3d simplex noise */
__DEVICE__ float simplex3d(float3 p) {
 
   const float F3 = 1.0f/3.0f;
   const float G3 = 1.0f/6.0f; 
  
   /* 1.0f find current tetrahedron T and it's four vertices */
   /* s, s+i1, s+i2, s+1.0f - absolute skewed (integer) coordinates of T vertices */
   /* x, x1, x2, x3 - unskewed coordinates of p relative to each of T vertices*/
   
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
   d.x = dot(hash33(s) - 0.5f, x);
   d.y = dot(hash33(s + i1) - 0.5f, x1);
   d.z = dot(hash33(s + i2) - 0.5f, x2);
   d.w = dot(hash33(s + 1.0f) - 0.5f, x3);
   
   /* multiply d by w^4 */
   w *= w;
   w *= w;
   d *= w;
   
   /* 3.0f return the sum of the four surflets */
   return dot(d, to_float4_s(52.0f));
}


// https://iquilezles.org/articles/fbm

__DEVICE__ float fbm( in float3 x, in float G, in int numOctaves )
{    
    float f = 1.0f;
    float a = 1.0f;
    float t = 0.0f;
    for( int i=0; i<numOctaves; i++ )
    {
        t += a*simplex3d(f*x);
        f *= 2.0f;
        a *= G;
    }
    return t;
}


/* Texture Stencil Library https://www.shadertoy.com/view/ssBczm

The MIT License

Copyright (c) 2022 David A Roberts <https://davidar.io/>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

__DEVICE__ float4 texStencil(__TEXTURE2D__ ch, float2 uv, float coeff[9], float2 iResolution) {
    //float2 texel = 1.0f / to_float2(textureSize(ch, 0));
    float2 texel = 1.0f / iResolution;
    const float2 stencilOffset[9] = {
                              to_float2(-1, 1), to_float2( 0, 1), to_float2( 1, 1),
                              to_float2(-1, 0), to_float2( 0, 0), to_float2( 1, 0),
                              to_float2(-1,-1), to_float2( 0,-1), to_float2( 1,-1)
                              };
    float4 r = to_float4_s(0);
    for (int i = 0; i < 9; i++)
        r += coeff[i] * texture(ch, uv + texel * stencilOffset[i]);
    return r;
}

// Gaussian/binomial blur
// https://bartwronski.com/2021/10/31/practical-gaussian-filter-binomial-filter-and-small-sigma-gaussians/
__DEVICE__ float4 texBlur(__TEXTURE2D__ ch, float2 uv, float2 iResolution) {
   
    float par[] = {
                  0.0625f, 0.125f, 0.0625f,
                  0.125f,  0.25f,  0.125f,
                  0.0625f, 0.125f, 0.0625f
                  };
    
    return texStencil(ch, uv, par,iResolution);
}

// Laplacian, optimal 9-point stencil
// https://docs.lib.purdue.edu/cgi/viewcontent.cgi?article=1928&context=cstech
__DEVICE__ float4 texLapl(__TEXTURE2D__ ch, float2 uv, float2 iResolution) {
    
    float par[] = { 
                  1.0f,   4.0f, 1.0f,
                  4.0f, -20.0f, 4.0f,
                  1.0f,   4.0f, 1.0f
                  };
    
    return texStencil(ch, uv, par,iResolution) / 6.0f;
}

// horizontal gradient (Sobel filter)
__DEVICE__ float4 texGradX(__TEXTURE2D__ ch, float2 uv, float2 iResolution) {
    
    float par[] = {
                  -1.0f, 0.0f, 1.0f,
                  -2.0f, 0.0f, 2.0f,
                  -1.0f, 0.0f, 1.0f
                  };
    
    return texStencil(ch, uv, par, iResolution) / 8.0f;
}

// vertical gradient (Sobel filter)
__DEVICE__ float4 texGradY(__TEXTURE2D__ ch, float2 uv, float2 iResolution) {
    
    float par[] = {
                   1.0f,  2.0f,  1.0f,
                   0.0f,  0.0f,  0.0f,
                  -1.0f, -2.0f, -1.0f
                  };
    
    return texStencil(ch, uv, par,iResolution) / 8.0f;
}

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer C' to iChannel2
// Connect Buffer A 'Previsualization: Buffer D' to iChannel3
// Connect Buffer A 'Texture: Blending' to iChannel1

__DEVICE__ void interaction(in float2 uv, inout float2 *velocity, inout float *ink, float iTime, float2 iResolution, float4 iMouse, __TEXTURE2D__ iChannel3) {
    // this function describes any external forces on the fluid, for example:

    float3 seed = to_float3_aw(uv, iTime / 10.0f);
    *velocity += 0.05f * fbm(20.0f * seed, 0.5f, 8);
    *velocity *= 0.99f;
    *ink += (fbm(seed, 0.5f, 8) + 1.0f) * 7e-3;
    *ink = clamp(0.99f * *ink, 0.0f, 1.0f);

    if (iMouse.z > 0.0f) {
        float2 p = swi2(iMouse,x,y) / iResolution;
        //float4 iMousePrev = texelFetch(iChannel3, to_int2(0,0), 0);
        float4 iMousePrev = texture(iChannel3, (make_float2(to_int2(0,0))+0.5f)/iResolution);
        float2 v = swi2(iMouse,x,y) - swi2(iMousePrev,x,y);
        if (length(v) > 1.0f) v = normalize(v);
        *velocity += v * _expf(-1000.0f * dot(uv-p, uv-p));
    }
}

__DEVICE__ float3 fetch(float2 uv, float2 iResolution, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel2) {
    // subtract pressure gradient from velocity
    return to_float3(
                    _tex2DVecN(iChannel0,uv.x,uv.y,15).x - texGradX(iChannel2, uv,iResolution).x,
                    _tex2DVecN(iChannel0,uv.x,uv.y,15).y - texGradY(iChannel2, uv,iResolution).x,
                    _tex2DVecN(iChannel0,uv.x,uv.y,15).z);
}

__DEVICE__ float3 advect(float2 uv, float2 iResolution, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel2) {
    //return fetch(uv - fetch(uv).xy / iResolution.xy);
    return fetch(uv - swi2(fetch(uv,iResolution,iChannel0,iChannel2),x,y) / iResolution,iResolution,iChannel0,iChannel2);
}

__KERNEL__ void SimpleIncompressibleFluidFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, int iFrame, float4 iMouse, sampler2D iChannel0, sampler2D iChannel2, sampler2D iChannel3)
{
    CONNECT_CHECKBOX0(Reset, 0); 
        //Blending
    CONNECT_SLIDER5(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER6(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER7(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Ink,  Velo, Mass, InvMass, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);
  
    fragCoord+=0.5f;

    float2 uv = fragCoord / iResolution;
    float3 r = advect(uv,iResolution,iChannel0,iChannel2);

    float2 velocity = swi2(r,x,y);
    float ink       = r.z;
    
    if(Blend1>0.0f)
    {
      float4 tex = texture(iChannel1, uv);
      
      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2) //Ink
        {
          ink = _mix(ink, tex.x*Blend1Mul+Blend1Off, Blend1);
        }  
        if ((int)Modus&4) // Geschwindigkeit
        {
          velocity = _mix(velocity, swi2(tex,x,y)*Blend1Mul+Blend1Off, Blend1);
        }
        
        if ((int)Modus&8) // Geschwindigkeit mit Parameter
          velocity = _mix(velocity, Par1, Blend1);

      }
      else
      {
        if ((int)Modus&16) 
          ink = _mix(ink, tex.x*Blend1Mul+Blend1Off, Blend1);
      
        if ((int)Modus&32) //Special
        {
          velocity = _mix(velocity, Par1, Blend1);
        }  
      }
    }
    
    interaction(uv, &velocity, &ink, iTime, iResolution, iMouse, iChannel3);
    //fragColor = to_float4_aw(r, 1.0f);
    fragColor = to_float4(velocity.x,velocity.y,ink, 1.0f);
    
    if(iFrame<1 || Reset)  fragColor = to_float4_s(0.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer C' to iChannel2


// Poisson solver - horizontal pass

__DEVICE__ float rhs(float2 pos, float2 iResolution, __TEXTURE2D__ iChannel0) { // rhs of the poisson equation
    float2 uv = pos / iResolution;
    float divergence = texGradX(iChannel0, uv,iResolution).x + texGradY(iChannel0, uv,iResolution).y;
    return -divergence;
}

__KERNEL__ void SimpleIncompressibleFluidFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel2)
{
    fragCoord+=0.5f;

    // https://youtu.be/_3eyPUyqluc?t=355
    const float poisson_filter[7] = {
        0.57843719174f,
        0.36519596949f,
        0.23187988879f,
        0.14529589353f,
        0.08816487385f,
        0.05184872885f,
        0.02906462467f
        };


    float p = 0.0f, g = 0.0f;
    for (int i = -6; i <= 6; i++) {
        float2 pos = fragCoord + to_float2(i,0);
        p += poisson_filter[(int)_fabs(i)] * rhs(pos, iResolution, iChannel0);
        //g += gaussian((float)(i), 3.0f) * texelFetch(iChannel2, to_int2_cfloat(pos), 0).x;
        g += gaussian((float)(i), 3.0f) * texture(iChannel2, (make_float2(to_int2_cfloat(pos))+0.5f)/iResolution).x;
    }
    fragColor = to_float4(p, g, 0, 0);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1


// Poisson solver - vertical pass

__KERNEL__ void SimpleIncompressibleFluidFuse__Buffer_C(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel1)
{
    fragCoord+=0.5f;

    // https://youtu.be/_3eyPUyqluc?t=355
    const float poisson_filter[7] = {
        0.57843719174f,
        0.36519596949f,
        0.23187988879f,
        0.14529589353f,
        0.08816487385f,
        0.05184872885f,
        0.02906462467f
        };


    float p = 0.0f, g = 0.0f;
    for (int j = -6; j <= 6; j++) {
        //float4 pass1 = texelFetch(iChannel1, to_int2(fragCoord) + to_int2(0,j), 0);
        float4 pass1 = texture(iChannel1, (make_float2( to_int2_cfloat(fragCoord) + to_int2(0,j))+0.5f)/iResolution);
        p += poisson_filter[(int)_fabs(j)] * pass1.x;
        g += gaussian((float)(j), 3.0f) * pass1.y;
    }
    fragColor = to_float4_s(g + p);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------


__KERNEL__ void SimpleIncompressibleFluidFuse__Buffer_D(float4 fragColor, float2 fragCoord, float4 iMouse)
{
    fragCoord+=0.5f;

    fragColor = iMouse;

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer C' to iChannel2


__DEVICE__ float3 pal( in float t, in float3 a, in float3 b, in float3 c, in float3 d ) {
    // https://iquilezles.org/articles/palettes
    return a + b*cos_f3( 6.28318f*(c*t+d) );
}

__KERNEL__ void SimpleIncompressibleFluidFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel2)
{
    fragCoord+=0.5f;

    // Normalized pixel coordinates (from 0 to 1)
    float2 uv = fragCoord/iResolution;

    // Time varying pixel color
    float3 col = 0.5f + 0.5f*cos_f3(iTime+swi3(uv,x,y,x)+to_float3(0,2,4));
    
    float d = _tex2DVecN(iChannel0,uv.x,uv.y,15).z;
    //d = _mix(-1.0f, 1.0f, d);
    
    col = pal( d, to_float3(0.5f,0.5f,0.5f),to_float3(0.5f,0.5f,0.5f),to_float3(1.0f,1.0f,1.0f),to_float3(0.0f,0.10f,0.20f) );

    // Output to screen
    fragColor = to_float4_aw(col, 1.0f);
    //fragColor = _tex2DVecN(iChannel2,uv.x,uv.y,15);

  SetFragmentShaderComputedColor(fragColor);
}