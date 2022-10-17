
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Texture: Blending' to iChannel1

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
#define A(u) texture(iChannel0,(make_float2(to_int2_cfloat(u))+0.5f)/R)

__DEVICE__ float4 Blending( __TEXTURE2D__ channel, float2 uv, float4 Q, float Blend, float2 Par, float2 MulOff, int Modus, float2 U, float2 R)
{
   
    if (Blend > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(channel,uv);

      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2)
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
          //swi3S(Q,x,y,w, _mix(swi3(Q,x,y,w),(swi3(tex,x,y,z)+MulOff.y)*MulOff.x,Blend));

        if ((int)Modus&4)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par , Blend));
          //swi2S(Q,x,y, _mix( swi2(Q,x,y),  Par, Blend));
          //swi3S(Q,x,y,z, _mix(swi3(Q,x,y,z), (swi3(tex,x,y,z)+MulOff.y)*MulOff.x, Blend));  
          Q = _mix(Q,to_float4(Par.x,Par.y,(tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x),Blend);
        
        
        if ((int)Modus&8)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par, Blend));
          Q = _mix(Q,to_float4((tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x,Par.x,Par.y),Blend);
          //Q.z = _mix( Q.z,  (tex.x+MulOff.y)*MulOff.x, Blend);
          //swi2S(Q,z,w, _mix( swi2(Q,z,w), swi2(tex,x,y)*Par, Blend));

        if ((int)Modus&16) 
          //swi2S(Q,z,w, _mix(swi2(Q,z,w),  swi2(tex,x,y)*Par, Blend));
          Q = _mix(Q,to_float4(Par.x,Par.y,MulOff.x,MulOff.y),Blend);
      }
      else
        if ((int)Modus&32) //Special
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
    }
  
  return Q;
}


__KERNEL__ void ConvolutionmazeJipiFuse__Buffer_A(float4 O, float2 u, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_CHECKBOX1(StartTex, 0);
    
    CONNECT_COLOR0(k1,  1.0f, 3.24f,  6.0f,   0.2f);
    CONNECT_COLOR1(k2, -1.0f, 1.07f, -0.03f, -0.03f);
    CONNECT_SLIDER0(DM, -1.0f, 1.0f, 0.0f);
    
        //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);
    
    
    u+=0.5f;
    
    float z =8.0f,dm=1.0f+DM;
//    float4 k1= to_float4(1.0f,3.24f,6.0f,0.2f), 
//           k2= to_float4(-1,1.07f,-0.03f,-0.03f);
    float4 s=to_float4_s(0), b=s;
  
    for(float i = 0.0f; i<=z;i+=1.0f) for(float j = 0.0f; j<=z;j+=1.0f){
        float  l=(i*i+j*j)/z/z;
        if(l<=1.0f) {
            float4 e=exp_f4(-l*k1);
            s+= (i==0.0f?1.0f:2.0f)*(j==0.?1.0f:2.0f)* e;
            for(float i1 =-1.0f;i1<=1.0f;i1+=2.0f) 
              for(float j1 =-1.0f;j1<=1.0f;j1+=2.0f){  //symmetry
                float d = A(u+to_float2(i*i1,j*j1)).x;             
                b+=d*e;
                if(d>=1.0f) dm=_fminf(dm,l);
            }
        }
    }    
float AAAAAAAA;    
    if(StartTex)
    {
      O=to_float4_s(0.0f); 
      float tex = texture(iChannel1, u/R).w;
      if(tex>0.0f)
      {
        O=to_float4(1.0f, b.x/s.x, b.y/s.y*6.0f, dm*z) * 0.01;
      }

    }
    else
    
      O=to_float4(1.0f, b.x/s.x, b.y/s.y*6.0f, dm*z)* 
                 (((iFrame%1200)==0 || Reset) ? 
                      smoothstep(0.01f,0.0f,_fabs(0.1f+0.01f*_sinf(u.x)-length((2.0f*u-swi2(R,x,y))/R.y)))
                      :clamp( z* dot(b/s,k2),0.0f,1.0f));  


    if (Blend1>0.0) O = Blending(iChannel1, u/R, O, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, u, R);    

                    
    if(iMouse.z>0.0f) O =_mix(O,to_float4_s(1.0f)-500.0f*O, _fmaxf(0.0f,0.1f-length(2.0f*(u-swi2(iMouse,x,y))/R.y))); 


  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


//forked from https://www.shadertoy.com/view/stfGzr by lomateron 
__KERNEL__ void ConvolutionmazeJipiFuse(float4 O, float2 U, float2 iResolution, sampler2D iChannel0)
{
    CONNECT_CHECKBOX1(StartTex, 0);
  
    CONNECT_COLOR2(Color1, 0.9f, 5.2f, 3.5f, 0.0f);
    CONNECT_COLOR3(ColorBKG, 0.2f, 0.2f, 0.2f, 0.2f);
    CONNECT_SLIDER1(Border, -1.0f, 2.0f, 0.7f);
    
    CONNECT_CHECKBOX2(BKGAlpha, 0);
    
    U+=0.5f;
    
    //float4 v=texelFetch(iChannel0,to_int2(U),0);
    float4 v=A(U);
    
    O = (0.3f+Border*smoothstep(0.01f,0.0f,v.w) )  //border    
     * (sin_f4(     
         v*Color1                                //to_float4(0.9f, 5.2f,3.5f,0)  //colors: base, node, vein
         +ColorBKG                               //to_float4_s(0.2f)                      //background
        )*0.5f+0.5f); 

  if(StartTex) O = texture(iChannel1, U/R);

  if(BKGAlpha) O *= v.x>0.0f ? 1.0f:0.0f;

  SetFragmentShaderComputedColor(O);
}