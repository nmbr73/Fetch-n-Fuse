
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


// Author: paperu
// Title: plasma storm

// 3D noise, from : https://gist.github.com/patriciogonzalezvivo/670c22f3966e662d2f83
__DEVICE__ float mod289(float x)   {return x - _floor(x * (1.0f / 289.0f)) * 289.0f;}
__DEVICE__ float4 mod289(float4 x) {return x - _floor(x * (1.0f / 289.0f)) * 289.0f;}
__DEVICE__ float4 perm(float4 x)   {return mod289(((x * 34.0f) + 1.0f) * x);}
__DEVICE__ float noise(float3 p){
    float3 a = _floor(p);
    float3 d = p - a;
    d = d * d * (3.0f - 2.0f * d);

    float4 b = swi4(a,x,x,y,y) + to_float4(0.0f, 1.0f, 0.0f, 1.0f);
    float4 k1 = perm(swi4(b,x,y,x,y));
    float4 k2 = perm(swi4(k1,x,y,x,y) + swi4(b,z,z,w,w));

    float4 c = k2 + swi4(a,z,z,z,z);
    float4 k3 = perm(c);
    float4 k4 = perm(c + 1.0f);

    float4 o1 = fract_f4(k3 * (1.0f / 41.0f));
    float4 o2 = fract_f4(k4 * (1.0f / 41.0f));

    float4 o3 = o2 * d.z + o1 * (1.0f - d.z);
    float2 o4 = swi2(o3,y,w) * d.x + swi2(o3,x,z) * (1.0f - d.x);

    return o4.y * d.y + o4.x * (1.0f - d.y);
}

__DEVICE__ mat2 rot(in float a) { return to_mat2(_cosf(a),_sinf(a),-_sinf(a),_cosf(a)); }

// by decreasing RES_INV value we can get better precision but at the cost of a worse framerate
#define RES_INV 5.0f

// some alternatives (ALT1 to ALT4)
#define ALT1

#define MAX_IT_VOL (100*4)/int(RES_INV)
#define STEP_SIZE 0.01f*RES_INV
#define MAX_COL 17.0f/RES_INV*RES_INV*RES_INV

__DEVICE__ float3 rmvolum(in float3 p, in float3 r, in float t)
{
    float3 cout = to_float3_s(0.0f);
    
    for(int i = 0; i < MAX_IT_VOL; i++)
    {
        float3 q = p;
        float f = smoothstep(0.0f,1.0f,q.z*q.z*0.2f + _cosf(t*0.1f + q.z*0.2f));
        swi2S(q,x,y, mul_f2_mat2(swi2(q,x,y) , rot(-t*0.172f - q.z*f)));
        swi2S(q,x,z, mul_f2_mat2(swi2(q,x,z) , rot(t*0.48f - q.y*f)));
        swi2S(q,x,y, mul_f2_mat2(swi2(q,x,y) , rot(t*0.33f + q.z*f)));

        float3 c1 = to_float3(
            noise(q*1.5f + 0.15f + t),
            noise(q*1.5f + t),
            noise(q*1.5f - 0.15f + t)
            );
        float c2 = noise(q*4.0f + t);
        float c3 = noise(q*8.0f + t);
        float c4 = noise(q*16.0f + t);

#ifdef ALT1
        float3 c = c1*0.6f + c2*0.2f + c3*0.1f + c4*0.1f;
#endif
#ifdef ALT2
        float3 c = c1*0.7f + c2*0.2f + c3*0.1f;
#endif
#ifdef ALT3
        float3 c = c1*0.9f + c2*0.1f;
#endif
#ifdef ALT4
        float3 c = c1;
#endif
        
        cout += c*c*c*c*c*c*c;
        p += r*STEP_SIZE;
    }
float zzzzzzzzzzzzzzzz;    
  return cout/MAX_COL;
}

__KERNEL__ void PlasmaStormJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{

    float2 st = (fragCoord - iResolution*0.5f)/iResolution.y;
    fragColor = to_float4_aw(rmvolum(to_float3(0.0f,0.0f,-2.0f), normalize(to_float3_aw(st,0.17f)), iTime*0.75f), 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}