// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define NEIGHBOR_DIST 6
#define DIFFUSION     1.12f
#define ALIGNMENT     0.45f
#define SEPARATION    1.0f
#define COHESION      0.9f

///

#define PI 3.14159265359

__DEVICE__ float2 clamp_length(float2 v, float r) {
    if(length(v) > r) return r * normalize(v);
    return v;
}

__DEVICE__ uint pack(float2 x)
{
    x = 65535.0f*clamp(0.5f*x+0.5f, 0.0f, 1.0f);
    return uint(round(x.x)) + 65535u*uint(round(x.y));
}

__DEVICE__ float2 unpack(uint a)
{
    float2 x = to_float2(a%65535u, a/65535u);
    return clamp(x/65535.0f, 0.0f,1.0f)*2.0f - 1.0f;
}



union Zahl
 {
   float  _Float; //32bit float
   uint   _Uint;  //32bit unsigend integer
 };

__DEVICE__ float2 decode(float _x)
{
	Zahl z;
  //uint X = floatBitsToUint(x);
	z._Float = _x;
  return unpack(z._Uint); 
}

__DEVICE__ float encode(float2 _x)
{
	Zahl z;
    uint X = pack(_x);
	
	z._Uint = X;
  //return uintBitsToFloat(X); 
	return (z._Float);
}


struct particle
{
    float2 X;
    float2 V;
    float M;
};
    
__DEVICE__ particle getParticle(float4 data, float2 pos)
{
    particle P = {to_float2_s(0.0f),to_float2_s(0.0f),0.0f};
    if (data.x == 0.0f && data.y == 0.0f && data.z == 0.0f && data.w == 0.0f) return P;
    P.X = decode(data.x) + pos;
    P.M = data.y;
    P.V = swi2(data,z,w);
    return P;
}

__DEVICE__ float4 saveParticle(particle P, float2 pos)
{
    float2 x = clamp(P.X - pos, to_float2_s(-0.5f), to_float2_s(0.5f));
    return to_float4(encode(x), P.M, P.V.x,P.V.y);
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
//  1 out, 2 in...
__DEVICE__ float hash12(float2 p)
{
    float3 p3  = fract_f3((swi3(p,x,y,x)) * HASHSCALE1);
    p3 += dot(p3, swi3(p3,y,z,x) + 19.19f);
    return fract((p3.x + p3.y) * p3.z);
}


// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer B' to iChannel0
// Connect Buffer A 'Previsualization: Buffer C' to iChannel1



#define MAX_SPEED 1.9f
#define MAX_FORCE 0.05f

__KERNEL__ void FluidicBoidsIiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(MouseDisplay, 1);
    CONNECT_SLIDER1(MouseSize, 0.0f, 2000.0f, 200.0f);
    
    fragCoord += 0.5f;
    fragColor = to_float4_s(0);
    if(iFrame < 10) {
        float q = 2.0f*PI * hash12(1.0f + fragCoord);
        particle P;
        P.X = fragCoord;
        P.V = MAX_SPEED * to_float2(_cosf(q), _sinf(q));
        P.M = 0.25f;
        fragColor = saveParticle(P, fragCoord);
        SetFragmentShaderComputedColor(fragColor);
        return;
    }
    
    float4 data = texture(iChannel0, fragCoord/iResolution);
    particle P = getParticle(data, fragCoord);

    float2 pos = P.X;
    float2 vel = P.V;
    float m0 = P.M;

    float mass = 0.0f;

    float2 alignment = to_float2_s(0);
    float2 cohesion = to_float2_s(0);
    float2 separation = to_float2_s(0);

    for(int i = -NEIGHBOR_DIST; i <= NEIGHBOR_DIST; i++) {
        for(int j = -NEIGHBOR_DIST; j <= NEIGHBOR_DIST; j++) {
            float2 ij = to_float2(i,j);
            if((ij.x == 0.0f && ij.y == 0.0f )|| length(ij) > (float)(NEIGHBOR_DIST)) continue;

            float4 data2 = texture(iChannel0, fract_f2((fragCoord + ij) / iResolution));
            particle P2 = getParticle(data2, fragCoord + ij);
            float2 pos2 = P2.X;
            float2 vel2 = P2.V;
            float m = P2.M;

            float d2 = dot(pos - pos2, pos - pos2);
            if(d2 < 1e-6) continue;
            separation += clamp(_powf(m + m0, 9.0f), 0.0f, 9.0f) * (pos - pos2) / d2;

            alignment += m * vel2;
            cohesion  += m * pos2;
            mass      += m;
        }
    }


    float d = length(pos - swi2(iMouse,x,y));
    if(iMouse.z > 0.0f && d < MouseSize) { //200.0f
        alignment += swi2(iMouse,x,y) - swi2(texture(iChannel1, to_float2_s(0)),x,y);
        if(d < 50.0f) {
            separation += pos - swi2(iMouse,x,y);
        } else {
            cohesion += d/1e3 * swi2(iMouse,x,y);
            mass     += d/1e3;
        }
    }
  
    
    cohesion = cohesion / mass - pos;
    cohesion   = clamp_length(MAX_SPEED * normalize(cohesion)   - vel, MAX_FORCE);
    alignment  = clamp_length(MAX_SPEED * normalize(alignment)  - vel, MAX_FORCE);
    separation = clamp_length(MAX_SPEED * normalize(separation) - vel, MAX_FORCE);

    if(!(isnan(cohesion.x) || (isnan(cohesion.y))))   vel += cohesion * COHESION;
    if(!(isnan(alignment.x) || (isnan(alignment.y))))  vel += alignment * ALIGNMENT;
    if(!(isnan(separation.x) || (isnan(separation.y)))) vel += separation * SEPARATION;
    P.V = clamp_length(vel, MAX_SPEED);
    fragColor = saveParticle(P, fragCoord);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


// reintegration tracking code from https://www.shadertoy.com/view/ttBcWm
#define Bi(p) to_int2_cfloat(mod_f2f2(p,iResolution))
#define texel(a, p) texture(a, (make_float2(Bi(p))+0.5)/iResolution) // !make statt to !


#define range(i,a,b) for(int i = a; i <= b; i++)

#define dt 1.5 // über Parameter -> Geschwindigkeit 

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
__DEVICE__ particle Reintegration(__TEXTURE2D__ ch, particle P, float2 pos, float2 iResolution, float DT)
{
    //basically integral over all updated neighbor distributions
    //that fall inside of this pixel
    //this makes the tracking conservative
    range(i, -3, 3) range(j, -3, 3)
    {
        float2 tpos = pos + to_float2(i,j);
        float4 data = texel(ch, tpos);
       
        particle P0 = getParticle(data, tpos);
       
        //P0.X += P0.V*dt; //integrate position
        P0.X += P0.V*DT; //integrate position

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

__KERNEL__ void FluidicBoidsIiFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

    CONNECT_SLIDER0(DT, 0.0f, 5.0f, 1.5f);

    fragCoord += 0.5f;
    particle P = {to_float2_s(0.0f),to_float2_s(0.0f),0.0f};
    P = Reintegration(iChannel0, P, fragCoord, iResolution, DT);
    fragColor = saveParticle(P, fragCoord);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------


__KERNEL__ void FluidicBoidsIiFuse__Buffer_C(float4 fragColor, float2 fragCoord, float4 iMouse)
{

    fragColor = iMouse;


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer B' to iChannel0


__KERNEL__ void FluidicBoidsIiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
  
    CONNECT_CHECKBOX0(MouseDisplay, 1);
    CONNECT_SLIDER1(MouseSize, 0.0f, 2000.0f, 200.0f);


    fragColor = to_float4(0,0,0,1);
    float2 uv = fragCoord / iResolution;

    float4 data = texture(iChannel0, fragCoord / iResolution);
    particle P = getParticle(data, fragCoord);
    float2 vel = P.V;

    swi3S(fragColor,x,y,z, 0.6f + 0.6f * cos_f3(_atan2f(vel.y,vel.x) + to_float3(0,23,21)));
    swi3S(fragColor,x,y,z, swi3(fragColor,x,y,z) * _powf(clamp(P.M, 0.0f, 1.0f), 0.4f));

    //if(iMouse.z > 0.0f && length(swi2(iMouse,x,y) - fragCoord) < 10.0f) fragColor += 0.5f;
    if(iMouse.z > 0.0f && MouseDisplay == 1 && length(swi2(iMouse,x,y) - fragCoord) < MouseSize/20.0f) fragColor += 0.5f;


  SetFragmentShaderComputedColor(fragColor);
}