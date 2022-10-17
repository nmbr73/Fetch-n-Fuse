
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


// Port of Humus Electro demo http://humus.name/index.php?page=3D&ID=35
// Not exactly right as the noise is wrong, but is the closest I could make it.
// Uses Simplex noise by Nikita Miropolskiy https://www.shadertoy.com/view/XsX3zB

/* Simplex code license
 * This work is licensed under a 
 * Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License
 * http://creativecommons.org/licenses/by-nc-sa/3.0f/
 *  - You must attribute the work in the source code 
 *    (link to https://www.shadertoy.com/view/XsX3zB).
 *  - You may not use this work for commercial purposes.
 *  - You may distribute a derivative work only under the same license.
 */


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
   
   /* skew constants for 3d simplex functions */
   const float F3 =  0.3333333f;
   const float G3 =  0.1666667f;
   
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

__DEVICE__ float noise(float3 m) {
    return   0.5333333f*simplex3d(m)
            +0.2666667f*simplex3d(2.0f*m)
            +0.1333333f*simplex3d(4.0f*m)
            +0.0666667f*simplex3d(8.0f*m);
}

__KERNEL__ void ElectroJipi968Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{
    CONNECT_CHECKBOX1(Invers, 0);
    CONNECT_CHECKBOX2(ApplyColor, 0);
    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f); 
    CONNECT_SLIDER0(AlphaThres, 0.0f, 1.0f, 0.1f);

  float2 uv = fragCoord / iResolution;    
  uv = uv * 2.0f -1.0f;  
 
  float2 p = fragCoord/iResolution.x;
  float3 p3 = to_float3_aw(p, iTime*0.4f);    
    
  float intensity = noise((p3*12.0f+12.0f));
                          
  float t = clamp((uv.x * -uv.x * 0.16f) + 0.15f, 0.0f, 1.0f);                         
  float y = _fabs(intensity * -t + uv.y);
    
  float g = _powf(y, 0.2f);
                          
  float3 col = to_float3(1.70f, 1.48f, 1.78f);
  
  col = col * -g + col;                    
  col = col * col;
  col = col * col;
                          
  fragColor = to_float4_aw(col,1.0f);
  
    if (Invers) fragColor = to_float4_s(1.0f) - fragColor;
    if (ApplyColor)
    {
      if (fragColor.x <= AlphaThres)      fragColor.w = Color.w;  
      fragColor = (fragColor + (Color-0.5f))*fragColor.w;
    }

  
  SetFragmentShaderComputedColor(fragColor);
}