
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define R    iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


#define RADIUS 8
//#define DIFFUSION 1.12f
//#define MAX_DENSITY 4.0f

#define ALPHA radians(uv.y < 0.5f ? 117.0f :  0.0f) // intrinsic turning angle
#define BETA  radians(uv.y < 0.5f ?  -4.0f : 13.0f) // reactive turning angle

#define PI 3.14159265359f

struct particle {
    float2 X, V;
    float M;
};

__DEVICE__ particle getParticle(float4 data, float2 pos) {
    particle P = {to_float2_s(0.0f),to_float2_s(0.0f),0.0f};
    //if (data == to_float4_s(0)) return P;
    if (data.x == 0.0f&&data.y == 0.0f&&data.z == 0.0f&&data.w == 0.0f) return P;
    P.X = swi2(data,x,y) + pos;
    P.M = data.z;
    P.V = to_float2(_cosf(data.w), _sinf(data.w));
    return P;
}

__DEVICE__ float4 saveParticle(particle P, float2 pos, float MAX_DENSITY) {
    return to_float4(clamp(P.X.x - pos.x, -0.5f, 0.5f),clamp(P.X.y - pos.y, -0.5f, 0.5f), clamp(P.M, 0.0f, MAX_DENSITY), _atan2f(P.V.y, P.V.x));
}

// Hash without Sine
// Creative Commons Attribution-ShareAlike 4.0f International Public License
// Created by David Hoskins.

// https://www.shadertoy.com/view/4djSRW
// Trying to find a Hash function that is the same on ALL systens
// and doesn't rely on trigonometry functions that change accuracy 
// depending on GPU. 
// New one on the left, sine function on the right.
// It appears to be the same speed, but I suppose that depends.

// * Note. It still goes wrong eventually!
// * Try full-screen paused to see details.


#define ITERATIONS 4


// *** Change these to suit your range of random numbers..

// *** Use this for integer stepped ranges, ie Value-Noise/Perlin noise functions.
#define HASHSCALE1 0.1031f
#define HASHSCALE3 to_float3(0.1031f, 0.1030f, 0.0973f)
#define HASHSCALE4 to_float4(0.1031f, 0.1030f, 0.0973f, 0.1099f)

// For smaller input rangers like audio tick or 0-1 UVs use these...
//#define HASHSCALE1 443.8975
//#define HASHSCALE3 to_float3(443.897f, 441.423f, 437.195f)
//#define HASHSCALE4 to_float3(443.897f, 441.423f, 437.195f, 444.129f)

//----------------------------------------------------------------------------------------
//  1 out, 1 in...
__DEVICE__ float hash11(float p)
{
    float3 p3  = fract_f3(to_float3_s(p) * HASHSCALE1);
    p3 += dot(p3, swi3(p3,y,z,x) + 19.19f);
    return fract((p3.x + p3.y) * p3.z);
}

//----------------------------------------------------------------------------------------
//  1 out, 2 in...
__DEVICE__ float hash12(float2 p)
{
    float3 p3  = fract_f3(swi3(p,x,y,x) * HASHSCALE1);
    p3 += dot(p3, swi3(p3,y,z,x) + 19.19f);
    return fract((p3.x + p3.y) * p3.z);
}

//----------------------------------------------------------------------------------------
//  1 out, 3 in...
__DEVICE__ float hash13(float3 p3)
{
    p3  = fract_f3(p3 * HASHSCALE1);
    p3 += dot(p3, swi3(p3,y,z,x) + 19.19f);
    return fract((p3.x + p3.y) * p3.z);
}

//----------------------------------------------------------------------------------------
//  2 out, 1 in...
__DEVICE__ float2 hash21(float p)
{
    float3 p3 = fract_f3(to_float3_s(p) * HASHSCALE3);
    p3 += dot(p3, swi3(p3,y,z,x) + 19.19f);
    return fract_f2((swi2(p3,x,x)+swi2(p3,y,z))*swi2(p3,z,y));
}

//----------------------------------------------------------------------------------------
///  2 out, 2 in...
__DEVICE__ float2 hash22(float2 p)
{
    float3 p3 = fract_f3(swi3(p,x,y,x) * HASHSCALE3);
    p3 += dot(p3, swi3(p3,y,z,x)+19.19f);
    return fract_f2((swi2(p3,x,x)+swi2(p3,y,z))*swi2(p3,z,y));
}

//----------------------------------------------------------------------------------------
///  2 out, 3 in...
__DEVICE__ float2 hash23(float3 p3)
{
    p3 = fract_f3(p3 * HASHSCALE3);
    p3 += dot(p3, swi3(p3,y,z,x)+19.19f);
    return fract_f2((swi2(p3,x,x)+swi2(p3,y,z))*swi2(p3,z,y));
}

//----------------------------------------------------------------------------------------
//  3 out, 1 in...
__DEVICE__ float3 hash31(float p)
{
   float3 p3 = fract_f3(to_float3_s(p) * HASHSCALE3);
   p3 += dot(p3, swi3(p3,y,z,x)+19.19f);
   return fract_f3((swi3(p3,x,x,y)+swi3(p3,y,z,z))*swi3(p3,z,y,x)); 
}


//----------------------------------------------------------------------------------------
///  3 out, 2 in...
__DEVICE__ float3 hash32(float2 p)
{
    float3 p3 = fract_f3(swi3(p,x,y,x) * HASHSCALE3);
    p3 += dot(p3, swi3(p3,y,x,z)+19.19f);
    return fract_f3((swi3(p3,x,x,y)+swi3(p3,y,z,z))*swi3(p3,z,y,x));
}

//----------------------------------------------------------------------------------------
///  3 out, 3 in...
__DEVICE__ float3 hash33(float3 p3)
{
    p3 = fract_f3(p3 * HASHSCALE3);
    p3 += dot(p3, swi3(p3,y,x,z)+19.19f);
    return fract_f3((swi3(p3,x,x,y) + swi3(p3,y,x,x))*swi3(p3,z,y,x));
}

//----------------------------------------------------------------------------------------
// 4 out, 1 in...
__DEVICE__ float4 hash41(float p)
{
    float4 p4 = fract_f4(to_float4_s(p) * HASHSCALE4);
    p4 += dot(p4, swi4(p4,w,z,x,y)+19.19f);
    return fract_f4((swi4(p4,x,x,y,z)+swi4(p4,y,z,z,w))*swi4(p4,z,y,w,x));
}

//----------------------------------------------------------------------------------------
// 4 out, 2 in...
__DEVICE__ float4 hash42(float2 p)
{
    float4 p4 = fract_f4((swi4(p,x,y,x,y)) * HASHSCALE4);
    p4 += dot(p4, swi4(p4,w,z,x,y)+19.19f);
    return fract_f4((swi4(p4,x,x,y,z)+swi4(p4,y,z,z,w))*swi4(p4,z,y,w,x));
}

//----------------------------------------------------------------------------------------
// 4 out, 3 in...
__DEVICE__ float4 hash43(float3 p)
{
    float4 p4 = fract_f4((swi4(p,x,y,z,x))  * HASHSCALE4);
    p4 += dot(p4, swi4(p4,w,z,x,y)+19.19f);
    return fract_f4((swi4(p4,x,x,y,z)+swi4(p4,y,z,z,w))*swi4(p4,z,y,w,x));
}

//----------------------------------------------------------------------------------------
// 4 out, 4 in...
__DEVICE__ float4 hash44(float4 p4)
{
    p4 = fract_f4(p4  * HASHSCALE4);
    p4 += dot(p4, swi4(p4,w,z,x,y)+19.19f);
    return fract_f4((swi4(p4,x,x,y,z)+swi4(p4,y,z,z,w))*swi4(p4,z,y,w,x));
}





__DEVICE__ particle Blending( __TEXTURE2D__ channel, float2 uv, particle P, float Blend, float2 Par, float2 MulOff, int Modus, float2 fragCoord, float2 R, float MAX_SPEED)
{
 
    if (Blend > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(channel,uv);

      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2) //Startbedingung
        {
          float q = 2.0f*PI * hash12(1.0f + fragCoord);
          P.X = _mix(P.X, fragCoord, Blend);
          P.V = _mix(P.V, MAX_SPEED * to_float2(_cosf(q), _sinf(q)), Blend);
          P.M = _mix(P.M, 0.45f - _fabs(fragCoord.x/iResolution.x - 0.5f), Blend);
        }      
        if ((int)Modus&4) // Geschwindigkeit
        {
          P.V = Par.x*_mix(P.V, to_float2((tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x), Blend);
        }
        
        if ((int)Modus&8) // Masse
          P.M = _mix(P.M, (tex.x+MulOff.y)*MulOff.x, Blend);

      }
      else
      {
        if ((int)Modus&16) 
          P.M = _mix(P.M, (MulOff.y+0.45) - _fabs(fragCoord.x/iResolution.x - 0.5f), Blend);
      
        if ((int)Modus&32) //Special
        {
          float q = 2.0f*PI * hash12(1.0f + fragCoord);
          P.X = fragCoord;
          P.V = MAX_SPEED * to_float2(_cosf(q), _sinf(q));
          P.M = 0.45f - _fabs(fragCoord.x/iResolution.x - 0.5f);
        }  
      }
    }
  
  return P;
}

#define GS(x) _expf(-dot(x,x))
#define GS0(x) _expf(-length(x))

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer B' to iChannel0


//#define MAX_SPEED 1.9f
#define MAX_FORCE 0.05f

__KERNEL__ void PrimordialSoupSystemFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
  CONNECT_CHECKBOX0(Reset, 0);
  
  CONNECT_CHECKBOX1(ManTurn, 0);
  CONNECT_POINT0(Turning, 117.0f, -4.0f);
  CONNECT_POINT1(TurnCoord, 0.5f, 0.0f);
  
    //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);

    CONNECT_SLIDER5(MAX_SPEED, 0.0f, 5.0f, 1.9f);
    CONNECT_SLIDER6(MAX_DENSITY, 0.0f, 10.0f, 4.0f);
    
    CONNECT_SLIDER8(PenThickness, -1.0f, 50.0f, 13.0f);
  
    fragCoord+=0.5f;

    fragColor = to_float4_s(0);
    float2 uv = fragCoord/iResolution;
    if(iFrame < 10 || Reset) {
        float q = 2.0f*PI * hash12(1.0f + fragCoord);
        particle P;
        P.X = fragCoord;
        P.V = MAX_SPEED * to_float2(_cosf(q), _sinf(q));
        P.M = 0.45f - _fabs(fragCoord.x/iResolution.x - 0.5f);
        fragColor = saveParticle(P, fragCoord, MAX_DENSITY);
        SetFragmentShaderComputedColor(fragColor);
        return;
    }
    
    //float4 data = texelFetch(iChannel0, to_int2_cfloat(fragCoord), 0);
    float4 data = texture(iChannel0, (make_float2(to_int2_cfloat(fragCoord))+0.5f)/R);
    
    particle P = getParticle(data, fragCoord);
    float2 pos = P.X, vel = P.V;

    float r = 0.0f, l = 0.0f;

    for(int i = -RADIUS; i <= RADIUS; i++) {
        for(int j = -RADIUS; j <= RADIUS; j++) {
            float2 ij = to_float2(i,j);
            if((ij.x == 0.0f && ij.y == 0.0f) || length(ij) > (float)(RADIUS)) continue;

            float4 data2 = texture(iChannel0, (make_float2(to_int2_cfloat(mod_f2f2(fragCoord + ij, iResolution)))+0.5f)/R);
            
            particle P2 = getParticle(data2, fragCoord + ij);
            float2 pos2 = P2.X;
            float m = P2.M;

            float2 d = pos - pos2;
            float side = dot(vel, to_float2(-d.y, d.x));
            if(side > 0.0f)
                r += m;
            else
                l += m;
        }
    }

    float angle = _atan2f(vel.y, vel.x);
   
    
    if(ManTurn) 
      angle += radians(Turning.x) + radians(Turning.y) * (r + l) * _tanhf(r - l);
    else
    {
      if (uv.y < 0.5f+TurnCoord.y && uv.x < 0.5f + TurnCoord.x)
        angle += radians(117.0f) + radians(-4.0f) * (r + l) * _tanhf(r - l);
      else
        angle += radians(0.0f) + radians(13.0f) * (r + l) * _tanhf(r - l);
    }
    
    if(iMouse.z > 0.0f)
      P.M = _mix(P.M, 0.5f, GS((pos - swi2(iMouse,x,y))/PenThickness));
    
    P.V = to_float2(_cosf(angle), _sinf(angle));
  
    if (Blend1>0.0) P = Blending(iChannel1, fragCoord/R, P, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, fragCoord, R, MAX_SPEED);  
    
    fragColor = saveParticle(P, fragCoord, MAX_DENSITY);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


// reintegration tracking code from https://www.shadertoy.com/view/ttBcWm
#define Bi(p) to_int2_cfloat(mod_f2f2(p,iResolution))
//#define texel(a, p) texelFetch(a, Bi(p), 0)
#define texel(a, p) texture(a, (make_float2(Bi(p))+0.5f)/R)

#define range(i,a,b) for(int i = a; i <= b; i++)

//#define dt 1.5f

__DEVICE__ float3 distribution(float2 x, float2 p, float K)
{
    float4 aabb0 = to_float4_f2f2(p - 0.5f, p + 0.5f);
    float4 aabb1 = to_float4_f2f2(x - K*0.5f, x + K*0.5f);
    float4 aabbX = to_float4_f2f2(_fmaxf(swi2(aabb0,x,y), swi2(aabb1,x,y)), _fminf(swi2(aabb0,z,w), swi2(aabb1,z,w)));
    float2 center = 0.5f*(swi2(aabbX,x,y) + swi2(aabbX,z,w)); //center of mass
    float2 size = _fmaxf(swi2(aabbX,z,w) - swi2(aabbX,x,y), to_float2_s(0.0f)); //only positive
    float m = size.x*size.y/(K*K); //relative amount
    //if any of the dimensions are 0 then the mass is 0
    return to_float3_aw(center, m);
}

//diffusion and advection basically
__DEVICE__ particle Reintegration(__TEXTURE2D__ ch, particle P, float2 pos, float2 R, float dt, float DIFFUSION)
{
    //basically integral over all updated neighbor distributions
    //that fall inside of this pixel
    //this makes the tracking conservative
    range(i, -3, 3) range(j, -3, 3)
    {
        float2 tpos = pos + to_float2(i,j);
        float4 data = texel(ch, tpos);
       
        particle P0 = getParticle(data, tpos);
       
        P0.X += P0.V*dt; //integrate position

        float3 D = distribution(P0.X, pos, DIFFUSION);
        //the deposited mass into this cell
        float m = P0.M*D.z;
        
        //add weighted by mass
        P.X += swi2(D,x,y)*m;
        P.V += P0.V*m;
        
        //add mass
        P.M += m;
    }

    //normalization
    if(P.M != 0.0f)
    {
        P.X /= P.M;
        P.V /= P.M;
    }
    return P; 
}

__KERNEL__ void PrimordialSoupSystemFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    CONNECT_SLIDER1(dt, -1.0f, 10.0f, 1.5f);
    CONNECT_SLIDER6(MAX_DENSITY, 0.0f, 10.0f, 4.0f);
    CONNECT_SLIDER7(DIFFUSION, 0.0f, 10.0f, 1.12f);
    
    fragCoord+=0.5f;

    particle P = {to_float2_s(0.0f),to_float2_s(0.0f), 0.0f};
    P = Reintegration(iChannel0, P, fragCoord, R, dt, DIFFUSION);
    fragColor = saveParticle(P, fragCoord, MAX_DENSITY);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer B' to iChannel0


__KERNEL__ void PrimordialSoupSystemFuse(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
    CONNECT_COLOR0(Color, 0.0f, 23.0f, 21.0f, 1.0f);
    CONNECT_CHECKBOX2(ShowMouse, 0);
    
    fragCoord+=0.5f;
    
    fragColor = to_float4(0,0,0,1);
    float2 uv = fragCoord / iResolution;

    float4 data = texture(iChannel0, fragCoord / iResolution);
    particle P = getParticle(data, fragCoord);
    float2 vel = P.V;

    fragColor = to_float4_s(_powf(clamp(P.M, 0.0f, 1.0f), 0.4f));
    swi3S(fragColor,x,y,z, swi3(fragColor,x,y,z) * ( 0.6f + 0.6f * cos_f3(3.5f*fragColor.x + swi3(Color,x,y,z)))); //to_float3(0,23,21))));

    if(iMouse.z > 0.0f && length(swi2(iMouse,x,y) - fragCoord) < 10.0f && ShowMouse ) fragColor += 0.5f;

    fragColor.w=Color.w;

  SetFragmentShaderComputedColor(fragColor);
}