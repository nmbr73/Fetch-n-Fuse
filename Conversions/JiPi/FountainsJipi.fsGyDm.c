
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
//#define Main void mainImage(out float4 Q, in float2 U)
#define box for(int x=-1;x<=1;x++)for(int y=-1;y<=1;y++)
#define r2 0.70710678118f
// oneshade:
//https://www.shadertoy.com/view/7sKSRh

__DEVICE__ float erf(in float x) {
    float std = 0.0f;

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
// Connect Buffer A 'Previsualization: Buffer B' to iChannel0
// Connect Buffer A 'Previsualization: Buffer D' to iChannel1


// Forces
__KERNEL__ void FountainsJipiFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float iTime, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    
    U+=0.5f;

    Q = A(U);
    float4 dQ = to_float4_s(0);
    box if(_fabs(x)!=_fabs(y))
    {
        float2 u = to_float2(x,y);
        float4 a = A(U+u),b=B(U+u);
        float f = 0.05f*(a.w*(a.w-0.8f)+0.2f*b.w);
        //swi2(dQ,x,y) -= f*u;
        dQ.x -= f*u.x;
        dQ.y -= f*u.y;
    }
    Q += dQ;
    Q.y -= 0.1f/R.y;
    Q = clamp(Q,-2.0f,2.0f);
    float2 M = 1.5f*R;
    if (iMouse.z>0.0f)            M = swi2(iMouse,x,y);
    if(length(U-M)<0.02f*R.y)     Q = to_float4(0,0,0.0f*_sinf(iTime),1.0f);
    if (length(swi2(Q,x,y))>0.5f) swi2S(Q,x,y, 0.5f*normalize(swi2(Q,x,y)));
    if (iFrame < 1 || Reset)      Q = to_float4_s(0);
    
    if (_fabs(U.x-0.5f*R.x)<0.005f*R.x&&U.y<0.3f*R.y)
        Q.x = 0.0f,Q.w = 0.2f, Q.y = 1.0f;
    if (_fabs(U.x-0.25f*R.x)<0.005f*R.x&&U.y<0.2f*R.y)
        Q.x = 0.0f,Q.w = 0.2f, Q.y = 1.0f;
    if (_fabs(U.x-0.75f*R.x)<0.005f*R.x&&U.y<0.2f*R.y)
        Q.x = 0.0f,Q.w = 0.2f, Q.y = 1.0f;
    
    if (U.x<1.0f||U.y<1.0f||R.y-U.y<1.0f) Q.x*=0.0f,Q.y*=0.0f;//swi2(Q,x,y)*=0.0f;
    if (R.x-U.x<1.0f)                     Q.x*=0.0f,Q.y*=0.0f;//swi2(Q,x,y)*=0.0f;

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


// Advect
__KERNEL__ void FountainsJipiFuse__Buffer_B(float4 Q, float2 U, float2 iResolution)
{
  
    U+=0.5f;
    
    Q = A(U);
    float4 dQ = to_float4_s(0);
    box if(_fabs(x)!=_fabs(y))
    {
      float2 u = to_float2(x,y);
      float4 q = A(U+u);
      float2 a = swi2(Q,x,y),
             b = swi2(q,x,y)+u;
      float ab = dot(u,b-a);
      float i = dot(u,(0.5f*u-a))/ab;
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
         //swi3S(dQ,x,y,z, swi3(dQ,x,y,z) + swi3(Q,x,y,z)*wa+swi3(q,x,y,z)*wb);
         
         dQ.x +=  Q.x*wa+q.x*wb;
         dQ.y +=  Q.y*wa+q.y*wb;
         dQ.z +=  Q.z*wa+q.z*wb;
         
      }
        
    }
    if (dQ.w>0.0f)   dQ.x/=dQ.w, dQ.y/=dQ.w, dQ.z/=dQ.w;  //dQ.xyz/=dQ.w;
    Q = dQ;

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1
// Connect Buffer C 'Previsualization: Buffer D' to iChannel0


// Forces
__KERNEL__ void FountainsJipiFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0);
  
    U+=0.5f;
    
    Q = A(U);
    float4 dQ = to_float4_s(0);
    box if(_fabs(x)!=_fabs(y))
    {
      float2 u = to_float2(x,y);
      float4 a = A(U+u),b=B(U+u);
      float f = 0.1f*(a.w+b.w);
      //swi2(dQ,x,y) -= f*u;
      dQ.x -= f*u.x;
      dQ.y -= f*u.y;
    }
    Q += dQ;
    Q = clamp(Q,-2.0f,2.0f);
    if (length(swi2(Q,x,y))>0.5f)  swi2S(Q,x,y, 0.5f*normalize(swi2(Q,x,y)));
    if (iFrame < 1 || Reset)       Q = to_float4(0,0,0,0.1f);
    if (U.x<1.0f||U.y<1.0f||R.y-U.y<1.0f) Q.x*=0.0f,Q.y*=0.0f;//swi2(Q,x,y)*=0.0f;
    if(R.x-U.x<1.0f)                      Q.x*=0.0f,Q.y*=0.0f;//swi2(Q,x,y)*=0.0f;

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


// Advect
__KERNEL__ void FountainsJipiFuse__Buffer_D(float4 Q, float2 U, float2 iResolution)
{
    U+=0.5f;
    
    Q = A(U);
    float4 dQ = to_float4_s(0);
    box if(_fabs(x)!=_fabs(y))
    {
        float2 u = to_float2(x,y);
        float4 q = A(U+u);
        float2 a = swi2(Q,x,y),
             b = swi2(q,x,y)+u;
       float ab = dot(u,b-a);
       float i = dot(u,(0.5f*u-a))/ab;
       float j = 0.6f;
       float k = 0.6f;
       float wa = 0.25f*Q.w*_fminf(i,j)/j;
       float wb = 0.25f*q.w*_fmaxf(k+i-1.0f,0.0f)/k;
       //swi3S(dQ,x,y,z, swi3(dQ,x,y,z) + swi3(Q,x,y,z)*wa+swi3(q,x,y,z)*wb);
       dQ.x +=  Q.x*wa+q.x*wb;
       dQ.y +=  Q.y*wa+q.y*wb;
       dQ.z +=  Q.z*wa+q.z*wb;

       
       dQ.w += wa+wb;
    }
    if (dQ.w>0.0f)  dQ.x/=dQ.w, dQ.y/=dQ.w, dQ.z/=dQ.w;  //dQ.xyz/=dQ.w;
    Q = dQ;

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer C' to iChannel1


// Fork of "In Air" by wyatt. https://shadertoy.com/view/7tVXDw
// 2022-06-09 22:26:36

// Fork of "Water Fall" by wyatt. https://shadertoy.com/view/NtKGWD
// 2021-12-30 04:31:49

__KERNEL__ void FountainsJipiFuse(float4 Q, float2 U, float2 iResolution)
{
    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);
  
    U+=0.5f;

    float4 f = A(U),b=B(U);
    Q = 0.6f*swi4(f,w,w,w,w)*to_float4(1,2,3,4) + (Color-0.5f)*f.w;
    
    Q.w = Color.w;

  SetFragmentShaderComputedColor(Q);
}