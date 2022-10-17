
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)




#define T(uv) swi2(texture(ch, (uv)/res),x,y)

__DEVICE__ float4 render( float2 res, __TEXTURE2D__ ch, float2 uv )
{
    //settings
    float2 dif = to_float2(1,0.5f);
    float f = 0.055f; // 0.040f // 0.030
    float k = 0.062f;
  
    float2 v = T(uv);
    
    float3 e = to_float3(0, -1, 1);
    float4 ret = to_float4_f2f2(
        v + dif * (- v
            + 0.2f * T(uv+swi2(e,x,y))
            + 0.2f * T(uv+swi2(e,x,z))
            + 0.2f * T(uv+swi2(e,y,x))
            + 0.2f * T(uv+swi2(e,z,x))
            + 0.05f * T(uv+swi2(e,z,y))
            + 0.05f * T(uv+swi2(e,y,z))
            + 0.05f * T(uv+swi2(e,y,y))
            + 0.05f * T(uv+swi2(e,z,z))
        ) + v.x * v.y * v.y * to_float2(-1,1)
          + to_float2(f, f + k) * (swi2(e,z,x) - v),
    to_float2(0,0));

    //if (isnan(ret.x)) ret.x = -0.001f;
    //if (isnan(ret.y)) ret.y = -0.001f;
    
    ret = clamp(ret, 0.0f, 100.0f);
    
    return ret;
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer D' to iChannel0
// Connect Buffer A 'Texture: Blending' to iChannel1

__KERNEL__ void ReactionDiffusionLoluckyJipiFuse__Buffer_A(float4 fragColor, float2 uv, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{

    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_CHECKBOX1(StartTex, 0);
    
    CONNECT_SLIDER0(TexOff, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER1(TexMul, -10.0f, 10.0f, 1.0f);

    float2 res = 200.0f*to_float2(16.0f/9.0f,1);
    float2 tuv = uv; 

    uv+=0.5f;

    if(iFrame==0 || Reset) {
        uv = (2.0f * uv - res) / res.y;
        
        if (StartTex)
        {
          float4 tex = texture(iChannel1, tuv/res);//+TexOff)*TexMul;
          fragColor = (tex+TexOff)*TexMul*tex.w;
          
        }
        else
          fragColor = to_float4(1,length(uv)<0.1f,0,0);
        
        SetFragmentShaderComputedColor(fragColor);
        
        return;
    }
    
    //if(any(greaterThan(uv, res))) return;
    if(uv.x > res.x || uv.y > res.y)  { SetFragmentShaderComputedColor(fragColor); return; }

    fragColor = render(iResolution, iChannel0, uv);

    if(distance_f2(uv,swi2(iMouse,x,y)/iResolution*res)<0.01f*iResolution.y && iMouse.z>0.0f)
        fragColor.y = 0.0f;


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void ReactionDiffusionLoluckyJipiFuse__Buffer_B(float4 fragColor, float2 uv, float2 iResolution, sampler2D iChannel0)
{

    float2 res = 200.0f*to_float2(16.0f/9.0f,1);

    uv+=0.5f;

    //if(any(greaterThan(uv, res))) return;
    if(uv.x > res.x || uv.y > res.y)   { SetFragmentShaderComputedColor(fragColor); return; }
    fragColor = render(iResolution, iChannel0, uv);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


__KERNEL__ void ReactionDiffusionLoluckyJipiFuse__Buffer_C(float4 fragColor, float2 uv, float2 iResolution, sampler2D iChannel0)
{

    float2 res = 200.0f*to_float2(16.0f/9.0f,1);

    uv+=0.5f;

    //if(any(greaterThan(uv, res))) return;
    if(uv.x > res.x || uv.y > res.y)  { SetFragmentShaderComputedColor(fragColor); return;}
    fragColor = render(iResolution, iChannel0, uv);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


__KERNEL__ void ReactionDiffusionLoluckyJipiFuse__Buffer_D(float4 fragColor, float2 uv, float2 iResolution, sampler2D iChannel0)
{

    float2 res = 200.0f*to_float2(16.0f/9.0f,1);

    uv+=0.5f;

    //if(any(greaterThan(uv, res)))   SetFragmentShaderComputedColor(fragColor), return;
    if(uv.x > res.x || uv.y > res.y)  { SetFragmentShaderComputedColor(fragColor); return; }

    fragColor = render(iResolution, iChannel0, uv);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__DEVICE__ float3 viridis(float t) {

    const float3 c0 = to_float3(0.2777273272234177f, 0.005407344544966578f, 0.3340998053353061f);
    const float3 c1 = to_float3(0.1050930431085774f, 1.404613529898575f, 1.384590162594685f);
    const float3 c2 = to_float3(-0.3308618287255563f, 0.214847559468213f, 0.09509516302823659f);
    const float3 c3 = to_float3(-4.634230498983486f, -5.799100973351585f, -19.33244095627987f);
    const float3 c4 = to_float3(6.228269936347081f, 14.17993336680509f, 56.69055260068105f);
    const float3 c5 = to_float3(4.776384997670288f, -13.74514537774601f, -65.35303263337234f);
    const float3 c6 = to_float3(-5.435455855934631f, 4.645852612178535f, 26.3124352495832f);

    return c0+t*(c1+t*(c2+t*(c3+t*(c4+t*(c5+t*c6)))));

}

__KERNEL__ void ReactionDiffusionLoluckyJipiFuse(float4 fragColor, float2 uv, float2 iResolution, sampler2D iChannel0)
{
  
    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);
  
    float2 res = 200.0f*to_float2(16.0f/9.0f,1);

    uv+=0.5f;

    float2 ab = swi2(texture(iChannel0, uv/iResolution*res/iResolution),x,y);
    float v = clamp(1.0f+ab.y-ab.x,0.0f,1.0f);
    fragColor = to_float4_aw(viridis(v)+swi3(Color,x,y,z)-0.5f,Color.w);

  SetFragmentShaderComputedColor(fragColor);
}