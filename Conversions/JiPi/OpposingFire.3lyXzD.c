
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: 1 2 3 5' to iChannel0
// Connect Image 'Texture: 3 5' to iChannel1
// Connect Image 'Texture: 3' to iChannel2

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

//#define timescale 0.5f
#define scaleX 1.5f
#define scaleY 0.3f


#define inside(P) step(0.1f, length(  texture(iChannel0, (P)/iResolution) - to_float4(0.05f, 0.64f, 0.15f, 1.0f) ) )

#define FROSTYNESS 0.5f
//#define RANDNERF 2.5

__DEVICE__ float rand(float2 uv) {
    #ifdef RANDNERF
    uv = _floor(uv*_powf(10.0f, RANDNERF))/_powf(10.0f, RANDNERF);
    #endif
    
    float a = dot(uv, to_float2(92.0f, 80.0f));
    float b = dot(uv, to_float2(41.0f, 62.0f));
    
    float x = _sinf(a) + _cosf(b) * 51.0f;
    return fract(x);
}

__DEVICE__ float2 thingPosition(float t, float aspect) {
    float tx = t / aspect;
    float2 p = to_float2(_sinf(2.2f * tx) - _cosf(1.4f * tx), _cosf(1.3f * t) + _sinf(-1.9f * t));
    p.y *= 0.2f;
    p.x *= 0.4f;
 	return p;
}


__KERNEL__ void OpposingFireFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{

    CONNECT_BUTTON0(Shader, 0, Fire, Outline, Frost, Chasers, BurnFade);
    
    // Parameter Fire
    CONNECT_COLOR0(FColor1, 0.0f, 0.2f, 1.0f, 1.0f);
    CONNECT_COLOR1(FColor2, 1.0f, 0.21f, 0.0f, 1.0f);
    
    CONNECT_COLOR2(CColor1, 0.9f, 0.2f, 0.4f, 1.0f);
    CONNECT_COLOR3(CColor2, 0.8f, 0.3f, 0.2f, 1.0f);
    
    CONNECT_SLIDER0(timescale, -1.0f, 1.0f, 0.5f);
    CONNECT_POINT0(FireScale, 0.0f, 0.0f );

    // Parameter Outline
    CONNECT_SLIDER1(Outline1, -1.0f, 20.0f, 9.0f);
    CONNECT_SLIDER2(Outline2, -1.0f, 20.0f, 5.0f);
    CONNECT_INTSLIDER0(Outline3, 3, 10, 3);
    CONNECT_INTSLIDER1(Outline4, 3, 30, 9);

    // Parameter Chasers
    CONNECT_CHECKBOX0(Chasers2Col, 0);
    CONNECT_SLIDER3(CRadius, -1.0f, 2.0f, 0.0f);
    CONNECT_SLIDER4(CTailLength, -1.0f, 2.0f, 0.0f);
    CONNECT_SLIDER5(CEdgeWidth, -1.0f, 1.0f, 0.0f);
    
    
    if (Shader == 1) //Opposinf Fire : iCh0: Peebles
    {
      float3 col = to_float3(0,0,0);
      float2 uv = fragCoord/iResolution;
      
      float dist = texture(iChannel0,to_float2(uv.x*scaleX+FireScale.x - iTime*1.1f*timescale,uv.y*scaleY+FireScale.y - iTime*1.8f*timescale)).x;
      
      float tex = texture(iChannel0,to_float2(uv.x*scaleX+FireScale.x + dist*0.2f,uv.y*scaleY+FireScale.y - iTime*1.5f*timescale)).x;
      
      tex += uv.y*0.5f;
      float fire = _powf(1.0f-tex,2.3f);
      fire -= (1.0f-(_fabs(uv.x-0.5f)*2.0f))*0.5f;
      
      //col += fire *5.0f* _mix(to_float3(0.0f,0.2f,1),to_float3(1,0.21f,0),uv.x);
      col += fire *5.0f* _mix(swi3(FColor1,x,y,z),swi3(FColor2,x,y,z),uv.x);

      fragColor = to_float4_aw(col,1.0f);
    }
    else    
    if(Shader == 2) // Pixel Perfect Outline: iCh0 Video
    {    
       fragColor = to_float4_s(0);
    
       for (int k=0; k<Outline4; k++)
          //fragColor += inside( fragCoord + to_float2(k%3, k/3) - 1.0f );
        fragColor += inside( fragCoord + to_float2(k%Outline3, k/Outline3) - 1.0f );
        
       //fragColor = _fminf( to_float4_s(9.0f)-fragColor , fragColor ) / 5.0f;
       fragColor = _fminf( to_float4_s(Outline1)-fragColor , fragColor ) / Outline2;
    }    
    else    
    if(Shader == 3) // Spreading Frost iCh0: Texture(London) iCh1: & iCh2: Frostmaserung
    {  
      float2 uv = fragCoord / iResolution;
      float progress = fract(iTime / 4.0f);

      float4 frost = texture(iChannel1, uv);
      float icespread = texture(iChannel2, uv).x;

      float2 rnd = to_float2(rand(uv+frost.x*0.05f), rand(uv+frost.z*0.05f));
              
      float size = _mix(progress, sqrt(progress), 0.5f);   
      size = size * 1.12f + 0.0000001f; // just so 0.0 and 1.0 are fully (un)frozen and i'm lazy
      
      float2 lens = to_float2(size, _powf(size, 4.0f) / 2.0f);
      float dist = distance_f2(uv, to_float2(0.5f, 0.5f)); // the center of the froziness
      float vignette = _powf(1.0f-smoothstep(lens.x, lens.y, dist), 2.0f);
     
      rnd *= swi2(frost,x,y)*vignette*FROSTYNESS;
      
      rnd *= 1.0f - floor(vignette); // optimization - brings rnd to 0.0 if it won't contribute to the image
      
      float4 regular = texture(iChannel0, uv);
      float4 frozen = texture(iChannel0, uv + rnd);
      frozen *= to_float4(0.9f, 0.9f, 1.1f, 1.0f);
          
      fragColor = _mix(frozen, regular, smoothstep(icespread, 1.0f, _powf(vignette, 2.0f)));
    }
    else    
    if(Shader == 4) // Chasers no iCh
    {  
   
    float2 uv = to_float2_s(0.5f) - fragCoord / iResolution;
    float aspect = iResolution.x / iResolution.y;
    uv.x *= aspect;
    float3 cFinal = to_float3_s(0.0f);
    
    //float3 color1 = to_float3(0.9f, 0.2f, 0.4f);
    //vec3 color2 = vec3(0.8, 0.3, 0.2);
    const float radius = 0.035f+CRadius;
    const float tailLength = 0.7f+CTailLength;
    const float edgeWidth = 0.03f+CEdgeWidth;
    for (int j = 0; j < 11; j++) {
        float thisRadius = radius + _sinf((float)(j) * 0.7 + iTime * 1.2f) * 0.02f;
        float dMin = 1.0f;
        const int iMax = 12;
        for (int i = 0; i < iMax; i++) {
            float iPct = (float)(i) / (float)(iMax);
            float segmentDistance = length(thingPosition(iTime * 2.0f + (float)(j) * 1.5f - iPct * tailLength, aspect) - uv);
            dMin = min(dMin, segmentDistance + pow(iPct, 0.8f) * (thisRadius + edgeWidth));
        }
        if(Chasers2Col)
          swi3S(CColor1,x,y,z, _mix(swi3(CColor1,x,y,z), swi3(CColor2,x,y,z), mod_f((float)(j), 2.0f)));
        //float4 _CColor1 = to_float4_aw(_mix(swi3(CColor1,x,y,z), swi3(CColor2,x,y,z), mod_f((float)(j), 2.0f)), 1.0f);
        
        cFinal += 5.0f * (1.0f - smoothstep(thisRadius, thisRadius + edgeWidth, dMin)) * swi3(CColor1,x,y,z); //_mix(swi3(CColor1,x,y,z), swi3(CColor2,x,y,z), mod(float(j), 2.0));
    }
    
	  fragColor = to_float4_aw(to_float3_s(1.0) - cFinal, 1.0f);
    }
    else    
    if(Shader == 5) // Digital Burn Fade iCh0: from Texture iCh1: to Texture
    { 

      float2 uv = fragCoord / iResolution;
      uv.y = 1.0-uv.y;
      fragColor = texture(iChannel0, uv);
      
      //get grey comp of ichannel1
      float4 secondImageGrey = texture(iChannel1, uv);
      float grey = (secondImageGrey.x + secondImageGrey.y + secondImageGrey.z) / 3.0f;
      secondImageGrey.x = grey;
      secondImageGrey.y = grey;
      secondImageGrey.z = grey;
      
      //threshold on sin time
      float thresh = (_sinf(iTime)*0.5f) + 0.5f;
      float thresh2 = (_sinf(iTime+0.1f)*0.5f) + 0.5f;
      float thresh3 = (sin(iTime-0.1f)*0.5f) + 0.5f;
      
      if(secondImageGrey.x > thresh3)
      {
            fragColor.x = 0.0;
            fragColor.y = 1.0;
            fragColor.z = 0.0;
      }    
      
      if(secondImageGrey.x > thresh2)
      {
            fragColor.x = 0.0;
            fragColor.y = 1.0;
            fragColor.z = 1.0;
      }
      
      if(secondImageGrey.x > thresh)
      {
          fragColor.x = 0.0;
          fragColor.y = 0.0;
          fragColor.z = 0.0;
      }
      
      fragColor.w = FColor1.w;
    }


  SetFragmentShaderComputedColor(fragColor);
}