
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

//__DEVICE__ float sign_f(float value) { if (value == 0.0f) return 0.0f; return value > 0.0f ? 1.0f : -1.0f;}
//__DEVICE__ float4 sin_f4(float4 i) {float4 r; r.x = _sinf(i.x); r.y = _sinf(i.y); r.z = _sinf(i.z); r.w = _sinf(i.w); return r;}

#define R iResolution
#define A(U) _tex2DVecN(iChannel0,(U.x)/R.x,(U.y)/R.y,15)
#define B(U) _tex2DVecN(iChannel1,(U.x)/R.x,(U.y)/R.y,15)
//#define Main void mainImage(out float4 Q, in float2 U)
#define box for(int _x=-1;_x<=1;_x++)for(int _y=-1;_y<=1;_y++)
#define r2 0.70710678118
// oneshade:
//https://www.shadertoy.com/view/7sKSRh

__DEVICE__ float erf(in float x) {
    float std = 0.0f;  //What a Hack :-)
    x *= std;
    //return sign(x) * _sqrtf(1.0f - _expf(-1.239192f * x * x));
    return sign_f(x) * _sqrtf(1.0f - _exp2f(-1.787776f * x * x)); // likely faster version by @spalmer
}
__DEVICE__ float erfstep (float a, float b, float x) {
    return 0.5f*(erf(b-x)-erf(a-x));
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect 'Buffer B' to iChannel0
// Connect 'Buffer D' to iChannel1


// Forces
__KERNEL__ void InAirFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float iTime, float4 iMouse, int iFrame)
{

    U+=0.5f;
    
    Q = A(U);
    float4 dQ = to_float4_s(0);
    box if(_fabs(_x)!=_fabs(_y))
    {
        float2 u = to_float2(_x,_y);
        float4 a = A((U+u)),b=B((U+u));
        float f = 0.05f*(a.w*(a.w-0.8f)+b.w);
        //swi2(dQ,x,y) -= f*u;
        dQ.x -= f*u.x;
        dQ.y -= f*u.y;
    }
    Q += dQ;
    Q.y -= 0.2f/R.y;
    Q = clamp(Q,-2.0f,2.0f);
    float2 M = 1.5f*R;
    if (iMouse.z>0.0f)              M = swi2(iMouse,x,y);
    if(length(U-M)<0.02f*R.y)       Q = to_float4(0,0,0.0f*_sinf(iTime),1.0f);
    if (length(swi2(Q,x,y))>0.5f) { float2 Qxy = 0.5f*normalize(swi2(Q,x,y)); Q.x=Qxy.x;Q.y=Qxy.y;}
    if (iFrame < 1)                 Q = to_float4_s(0);
    if (U.x<3.&&U.y>0.9f*R.y)       Q.w=1.0f;
    
    if (R.x-U.x<3.0f&&U.y>0.9f*R.y)   Q.w=1.0f;
    if (U.x<1.0f||U.y<1.0f||R.y-U.y<1.0f) Q.x*=0.0f, Q.y*=0.0f;
    if(R.x-U.x<1.0f)                  Q.x*=0.0f, Q.y*=0.0f;


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect 'Buffer A' to iChannel0


// Advect
__KERNEL__ void InAirFuse__Buffer_B(float4 Q, float2 U, float2 iResolution)
{
    U+=0.5f;

    Q = A(U);
    float4 dQ = to_float4_s(0);
    box if(_fabs(_x)!=_fabs(_y))
    {
        float2 u = to_float2(_x,_y);
        float4 q = A((U+u));
        float2 a = swi2(Q,x,y),
               b = swi2(q,x,y)+u;
       float  ab = dot(u,b-a);
       float   i = dot(u,(0.5f*u-a))/ab;
       {
           float j = 1.0f;
           float k = 1.0f;
           float wa = 0.25f*Q.w*_fminf(i,j)/j;
           float wb = 0.25f*q.w*_fmaxf(k+i-1.0f,0.0f)/k;
            dQ.w += wa+wb;
      }
      {
           float j = 1.0f;
           float k = 1.0f;
           float wa = 0.25f*Q.w*_fminf(i,j)/j;
           float wb = 0.25f*q.w*_fmaxf(k+i-1.0f,0.0f)/k;
           float3 dQxyz = swi3(dQ,x,y,z) + swi3(Q,x,y,z)*wa+swi3(q,x,y,z)*wb;
           dQ.x=dQxyz.x;dQ.y=dQxyz.y;dQ.z=dQxyz.z;
      }
        
    }
    if (dQ.w>0.0f)  dQ.x/=dQ.w,dQ.y/=dQ.w,dQ.z/=dQ.w;
    Q = dQ;
    


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect 'Buffer B' to iChannel1
// Connect 'Buffer D' to iChannel0


// Forces
__KERNEL__ void InAirFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, int iFrame)
{
    U+=0.5f;
   
    Q = A(U);
    float4 dQ = to_float4_s(0);
    box if(_fabs(_x)!=_fabs(_y))
    {
        float2 u = to_float2(_x,_y);
        float4 a = A((U+u)),b=B((U+u));
        float f = 0.1f*(a.w+b.w);
        //swi2(dQ,x,y) -= f*u;
        dQ.x -= f*u.x;
        dQ.y -= f*u.y;
    }
    Q += dQ;
    Q = clamp(Q,-2.0f,2.0f);
    if (length(swi2(Q,x,y))>0.5f)    { float2 Qxy = 0.5f*normalize(swi2(Q,x,y)); Q.x=Qxy.x;Q.y=Qxy.y;}
                                     
    if (iFrame < 1)                   Q = to_float4(0,0,0,0.1f);
    if (U.x<1.0f||U.y<1.0f||R.y-U.y<1.0f) Q.x*=0.0f, Q.y*=0.0f;
    if(R.x-U.x<1.0f)                  Q.x*=0.0f, Q.y*=0.0f;


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect 'Buffer C' to iChannel0


// Advect
__KERNEL__ void InAirFuse__Buffer_D(float4 Q, float2 U, float2 iResolution)
{
    U+=0.5f;

    Q = A(U);
    float4 dQ = to_float4_s(0);
    box if(_fabs(_x)!=_fabs(_y))
    {
        float2 u = to_float2(_x,_y);
        float4 q = A((U+u));
        float2 a = swi2(Q,x,y),
             b = swi2(q,x,y)+u;
       float ab = dot(u,b-a);
       float i = dot(u,(0.5f*u-a))/ab;
       float j = 0.55f;
       float k = 0.55f;
       float wa = 0.25f*Q.w*_fminf(i,j)/j;
       float wb = 0.25f*q.w*_fmaxf(k+i-1.0f,0.0f)/k;
       float3 dQxyz = swi3(dQ,x,y,z) + swi3(Q,x,y,z)*wa+swi3(q,x,y,z)*wb;
       dQ.x=dQxyz.x;dQ.y=dQxyz.y;dQ.z=dQxyz.z;
       
       dQ.w += wa+wb;
        
    }
    if (dQ.w>0.0f)   dQ.x/=dQ.w,dQ.y/=dQ.w,dQ.z/=dQ.w;
    Q = dQ;
    


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect 'Buffer A' to iChannel0
// Connect 'Buffer C' to iChannel1


// Fork of "Water Fall" by wyatt. https://shadertoy.com/view/NtKGWD
// 2021-12-30 04:31:49

__KERNEL__ void InAirFuse(float4 Q, float2 U, float2 iResolution)
{

    float4 f = A(U),b=B(U);
    float4 m = to_float4_s(0);
    box if(_x!=0 && _y!=0)  { m+=1.0f/4.0f*A((U+to_float2(_x,_y)));}
    Q = _fmaxf(to_float4_s(1.0f)-30.0f*to_float4_s((f-m).w),to_float4_s(0.0f));
    Q -= f.w*(to_float4_s(1.0f)-sin_f4((1.0f+0.5f*f.z)*to_float4(0.5f,0.7f,1,1)));
    Q -= 0.3f*swi4(b,w,w,w,w);

float zzzzzzzzzzzzzzzzzzzz;
  SetFragmentShaderComputedColor(Q);
}