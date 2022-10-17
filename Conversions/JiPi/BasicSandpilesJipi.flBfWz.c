
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer D' to iChannel0

#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


__KERNEL__ void BasicSandpilesJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_SLIDER1(PenSize, 0.0f, 10.0f, 1.0f);
    CONNECT_SLIDER2(PenSizeStart, 0.0f, 1.0f, 0.2f);
    
    //Blending
    CONNECT_SLIDER3(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER5(Blend1Mul, -10.0f, 10.0f, 1.0f);
    
    fragCoord+=0.5f;
  
    float s = texture(iChannel0,fragCoord/iResolution).x; 
    
    if(s>3.0f) s-=4.0f;

    float d = texture(iChannel0,(fragCoord+to_float2( 0.0f,-1.0f))/iResolution).x; 
    if(d>3.0f) s++;
    float l = texture(iChannel0,(fragCoord+to_float2(-1.0f, 0.0f))/iResolution).x; 
    if(l>3.0f) s++;
    float r = texture(iChannel0,(fragCoord+to_float2( 1.0f, 0.0f))/iResolution).x; 
    if(r>3.0f) s++;
    float u = texture(iChannel0,(fragCoord+to_float2( 0.0f, 1.0f))/iResolution).x; 
    if(u>3.0f) s++;
    
    if(iMouse.z>0.0f && distance_f2(fragCoord,swi2(iMouse,x,y)) < PenSize) s = 1000.0f;

    if(iFrame==0 || Reset) s=0.0f;
    
    if((iFrame==0 || Reset) && distance_f2(fragCoord,0.5f+iResolution/2.0f) < PenSizeStart) s = 80000.0f;

    //Blending
    float4 tex = texture(iChannel1,fragCoord/R); // Blendingtexture
    if (Blend1>0.0)
    {
      s = _mix(s, ((tex.x+tex.y+tex.z+Blend1Off)*Blend1Mul), Blend1);
    }

   
    fragColor = to_float4_s(s);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void BasicSandpilesJipiFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    fragCoord+=0.5f;

    float s = texture(iChannel0,fragCoord/iResolution).x; 
    
    if(s>3.0f) s-=4.0f;
    
    float d = texture(iChannel0,(fragCoord+to_float2( 0.0f,-1.0f))/iResolution).x; 
    if(d>3.0f) s++;
    float l = texture(iChannel0,(fragCoord+to_float2(-1.0f, 0.0f))/iResolution).x; 
    if(l>3.0f) s++;
    float r = texture(iChannel0,(fragCoord+to_float2( 1.0f, 0.0f))/iResolution).x; 
    if(r>3.0f) s++;
    float u = texture(iChannel0,(fragCoord+to_float2( 0.0f, 1.0f))/iResolution).x; 
    if(u>3.0f) s++;
    
    fragColor = to_float4_s(s);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


__KERNEL__ void BasicSandpilesJipiFuse__Buffer_C(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    fragCoord+=0.5f;
  
    float s = texture(iChannel0,fragCoord/iResolution).x; 
    
    if(s>3.0f) s-=4.0f;
    
    float d = texture(iChannel0,(fragCoord+to_float2( 0.0f,-1.0f))/iResolution).x; 
    if(d>3.0f) s++;
    float l = texture(iChannel0,(fragCoord+to_float2(-1.0f, 0.0f))/iResolution).x; 
    if(l>3.0f) s++;
    float r = texture(iChannel0,(fragCoord+to_float2( 1.0f, 0.0f))/iResolution).x; 
    if(r>3.0f) s++;
    float u = texture(iChannel0,(fragCoord+to_float2( 0.0f, 1.0f))/iResolution).x; 
    if(u>3.0f) s++;
    
    fragColor = to_float4_s(s);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


__KERNEL__ void BasicSandpilesJipiFuse__Buffer_D(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    fragCoord+=0.5f;
  
    float s = texture(iChannel0,fragCoord/iResolution).x; 
    
    if(s>3.0f) s-=4.0f;

    float d = texture(iChannel0,(fragCoord+to_float2( 0.0f,-1.0f))/iResolution).x; 
    if(d>3.0f) s++;
    float l = texture(iChannel0,(fragCoord+to_float2(-1.0f, 0.0f))/iResolution).x; 
    if(l>3.0f) s++;
    float r = texture(iChannel0,(fragCoord+to_float2( 1.0f, 0.0f))/iResolution).x; 
    if(r>3.0f) s++;
    float u = texture(iChannel0,(fragCoord+to_float2( 0.0f, 1.0f))/iResolution).x; 
    if(u>3.0f) s++;

    fragColor = to_float4_s(s);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__DEVICE__ float4 color(float2 p, float2 R, float4 Color,__TEXTURE2D__ iChannel0, bool BKG){
    //return to_float4_s(0.5f)+to_float4_s(0.5f)*cos_f4(6.28f*(texture(iChannel0,p/iResolution)*0.25f+to_float4(0,0.1f,0.2f,0.3f)));
    
    float4 tex = texture(iChannel0,p/iResolution); 
    
    float4 ret = to_float4_s(0.5f)+to_float4_s(0.5f)*cos_f4(6.28f*(tex*0.25f+Color));
    
    if(BKG)
    {
      if (tex.w == 0.0f) ret = to_float4_s(0.0f);//color *= tex.w;
    }
   
    return ret;
}

__KERNEL__ void BasicSandpilesJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    CONNECT_CHECKBOX1(BKG, 0);
    CONNECT_COLOR0(Color, 0.0f, 0.1f, 0.2f, 0.3f);
    CONNECT_SLIDER0(Brightness, 0.0f, 1.0f, 0.2f);
  
    fragCoord+=0.5f;      
    fragColor += color(fragCoord,R,Color,iChannel0,BKG);
    fragColor += color(fragCoord+to_float2( 0.5f, 0),R,Color,iChannel0,BKG);
    fragColor += color(fragCoord+to_float2(-0.5f, 0),R,Color,iChannel0,BKG);
    fragColor += color(fragCoord+to_float2( 0, 0.5f),R,Color,iChannel0,BKG);
    fragColor += color(fragCoord+to_float2( 0,-0.5f),R,Color,iChannel0,BKG);
    
    fragColor *= Brightness;//0.2f;

  SetFragmentShaderComputedColor(fragColor);
}
