
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: London' to iChannel3
// Connect Buffer A 'Texture: Abstract 1' to iChannel1
// Connect Buffer A 'Texture: RGBA Noise Medium' to iChannel2
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// --- metrics for color similarity 

//#define dist(c1,c2) distance_f3(c1,c2)
  #define dist(c1,c2) distance_f3(swi3(c1,x,y,z),swi3(c2,x,y,z))
//#define dist(c1,c2) distance_f3( c1 / _fmaxf(c1.x,_fmaxf(c1.y,c1.z)), c2 / _fmaxf(c2.x,_fmaxf(c2.y,c2.z)) )
//#define dist(c1,c2) distance_f3(c1.x,c2.x)
//#define dist(c1,c2) _fmaxf(to_float3_s(0.0f),1.0f-distance_f3(swi3(c1,x,y,z),swi3(c2,x,y,z)))

#define DIST 8.0f

#define R iResolution

#define hash(s)  ( swi2(texture(iChannel2, U + iTime * s /R ),x,y) - 0.5f )

#define T(s)       texture(iChannel0, U + amnt* hash(s)/R )
 
__KERNEL__ void ImageMorphBleedEffectVFabFuse__Buffer_A(float4 O, float2 u, float2 iResolution, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
    CONNECT_CHECKBOX0(Reset, 0);
  
    CONNECT_SLIDER0(Level0, -1.0f, 1.0f, 0.0f);
  
    u+=0.5f; 

    //float2 R = iResolution, 
    float2 U = u/R;
         
    float4 bg = _tex2DVecN(iChannel3,U.x,U.y,15); // target image
    
    O = _tex2DVecN(iChannel0,U.x,U.y,15);        // current stage
    float amnt = DIST * dist(bg, O);             // difference to target
    float4 c1 = T(1.93937174e6),                 // pull alternate value
           c2 = T(1.12380517e5);                 // as far as unconverged
   
    float a = dist(c1, bg),                      // difference to target
          b = dist(c2, bg);
    if ( b < a ) a = b, c1 = c2;                 // get the closest
    O =  _mix(c1, O, _fminf(2.0f*a, 1.0f) );     //  is close enought, blend it weigthed by convergence

    if (O.x==0.0f&&O.y==0.0f&&O.z==0.0f&&O.w==0.0f || Reset)  O = _tex2DVecN(iChannel1,U.x,U.y,15);  // init texture (for the color palette)

  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


// Fork of "Image Morph Bleed Effect" by _bm. https://shadertoy.com/view/NdS3WK
// 2021-04-09 11:50:28
// Refactoring, commenting and extending

__KERNEL__ void ImageMorphBleedEffectVFabFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    fragCoord+=0.5f;

    fragColor = texture(iChannel0, fragCoord / iResolution);

  SetFragmentShaderComputedColor(fragColor);
}