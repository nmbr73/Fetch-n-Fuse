
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer B' to iChannel0
// Connect Buffer A 'Texture: Blending' to iChannel1

#define A(u) _tex2DVecN(iChannel0, (u).x/iResolution.x, (u).y/iResolution.y, 15)
#define B(u) _tex2DVecN(iChannel1, (u).x/iResolution.x, (u).y/iResolution.y, 15)

__DEVICE__ float4 Blending( __TEXTURE2D__ channel, float2 uv, float4 Q, float Blend, float2 Par, float2 MulOff, int Modus, float2 U, float2 R)
{
   
    if (Blend > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = _tex2DVecN(channel, uv.x, uv.y, 15);

      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2)
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q.x = _mix(Q.x,(tex.x+MulOff.y)*MulOff.x,Blend);
          //swi3S(Q,x,y,w, _mix(swi3(Q,x,y,w),(swi3(tex,x,y,z)+MulOff.y)*MulOff.x,Blend));

        if ((int)Modus&4)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par , Blend));
          //swi2S(Q,x,y, _mix( swi2(Q,x,y),  Par, Blend));
          //swi3S(Q,x,y,z, _mix(swi3(Q,x,y,z), (swi3(tex,x,y,z)+MulOff.y)*MulOff.x, Blend));  
          //Q.y = _mix(Q.y,to_float4(Par.x,Par.y,(tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x),Blend);
          Q.y = _mix(Q.y,(tex.y+MulOff.y)*MulOff.x,Blend);
        
        
        if ((int)Modus&8)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par, Blend));
          //Q = _mix(Q,to_float4((tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x,Par.x,Par.y),Blend);
          //Q.z = _mix( Q.z,  (tex.x+MulOff.y)*MulOff.x, Blend);
          //swi2S(Q,z,w, _mix( swi2(Q,z,w), swi2(tex,x,y)*Par, Blend));
          Q.z = _mix(Q.z,(tex.z+MulOff.y)*MulOff.x,Blend);

        if ((int)Modus&16) 
          //swi2S(Q,z,w, _mix(swi2(Q,z,w),  swi2(tex,x,y)*Par, Blend));
          //Q = _mix(Q,to_float4(Par.x,Par.y,MulOff.x,MulOff.y),Blend);
          Q.w = _mix(Q.w,((tex.x+tex.y+tex.z)/3.0f+MulOff.y)*MulOff.x,Blend);
      }
      else
        if ((int)Modus&32) //Special
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
    }
  
  return Q;
}



__KERNEL__ void SimpleDetailedFluidFuse__Buffer_A(float4 fragColor, float2 u, float2 iResolution, int iFrame, sampler2D iChannel0)
{
  CONNECT_CHECKBOX0(Reset, 0);
  CONNECT_POINT0(Wall_UL, 0.05f, 0.05f);
  CONNECT_POINT1(Wall_OR, 0.95f, 0.95f);
  
  //Blending
  CONNECT_SLIDER3(Blend1, 0.0f, 1.0f, 0.0f);
  CONNECT_SLIDER4(Blend1Off, -10.0f, 10.0f, 0.0f);
  CONNECT_SLIDER5(Blend1Mul, -10.0f, 10.0f, 1.0f);
  CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
  CONNECT_POINT2(Par1, 0.0f, 0.0f);
  
  //CONNECT_SLIDER0(Level0, -1.0f, 1.0f, 0.0f);

    u+=0.5f;
float AAAAAAAAAAAAAA;
    float2 v = u/iResolution;
    float4 a = A(u);
  #ifdef ORG
    float2 m = swi2(a,x,y)                       //fluid velocity
               -to_float2(0,1)*0.01f             //gravity
               +float(v.x<0.05f)*to_float2(1,0)  //wall
               +float(v.y<0.05f)*to_float2(0,1)  //wall
               -float(v.x>0.95f)*to_float2(1,0)  //wall
               -float(v.y>0.95f)*to_float2(0,1); //wall
  #endif             
    float2 m = swi2(a,x,y)                       //fluid velocity
               -to_float2(0,1)*0.01f             //gravity
               +float(v.x<Wall_UL.x)*to_float2(1,0)  //wall
               +float(v.y<Wall_UL.y)*to_float2(0,1)  //wall
               -float(v.x>Wall_OR.x)*to_float2(1,0)  //wall
               -float(v.y>Wall_OR.y)*to_float2(0,1); //wall
  
  
    float s = 0.0f;
    float z = 4.0f;                //kernel convolution size
    for(float i=-z; i<=z; ++i){
    for(float j=-z; j<=z; ++j){
      float2 c = -m+to_float2(i,j);//translate the gaussian 2Dimage using the velocity
      s += _expf(-dot(c,c));       //calculate the gaussian 2Dimage
    }}
    
    if(s==0.0f) {s = 1.0f;}        //avoid division by zero
    s = 1.0f/s;
    
    if (isinf(s)) s = 0.0f;
    
    fragColor = to_float4(m.x,m.y,s,0);//velocity in .xy
                                   //convolution normalization in .z

    if(iFrame==0 || Reset)
    {
      fragColor = to_float4_s(0.0f);
    }
    
    if (isnan(fragColor.x)) fragColor.x = 0.0f;
    if (isnan(fragColor.y)) fragColor.y = 0.0f;
    if (isnan(fragColor.z)) fragColor.z = 0.0f;
    if (isnan(fragColor.w)) fragColor.w = 0.0f;
    
    
//    if (Blend1>0.0 && v.x > Wall_UL.x && v.x < Wall_OR.x && v.y > Wall_UL.y && v.y < Wall_OR.y) fragColor = Blending(iChannel1, u/iResolution, fragColor, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, u, iResolution);
//    fragColor.w=0.0f;
  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Texture: Blending' to iChannel2
// Connect Buffer B 'Previsualization: Buffer A' to iChannel1
// Connect Buffer B 'Previsualization: Buffer B' to iChannel0


__KERNEL__ void SimpleDetailedFluidFuse__Buffer_B(float4 fragColor, float2 u, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0);
    
    CONNECT_POINT0(Wall_UL, 0.05f, 0.05f);
    CONNECT_POINT1(Wall_OR, 0.95f, 0.95f);
    
    //Blending
    CONNECT_SLIDER3(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER5(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);

    u+=0.5f;
    float2 v = u/iResolution;
    
float BBBBBBBBBBBBBBBB;
    float4  o = to_float4_s(0);
    float z = 4.0f;                       //kernel convolution size
    for(float i=-z; i<=z; ++i){
    for(float j=-z; j<=z; ++j){
      float4  a = A(u+to_float2(i,j));    //old velocity in swi2(a,x,y), mass in a.z
      float4  b = B(u+to_float2(i,j));    //new velocity in swi2(b,x,y), normalization of convolution in .z
      float2  c = -1.0f*swi2(b,x,y)-to_float2(i,j);       //translate the gaussian 2Dimage
      float s = a.z*_expf(-dot(c,c))*b.z; //calculate the normalized gaussian 2Dimage multiplied by mass
      float2  e = c*(a.z-0.8f);           //fluid expands or atracts itself depending on mass
      swi2S(o,x,y, swi2(o,x,y) + s*(swi2(b,x,y)+e));              //sum all translated velocities
      o.z  += s;                         //sum all translated masses
    }}
    float tz = 1.0f/o.z;
    if(o.z==0.0f){tz = 0.0f;}            //avoid division by zero
    //swi2(o,x,y) *= tz;                 //calculate the average velocity
    o.x *= tz;                           //calculate the average velocity
    o.y *= tz;                           //calculate the average velocity
        
    if(iMouse.z>0.0f)                    //mouse click adds velocity
    {
        float2 m = 8.0f*(u-swi2(iMouse,x,y))/iResolution.y;
        o += to_float4(m.x,m.y,0,0)*0.1f*_expf(-dot(m,m));
    }
    if(iFrame==0 || Reset)
    {
        float2 m = 3.0f*(u-iResolution*0.5f)/iResolution.y;
        o = to_float4(0,0,1,1)*_expf(-dot(m,m));
    }
    fragColor = o;

    if (Blend1>0.0 && v.x > Wall_UL.x && v.x < Wall_OR.x && v.y > Wall_UL.y && v.y < Wall_OR.y) 
        fragColor = Blending(iChannel2, u/iResolution, fragColor, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, u, iResolution);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer B' to iChannel0


__KERNEL__ void SimpleDetailedFluidFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);
    
    CONNECT_SLIDER0(ColMul1, -1.0f, 10.0f, 4.0f);
    CONNECT_SLIDER1(ColMul2, -1.0f, 1.0f, 0.2f);
    CONNECT_SLIDER2(ColOff, -1.0f, 2.0f, 0.6f);

    float2 u = fragCoord/iResolution;
    float4 a = _tex2DVecN(iChannel0,u.x,u.y,15);
    fragColor = a.z*( sin_f4(a.x*ColMul1+to_float4(1,3,5,4))*ColMul2
                     +sin_f4(a.y*ColMul1+to_float4(1,3,2,4))*ColMul2+ColOff);


    if (isnan(fragColor.x)) fragColor.x = 0.0f;
    if (isnan(fragColor.y)) fragColor.y = 0.0f;
    if (isnan(fragColor.z)) fragColor.z = 0.0f;
    if (isnan(fragColor.w)) fragColor.w = 0.0f;

    fragColor.w = clamp( fragColor.w, 0.0f, 1.0f);

    fragColor = (fragColor + Color - 0.5f)*fragColor.w ;
    
    if(Color.w > 0.0f)
      fragColor.w = Color.w;


  SetFragmentShaderComputedColor(fragColor);
}