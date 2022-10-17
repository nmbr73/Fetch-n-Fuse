

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
    fragColor = vec4(0,0,0,1);
    vec2 uv = fragCoord / iResolution.xy;

    vec4 data = texture(iChannel0, fragCoord / iResolution.xy);
    particle P = getParticle(data, fragCoord);
    vec2 vel = P.V;

    fragColor = vec4(pow(clamp(P.M, 0., 1.), .4));
    fragColor.rgb *= .6 + .6 * cos(3.5*fragColor.x + vec3(0,23,21));

    if(iMouse.z > 0. && length(iMouse.xy - fragCoord) < 10.) fragColor += 0.5;
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
#define MAX_SPEED 1.9
#define MAX_FORCE 0.05

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
    fragColor = vec4(0);
    vec2 uv = fragCoord/iResolution.xy;
    if(iFrame < 10) {
        float q = 2.*PI * hash12(1. + fragCoord);
        particle P;
        P.X = fragCoord;
        P.V = MAX_SPEED * vec2(cos(q), sin(q));
        P.M = .45 - abs(fragCoord.x/iResolution.x - 0.5);
        fragColor = saveParticle(P, fragCoord);
        return;
    }
    
    vec4 data = texelFetch(iChannel0, ivec2(fragCoord), 0);
    particle P = getParticle(data, fragCoord);
    vec2 pos = P.X, vel = P.V;

    float r = 0., l = 0.;

    for(int i = -RADIUS; i <= RADIUS; i++) {
        for(int j = -RADIUS; j <= RADIUS; j++) {
            vec2 ij = vec2(i,j);
            if(ij == vec2(0) || length(ij) > float(RADIUS)) continue;

            vec4 data2 = texelFetch(iChannel0, ivec2(mod(fragCoord + ij, iResolution.xy)), 0);
            particle P2 = getParticle(data2, fragCoord + ij);
            vec2 pos2 = P2.X;
            float m = P2.M;

            vec2 d = pos - pos2;
            float side = dot(vel, vec2(-d.y, d.x));
            if(side > 0.0)
                r += m;
            else
                l += m;
        }
    }

    float angle = atan(vel.y, vel.x);
    angle += ALPHA + BETA * (r + l) * tanh(r - l);
    P.V = vec2(cos(angle), sin(angle));
    fragColor = saveParticle(P, fragCoord);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// reintegration tracking code from https://www.shadertoy.com/view/ttBcWm
#define Bi(p) ivec2(mod(p,iResolution.xy))
#define texel(a, p) texelFetch(a, Bi(p), 0)

#define range(i,a,b) for(int i = a; i <= b; i++)

#define dt 1.5

vec3 distribution(vec2 x, vec2 p, float K)
{
    vec4 aabb0 = vec4(p - 0.5, p + 0.5);
    vec4 aabb1 = vec4(x - K*0.5, x + K*0.5);
    vec4 aabbX = vec4(max(aabb0.xy, aabb1.xy), min(aabb0.zw, aabb1.zw));
    vec2 center = 0.5*(aabbX.xy + aabbX.zw); //center of mass
    vec2 size = max(aabbX.zw - aabbX.xy, 0.); //only positive
    float m = size.x*size.y/(K*K); //relative amount
    //if any of the dimensions are 0 then the mass is 0
    return vec3(center, m);
}

//diffusion and advection basically
void Reintegration(sampler2D ch, inout particle P, vec2 pos)
{
    //basically integral over all updated neighbor distributions
    //that fall inside of this pixel
    //this makes the tracking conservative
    range(i, -3, 3) range(j, -3, 3)
    {
        vec2 tpos = pos + vec2(i,j);
        vec4 data = texel(ch, tpos);
       
        particle P0 = getParticle(data, tpos);
       
        P0.X += P0.V*dt; //integrate position

        vec3 D = distribution(P0.X, pos, DIFFUSION);
        //the deposited mass into this cell
        float m = P0.M*D.z;
        
        //add weighted by mass
        P.X += D.xy*m;
        P.V += P0.V*m;
        
        //add mass
        P.M += m;
    }
    
    //normalization
    if(P.M != 0.)
    {
        P.X /= P.M;
        P.V /= P.M;
    }
}

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
    particle P;
    Reintegration(iChannel0, P, fragCoord);
    fragColor = saveParticle(P, fragCoord);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define RADIUS 8
#define DIFFUSION 1.12
#define MAX_DENSITY 4.

#define ALPHA radians(uv.y < .5 ? 117. :  0.) // intrinsic turning angle
#define BETA  radians(uv.y < .5 ?  -4. : 13.) // reactive turning angle

#define PI 3.14159265359

struct particle {
    vec2 X, V;
    float M;
};

particle getParticle(vec4 data, vec2 pos) {
    particle P;
    if (data == vec4(0)) return P;
    P.X = data.xy + pos;
    P.M = data.z;
    P.V = vec2(cos(data.w), sin(data.w));
    return P;
}

vec4 saveParticle(particle P, vec2 pos) {
    return vec4(clamp(P.X - pos, -.5, .5), clamp(P.M, 0., MAX_DENSITY), atan(P.V.y, P.V.x));
}

// Hash without Sine
// Creative Commons Attribution-ShareAlike 4.0 International Public License
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
#define HASHSCALE1 .1031
#define HASHSCALE3 vec3(.1031, .1030, .0973)
#define HASHSCALE4 vec4(.1031, .1030, .0973, .1099)

// For smaller input rangers like audio tick or 0-1 UVs use these...
//#define HASHSCALE1 443.8975
//#define HASHSCALE3 vec3(443.897, 441.423, 437.195)
//#define HASHSCALE4 vec3(443.897, 441.423, 437.195, 444.129)



//----------------------------------------------------------------------------------------
//  1 out, 1 in...
float hash11(float p)
{
        vec3 p3  = fract(vec3(p) * HASHSCALE1);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}

//----------------------------------------------------------------------------------------
//  1 out, 2 in...
float hash12(vec2 p)
{
        vec3 p3  = fract(vec3(p.xyx) * HASHSCALE1);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}

//----------------------------------------------------------------------------------------
//  1 out, 3 in...
float hash13(vec3 p3)
{
        p3  = fract(p3 * HASHSCALE1);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}

//----------------------------------------------------------------------------------------
//  2 out, 1 in...
vec2 hash21(float p)
{
        vec3 p3 = fract(vec3(p) * HASHSCALE3);
        p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.xx+p3.yz)*p3.zy);

}

//----------------------------------------------------------------------------------------
///  2 out, 2 in...
vec2 hash22(vec2 p)
{
        vec3 p3 = fract(vec3(p.xyx) * HASHSCALE3);
    p3 += dot(p3, p3.yzx+19.19);
    return fract((p3.xx+p3.yz)*p3.zy);

}

//----------------------------------------------------------------------------------------
///  2 out, 3 in...
vec2 hash23(vec3 p3)
{
        p3 = fract(p3 * HASHSCALE3);
    p3 += dot(p3, p3.yzx+19.19);
    return fract((p3.xx+p3.yz)*p3.zy);
}

//----------------------------------------------------------------------------------------
//  3 out, 1 in...
vec3 hash31(float p)
{
   vec3 p3 = fract(vec3(p) * HASHSCALE3);
   p3 += dot(p3, p3.yzx+19.19);
   return fract((p3.xxy+p3.yzz)*p3.zyx); 
}


//----------------------------------------------------------------------------------------
///  3 out, 2 in...
vec3 hash32(vec2 p)
{
        vec3 p3 = fract(vec3(p.xyx) * HASHSCALE3);
    p3 += dot(p3, p3.yxz+19.19);
    return fract((p3.xxy+p3.yzz)*p3.zyx);
}

//----------------------------------------------------------------------------------------
///  3 out, 3 in...
vec3 hash33(vec3 p3)
{
        p3 = fract(p3 * HASHSCALE3);
    p3 += dot(p3, p3.yxz+19.19);
    return fract((p3.xxy + p3.yxx)*p3.zyx);

}

//----------------------------------------------------------------------------------------
// 4 out, 1 in...
vec4 hash41(float p)
{
        vec4 p4 = fract(vec4(p) * HASHSCALE4);
    p4 += dot(p4, p4.wzxy+19.19);
    return fract((p4.xxyz+p4.yzzw)*p4.zywx);
    
}

//----------------------------------------------------------------------------------------
// 4 out, 2 in...
vec4 hash42(vec2 p)
{
        vec4 p4 = fract(vec4(p.xyxy) * HASHSCALE4);
    p4 += dot(p4, p4.wzxy+19.19);
    return fract((p4.xxyz+p4.yzzw)*p4.zywx);

}

//----------------------------------------------------------------------------------------
// 4 out, 3 in...
vec4 hash43(vec3 p)
{
        vec4 p4 = fract(vec4(p.xyzx)  * HASHSCALE4);
    p4 += dot(p4, p4.wzxy+19.19);
    return fract((p4.xxyz+p4.yzzw)*p4.zywx);
}

//----------------------------------------------------------------------------------------
// 4 out, 4 in...
vec4 hash44(vec4 p4)
{
        p4 = fract(p4  * HASHSCALE4);
    p4 += dot(p4, p4.wzxy+19.19);
    return fract((p4.xxyz+p4.yzzw)*p4.zywx);
}
