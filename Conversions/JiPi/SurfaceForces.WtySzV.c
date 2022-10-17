
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)
#define Neighborhood float4 n = A(U+to_float2(0,1)), e = A(U+to_float2(1,0)), s = A(U-to_float2(0,1)), w = A(U-to_float2(1,0)), m = 0.25f*(n+e+s+w);
#define Neighborhood2 float4 N = B(U+to_float2(0,1)), E = B(U+to_float2(1,0)), S = B(U-to_float2(0,1)), W = B(U-to_float2(1,0)), M = 0.25f*(n+e+s+w);
#define Neighborhood3 n = D(U+to_float2(0,1)), e = D(U+to_float2(1,0)), s = D(U-to_float2(0,1)), w = D(U-to_float2(1,0)), m = 0.25f*(n+e+s+w);
#define grd 0.25f*to_float2(e.w-w.w,n.w-s.w)
#define grdx 0.25f*to_float2(e.x-w.x,n.x-s.x)
#define grdy 0.25f*to_float2(e.y-w.y,n.y-s.y)
#define grdz 0.25f*to_float2(e.z-w.z,n.z-s.z)
#define grd2 0.25f*to_float2(E.x-W.x,N.x-S.x)
#define div 0.25f*(e.x-w.x+n.y-s.y)

#define I 20.0f
#define loop for (float i = -I; i <= I; i++)
#define std to_float4(16,8,4,1)
#define gau(x) 0.3989422804f/std*exp_f4(-x*x/std/std)
#define Input if ((iMouse.z>0.0f&&length(U-swi2(iMouse,x,y))<30.0f)||(iFrame<1&&length(U-0.5f*R)<26.0f))
#define Border if (U.x<1.0f||U.y<1.0f||R.x-U.x<1.0f||R.y-U.y<1.0f)
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel1
// Connect Buffer A 'Previsualization: Buffer B' to iChannel0
// Connect Buffer A 'Previsualization: Buffer D' to iChannel3


// Fluid step 1 : d/dt V = d/dx P

__KERNEL__ void SurfaceForcesFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float iTime, float4 iMouse, int iFrame)
{
    U+=0.5f;

    float3 f = swi3(A(U),x,y,z);
    Q = B(U-0.5f*swi2(B(U-0.5f*swi2(B(U),x,y)),x,y));
    Neighborhood;
    swi2S(Q,x,y, swi2(Q,x,y) - grd);  
    Neighborhood3;
    float2 g = grdx*(3.0f*f.y-f.z)+grdy*(3.0f*f.z-f.x)+grdz*(3.0f*f.x-f.y);
    
    //swi2(Q,x,y) += 0.05f*g;
    Q.x += 0.05f*g.x;
    Q.y += 0.05f*g.y;
    
    //swi2(Q,x,y) *= 0.99f;
    Q.x *= 0.99f;
    Q.y *= 0.99f;
    Border Q.x*=0.0f,Q.y*=0.0f; //swi2(Q,x,y) *= 0.0f;

  if (iFrame < 3) Q = to_float4_s(0);

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1
// Connect Buffer B 'Previsualization: Buffer D' to iChannel3


// Fluid step 2 : d/dt P = d/dx V

__KERNEL__ void SurfaceForcesFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, float iTime, float4 iMouse, int iFrame)
{
    U+=0.5f;

    Q = B(U-0.5f*swi2(A(U-0.5f*swi2(A(U),x,y)),x,y));
    Neighborhood;
    Q.w  -= div;
    Neighborhood2;
    swi3S(Q,x,y,z, swi3(Q,x,y,z) - 0.25f*swi3((N*n.y-S*s.y+E*e.x-W*w.x),x,y,z));
    if (length(swi3(Q,x,y,z))>0.0f) 
        swi3S(Q,x,y,z, _mix(swi3(Q,x,y,z),normalize(swi3(Q,x,y,z)),0.01f));
    if (iFrame < 1) Q = to_float4_s(0);
    Input
        swi3S(Q,x,y,z, sin_f3(to_float3(1,2,3)*iTime));
    Border Q.x = -1.0f;
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1


// gaussian blur pass 1
__KERNEL__ void SurfaceForcesFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, float iTime, float4 iMouse, int iFrame)
{
  U+=0.5f;

  Q = to_float4_s(0);
  loop Q += gau(i) * B(U+to_float2(i,0));

  //if (iFrame < 1) Q = to_float4_s(0);
  
  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel0
// Connect Buffer D 'Previsualization: Buffer B' to iChannel1
// Connect Buffer D 'Previsualization: Buffer C' to iChannel2
// Connect Buffer D 'Previsualization: Buffer D' to iChannel3


// gaussian blur pas2
__KERNEL__ void SurfaceForcesFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, float iTime, float4 iMouse, int iFrame)
{
  U+=0.5f;
  Q = 0.1f*D(U);
  loop Q += gau(i) * C(U+to_float2(0,i));

  //if (iFrame < 1) Q = to_float4_s(0);

  SetFragmentShaderComputedColor(Q);  
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer B' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1


// Fork of "Surface Forces" by wyatt. https://shadertoy.com/view/3tKXWm
// 2020-03-01 23:03:05

// Fork of "Last Fluid" by wyatt. https://shadertoy.com/view/3tcSDj
// 2020-02-28 04:10:38

__DEVICE__ float ln (float3 p, float3 a, float3 b) {return length(p-a-(b-a)*dot(p-a,b-a)/dot(b-a,b-a));}

__KERNEL__ void SurfaceForcesFuse(float4 Q, float2 U, float2 iResolution, float iTime, float4 iMouse, int iFrame)
{
    U+=0.5f;

    float4 b = B(U);
    Neighborhood;
    
    //float4 n = B(U+to_float2(0,1)), e = B(U+to_float2(1,0)), s = B(U-to_float2(0,1)), w = B(U-to_float2(1,0)), m = 0.25f*(n+e+s+w);
    
    float3 no = normalize(0.3f*swi3(b,x,y,z)+to_float3_aw(grdx+grdy+grdz,2)),
           re = reflect(no,to_float3(0,0,1));
    
    swi3S(Q,x,y,z, abs_f3(to_float3_s(0.5f)+mul_f3_mat3(0.8f*swi3(b,x,y,z) , to_mat3(0.6f,0.7f,0.7f,0.3f,0.6f,0.1f,-0.4f,0.9f,0.1f))));
    
    float l = ln(to_float3(0.5f,0.5f,6)*swi3(R,x,y,y),to_float3_aw(U,b.x),to_float3_aw(U,b.x)+re);
    Q *= 0.5f*_expf(-0.001f*l)+0.5f*_expf(-0.01f*l)+_expf(-0.1f*l);
    
  SetFragmentShaderComputedColor(Q);    
}