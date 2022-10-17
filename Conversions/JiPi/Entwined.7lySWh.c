
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


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

__DEVICE__ float4 texStencil(__TEXTURE2D__ ch, float2 uv, float coeff[9], float2 R) {
    float2 texel = 1.0f / R;//to_float2(textureSize(ch, 0));
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
    #define ORG
    #ifdef ORG
    float par_ary[9] =  {
        0.0625f, 0.125f, 0.0625f,
        0.125f,  0.25f,  0.125f,
        0.0625f, 0.125f, 0.0625f 
        };
    #else    
    float par_ary[9] =  {
        0.0325f, 0.0125f, 0.0325f,
        0.9125f,  0.025f,  0.9125f,
        0.0325f, 0.0125f, 0.0325f 
        };        
    #endif    
    return texStencil(ch, uv, par_ary, R);
}

// Laplacian, optimal 9-point stencil
// https://docs.lib.purdue.edu/cgi/viewcontent.cgi?article=1928&context=cstech
__DEVICE__ float4 texLapl(__TEXTURE2D__ ch, float2 uv, float2 R) {
    float par_ary[9] =  {
        1.0f,   4.0f, 1.0f,
        4.0f, -20.0f, 4.0f,
        1.0f,   4.0f, 1.0f
        };
    return texStencil(ch, uv, par_ary,R) / 6.0f;
}

// horizontal gradient (Sobel filter)
__DEVICE__ float4 texGradX(__TEXTURE2D__ ch, float2 uv, float2 R) {
    float par_ary[9] =  {    
        -1.0f, 0.0f, 1.0f,
        -2.0f, 0.0f, 2.0f,
        -1.0f, 0.0f, 1.0f
        };
    return texStencil(ch, uv, par_ary, R) / 8.0f;
}

// vertical gradient (Sobel filter)
__DEVICE__ float4 texGradY(__TEXTURE2D__ ch, float2 uv, float2 R) {
    
    float par_ary[9] =  {
         1.0f,  2.0f,  1.0f,
         0.0f,  0.0f,  0.0f,
        -1.0f, -2.0f, -1.0f
        };
    return texStencil(ch, uv, par_ary,R) / 8.0f;
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0


__DEVICE__ float4 _char(float2 p, int c, __TEXTURE2D__ iChannel2) {
    if (p.x < 0.0f || p.x > 1.0f || p.y < 0.0f|| p.y > 1.0f) return to_float4(0,0,0,1);
    return texture(iChannel2, p/16.0f + fract_f2(to_float2(c, 15-c/16)/16.0f));
}

__KERNEL__ void EntwinedFuse__Buffer_A(float4 r, float2 u, float iTime, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel2)
{
    u+=0.5f;
    float2 i = u-u; r -= r;
    int z = 15;
    for(int k = (2*z+1)*(2*z+1); k-->0;) {
        i = to_float2(k%(2*z+1),k/(2*z+1)) - (float)(z);
        float q = _mix(0.015f, 0.06f, smoothstep(0.0f, 16e-3 * iResolution.y, iTime));
        //r += 0.5f * texelFetch(iChannel0,to_int2(i+u),0) * (1.0f - q*dot(i,i))*_expf(-q*dot(i,i));
        r += 0.5f * texture(iChannel0, (make_float2(to_int2_cfloat(i+u))+0.5f)/R) * (1.0f - q*dot(i,i))*_expf(-q*dot(i,i));
    }

    float2 uv = u/iResolution;
    r = clamp(r,0.0f,1.0f);
    if (iFrame < 9) {
        if (u.x < 0.25f * iResolution.x) r.x = 1.0f;
        if (u.x > 0.75f * iResolution.x) r.y = 1.0f;
        //if (u.y < 0.05f * iResolution.y) r.z = 1.0f;
    }
    
    float2 p = to_float2(3.7f,2.7f) * (uv - to_float2(0.5f,0.38f));
    float heart = distance_f2(p, to_float2(0, _sqrtf(_fabs(p.x))));
    if(heart > 1.0f) r = to_float4_s(0);

  SetFragmentShaderComputedColor(r);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Texture: Blue Noise' to iChannel1


__KERNEL__ void EntwinedFuse(float4 r, float2 u, float iTime, float2 iResolution, float3 iChannelResolution[], sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_COLOR0(Color1, 0.0f, 0.35f, 0.85f, 1.0f);
    CONNECT_COLOR1(Color2, 1.0f, 0.8f, 0.0f, 1.0f);
  
    u+=0.5f;
    float2 uv = u / iResolution;
    
    // edge detection
    float4 dx = texGradX(iChannel0, uv,R);
    float4 dy = texGradY(iChannel0, uv,R);
    r = sqrt_f4(dx*dx + dy*dy);
    
    // layering
    float4 mask = texBlur(iChannel0, uv,R);
    float blend = smoothstep(0.45f, 0.55f, uv.x);
    swi2S(r,x,y, swi2(r,x,y) * swi2(mask,x,y) - to_float2(1.0f - blend, blend) * swi2(mask,y,x));



    // colours
    r = to_float4_s(1.0f) - clamp(2.0f * r, 0.0f, 1.0f);
#define ORG2
#ifdef ORG2

    float4 coldeb = r;

    //r.z = r.y;

    r.y -= 0.5f - 0.5f * r.x;
    //r.z -= 0.5f - 0.5f * r.x;
    
    //r.y = 1.0f;


if (r.x == 1.0f && r.y < 0.95f && r.z == 1.0f) r = Color2;

#else
float4 r_tmp = r;    
    if (r.x    <0.95f) r = Color1-r.x;
    if (r_tmp.y<0.95f) r = Color2-r_tmp.y;
#endif    
    
    // paper
    r -= 0.05f * texture(iChannel1, 0.5f * u / R).x;
    
    // shadow
    float shadow = 0.1f * length(texBlur(iChannel0, (u + to_float2(-5, 5)) / iResolution,R));
    shadow *= smoothstep(0.0f, 4e-3 * iResolution.y, iTime);
    shadow *= 1.0f - clamp(mask.x + mask.y, 0.0f, 1.0f);
    r -= shadow;
    
    //if (_fabs(u.x - 0.5f * iResolution.x) / iResolution.y > 0.5f) r = to_float4(0);

//r = coldeb;

  SetFragmentShaderComputedColor(r);
}