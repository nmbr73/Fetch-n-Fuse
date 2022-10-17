
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: Rock Tiles' to iChannel1
// Connect Buffer A 'Texture: Font 1' to iChannel2
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)



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



// I implemented three variants of the algorithm with different
// interpretations:
//
// VARIANT = 0: traditional
// VARIANT = 1: box fiter
// VARIANT = 2: high pass filter
//#define VARIANT 0

__DEVICE__ float Cell( in int2 p, float2 R, __TEXTURE2D__ iChannel0 )
{
    // do wrapping
    //int2 r = to_int2(textureSize(iChannel0, 0));
    int2 r = to_int2_cfloat(R);
    p = to_int2((p.x+r.x) % r.x, (p.y+r.y) % r.y);
    
    // fetch texel
    //return (texelFetch(iChannel0, p, 0 ).x > 0.5f ) ? texelFetch(iChannel0, p, 0 ).w : texelFetch(iChannel0, p, 0 ).w;    //?????????????????
    //return (texture(iChannel0, (make_float2(p)+0.5f)/R ).x > 0.5f ) ? texture(iChannel0, (make_float2(p)+0.5f)/R).w : texture(iChannel0, (make_float2(p)+0.5f)/R ).w;
    return texture(iChannel0, (make_float2(p)+0.5f)/R).w;
}

__DEVICE__ float hash1( float n )
{
    return fract(_sinf(n)*138.5453123f);
}

__KERNEL__ void HeatDiffusionFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    CONNECT_CHECKBOX0(Reset, 0);
    //CONNECT_BUTTON0(Variante, 0, Traditional , Box, Highpass, BTN4, BTN5);
    
        //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON1(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);
    
       
    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);
    
    Color -= 0.5f;
    
    
    fragCoord+=0.5f;
 
    int2 px = to_int2_cfloat( fragCoord );
    
//#if VARIANT==0

    float d0 = 0.5f;
    float d1 = 0.125f;
    float d2 = 0.0f;

  
    float k = 0.0f;
    float x = 0.0f;
    int n = 5;
    
    for(int i=-n;i<=n;i++){
        for(int j=-n;j<=n;j++){
            x = x+texture(iChannel1, to_float2((float)(i+n)+0.5f,(float)(j+n)+0.5f)/(float)(2*n+1)).x;
            }
        }
    
    for(int i=-n;i<=n;i++){
        for(int j=-n;j<=n;j++){
            k = k + Cell(px+to_int2((int)(i),(int)(j)),R,iChannel0)*(1.0f/x)*texture(iChannel1, to_float2((float)(i+n)+0.5f,(float)(j+n)+0.5f)/(float)(2*n+1)).x;        
            }
        }
        
    /*    
    float k =    d2*Cell(px+to_int2(-5,-5),R,iChannel0) + d1*Cell(px+to_int2(0,-5),R,iChannel0) + d2*Cell(px+to_int2(5,-5),R,iChannel0)
              +  d1*Cell(px+to_int2(-5, 0),R,iChannel0) + d0*Cell(px)                           + d1*Cell(px+to_int2(5, 0),R,iChannel0)
              +  d2*Cell(px+to_int2(-5, 5),R,iChannel0) + d1*Cell(px+to_int2(0, 5),R,iChannel0) + d2*Cell(px+to_int2(5, 5),R,iChannel0);
          */  
    float e = Cell(px,R,iChannel0);
    float f = k;
    
//#endif
    

    if( iFrame==0 || Reset ) f = 100.0f*step(0.99f, hash1(fragCoord.x*13.0f+hash1(fragCoord.y*71.1f)));
    
    float fx = fragCoord.x-iMouse.x;
    float fy = fragCoord.y-iMouse.y;
    float4 m = iMouse;
    if (sign_f(m.z)>0.0f){
      f = f + 10.0f*(1.0f-step(10.0f, (fx*fx+fy*fy)));
    }
  
  fragColor = to_float4_aw( swi3(texture(iChannel2,to_float2(f/4.0f,0.5f)),x,y,z)+swi3(Color,x,y,z), f );
   
  if (Blend1>0.0) fragColor = Blending(iChannel1, fragCoord/R, fragColor, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, fragCoord, R);
   
  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


// Conway's Game of Life
// https://iquilezles.org/articles/gameoflife
//
// State based simulation. Buffer A contains the simulated world,
// and it reads and writes to itself to perform the simulation.

__KERNEL__ void HeatDiffusionFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    fragCoord+=0.5f;
    fragColor = texture( iChannel0, (make_float2(to_int2_cfloat(fragCoord))+0.5f)/R );

  SetFragmentShaderComputedColor(fragColor);
}