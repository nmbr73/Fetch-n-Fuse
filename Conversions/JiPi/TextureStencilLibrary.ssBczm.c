
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


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

#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

__DEVICE__ float4 texStencil(__TEXTURE2D__ ch, float2 uv, float coeff[9], float2 R) {
    float2 texel = 1.0f / R; //to_float2(textureSize(ch, 0));
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
__DEVICE__ float4 texBlur(__TEXTURE2D__ ch, float2 uv, float2 R) {
    
    float coeff[9] = { 0.0625f, 0.125f, 0.0625f,
                       0.125f,  0.25f,  0.125f,
                       0.0625f, 0.125f, 0.0625f };
    
    return texStencil(ch, uv, coeff,R);
}

// Laplacian, optimal 9-point stencil
// https://docs.lib.purdue.edu/cgi/viewcontent.cgi?article=1928&context=cstech
__DEVICE__ float4 texLapl(__TEXTURE2D__ ch, float2 uv, float2 R) {
    
    float coeff[9] = { 1.0f,   4.0f, 1.0f,
                       4.0f, -20.0f, 4.0f,
                       1.0f,   4.0f, 1.0f };
    
    return texStencil(ch, uv, coeff,R) / 6.0f;
}

// horizontal gradient (Sobel filter)
__DEVICE__ float4 texGradX(__TEXTURE2D__ ch, float2 uv, float2 R) {
   
   float coeff[9] = { -1.0f, 0.0f, 1.0f,
                      -2.0f, 0.0f, 2.0f,
                      -1.0f, 0.0f, 1.0f };

   return texStencil(ch, uv, coeff,R) / 8.0f;
}

// vertical gradient (Sobel filter)
__DEVICE__ float4 texGradY(__TEXTURE2D__ ch, float2 uv, float2 R) {
    
   float coeff[9] = { 1.0f,  2.0f,  1.0f,
                      0.0f,  0.0f,  0.0f,
                     -1.0f, -2.0f, -1.0f };   
    
    return texStencil(ch, uv, coeff,R) / 8.0f;
}




// IQ's simplex noise:

// The MIT License
// Copyright © 2013 Inigo Quilez
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

__DEVICE__ float2 hash( float2 p ) // replace this by something better
{
  p = to_float2( dot(p,to_float2(127.1f,311.7f)),
                 dot(p,to_float2(269.5f,183.3f)) );

  return -1.0f + 2.0f*fract_f2(sin_f2(p)*43758.5453123f);
}

__DEVICE__ float noise( in float2 p )
{
  const float K1 = 0.366025404f; // (_sqrtf(3)-1)/2;
  const float K2 = 0.211324865f; // (3-_sqrtf(3))/6;

  float2 i = _floor( p + (p.x+p.y)*K1 );
  
  float2 a = p - i + (i.x+i.y)*K2;
  float2 o = step(swi2(a,y,x),swi2(a,x,y));    
  float2 b = a - o + K2;
  float2 c = a - 1.0f + 2.0f*K2;

  float3 h = _fmaxf( 0.5f-to_float3(dot(a,a), dot(b,b), dot(c,c) ), to_float3_s(0.0f) );

  float3 n = h*h*h*h*to_float3( dot(a,hash(i+0.0f)), dot(b,hash(i+o)), dot(c,hash(i+1.0f)));

  return dot( n, to_float3_s(70.0f) );
}

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0


// Suture Fluid

__DEVICE__ float2 normz(float2 _x) {
  return (_x.x == 0.0f && _x.y == 0.0f) ? to_float2_s(0) : normalize(_x);
}

__DEVICE__ mat2 rotate2d(float a) {
    return to_mat2(_cosf(a),-_sinf(a),
                   _sinf(a),_cosf(a));
}

__KERNEL__ void TextureStencilLibraryFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_SLIDER9(Alpha, 0.0f, 1.0f, 1.0f);

    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(BlendX_Thr, 0.0f, 10.0f, 1.0f);
    CONNECT_SLIDER2(BlendY_Thr, 0.0f, 10.0f, 1.0f);
    CONNECT_SLIDER3(BlendZ_Thr, 0.0f, 10.0f, 1.0f);
    CONNECT_SLIDER4(BlendW_Thr, 0.0f, 10.0f, 1.0f);
    CONNECT_SLIDER5(BlendC_Thr, -1.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, X,  Y, Z, W, C);


    fragCoord+=0.5f; 
    float2 uv = fragCoord / iResolution;

    if (iFrame < 10 || Reset) {
        fragColor = to_float4(noise(16.0f * uv + 1.1f), noise(16.0f * uv + 2.2f), noise(16.0f * uv + 3.3f), 0);
        SetFragmentShaderComputedColor(fragColor);
        return;
    }

    float divergence = _tex2DVecN(iChannel0,uv.x,uv.y,15).z;
    divergence += 0.3f * texLapl(iChannel0, uv,R).z; // divergence smoothing
    divergence += 0.2f * (texGradX(iChannel0, uv,R).x + texGradY(iChannel0, uv,R).y); // divergence update

    float2 stepSize = 6.0f / iResolution;
    float2 velocity = swi2(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y);
    float2 advected = swi2(texBlur(iChannel0, uv - stepSize * velocity,R),x,y);
    advected += 0.05f*swi2(texLapl(iChannel0, uv,R),x,y);
    advected -= 0.05f*swi2(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y) * divergence;
    advected -= 0.80f*texLapl(iChannel0, uv,R).z  * normz(velocity);

    float curl = texGradX(iChannel0, uv,R).y - texGradY(iChannel0, uv,R).x;
    advected = mul_f2_mat2(advected,rotate2d(2.4f * curl));

    if (length(advected) > 1.0f) advected = normalize(advected);
    divergence = clamp(divergence, -1.0f, 1.0f);
    fragColor = to_float4_aw(_mix(swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z), to_float3_aw(advected, divergence), 0.2f), 0.0f); // update smoothing

    //Textureblending
    if (Blend1 > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(iChannel1,fragCoord/iResolution);

      if (tex.w > 0.0f)
      {
        //swi3S(Co,x,y,z, _mix(swi3(Co,x,y,z),(swi3(tex,x,y,z)*2.0f-0.5f)*Blend1_Thr, Blend1));       
        
        if ((int)Modus&2)
          fragColor.x = _mix(fragColor.x,(tex.x*2.0f-1.0f)*BlendX_Thr, Blend1);

        if ((int)Modus&4)
          fragColor.y = _mix(fragColor.y,(tex.y*2.0f-1.0f)*BlendY_Thr, Blend1);
        
        if ((int)Modus&8)
          fragColor.z = _mix(fragColor.z,(tex.z*1.0f-1.0f)*BlendZ_Thr, Blend1);
        
        if ((int)Modus&16)
          fragColor = _mix(fragColor,tex*BlendW_Thr, Blend1); //*2.0f-0.5f
        
        if ((int)Modus&32)
          fragColor.w = _mix(fragColor.w,0.0f+BlendC_Thr, Blend1);            
        
        
        //Q.w = 1.0;
        //V = _mix(V,swi2(tex,x,y)*Blend2_Thr,Blend1);
      }
    }

//fragColor = texture(iChannel1,fragCoord/iResolution);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


// Visualization of the system in Buffer A

__KERNEL__ void TextureStencilLibraryFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    fragCoord+=0.5f;
    float2 uv = fragCoord / iResolution;
    float3 norm = normalize(swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z));
    fragColor = to_float4_aw(0.5f + 0.6f * cross(norm, to_float3(0.5f,-0.4f,0.5f)) + 0.1f * norm.z,0.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer B' to iChannel0


// Library code is in the Common tab
// Demo of using it to reimplement Suture Fluid is in Buffer A

__KERNEL__ void TextureStencilLibraryFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    CONNECT_SLIDER9(Alpha, 0.0f, 1.0f, 1.0f);

    fragCoord+=0.5f;
    float2 uv = fragCoord / iResolution;
    // apply Sobel filter to Buffer B
    float4 dx = texGradX(iChannel0, uv,R);
    float4 dy = texGradY(iChannel0, uv,R);
    fragColor = 10.0f * sqrt_f4(dx*dx + dy*dy);

    fragColor.w = Alpha;

  SetFragmentShaderComputedColor(fragColor);
}