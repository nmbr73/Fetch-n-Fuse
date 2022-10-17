
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image '/media/a/35c87bcb8d7af24c54d41122dadb619dd920646a0bd0e477e7bdc6d12876df17.webm' to iChannel0
// Connect Image 'Texture: Blue Noise' to iChannel1


/** 
 * Sketchy Stippling / Dot-Drawing Effect by Ruofei Du (DuRuofei.com)
 * Link to demo: https://www.shadertoy.com/view/ldSyzV
 * starea @ ShaderToy, License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License. 
 *
 * A one-pass shader for dotted drawing / sketch post processing effect. 
 * Press the mouse for a slower but more classic sketching effect, though I prefer the dotted version.
 * Shader forked and related ones are listed below.
 * Works better with video mipmaping.
 *
 * Reference: 
 * [1] Pencil vs Camera. http://www.duruofei.com/Research/pencilvscamera
 *
 * Forked or related:
 * [1] Pol's Photoshop Blends Branchless: https://www.shadertoy.com/view/Md3GzX
 * [2] Gaussian Blur: https://www.shadertoy.com/view/ltBXRh
 * [3] williammalo2's Blur with only one pixel read: https://www.shadertoy.com/view/XtGGzz
 * [3] demofox's greyscale: https://www.shadertoy.com/view/XdXSzX 
 * [4] iq's Postprocessing: https://www.shadertoy.com/view/4dfGzn
 * [5] related blur: https://www.shadertoy.com/view/XsVBDR
 *
 * Related masterpieces:
 * [1] flockaroo's Notebook Drawings: https://www.shadertoy.com/view/XtVGD1
 * [2] HLorenzi's Hand-drawn sketch: https://www.shadertoy.com/view/MsSGD1 
 **/
 
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15) 
 
#define PI  3.1415926536f
#define PI2  (PI * 2.0f) 


// Gaussian PDF
__DEVICE__ float normpdf(in float x, in float sigma) 
{
  return 0.39894f * _expf(-0.5f * x * x / (sigma * sigma)) / sigma;
}

// 
__DEVICE__ float3 colorDodge(in float3 src, in float3 dst)
{
    return step(to_float3_s(0.0f), dst) * mix_f3(_fminf(to_float3_s(1.0f), dst/ (1.0f - src)), to_float3_s(1.0f), step(to_float3_s(1.0f), src)); 
}

__DEVICE__ float greyScale(in float3 col) 
{
    return dot(col, to_float3(0.3f, 0.59f, 0.11f));
    //return dot(col, to_float3(0.2126f, 0.7152f, 0.0722f)); //sRGB
}

__DEVICE__ float2 random(float2 p){
  p = fract_f2(p * (to_float2(314.159f, 314.265f)));
    p += dot(p, swi2(p,y,x) + 17.17f);
    return fract_f2((swi2(p,x,x) + swi2(p,y,x)) * swi2(p,x,y));
}

__DEVICE__ float2 random2(float2 p, __TEXTURE2D__ iChannel1)
{
    return swi2(texture(iChannel1, p / to_float2_s(1024.0f)),x,y);
    //blue1 = texture(iChannel1, p / to_float2_s(1024.0f));
    //blue2 = texture(iChannel1, (p+to_float2(137.0f, 189.0f)) / to_float2_s(1024.0f));    
}

__KERNEL__ void SketchyStipplingStylizationFuse(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{

    const int mSize = 9;
    const int kSize = (mSize-1)/2;
    const float sigma = 3.0f;
    float kernel[mSize];

    float2 q = fragCoord / iResolution;
    float3 col = swi3(_tex2DVecN(iChannel0,q.x,q.y,15),x,y,z);

    float2 r = random(q);
    r.x *= PI2;
    float2 cr = to_float2(_sinf(r.x),_cosf(r.x))*_sqrtf(r.y);
    
    float3 blurred = swi3(texture(iChannel0, q + cr * (to_float2_s(mSize) / iResolution) ),x,y,z);
    
    // comparison
    if (iMouse.z > 0.5f) {
        blurred = to_float3_s(0.0f); 
        float Z = 0.0f;
        for (int j = 0; j <= kSize; ++j) {
            kernel[kSize+j] = kernel[kSize-j] = normpdf((float)(j), sigma);
        }
        for (int j = 0; j < mSize; ++j) {
            Z += kernel[j];
        }
        
    // this can be done in two passes
        for (int i = -kSize; i <= kSize; ++i) {
            for (int j = -kSize; j <= kSize; ++j) {
                blurred += kernel[kSize+j]*kernel[kSize+i]*swi3(texture(iChannel0, (fragCoord+to_float2((float)(i),(float)(j))) / iResolution),x,y,z);
            }
      }
       blurred = blurred / Z / Z;
        
        // an interesting ink effect
        //r = random2(q,iChannel1);
        //vec2 cr = to_float2(_sinf(r.x),_cosf(r.x))*_sqrtf(-2.0f*r.y);
        //blurred = texture(iChannel0, q + cr * (to_float2(mSize) / iResolution) ).rgb;
    }
    
    float3 inv = to_float3_s(1.0f) - blurred; 
    // color dodge
    float3 lighten = colorDodge(col, inv);
    // grey scale
    float3 res = to_float3_s(greyScale(lighten));
    
    // more contrast
    res = to_float3_s(_powf(res.x, 3.0f)); 
    //res = clamp(res * 0.7f + 0.3f * res * res * 1.2f, 0.0f, 1.0f);
    
    // edge effect
    if (iMouse.z > 0.5f) res *= 0.25f + 0.75f * _powf( 16.0f * q.x * q.y * (1.0f - q.x) * (1.0f - q.y), 0.15f );
    fragColor = to_float4_aw(res, 1.0f); 

  SetFragmentShaderComputedColor(fragColor);
}