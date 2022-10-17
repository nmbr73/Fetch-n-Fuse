
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: Video' to iChannel1
// Connect Buffer A 'Texture: Pebbles' to iChannel2
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// https://www.cs.ubc.ca/~rbridson/docs/bridson-siggraph2007-curlnoise.pdf

__DEVICE__ float4 ssamp( float2 uv, float oct, float iTime, float2 iResolution, __TEXTURE2D__ iChannel1, __TEXTURE2D__ iChannel2 )
{
    uv /= oct;
    
    float2 d = 2.0f*to_float2(1.0f,-1.0f)/iResolution;
    
    float4 tex = (texture( iChannel1, 1.25f*uv - 0.0f*0.004f*iTime )
                + texture( iChannel1, 1.25f*uv - 0.0f*0.004f*iTime + swi2(d,x,y) )
                + texture( iChannel1, 1.25f*uv - 0.0f*0.004f*iTime + swi2(d,x,x) )
                + texture( iChannel1, 1.25f*uv - 0.0f*0.004f*iTime + swi2(d,y,y) )
                + texture( iChannel1, 1.25f*uv - 0.0f*0.004f*iTime - swi2(d,x,y) )) / 5.0f;
    
    float4 noise = 0.15f*(to_float4_s(1.0f)-texture( iChannel2, 4.0f*uv - 0.17f*iTime ))
                 + 0.15f*(to_float4_s(1.0f)-texture( iChannel2, 3.3f*uv + 0.1f*iTime ));
float zzzzzzzzzzzzz;    
    return tex + noise;
}


__DEVICE__ float4 dx( float2 uv, float oct, float iTime, float2 iResolution, __TEXTURE2D__ iChannel1, __TEXTURE2D__ iChannel2 ) 
{ 
  float2 e = to_float2(1.0f/256.0f, 0.0f);
  return (ssamp(uv+swi2(e,x,y),oct,iTime,iResolution,iChannel1,iChannel2) - ssamp(uv-swi2(e,x,y),oct,iTime,iResolution,iChannel1,iChannel2)) / (2.0f*e.x);
}
__DEVICE__ float4 dy( float2 uv, float oct, float iTime, float2 iResolution, __TEXTURE2D__ iChannel1, __TEXTURE2D__ iChannel2 ) 
{ 
  float2 e = to_float2(1.0f/256.0f, 0.0f);
  return (ssamp(uv+swi2(e,y,x),oct,iTime,iResolution,iChannel1,iChannel2) - ssamp(uv-swi2(e,y,x),oct,iTime,iResolution,iChannel1,iChannel2)) / (2.0f*e.x); 
}

__DEVICE__ float2 hash2( float n ) { return fract_f2(sin_f2(to_float2(n,n+1.0f))*to_float2(43758.5453123f,22578.1459123f)); }

__KERNEL__ void SmokeyJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, float3 iChannelResolution[], sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{

    fragCoord+=0.5f;

    float2 uv = fragCoord/iResolution;
  
    // vel
    //vec2 vel = to_float2( dx(uv,iChannel1,iChannel2).x, dy(uv,iChannel1,iChannel2).x );
    
    // curl
    float oct = 1.25f;
    float2 curl1 = 0.7f*to_float2( dy(uv,oct,iTime,iResolution,iChannel1,iChannel2).x, -dx(uv,oct,iTime,iResolution,iChannel1,iChannel2).x );
    //oct = 7.37f;
    //vec2 curl2 = 0.0f*to_float2( dy(uv,oct,iTime,iResolution,iChannel1,iChannel2).x, -dx(uv,oct,iTime,iResolution,iChannel1,iChannel2).x );
    //oct = 0.1f;
    //vec2 curl3 = 0.0f*to_float2( dy(uv,oct,iTime,iResolution,iChannel1,iChannel2).x, -dx(uv,oct,iTime,iResolution,iChannel1,iChannel2).x );
    
    float2 curl = 0.0004f*curl1;// + 0.0001f*curl2 + 0.00001f*curl3;
    curl *= _sqrtf(iResolution.x/640.0f);
    
    float2 wind = 0.002f*to_float2(0.45f + 0.1f*_logf(_fmaxf(iTime,1.0f)),1.0f); // grav and wind
    float2 rand = 0.0005f*(hash2(iTime)-0.5f);

    fragColor = 0.991f*texture( iChannel0, uv - curl + rand - wind );
    
    //fragColor += 0.1f * smoothstep(0.9f,1.0f,_tex2DVecN( iChannel1,uv.x,uv.y,15));
    
    if( iMouse.z > 0.0f )
        fragColor += 0.5f*length(_tex2DVecN( iChannel1,uv.x,uv.y,15))*0.15f*smoothstep( iResolution.x/10.0f,iResolution.x/10.0f-29.0f,length(swi2(iMouse,x,y)-fragCoord));
      //fragColor += 0.1f*smoothstep( iResolution.x/10.0f,iResolution.x/10.0f-29.0f,length(swi2(iMouse,x,y)-fragCoord));
    else
      fragColor += 0.035f*smoothstep( iResolution.x/8.0f,iResolution.x/8.0f-29.0f,length(fragCoord-iResolution/(3.0f+iTime/5.0f)));

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel1


//https://www.cs.ubc.ca/~rbridson/docs/bridson-siggraph2007-curlnoise.pdf


__DEVICE__ float4 ssampI( float2 uv, __TEXTURE2D__ iChannel1 )
{
    return _tex2DVecN( iChannel1,uv.x,uv.y,15);
}

//vec3 e = to_float3_aw(1.0f/iResolution, 0.0f);
//vec4 dx( float2 uv ) { return (ssamp(uv+swi2(e,x,z)) - ssamp(uv-swi2(e,x,z))) / (2.0f*e.x); }
//vec4 dy( float2 uv ) { return (ssamp(uv+swi2(e,z,y)) - ssamp(uv-swi2(e,z,y))) / (2.0f*e.y); }

__KERNEL__ void SmokeyJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel1)
{

  fragCoord+=0.5f; 
   
  float2 uv = fragCoord / iResolution;
  fragColor = 0.5f*ssampI(uv, iChannel1);
    
    //vec2 L = normalize(to_float2(-0.5f,0.5f));
    //vec2 n = 0.005f*to_float2(dy(uv).x,-dx(uv).x);
    //swi2(fragColor,x,y) = n;
    //fragColor *= 0.5f+0.3f*_fmaxf(0.0f,dot(L,n));
    //fragColor = clamp(fragColor,0.0f,1.0f);
    //fragColor = _sqrtf(fragColor);
    //fragColor = _powf(fragColor,2.0f);

  SetFragmentShaderComputedColor(fragColor);
}