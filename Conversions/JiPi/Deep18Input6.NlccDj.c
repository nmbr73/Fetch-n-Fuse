
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Texture: Blending' to iChannel1

#define A(u) _tex2DVecN(iChannel0,(u).x/iResolution.x, (u).y/iResolution.y, 15)

__DEVICE__ float4 Blending( __TEXTURE2D__ channel, float2 uv, float4 Q, float Blend, float2 Par, float2 MulOff, int Modus, float2 U, float2 R)
{

if (Blend > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = _tex2DVecN(channel,uv.x,uv.y,15);

      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2)
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
          
        if ((int)Modus&4)
          Q = _mix(Q,to_float4(Par.x,Par.y,(tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x),Blend);
        
         if ((int)Modus&8)
          Q = _mix(Q,to_float4((tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x,Par.x,Par.y),Blend);

        if ((int)Modus&16) 
          Q = _mix(Q,to_float4(Par.x,Par.y,MulOff.x,MulOff.y),Blend);
      }
      else
        if ((int)Modus&32) //Special
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
    }
  
  return Q;
}



__KERNEL__ void Deep18Input6Fuse__Buffer_A(float4 fragColor, float2 u, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{

    // 0: deep 18input 6
    // 1: deep 18input
    // 2: deep 29input
    // 3: deep 18input 5
    // 4: deep 18input 4
    // 5: deep 18input 2



    CONNECT_INTSLIDER0(Variante, 0, 5, 0);
    
    CONNECT_SLIDER4(StepDot, 0.0f, 1.0f, 0.2f);
    CONNECT_SLIDER5(Radius, 0.0f, 5.0f, 2.0f);
    
    if(Variante == 1 || Variante == 2) StepDot+=0.3f;
    if(Variante == 3) StepDot-=0.15f;
    if(Variante == 4) StepDot-=0.16f;
    if(Variante == 5) StepDot-=0.19f;
    

    //Blending
    CONNECT_SLIDER1(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);


    u+=0.5f;


    float4 a =  A(u+to_float2( 1, 0))
               +A(u+to_float2( 0, 1))
               +A(u+to_float2(-1, 0))
               +A(u+to_float2( 0,-1))
               +A(u+to_float2( 1, 1))
               +A(u+to_float2(-1, 1))
               +A(u+to_float2( 1,-1))
               +A(u+to_float2(-1,-1))
               +A(u+to_float2( 0, 0));
    uint s = (uint)(dot(a,to_float4(1,1,0,0)));
    float4 o = swi4(A(u+to_float2(0,0)),x,x,y,z);
     
         o.x = (float)(((+16U*8U*16U*8U* 8U
                         +16U*8U*16U*    7U
                         +16U*8U*        2U
                         +16U*           1U
                         +               0U)>>s)&1U);

    if(Variante == 1) o.x=float(((16U*8U*16U*8U* 8U + 7U*16U+14U)>>s)&1U);


    if(Variante == 2)
    {
                 a =  A(u+to_float2( 1, 0))
                     +A(u+to_float2( 0, 1))
                     +A(u+to_float2(-1, 0))
                     +A(u+to_float2( 0,-1))
                     +A(u+to_float2( 1, 1))
                     +A(u+to_float2(-1, 1))
                     +A(u+to_float2( 1,-1))
                     +A(u+to_float2(-1,-1))
                     +A(u+to_float2( 0, 0))
                     
                     +A(u+to_float2( 2,-2))
                     +A(u+to_float2( 2,-1))
                     +A(u+to_float2( 2, 0))
                     +A(u+to_float2( 2, 1))
                     +A(u+to_float2( 2, 2))
                     +A(u+to_float2( 1, 2))
                     +A(u+to_float2( 0, 2))
                     +A(u+to_float2(-1, 2))
                     +A(u+to_float2(-2, 2))
                     +A(u+to_float2(-2, 1))
                     +A(u+to_float2(-2, 0))
                     +A(u+to_float2(-2,-1))
                     +A(u+to_float2(-2,-2))
                     +A(u+to_float2(-1,-2))
                     +A(u+to_float2( 0,-2))
                     +A(u+to_float2( 1,-2))
                     
                     +A(u+to_float2( 3, 0))
                     +A(u+to_float2( 0, 3))
                     +A(u+to_float2(-3, 0))
                     +A(u+to_float2( 0,-3));
    uint s = (uint)(dot(a,to_float4(1,0,0,0)));
         o = swi4(A(u+to_float2(0,0)),x,x,y,z);
         o.x = (float)(((+16U*8U*16U*8U* (1U<<12U)
                        +16U*8U*16U*    0U
                        +16U*8U*        0U
                        +16U*           0U
                        +               15U)>>s)&1U);
                        
    }


    if(Variante == 3)
               o.x=(float)(((+16U*8U*16U*8U* 3U
                             +16U*8U*16U*    7U
                             +16U*8U*        0U
                             +16U*           3U
                             +               15U)>>s)&1U);


    if( Variante == 4)
         o.x = (float)(((+16U*8U*16U*8U* 5U
                         +16U*8U*16U*    7U
                         +16U*8U*        12U
                         +16U*           1U
                         +               0U)>>s)&1U);
                         
    if( Variante == 5)
         o.x = (float)(((+16U*8U*16U*8U* 1U 
                         +16U*8U*16U*    2U
                         +16U*8U*        8U
                         +16U*           0U
                         +               11U)>>s)&1U);

    if(iFrame==0||iMouse.z>0.5f)
    {
      float2 v = 1.0f*(u        *Radius-iResolution)/iResolution.y;
      float2 m = 1.0f*(swi2(iMouse,x,y)*Radius-iResolution)/iResolution.y;
      
      o = _floor(fract_f4(_cosf(dot(u,to_float2(1.76543f,iTime+22.5363f)))*to_float4(2467.5678f,
                                                                                      3467.5678f,
                                                                                      4467.5678f,
                                                                                      5467.5678f))+0.5f);
      if( Variante == 1 || Variante == 5 )  
        o = _floor(fract_f4(_cosf(dot(v,to_float2(234.76543,iTime+22.5363)))*to_float4(2467.5678,
                                                                                       3467.5678,
                                                                                       4467.5678,
                                                                                       5467.5678))+0.5f);        
                      
      if( Variante == 2)  
        o = _floor(fract_f4(_cosf(dot(v,to_float2(234.76543,iTime+22.5363)))*to_float4(2467.5678,
                                                                                       3467.5678,
                                                                                       4467.5678,
                                                                                       5467.5678))+0.1f); 
                                                                                      
                                                                                              
        
                      
      o*= step(dot(v,v),dot(m,m)*StepDot); //0.2f);
      //o*= step(dot(v,v),dot(m,m)*0.2f);
    }
    fragColor = o;

    if (Blend1>0.0) fragColor = Blending(iChannel1, u/iResolution, fragColor, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, u, iResolution);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


//#define A(u) texture(iChannel0,(u)/iResolution)
__KERNEL__ void Deep18Input6Fuse(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel0)
{
  
    CONNECT_INTSLIDER0(Variante, 0, 5, 0);
  

    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);
    CONNECT_SLIDER0(AlphaBKG, -1.0f, 1.0f, 0.0f);



    float4 a = A(fragCoord);
    if((iFrame%3)==0){a = swi4(a,x,y,z,w);}
    if((iFrame%3)==1){a = swi4(a,y,z,x,w);}
    if((iFrame%3)==2){a = swi4(a,z,x,y,w);}

    if (Variante==2)
    {
      float2 _a = swi2(A(fragCoord),x,y);
      if((iFrame&1)==0){_a = swi2(_a,x,y);}
      if((iFrame&1)==1){_a = swi2(_a,y,x);}
      a = to_float4_s(dot(_a,to_float2(1,2)/3.0f));
    }

    if(a.w != 0.0f) a+=Color-0.5f, a.w = Color.w;  
    else            a.w = AlphaBKG; 
    

  fragColor = a;

  SetFragmentShaderComputedColor(fragColor);
}