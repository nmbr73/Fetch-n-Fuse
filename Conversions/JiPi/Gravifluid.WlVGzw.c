
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)
#define R iResolution



//#define pixel(a, p) texture(a, p/to_float2(textureSize(a,0)))
#define pixel(a, p) texture(a, p/R)
//#define texel(a, p) texelFetch(a, to_int2(p-0.5f),0)
#define texel(a, p) texture(a, (make_float2(to_int2_cfloat((p)-0.5f))+0.5f)/R)

#define s2d iResolution
#define ch0 iChannel0
#define ch1 iChannel1
#define ch2 iChannel2
#define ch3 iChannel3

//#define dt 0.5f

#ifdef xxx
const float scale = 0.6f;
//fluid 1 density 
const float a1 = 0.8f;
//fluid 2 density
const float a2 = 1.0f;

#define sign to_float4(1.0f,1.0f,1.0f,1.0f)

//set to 0 if you want a gas-like behaviour
//fluid 1 fluid-like/gas-like regulator 
const float b1 = 0.99f;
//fluid 2 fluid-like/gas-like regulator 
const float b2 = 0.0f;

//interaction energy cost
//const float ie = -0.1f;

//initial conditions for amplitudes
const float amp = 0.6f;
const float4 fluid1_Q = amp*to_float4(a1,0,0,0);
const float4 fluid1_Qv = amp*to_float4(0,a1,0,0); //minus for antifluid
//const float4 fluid2_Q = 0.0f*amp*to_float4(0,0,a2,0);
//const float4 fluid2_Qv = 0.0f*amp*to_float4(0,0,0,a2);
const float mouser = 25.0f;
const float initr = 20.0f;
#endif

__DEVICE__ float sq(float x) { return x*x; }
__DEVICE__ float cb(float x) { return x*x*x; }
__DEVICE__ float sq(float2 x){ return dot(x,x); }

//const float pressure = 0.001f;

//wave potential
__DEVICE__ float P(float4 Q, float2 p, float a1, float a2, float b1, float b2, float scale)
{
    //fluid 1 amplitude
    float fd1 = length(swi2(Q,x,y));
    //fluid 2 amplitude
    float fd2 = length(swi2(Q,z,w));
    
    //liquifier term 1
    float liq1 = 1.0f - b1*_expf(-3.0f*sq(fd1-a1));      
    //liquifier term 2
    float liq2 = 1.0f - b2*_expf(-3.0f*sq(fd2-a2));     
    float grav = 0.002f*length(p - to_float2(400,225));
        
    float E =(scale*(cb(fd1)*liq1)*(1.0f - _tanhf(0.15f*Q.z)) + 0.000f*sq(Q.z));
    return 20.0f*_tanhf(0.05f*E);
}

//force
#define d 0.001
__DEVICE__ float4 F(float4 Q, float2 p, float a1, float a2, float b1, float b2, float scale)
{
    float3 dx = 0.5f*to_float3(-d,0.0f,d);
    return to_float4(P(Q + swi4(dx,z,y,y,y), p,a1,a2,b1,b2,scale) - P(Q + swi4(dx,x,y,y,y), p,a1,a2,b1,b2,scale),
                     P(Q + swi4(dx,y,z,y,y), p,a1,a2,b1,b2,scale) - P(Q + swi4(dx,y,x,y,y), p,a1,a2,b1,b2,scale),
                     P(Q + swi4(dx,y,y,z,y), p,a1,a2,b1,b2,scale) - P(Q + swi4(dx,y,y,x,y), p,a1,a2,b1,b2,scale),
                     P(Q + swi4(dx,y,y,y,z), p,a1,a2,b1,b2,scale) - P(Q + swi4(dx,y,y,y,x), p,a1,a2,b1,b2,scale))/d;
}

//Laplacian operator
__DEVICE__ float4 Laplace(__TEXTURE2D__ ch, float2 p, float2 R)
{
    float3 dx = to_float3(-1,0.0f,1);
    return texel(ch, p+swi2(dx,x,y))+texel(ch, p+swi2(dx,y,x))+texel(ch, p+swi2(dx,z,y))+texel(ch, p+swi2(dx,y,z))-4.0f*texel(ch, p);
}

__DEVICE__ float2 Grad(__TEXTURE2D__ ch, float2 p, float2 R)
{
    float3 dx = to_float3(-1,0.0f,1);
    return to_float2(length(texel(ch, p+swi2(dx,z,y))), length(texel(ch, p+swi2(dx,y,z)))) - length(texel(ch, p));
}

__DEVICE__ float dborder(float2 x, float2 s)
{
    return _fminf(_fminf(_fminf(x.x, x.y), s.x-x.x),s.y-x.y);
}


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


// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer C' to iChannel0
// Connect Buffer A 'Previsualization: Buffer D' to iChannel1
// Connect Buffer A 'Cubemap: Forest Blurred_0' to iChannel2
// Connect Buffer A 'Texture: Blending' to iChannel3


__DEVICE__ void mouse(inout float4 *Q, inout float4 *Qv, float2 p, float4 iMouse, float mouser, float4 fluid1_Q, float4 fluid1_Qv)
{
    if(iMouse.z > 0.0f)
    {
        float f1 = _expf(-sq((p-swi2(iMouse,x,y))/mouser));
        *Q += 0.1f*fluid1_Q*f1;
        *Qv += 0.1f*fluid1_Qv*f1;
    }
}

__KERNEL__ void GravifluidFuse__Buffer_A(float4 Q, float2 p, float4 iMouse, float2 iResolution, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0);
    p+=0.5f;

    CONNECT_SLIDER2(scale, -1.0f, 2.0f, 0.6f);
    CONNECT_SLIDER3(a1, -1.0f, 2.0f, 0.8f);
    CONNECT_SLIDER4(a2, -1.0f, 2.0f, 1.0f);
    CONNECT_SLIDER5(b1, -1.0f, 2.0f, 0.99f);
    CONNECT_SLIDER6(b2, -1.0f, 2.0f, 0.0f);

    CONNECT_SLIDER7(amp, -1.0f, 2.0f, 0.6f);
    CONNECT_SLIDER8(mouser, -1.0f, 50.0f, 25.0f);
    CONNECT_SLIDER9(initr, -1.0f, 50.0f, 20.0f);
    CONNECT_SLIDER10(dt, -1.0f, 2.0f, 0.5f);
    
        //Blending
    CONNECT_SLIDER11(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER12(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER13(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);
    

//    const float scale = 0.6f;
    //fluid 1 density 
//    const float a1 = 0.8f;
    //fluid 2 density
//    const float a2 = 1.0f;

    #define sign to_float4(1.0f,1.0f,1.0f,1.0f)

    //set to 0 if you want a gas-like behaviour
    //fluid 1 fluid-like/gas-like regulator 
//    const float b1 = 0.99f;
    //fluid 2 fluid-like/gas-like regulator 
//    const float b2 = 0.0f;

    //interaction energy cost
    //const float ie = -0.1f;

    //initial conditions for amplitudes
//    const float amp = 0.6f;
    const float4 fluid1_Q = amp*to_float4(a1,0,0,0);
    const float4 fluid1_Qv = amp*to_float4(0,a1,0,0); //minus for antifluid
    //const float4 fluid2_Q = 0.0f*amp*to_float4(0,0,a2,0);
    //const float4 fluid2_Qv = 0.0f*amp*to_float4(0,0,0,a2);
//    const float mouser = 25.0f;
//    const float initr = 20.0f;


    //get old value
    Q = texel(ch0, p);
    float4 Qv = texel(ch1, p);
    
    Q += Qv*dt + to_float4(0.0001f,0.0001f,0.001f,0.001f)*Laplace(ch0, p, R);
   
    mouse(&Q,&Qv,p,iMouse,mouser,fluid1_Q, fluid1_Qv);
    
    Q.z *= 1.0f-0.1f*_expf(-0.002f*sq(dborder(p, s2d)));

    if (Blend1>0.0) Q = Blending(iChannel3, p/R, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, p, R);

    
    if(iFrame < 1 || Reset) 
    {
        float f1 = step(sq((p-s2d*0.3f)/initr),1.0f);
        float f2 = step(sq((p-s2d*0.7f)/initr),1.0f);
        Q = fluid1_Q*f1*(1.0f-f2) + fluid1_Q*f2*(1.0f-f1);
    }

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer D' to iChannel1
// Connect Buffer B 'Cubemap: Forest Blurred_0' to iChannel2
// Connect Buffer B 'Texture: Blending' to iChannel3


__KERNEL__ void GravifluidFuse__Buffer_B(float4 Qv, float2 p, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0);
    p+=0.5f;
    
    CONNECT_SLIDER2(scale, -1.0f, 2.0f, 0.6f);
    CONNECT_SLIDER3(a1, -1.0f, 2.0f, 0.8f);
    CONNECT_SLIDER4(a2, -1.0f, 2.0f, 1.0f);
    CONNECT_SLIDER5(b1, -1.0f, 2.0f, 0.99f);
    CONNECT_SLIDER6(b2, -1.0f, 2.0f, 0.0f);

    CONNECT_SLIDER7(amp, -1.0f, 2.0f, 0.6f);
    CONNECT_SLIDER8(mouser, -1.0f, 50.0f, 25.0f);
    CONNECT_SLIDER9(initr, -1.0f, 50.0f, 20.0f);
    CONNECT_SLIDER10(dt, -1.0f, 2.0f, 0.5f);
    
            //Blending
    CONNECT_SLIDER14(Blend2, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER15(Blend2Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER16(Blend2Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON1(Modus2, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT3(Par2, 0.0f, 0.0f);

//    const float scale = 0.6f;
    //fluid 1 density 
//    const float a1 = 0.8f;
    //fluid 2 density
//    const float a2 = 1.0f;

    #define sign to_float4(1.0f,1.0f,1.0f,1.0f)

    //set to 0 if you want a gas-like behaviour
    //fluid 1 fluid-like/gas-like regulator 
//    const float b1 = 0.99f;
    //fluid 2 fluid-like/gas-like regulator 
//    const float b2 = 0.0f;

    //interaction energy cost
    //const float ie = -0.1f;

    //initial conditions for amplitudes
//    const float amp = 0.6f;
    const float4 fluid1_Q = amp*to_float4(a1,0,0,0);
    const float4 fluid1_Qv = amp*to_float4(0,a1,0,0); //minus for antifluid
    //const float4 fluid2_Q = 0.0f*amp*to_float4(0,0,a2,0);
    //const float4 fluid2_Qv = 0.0f*amp*to_float4(0,0,0,a2);
//    const float mouser = 25.0f;
//    const float initr = 20.0f;

    //get old value
    float4 Q = texel(ch0, p);
    Qv = texel(ch1, p);
    
    Qv += dt*(Laplace(ch0, p, R) - sign*F(Q, p,a1,a2,b1,b2,scale));
    
    mouse(&Q,&Qv,p,iMouse,mouser,fluid1_Q, fluid1_Qv);
    
    if (Blend2>0.0) Qv = Blending(iChannel3, p/R, Qv, Blend2, Par2, to_float2(Blend2Mul,Blend2Off), Modus2, p, R);
    
    if(iFrame < 1 || Reset) 
    {
        float f1 = step(sq((p-s2d*0.3f)/initr),1.0f);
        float f2 = step(sq((p-s2d*0.7f)/initr),1.0f);
        Qv = fluid1_Qv*f1*(1.0f-f2) + fluid1_Qv*f2*(1.0f-f1);
    }


  SetFragmentShaderComputedColor(Qv);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1


__KERNEL__ void GravifluidFuse__Buffer_C(float4 Q, float2 p, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0);
    p+=0.5f;
    
    CONNECT_SLIDER2(scale, -1.0f, 2.0f, 0.6f);
    CONNECT_SLIDER3(a1, -1.0f, 2.0f, 0.8f);
    CONNECT_SLIDER4(a2, -1.0f, 2.0f, 1.0f);
    CONNECT_SLIDER5(b1, -1.0f, 2.0f, 0.99f);
    CONNECT_SLIDER6(b2, -1.0f, 2.0f, 0.0f);

    CONNECT_SLIDER7(amp, -1.0f, 2.0f, 0.6f);
    CONNECT_SLIDER8(mouser, -1.0f, 50.0f, 25.0f);
    CONNECT_SLIDER9(initr, -1.0f, 50.0f, 20.0f);
    CONNECT_SLIDER10(dt, -1.0f, 2.0f, 0.5f);

//    const float scale = 0.6f;
    //fluid 1 density 
//    const float a1 = 0.8f;
    //fluid 2 density
//    const float a2 = 1.0f;

    #define sign to_float4(1.0f,1.0f,1.0f,1.0f)

    //set to 0 if you want a gas-like behaviour
    //fluid 1 fluid-like/gas-like regulator 
//    const float b1 = 0.99f;
    //fluid 2 fluid-like/gas-like regulator 
//    const float b2 = 0.0f;

    //interaction energy cost
    //const float ie = -0.1f;

    //initial conditions for amplitudes
//    const float amp = 0.6f;
    const float4 fluid1_Q = amp*to_float4(a1,0,0,0);
    const float4 fluid1_Qv = amp*to_float4(0,a1,0,0); //minus for antifluid
    //const float4 fluid2_Q = 0.0f*amp*to_float4(0,0,a2,0);
    //const float4 fluid2_Qv = 0.0f*amp*to_float4(0,0,0,a2);
//    const float mouser = 25.0f;
//    const float initr = 20.0f;


    //get old value
    Q = texel(ch0, p);
    float4 Qv = texel(ch1, p);
    
    Q += Qv*dt + 0.0005f*Laplace(ch0, p, R);
   
    mouse(&Q,&Qv,p,iMouse,mouser,fluid1_Q, fluid1_Qv);
    
    if(iFrame < 1) 
    {
        float f1 = step(sq((p-s2d*0.3f)/initr),1.0f);
        float f2 = step(sq((p-s2d*0.7f)/initr),1.0f);
        Q = fluid1_Q*f1*(1.0f-f2) + fluid1_Q*f2*(1.0f-f1);
    }


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer B' to iChannel1
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0

__KERNEL__ void GravifluidFuse__Buffer_D(float4 Qv, float2 p, float2 iResolution, float4 iMouse, int iFrame)
{

    CONNECT_CHECKBOX0(Reset, 0);
       
    p+=0.5f;

    CONNECT_SLIDER2(scale, -1.0f, 2.0f, 0.6f);
    CONNECT_SLIDER3(a1, -1.0f, 2.0f, 0.8f);
    CONNECT_SLIDER4(a2, -1.0f, 2.0f, 1.0f);
    CONNECT_SLIDER5(b1, -1.0f, 2.0f, 0.99f);
    CONNECT_SLIDER6(b2, -1.0f, 2.0f, 0.0f);

    CONNECT_SLIDER7(amp, -1.0f, 2.0f, 0.6f);
    CONNECT_SLIDER8(mouser, -1.0f, 50.0f, 25.0f);
    CONNECT_SLIDER9(initr, -1.0f, 50.0f, 20.0f);
    CONNECT_SLIDER10(dt, -1.0f, 2.0f, 0.5f);
    
//    const float scale = 0.6f;
    //fluid 1 density 
//    const float a1 = 0.8f;
    //fluid 2 density
//    const float a2 = 1.0f;

    #define sign to_float4(1.0f,1.0f,1.0f,1.0f)

    //set to 0 if you want a gas-like behaviour
    //fluid 1 fluid-like/gas-like regulator 
//    const float b1 = 0.99f;
    //fluid 2 fluid-like/gas-like regulator 
//    const float b2 = 0.0f;

    //interaction energy cost
    //const float ie = -0.1f;

    //initial conditions for amplitudes
//    const float amp = 0.6f;
    const float4 fluid1_Q = amp*to_float4(a1,0,0,0);
    const float4 fluid1_Qv = amp*to_float4(0,a1,0,0); //minus for antifluid
    //const float4 fluid2_Q = 0.0f*amp*to_float4(0,0,a2,0);
    //const float4 fluid2_Qv = 0.0f*amp*to_float4(0,0,0,a2);
//    const float mouser = 25.0f;
//    const float initr = 20.0f;

    //get old value
    float4 Q = texel(ch0, p);
    Qv = texel(ch1, p);
    
     Qv += dt*(Laplace(ch0, p, R) - sign*F(Q, p,a1,a2,b1,b2,scale));
    
    mouse(&Q,&Qv,p,iMouse,mouser,fluid1_Q, fluid1_Qv);
    
    if(iFrame < 1) 
    {
        float f1 = step(sq((p-s2d*0.3f)/initr),1.0f);
        float f2 = step(sq((p-s2d*0.7f)/initr),1.0f);
        Qv = fluid1_Qv*f1*(1.0f-f2) + fluid1_Qv*f2*(1.0f-f1);
    }


  SetFragmentShaderComputedColor(Qv);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: Forest Blurred_0' to iChannel2
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer D' to iChannel1


// Fork of "Oil and water" by michael0884. https://shadertoy.com/view/wtVGzW
// 2020-01-09 01:38:11

__KERNEL__ void GravifluidFuse(float4 fragColor, float2 p, float2 iResolution)
{
  
    CONNECT_CHECKBOX1(QvOn, 0);
    CONNECT_COLOR0(Color1, 1.0f, 1.0f, 0.0f, 1.0f);
    CONNECT_COLOR1(Color2, 0.0f, 0.3f, 1.0f, 1.0f);
    CONNECT_SLIDER0(Level_fd1, -1.0f, 10.0f, 1.0f);
    CONNECT_SLIDER1(Level_Qv, -1.0f, 50.0f, 10.0f);

    CONNECT_SLIDER2(scale, -1.0f, 2.0f, 0.6f);
    CONNECT_SLIDER3(a1, -1.0f, 2.0f, 0.8f);
    CONNECT_SLIDER4(a2, -1.0f, 2.0f, 1.0f);
            
    p+=0.5f;
    
    //const float scale = 0.6f;
    //fluid 1 density 
    //const float a1 = 0.8f;
    //fluid 2 density
    //const float a2 = 1.0f;

    float4 Q = pixel(ch0, p);
    float4 Qv = pixel(ch1, p);
    
    if(QvOn) Q = Qv * Level_Qv;
    
    float2 g = 0.1f*normalize(Grad(ch0, p, R));
    float3 v = to_float3_aw(g,  _sqrtf(1.0f-sq(g)) );
    float3 col = swi3(decube_f3(ch2,v),x,y,z);
    //fluid 1 amplitude
    float fd1 = smoothstep(0.0f, 0.7f, 1.0f*dot(swi2(Q,x,y),swi2(Q,x,y))/sq(a1)) * ( 1.0f + 0.001f*p.y*Level_fd1);
    //fluid 2 amplitude
    float fd2 = _logf(Q.z*Q.z+1.0f);

    // Output to screen
    //swi3S(fragColor,x,y,z, col*(sin_f3(to_float3(1.0f, 1.0f, 0.0f)*fd1) + sin_f3(to_float3(0.0f, 0.3f, 1.0f)*fd2)));
    swi3S(fragColor,x,y,z, col*(sin_f3(swi3(Color1,x,y,z)*fd1) + sin_f3(swi3(Color2,x,y,z)*fd2)));

    fragColor.w = Color1.w;
    
  SetFragmentShaderComputedColor(fragColor);
}