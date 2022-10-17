
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1

#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

//Water "Physics"

//Random hash from here:
//https://www.shadertoy.com/view/4djSRW
__DEVICE__ float2 hash21(float p){
  float3 p3 = fract_f3(to_float3_s(p) * to_float3(0.1031f, 0.1030f, 0.0973f));
  p3 += dot(p3, swi3(p3,y,z,x) + 33.33f);
    return fract_f2((swi2(p3,x,x)+swi2(p3,y,z))*swi2(p3,z,y));
}

//Comment this line out to try the simulation without automatic drops
#define AUTO

//Ripples code based on a few old ripples tutorials
//#define T(a,b) texelFetch(iChannel0,to_int2(U)+to_int2(a,b),0)
#define T(a,b) texture(iChannel0,(make_float2((int)(U).x+(int)(a),(int)(U).y+(int)b)+0.5f)/R)

#define C(U) _tex2DVecN(iChannel2, (U).x/R.x,(U).y/R.y,15)

__KERNEL__ void GolfBallsAndBigWaterRipplesFuse__Buffer_A(float4 O, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Power, -1.0f, 10.0f, 1.0f);
    
    CONNECT_BUTTON0(Modus, 1, Icks, Yps, XY, Weh, Erase);
    
    U+=0.5f;

    float h=(T( 0, 1).x+
             T( 0,-1).x+
             T( 1, 0).x+
             T(-1, 0).x)/2.0f,
          t=0.99f*(h-T(0,0).y);
    
    if((iMouse.z>0.5f&&10.0f-length(swi2(iMouse,x,y)-U)>0.0f)
#ifdef AUTO       
       ||(iFrame%20==0&&10.0f-length(iResolution*hash21((float)(iFrame))-U)>0.0f)
#endif
      ){
      t=1.0f;
    }
    //float4 B=texelFetch(iChannel1,to_int2(U),0);
    float4 B=texture(iChannel1,(make_float2((int)(U).x,(int)(U).y)+0.5f)/R);
    O=(float)(iFrame>10)*
             to_float4(t-_fmaxf(4.0f-length(swi2(B,x,y)-U),0.0f)*length(swi2(B,z,w)+to_float2_s(0.001f))/200.0f
             ,T(0,0).x
             ,0
             ,0);

    if (Blend1>0.0f)
    {
      float4 tex = C(U);
      if (tex.w != 0.0f)    
      {
        tex = tex*Power;
        if ((int)Modus & 2)  O.x = _mix(O.x,tex.x,Blend1);//,O.y = _mix(O.y,-tex.x,Blend1);
        if ((int)Modus & 4)  O.y = _mix(O.y,tex.y,Blend1);
        if ((int)Modus & 8)  O.x = _mix(O.x,tex.x,Blend1), O.y = _mix(O.y,tex.y,Blend1);
        if ((int)Modus & 16) O.x = _mix(O.x,0.0f,Blend1);
        if ((int)Modus & 32) O.x = _mix(O.x,1.0f*Power,Blend1), O.y = _mix(O.y,1.0f*Power,Blend1);//O = to_float4(0.0f,0.0f,1.0f,-1.0f);
      }  
    } 

  if (Reset)  O=to_float4_s(0.0f);


  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1


//Particle storage and updating

#define Pspawn 20.0f
#define Pgrid 5.0f

//Similar particle implementation to:
//https://www.shadertoy.com/view/MlVfDR

//#define A(U) texelFetch(iChannel0,to_int2(U),0).x
#define A(U) texture(iChannel0, (make_float2((int)(U).x,(int)(U).y)+0.5f)/R).x
//#define B(a,b) texelFetch(iChannel1,to_int2(U)+to_int2(a,b),0)
#define B(a,b) texture(iChannel1,(make_float2((int)(U).x+(int)(a),(int)(U).y+(int)b)+0.5f)/R)

#define dA(U,R) A(U+R)-A(U-R)
#define DB(U) length(B(U).xy-U)

#define N(U,A,B) if(length(U-swi2(B,x,y))<length(U-swi2(A,x,y))){A = B;}

__KERNEL__ void GolfBallsAndBigWaterRipplesFuse__Buffer_B(float4 O, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    U+=0.5f;

    O  =  B( 0, 0) ;
    N(U,O,B( 0, 1));
    N(U,O,B( 1, 0));
    N(U,O,B( 0,-1));
    N(U,O,B(-1, 0));
    N(U,O,B( 2, 2));
    N(U,O,B( 2,-2));
    N(U,O,B(-2, 2));
    N(U,O,B(-2,-2));
    if(iFrame==0||(length(U-swi2(O,x,y))>Pspawn) || Reset){
        float2 tmp = round(U/Pgrid)*Pgrid; 
        O=to_float4(tmp.x,tmp.y,0.0f,0.0f);
    }
    swi2S(O,z,w, swi2(O,z,w) - to_float2(dA(swi2(O,x,y),to_float2(1,0)),
                                         dA(swi2(O,x,y),to_float2(0,1))));
    //swi2(O,w,z)*=0.99f;
    O.w*=0.99f;
    O.z*=0.99f;
    //swi2(O,x,y)+=swi2(O,z,w);
    O.x+=O.z;
    O.y+=O.w;


  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1


//Visualize Both Buffers

//inspired by these two excellent shaders
//https://www.shadertoy.com/view/wsfGzS
//https://www.shadertoy.com/view/tsBSWh

#define l normalize(to_float2_s(1))

//#define A(U) texelFetch(iChannel0,to_int2(U),0).x  // BufferB
//#define B(U) texelFetch(iChannel1,to_int2(U),0).xy // anders als BufferB

#define BI(U) swi2(texture(iChannel1, (make_float2((int)(U).x,(int)(U).y)+0.5f)/R),x,y) //statt B(U)

#define dA(U,R) A(U+R)-A(U-R)
#define DBI(U) length(BI(U)-U)
#define dDBI(U,R) DBI(U+R)-DBI(U-R)

__KERNEL__ void GolfBallsAndBigWaterRipplesFuse(float4 O, float2 U, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
  CONNECT_CHECKBOX1(Textur, 0); 
  CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
  CONNECT_SLIDER2(PowerTex, 0.0f, 2.0f, 1.0f);
  CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);
  
  U+=0.5f;

  float dAdBI = dot(
              to_float2(dA(U,to_float2(1,0)),
                        dA(U,to_float2(0,1))),
              l)+
             (0.5f*dot(
              normalize(
              to_float2(dDBI(U,to_float2(1,0)),
                        dDBI(U,to_float2(0,1)))),
              l)
              +0.5f)
              *_fmaxf(1.0f-0.5f*length(U-BI(U)),0.0f);

  if (isnan(dAdBI)) O = to_float4(0.3f,0.3f,1.0f,1.0f);
  else              O = O=to_float4_aw(
                          to_float3(0.3f,0.3f,1.0f)+dAdBI,1.0f); 

#ifdef ORIGINAL
  O=to_float4_aw(
    to_float3(0.3f,0.3f,1.0f)+
          dot(
              to_float2(dA(U,to_float2(1,0)),
                        dA(U,to_float2(0,1))),
              l)+
             (0.5f*dot(
              normalize(
              to_float2(dDBI(U,to_float2(1,0)),
                        dDBI(U,to_float2(0,1)))),
              l)
              +0.5f)
              *_fmaxf(1.0f-0.5f*length(U-BI(U)),0.0f),
              1.0f);
#endif              
              
              
    if (Blend1>0.0f && Textur)
    {
      float4 tex = C(U);
      if (tex.w != 0.0f)    
      {
        tex = tex*PowerTex;
        O = _mix(O,tex,Blend1); //,O.y = _mix(O.y,-tex.x,Blend1);              
      }
    }
    Color.w-=0.5f;
    O += Color-0.5f;

  SetFragmentShaderComputedColor(O);
}