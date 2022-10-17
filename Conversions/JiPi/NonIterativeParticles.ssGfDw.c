
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


#define R iResolution
#define m to_float2((swi2(iMouse,x,y) - 0.5f*R) / R.y)
//#define KEY(v,m) texelFetch(iChannel3, to_int2(v, m), 0).x
#define ss(a, b, t) smoothstep(a, b, t)

#define A(p) texture(iChannel0,(make_float2(to_int2_cfloat(p))+0.5f)/R)
#define B(p) texture(iChannel1,(make_float2(to_int2_cfloat(p))+0.5f)/R)
#define C(p) texture(iChannel2,(make_float2(to_int2_cfloat(p))+0.5f)/R)
#define D(p) texture(iChannel3,(make_float2(to_int2_cfloat(p))+0.5f)/R)


// Add the force coming from the side
#define SIDE_FORCE



// Constants
#define UP    to_float2(0.0f, 1.0f)
#define DOWN  to_float2(0.0f, -1.0f)
#define LEFT  to_float2(-1.0f, 0.0f)
#define RIGHT to_float2(1.0f, 0.0f)
#define UPL   to_float2(-1.0f, 1.0f)
#define DOWNL to_float2(-1.0f, -1.0f)
#define UPR   to_float2(1.0f, 1.0f)
#define DOWNR to_float2(1.0f, -1.0f)



// https://www.shadertoy.com/view/4djSRW
__DEVICE__ float2 hash22(float2 p){
  float3 p3 = fract_f3((swi3(p,x,y,x)) * to_float3(0.1031f, 0.1030f, 0.0973f));
  p3 += dot(p3, swi3(p3,y,z,x)+33.33f);
  return fract_f2((swi2(p3,x,x)+swi2(p3,y,z))*swi2(p3,z,y));
}

__DEVICE__ float hash12(float2 p){
  float3 p3  = fract_f3((swi3(p,x,y,x)) * 0.1031f);
  p3 += dot(p3, swi3(p3,y,z,x) + 33.33f);
  return fract((p3.x + p3.y) * p3.z);
}


// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Texture: Blending' to iChannel1
// Connect Buffer A 'Previsualization: Buffer B' to iChannel2


/*
    Buffer A: 
    
    Track cell's mass and velocity.
    
    4 main things hapening.
    
    1.0f Add mass to cell proportional  to: (change in mass) * (dot(mass gradient direction, cell  velocity))
    2.0f Subtract mass from cell proportional to: (average mass of neighborhood) * (dot(mass gradient direction, cell  velocity))
    3.0f Apply acceleration to cell velocity: CellVelocity += (VelocityAverage - CellVelocity) 
    4.0f Apply a gravity ish force to cell velocity. This "de snakes" the particles and clumps them into points.
    
    x = mass
    zw = velocity
    
*/


 
// Neighborhood averages
__DEVICE__ float4 Grad(float2 u, float2 R, __TEXTURE2D__ iChannel0)
{
  
    float h = 1.0f;
    
    float4 up = A((u + to_float2(0.0f, h)));
    float4 down = A((u - to_float2(0.0f, h)));
    float4 left = A((u - to_float2(h, 0.0f)));
    float4 right = A((u + to_float2(h, 0.0f)));
    

    float4 upl = A((u + UPL));
    float4 downl = A((u + DOWNL));
    float4 upr = A((u + UPR));
    float4 downr = A((u + DOWNR));
    
    float4 avg = (up+down+left+right+upr+downr+upl+downl) / 8.0f;
    return avg;
}

// Direction of most mass relative to cell
__DEVICE__ float2 massGrad(float2 u, float2 R, __TEXTURE2D__ iChannel0)
{
    float up = A((u + UP)).x;
    float down = A((u + DOWN)).x;
    float left = A((u + LEFT)).x;
    float right = A((u + RIGHT)).x;

    float upl = A((u + UPL)).x;
    float downl = A((u + DOWNL)).x;
    float upr = A((u + UPR)).x;
    float downr = A((u + DOWNR)).x;
    
    float2 cm = to_float2(0.0f, 0.0f);
    cm += up*UP;
    cm += down*DOWN;
    cm += left*LEFT;
    cm += right*RIGHT;
    
    cm += upl*UPL;
    cm += downl*DOWNL;
    cm += upr*UPR;
    cm += downr*DOWNR;
    
    return cm;
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


__KERNEL__ void NonIterativeParticlesFuse__Buffer_A(float4 f, float2 u, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    CONNECT_CHECKBOX0(Reset, 0);
    
    //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);
    
    
    u+=0.5f;

    float2 uv = (swi2(u,x,y) - 0.5f*R)/R.y;

    // bA.x = mass, swi2(bA,z,w) = velocity
    float4 bA = A(u);
    
    float4 grad = Grad(u,R,iChannel0); // Gradient at point of cell
    float2 vAvg = swi2(grad,z,w); // Average velocity of surrounding cells
    float2 acc = vAvg - swi2(bA,z,w); // Cell acceleration
    acc = clamp(acc, -1.0f, 1.0f);

    bA = _mix(bA, grad, 0.02f); // Substitute in a little bit of the average

    float2 mg = massGrad(u, R, iChannel0); // Direction relative to current cell of most mass

    float massAvg = grad.x; // Average surrounding mass
    float massDif = massAvg - bA.x; // How fast the mass should be changing
    
    // Add and subtract mass from cell based on how much its moving in the direction of most mass
    // and how fast the mass is changing
    float dp = dot(mg, swi2(bA,z,w)); // How much the cell is moving toward the center of mass gradient
    bA.x -= dp*massAvg;
    bA.x += 0.999f*dp*massDif;
    
    // Apply acceleration
    //swi2(bA,z,w) += acc;
    bA.z += acc.x;
    bA.w += acc.y;
    
    // Apply a gravity ish force (clumps stuff into single points)
    float r = _fmaxf(length(mg), 1.0f);
    float grav = -(3.5f*massAvg*bA.x) / (r*r);
    //swi2(bA,z,w) -= (mg)*grav;
    bA.z -= (mg.x)*grav;
    bA.w -= (mg.y)*grav;
    
    // Apply some friction
    //swi2(bA,z,w) -= swi2(bA,z,w)*0.0003f;
    bA.z -= bA.z*0.0003f;
    bA.w -= bA.w*0.0003f;
    
    
    // Add stuff with mouse
    if(iMouse.z > 0.0f){
       float2 v = (swi2(C(u),x,y) - swi2(iMouse,x,y));
       if(length(v) < 100.0f){
        
            float d = length(swi2(iMouse,x,y)-u);
            float frc = 0.025f*_exp2f(-d*0.025f);       
        
            //swi2(bA,z,w) -= frc * v;    
            bA.z -= frc * v.x;
            bA.w -= frc * v.y;
            
            bA.x+=0.08f*frc;
        }
    }
   
    
    #ifdef SIDE_FORCE
    // Apple a small force coming from the left and add some mass
    float sf = 0.002f*ss(R.x, 0.0f, u.x * 0.9f);
    //swi2(bA,z,w) += sf*to_float2(1.0f, 0.0f);
    bA.z += sf*(1.0f);
    bA.w += sf*(0.0f);
    
    
    bA.x += 0.06f*sf;
    #endif
   
    // Inita
    if(iFrame < 8 || Reset){
        bA = to_float4_s(0);
        
        if(hash12(u*343.0f + 232.0f) < 0.1f){
            swi2S(bA,z,w, 12.0f*(2.0f*hash22(u*543.0f + 332.0f) - 1.0f));
            bA.x = hash22(u*543.0f + 332.0f).x;
        }
    }
    
    
    swi2S(bA,z,w, clamp(swi2(bA,z,w), -120.0f, 120.0f));
    bA.x = clamp(bA.x, 0.0f, 1.0f);
   
    
    if (Blend1>0.0) bA = Blending(iChannel1, u/R, bA, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, u, R);
    
    f = bA;
 
  SetFragmentShaderComputedColor(f);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------


// Store mouse last pos nothin to see here
__KERNEL__ void NonIterativeParticlesFuse__Buffer_B(float4 f, float2 u, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{

    f = to_float4(iMouse.x,iMouse.y, 0.0f, 0.0f);

  SetFragmentShaderComputedColor(f);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Preset: Keyboard' to iChannel3
// Connect Image 'Previsualization: Buffer A' to iChannel0


/*
    By Cole Peterson (Plento)
    
    
    A WIP experiment in making a ton of individual particles that behave like you'd expect.
    Still some things to work out but so far Im happy.
    
    Cells store mass and velocity.
    
    Use the MOUSE to drag stuff around.
    
    Press A to zoom while using the mouse.
    
*/


__DEVICE__ void zoom(inout float2 *u, float4 iMouse){
    *u -= swi2(iMouse,x,y);
    *u *= 0.6f;
    *u += swi2(iMouse,x,y);
}

__DEVICE__ float3 pal(float t){
    return 0.5f+0.44f*cos_f3(to_float3(1.4f, 1.1f, 1.4f)*t + to_float3(1.1f, 6.1f, 4.4f) + 0.5f);
}

// Main color
__DEVICE__ float3 color(float2 u, float4 bA){
    float t = _fabs(bA.z*2.0f) + _fabs(bA.w*4.0f) + 3.6f*length(swi2(bA,z,w));
    float3 col = to_float3_s(clamp(bA.x, 0.0f, 1.0f)) * pal(t*0.2f + 0.3f);
    return col * 1.6f;
}


// glow effect
__DEVICE__ float glow(float2 u, float2 R, __TEXTURE2D__ iChannel0){
    float2 uv = u / R;
    float blur = 0.0f;

    const float N = 3.0f;
  
    for(float i = 0.0f; i < N; i++)
    {
        blur += texture(iChannel0, uv + to_float2(i*0.001f, 0.0f)).x;
        blur += texture(iChannel0, uv - to_float2(i*0.001f, 0.0f)).x;
    
        blur += texture(iChannel0, uv + to_float2(0.0f, i*0.001f)).x;
        blur += texture(iChannel0, uv - to_float2(0.0f, i*0.001f)).x;
    }
        
    return blur / N*4.0f;
}


__KERNEL__ void NonIterativeParticlesFuse(float4 f, float2 u, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{

    CONNECT_CHECKBOX0(Zoom, 0);

    float2 uv = u / R;
        
    if(Zoom > 0.0f) zoom(&u, iMouse);

    float4 bA = A(u);

    float3 col = color(u, bA);

    float g = glow(u, R, iChannel0);
    
    col += 0.08f*g*pal(1.5f*length(swi2(bA,z,w)));
    
    f = to_float4_aw(sqrt_f3(clamp(col, 0.0f, 1.0f)), 1.0f);

  SetFragmentShaderComputedColor(f);
}