
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: Uffizi Gallery_0' to iChannel0

__DEVICE__ float3 _refract_f3(float3 I, float3 N, float eta, float refmul, float refoff) {
   float dotNI = dot(N, I);
   float k = 1.0f - eta * eta * (1.0f - dotNI * dotNI);
   if (k < 0.0f) {
     return to_float3_s(0.0);
   }
   return eta * I - (eta * dotNI * _sqrtf(k)) * N * refmul + refoff; //+0.5f;   * -01.50f;(MarchingCubes)  - 0.15f; (GlassDuck)
}


// Inspired by https://youtu.be/NCpaaLkmXI8


// Better not modify these 6, will probably mess up things
// Constants
#define PI 3.1415926535f
#define TAU 2.0f * PI

// Ray Marching
#define EPSILON 0.001f
#define MAX_STEPS 250
#define MAX_DIST 1000.0f
#define MAX_INTERNAL_REFLECTIONS 10


// Modify these
// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
#define ZOOM 1.0f
#define DISTANCE 5.0f
#define ROTATION_SPEED 0.5f

// My implementation of chromatic aberration (CA) is multiplicative (instead of additive)
// which indicates that 1.0f means no aberration and values greater than 1.0f create the
// rainbow-like effect of CA. Values in (0.0f; 1.0f) invert the order of the rainbow colors.
// Negative values don't make sense I guess since the ratio of the refractive indices, eta,
// would then also be negative
#define CHROMATIC_ABERRATION 1.025f
//#define COLOR to_float3(0.0f, 1.0f, 1.0f)
//#define DARKER_WHEN_THICKER

// These are the refractive indices
#define AIR 1.000293f
#define GLASS 1.52f
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


__DEVICE__ float sdSphere(float3 p, float r)
{
    return length(p) - r;
}

// https://iquilezles.org/articles/distfunctions/
__DEVICE__ float sdRoundBox(float3 p, float3 b, float r)
{
    float3 q = abs_f3(p) - b;
    return length(_fmaxf(q,to_float3_s(0.0f))) + _fminf(max(q.x,_fmaxf(q.y,q.z)),0.0f) - r;
}

__DEVICE__ float GetDist(float3 p)
{
    float minDist = 1e20;
    
    //float sphere1 = sdSphere(p, 0.5f);
    float roundBox1 = sdRoundBox(p, to_float3(1.0f, 1.0f ,1.0f), 0.333f);
    //minDist = _fmaxf(roundBox1, -sphere1);
    minDist = roundBox1;
    
    return minDist;
}


__DEVICE__ float3 GetNormal(float3 p)
{
    //return normalize(cross(dFdx(p), dFdy(p)));  // Works best for voxels

    float d = GetDist(p);
    float2 e = to_float2(EPSILON, 0);
    return normalize(d - to_float3(GetDist(p - swi3(e,x,y,y)),
                                   GetDist(p - swi3(e,y,x,y)),
                                   GetDist(p - swi3(e,y,y,x))));
}


__DEVICE__ float RayMarch(float3 ro, float3 rd, float side)
{
    float dO = 0.0f;

    for (int i = 0; i < MAX_STEPS; i++)
    {
        float3 p = ro + rd * dO;
        float dS = side * GetDist(p);
        dO += dS;
        if (_fabs(dS) <= EPSILON || dO > MAX_DIST) break;
    }
    
    return dO;
}

// Correction offset so that RayMarch does not return immediately after starting to march
// Add it to the ray origin for reflection
// Subtract it from the ray origin for refraction
__DEVICE__ float3 offset(float3 n)
{
    return EPSILON * 2.0f * n;
}

__DEVICE__ bool TIR(float3 refraction)
{
    //return dot(refraction, refraction) == 0.0f;
    
    // Probably faster because the entire expression will be false 
    // if it encounters one false expression
    return refraction.x == 0.0f && refraction.y == 0.0f && refraction.z == 0.0f;
}

__DEVICE__ float4 RayMarchRefraction(float3 pOut, float3 rd, float3 nOut, float eta, float4 color, __TEXTURE2D__ iChannel0, float refmul, float refoff)
{
    float etaEnter = eta;  // AIR / GLASS
    float etaExit = 1.0f / eta;  // GLASS / AIR

    float3 refrDirEnter = _refract_f3(rd, nOut, etaEnter, refmul, refoff);

    // We need to march for the refraction since we need to know the point of intersection
    float dIn = RayMarch(pOut - offset(nOut), refrDirEnter, -1.0f);
    float thickness = dIn;

    // We do not need to check for anything, since we are guaranteed to hit the object
    // again when we start to march in the interior of the object
    float3 pIn = pOut + refrDirEnter * dIn;
    float3 nIn = -1.0f*GetNormal(pIn);
    float3 refrDirExit = _refract_f3(refrDirEnter, nIn, etaExit, refmul, refoff);

    // TIR -> Total Internal Reflection
    for (int i = 0; TIR(refrDirExit) && i < MAX_INTERNAL_REFLECTIONS; i++)
    {
        refrDirEnter = reflect(refrDirEnter, nIn);
        dIn = RayMarch(pIn + offset(nIn), refrDirEnter, -1.0f);
        pIn = pIn + refrDirEnter * dIn;
        nIn = -1.0f*GetNormal(pIn);
        refrDirExit = _refract_f3(refrDirEnter, nIn, etaExit, refmul, refoff);
        thickness += dIn;
    }

#ifndef DARKER_WHEN_THICKER
    thickness = 1.0f;
#endif

    return color * decube_f3(iChannel0,refrDirExit) / thickness;
}


__KERNEL__ void RefractionSpi3LotFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
    CONNECT_COLOR0(COLOR, 0.0f, 1.0f, 1.0f, 1.0f);
    CONNECT_SLIDER1(refmul, -1.0f, 10.0f, 1.0f); 
    CONNECT_SLIDER2(refoff, -1.0f, 10.0f, 0.0f);
    


    float2 uv = (fragCoord - iResolution * 0.5f) / _fminf(iResolution.x, iResolution.y);
    float2 mouse = swi2(iMouse,x,y) / iResolution;

    float3 ro;

    if (iMouse.z > 0.0f) 
    {
        float yaw = TAU * mouse.x;
        float pitch = PI * mouse.y;
        
        float cy = _cosf(yaw);
        float sy = _sinf(yaw);
        float cp = _cosf(pitch);
        float sp = _sinf(pitch);
        
        ro = DISTANCE * to_float3(cy, cp, sy);
        //swi2(ro,x,z) *= sp;  // Adjusting X and Z position accordingly when changing the pitch
        ro.x *= sp;  // Adjusting X and Z position accordingly when changing the pitch
        ro.z *= sp;  // Adjusting X and Z position accordingly when changing the pitch
    }
    else
    {
        float x = iTime * ROTATION_SPEED;
        float c = _cosf(x);
        float s = _sinf(x);
        float3 cs0 = to_float3(c, s, 0);
        ro = DISTANCE * swi3(cs0,x,y,y);
    }

    float3 lookAt = to_float3_s(0),
         f = normalize(lookAt - ro),
         r = normalize(cross(f, to_float3(0,1,0))),
         u = cross(r, f),
         c = ro + f * ZOOM,
         i = c + uv.x * r + uv.y * u,
         rd = normalize(i - ro);
         
    float4 col;
    float dOut = RayMarch(ro, rd, 1.0f);
    
    if (dOut <= MAX_DIST)
    {
        float3 pOut = ro + rd * dOut;
        //float distOut = GetDist(pOut);
       
        // We don't need to check for distOut <= EPSILON because the only other thing that
        // could've possibly happened when dOut <= MAX_DIST is that we ran out of steps which
        // is an indicator that we've very likely hit or got very close to hitting an object
        
        float3 nOut = GetNormal(pOut);
        float3 reflDir = reflect(rd, nOut);
        
        // We don't need to march again for the reflection since a box/sphere
        // cannot reflect itself - at least not from the outside
        float4 reflCol = decube_f3(iChannel0,reflDir);
        
        float eta = AIR / GLASS;

        float4 refrColRed = RayMarchRefraction(pOut, rd, nOut, eta / CHROMATIC_ABERRATION, to_float4(1,0,0,1),iChannel0,refmul,refoff);
        float4 refrColGreen = RayMarchRefraction(pOut, rd, nOut, eta, to_float4(0,1,0,1),iChannel0,refmul,refoff);
        float4 refrColBlue = RayMarchRefraction(pOut, rd, nOut, eta * CHROMATIC_ABERRATION, to_float4(0,0,1,1),iChannel0,refmul,refoff);
        
        //vec4 refrCol = _fmaxf(max(refrColRed, refrColGreen), refrColBlue);
        float4 refrCol = refrColRed + refrColGreen + refrColBlue;
        refrCol *= to_float4_aw(swi3(COLOR,x,y,z), 0.333f);
        
        //col = refrCol;  // No reflection - pure refraction
        col = _mix(refrCol, reflCol, 0.333f);  // Refraction and reflection blended together
    }
    else
    {
        col = decube_f3(iChannel0,rd);
    }

    fragColor = col;

  SetFragmentShaderComputedColor(fragColor);
}