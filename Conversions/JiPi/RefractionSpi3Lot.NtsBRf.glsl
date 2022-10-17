

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Inspired by https://youtu.be/NCpaaLkmXI8


// Better not modify these 6, will probably mess up things
// Constants
#define PI 3.1415926535
#define TAU 2.0 * PI

// Ray Marching
#define EPSILON 0.001
#define MAX_STEPS 250
#define MAX_DIST 1000.0
#define MAX_INTERNAL_REFLECTIONS 10


// Modify these
// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
#define ZOOM 1.0
#define DISTANCE 5.0
#define ROTATION_SPEED 0.5

// My implementation of chromatic aberration (CA) is multiplicative (instead of additive)
// which indicates that 1.0 means no aberration and values greater than 1.0 create the
// rainbow-like effect of CA. Values in (0.0; 1.0) invert the order of the rainbow colors.
// Negative values don't make sense I guess since the ratio of the refractive indices, eta,
// would then also be negative
#define CHROMATIC_ABERRATION 1.025
#define COLOR vec3(0.0, 1.0, 1.0)
//#define DARKER_WHEN_THICKER

// These are the refractive indices
#define AIR 1.000293
#define GLASS 1.52
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


float sdSphere(vec3 p, float r)
{
    return length(p) - r;
}

// https://iquilezles.org/articles/distfunctions/
float sdRoundBox(vec3 p, vec3 b, float r)
{
    vec3 q = abs(p) - b;
    return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0) - r;
}

float GetDist(vec3 p)
{
    float minDist = 1e20;
    
    //float sphere1 = sdSphere(p, 0.5);
    float roundBox1 = sdRoundBox(p, vec3(1.0, 1.0 ,1.0), 0.333);
    //minDist = max(roundBox1, -sphere1);
    minDist = roundBox1;
    
    return minDist;
}


vec3 GetNormal(vec3 p)
{
    //return normalize(cross(dFdx(p), dFdy(p)));  // Works best for voxels

    float d = GetDist(p);
    vec2 e = vec2(EPSILON, 0);
    return normalize(d - vec3(GetDist(p - e.xyy),
                              GetDist(p - e.yxy),
                              GetDist(p - e.yyx)));
}


float RayMarch(vec3 ro, vec3 rd, float side)
{
    float dO = 0.0;

    for (int i = 0; i < MAX_STEPS; i++)
    {
        vec3 p = ro + rd * dO;
        float dS = side * GetDist(p);
        dO += dS;
        if (abs(dS) <= EPSILON || dO > MAX_DIST) break;
    }
    
    return dO;
}

// Correction offset so that RayMarch does not return immediately after starting to march
// Add it to the ray origin for reflection
// Subtract it from the ray origin for refraction
vec3 offset(vec3 n)
{
    return EPSILON * 2.0 * n;
}

bool TIR(vec3 refraction)
{
    //return dot(refraction, refraction) == 0.;
    
    // Probably faster because the entire expression will be false 
    // if it encounters one false expression
    return refraction.x == 0. && refraction.y == 0. && refraction.z == 0.;
}

vec4 RayMarchRefraction(vec3 pOut, vec3 rd, vec3 nOut, float eta, vec4 color)
{
    float etaEnter = eta;  // AIR / GLASS
    float etaExit = 1.0 / eta;  // GLASS / AIR

    vec3 refrDirEnter = refract(rd, nOut, etaEnter);

    // We need to march for the refraction since we need to know the point of intersection
    float dIn = RayMarch(pOut - offset(nOut), refrDirEnter, -1.0);
    float thickness = dIn;

    // We do not need to check for anything, since we are guaranteed to hit the object
    // again when we start to march in the interior of the object
    vec3 pIn = pOut + refrDirEnter * dIn;
    vec3 nIn = -GetNormal(pIn);
    vec3 refrDirExit = refract(refrDirEnter, nIn, etaExit);

    // TIR -> Total Internal Reflection
    for (int i = 0; TIR(refrDirExit) && i < MAX_INTERNAL_REFLECTIONS; i++)
    {
        refrDirEnter = reflect(refrDirEnter, nIn);
        dIn = RayMarch(pIn + offset(nIn), refrDirEnter, -1.0);
        pIn = pIn + refrDirEnter * dIn;
        nIn = -GetNormal(pIn);
        refrDirExit = refract(refrDirEnter, nIn, etaExit);
        thickness += dIn;
    }

#ifndef DARKER_WHEN_THICKER
    thickness = 1.0;
#endif

    return color * texture(iChannel0, refrDirExit) / thickness;
}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (fragCoord - iResolution.xy * 0.5) / min(iResolution.x, iResolution.y);
    vec2 mouse = iMouse.xy / iResolution.xy;

    vec3 ro;

    if (iMouse.z > 0.0) 
    {
        float yaw = TAU * mouse.x;
        float pitch = PI * mouse.y;
        
        float cy = cos(yaw);
        float sy = sin(yaw);
        float cp = cos(pitch);
        float sp = sin(pitch);
        
        ro = DISTANCE * vec3(cy, cp, sy);
        ro.xz *= sp;  // Adjusting X and Z position accordingly when changing the pitch
    }
    else
    {
        float x = iTime * ROTATION_SPEED;
        float c = cos(x);
        float s = sin(x);
        vec3 cs0 = vec3(c, s, 0);
        ro = DISTANCE * cs0.xyy;
    }

    vec3 lookAt = vec3(0),
         f = normalize(lookAt - ro),
         r = normalize(cross(f, vec3(0,1,0))),
         u = cross(r, f),
         c = ro + f * ZOOM,
         i = c + uv.x * r + uv.y * u,
         rd = normalize(i - ro);
         
    vec4 col;
    float dOut = RayMarch(ro, rd, 1.0);
    
    if (dOut <= MAX_DIST)
    {
        vec3 pOut = ro + rd * dOut;
        //float distOut = GetDist(pOut);
       
        // We don't need to check for distOut <= EPSILON because the only other thing that
        // could've possibly happened when dOut <= MAX_DIST is that we ran out of steps which
        // is an indicator that we've very likely hit or got very close to hitting an object
        
        vec3 nOut = GetNormal(pOut);
        vec3 reflDir = reflect(rd, nOut);
        
        // We don't need to march again for the reflection since a box/sphere
        // cannot reflect itself - at least not from the outside
        vec4 reflCol = texture(iChannel0, reflDir);
        
        float eta = AIR / GLASS;

        vec4 refrColRed = RayMarchRefraction(pOut, rd, nOut, eta / CHROMATIC_ABERRATION, vec4(1,0,0,1));
        vec4 refrColGreen = RayMarchRefraction(pOut, rd, nOut, eta, vec4(0,1,0,1));
        vec4 refrColBlue = RayMarchRefraction(pOut, rd, nOut, eta * CHROMATIC_ABERRATION, vec4(0,0,1,1));
        
        //vec4 refrCol = max(max(refrColRed, refrColGreen), refrColBlue);
        vec4 refrCol = refrColRed + refrColGreen + refrColBlue;
        refrCol *= vec4(COLOR, 0.333);
        
        //col = refrCol;  // No reflection - pure refraction
        col = mix(refrCol, reflCol, 0.333);  // Refraction and reflection blended together
    }
    else
    {
        col = texture(iChannel0, rd);
    }

    fragColor = col;
}
