
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// Tnx Inigo Quilez for palette function, see https://www.shadertoy.com/view/ll2GD3
__DEVICE__ float3 Palette( in float t, in float3 a, in float3 b, in float3 c, in float3 d )
{
    return a + b*cos_f3( 6.28318f*(c*t+d) );
}

__DEVICE__ float dot2( in float2 v ) { return dot(v,v); }

__DEVICE__ float sdHeart(in float2 p)
{
    p.x = _fabs(p.x);
    if( p.y+p.x>1.0f )
        return _sqrtf(dot2(p-to_float2(0.25f,0.75f))) - _sqrtf(2.0f)/4.0f;
    return _sqrtf(_fminf(dot2(p-to_float2(0.00f,1.00f)),
                    dot2(p-0.5f*_fmaxf(p.x+p.y,0.0f)))) * sign_f(p.x-p.y);
}

__DEVICE__ float4 Blending( __TEXTURE2D__ channel, float2 uv, float4 Q, float Blend, float2 Par, float2 MulOff, int Modus, float2 U, float2 R)
{
    // Modus *= 2; // Fuse   
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



/*

# Phyton script
#
# The Poisson equation can be solved with iterative Jacobi,  as explained in https://developer.nvidia.com/gpugems/gpugems/part-vi-beyond-triangles/chapter-38-fast-fluid-dynamics-simulation-gpu
# But on Shadertoy it's not possible use Jacobi that way, due it's not possible read and write to a texture more times for frame.
# Anyway it's possible precompute the coefficient of a two filters that summed "pre-compute" the twenty steps.

import numpy as np
import seaborn as sns; sns.set()
from scipy import signal
import sys
from sympy import *
from math import *

dx = 1
dt = 1/60

# This parameters are to precompute the filter for diffusion, not used in the shader
#diffuse velocity
#viscosity = 0.1
#alpha = (dx)*(dx) / (viscosity * dt)
#rBeta = 1/(alpha+4)

#Coefficients for Poisson equation. See https://developer.nvidia.com/gpugems/gpugems/part-vi-beyond-triangles/chapter-38-fast-fluid-dynamics-simulation-gpu
alpha = -(dx)*(dx);
rBeta = 1/4

       
def PrintMatrix2D(M):
    sizeX = shape(M)[0]
    sizeY = shape(M)[1]
    
    for i in range(0,sizeX):
        for j in range(0,sizeY):
          #print('(' + str(simplify(M[i,j])) + ') ')
          print('(' + str(simplify(M[i,j])) + ') ', end='')
        print()

def PrintMatrixShader(M):
    sizeX = shape(M)[0]
    sizeY = shape(M)[1]
    
    ci = _floor((sizeX)/2)
    cj = _floor((sizeX)/2)
    
    for i in range(0,sizeX):
        for j in range(0,sizeY):
            v = simplify(M[i,j])
            if(v != 0):
                print('v += ' + str(v) + '*U(x+to_float2(' + str(i-ci) + ',' + str(j-cj) + '));')

def Jacobi20Step(X, P, n_iter):

    XNEW = X.copy()
    TMP = zeros(shape(X)[0],shape(X)[1])
    for n in range(0,n_iter):
        for i in range(0,shape(X)[0]):
            for j in range(0,shape(X)[1]):
                xT = XNEW[i-1,j] if (i-1>=0 and i-1<shape(X)[0]) else 0;
                xB = XNEW[i+1,j] if (i+1>=0 and i+1<shape(X)[0]) else 0;
                xL = XNEW[i,j-1] if (j-1>=0 and j-1<shape(X)[1]) else 0;
                xR = XNEW[i,j+1] if (j+1>=0 and j+1<shape(X)[1]) else 0;
                p =  XNEW[i,j]*alpha
                TMP[i,j] = (xL + xR + xT + xB + p)*rBeta
        XNEW = TMP.copy();
    return XNEW

n = 20
k = 43

ci = _floor((k)/2)
cj = _floor((k)/2)

# Generate the first matrix, the ones that apply to the divergence buffer
U = zeros(k,k)
for i in range(0, k):
    for j in range(0, k):
        U[i,j] = 0
        
P = zeros(k,k)
for i in range(0, k):
    for j in range(0, k):
        P[i,j] = 0

U[ci,cj] = 1
P[ci,cj] = 0

XU = Jacobi20Step(U, P, n)
PrintMatrixShader(XU)
print()

# Generate the second matrix, the ones that apply to the pressure buffer
U = zeros(k,k)
for i in range(0, k):
    for j in range(0, k):
        U[i,j] = 0
        
P = zeros(k,k)
for i in range(0, k):
    for j in range(0, k):
        P[i,j] = 0

U[ci,cj] = 0
P[ci,cj] = 1

XU = Jacobi20Step(U, P, n)
PrintMatrixShader(XU)
print()
*/
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer D' to iChannel0
// Connect Buffer A 'Texture: Blending' to iChannel1


// BUFFER A: output velocity field after advection

// Backward advection
__DEVICE__ float4 Advect(in float2 x, in float dt, float2 iResolution, __TEXTURE2D__ iChannel0) 
{
    float2 pos = x - dt*swi2(texture(iChannel0, x/iResolution),x,y);
    return texture(iChannel0, pos/iResolution);
}

__KERNEL__ void FluidatheartJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float iTimeDelta, int iFrame, float4 iMouse, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_CHECKBOX1(SecondSource, 0);
    CONNECT_POINT0(SecondSourceXY, 0.0f, 0.0f );
    CONNECT_POINT1(SecondSourceVel, 0.0f, 0.0f );
    
    CONNECT_POINT3(SourceVel, 0.0f, 0.0f );
    
        //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);
    
    CONNECT_POINT4(StartVel, 0.0f, 0.0f);
    CONNECT_POINT5(BoundarieVel, 0.0f, 0.0f);
    
    CONNECT_SLIDER5(AdvectVel, -10.0f, 10.0f, 1.0f);
    CONNECT_SLIDER6(HeartSize, 0.0f, 10.0f, 1.8f);
    
     
    fragCoord+=0.5f;

    float2 iM = swi2(iMouse,x,y) / iResolution - 0.5f;


    float dt = iTimeDelta;
   
    float2 x = fragCoord;    // Current cell coordinates from 0.5f to iResolution-0.5
    float4 u = to_float4_s(0.0f);                // Holds computed velocity value for the current cell (pixel)
    
    // Set boundaries velocities
    if( x.x < 2.5f || x.x > iResolution.x-2.5f || x.y > iResolution.y-2.5f || x.y < 2.5f) u.x=-50.0f,u.y=50.0f;//swi2(u,x,y) = to_float2(-50,50);
    
    if(iFrame < 2 || Reset) 
    {
       // Init velocity field
       //swi2(u,x,y) = to_float2(-50,50.0f);
       u.x=-50.0f+StartVel.x,u.y=50.0f+StartVel.y;
    }
    else
    {
        // Set velocity at  boundaries
        if( x.x<2.0f || x.x>iResolution.x-2.0f || x.y>iResolution.y-2.0f || x.y<2.0f) u.x=-100.0f,u.y=100.0f; //to_float2(-100,100); ??????????????
        else u = Advect(x,1.0f/60.0f*AdvectVel, iResolution, iChannel0); 
    }
    
    // Heart shape where there is no fluid
    float2 p = (x*2.0f-iResolution)/iResolution.y;
    float d = sdHeart(p*HeartSize+to_float2(0.0f,0.5f));
    if(d<0.0f) u.x=0.0f,u.y=0.0f;//swi2(u,x,y) = to_float2(0);
    
    float s = iResolution.x / 100.0f;
    //float d1 = length(x-to_float2(iResolution.x*3.7f/5.0f,iResolution.y*0.5f/5.0f));
    float d1 = length(x-to_float2(iResolution.x*(3.7f+iM.x)/5.0f,iResolution.y*(0.5f+iM.y)/5.0f));
    if(d1 < s) 
    {
        u.x=100.0f+SourceVel.x,u.y=100.0f+SourceVel.y;//swi2(u,x,y) = to_float2(100,100);
    }
    
    if(SecondSource)
    {
      float d1 = length(x-to_float2(iResolution.x*(3.7f+SecondSourceXY.x)/5.0f,iResolution.y*(0.5f+SecondSourceXY.y)/5.0f));
      if(d1 < s) 
      {
          u.x=100.0f+SecondSourceVel.x, u.y=100.0f+SecondSourceVel.y;//swi2(u,x,y) = to_float2(100,100);
      }
    }
    
    //if (Blend1>0.0) Q = Blending(iChannel1, U/R, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, U, R);
   
    fragColor = u;

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1
// Connect Buffer B 'Texture: Blending' to iChannel2


// BUFFER B: output the divergence of the velocity field in x and generate and advect the RGB values colors in yxw
#ifdef XXX
__DEVICE__ float3 AdvectColors(in float2 x, in float dt, float2 iResolution, float iTime, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1, bool SecondSource, float2 SecondSourceXY, float2 iM) 
{
    float3 c = to_float3_s(0); 

    float s = iResolution.x / 100.0f;
    //float d = length(x-to_float2(iResolution.x*3.7f/5.0f,iResolution.y*0.5f/5.0f));
    float d = length(x-to_float2(iResolution.x*(3.7f+iM.x)/5.0f,iResolution.y*(0.5f+iM.y)/5.0f));
    if(d < s) 
    {
        c = Palette(x.x*10.0f+50.0f*_sinf(iTime*0.001f), to_float3(0.5f,0.5f,0.5f),to_float3(0.5f,0.5f,0.5f),to_float3(2.0f,1.0f,0.0f),to_float3(0.5f,0.20f,0.25f));
    }
    
    else
    {
        float2 pos = x - dt*swi2(texture(iChannel0, x/iResolution),x,y);    
        c = swi3(texture(iChannel1, pos/iResolution),y,z,w);
    }

    return c;
}
#endif
__DEVICE__ float Divergence(float2 x, float2 iResolution, __TEXTURE2D__ iChannel0)
{
    float halfrdx = 0.5f; // Half cell size
    
    float4 xL = texture(iChannel0, (x - to_float2(1, 0))/iResolution);
    float4 xR = texture(iChannel0, (x + to_float2(1, 0))/iResolution);
    float4 xT = texture(iChannel0, (x + to_float2(0, 1))/iResolution);
    float4 xB = texture(iChannel0, (x - to_float2(0, 1))/iResolution);
    
    return halfrdx * (-xR.x + xL.x - xT.y + xB.y);
}

__KERNEL__ void FluidatheartJipiFuse__Buffer_B(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX1(SecondSource, 0);
    CONNECT_POINT0(SecondSourceXY, 0.0f, 0.0f );
    CONNECT_POINT1(SecondSourceVel, 0.0f, 0.0f );
    CONNECT_POINT3(SourceVel, 0.0f, 0.0f );
    
        //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);
      
    CONNECT_SLIDER0(dt, -1.0f, 1.0f, 0.0166f);   // 1.0f/60.0f
      
    fragCoord+=0.5f;
    
    float2 iM = swi2(iMouse,x,y) / iResolution - 0.5f;
  
    //float3 c =  AdvectColors(fragCoord, 1.0f/60.0f, iResolution, iTime, iChannel0, iChannel1, SecondSource, SecondSourceXY, iM);
    
    float3 c = to_float3_s(0); 

    float s = iResolution.x / 100.0f;
    //float d = length(x-to_float2(iResolution.x*3.7f/5.0f,iResolution.y*0.5f/5.0f));
    float d = length(fragCoord-to_float2(iResolution.x*(3.7f+iM.x)/5.0f,iResolution.y*(0.5f+iM.y)/5.0f));
    if(d < s) 
    {
        c = Palette(fragCoord.x*10.0f+50.0f*_sinf(iTime*0.001f), to_float3(0.5f,0.5f,0.5f),to_float3(0.5f,0.5f,0.5f),to_float3(2.0f,1.0f,0.0f),to_float3(0.5f,0.20f,0.25f));
    }
    else
    {
       float d = length(fragCoord-to_float2(iResolution.x*(3.7f+SecondSourceXY.x)/5.0f,iResolution.y*(0.5f+SecondSourceXY.y)/5.0f));
       if(d < s && SecondSource) 
       {
           c = Palette(fragCoord.x*10.0f+50.0f*_sinf(iTime*0.001f), to_float3(0.5f,0.5f,0.5f),to_float3(0.5f,0.5f,0.5f),to_float3(2.0f,1.0f,0.0f),to_float3(0.5f,0.20f,0.25f));
       }  
       else
       {
           float2 pos = fragCoord - dt*swi2(texture(iChannel0, fragCoord/iResolution),x,y);    
           c = swi3(texture(iChannel1, pos/iResolution),y,z,w);
       }
    }   

    fragColor = to_float4(Divergence(fragCoord,iResolution,iChannel0), c.x,c.y,c.z);


    if (Blend1>0.0) fragColor = Blending(iChannel2, fragCoord/iResolution, fragColor, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, fragCoord, iResolution);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1
// Connect Buffer C 'Previsualization: Buffer C' to iChannel0


// BUFFER C: solve the Poisson equation and output the pressure field 

__DEVICE__ float JacobiSolveForPressureXPart(float2 x, float2 iResolution, __TEXTURE2D__ iChannel0)
{
    float4 v;

    v += 9.09494701772928e-13*texture(iChannel0,(x+to_float2(-20,0))/iResolution);
    v += 1.81898940354586e-11*texture(iChannel0,(x+to_float2(-19,-1))/iResolution);
    v += 1.81898940354586e-11*texture(iChannel0,(x+to_float2(-19,1))/iResolution);
    v += 1.72803993336856e-10*texture(iChannel0,(x+to_float2(-18,-2))/iResolution);
    v += 3.63797880709171e-10*texture(iChannel0,(x+to_float2(-18,0))/iResolution);
    v += 1.72803993336856e-10*texture(iChannel0,(x+to_float2(-18,2))/iResolution);
    v += 1.03682396002114e-9*texture(iChannel0,(x+to_float2(-17,-3))/iResolution);
    v += 3.45607986673713e-9*texture(iChannel0,(x+to_float2(-17,-1))/iResolution);
    v += 3.45607986673713e-9*texture(iChannel0,(x+to_float2(-17,1))/iResolution);
    v += 1.03682396002114e-9*texture(iChannel0,(x+to_float2(-17,3))/iResolution);
    v += 4.40650183008984e-9*texture(iChannel0,(x+to_float2(-16,-4))/iResolution);
    v += 2.07364792004228e-8*texture(iChannel0,(x+to_float2(-16,-2))/iResolution);
    v += 3.28327587340027e-8*texture(iChannel0,(x+to_float2(-16,0))/iResolution);
    v += 2.07364792004228e-8*texture(iChannel0,(x+to_float2(-16,2))/iResolution);
    v += 4.40650183008984e-9*texture(iChannel0,(x+to_float2(-16,4))/iResolution);
    v += 1.41008058562875e-8*texture(iChannel0,(x+to_float2(-15,-5))/iResolution);
    v += 8.81300366017967e-8*texture(iChannel0,(x+to_float2(-15,-3))/iResolution);
    v += 1.96996552404016e-7*texture(iChannel0,(x+to_float2(-15,-1))/iResolution);
    v += 1.96996552404016e-7*texture(iChannel0,(x+to_float2(-15,1))/iResolution);
    v += 8.81300366017967e-8*texture(iChannel0,(x+to_float2(-15,3))/iResolution);
    v += 1.41008058562875e-8*texture(iChannel0,(x+to_float2(-15,5))/iResolution);
    v += 3.52520146407187e-8*texture(iChannel0,(x+to_float2(-14,-6))/iResolution);
    v += 2.82016117125750e-7*texture(iChannel0,(x+to_float2(-14,-4))/iResolution);
    v += 8.37235347717069e-7*texture(iChannel0,(x+to_float2(-14,-2))/iResolution);
    v += 1.18197931442410e-6*texture(iChannel0,(x+to_float2(-14,0))/iResolution);
    v += 8.37235347717069e-7*texture(iChannel0,(x+to_float2(-14,2))/iResolution);
    v += 2.82016117125750e-7*texture(iChannel0,(x+to_float2(-14,4))/iResolution);
    v += 3.52520146407187e-8*texture(iChannel0,(x+to_float2(-14,6))/iResolution);
    v += 7.05040292814374e-8*texture(iChannel0,(x+to_float2(-13,-7))/iResolution);
    v += 7.05040292814374e-7*texture(iChannel0,(x+to_float2(-13,-5))/iResolution);
    v += 2.67915311269462e-6*texture(iChannel0,(x+to_float2(-13,-3))/iResolution);
    v += 5.02341208630241e-6*texture(iChannel0,(x+to_float2(-13,-1))/iResolution);
    v += 5.02341208630241e-6*texture(iChannel0,(x+to_float2(-13,1))/iResolution);
    v += 2.67915311269462e-6*texture(iChannel0,(x+to_float2(-13,3))/iResolution);
    v += 7.05040292814374e-7*texture(iChannel0,(x+to_float2(-13,5))/iResolution);
    v += 7.05040292814374e-8*texture(iChannel0,(x+to_float2(-13,7))/iResolution);
    v += 1.14569047582336e-7*texture(iChannel0,(x+to_float2(-12,-8))/iResolution);
    v += 1.41008058562875e-6*texture(iChannel0,(x+to_float2(-12,-6))/iResolution);
    v += 6.69788278173655e-6*texture(iChannel0,(x+to_float2(-12,-4))/iResolution);
    v += 1.60749186761677e-5*texture(iChannel0,(x+to_float2(-12,-2))/iResolution);
    v += 2.13495013667853e-5*texture(iChannel0,(x+to_float2(-12,0))/iResolution);
    v += 1.60749186761677e-5*texture(iChannel0,(x+to_float2(-12,2))/iResolution);
    v += 6.69788278173655e-6*texture(iChannel0,(x+to_float2(-12,4))/iResolution);
    v += 1.41008058562875e-6*texture(iChannel0,(x+to_float2(-12,6))/iResolution);
    v += 1.14569047582336e-7*texture(iChannel0,(x+to_float2(-12,8))/iResolution);
    v += 1.52758730109781e-7*texture(iChannel0,(x+to_float2(-11,-9))/iResolution);
    v += 2.29138095164672e-6*texture(iChannel0,(x+to_float2(-11,-7))/iResolution);
    v += 1.33957655634731e-5*texture(iChannel0,(x+to_float2(-11,-5))/iResolution);
    v += 4.01872966904193e-5*texture(iChannel0,(x+to_float2(-11,-3))/iResolution);
    v += 6.83184043737128e-5*texture(iChannel0,(x+to_float2(-11,-1))/iResolution);
    v += 6.83184043737128e-5*texture(iChannel0,(x+to_float2(-11,1))/iResolution);
    v += 4.01872966904193e-5*texture(iChannel0,(x+to_float2(-11,3))/iResolution);
    v += 1.33957655634731e-5*texture(iChannel0,(x+to_float2(-11,5))/iResolution);
    v += 2.29138095164672e-6*texture(iChannel0,(x+to_float2(-11,7))/iResolution);
    v += 1.52758730109781e-7*texture(iChannel0,(x+to_float2(-11,9))/iResolution);
    v += 1.68034603120759e-7*texture(iChannel0,(x+to_float2(-10,-10))/iResolution);
    v += 3.05517460219562e-6*texture(iChannel0,(x+to_float2(-10,-8))/iResolution);
    v += 2.17681190406438e-5*texture(iChannel0,(x+to_float2(-10,-6))/iResolution);
    v += 8.03745933808386e-5*texture(iChannel0,(x+to_float2(-10,-4))/iResolution);
    v += 0.000170796010934282f*texture(iChannel0,(x+to_float2(-10,-2))/iResolution);
    v += 0.000218618893995881f*texture(iChannel0,(x+to_float2(-10,0))/iResolution);
    v += 0.000170796010934282f*texture(iChannel0,(x+to_float2(-10,2))/iResolution);
    v += 8.03745933808386e-5*texture(iChannel0,(x+to_float2(-10,4))/iResolution);
    v += 2.17681190406438e-5*texture(iChannel0,(x+to_float2(-10,6))/iResolution);
    v += 3.05517460219562e-6*texture(iChannel0,(x+to_float2(-10,8))/iResolution);
    v += 1.68034603120759e-7*texture(iChannel0,(x+to_float2(-10,10))/iResolution);
    v += 1.52758730109781e-7*texture(iChannel0,(x+to_float2(-9,-11))/iResolution);
    v += 3.36069206241518e-6*texture(iChannel0,(x+to_float2(-9,-9))/iResolution);
    v += 2.90241587208584e-5*texture(iChannel0,(x+to_float2(-9,-7))/iResolution);
    v += 0.000130608714243863f*texture(iChannel0,(x+to_float2(-9,-5))/iResolution);
    v += 0.000341592021868564f*texture(iChannel0,(x+to_float2(-9,-3))/iResolution);
    v += 0.000546547234989703f*texture(iChannel0,(x+to_float2(-9,-1))/iResolution);
    v += 0.000546547234989703f*texture(iChannel0,(x+to_float2(-9,1))/iResolution);
    v += 0.000341592021868564f*texture(iChannel0,(x+to_float2(-9,3))/iResolution);
    v += 0.000130608714243863f*texture(iChannel0,(x+to_float2(-9,5))/iResolution);
    v += 2.90241587208584e-5*texture(iChannel0,(x+to_float2(-9,7))/iResolution);
    v += 3.36069206241518e-6*texture(iChannel0,(x+to_float2(-9,9))/iResolution);
    v += 1.52758730109781e-7*texture(iChannel0,(x+to_float2(-9,11))/iResolution);
    v += 1.14569047582336e-7*texture(iChannel0,(x+to_float2(-8,-12))/iResolution);
    v += 3.05517460219562e-6*texture(iChannel0,(x+to_float2(-8,-10))/iResolution);
    v += 3.19265745929442e-5*texture(iChannel0,(x+to_float2(-8,-8))/iResolution);
    v += 0.000174144952325150f*texture(iChannel0,(x+to_float2(-8,-6))/iResolution);
    v += 0.000555087035536417f*texture(iChannel0,(x+to_float2(-8,-4))/iResolution);
    v += 0.00109309446997941f*texture(iChannel0,(x+to_float2(-8,-2))/iResolution);
    v += 0.00136636808747426f*texture(iChannel0,(x+to_float2(-8,0))/iResolution);
    v += 0.00109309446997941f*texture(iChannel0,(x+to_float2(-8,2))/iResolution);
    v += 0.000555087035536417f*texture(iChannel0,(x+to_float2(-8,4))/iResolution);
    v += 0.000174144952325150f*texture(iChannel0,(x+to_float2(-8,6))/iResolution);
    v += 3.19265745929442e-5*texture(iChannel0,(x+to_float2(-8,8))/iResolution);
    v += 3.05517460219562e-6*texture(iChannel0,(x+to_float2(-8,10))/iResolution);
    v += 1.14569047582336e-7*texture(iChannel0,(x+to_float2(-8,12))/iResolution);
    v += 7.05040292814374e-8*texture(iChannel0,(x+to_float2(-7,-13))/iResolution);
    v += 2.29138095164672e-6*texture(iChannel0,(x+to_float2(-7,-11))/iResolution);
    v += 2.90241587208584e-5*texture(iChannel0,(x+to_float2(-7,-9))/iResolution);
    v += 0.000191559447557665f*texture(iChannel0,(x+to_float2(-7,-7))/iResolution);
    v += 0.000740116047381889f*texture(iChannel0,(x+to_float2(-7,-5))/iResolution);
    v += 0.00177627851371653f*texture(iChannel0,(x+to_float2(-7,-3))/iResolution);
    v += 0.00273273617494851f*texture(iChannel0,(x+to_float2(-7,-1))/iResolution);
    v += 0.00273273617494851f*texture(iChannel0,(x+to_float2(-7,1))/iResolution);
    v += 0.00177627851371653f*texture(iChannel0,(x+to_float2(-7,3))/iResolution);
    v += 0.000740116047381889f*texture(iChannel0,(x+to_float2(-7,5))/iResolution);
    v += 0.000191559447557665f*texture(iChannel0,(x+to_float2(-7,7))/iResolution);
    v += 2.90241587208584e-5*texture(iChannel0,(x+to_float2(-7,9))/iResolution);
    v += 2.29138095164672e-6*texture(iChannel0,(x+to_float2(-7,11))/iResolution);
    v += 7.05040292814374e-8*texture(iChannel0,(x+to_float2(-7,13))/iResolution);
    v += 3.52520146407187e-8*texture(iChannel0,(x+to_float2(-6,-14))/iResolution);
    v += 1.41008058562875e-6*texture(iChannel0,(x+to_float2(-6,-12))/iResolution);
    v += 2.17681190406438e-5*texture(iChannel0,(x+to_float2(-6,-10))/iResolution);
    v += 0.000174144952325150f*texture(iChannel0,(x+to_float2(-6,-8))/iResolution);
    v += 0.000814127652120078f*texture(iChannel0,(x+to_float2(-6,-6))/iResolution);
    v += 0.00236837135162205f*texture(iChannel0,(x+to_float2(-6,-4))/iResolution);
    v += 0.00444069628429133f*texture(iChannel0,(x+to_float2(-6,-2))/iResolution);
    v += 0.00546547234989703f*texture(iChannel0,(x+to_float2(-6,0))/iResolution);
    v += 0.00444069628429133f*texture(iChannel0,(x+to_float2(-6,2))/iResolution);
    v += 0.00236837135162205f*texture(iChannel0,(x+to_float2(-6,4))/iResolution);
    v += 0.000814127652120078f*texture(iChannel0,(x+to_float2(-6,6))/iResolution);
    v += 0.000174144952325150f*texture(iChannel0,(x+to_float2(-6,8))/iResolution);
    v += 2.17681190406438e-5*texture(iChannel0,(x+to_float2(-6,10))/iResolution);
    v += 1.41008058562875e-6*texture(iChannel0,(x+to_float2(-6,12))/iResolution);
    v += 3.52520146407187e-8*texture(iChannel0,(x+to_float2(-6,14))/iResolution);
    v += 1.41008058562875e-8*texture(iChannel0,(x+to_float2(-5,-15))/iResolution);
    v += 7.05040292814374e-7*texture(iChannel0,(x+to_float2(-5,-13))/iResolution);
    v += 1.33957655634731e-5*texture(iChannel0,(x+to_float2(-5,-11))/iResolution);
    v += 0.000130608714243863f*texture(iChannel0,(x+to_float2(-5,-9))/iResolution);
    v += 0.000740116047381889f*texture(iChannel0,(x+to_float2(-5,-7))/iResolution);
    v += 0.00260520848678425f*texture(iChannel0,(x+to_float2(-5,-5))/iResolution);
    v += 0.00592092837905511f*texture(iChannel0,(x+to_float2(-5,-3))/iResolution);
    v += 0.00888139256858267f*texture(iChannel0,(x+to_float2(-5,-1))/iResolution);
    v += 0.00888139256858267f*texture(iChannel0,(x+to_float2(-5,1))/iResolution);
    v += 0.00592092837905511f*texture(iChannel0,(x+to_float2(-5,3))/iResolution);
    v += 0.00260520848678425f*texture(iChannel0,(x+to_float2(-5,5))/iResolution);
    v += 0.000740116047381889f*texture(iChannel0,(x+to_float2(-5,7))/iResolution);
    v += 0.000130608714243863f*texture(iChannel0,(x+to_float2(-5,9))/iResolution);
    v += 1.33957655634731e-5*texture(iChannel0,(x+to_float2(-5,11))/iResolution);
    v += 7.05040292814374e-7*texture(iChannel0,(x+to_float2(-5,13))/iResolution);
    v += 1.41008058562875e-8*texture(iChannel0,(x+to_float2(-5,15))/iResolution);
    v += 4.40650183008984e-9*texture(iChannel0,(x+to_float2(-4,-16))/iResolution);
    v += 2.82016117125750e-7*texture(iChannel0,(x+to_float2(-4,-14))/iResolution);
    v += 6.69788278173655e-6*texture(iChannel0,(x+to_float2(-4,-12))/iResolution);
    v += 8.03745933808386e-5*texture(iChannel0,(x+to_float2(-4,-10))/iResolution);
    v += 0.000555087035536417f*texture(iChannel0,(x+to_float2(-4,-8))/iResolution);
    v += 0.00236837135162205f*texture(iChannel0,(x+to_float2(-4,-6))/iResolution);
    v += 0.00651302121696062f*texture(iChannel0,(x+to_float2(-4,-4))/iResolution);
    v += 0.0118418567581102f*texture(iChannel0,(x+to_float2(-4,-2))/iResolution);
    v += 0.0144322629239468f*texture(iChannel0,(x+to_float2(-4,0))/iResolution);
    v += 0.0118418567581102f*texture(iChannel0,(x+to_float2(-4,2))/iResolution);
    v += 0.00651302121696062f*texture(iChannel0,(x+to_float2(-4,4))/iResolution);
    v += 0.00236837135162205f*texture(iChannel0,(x+to_float2(-4,6))/iResolution);
    v += 0.000555087035536417f*texture(iChannel0,(x+to_float2(-4,8))/iResolution);
    v += 8.03745933808386e-5*texture(iChannel0,(x+to_float2(-4,10))/iResolution);
    v += 6.69788278173655e-6*texture(iChannel0,(x+to_float2(-4,12))/iResolution);
    v += 2.82016117125750e-7*texture(iChannel0,(x+to_float2(-4,14))/iResolution);
    v += 4.40650183008984e-9*texture(iChannel0,(x+to_float2(-4,16))/iResolution);
    v += 1.03682396002114e-9*texture(iChannel0,(x+to_float2(-3,-17))/iResolution);
    v += 8.81300366017967e-8*texture(iChannel0,(x+to_float2(-3,-15))/iResolution);
    v += 2.67915311269462e-6*texture(iChannel0,(x+to_float2(-3,-13))/iResolution);
    v += 4.01872966904193e-5*texture(iChannel0,(x+to_float2(-3,-11))/iResolution);
    v += 0.000341592021868564f*texture(iChannel0,(x+to_float2(-3,-9))/iResolution);
    v += 0.00177627851371653f*texture(iChannel0,(x+to_float2(-3,-7))/iResolution);
    v += 0.00592092837905511f*texture(iChannel0,(x+to_float2(-3,-5))/iResolution);
    v += 0.0130260424339212f*texture(iChannel0,(x+to_float2(-3,-3))/iResolution);
    v += 0.0192430172319291f*texture(iChannel0,(x+to_float2(-3,-1))/iResolution);
    v += 0.0192430172319291f*texture(iChannel0,(x+to_float2(-3,1))/iResolution);
    v += 0.0130260424339212f*texture(iChannel0,(x+to_float2(-3,3))/iResolution);
    v += 0.00592092837905511f*texture(iChannel0,(x+to_float2(-3,5))/iResolution);
    v += 0.00177627851371653f*texture(iChannel0,(x+to_float2(-3,7))/iResolution);
    v += 0.000341592021868564f*texture(iChannel0,(x+to_float2(-3,9))/iResolution);
    v += 4.01872966904193e-5*texture(iChannel0,(x+to_float2(-3,11))/iResolution);
    v += 2.67915311269462e-6*texture(iChannel0,(x+to_float2(-3,13))/iResolution);
    v += 8.81300366017967e-8*texture(iChannel0,(x+to_float2(-3,15))/iResolution);
    v += 1.03682396002114e-9*texture(iChannel0,(x+to_float2(-3,17))/iResolution);
    v += 1.72803993336856e-10*texture(iChannel0,(x+to_float2(-2,-18))/iResolution);
    v += 2.07364792004228e-8*texture(iChannel0,(x+to_float2(-2,-16))/iResolution);
    v += 8.37235347717069e-7*texture(iChannel0,(x+to_float2(-2,-14))/iResolution);
    v += 1.60749186761677e-5*texture(iChannel0,(x+to_float2(-2,-12))/iResolution);
    v += 0.000170796010934282f*texture(iChannel0,(x+to_float2(-2,-10))/iResolution);
    v += 0.00109309446997941f*texture(iChannel0,(x+to_float2(-2,-8))/iResolution);
    v += 0.00444069628429133f*texture(iChannel0,(x+to_float2(-2,-6))/iResolution);
    v += 0.0118418567581102f*texture(iChannel0,(x+to_float2(-2,-4))/iResolution);
    v += 0.0211673189551220f*texture(iChannel0,(x+to_float2(-2,-2))/iResolution);
    v += 0.0256573563092388f*texture(iChannel0,(x+to_float2(-2,0))/iResolution);
    v += 0.0211673189551220f*texture(iChannel0,(x+to_float2(-2,2))/iResolution);
    v += 0.0118418567581102f*texture(iChannel0,(x+to_float2(-2,4))/iResolution);
    v += 0.00444069628429133f*texture(iChannel0,(x+to_float2(-2,6))/iResolution);
    v += 0.00109309446997941f*texture(iChannel0,(x+to_float2(-2,8))/iResolution);
    v += 0.000170796010934282f*texture(iChannel0,(x+to_float2(-2,10))/iResolution);
    v += 1.60749186761677e-5*texture(iChannel0,(x+to_float2(-2,12))/iResolution);
    v += 8.37235347717069e-7*texture(iChannel0,(x+to_float2(-2,14))/iResolution);
    v += 2.07364792004228e-8*texture(iChannel0,(x+to_float2(-2,16))/iResolution);
    v += 1.72803993336856e-10*texture(iChannel0,(x+to_float2(-2,18))/iResolution);
    v += 1.81898940354586e-11*texture(iChannel0,(x+to_float2(-1,-19))/iResolution);
    v += 3.45607986673713e-9*texture(iChannel0,(x+to_float2(-1,-17))/iResolution);
    v += 1.96996552404016e-7*texture(iChannel0,(x+to_float2(-1,-15))/iResolution);
    v += 5.02341208630241e-6*texture(iChannel0,(x+to_float2(-1,-13))/iResolution);
    v += 6.83184043737128e-5*texture(iChannel0,(x+to_float2(-1,-11))/iResolution);
    v += 0.000546547234989703f*texture(iChannel0,(x+to_float2(-1,-9))/iResolution);
    v += 0.00273273617494851f*texture(iChannel0,(x+to_float2(-1,-7))/iResolution);
    v += 0.00888139256858267f*texture(iChannel0,(x+to_float2(-1,-5))/iResolution);
    v += 0.0192430172319291f*texture(iChannel0,(x+to_float2(-1,-3))/iResolution);
    v += 0.0282230919401627f*texture(iChannel0,(x+to_float2(-1,-1))/iResolution);
    v += 0.0282230919401627f*texture(iChannel0,(x+to_float2(-1,1))/iResolution);
    v += 0.0192430172319291f*texture(iChannel0,(x+to_float2(-1,3))/iResolution);
    v += 0.00888139256858267f*texture(iChannel0,(x+to_float2(-1,5))/iResolution);
    v += 0.00273273617494851f*texture(iChannel0,(x+to_float2(-1,7))/iResolution);
    v += 0.000546547234989703f*texture(iChannel0,(x+to_float2(-1,9))/iResolution);
    v += 6.83184043737128e-5*texture(iChannel0,(x+to_float2(-1,11))/iResolution);
    v += 5.02341208630241e-6*texture(iChannel0,(x+to_float2(-1,13))/iResolution);
    v += 1.96996552404016e-7*texture(iChannel0,(x+to_float2(-1,15))/iResolution);
    v += 3.45607986673713e-9*texture(iChannel0,(x+to_float2(-1,17))/iResolution);
    v += 1.81898940354586e-11*texture(iChannel0,(x+to_float2(-1,19))/iResolution);
    v += 9.09494701772928e-13*texture(iChannel0,(x+to_float2(0,-20))/iResolution);
    v += 3.63797880709171e-10*texture(iChannel0,(x+to_float2(0,-18))/iResolution);
    v += 3.28327587340027e-8*texture(iChannel0,(x+to_float2(0,-16))/iResolution);
    v += 1.18197931442410e-6*texture(iChannel0,(x+to_float2(0,-14))/iResolution);
    v += 2.13495013667853e-5*texture(iChannel0,(x+to_float2(0,-12))/iResolution);
    v += 0.000218618893995881f*texture(iChannel0,(x+to_float2(0,-10))/iResolution);
    v += 0.00136636808747426f*texture(iChannel0,(x+to_float2(0,-8))/iResolution);
    v += 0.00546547234989703f*texture(iChannel0,(x+to_float2(0,-6))/iResolution);
    v += 0.0144322629239468f*texture(iChannel0,(x+to_float2(0,-4))/iResolution);
    v += 0.0256573563092388f*texture(iChannel0,(x+to_float2(0,-2))/iResolution);
    v += 0.0310454011341790f*texture(iChannel0,(x+to_float2(0,0))/iResolution);
    v += 0.0256573563092388f*texture(iChannel0,(x+to_float2(0,2))/iResolution);
    v += 0.0144322629239468f*texture(iChannel0,(x+to_float2(0,4))/iResolution);
    v += 0.00546547234989703f*texture(iChannel0,(x+to_float2(0,6))/iResolution);
    v += 0.00136636808747426f*texture(iChannel0,(x+to_float2(0,8))/iResolution);
    v += 0.000218618893995881f*texture(iChannel0,(x+to_float2(0,10))/iResolution);
    v += 2.13495013667853e-5*texture(iChannel0,(x+to_float2(0,12))/iResolution);
    v += 1.18197931442410e-6*texture(iChannel0,(x+to_float2(0,14))/iResolution);
    v += 3.28327587340027e-8*texture(iChannel0,(x+to_float2(0,16))/iResolution);
    v += 3.63797880709171e-10*texture(iChannel0,(x+to_float2(0,18))/iResolution);
    v += 9.09494701772928e-13*texture(iChannel0,(x+to_float2(0,20))/iResolution);
    v += 1.81898940354586e-11*texture(iChannel0,(x+to_float2(1,-19))/iResolution);
    v += 3.45607986673713e-9*texture(iChannel0,(x+to_float2(1,-17))/iResolution);
    v += 1.96996552404016e-7*texture(iChannel0,(x+to_float2(1,-15))/iResolution);
    v += 5.02341208630241e-6*texture(iChannel0,(x+to_float2(1,-13))/iResolution);
    v += 6.83184043737128e-5*texture(iChannel0,(x+to_float2(1,-11))/iResolution);
    v += 0.000546547234989703f*texture(iChannel0,(x+to_float2(1,-9))/iResolution);
    v += 0.00273273617494851f*texture(iChannel0,(x+to_float2(1,-7))/iResolution);
    v += 0.00888139256858267f*texture(iChannel0,(x+to_float2(1,-5))/iResolution);
    v += 0.0192430172319291f*texture(iChannel0,(x+to_float2(1,-3))/iResolution);
    v += 0.0282230919401627f*texture(iChannel0,(x+to_float2(1,-1))/iResolution);
    v += 0.0282230919401627f*texture(iChannel0,(x+to_float2(1,1))/iResolution);
    v += 0.0192430172319291f*texture(iChannel0,(x+to_float2(1,3))/iResolution);
    v += 0.00888139256858267f*texture(iChannel0,(x+to_float2(1,5))/iResolution);
    v += 0.00273273617494851f*texture(iChannel0,(x+to_float2(1,7))/iResolution);
    v += 0.000546547234989703f*texture(iChannel0,(x+to_float2(1,9))/iResolution);
    v += 6.83184043737128e-5*texture(iChannel0,(x+to_float2(1,11))/iResolution);
    v += 5.02341208630241e-6*texture(iChannel0,(x+to_float2(1,13))/iResolution);
    v += 1.96996552404016e-7*texture(iChannel0,(x+to_float2(1,15))/iResolution);
    v += 3.45607986673713e-9*texture(iChannel0,(x+to_float2(1,17))/iResolution);
    v += 1.81898940354586e-11*texture(iChannel0,(x+to_float2(1,19))/iResolution);
    v += 1.72803993336856e-10*texture(iChannel0,(x+to_float2(2,-18))/iResolution);
    v += 2.07364792004228e-8*texture(iChannel0,(x+to_float2(2,-16))/iResolution);
    v += 8.37235347717069e-7*texture(iChannel0,(x+to_float2(2,-14))/iResolution);
    v += 1.60749186761677e-5*texture(iChannel0,(x+to_float2(2,-12))/iResolution);
    v += 0.000170796010934282f*texture(iChannel0,(x+to_float2(2,-10))/iResolution);
    v += 0.00109309446997941f*texture(iChannel0,(x+to_float2(2,-8))/iResolution);
    v += 0.00444069628429133f*texture(iChannel0,(x+to_float2(2,-6))/iResolution);
    v += 0.0118418567581102f*texture(iChannel0,(x+to_float2(2,-4))/iResolution);
    v += 0.0211673189551220f*texture(iChannel0,(x+to_float2(2,-2))/iResolution);
    v += 0.0256573563092388f*texture(iChannel0,(x+to_float2(2,0))/iResolution);
    v += 0.0211673189551220f*texture(iChannel0,(x+to_float2(2,2))/iResolution);
    v += 0.0118418567581102f*texture(iChannel0,(x+to_float2(2,4))/iResolution);
    v += 0.00444069628429133f*texture(iChannel0,(x+to_float2(2,6))/iResolution);
    v += 0.00109309446997941f*texture(iChannel0,(x+to_float2(2,8))/iResolution);
    v += 0.000170796010934282f*texture(iChannel0,(x+to_float2(2,10))/iResolution);
    v += 1.60749186761677e-5*texture(iChannel0,(x+to_float2(2,12))/iResolution);
    v += 8.37235347717069e-7*texture(iChannel0,(x+to_float2(2,14))/iResolution);
    v += 2.07364792004228e-8*texture(iChannel0,(x+to_float2(2,16))/iResolution);
    v += 1.72803993336856e-10*texture(iChannel0,(x+to_float2(2,18))/iResolution);
    v += 1.03682396002114e-9*texture(iChannel0,(x+to_float2(3,-17))/iResolution);
    v += 8.81300366017967e-8*texture(iChannel0,(x+to_float2(3,-15))/iResolution);
    v += 2.67915311269462e-6*texture(iChannel0,(x+to_float2(3,-13))/iResolution);
    v += 4.01872966904193e-5*texture(iChannel0,(x+to_float2(3,-11))/iResolution);
    v += 0.000341592021868564f*texture(iChannel0,(x+to_float2(3,-9))/iResolution);
    v += 0.00177627851371653f*texture(iChannel0,(x+to_float2(3,-7))/iResolution);
    v += 0.00592092837905511f*texture(iChannel0,(x+to_float2(3,-5))/iResolution);
    v += 0.0130260424339212f*texture(iChannel0,(x+to_float2(3,-3))/iResolution);
    v += 0.0192430172319291f*texture(iChannel0,(x+to_float2(3,-1))/iResolution);
    v += 0.0192430172319291f*texture(iChannel0,(x+to_float2(3,1))/iResolution);
    v += 0.0130260424339212f*texture(iChannel0,(x+to_float2(3,3))/iResolution);
    v += 0.00592092837905511f*texture(iChannel0,(x+to_float2(3,5))/iResolution);
    v += 0.00177627851371653f*texture(iChannel0,(x+to_float2(3,7))/iResolution);
    v += 0.000341592021868564f*texture(iChannel0,(x+to_float2(3,9))/iResolution);
    v += 4.01872966904193e-5*texture(iChannel0,(x+to_float2(3,11))/iResolution);
    v += 2.67915311269462e-6*texture(iChannel0,(x+to_float2(3,13))/iResolution);
    v += 8.81300366017967e-8*texture(iChannel0,(x+to_float2(3,15))/iResolution);
    v += 1.03682396002114e-9*texture(iChannel0,(x+to_float2(3,17))/iResolution);
    v += 4.40650183008984e-9*texture(iChannel0,(x+to_float2(4,-16))/iResolution);
    v += 2.82016117125750e-7*texture(iChannel0,(x+to_float2(4,-14))/iResolution);
    v += 6.69788278173655e-6*texture(iChannel0,(x+to_float2(4,-12))/iResolution);
    v += 8.03745933808386e-5*texture(iChannel0,(x+to_float2(4,-10))/iResolution);
    v += 0.000555087035536417f*texture(iChannel0,(x+to_float2(4,-8))/iResolution);
    v += 0.00236837135162205f*texture(iChannel0,(x+to_float2(4,-6))/iResolution);
    v += 0.00651302121696062f*texture(iChannel0,(x+to_float2(4,-4))/iResolution);
    v += 0.0118418567581102f*texture(iChannel0,(x+to_float2(4,-2))/iResolution);
    v += 0.0144322629239468f*texture(iChannel0,(x+to_float2(4,0))/iResolution);
    v += 0.0118418567581102f*texture(iChannel0,(x+to_float2(4,2))/iResolution);
    v += 0.00651302121696062f*texture(iChannel0,(x+to_float2(4,4))/iResolution);
    v += 0.00236837135162205f*texture(iChannel0,(x+to_float2(4,6))/iResolution);
    v += 0.000555087035536417f*texture(iChannel0,(x+to_float2(4,8))/iResolution);
    v += 8.03745933808386e-5*texture(iChannel0,(x+to_float2(4,10))/iResolution);
    v += 6.69788278173655e-6*texture(iChannel0,(x+to_float2(4,12))/iResolution);
    v += 2.82016117125750e-7*texture(iChannel0,(x+to_float2(4,14))/iResolution);
    v += 4.40650183008984e-9*texture(iChannel0,(x+to_float2(4,16))/iResolution);
    v += 1.41008058562875e-8*texture(iChannel0,(x+to_float2(5,-15))/iResolution);
    v += 7.05040292814374e-7*texture(iChannel0,(x+to_float2(5,-13))/iResolution);
    v += 1.33957655634731e-5*texture(iChannel0,(x+to_float2(5,-11))/iResolution);
    v += 0.000130608714243863f*texture(iChannel0,(x+to_float2(5,-9))/iResolution);
    v += 0.000740116047381889f*texture(iChannel0,(x+to_float2(5,-7))/iResolution);
    v += 0.00260520848678425f*texture(iChannel0,(x+to_float2(5,-5))/iResolution);
    v += 0.00592092837905511f*texture(iChannel0,(x+to_float2(5,-3))/iResolution);
    v += 0.00888139256858267f*texture(iChannel0,(x+to_float2(5,-1))/iResolution);
    v += 0.00888139256858267f*texture(iChannel0,(x+to_float2(5,1))/iResolution);
    v += 0.00592092837905511f*texture(iChannel0,(x+to_float2(5,3))/iResolution);
    v += 0.00260520848678425f*texture(iChannel0,(x+to_float2(5,5))/iResolution);
    v += 0.000740116047381889f*texture(iChannel0,(x+to_float2(5,7))/iResolution);
    v += 0.000130608714243863f*texture(iChannel0,(x+to_float2(5,9))/iResolution);
    v += 1.33957655634731e-5*texture(iChannel0,(x+to_float2(5,11))/iResolution);
    v += 7.05040292814374e-7*texture(iChannel0,(x+to_float2(5,13))/iResolution);
    v += 1.41008058562875e-8*texture(iChannel0,(x+to_float2(5,15))/iResolution);
    v += 3.52520146407187e-8*texture(iChannel0,(x+to_float2(6,-14))/iResolution);
    v += 1.41008058562875e-6*texture(iChannel0,(x+to_float2(6,-12))/iResolution);
    v += 2.17681190406438e-5*texture(iChannel0,(x+to_float2(6,-10))/iResolution);
    v += 0.000174144952325150f*texture(iChannel0,(x+to_float2(6,-8))/iResolution);
    v += 0.000814127652120078f*texture(iChannel0,(x+to_float2(6,-6))/iResolution);
    v += 0.00236837135162205f*texture(iChannel0,(x+to_float2(6,-4))/iResolution);
    v += 0.00444069628429133f*texture(iChannel0,(x+to_float2(6,-2))/iResolution);
    v += 0.00546547234989703f*texture(iChannel0,(x+to_float2(6,0))/iResolution);
    v += 0.00444069628429133f*texture(iChannel0,(x+to_float2(6,2))/iResolution);
    v += 0.00236837135162205f*texture(iChannel0,(x+to_float2(6,4))/iResolution);
    v += 0.000814127652120078f*texture(iChannel0,(x+to_float2(6,6))/iResolution);
    v += 0.000174144952325150f*texture(iChannel0,(x+to_float2(6,8))/iResolution);
    v += 2.17681190406438e-5*texture(iChannel0,(x+to_float2(6,10))/iResolution);
    v += 1.41008058562875e-6*texture(iChannel0,(x+to_float2(6,12))/iResolution);
    v += 3.52520146407187e-8*texture(iChannel0,(x+to_float2(6,14))/iResolution);
    v += 7.05040292814374e-8*texture(iChannel0,(x+to_float2(7,-13))/iResolution);
    v += 2.29138095164672e-6*texture(iChannel0,(x+to_float2(7,-11))/iResolution);
    v += 2.90241587208584e-5*texture(iChannel0,(x+to_float2(7,-9))/iResolution);
    v += 0.000191559447557665f*texture(iChannel0,(x+to_float2(7,-7))/iResolution);
    v += 0.000740116047381889f*texture(iChannel0,(x+to_float2(7,-5))/iResolution);
    v += 0.00177627851371653f*texture(iChannel0,(x+to_float2(7,-3))/iResolution);
    v += 0.00273273617494851f*texture(iChannel0,(x+to_float2(7,-1))/iResolution);
    v += 0.00273273617494851f*texture(iChannel0,(x+to_float2(7,1))/iResolution);
    v += 0.00177627851371653f*texture(iChannel0,(x+to_float2(7,3))/iResolution);
    v += 0.000740116047381889f*texture(iChannel0,(x+to_float2(7,5))/iResolution);
    v += 0.000191559447557665f*texture(iChannel0,(x+to_float2(7,7))/iResolution);
    v += 2.90241587208584e-5*texture(iChannel0,(x+to_float2(7,9))/iResolution);
    v += 2.29138095164672e-6*texture(iChannel0,(x+to_float2(7,11))/iResolution);
    v += 7.05040292814374e-8*texture(iChannel0,(x+to_float2(7,13))/iResolution);
    v += 1.14569047582336e-7*texture(iChannel0,(x+to_float2(8,-12))/iResolution);
    v += 3.05517460219562e-6*texture(iChannel0,(x+to_float2(8,-10))/iResolution);
    v += 3.19265745929442e-5*texture(iChannel0,(x+to_float2(8,-8))/iResolution);
    v += 0.000174144952325150f*texture(iChannel0,(x+to_float2(8,-6))/iResolution);
    v += 0.000555087035536417f*texture(iChannel0,(x+to_float2(8,-4))/iResolution);
    v += 0.00109309446997941f*texture(iChannel0,(x+to_float2(8,-2))/iResolution);
    v += 0.00136636808747426f*texture(iChannel0,(x+to_float2(8,0))/iResolution);
    v += 0.00109309446997941f*texture(iChannel0,(x+to_float2(8,2))/iResolution);
    v += 0.000555087035536417f*texture(iChannel0,(x+to_float2(8,4))/iResolution);
    v += 0.000174144952325150f*texture(iChannel0,(x+to_float2(8,6))/iResolution);
    v += 3.19265745929442e-5*texture(iChannel0,(x+to_float2(8,8))/iResolution);
    v += 3.05517460219562e-6*texture(iChannel0,(x+to_float2(8,10))/iResolution);
    v += 1.14569047582336e-7*texture(iChannel0,(x+to_float2(8,12))/iResolution);
    v += 1.52758730109781e-7*texture(iChannel0,(x+to_float2(9,-11))/iResolution);
    v += 3.36069206241518e-6*texture(iChannel0,(x+to_float2(9,-9))/iResolution);
    v += 2.90241587208584e-5*texture(iChannel0,(x+to_float2(9,-7))/iResolution);
    v += 0.000130608714243863f*texture(iChannel0,(x+to_float2(9,-5))/iResolution);
    v += 0.000341592021868564f*texture(iChannel0,(x+to_float2(9,-3))/iResolution);
    v += 0.000546547234989703f*texture(iChannel0,(x+to_float2(9,-1))/iResolution);
    v += 0.000546547234989703f*texture(iChannel0,(x+to_float2(9,1))/iResolution);
    v += 0.000341592021868564f*texture(iChannel0,(x+to_float2(9,3))/iResolution);
    v += 0.000130608714243863f*texture(iChannel0,(x+to_float2(9,5))/iResolution);
    v += 2.90241587208584e-5*texture(iChannel0,(x+to_float2(9,7))/iResolution);
    v += 3.36069206241518e-6*texture(iChannel0,(x+to_float2(9,9))/iResolution);
    v += 1.52758730109781e-7*texture(iChannel0,(x+to_float2(9,11))/iResolution);
    v += 1.68034603120759e-7*texture(iChannel0,(x+to_float2(10,-10))/iResolution);
    v += 3.05517460219562e-6*texture(iChannel0,(x+to_float2(10,-8))/iResolution);
    v += 2.17681190406438e-5*texture(iChannel0,(x+to_float2(10,-6))/iResolution);
    v += 8.03745933808386e-5*texture(iChannel0,(x+to_float2(10,-4))/iResolution);
    v += 0.000170796010934282f*texture(iChannel0,(x+to_float2(10,-2))/iResolution);
    v += 0.000218618893995881f*texture(iChannel0,(x+to_float2(10,0))/iResolution);
    v += 0.000170796010934282f*texture(iChannel0,(x+to_float2(10,2))/iResolution);
    v += 8.03745933808386e-5*texture(iChannel0,(x+to_float2(10,4))/iResolution);
    v += 2.17681190406438e-5*texture(iChannel0,(x+to_float2(10,6))/iResolution);
    v += 3.05517460219562e-6*texture(iChannel0,(x+to_float2(10,8))/iResolution);
    v += 1.68034603120759e-7*texture(iChannel0,(x+to_float2(10,10))/iResolution);
    v += 1.52758730109781e-7*texture(iChannel0,(x+to_float2(11,-9))/iResolution);
    v += 2.29138095164672e-6*texture(iChannel0,(x+to_float2(11,-7))/iResolution);
    v += 1.33957655634731e-5*texture(iChannel0,(x+to_float2(11,-5))/iResolution);
    v += 4.01872966904193e-5*texture(iChannel0,(x+to_float2(11,-3))/iResolution);
    v += 6.83184043737128e-5*texture(iChannel0,(x+to_float2(11,-1))/iResolution);
    v += 6.83184043737128e-5*texture(iChannel0,(x+to_float2(11,1))/iResolution);
    v += 4.01872966904193e-5*texture(iChannel0,(x+to_float2(11,3))/iResolution);
    v += 1.33957655634731e-5*texture(iChannel0,(x+to_float2(11,5))/iResolution);
    v += 2.29138095164672e-6*texture(iChannel0,(x+to_float2(11,7))/iResolution);
    v += 1.52758730109781e-7*texture(iChannel0,(x+to_float2(11,9))/iResolution);
    v += 1.14569047582336e-7*texture(iChannel0,(x+to_float2(12,-8))/iResolution);
    v += 1.41008058562875e-6*texture(iChannel0,(x+to_float2(12,-6))/iResolution);
    v += 6.69788278173655e-6*texture(iChannel0,(x+to_float2(12,-4))/iResolution);
    v += 1.60749186761677e-5*texture(iChannel0,(x+to_float2(12,-2))/iResolution);
    v += 2.13495013667853e-5*texture(iChannel0,(x+to_float2(12,0))/iResolution);
    v += 1.60749186761677e-5*texture(iChannel0,(x+to_float2(12,2))/iResolution);
    v += 6.69788278173655e-6*texture(iChannel0,(x+to_float2(12,4))/iResolution);
    v += 1.41008058562875e-6*texture(iChannel0,(x+to_float2(12,6))/iResolution);
    v += 1.14569047582336e-7*texture(iChannel0,(x+to_float2(12,8))/iResolution);
    v += 7.05040292814374e-8*texture(iChannel0,(x+to_float2(13,-7))/iResolution);
    v += 7.05040292814374e-7*texture(iChannel0,(x+to_float2(13,-5))/iResolution);
    v += 2.67915311269462e-6*texture(iChannel0,(x+to_float2(13,-3))/iResolution);
    v += 5.02341208630241e-6*texture(iChannel0,(x+to_float2(13,-1))/iResolution);
    v += 5.02341208630241e-6*texture(iChannel0,(x+to_float2(13,1))/iResolution);
    v += 2.67915311269462e-6*texture(iChannel0,(x+to_float2(13,3))/iResolution);
    v += 7.05040292814374e-7*texture(iChannel0,(x+to_float2(13,5))/iResolution);
    v += 7.05040292814374e-8*texture(iChannel0,(x+to_float2(13,7))/iResolution);
    v += 3.52520146407187e-8*texture(iChannel0,(x+to_float2(14,-6))/iResolution);
    v += 2.82016117125750e-7*texture(iChannel0,(x+to_float2(14,-4))/iResolution);
    v += 8.37235347717069e-7*texture(iChannel0,(x+to_float2(14,-2))/iResolution);
    v += 1.18197931442410e-6*texture(iChannel0,(x+to_float2(14,0))/iResolution);
    v += 8.37235347717069e-7*texture(iChannel0,(x+to_float2(14,2))/iResolution);
    v += 2.82016117125750e-7*texture(iChannel0,(x+to_float2(14,4))/iResolution);
    v += 3.52520146407187e-8*texture(iChannel0,(x+to_float2(14,6))/iResolution);
    v += 1.41008058562875e-8*texture(iChannel0,(x+to_float2(15,-5))/iResolution);
    v += 8.81300366017967e-8*texture(iChannel0,(x+to_float2(15,-3))/iResolution);
    v += 1.96996552404016e-7*texture(iChannel0,(x+to_float2(15,-1))/iResolution);
    v += 1.96996552404016e-7*texture(iChannel0,(x+to_float2(15,1))/iResolution);
    v += 8.81300366017967e-8*texture(iChannel0,(x+to_float2(15,3))/iResolution);
    v += 1.41008058562875e-8*texture(iChannel0,(x+to_float2(15,5))/iResolution);
    v += 4.40650183008984e-9*texture(iChannel0,(x+to_float2(16,-4))/iResolution);
    v += 2.07364792004228e-8*texture(iChannel0,(x+to_float2(16,-2))/iResolution);
    v += 3.28327587340027e-8*texture(iChannel0,(x+to_float2(16,0))/iResolution);
    v += 2.07364792004228e-8*texture(iChannel0,(x+to_float2(16,2))/iResolution);
    v += 4.40650183008984e-9*texture(iChannel0,(x+to_float2(16,4))/iResolution);
    v += 1.03682396002114e-9*texture(iChannel0,(x+to_float2(17,-3))/iResolution);
    v += 3.45607986673713e-9*texture(iChannel0,(x+to_float2(17,-1))/iResolution);
    v += 3.45607986673713e-9*texture(iChannel0,(x+to_float2(17,1))/iResolution);
    v += 1.03682396002114e-9*texture(iChannel0,(x+to_float2(17,3))/iResolution);
    v += 1.72803993336856e-10*texture(iChannel0,(x+to_float2(18,-2))/iResolution);
    v += 3.63797880709171e-10*texture(iChannel0,(x+to_float2(18,0))/iResolution);
    v += 1.72803993336856e-10*texture(iChannel0,(x+to_float2(18,2))/iResolution);
    v += 1.81898940354586e-11*texture(iChannel0,(x+to_float2(19,-1))/iResolution);
    v += 1.81898940354586e-11*texture(iChannel0,(x+to_float2(19,1))/iResolution);
    v += 9.09494701772928e-13*texture(iChannel0,(x+to_float2(20,0))/iResolution);
   
    return v.x;
}

__DEVICE__ float JacobiSolveForPressureBPart(float2 x, float2 iResolution, __TEXTURE2D__ iChannel1) 
{
    float4 v;

v += -9.09494701772928e-13*texture(iChannel1,(x+to_float2(-19,0))/iResolution);
v += -1.72803993336856e-11*texture(iChannel1,(x+to_float2(-18,-1))/iResolution);
v += -3.63797880709171e-12*texture(iChannel1,(x+to_float2(-18,0))/iResolution);
v += -1.72803993336856e-11*texture(iChannel1,(x+to_float2(-18,1))/iResolution);
v += -1.55523594003171e-10*texture(iChannel1,(x+to_float2(-17,-2))/iResolution);
v += -6.54836185276508e-11*texture(iChannel1,(x+to_float2(-17,-1))/iResolution);
v += -3.42879502568394e-10*texture(iChannel1,(x+to_float2(-17,0))/iResolution);
v += -6.54836185276508e-11*texture(iChannel1,(x+to_float2(-17,1))/iResolution);
v += -1.55523594003171e-10*texture(iChannel1,(x+to_float2(-17,2))/iResolution);
v += -8.81300366017967e-10*texture(iChannel1,(x+to_float2(-16,-3))/iResolution);
v += -5.56610757485032e-10*texture(iChannel1,(x+to_float2(-16,-2))/iResolution);
v += -3.20233084494248e-9*texture(iChannel1,(x+to_float2(-16,-1))/iResolution);
v += -1.23691279441118e-9*texture(iChannel1,(x+to_float2(-16,0))/iResolution);
v += -3.20233084494248e-9*texture(iChannel1,(x+to_float2(-16,1))/iResolution);
v += -5.56610757485032e-10*texture(iChannel1,(x+to_float2(-16,2))/iResolution);
v += -8.81300366017967e-10*texture(iChannel1,(x+to_float2(-16,3))/iResolution);
v += -3.52520146407187e-9*texture(iChannel1,(x+to_float2(-15,-4))/iResolution);
v += -2.96859070658684e-9*texture(iChannel1,(x+to_float2(-15,-3))/iResolution);
v += -1.87237674253993e-8*texture(iChannel1,(x+to_float2(-15,-2))/iResolution);
v += -1.09503162093461e-8*texture(iChannel1,(x+to_float2(-15,-1))/iResolution);
v += -3.10328687191941e-8*texture(iChannel1,(x+to_float2(-15,0))/iResolution);
v += -1.09503162093461e-8*texture(iChannel1,(x+to_float2(-15,1))/iResolution);
v += -1.87237674253993e-8*texture(iChannel1,(x+to_float2(-15,2))/iResolution);
v += -2.96859070658684e-9*texture(iChannel1,(x+to_float2(-15,3))/iResolution);
v += -3.52520146407187e-9*texture(iChannel1,(x+to_float2(-15,4))/iResolution);
v += -1.05756043922156e-8*texture(iChannel1,(x+to_float2(-14,-5))/iResolution);
v += -1.11322151497006e-8*texture(iChannel1,(x+to_float2(-14,-4))/iResolution);
v += -7.68741301726550e-8*texture(iChannel1,(x+to_float2(-14,-3))/iResolution);
v += -6.04195520281792e-8*texture(iChannel1,(x+to_float2(-14,-2))/iResolution);
v += -1.87838850251865e-7*texture(iChannel1,(x+to_float2(-14,-1))/iResolution);
v += -1.00993929663673e-7*texture(iChannel1,(x+to_float2(-14,0))/iResolution);
v += -1.87838850251865e-7*texture(iChannel1,(x+to_float2(-14,1))/iResolution);
v += -6.04195520281792e-8*texture(iChannel1,(x+to_float2(-14,2))/iResolution);
v += -7.68741301726550e-8*texture(iChannel1,(x+to_float2(-14,3))/iResolution);
v += -1.11322151497006e-8*texture(iChannel1,(x+to_float2(-14,4))/iResolution);
v += -1.05756043922156e-8*texture(iChannel1,(x+to_float2(-14,5))/iResolution);
v += -2.46764102485031e-8*texture(iChannel1,(x+to_float2(-13,-6))/iResolution);
v += -3.11702024191618e-8*texture(iChannel1,(x+to_float2(-13,-5))/iResolution);
v += -2.35570041695610e-7*texture(iChannel1,(x+to_float2(-13,-4))/iResolution);
v += -2.32976162806153e-7*texture(iChannel1,(x+to_float2(-13,-3))/iResolution);
v += -7.95476807979867e-7*texture(iChannel1,(x+to_float2(-13,-2))/iResolution);
v += -5.78991603106260e-7*texture(iChannel1,(x+to_float2(-13,-1))/iResolution);
v += -1.17924446385587e-6*texture(iChannel1,(x+to_float2(-13,0))/iResolution);
v += -5.78991603106260e-7*texture(iChannel1,(x+to_float2(-13,1))/iResolution);
v += -7.95476807979867e-7*texture(iChannel1,(x+to_float2(-13,2))/iResolution);
v += -2.32976162806153e-7*texture(iChannel1,(x+to_float2(-13,3))/iResolution);
v += -2.35570041695610e-7*texture(iChannel1,(x+to_float2(-13,4))/iResolution);
v += -3.11702024191618e-8*texture(iChannel1,(x+to_float2(-13,5))/iResolution);
v += -2.46764102485031e-8*texture(iChannel1,(x+to_float2(-13,6))/iResolution);
v += -4.58276190329343e-8*texture(iChannel1,(x+to_float2(-12,-7))/iResolution);
v += -6.75354385748506e-8*texture(iChannel1,(x+to_float2(-12,-6))/iResolution);
v += -5.58899046154693e-7*texture(iChannel1,(x+to_float2(-12,-5))/iResolution);
v += -6.67001586407423e-7*texture(iChannel1,(x+to_float2(-12,-4))/iResolution);
v += -2.50313678407110e-6*texture(iChannel1,(x+to_float2(-12,-3))/iResolution);
v += -2.30951991397887e-6*texture(iChannel1,(x+to_float2(-12,-2))/iResolution);
v += -5.17681837663986e-6*texture(iChannel1,(x+to_float2(-12,-1))/iResolution);
v += -3.45800071954727e-6*texture(iChannel1,(x+to_float2(-12,0))/iResolution);
v += -5.17681837663986e-6*texture(iChannel1,(x+to_float2(-12,1))/iResolution);
v += -2.30951991397887e-6*texture(iChannel1,(x+to_float2(-12,2))/iResolution);
v += -2.50313678407110e-6*texture(iChannel1,(x+to_float2(-12,3))/iResolution);
v += -6.67001586407423e-7*texture(iChannel1,(x+to_float2(-12,4))/iResolution);
v += -5.58899046154693e-7*texture(iChannel1,(x+to_float2(-12,5))/iResolution);
v += -6.75354385748506e-8*texture(iChannel1,(x+to_float2(-12,6))/iResolution);
v += -4.58276190329343e-8*texture(iChannel1,(x+to_float2(-12,7))/iResolution);
v += -6.87414285494015e-8*texture(iChannel1,(x+to_float2(-11,-8))/iResolution);
v += -1.15775037556887e-7*texture(iChannel1,(x+to_float2(-11,-7))/iResolution);
v += -1.05081926449202e-6*texture(iChannel1,(x+to_float2(-11,-6))/iResolution);
v += -1.46988895721734e-6*texture(iChannel1,(x+to_float2(-11,-5))/iResolution);
v += -6.06828325544484e-6*texture(iChannel1,(x+to_float2(-11,-4))/iResolution);
v += -6.80304947309196e-6*texture(iChannel1,(x+to_float2(-11,-3))/iResolution);
v += -1.68375663633924e-5*texture(iChannel1,(x+to_float2(-11,-2))/iResolution);
v += -1.43607612699270e-5*texture(iChannel1,(x+to_float2(-11,-1))/iResolution);
v += -2.36486230278388e-5*texture(iChannel1,(x+to_float2(-11,0))/iResolution);
v += -1.43607612699270e-5*texture(iChannel1,(x+to_float2(-11,1))/iResolution);
v += -1.68375663633924e-5*texture(iChannel1,(x+to_float2(-11,2))/iResolution);
v += -6.80304947309196e-6*texture(iChannel1,(x+to_float2(-11,3))/iResolution);
v += -6.06828325544484e-6*texture(iChannel1,(x+to_float2(-11,4))/iResolution);
v += -1.46988895721734e-6*texture(iChannel1,(x+to_float2(-11,5))/iResolution);
v += -1.05081926449202e-6*texture(iChannel1,(x+to_float2(-11,6))/iResolution);
v += -1.15775037556887e-7*texture(iChannel1,(x+to_float2(-11,7))/iResolution);
v += -6.87414285494015e-8*texture(iChannel1,(x+to_float2(-11,8))/iResolution);
v += -8.40173015603796e-8*texture(iChannel1,(x+to_float2(-10,-9))/iResolution);
v += -1.59190676640719e-7*texture(iChannel1,(x+to_float2(-10,-8))/iResolution);
v += -1.58909278979991e-6*texture(iChannel1,(x+to_float2(-10,-7))/iResolution);
v += -2.55007762461901e-6*texture(iChannel1,(x+to_float2(-10,-6))/iResolution);
v += -1.15973198262509e-5*texture(iChannel1,(x+to_float2(-10,-5))/iResolution);
v += -1.53331930050626e-5*texture(iChannel1,(x+to_float2(-10,-4))/iResolution);
v += -4.19905081798788e-5*texture(iChannel1,(x+to_float2(-10,-3))/iResolution);
v += -4.38769347965717e-5*texture(iChannel1,(x+to_float2(-10,-2))/iResolution);
v += -8.00984416855499e-5*texture(iChannel1,(x+to_float2(-10,-1))/iResolution);
v += -6.24149688519537e-5*texture(iChannel1,(x+to_float2(-10,0))/iResolution);
v += -8.00984416855499e-5*texture(iChannel1,(x+to_float2(-10,1))/iResolution);
v += -4.38769347965717e-5*texture(iChannel1,(x+to_float2(-10,2))/iResolution);
v += -4.19905081798788e-5*texture(iChannel1,(x+to_float2(-10,3))/iResolution);
v += -1.53331930050626e-5*texture(iChannel1,(x+to_float2(-10,4))/iResolution);
v += -1.15973198262509e-5*texture(iChannel1,(x+to_float2(-10,5))/iResolution);
v += -2.55007762461901e-6*texture(iChannel1,(x+to_float2(-10,6))/iResolution);
v += -1.58909278979991e-6*texture(iChannel1,(x+to_float2(-10,7))/iResolution);
v += -1.59190676640719e-7*texture(iChannel1,(x+to_float2(-10,8))/iResolution);
v += -8.40173015603796e-8*texture(iChannel1,(x+to_float2(-10,9))/iResolution);
v += -8.40173015603796e-8*texture(iChannel1,(x+to_float2(-9,-10))/iResolution);
v += -1.76878529600799e-7*texture(iChannel1,(x+to_float2(-9,-9))/iResolution);
v += -1.95008578884881e-6*texture(iChannel1,(x+to_float2(-9,-8))/iResolution);
v += -3.53132782038301e-6*texture(iChannel1,(x+to_float2(-9,-7))/iResolution);
v += -1.77311976585770e-5*texture(iChannel1,(x+to_float2(-9,-6))/iResolution);
v += -2.70361197181046e-5*texture(iChannel1,(x+to_float2(-9,-5))/iResolution);
v += -8.20512541395146e-5*texture(iChannel1,(x+to_float2(-9,-4))/iResolution);
v += -0.000101948855444789f*texture(iChannel1,(x+to_float2(-9,-3))/iResolution);
v += -0.000207377233891748f*texture(iChannel1,(x+to_float2(-9,-2))/iResolution);
v += -0.000199741101823747f*texture(iChannel1,(x+to_float2(-9,-1))/iResolution);
v += -0.000284433263004757f*texture(iChannel1,(x+to_float2(-9,0))/iResolution);
v += -0.000199741101823747f*texture(iChannel1,(x+to_float2(-9,1))/iResolution);
v += -0.000207377233891748f*texture(iChannel1,(x+to_float2(-9,2))/iResolution);
v += -0.000101948855444789f*texture(iChannel1,(x+to_float2(-9,3))/iResolution);
v += -8.20512541395146e-5*texture(iChannel1,(x+to_float2(-9,4))/iResolution);
v += -2.70361197181046e-5*texture(iChannel1,(x+to_float2(-9,5))/iResolution);
v += -1.77311976585770e-5*texture(iChannel1,(x+to_float2(-9,6))/iResolution);
v += -3.53132782038301e-6*texture(iChannel1,(x+to_float2(-9,7))/iResolution);
v += -1.95008578884881e-6*texture(iChannel1,(x+to_float2(-9,8))/iResolution);
v += -1.76878529600799e-7*texture(iChannel1,(x+to_float2(-9,9))/iResolution);
v += -8.40173015603796e-8*texture(iChannel1,(x+to_float2(-9,10))/iResolution);
v += -6.87414285494015e-8*texture(iChannel1,(x+to_float2(-8,-11))/iResolution);
v += -1.59190676640719e-7*texture(iChannel1,(x+to_float2(-8,-10))/iResolution);
v += -1.95008578884881e-6*texture(iChannel1,(x+to_float2(-8,-9))/iResolution);
v += -3.93294612877071e-6*texture(iChannel1,(x+to_float2(-8,-8))/iResolution);
v += -2.18790937651647e-5*texture(iChannel1,(x+to_float2(-8,-7))/iResolution);
v += -3.78072654712014e-5*texture(iChannel1,(x+to_float2(-8,-6))/iResolution);
v += -0.000127373421491939f*texture(iChannel1,(x+to_float2(-8,-5))/iResolution);
v += -0.000183886848390102f*texture(iChannel1,(x+to_float2(-8,-4))/iResolution);
v += -0.000417968447436579f*texture(iChannel1,(x+to_float2(-8,-3))/iResolution);
v += -0.000483942043501884f*texture(iChannel1,(x+to_float2(-8,-2))/iResolution);
v += -0.000773602703702636f*texture(iChannel1,(x+to_float2(-8,-1))/iResolution);
v += -0.000675835879519582f*texture(iChannel1,(x+to_float2(-8,0))/iResolution);
v += -0.000773602703702636f*texture(iChannel1,(x+to_float2(-8,1))/iResolution);
v += -0.000483942043501884f*texture(iChannel1,(x+to_float2(-8,2))/iResolution);
v += -0.000417968447436579f*texture(iChannel1,(x+to_float2(-8,3))/iResolution);
v += -0.000183886848390102f*texture(iChannel1,(x+to_float2(-8,4))/iResolution);
v += -0.000127373421491939f*texture(iChannel1,(x+to_float2(-8,5))/iResolution);
v += -3.78072654712014e-5*texture(iChannel1,(x+to_float2(-8,6))/iResolution);
v += -2.18790937651647e-5*texture(iChannel1,(x+to_float2(-8,7))/iResolution);
v += -3.93294612877071e-6*texture(iChannel1,(x+to_float2(-8,8))/iResolution);
v += -1.95008578884881e-6*texture(iChannel1,(x+to_float2(-8,9))/iResolution);
v += -1.59190676640719e-7*texture(iChannel1,(x+to_float2(-8,10))/iResolution);
v += -6.87414285494015e-8*texture(iChannel1,(x+to_float2(-8,11))/iResolution);
v += -4.58276190329343e-8*texture(iChannel1,(x+to_float2(-7,-12))/iResolution);
v += -1.15775037556887e-7*texture(iChannel1,(x+to_float2(-7,-11))/iResolution);
v += -1.58909278979991e-6*texture(iChannel1,(x+to_float2(-7,-10))/iResolution);
v += -3.53132782038301e-6*texture(iChannel1,(x+to_float2(-7,-9))/iResolution);
v += -2.18790937651647e-5*texture(iChannel1,(x+to_float2(-7,-8))/iResolution);
v += -4.22448356403038e-5*texture(iChannel1,(x+to_float2(-7,-7))/iResolution);
v += -0.000158390301294276f*texture(iChannel1,(x+to_float2(-7,-6))/iResolution);
v += -0.000260763452388346f*texture(iChannel1,(x+to_float2(-7,-5))/iResolution);
v += -0.000663241306028794f*texture(iChannel1,(x+to_float2(-7,-4))/iResolution);
v += -0.000902096042409539f*texture(iChannel1,(x+to_float2(-7,-3))/iResolution);
v += -0.00162991425895598f*texture(iChannel1,(x+to_float2(-7,-2))/iResolution);
v += -0.00173489178996533f*texture(iChannel1,(x+to_float2(-7,-1))/iResolution);
v += -0.00223807293514255f*texture(iChannel1,(x+to_float2(-7,0))/iResolution);
v += -0.00173489178996533f*texture(iChannel1,(x+to_float2(-7,1))/iResolution);
v += -0.00162991425895598f*texture(iChannel1,(x+to_float2(-7,2))/iResolution);
v += -0.000902096042409539f*texture(iChannel1,(x+to_float2(-7,3))/iResolution);
v += -0.000663241306028794f*texture(iChannel1,(x+to_float2(-7,4))/iResolution);
v += -0.000260763452388346f*texture(iChannel1,(x+to_float2(-7,5))/iResolution);
v += -0.000158390301294276f*texture(iChannel1,(x+to_float2(-7,6))/iResolution);
v += -4.22448356403038e-5*texture(iChannel1,(x+to_float2(-7,7))/iResolution);
v += -2.18790937651647e-5*texture(iChannel1,(x+to_float2(-7,8))/iResolution);
v += -3.53132782038301e-6*texture(iChannel1,(x+to_float2(-7,9))/iResolution);
v += -1.58909278979991e-6*texture(iChannel1,(x+to_float2(-7,10))/iResolution);
v += -1.15775037556887e-7*texture(iChannel1,(x+to_float2(-7,11))/iResolution);
v += -4.58276190329343e-8*texture(iChannel1,(x+to_float2(-7,12))/iResolution);
v += -2.46764102485031e-8*texture(iChannel1,(x+to_float2(-6,-13))/iResolution);
v += -6.75354385748506e-8*texture(iChannel1,(x+to_float2(-6,-12))/iResolution);
v += -1.05081926449202e-6*texture(iChannel1,(x+to_float2(-6,-11))/iResolution);
v += -2.55007762461901e-6*texture(iChannel1,(x+to_float2(-6,-10))/iResolution);
v += -1.77311976585770e-5*texture(iChannel1,(x+to_float2(-6,-9))/iResolution);
v += -3.78072654712014e-5*texture(iChannel1,(x+to_float2(-6,-8))/iResolution);
v += -0.000158390301294276f*texture(iChannel1,(x+to_float2(-6,-7))/iResolution);
v += -0.000292745651677251f*texture(iChannel1,(x+to_float2(-6,-6))/iResolution);
v += -0.000834164828120265f*texture(iChannel1,(x+to_float2(-6,-5))/iResolution);
v += -0.00130621888092719f*texture(iChannel1,(x+to_float2(-6,-4))/iResolution);
v += -0.00267353867093334f*texture(iChannel1,(x+to_float2(-6,-3))/iResolution);
v += -0.00339872715994716f*texture(iChannel1,(x+to_float2(-6,-2))/iResolution);
v += -0.00503071343700867f*texture(iChannel1,(x+to_float2(-6,-1))/iResolution);
v += -0.00480667228111997f*texture(iChannel1,(x+to_float2(-6,0))/iResolution);
v += -0.00503071343700867f*texture(iChannel1,(x+to_float2(-6,1))/iResolution);
v += -0.00339872715994716f*texture(iChannel1,(x+to_float2(-6,2))/iResolution);
v += -0.00267353867093334f*texture(iChannel1,(x+to_float2(-6,3))/iResolution);
v += -0.00130621888092719f*texture(iChannel1,(x+to_float2(-6,4))/iResolution);
v += -0.000834164828120265f*texture(iChannel1,(x+to_float2(-6,5))/iResolution);
v += -0.000292745651677251f*texture(iChannel1,(x+to_float2(-6,6))/iResolution);
v += -0.000158390301294276f*texture(iChannel1,(x+to_float2(-6,7))/iResolution);
v += -3.78072654712014e-5*texture(iChannel1,(x+to_float2(-6,8))/iResolution);
v += -1.77311976585770e-5*texture(iChannel1,(x+to_float2(-6,9))/iResolution);
v += -2.55007762461901e-6*texture(iChannel1,(x+to_float2(-6,10))/iResolution);
v += -1.05081926449202e-6*texture(iChannel1,(x+to_float2(-6,11))/iResolution);
v += -6.75354385748506e-8*texture(iChannel1,(x+to_float2(-6,12))/iResolution);
v += -2.46764102485031e-8*texture(iChannel1,(x+to_float2(-6,13))/iResolution);
v += -1.05756043922156e-8*texture(iChannel1,(x+to_float2(-5,-14))/iResolution);
v += -3.11702024191618e-8*texture(iChannel1,(x+to_float2(-5,-13))/iResolution);
v += -5.58899046154693e-7*texture(iChannel1,(x+to_float2(-5,-12))/iResolution);
v += -1.46988895721734e-6*texture(iChannel1,(x+to_float2(-5,-11))/iResolution);
v += -1.15973198262509e-5*texture(iChannel1,(x+to_float2(-5,-10))/iResolution);
v += -2.70361197181046e-5*texture(iChannel1,(x+to_float2(-5,-9))/iResolution);
v += -0.000127373421491939f*texture(iChannel1,(x+to_float2(-5,-8))/iResolution);
v += -0.000260763452388346f*texture(iChannel1,(x+to_float2(-5,-7))/iResolution);
v += -0.000834164828120265f*texture(iChannel1,(x+to_float2(-5,-6))/iResolution);
v += -0.00147693132748827f*texture(iChannel1,(x+to_float2(-5,-5))/iResolution);
v += -0.00342230207024841f*texture(iChannel1,(x+to_float2(-5,-4))/iResolution);
v += -0.00508711260044947f*texture(iChannel1,(x+to_float2(-5,-3))/iResolution);
v += -0.00870143855718197f*texture(iChannel1,(x+to_float2(-5,-2))/iResolution);
v += -0.0101825625170022f*texture(iChannel1,(x+to_float2(-5,-1))/iResolution);
v += -0.0123926616652170f*texture(iChannel1,(x+to_float2(-5,0))/iResolution);
v += -0.0101825625170022f*texture(iChannel1,(x+to_float2(-5,1))/iResolution);
v += -0.00870143855718197f*texture(iChannel1,(x+to_float2(-5,2))/iResolution);
v += -0.00508711260044947f*texture(iChannel1,(x+to_float2(-5,3))/iResolution);
v += -0.00342230207024841f*texture(iChannel1,(x+to_float2(-5,4))/iResolution);
v += -0.00147693132748827f*texture(iChannel1,(x+to_float2(-5,5))/iResolution);
v += -0.000834164828120265f*texture(iChannel1,(x+to_float2(-5,6))/iResolution);
v += -0.000260763452388346f*texture(iChannel1,(x+to_float2(-5,7))/iResolution);
v += -0.000127373421491939f*texture(iChannel1,(x+to_float2(-5,8))/iResolution);
v += -2.70361197181046e-5*texture(iChannel1,(x+to_float2(-5,9))/iResolution);
v += -1.15973198262509e-5*texture(iChannel1,(x+to_float2(-5,10))/iResolution);
v += -1.46988895721734e-6*texture(iChannel1,(x+to_float2(-5,11))/iResolution);
v += -5.58899046154693e-7*texture(iChannel1,(x+to_float2(-5,12))/iResolution);
v += -3.11702024191618e-8*texture(iChannel1,(x+to_float2(-5,13))/iResolution);
v += -1.05756043922156e-8*texture(iChannel1,(x+to_float2(-5,14))/iResolution);
v += -3.52520146407187e-9*texture(iChannel1,(x+to_float2(-4,-15))/iResolution);
v += -1.11322151497006e-8*texture(iChannel1,(x+to_float2(-4,-14))/iResolution);
v += -2.35570041695610e-7*texture(iChannel1,(x+to_float2(-4,-13))/iResolution);
v += -6.67001586407423e-7*texture(iChannel1,(x+to_float2(-4,-12))/iResolution);
v += -6.06828325544484e-6*texture(iChannel1,(x+to_float2(-4,-11))/iResolution);
v += -1.53331930050626e-5*texture(iChannel1,(x+to_float2(-4,-10))/iResolution);
v += -8.20512541395146e-5*texture(iChannel1,(x+to_float2(-4,-9))/iResolution);
v += -0.000183886848390102f*texture(iChannel1,(x+to_float2(-4,-8))/iResolution);
v += -0.000663241306028794f*texture(iChannel1,(x+to_float2(-4,-7))/iResolution);
v += -0.00130621888092719f*texture(iChannel1,(x+to_float2(-4,-6))/iResolution);
v += -0.00342230207024841f*texture(iChannel1,(x+to_float2(-4,-5))/iResolution);
v += -0.00581894547212869f*texture(iChannel1,(x+to_float2(-4,-4))/iResolution);
v += -0.0114720994824893f*texture(iChannel1,(x+to_float2(-4,-3))/iResolution);
v += -0.0161373519513290f*texture(iChannel1,(x+to_float2(-4,-2))/iResolution);
v += -0.0234868289771839f*texture(iChannel1,(x+to_float2(-4,-1))/iResolution);
v += -0.0243988493457437f*texture(iChannel1,(x+to_float2(-4,0))/iResolution);
v += -0.0234868289771839f*texture(iChannel1,(x+to_float2(-4,1))/iResolution);
v += -0.0161373519513290f*texture(iChannel1,(x+to_float2(-4,2))/iResolution);
v += -0.0114720994824893f*texture(iChannel1,(x+to_float2(-4,3))/iResolution);
v += -0.00581894547212869f*texture(iChannel1,(x+to_float2(-4,4))/iResolution);
v += -0.00342230207024841f*texture(iChannel1,(x+to_float2(-4,5))/iResolution);
v += -0.00130621888092719f*texture(iChannel1,(x+to_float2(-4,6))/iResolution);
v += -0.000663241306028794f*texture(iChannel1,(x+to_float2(-4,7))/iResolution);
v += -0.000183886848390102f*texture(iChannel1,(x+to_float2(-4,8))/iResolution);
v += -8.20512541395146e-5*texture(iChannel1,(x+to_float2(-4,9))/iResolution);
v += -1.53331930050626e-5*texture(iChannel1,(x+to_float2(-4,10))/iResolution);
v += -6.06828325544484e-6*texture(iChannel1,(x+to_float2(-4,11))/iResolution);
v += -6.67001586407423e-7*texture(iChannel1,(x+to_float2(-4,12))/iResolution);
v += -2.35570041695610e-7*texture(iChannel1,(x+to_float2(-4,13))/iResolution);
v += -1.11322151497006e-8*texture(iChannel1,(x+to_float2(-4,14))/iResolution);
v += -3.52520146407187e-9*texture(iChannel1,(x+to_float2(-4,15))/iResolution);
v += -8.81300366017967e-10*texture(iChannel1,(x+to_float2(-3,-16))/iResolution);
v += -2.96859070658684e-9*texture(iChannel1,(x+to_float2(-3,-15))/iResolution);
v += -7.68741301726550e-8*texture(iChannel1,(x+to_float2(-3,-14))/iResolution);
v += -2.32976162806153e-7*texture(iChannel1,(x+to_float2(-3,-13))/iResolution);
v += -2.50313678407110e-6*texture(iChannel1,(x+to_float2(-3,-12))/iResolution);
v += -6.80304947309196e-6*texture(iChannel1,(x+to_float2(-3,-11))/iResolution);
v += -4.19905081798788e-5*texture(iChannel1,(x+to_float2(-3,-10))/iResolution);
v += -0.000101948855444789f*texture(iChannel1,(x+to_float2(-3,-9))/iResolution);
v += -0.000417968447436579f*texture(iChannel1,(x+to_float2(-3,-8))/iResolution);
v += -0.000902096042409539f*texture(iChannel1,(x+to_float2(-3,-7))/iResolution);
v += -0.00267353867093334f*texture(iChannel1,(x+to_float2(-3,-6))/iResolution);
v += -0.00508711260044947f*texture(iChannel1,(x+to_float2(-3,-5))/iResolution);
v += -0.0114720994824893f*texture(iChannel1,(x+to_float2(-3,-4))/iResolution);
v += -0.0188449879060499f*texture(iChannel1,(x+to_float2(-3,-3))/iResolution);
v += -0.0327308975465712f*texture(iChannel1,(x+to_float2(-3,-2))/iResolution);
v += -0.0432285520946607f*texture(iChannel1,(x+to_float2(-3,-1))/iResolution);
v += -0.0526613406873366f*texture(iChannel1,(x+to_float2(-3,0))/iResolution);
v += -0.0432285520946607f*texture(iChannel1,(x+to_float2(-3,1))/iResolution);
v += -0.0327308975465712f*texture(iChannel1,(x+to_float2(-3,2))/iResolution);
v += -0.0188449879060499f*texture(iChannel1,(x+to_float2(-3,3))/iResolution);
v += -0.0114720994824893f*texture(iChannel1,(x+to_float2(-3,4))/iResolution);
v += -0.00508711260044947f*texture(iChannel1,(x+to_float2(-3,5))/iResolution);
v += -0.00267353867093334f*texture(iChannel1,(x+to_float2(-3,6))/iResolution);
v += -0.000902096042409539f*texture(iChannel1,(x+to_float2(-3,7))/iResolution);
v += -0.000417968447436579f*texture(iChannel1,(x+to_float2(-3,8))/iResolution);
v += -0.000101948855444789f*texture(iChannel1,(x+to_float2(-3,9))/iResolution);
v += -4.19905081798788e-5*texture(iChannel1,(x+to_float2(-3,10))/iResolution);
v += -6.80304947309196e-6*texture(iChannel1,(x+to_float2(-3,11))/iResolution);
v += -2.50313678407110e-6*texture(iChannel1,(x+to_float2(-3,12))/iResolution);
v += -2.32976162806153e-7*texture(iChannel1,(x+to_float2(-3,13))/iResolution);
v += -7.68741301726550e-8*texture(iChannel1,(x+to_float2(-3,14))/iResolution);
v += -2.96859070658684e-9*texture(iChannel1,(x+to_float2(-3,15))/iResolution);
v += -8.81300366017967e-10*texture(iChannel1,(x+to_float2(-3,16))/iResolution);
v += -1.55523594003171e-10*texture(iChannel1,(x+to_float2(-2,-17))/iResolution);
v += -5.56610757485032e-10*texture(iChannel1,(x+to_float2(-2,-16))/iResolution);
v += -1.87237674253993e-8*texture(iChannel1,(x+to_float2(-2,-15))/iResolution);
v += -6.04195520281792e-8*texture(iChannel1,(x+to_float2(-2,-14))/iResolution);
v += -7.95476807979867e-7*texture(iChannel1,(x+to_float2(-2,-13))/iResolution);
v += -2.30951991397887e-6*texture(iChannel1,(x+to_float2(-2,-12))/iResolution);
v += -1.68375663633924e-5*texture(iChannel1,(x+to_float2(-2,-11))/iResolution);
v += -4.38769347965717e-5*texture(iChannel1,(x+to_float2(-2,-10))/iResolution);
v += -0.000207377233891748f*texture(iChannel1,(x+to_float2(-2,-9))/iResolution);
v += -0.000483942043501884f*texture(iChannel1,(x+to_float2(-2,-8))/iResolution);
v += -0.00162991425895598f*texture(iChannel1,(x+to_float2(-2,-7))/iResolution);
v += -0.00339872715994716f*texture(iChannel1,(x+to_float2(-2,-6))/iResolution);
v += -0.00870143855718197f*texture(iChannel1,(x+to_float2(-2,-5))/iResolution);
v += -0.0161373519513290f*texture(iChannel1,(x+to_float2(-2,-4))/iResolution);
v += -0.0327308975465712f*texture(iChannel1,(x+to_float2(-2,-3))/iResolution);
v += -0.0527126982342452f*texture(iChannel1,(x+to_float2(-2,-2))/iResolution);
v += -0.0832781583994802f*texture(iChannel1,(x+to_float2(-2,-1))/iResolution);
v += -0.0997894092142815f*texture(iChannel1,(x+to_float2(-2,0))/iResolution);
v += -0.0832781583994802f*texture(iChannel1,(x+to_float2(-2,1))/iResolution);
v += -0.0527126982342452f*texture(iChannel1,(x+to_float2(-2,2))/iResolution);
v += -0.0327308975465712f*texture(iChannel1,(x+to_float2(-2,3))/iResolution);
v += -0.0161373519513290f*texture(iChannel1,(x+to_float2(-2,4))/iResolution);
v += -0.00870143855718197f*texture(iChannel1,(x+to_float2(-2,5))/iResolution);
v += -0.00339872715994716f*texture(iChannel1,(x+to_float2(-2,6))/iResolution);
v += -0.00162991425895598f*texture(iChannel1,(x+to_float2(-2,7))/iResolution);
v += -0.000483942043501884f*texture(iChannel1,(x+to_float2(-2,8))/iResolution);
v += -0.000207377233891748f*texture(iChannel1,(x+to_float2(-2,9))/iResolution);
v += -4.38769347965717e-5*texture(iChannel1,(x+to_float2(-2,10))/iResolution);
v += -1.68375663633924e-5*texture(iChannel1,(x+to_float2(-2,11))/iResolution);
v += -2.30951991397887e-6*texture(iChannel1,(x+to_float2(-2,12))/iResolution);
v += -7.95476807979867e-7*texture(iChannel1,(x+to_float2(-2,13))/iResolution);
v += -6.04195520281792e-8*texture(iChannel1,(x+to_float2(-2,14))/iResolution);
v += -1.87237674253993e-8*texture(iChannel1,(x+to_float2(-2,15))/iResolution);
v += -5.56610757485032e-10*texture(iChannel1,(x+to_float2(-2,16))/iResolution);
v += -1.55523594003171e-10*texture(iChannel1,(x+to_float2(-2,17))/iResolution);
v += -1.72803993336856e-11*texture(iChannel1,(x+to_float2(-1,-18))/iResolution);
v += -6.54836185276508e-11*texture(iChannel1,(x+to_float2(-1,-17))/iResolution);
v += -3.20233084494248e-9*texture(iChannel1,(x+to_float2(-1,-16))/iResolution);
v += -1.09503162093461e-8*texture(iChannel1,(x+to_float2(-1,-15))/iResolution);
v += -1.87838850251865e-7*texture(iChannel1,(x+to_float2(-1,-14))/iResolution);
v += -5.78991603106260e-7*texture(iChannel1,(x+to_float2(-1,-13))/iResolution);
v += -5.17681837663986e-6*texture(iChannel1,(x+to_float2(-1,-12))/iResolution);
v += -1.43607612699270e-5*texture(iChannel1,(x+to_float2(-1,-11))/iResolution);
v += -8.00984416855499e-5*texture(iChannel1,(x+to_float2(-1,-10))/iResolution);
v += -0.000199741101823747f*texture(iChannel1,(x+to_float2(-1,-9))/iResolution);
v += -0.000773602703702636f*texture(iChannel1,(x+to_float2(-1,-8))/iResolution);
v += -0.00173489178996533f*texture(iChannel1,(x+to_float2(-1,-7))/iResolution);
v += -0.00503071343700867f*texture(iChannel1,(x+to_float2(-1,-6))/iResolution);
v += -0.0101825625170022f*texture(iChannel1,(x+to_float2(-1,-5))/iResolution);
v += -0.0234868289771839f*texture(iChannel1,(x+to_float2(-1,-4))/iResolution);
v += -0.0432285520946607f*texture(iChannel1,(x+to_float2(-1,-3))/iResolution);
v += -0.0832781583994802f*texture(iChannel1,(x+to_float2(-1,-2))/iResolution);
v += -0.137381974054733f*texture(iChannel1,(x+to_float2(-1,-1))/iResolution);
v += -0.205597335680068f*texture(iChannel1,(x+to_float2(-1,0))/iResolution);
v += -0.137381974054733f*texture(iChannel1,(x+to_float2(-1,1))/iResolution);
v += -0.0832781583994802f*texture(iChannel1,(x+to_float2(-1,2))/iResolution);
v += -0.0432285520946607f*texture(iChannel1,(x+to_float2(-1,3))/iResolution);
v += -0.0234868289771839f*texture(iChannel1,(x+to_float2(-1,4))/iResolution);
v += -0.0101825625170022f*texture(iChannel1,(x+to_float2(-1,5))/iResolution);
v += -0.00503071343700867f*texture(iChannel1,(x+to_float2(-1,6))/iResolution);
v += -0.00173489178996533f*texture(iChannel1,(x+to_float2(-1,7))/iResolution);
v += -0.000773602703702636f*texture(iChannel1,(x+to_float2(-1,8))/iResolution);
v += -0.000199741101823747f*texture(iChannel1,(x+to_float2(-1,9))/iResolution);
v += -8.00984416855499e-5*texture(iChannel1,(x+to_float2(-1,10))/iResolution);
v += -1.43607612699270e-5*texture(iChannel1,(x+to_float2(-1,11))/iResolution);
v += -5.17681837663986e-6*texture(iChannel1,(x+to_float2(-1,12))/iResolution);
v += -5.78991603106260e-7*texture(iChannel1,(x+to_float2(-1,13))/iResolution);
v += -1.87838850251865e-7*texture(iChannel1,(x+to_float2(-1,14))/iResolution);
v += -1.09503162093461e-8*texture(iChannel1,(x+to_float2(-1,15))/iResolution);
v += -3.20233084494248e-9*texture(iChannel1,(x+to_float2(-1,16))/iResolution);
v += -6.54836185276508e-11*texture(iChannel1,(x+to_float2(-1,17))/iResolution);
v += -1.72803993336856e-11*texture(iChannel1,(x+to_float2(-1,18))/iResolution);
v += -9.09494701772928e-13*texture(iChannel1,(x+to_float2(0,-19))/iResolution);
v += -3.63797880709171e-12*texture(iChannel1,(x+to_float2(0,-18))/iResolution);
v += -3.42879502568394e-10*texture(iChannel1,(x+to_float2(0,-17))/iResolution);
v += -1.23691279441118e-9*texture(iChannel1,(x+to_float2(0,-16))/iResolution);
v += -3.10328687191941e-8*texture(iChannel1,(x+to_float2(0,-15))/iResolution);
v += -1.00993929663673e-7*texture(iChannel1,(x+to_float2(0,-14))/iResolution);
v += -1.17924446385587e-6*texture(iChannel1,(x+to_float2(0,-13))/iResolution);
v += -3.45800071954727e-6*texture(iChannel1,(x+to_float2(0,-12))/iResolution);
v += -2.36486230278388e-5*texture(iChannel1,(x+to_float2(0,-11))/iResolution);
v += -6.24149688519537e-5*texture(iChannel1,(x+to_float2(0,-10))/iResolution);
v += -0.000284433263004757f*texture(iChannel1,(x+to_float2(0,-9))/iResolution);
v += -0.000675835879519582f*texture(iChannel1,(x+to_float2(0,-8))/iResolution);
v += -0.00223807293514255f*texture(iChannel1,(x+to_float2(0,-7))/iResolution);
v += -0.00480667228111997f*texture(iChannel1,(x+to_float2(0,-6))/iResolution);
v += -0.0123926616652170f*texture(iChannel1,(x+to_float2(0,-5))/iResolution);
v += -0.0243988493457437f*texture(iChannel1,(x+to_float2(0,-4))/iResolution);
v += -0.0526613406873366f*texture(iChannel1,(x+to_float2(0,-3))/iResolution);
v += -0.0997894092142815f*texture(iChannel1,(x+to_float2(0,-2))/iResolution);
v += -0.205597335680068f*texture(iChannel1,(x+to_float2(0,-1))/iResolution);
v += -0.447835985396523f*texture(iChannel1,(x+to_float2(0,0))/iResolution);
v += -0.205597335680068f*texture(iChannel1,(x+to_float2(0,1))/iResolution);
v += -0.0997894092142815f*texture(iChannel1,(x+to_float2(0,2))/iResolution);
v += -0.0526613406873366f*texture(iChannel1,(x+to_float2(0,3))/iResolution);
v += -0.0243988493457437f*texture(iChannel1,(x+to_float2(0,4))/iResolution);
v += -0.0123926616652170f*texture(iChannel1,(x+to_float2(0,5))/iResolution);
v += -0.00480667228111997f*texture(iChannel1,(x+to_float2(0,6))/iResolution);
v += -0.00223807293514255f*texture(iChannel1,(x+to_float2(0,7))/iResolution);
v += -0.000675835879519582f*texture(iChannel1,(x+to_float2(0,8))/iResolution);
v += -0.000284433263004757f*texture(iChannel1,(x+to_float2(0,9))/iResolution);
v += -6.24149688519537e-5*texture(iChannel1,(x+to_float2(0,10))/iResolution);
v += -2.36486230278388e-5*texture(iChannel1,(x+to_float2(0,11))/iResolution);
v += -3.45800071954727e-6*texture(iChannel1,(x+to_float2(0,12))/iResolution);
v += -1.17924446385587e-6*texture(iChannel1,(x+to_float2(0,13))/iResolution);
v += -1.00993929663673e-7*texture(iChannel1,(x+to_float2(0,14))/iResolution);
v += -3.10328687191941e-8*texture(iChannel1,(x+to_float2(0,15))/iResolution);
v += -1.23691279441118e-9*texture(iChannel1,(x+to_float2(0,16))/iResolution);
v += -3.42879502568394e-10*texture(iChannel1,(x+to_float2(0,17))/iResolution);
v += -3.63797880709171e-12*texture(iChannel1,(x+to_float2(0,18))/iResolution);
v += -9.09494701772928e-13*texture(iChannel1,(x+to_float2(0,19))/iResolution);
v += -1.72803993336856e-11*texture(iChannel1,(x+to_float2(1,-18))/iResolution);
v += -6.54836185276508e-11*texture(iChannel1,(x+to_float2(1,-17))/iResolution);
v += -3.20233084494248e-9*texture(iChannel1,(x+to_float2(1,-16))/iResolution);
v += -1.09503162093461e-8*texture(iChannel1,(x+to_float2(1,-15))/iResolution);
v += -1.87838850251865e-7*texture(iChannel1,(x+to_float2(1,-14))/iResolution);
v += -5.78991603106260e-7*texture(iChannel1,(x+to_float2(1,-13))/iResolution);
v += -5.17681837663986e-6*texture(iChannel1,(x+to_float2(1,-12))/iResolution);
v += -1.43607612699270e-5*texture(iChannel1,(x+to_float2(1,-11))/iResolution);
v += -8.00984416855499e-5*texture(iChannel1,(x+to_float2(1,-10))/iResolution);
v += -0.000199741101823747f*texture(iChannel1,(x+to_float2(1,-9))/iResolution);
v += -0.000773602703702636f*texture(iChannel1,(x+to_float2(1,-8))/iResolution);
v += -0.00173489178996533f*texture(iChannel1,(x+to_float2(1,-7))/iResolution);
v += -0.00503071343700867f*texture(iChannel1,(x+to_float2(1,-6))/iResolution);
v += -0.0101825625170022f*texture(iChannel1,(x+to_float2(1,-5))/iResolution);
v += -0.0234868289771839f*texture(iChannel1,(x+to_float2(1,-4))/iResolution);
v += -0.0432285520946607f*texture(iChannel1,(x+to_float2(1,-3))/iResolution);
v += -0.0832781583994802f*texture(iChannel1,(x+to_float2(1,-2))/iResolution);
v += -0.137381974054733f*texture(iChannel1,(x+to_float2(1,-1))/iResolution);
v += -0.205597335680068f*texture(iChannel1,(x+to_float2(1,0))/iResolution);
v += -0.137381974054733f*texture(iChannel1,(x+to_float2(1,1))/iResolution);
v += -0.0832781583994802f*texture(iChannel1,(x+to_float2(1,2))/iResolution);
v += -0.0432285520946607f*texture(iChannel1,(x+to_float2(1,3))/iResolution);
v += -0.0234868289771839f*texture(iChannel1,(x+to_float2(1,4))/iResolution);
v += -0.0101825625170022f*texture(iChannel1,(x+to_float2(1,5))/iResolution);
v += -0.00503071343700867f*texture(iChannel1,(x+to_float2(1,6))/iResolution);
v += -0.00173489178996533f*texture(iChannel1,(x+to_float2(1,7))/iResolution);
v += -0.000773602703702636f*texture(iChannel1,(x+to_float2(1,8))/iResolution);
v += -0.000199741101823747f*texture(iChannel1,(x+to_float2(1,9))/iResolution);
v += -8.00984416855499e-5*texture(iChannel1,(x+to_float2(1,10))/iResolution);
v += -1.43607612699270e-5*texture(iChannel1,(x+to_float2(1,11))/iResolution);
v += -5.17681837663986e-6*texture(iChannel1,(x+to_float2(1,12))/iResolution);
v += -5.78991603106260e-7*texture(iChannel1,(x+to_float2(1,13))/iResolution);
v += -1.87838850251865e-7*texture(iChannel1,(x+to_float2(1,14))/iResolution);
v += -1.09503162093461e-8*texture(iChannel1,(x+to_float2(1,15))/iResolution);
v += -3.20233084494248e-9*texture(iChannel1,(x+to_float2(1,16))/iResolution);
v += -6.54836185276508e-11*texture(iChannel1,(x+to_float2(1,17))/iResolution);
v += -1.72803993336856e-11*texture(iChannel1,(x+to_float2(1,18))/iResolution);
v += -1.55523594003171e-10*texture(iChannel1,(x+to_float2(2,-17))/iResolution);
v += -5.56610757485032e-10*texture(iChannel1,(x+to_float2(2,-16))/iResolution);
v += -1.87237674253993e-8*texture(iChannel1,(x+to_float2(2,-15))/iResolution);
v += -6.04195520281792e-8*texture(iChannel1,(x+to_float2(2,-14))/iResolution);
v += -7.95476807979867e-7*texture(iChannel1,(x+to_float2(2,-13))/iResolution);
v += -2.30951991397887e-6*texture(iChannel1,(x+to_float2(2,-12))/iResolution);
v += -1.68375663633924e-5*texture(iChannel1,(x+to_float2(2,-11))/iResolution);
v += -4.38769347965717e-5*texture(iChannel1,(x+to_float2(2,-10))/iResolution);
v += -0.000207377233891748f*texture(iChannel1,(x+to_float2(2,-9))/iResolution);
v += -0.000483942043501884f*texture(iChannel1,(x+to_float2(2,-8))/iResolution);
v += -0.00162991425895598f*texture(iChannel1,(x+to_float2(2,-7))/iResolution);
v += -0.00339872715994716f*texture(iChannel1,(x+to_float2(2,-6))/iResolution);
v += -0.00870143855718197f*texture(iChannel1,(x+to_float2(2,-5))/iResolution);
v += -0.0161373519513290f*texture(iChannel1,(x+to_float2(2,-4))/iResolution);
v += -0.0327308975465712f*texture(iChannel1,(x+to_float2(2,-3))/iResolution);
v += -0.0527126982342452f*texture(iChannel1,(x+to_float2(2,-2))/iResolution);
v += -0.0832781583994802f*texture(iChannel1,(x+to_float2(2,-1))/iResolution);
v += -0.0997894092142815f*texture(iChannel1,(x+to_float2(2,0))/iResolution);
v += -0.0832781583994802f*texture(iChannel1,(x+to_float2(2,1))/iResolution);
v += -0.0527126982342452f*texture(iChannel1,(x+to_float2(2,2))/iResolution);
v += -0.0327308975465712f*texture(iChannel1,(x+to_float2(2,3))/iResolution);
v += -0.0161373519513290f*texture(iChannel1,(x+to_float2(2,4))/iResolution);
v += -0.00870143855718197f*texture(iChannel1,(x+to_float2(2,5))/iResolution);
v += -0.00339872715994716f*texture(iChannel1,(x+to_float2(2,6))/iResolution);
v += -0.00162991425895598f*texture(iChannel1,(x+to_float2(2,7))/iResolution);
v += -0.000483942043501884f*texture(iChannel1,(x+to_float2(2,8))/iResolution);
v += -0.000207377233891748f*texture(iChannel1,(x+to_float2(2,9))/iResolution);
v += -4.38769347965717e-5*texture(iChannel1,(x+to_float2(2,10))/iResolution);
v += -1.68375663633924e-5*texture(iChannel1,(x+to_float2(2,11))/iResolution);
v += -2.30951991397887e-6*texture(iChannel1,(x+to_float2(2,12))/iResolution);
v += -7.95476807979867e-7*texture(iChannel1,(x+to_float2(2,13))/iResolution);
v += -6.04195520281792e-8*texture(iChannel1,(x+to_float2(2,14))/iResolution);
v += -1.87237674253993e-8*texture(iChannel1,(x+to_float2(2,15))/iResolution);
v += -5.56610757485032e-10*texture(iChannel1,(x+to_float2(2,16))/iResolution);
v += -1.55523594003171e-10*texture(iChannel1,(x+to_float2(2,17))/iResolution);
v += -8.81300366017967e-10*texture(iChannel1,(x+to_float2(3,-16))/iResolution);
v += -2.96859070658684e-9*texture(iChannel1,(x+to_float2(3,-15))/iResolution);
v += -7.68741301726550e-8*texture(iChannel1,(x+to_float2(3,-14))/iResolution);
v += -2.32976162806153e-7*texture(iChannel1,(x+to_float2(3,-13))/iResolution);
v += -2.50313678407110e-6*texture(iChannel1,(x+to_float2(3,-12))/iResolution);
v += -6.80304947309196e-6*texture(iChannel1,(x+to_float2(3,-11))/iResolution);
v += -4.19905081798788e-5*texture(iChannel1,(x+to_float2(3,-10))/iResolution);
v += -0.000101948855444789f*texture(iChannel1,(x+to_float2(3,-9))/iResolution);
v += -0.000417968447436579f*texture(iChannel1,(x+to_float2(3,-8))/iResolution);
v += -0.000902096042409539f*texture(iChannel1,(x+to_float2(3,-7))/iResolution);
v += -0.00267353867093334f*texture(iChannel1,(x+to_float2(3,-6))/iResolution);
v += -0.00508711260044947f*texture(iChannel1,(x+to_float2(3,-5))/iResolution);
v += -0.0114720994824893f*texture(iChannel1,(x+to_float2(3,-4))/iResolution);
v += -0.0188449879060499f*texture(iChannel1,(x+to_float2(3,-3))/iResolution);
v += -0.0327308975465712f*texture(iChannel1,(x+to_float2(3,-2))/iResolution);
v += -0.0432285520946607f*texture(iChannel1,(x+to_float2(3,-1))/iResolution);
v += -0.0526613406873366f*texture(iChannel1,(x+to_float2(3,0))/iResolution);
v += -0.0432285520946607f*texture(iChannel1,(x+to_float2(3,1))/iResolution);
v += -0.0327308975465712f*texture(iChannel1,(x+to_float2(3,2))/iResolution);
v += -0.0188449879060499f*texture(iChannel1,(x+to_float2(3,3))/iResolution);
v += -0.0114720994824893f*texture(iChannel1,(x+to_float2(3,4))/iResolution);
v += -0.00508711260044947f*texture(iChannel1,(x+to_float2(3,5))/iResolution);
v += -0.00267353867093334f*texture(iChannel1,(x+to_float2(3,6))/iResolution);
v += -0.000902096042409539f*texture(iChannel1,(x+to_float2(3,7))/iResolution);
v += -0.000417968447436579f*texture(iChannel1,(x+to_float2(3,8))/iResolution);
v += -0.000101948855444789f*texture(iChannel1,(x+to_float2(3,9))/iResolution);
v += -4.19905081798788e-5*texture(iChannel1,(x+to_float2(3,10))/iResolution);
v += -6.80304947309196e-6*texture(iChannel1,(x+to_float2(3,11))/iResolution);
v += -2.50313678407110e-6*texture(iChannel1,(x+to_float2(3,12))/iResolution);
v += -2.32976162806153e-7*texture(iChannel1,(x+to_float2(3,13))/iResolution);
v += -7.68741301726550e-8*texture(iChannel1,(x+to_float2(3,14))/iResolution);
v += -2.96859070658684e-9*texture(iChannel1,(x+to_float2(3,15))/iResolution);
v += -8.81300366017967e-10*texture(iChannel1,(x+to_float2(3,16))/iResolution);
v += -3.52520146407187e-9*texture(iChannel1,(x+to_float2(4,-15))/iResolution);
v += -1.11322151497006e-8*texture(iChannel1,(x+to_float2(4,-14))/iResolution);
v += -2.35570041695610e-7*texture(iChannel1,(x+to_float2(4,-13))/iResolution);
v += -6.67001586407423e-7*texture(iChannel1,(x+to_float2(4,-12))/iResolution);
v += -6.06828325544484e-6*texture(iChannel1,(x+to_float2(4,-11))/iResolution);
v += -1.53331930050626e-5*texture(iChannel1,(x+to_float2(4,-10))/iResolution);
v += -8.20512541395146e-5*texture(iChannel1,(x+to_float2(4,-9))/iResolution);
v += -0.000183886848390102f*texture(iChannel1,(x+to_float2(4,-8))/iResolution);
v += -0.000663241306028794f*texture(iChannel1,(x+to_float2(4,-7))/iResolution);
v += -0.00130621888092719f*texture(iChannel1,(x+to_float2(4,-6))/iResolution);
v += -0.00342230207024841f*texture(iChannel1,(x+to_float2(4,-5))/iResolution);
v += -0.00581894547212869f*texture(iChannel1,(x+to_float2(4,-4))/iResolution);
v += -0.0114720994824893f*texture(iChannel1,(x+to_float2(4,-3))/iResolution);
v += -0.0161373519513290f*texture(iChannel1,(x+to_float2(4,-2))/iResolution);
v += -0.0234868289771839f*texture(iChannel1,(x+to_float2(4,-1))/iResolution);
v += -0.0243988493457437f*texture(iChannel1,(x+to_float2(4,0))/iResolution);
v += -0.0234868289771839f*texture(iChannel1,(x+to_float2(4,1))/iResolution);
v += -0.0161373519513290f*texture(iChannel1,(x+to_float2(4,2))/iResolution);
v += -0.0114720994824893f*texture(iChannel1,(x+to_float2(4,3))/iResolution);
v += -0.00581894547212869f*texture(iChannel1,(x+to_float2(4,4))/iResolution);
v += -0.00342230207024841f*texture(iChannel1,(x+to_float2(4,5))/iResolution);
v += -0.00130621888092719f*texture(iChannel1,(x+to_float2(4,6))/iResolution);
v += -0.000663241306028794f*texture(iChannel1,(x+to_float2(4,7))/iResolution);
v += -0.000183886848390102f*texture(iChannel1,(x+to_float2(4,8))/iResolution);
v += -8.20512541395146e-5*texture(iChannel1,(x+to_float2(4,9))/iResolution);
v += -1.53331930050626e-5*texture(iChannel1,(x+to_float2(4,10))/iResolution);
v += -6.06828325544484e-6*texture(iChannel1,(x+to_float2(4,11))/iResolution);
v += -6.67001586407423e-7*texture(iChannel1,(x+to_float2(4,12))/iResolution);
v += -2.35570041695610e-7*texture(iChannel1,(x+to_float2(4,13))/iResolution);
v += -1.11322151497006e-8*texture(iChannel1,(x+to_float2(4,14))/iResolution);
v += -3.52520146407187e-9*texture(iChannel1,(x+to_float2(4,15))/iResolution);
v += -1.05756043922156e-8*texture(iChannel1,(x+to_float2(5,-14))/iResolution);
v += -3.11702024191618e-8*texture(iChannel1,(x+to_float2(5,-13))/iResolution);
v += -5.58899046154693e-7*texture(iChannel1,(x+to_float2(5,-12))/iResolution);
v += -1.46988895721734e-6*texture(iChannel1,(x+to_float2(5,-11))/iResolution);
v += -1.15973198262509e-5*texture(iChannel1,(x+to_float2(5,-10))/iResolution);
v += -2.70361197181046e-5*texture(iChannel1,(x+to_float2(5,-9))/iResolution);
v += -0.000127373421491939f*texture(iChannel1,(x+to_float2(5,-8))/iResolution);
v += -0.000260763452388346f*texture(iChannel1,(x+to_float2(5,-7))/iResolution);
v += -0.000834164828120265f*texture(iChannel1,(x+to_float2(5,-6))/iResolution);
v += -0.00147693132748827f*texture(iChannel1,(x+to_float2(5,-5))/iResolution);
v += -0.00342230207024841f*texture(iChannel1,(x+to_float2(5,-4))/iResolution);
v += -0.00508711260044947f*texture(iChannel1,(x+to_float2(5,-3))/iResolution);
v += -0.00870143855718197f*texture(iChannel1,(x+to_float2(5,-2))/iResolution);
v += -0.0101825625170022f*texture(iChannel1,(x+to_float2(5,-1))/iResolution);
v += -0.0123926616652170f*texture(iChannel1,(x+to_float2(5,0))/iResolution);
v += -0.0101825625170022f*texture(iChannel1,(x+to_float2(5,1))/iResolution);
v += -0.00870143855718197f*texture(iChannel1,(x+to_float2(5,2))/iResolution);
v += -0.00508711260044947f*texture(iChannel1,(x+to_float2(5,3))/iResolution);
v += -0.00342230207024841f*texture(iChannel1,(x+to_float2(5,4))/iResolution);
v += -0.00147693132748827f*texture(iChannel1,(x+to_float2(5,5))/iResolution);
v += -0.000834164828120265f*texture(iChannel1,(x+to_float2(5,6))/iResolution);
v += -0.000260763452388346f*texture(iChannel1,(x+to_float2(5,7))/iResolution);
v += -0.000127373421491939f*texture(iChannel1,(x+to_float2(5,8))/iResolution);
v += -2.70361197181046e-5*texture(iChannel1,(x+to_float2(5,9))/iResolution);
v += -1.15973198262509e-5*texture(iChannel1,(x+to_float2(5,10))/iResolution);
v += -1.46988895721734e-6*texture(iChannel1,(x+to_float2(5,11))/iResolution);
v += -5.58899046154693e-7*texture(iChannel1,(x+to_float2(5,12))/iResolution);
v += -3.11702024191618e-8*texture(iChannel1,(x+to_float2(5,13))/iResolution);
v += -1.05756043922156e-8*texture(iChannel1,(x+to_float2(5,14))/iResolution);
v += -2.46764102485031e-8*texture(iChannel1,(x+to_float2(6,-13))/iResolution);
v += -6.75354385748506e-8*texture(iChannel1,(x+to_float2(6,-12))/iResolution);
v += -1.05081926449202e-6*texture(iChannel1,(x+to_float2(6,-11))/iResolution);
v += -2.55007762461901e-6*texture(iChannel1,(x+to_float2(6,-10))/iResolution);
v += -1.77311976585770e-5*texture(iChannel1,(x+to_float2(6,-9))/iResolution);
v += -3.78072654712014e-5*texture(iChannel1,(x+to_float2(6,-8))/iResolution);
v += -0.000158390301294276f*texture(iChannel1,(x+to_float2(6,-7))/iResolution);
v += -0.000292745651677251f*texture(iChannel1,(x+to_float2(6,-6))/iResolution);
v += -0.000834164828120265f*texture(iChannel1,(x+to_float2(6,-5))/iResolution);
v += -0.00130621888092719f*texture(iChannel1,(x+to_float2(6,-4))/iResolution);
v += -0.00267353867093334f*texture(iChannel1,(x+to_float2(6,-3))/iResolution);
v += -0.00339872715994716f*texture(iChannel1,(x+to_float2(6,-2))/iResolution);
v += -0.00503071343700867f*texture(iChannel1,(x+to_float2(6,-1))/iResolution);
v += -0.00480667228111997f*texture(iChannel1,(x+to_float2(6,0))/iResolution);
v += -0.00503071343700867f*texture(iChannel1,(x+to_float2(6,1))/iResolution);
v += -0.00339872715994716f*texture(iChannel1,(x+to_float2(6,2))/iResolution);
v += -0.00267353867093334f*texture(iChannel1,(x+to_float2(6,3))/iResolution);
v += -0.00130621888092719f*texture(iChannel1,(x+to_float2(6,4))/iResolution);
v += -0.000834164828120265f*texture(iChannel1,(x+to_float2(6,5))/iResolution);
v += -0.000292745651677251f*texture(iChannel1,(x+to_float2(6,6))/iResolution);
v += -0.000158390301294276f*texture(iChannel1,(x+to_float2(6,7))/iResolution);
v += -3.78072654712014e-5*texture(iChannel1,(x+to_float2(6,8))/iResolution);
v += -1.77311976585770e-5*texture(iChannel1,(x+to_float2(6,9))/iResolution);
v += -2.55007762461901e-6*texture(iChannel1,(x+to_float2(6,10))/iResolution);
v += -1.05081926449202e-6*texture(iChannel1,(x+to_float2(6,11))/iResolution);
v += -6.75354385748506e-8*texture(iChannel1,(x+to_float2(6,12))/iResolution);
v += -2.46764102485031e-8*texture(iChannel1,(x+to_float2(6,13))/iResolution);
v += -4.58276190329343e-8*texture(iChannel1,(x+to_float2(7,-12))/iResolution);
v += -1.15775037556887e-7*texture(iChannel1,(x+to_float2(7,-11))/iResolution);
v += -1.58909278979991e-6*texture(iChannel1,(x+to_float2(7,-10))/iResolution);
v += -3.53132782038301e-6*texture(iChannel1,(x+to_float2(7,-9))/iResolution);
v += -2.18790937651647e-5*texture(iChannel1,(x+to_float2(7,-8))/iResolution);
v += -4.22448356403038e-5*texture(iChannel1,(x+to_float2(7,-7))/iResolution);
v += -0.000158390301294276f*texture(iChannel1,(x+to_float2(7,-6))/iResolution);
v += -0.000260763452388346f*texture(iChannel1,(x+to_float2(7,-5))/iResolution);
v += -0.000663241306028794f*texture(iChannel1,(x+to_float2(7,-4))/iResolution);
v += -0.000902096042409539f*texture(iChannel1,(x+to_float2(7,-3))/iResolution);
v += -0.00162991425895598f*texture(iChannel1,(x+to_float2(7,-2))/iResolution);
v += -0.00173489178996533f*texture(iChannel1,(x+to_float2(7,-1))/iResolution);
v += -0.00223807293514255f*texture(iChannel1,(x+to_float2(7,0))/iResolution);
v += -0.00173489178996533f*texture(iChannel1,(x+to_float2(7,1))/iResolution);
v += -0.00162991425895598f*texture(iChannel1,(x+to_float2(7,2))/iResolution);
v += -0.000902096042409539f*texture(iChannel1,(x+to_float2(7,3))/iResolution);
v += -0.000663241306028794f*texture(iChannel1,(x+to_float2(7,4))/iResolution);
v += -0.000260763452388346f*texture(iChannel1,(x+to_float2(7,5))/iResolution);
v += -0.000158390301294276f*texture(iChannel1,(x+to_float2(7,6))/iResolution);
v += -4.22448356403038e-5*texture(iChannel1,(x+to_float2(7,7))/iResolution);
v += -2.18790937651647e-5*texture(iChannel1,(x+to_float2(7,8))/iResolution);
v += -3.53132782038301e-6*texture(iChannel1,(x+to_float2(7,9))/iResolution);
v += -1.58909278979991e-6*texture(iChannel1,(x+to_float2(7,10))/iResolution);
v += -1.15775037556887e-7*texture(iChannel1,(x+to_float2(7,11))/iResolution);
v += -4.58276190329343e-8*texture(iChannel1,(x+to_float2(7,12))/iResolution);
v += -6.87414285494015e-8*texture(iChannel1,(x+to_float2(8,-11))/iResolution);
v += -1.59190676640719e-7*texture(iChannel1,(x+to_float2(8,-10))/iResolution);
v += -1.95008578884881e-6*texture(iChannel1,(x+to_float2(8,-9))/iResolution);
v += -3.93294612877071e-6*texture(iChannel1,(x+to_float2(8,-8))/iResolution);
v += -2.18790937651647e-5*texture(iChannel1,(x+to_float2(8,-7))/iResolution);
v += -3.78072654712014e-5*texture(iChannel1,(x+to_float2(8,-6))/iResolution);
v += -0.000127373421491939f*texture(iChannel1,(x+to_float2(8,-5))/iResolution);
v += -0.000183886848390102f*texture(iChannel1,(x+to_float2(8,-4))/iResolution);
v += -0.000417968447436579f*texture(iChannel1,(x+to_float2(8,-3))/iResolution);
v += -0.000483942043501884f*texture(iChannel1,(x+to_float2(8,-2))/iResolution);
v += -0.000773602703702636f*texture(iChannel1,(x+to_float2(8,-1))/iResolution);
v += -0.000675835879519582f*texture(iChannel1,(x+to_float2(8,0))/iResolution);
v += -0.000773602703702636f*texture(iChannel1,(x+to_float2(8,1))/iResolution);
v += -0.000483942043501884f*texture(iChannel1,(x+to_float2(8,2))/iResolution);
v += -0.000417968447436579f*texture(iChannel1,(x+to_float2(8,3))/iResolution);
v += -0.000183886848390102f*texture(iChannel1,(x+to_float2(8,4))/iResolution);
v += -0.000127373421491939f*texture(iChannel1,(x+to_float2(8,5))/iResolution);
v += -3.78072654712014e-5*texture(iChannel1,(x+to_float2(8,6))/iResolution);
v += -2.18790937651647e-5*texture(iChannel1,(x+to_float2(8,7))/iResolution);
v += -3.93294612877071e-6*texture(iChannel1,(x+to_float2(8,8))/iResolution);
v += -1.95008578884881e-6*texture(iChannel1,(x+to_float2(8,9))/iResolution);
v += -1.59190676640719e-7*texture(iChannel1,(x+to_float2(8,10))/iResolution);
v += -6.87414285494015e-8*texture(iChannel1,(x+to_float2(8,11))/iResolution);
v += -8.40173015603796e-8*texture(iChannel1,(x+to_float2(9,-10))/iResolution);
v += -1.76878529600799e-7*texture(iChannel1,(x+to_float2(9,-9))/iResolution);
v += -1.95008578884881e-6*texture(iChannel1,(x+to_float2(9,-8))/iResolution);
v += -3.53132782038301e-6*texture(iChannel1,(x+to_float2(9,-7))/iResolution);
v += -1.77311976585770e-5*texture(iChannel1,(x+to_float2(9,-6))/iResolution);
v += -2.70361197181046e-5*texture(iChannel1,(x+to_float2(9,-5))/iResolution);
v += -8.20512541395146e-5*texture(iChannel1,(x+to_float2(9,-4))/iResolution);
v += -0.000101948855444789f*texture(iChannel1,(x+to_float2(9,-3))/iResolution);
v += -0.000207377233891748f*texture(iChannel1,(x+to_float2(9,-2))/iResolution);
v += -0.000199741101823747f*texture(iChannel1,(x+to_float2(9,-1))/iResolution);
v += -0.000284433263004757f*texture(iChannel1,(x+to_float2(9,0))/iResolution);
v += -0.000199741101823747f*texture(iChannel1,(x+to_float2(9,1))/iResolution);
v += -0.000207377233891748f*texture(iChannel1,(x+to_float2(9,2))/iResolution);
v += -0.000101948855444789f*texture(iChannel1,(x+to_float2(9,3))/iResolution);
v += -8.20512541395146e-5*texture(iChannel1,(x+to_float2(9,4))/iResolution);
v += -2.70361197181046e-5*texture(iChannel1,(x+to_float2(9,5))/iResolution);
v += -1.77311976585770e-5*texture(iChannel1,(x+to_float2(9,6))/iResolution);
v += -3.53132782038301e-6*texture(iChannel1,(x+to_float2(9,7))/iResolution);
v += -1.95008578884881e-6*texture(iChannel1,(x+to_float2(9,8))/iResolution);
v += -1.76878529600799e-7*texture(iChannel1,(x+to_float2(9,9))/iResolution);
v += -8.40173015603796e-8*texture(iChannel1,(x+to_float2(9,10))/iResolution);
v += -8.40173015603796e-8*texture(iChannel1,(x+to_float2(10,-9))/iResolution);
v += -1.59190676640719e-7*texture(iChannel1,(x+to_float2(10,-8))/iResolution);
v += -1.58909278979991e-6*texture(iChannel1,(x+to_float2(10,-7))/iResolution);
v += -2.55007762461901e-6*texture(iChannel1,(x+to_float2(10,-6))/iResolution);
v += -1.15973198262509e-5*texture(iChannel1,(x+to_float2(10,-5))/iResolution);
v += -1.53331930050626e-5*texture(iChannel1,(x+to_float2(10,-4))/iResolution);
v += -4.19905081798788e-5*texture(iChannel1,(x+to_float2(10,-3))/iResolution);
v += -4.38769347965717e-5*texture(iChannel1,(x+to_float2(10,-2))/iResolution);
v += -8.00984416855499e-5*texture(iChannel1,(x+to_float2(10,-1))/iResolution);
v += -6.24149688519537e-5*texture(iChannel1,(x+to_float2(10,0))/iResolution);
v += -8.00984416855499e-5*texture(iChannel1,(x+to_float2(10,1))/iResolution);
v += -4.38769347965717e-5*texture(iChannel1,(x+to_float2(10,2))/iResolution);
v += -4.19905081798788e-5*texture(iChannel1,(x+to_float2(10,3))/iResolution);
v += -1.53331930050626e-5*texture(iChannel1,(x+to_float2(10,4))/iResolution);
v += -1.15973198262509e-5*texture(iChannel1,(x+to_float2(10,5))/iResolution);
v += -2.55007762461901e-6*texture(iChannel1,(x+to_float2(10,6))/iResolution);
v += -1.58909278979991e-6*texture(iChannel1,(x+to_float2(10,7))/iResolution);
v += -1.59190676640719e-7*texture(iChannel1,(x+to_float2(10,8))/iResolution);
v += -8.40173015603796e-8*texture(iChannel1,(x+to_float2(10,9))/iResolution);
v += -6.87414285494015e-8*texture(iChannel1,(x+to_float2(11,-8))/iResolution);
v += -1.15775037556887e-7*texture(iChannel1,(x+to_float2(11,-7))/iResolution);
v += -1.05081926449202e-6*texture(iChannel1,(x+to_float2(11,-6))/iResolution);
v += -1.46988895721734e-6*texture(iChannel1,(x+to_float2(11,-5))/iResolution);
v += -6.06828325544484e-6*texture(iChannel1,(x+to_float2(11,-4))/iResolution);
v += -6.80304947309196e-6*texture(iChannel1,(x+to_float2(11,-3))/iResolution);
v += -1.68375663633924e-5*texture(iChannel1,(x+to_float2(11,-2))/iResolution);
v += -1.43607612699270e-5*texture(iChannel1,(x+to_float2(11,-1))/iResolution);
v += -2.36486230278388e-5*texture(iChannel1,(x+to_float2(11,0))/iResolution);
v += -1.43607612699270e-5*texture(iChannel1,(x+to_float2(11,1))/iResolution);
v += -1.68375663633924e-5*texture(iChannel1,(x+to_float2(11,2))/iResolution);
v += -6.80304947309196e-6*texture(iChannel1,(x+to_float2(11,3))/iResolution);
v += -6.06828325544484e-6*texture(iChannel1,(x+to_float2(11,4))/iResolution);
v += -1.46988895721734e-6*texture(iChannel1,(x+to_float2(11,5))/iResolution);
v += -1.05081926449202e-6*texture(iChannel1,(x+to_float2(11,6))/iResolution);
v += -1.15775037556887e-7*texture(iChannel1,(x+to_float2(11,7))/iResolution);
v += -6.87414285494015e-8*texture(iChannel1,(x+to_float2(11,8))/iResolution);
v += -4.58276190329343e-8*texture(iChannel1,(x+to_float2(12,-7))/iResolution);
v += -6.75354385748506e-8*texture(iChannel1,(x+to_float2(12,-6))/iResolution);
v += -5.58899046154693e-7*texture(iChannel1,(x+to_float2(12,-5))/iResolution);
v += -6.67001586407423e-7*texture(iChannel1,(x+to_float2(12,-4))/iResolution);
v += -2.50313678407110e-6*texture(iChannel1,(x+to_float2(12,-3))/iResolution);
v += -2.30951991397887e-6*texture(iChannel1,(x+to_float2(12,-2))/iResolution);
v += -5.17681837663986e-6*texture(iChannel1,(x+to_float2(12,-1))/iResolution);
v += -3.45800071954727e-6*texture(iChannel1,(x+to_float2(12,0))/iResolution);
v += -5.17681837663986e-6*texture(iChannel1,(x+to_float2(12,1))/iResolution);
v += -2.30951991397887e-6*texture(iChannel1,(x+to_float2(12,2))/iResolution);
v += -2.50313678407110e-6*texture(iChannel1,(x+to_float2(12,3))/iResolution);
v += -6.67001586407423e-7*texture(iChannel1,(x+to_float2(12,4))/iResolution);
v += -5.58899046154693e-7*texture(iChannel1,(x+to_float2(12,5))/iResolution);
v += -6.75354385748506e-8*texture(iChannel1,(x+to_float2(12,6))/iResolution);
v += -4.58276190329343e-8*texture(iChannel1,(x+to_float2(12,7))/iResolution);
v += -2.46764102485031e-8*texture(iChannel1,(x+to_float2(13,-6))/iResolution);
v += -3.11702024191618e-8*texture(iChannel1,(x+to_float2(13,-5))/iResolution);
v += -2.35570041695610e-7*texture(iChannel1,(x+to_float2(13,-4))/iResolution);
v += -2.32976162806153e-7*texture(iChannel1,(x+to_float2(13,-3))/iResolution);
v += -7.95476807979867e-7*texture(iChannel1,(x+to_float2(13,-2))/iResolution);
v += -5.78991603106260e-7*texture(iChannel1,(x+to_float2(13,-1))/iResolution);
v += -1.17924446385587e-6*texture(iChannel1,(x+to_float2(13,0))/iResolution);
v += -5.78991603106260e-7*texture(iChannel1,(x+to_float2(13,1))/iResolution);
v += -7.95476807979867e-7*texture(iChannel1,(x+to_float2(13,2))/iResolution);
v += -2.32976162806153e-7*texture(iChannel1,(x+to_float2(13,3))/iResolution);
v += -2.35570041695610e-7*texture(iChannel1,(x+to_float2(13,4))/iResolution);
v += -3.11702024191618e-8*texture(iChannel1,(x+to_float2(13,5))/iResolution);
v += -2.46764102485031e-8*texture(iChannel1,(x+to_float2(13,6))/iResolution);
v += -1.05756043922156e-8*texture(iChannel1,(x+to_float2(14,-5))/iResolution);
v += -1.11322151497006e-8*texture(iChannel1,(x+to_float2(14,-4))/iResolution);
v += -7.68741301726550e-8*texture(iChannel1,(x+to_float2(14,-3))/iResolution);
v += -6.04195520281792e-8*texture(iChannel1,(x+to_float2(14,-2))/iResolution);
v += -1.87838850251865e-7*texture(iChannel1,(x+to_float2(14,-1))/iResolution);
v += -1.00993929663673e-7*texture(iChannel1,(x+to_float2(14,0))/iResolution);
v += -1.87838850251865e-7*texture(iChannel1,(x+to_float2(14,1))/iResolution);
v += -6.04195520281792e-8*texture(iChannel1,(x+to_float2(14,2))/iResolution);
v += -7.68741301726550e-8*texture(iChannel1,(x+to_float2(14,3))/iResolution);
v += -1.11322151497006e-8*texture(iChannel1,(x+to_float2(14,4))/iResolution);
v += -1.05756043922156e-8*texture(iChannel1,(x+to_float2(14,5))/iResolution);
v += -3.52520146407187e-9*texture(iChannel1,(x+to_float2(15,-4))/iResolution);
v += -2.96859070658684e-9*texture(iChannel1,(x+to_float2(15,-3))/iResolution);
v += -1.87237674253993e-8*texture(iChannel1,(x+to_float2(15,-2))/iResolution);
v += -1.09503162093461e-8*texture(iChannel1,(x+to_float2(15,-1))/iResolution);
v += -3.10328687191941e-8*texture(iChannel1,(x+to_float2(15,0))/iResolution);
v += -1.09503162093461e-8*texture(iChannel1,(x+to_float2(15,1))/iResolution);
v += -1.87237674253993e-8*texture(iChannel1,(x+to_float2(15,2))/iResolution);
v += -2.96859070658684e-9*texture(iChannel1,(x+to_float2(15,3))/iResolution);
v += -3.52520146407187e-9*texture(iChannel1,(x+to_float2(15,4))/iResolution);
v += -8.81300366017967e-10*texture(iChannel1,(x+to_float2(16,-3))/iResolution);
v += -5.56610757485032e-10*texture(iChannel1,(x+to_float2(16,-2))/iResolution);
v += -3.20233084494248e-9*texture(iChannel1,(x+to_float2(16,-1))/iResolution);
v += -1.23691279441118e-9*texture(iChannel1,(x+to_float2(16,0))/iResolution);
v += -3.20233084494248e-9*texture(iChannel1,(x+to_float2(16,1))/iResolution);
v += -5.56610757485032e-10*texture(iChannel1,(x+to_float2(16,2))/iResolution);
v += -8.81300366017967e-10*texture(iChannel1,(x+to_float2(16,3))/iResolution);
v += -1.55523594003171e-10*texture(iChannel1,(x+to_float2(17,-2))/iResolution);
v += -6.54836185276508e-11*texture(iChannel1,(x+to_float2(17,-1))/iResolution);
v += -3.42879502568394e-10*texture(iChannel1,(x+to_float2(17,0))/iResolution);
v += -6.54836185276508e-11*texture(iChannel1,(x+to_float2(17,1))/iResolution);
v += -1.55523594003171e-10*texture(iChannel1,(x+to_float2(17,2))/iResolution);
v += -1.72803993336856e-11*texture(iChannel1,(x+to_float2(18,-1))/iResolution);
v += -3.63797880709171e-12*texture(iChannel1,(x+to_float2(18,0))/iResolution);
v += -1.72803993336856e-11*texture(iChannel1,(x+to_float2(18,1))/iResolution);
v += -9.09494701772928e-13*texture(iChannel1,(x+to_float2(19,0))/iResolution);

    return v.x;
}

__KERNEL__ void FluidatheartJipiFuse__Buffer_C(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX2(Poison, 0);
    CONNECT_SLIDER7(PoisonLevel, -500.0f, 500.0f, 0.0f);
    
    fragCoord+=0.5f;
 
    float p = JacobiSolveForPressureXPart(fragCoord, iResolution, iChannel0) +  JacobiSolveForPressureBPart(fragCoord, iResolution, iChannel1);
    fragColor = to_float4(p,0,0,1);

    if(Poison) fragColor=to_float4(PoisonLevel,0,0,1);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel1
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


// BUFFER D: subtract the gradient of the pressure from the advected velocity field to obtain the divergence free field// BUFFER C: solve the Poisson equation and output the pressure field 

// Gradient of the pressure field
__DEVICE__ float2 Gradient(float2 x, float2 iResolution, __TEXTURE2D__ iChannel0)
{
    float xL = texture(iChannel0, (x + to_float2(1, 0))/iResolution).x;
    float xR = texture(iChannel0, (x - to_float2(1, 0))/iResolution).x;
    float xB = texture(iChannel0, (x + to_float2(0, 1))/iResolution).x;
    float xT = texture(iChannel0, (x - to_float2(0, 1))/iResolution).x;
    return to_float2(xR - xL, xT - xB);
}

__KERNEL__ void FluidatheartJipiFuse__Buffer_D(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
    fragCoord+=0.5f;

    // Subtract the pressure gradient to the velocity field to obtain the divergence free field
    float4 u = texture(iChannel1, fragCoord/iResolution);
    fragColor = to_float4_f2f2(swi2(u,x,y) - Gradient(fragCoord, iResolution, iChannel0), swi2(u,z,w));

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'Previsualization: Buffer D' to iChannel0


// First experiment with fluid simulation ^_^
//
// This fluid simulation implements the method described in: https://developer.nvidia.com/sites/all/modules/custom/gpugems/books/GPUGems/gpugems_ch38.html
//
// Note1: on shadertoy it's not possible to execute the iterative Jacobi on the texture, 
// so I was inspired from this https://www.shadertoy.com/view/MdSczK to pre-compute 20 steps of the Jacobi solver.
// In the Common tab there is the Python script I used to compute the coefficients that are used in the Buffer C.
// 
// Note2: cannot add the diffusion step due there are no more buffers
//
//
// BUFFER A: output velocity field after advection of BUFFER D
// BUFFER B: output the divergence of the velocity field in x and generate and advect the RGB values colors in yxw
// BUFFER C: solve the Poisson equation and output the pressure field 
// BUFFER D: subtract the gradient of the pressure from the advected velocity field to obtain the divergence free field
//

__KERNEL__ void FluidatheartJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel1)
{
    CONNECT_SLIDER1(Alpha, 0.0f, 1.0f, 1.0f);
    
    CONNECT_SLIDER6(HeartSize, 0.0f, 10.0f, 1.8f);
  
    fragCoord+=0.5f;

    fragColor = to_float4_aw(swi3(texture(iChannel1, ( fragCoord-to_float2(0.5f,0.5f))/iResolution),y,z,w),1);
    
    // Gamma correction
    fragColor = pow_f4(fragColor, to_float4_s(1.0f/2.2f) );
    
    // Heart shape where there is no fluid
    float2 p = (fragCoord*2.0f-iResolution)/iResolution.y;
    float d = sdHeart(p*HeartSize+to_float2(0.0f,0.5f));
    if(d<0.0f) fragColor.w = Alpha;//u.x=0.0f,u.y=0.0f;//swi2(u,x,y) = to_float2(0);

  SetFragmentShaderComputedColor(fragColor);
}