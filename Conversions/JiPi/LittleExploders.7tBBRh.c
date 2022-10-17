
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


#define dot2(x) dot(x,x)
__KERNEL__ void LittleExplodersFuse__Buffer_A(float4 O, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0)
{
  
  CONNECT_CHECKBOX0(Reset, 0);
  CONNECT_CHECKBOX1(Texture, 0);
  
  CONNECT_SLIDER0(ParM1, -10.0f, 10.0f, 0.98f);
  CONNECT_SLIDER1(ParM2, -10.0f, 10.0f, 0.0001f);
  CONNECT_SLIDER2(ParM3, -10.0f, 10.0f, 0.0001f);
  
  CONNECT_SLIDER3(BLogic1, -10.0f, 10.0f, 0.00001f);
  CONNECT_SLIDER4(BLogic2, -10.0f, 10.0f, 0.98f);
  CONNECT_SLIDER5(BLogic3, -10.0f, 10.0f, 0.2f);
  CONNECT_SLIDER6(BLogic4, -10.0f, 50.0f, 10.0f);
  CONNECT_SLIDER7(BLogic5, -10.0f, 10.0f, 0.0f);
  CONNECT_SLIDER8(BLogic6, -10.0f, 10.0f, 0.03f);
  
  CONNECT_SLIDER9(Tuv, -10.0f, 500.0f, 100.0f);
  
  U+=0.5f;

  int X=(int)(_floor(U.x));
  O=texture(iChannel0,U/iResolution);
  bool bLogic=(U.y<1.0f && U.x<256.0f);
  if(iFrame<2 || Reset) O=to_float4_f2f2(bLogic?to_float2(_cosf(U.x),_sinf(U.x))*100.0f:to_float2_s(0),to_float2_s(0.0f));
  //if(iFrame<2 || Reset) O=to_float4_f2f2(bLogic?to_float2(1.0f,-1.0f)*100.0f:to_float2_s(0),to_float2_s(0.0f)); // is zu doof
  
  if (Texture) // is zu doof
  {
    float4 tex = texture(iChannel1,U/iResolution);
    if(tex.w>0.0f) 
    {
      float2 tuv = U/iResolution;
      O = to_float4_f2f2(bLogic?to_float2(tuv.x,tuv.y)*Tuv:to_float2_s(0),to_float2_s(0.0f));
    }
  }
  
  U-=iResolution*0.5f;
  float d=100.0f;
  for(int i=0;i<256;i++){
    float4 v=texture(iChannel0,to_float2((float)(i)+0.5f,0.5f)/iResolution);
    if(bLogic){
      if(i!=X){
        float2 dlt=swi2(v,x,y)-swi2(O,x,y),a=sign_f2(dlt)/_fmaxf(4.0f,dot2(dlt));
        //float m=_fminf(1.0f,0.98f+dot2(swi2(v,z,w)-swi2(O,z,w))*0.0001f+dot2(dlt)*0.0001f);
        float m=_fminf(1.0f,ParM1+dot2(swi2(v,z,w)-swi2(O,z,w))*ParM2+dot2(dlt)*ParM3);
        //O.zw=swi2(O,z,w)*m+a;
        O.z=O.z*m+a.x;
        O.w=O.w*m+a.y;
      }
    }else d=_fminf(d,length(U-swi2(v,x,y)));
  }
  if(bLogic){
    swi2S(O,x,y, swi2(O,x,y)+swi2(O,z,w));
    swi2S(O,z,w, swi2(O,z,w)-sign_f2(swi2(O,x,y))*length(swi2(O,x,y))*BLogic1);
    //O = to_float4_s(1.0f); is zu doof
    }
  else O.x=O.x*BLogic2+BLogic3*smoothstep(BLogic4/iResolution.y,BLogic5,d*BLogic6);

//if(U.y<1.0f && U.x < 256) O=to_float4_s(1.0f); is zu doof

  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__DEVICE__ float BG(float2 u, float iTime) {u=sin_f2(u+sin_f2(swi2(u,y,x)+iTime*0.1f));return (u.x*u.y+u.x+u.y)*0.025f+0.05f;}
__DEVICE__ float4 cmap(float a){a=clamp(a,0.0f,1.6f);return a*abs_f4(to_float4(_sinf(a),_sinf(a+0.4f),_sinf(a+1.7f),1.0f));}

__KERNEL__ void LittleExplodersFuse(float4 O, float2 U, float iTime, float2 iResolution, sampler2D iChannel0)
{
  U+=0.5f;

  O=U.y>1.0f?cmap(BG(U*0.01f,iTime)+BG(swi2(U,y,x)*0.03f,iTime)*0.2f+texture(iChannel0,U/iResolution).x):to_float4_s(0);

  SetFragmentShaderComputedColor(O);
}