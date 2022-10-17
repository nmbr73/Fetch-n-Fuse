
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// sorry this is a mess right now! i will write up a blog post about what the code
// below was written to do and update this later.

//float ten_r = 0.04f;

__DEVICE__ float GetAngle( float i, float t )
{
    //float amp = 0.75f; //0.25f is subtle wriggling
    //t += amp*_sinf(1.0f*t + 4.0f*iTime + float(i));
    float dir = mod_f(i,2.0f) < 0.5f ? 1.0f : -1.0f;
    return dir * (1.0f/(0.5f*i+1.0f)+1.0f) * t + i/2.0f;
}

#define POS_CNT 5
//float2 pos[POS_CNT];

__DEVICE__ float Potential( int numNodes, float2 x, float2 pos[POS_CNT] )
{
    if( numNodes == 0 ) return 0.0f;
    
    float res = 0.0f;
    float k = 16.0f;
    for( int i = 0; i < POS_CNT; i++ )
    {
        if( i == numNodes ) break;
        res += _expf( -k * length( pos[i]-x ) );
    }
    return -_logf(res) / k;
}

__DEVICE__ void ComputePos_Soft( float t, float ten_r, float2 pos[POS_CNT] )
{
    for( int i = 0; i < POS_CNT; i++ )
    {
        float a = GetAngle( (float)(i), t );
        float2 d = to_float2(_cosf(a),_sinf(a));
        float r = ten_r;
        
        for( int j = 0; j < 3; j++ )
        {
            r += ten_r-Potential(i,r*d, pos);
        }
        
        pos[i] = r * d;
    }
}


__DEVICE__ float3 drawSlice( float2 uv, float ten_r, float2 pos[POS_CNT], float iTime )
{
  float zzzzzzzzzzz;
    float t = iTime/2.0f;
    ComputePos_Soft(t, ten_r, pos);
    float pot = Potential(POS_CNT,uv,pos);
    return to_float3_s(smoothstep(0.03f,0.01f,pot));
}

__KERNEL__ void InkyJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, int iFrame, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(Reset, 0);
    fragCoord+=0.5f;
    
    float ten_r = 0.04f;
    float2 pos[POS_CNT];

    float2 uv = fragCoord / iResolution;
    
    // sample from previous frame, with slight offset for advection
    fragColor = texture( iChannel0, uv*0.992f);
    
    // clear on first frame (dont know if this is required)
    if( iFrame == 0 || Reset ) fragColor = to_float4_s(0.0f);
    
    // camera
    uv.x += 0.1f*_sinf(0.7f*iTime);
    uv.y += 0.05f*_sinf(0.3f*iTime);
    uv = 2.0f * uv - 1.0f;
    uv.x *= iResolution.x/iResolution.y;
    
    // draw spots
    float3 spots = drawSlice( uv, ten_r, pos, iTime );
    
    // accumulate
    swi3S(fragColor,x,y,z, swi3(fragColor,x,y,z)*0.95f + spots);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'https://soundcloud.com/majikband/save-me-majik-2' to iChannel1


// Music is Save Me by Majik: https://soundcloud.com/majikband/save-me-majik-2


__KERNEL__ void InkyJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    CONNECT_COLOR0(InkColor, 0.15f,0.3f,0.8f, 1.0f);
    CONNECT_SLIDER0(Level0, -1.0f, 2.0f, 0.8f);
    CONNECT_SLIDER1(Level1, -1.0f, 2.0f, 1.5f);
    CONNECT_SLIDER2(Vignette, -1.0f, 2.0f, 0.17f);
    CONNECT_SLIDER3(Level2, -1.0f, 2.0f, 0.1f);
    CONNECT_SLIDER4(AlphaThreshold, 0.0f, 1.0f, 0.1f);
    
    CONNECT_CHECKBOX1(AlphaSet, 0);

    float2 uv = fragCoord / iResolution;
    
    // read for accum buffer
    fragColor = to_float4_s(1.0f) - texture( iChannel0, uv);
    
    // tint
    swi3S(fragColor,x,y,z, swi3(fragColor,x,y,z) * Level0);//0.8f);
    swi3S(fragColor,x,y,z, swi3(fragColor,x,y,z) + Level1*swi3(InkColor,x,y,z));//to_float3(0.15f,0.3f,0.8f));
    
    // vign, treatment
    fragColor *= 1.0f - Vignette*length(2.0f * uv - 1.0f);
    swi3S(fragColor,x,y,z, clamp(swi3(fragColor,x,y,z),0.0f,1.0f));
    swi3S(fragColor,x,y,z, swi3(fragColor,x,y,z) * (0.5f + 0.5f*_powf( 16.0f*uv.x*uv.y*(1.0f-uv.x)*(1.0f-uv.y), Level2 )));

    //fragColor += (Color-0.5f)*texture( iChannel0, uv).x;
    
    fragColor.w = InkColor.w;
    if(AlphaSet) 
      if (texture( iChannel0, uv).x < AlphaThreshold ) fragColor = to_float4(0.0f,0.0f,0.0f, fragColor.w);
      else                                             fragColor.w = 1.0f;

  SetFragmentShaderComputedColor(fragColor);
}