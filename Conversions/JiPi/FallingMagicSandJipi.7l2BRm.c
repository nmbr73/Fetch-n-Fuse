
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)




__KERNEL__ void FallingMagicSandJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, int iFrame, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
  CONNECT_CHECKBOX0(Reset, 0);
  CONNECT_SLIDER2(PenSize, 0.0f, 1.0f, 0.05f);
  CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);
  
  fragCoord+=0.5f;
  
  float2 poss[]={to_float2(-0.004f,0.0f),to_float2(0.004f,0.0f),to_float2(0.0f,0.01f)};
  float pit=3.14159f*2.0f/3.0f;
  
  float2 uv = fragCoord / iResolution;
  float4 a=to_float4_s(-0.001f);
  for (int i=0;i<3;i++){
     a+=texture(iChannel0,uv+poss[i]);}
     a=a/3.0f;
     if(distance_f2(uv,swi2(iMouse,x,y)/iResolution) < PenSize){
        a = to_float4(_sinf(iTime),_sinf(iTime+pit),_sinf(iTime-pit),1.0f)*0.5f+0.5f;
     }
     //a=to_float4(1.0f,0.0f,0.0f,1.0f);
     
     a += (Color-0.5f);
     a.w = Color.w;//1.0f;
     
     fragColor = clamp(a,0.0f,1.0f);// to_float4(0.0f,0.0f,1.0f,1.0f);

  if (iFrame<1 || Reset) fragColor=to_float4_s(0.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Gray Noise Medium' to iChannel1
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void FallingMagicSandJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

    float2 uv = fragCoord / iResolution;
    float3 a= swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z);
    float3 b= swi3(texture(iChannel1,uv*to_float2(2.0f,1.5f)+to_float2(0,iTime*0.1f)),x,y,z);
    if (length(a)>length(b)) { a=normalize(a); }
    else                     { a=to_float3_s(0.0f); }
    
    fragColor = to_float4_aw(a,1.0f);//to_float4_aw(uv,0.5f+0.5f*_sinf(iTime),1.0f);

  SetFragmentShaderComputedColor(fragColor);
}