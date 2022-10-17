
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)



union Zahl
 {
   float  _Float; //32bit float
   uint   _Uint;  //32bit unsigend integer
 };

//hashing noise by IQ
__DEVICE__ float hash( int k )
{
    uint n = (uint)(k);
    n = (n << 13U) ^ n;
    n = n * (n * n * 15731U + 789221U) + 1376312589U;
    //return uintBitsToFloat( (n>>9U) | 0x3f800000U ) - 1.0f;
    Zahl z;
    uint X = (n>>9U) | 0x3f800000U;
    z._Uint = X;
    return (z._Float) - 1.0f;
}

__DEVICE__ float _powcf(float x, float y) {
    float ret = _powf(x,y);
    if (isnan(ret)) {
        ret = 0.0001f;
    }
    return ret;
}

//#define keyDown(ascii)    ( texelFetch(iChannel3,to_int2(ascii,0),0).x > 0.0f)
//#define KEY_SPACE 32

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Preset: Keyboard' to iChannel3
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

union A2F
 {
   float3  F; //32bit float
   float  A[3];  //32bit unsigend integer
 };

                                      
__DEVICE__ float convolve(float2 fragCoord, float2 iResolution, __TEXTURE2D__ iChannel0)
{
    float kernel[3][3] = { { 0.68f, -0.9f,   0.68f},
                         {-0.9f,  -0.66f, -0.9f},
                         { 0.68f, -0.9f,   0.68f} };
  
    A2F _kernel[3];
    
      _kernel[0].F = to_float3(0.68f, -0.9f,   0.68f);
      _kernel[1].F = to_float3(-0.9f,  -0.66f, -0.9f);
      _kernel[2].F = to_float3(0.68f, -0.9f,   0.68f);
  
  
  
    float a = 0.0f;
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            //a += texelFetch(iChannel0, to_int2(fragCoord) + to_int2(i - 1, j - 1), 0).x * kernel[i][j];
            a += texture(iChannel0, (make_float2(to_int2_cfloat(fragCoord) + to_int2(i - 1, j - 1))+0.5)/iResolution).x * kernel[i][j];
            //a += texture(iChannel0, (make_float2(to_int2_cfloat(fragCoord) + to_int2(i - 1, j - 1))+0.5f)/iResolution).x * kernel[i].A[j];
        }
    }
    return a;
}

// inverse gaussian
__DEVICE__ float activation(float x)
{
    return -1.0f / _powcf(2.0f, (0.6f * _powcf(x, 2.0f))) + 1.0f;
}

__KERNEL__ void NeuralCellularAutomatonWormsFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, int iFrame, float iTime, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(Reset, 0);
  
    fragCoord+=0.5f;

    //float4 state = texelFetch(iChannel0, to_int2(0), 0);
    float4 state = texture(iChannel0, (make_float2(to_int2(0,0))+0.5f)/iResolution);
    
    //if (to_int2(fragCoord) == to_int2(0))
    if ((int)(fragCoord.x) == 0 && (int)(fragCoord.y) == 0)
    {
        if ((iFrame == 0 || Reset) || _fabs(state.x) != iResolution.x * iResolution.y )
        {
            state.x = -iResolution.x * iResolution.y;
        }
        else
        {
            state.x = _fabs(state.x);
        }
        
        fragColor = state;
        SetFragmentShaderComputedColor(fragColor);
        return;
    }
    
    if ((iFrame == 0 || Reset) || state.x < 0.0f ||
        (iMouse.z > 0.0f && distance_f2(swi2(iMouse,x,y), fragCoord) < 0.1f*iResolution.y))
    {
        fragColor = to_float4_s(hash((int)(fragCoord.x * fragCoord.y + iTime) + iFrame));
        SetFragmentShaderComputedColor(fragColor);
        return;
    }
        
    fragColor = to_float4_s(activation(convolve(fragCoord,iResolution,iChannel0)));

    //fragColor = to_float4_s(convolve(fragCoord,iResolution,iChannel0));


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1


__KERNEL__ void NeuralCellularAutomatonWormsFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0);
  
    fragCoord+=0.5f;

    fragColor =  texture(iChannel0, (make_float2(to_int2_cfloat(fragCoord))+0.5f)/iResolution) * 0.1f 
               + texture(iChannel1, (make_float2(to_int2_cfloat(fragCoord))+0.5f)/iResolution) * 0.9f;
    
    if (iFrame == 0 || Reset) fragColor = to_float4_s(0);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer B' to iChannel0


// ---------------------------------------------------------------------------------------
//  Created by fenix in 2022
//  License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.
//
// I just watched this video and was inspired to create this shader.
//
//     https://www.youtube.com/watch?v=3H79ZcBuw4M
//
// I have no idea how this works, but in my defence neither does the guy who made the video
// (or so he says). Neural Cellular Atomaton is a pretty fancy name for essentially a 
// continuous (non-binary) CA. You can experiment with this automata (and others) at:
//
//     https://neuralpatterns.io/
//
// Other than porting to shadertoy, all I did was fancy up the rendering a little bit.
//
// Buffer A computes the neural cellular atomaton
// Buffer B performs temporal blur of buffer A
// Image computes gradient, applies lighting and color
//
// ---------------------------------------------------------------------------------------

__DEVICE__ float2 grad(float2 fragCoord, float d, float2 iResolution, __TEXTURE2D__ iChannel0)
{
    float2 delta = to_float2(d, 0);
    return to_float2(texture(iChannel0, fragCoord/iResolution + swi2(delta,x,y)).x -
                     texture(iChannel0, fragCoord/iResolution - swi2(delta,x,y)).x,
                     texture(iChannel0, fragCoord/iResolution + swi2(delta,y,x)).x -
                     texture(iChannel0, fragCoord/iResolution - swi2(delta,y,x)).x);
}

__KERNEL__ void NeuralCellularAutomatonWormsFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    fragCoord+=0.5f;

    float2 g = grad(fragCoord, 1.0f/iResolution.y, iResolution, iChannel0);
    float3 norm = normalize(to_float3_aw(g, 1.0f));
    //float value = texelFetch(iChannel0, to_int2(fragCoord), 0).r;
    float value = texture(iChannel0, (make_float2(to_int2_cfloat(fragCoord))+0.5f)/iResolution).x;
    float3 color = _mix(to_float3(0.3f,0.3f,0.5f), to_float3(1, 0.2f, 0.2f), smoothstep(0.0f, 0.1f, value));
    fragColor = to_float4_aw(color * dot(norm, normalize(to_float3(1,-1,1))), 1);

  SetFragmentShaderComputedColor(fragColor);
}