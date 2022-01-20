
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define swi2Z(a,b,c,d) (a).b = (d).x, (a).c = (d).y; 
#define swi3Z(a,b,c,d,e) (a).b = (e).x, (a).c = (e).y, (a).d = (e).z; 

#define R iResolution
#define A(U) _tex2DVecN(iChannel0,(U).x/R.x,(U).y/R.y,15)
#define B(U) _tex2DVecN(iChannel1,(U).x/R.x,(U).y/R.y,15)
#define C(U) _tex2DVecN(iChannel2,(U).x/R.x,(U).y/R.y,15)
#define D(U) _tex2DVecN(iChannel3,(U).x/R.x,(U).y/R.y,15)
//#define Main void mainImage(out float4 Q, in float2 U)
#define box for(int _x=-1;_x<=1;_x++)for(int _y=-1;_y<=1;_y++)


// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer B' to iChannel0
// Connect Buffer A 'Previsualization: Buffer C' to iChannel2


// Forces
__KERNEL__ void SedimentationFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
{

    U+=0.5f;
    Q = A(U);
    float4 c = C(U);
    float4 dQ = to_float4_s(0);
    box if(_fabs(_x)!=_fabs(_y)) 
    {
        float2 u = to_float2(_x,_y);
        float4 a = A(U+u);
        float4 c = C(U+u);
        float f = 0.25f*(a.w+c.x);
        swi2Z(Q,x,y,swi2(dQ,x,y) - f*u);
    }
    swi2Z(dQ,x,y, clamp(swi2(dQ,x,y),-1.0f,1.0f));
    Q += dQ;;
    // erosion 
    c.z;
    // sedimentation
    c.w;
    if (Q.w>0.0f)           Q.z = (Q.w*Q.z+c.z-c.w)/Q.w;
    if (U.x<1.0f && _fabs(U.y-0.5f*R.y)<20.0f) Q.w *= 0.0f;
    float2 M = 1.5f*R;
    if (iMouse.z>0.0f)              M = swi2(iMouse,x,y);
    if(length(U-M)<0.02f*R.y)       Q = to_float4((0.1f*normalize(M-0.5f*R)).x,(0.1f*normalize(M-0.5f*R)).y,-1,1.0f);
    if (length(swi2(Q,x,y))>0.5f)   swi2Z(Q,x,y, 0.5f*normalize(swi2(Q,x,y)) );
    if (iFrame < 1)                 Q = to_float4(0,0,0,0.3f);
    


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


// Advect
__KERNEL__ void SedimentationFuse__Buffer_B(float4 Q, float2 U, float2 iResolution)
{
    U+=0.5f;
    Q = A(U);
    float4 dQ = to_float4_s(0);
    box if(_fabs(_x)!=_fabs(_y))
    {
        float2 u = to_float2(_x,_y);
        float4 q = A(U+u);
        float2 a = swi2(Q,x,y),
             b = swi2(q,x,y)+u;
       float ab = dot(u,b-a);
       float i = dot(u,(0.5f*u-a))/ab;
       float wa = 0.5f*Q.w*_fminf(i,0.5f);
       float wb = 0.5f*q.w*_fmaxf(i-0.5f,0.0f);
       swi3Z(dQ,x,y,z, swi3(dQ,x,y,z) + swi3(Q,x,y,z)*wa+swi3(q,x,y,z)*wb);
       dQ.w += wa+wb;
    }
    if (dQ.w>0.0f)   swi3Z(dQ,x,y,z, swi3(dQ,x,y,z) / dQ.w);
    Q = dQ;
    


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Texture: Abstract 3' to iChannel1
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0
// Connect Buffer C 'Previsualization: Buffer C' to iChannel2


__KERNEL__ void SedimentationFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, int iFrame)
{
    U+=0.5f;
    Q = C(U);
    float4 a = A(U);
    
    // erosion
    Q.z = 5e-3*length(swi2(a,x,y))*a.w;
    // sedimentation
    Q.w = 1e-2*_fmaxf(0.5f-length(swi2(a,x,y)),0.0f)*a.w*a.z;
    
    Q.x -= Q.z-Q.w;
    
    if (iFrame < 100)
        Q = to_float4(B(U).x,0,0,0);


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer C' to iChannel2


// Fork of "Water Fall" by wyatt. https://shadertoy.com/view/NtKGWD
// 2022-01-06 18:16:24

// Fork of "Aqua Vista" by wyatt. https://shadertoy.com/view/ssXGDB
// 2021-11-24 02:54:37

// Fork of "Temperatures" by wyatt. https://shadertoy.com/view/fsf3zS
// 2021-03-22 22:23:14

// Fork of "Transport Dynamics II" by wyatt. https://shadertoy.com/view/sdl3RN
// 2021-03-18 22:39:28

// Display
__KERNEL__ void SedimentationFuse(float4 Q, float2 U, float2 iResolution) {


    float4 f = atan_f4(A(U),to_float4_s(1.0f));
    float4 g = atan_f4(C(U),to_float4_s(1.0f));
    Q = (to_float4_s(0.5f)-0.5f*sin_f4(f.z+to_float4(1,2,3,4)))*_fminf(f.w,1.0f);
    float4 C = (0.5f+0.5f*sin_f4(0.3f+2.0f*g.x+to_float4(1,2,3,4)));
    Q = _mix(Q,C,1.0f-1.7f*f.w);


  SetFragmentShaderComputedColor(Q);
}