
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


#define R iResolution
#define A(U) texture(iChannel0, (U)/R)
#define C(U) texture(iChannel2, (U)/R)
#define D(U) texture(iChannel3, (U)/R)

#define N 5.
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer B' to iChannel0


// Calculate forces and pressure
__KERNEL__ void PathsThroughTimeFuse__Buffer_A(float4 Q, float2 U, float4 iMouse, int iFrame)
{

    Q = A(U);
    float4 
        n = A(U+to_float2(0,1)),
        e = A(U+to_float2(1,0)),
        s = A(U-to_float2(0,1)),
        w = A(U-to_float2(1,0));
    swi2(Q,x,y) -= 
        // pressure force
        0.25f*to_float2(e.z-w.z,n.z-s.z)+
        // magnus force
        0.25f*Q.w*to_float2(n.w-s.w,e.w-w.w);
    // divergence
    Q.z  = 0.25f*(s.y-n.y+w.x-e.x+n.z+e.z+s.z+w.z);
    // curl
    Q.w = 0.25f*(s.x-n.x+w.y-e.y);
    
    //Boundary conditions
    float2 JET = to_float2(0.5f+0.25f*_sinf(float(iFrame)/500.0f),0.1f)*R;
    if (iMouse.z>0.0f) JET = swi2(iMouse,x,y);
    swi2(Q,x,y) = _mix(swi2(Q,x,y),to_float2(0,0.7f),0.3f*smoothstep(1.0f,-1.0f,length(U-JET)-6.0f));
    swi2(Q,x,y) = _mix(swi2(Q,x,y),to_float2(0,-0.7f),0.3f*smoothstep(1.0f,-1.0f,length(U-(R-JET))-6.0f));
    if (iFrame<1) Q = to_float4(0);
    if (U.x<4.||U.y<4.||R.x-U.x<4.||R.y-U.y<4.0f)swi3(Q,x,y,w)*=0.0f;


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


// Advect along velocity and curl feild
__KERNEL__ void PathsThroughTimeFuse__Buffer_B(float4 Q, float2 U)
{

    for (float i = 0.0f; i< N;i++) {
        Q = A(U);
        float co = _cosf(Q.w/N), si = _sinf(Q.w/N);
        U -= swi2(Q,x,y)*mat2(co,-si,si,co)/N;
    }
    Q = A(U);


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer C' to iChannel2


void swap (float2 U, float2 r, inout float4 Q) {
  float4 n = C(U+r);
    if (length(U-swi2(n,x,y)) < length(U-swi2(Q,x,y))) Q = n;
}
__KERNEL__ void PathsThroughTimeFuse__Buffer_C(float4 Q, float2 U, int iFrame)
{

    Q = C(U);
    swap(U, to_float2(1,0),Q);
    swap(U, to_float2(0,1),Q);
    swap(U,-to_float2(1,0),Q);
    swap(U,-to_float2(0,1),Q);
    swap(U, to_float2(1,1),Q);
    swap(U,to_float2(-1,1),Q);
    swap(U,to_float2(1,-1),Q);
    swap(U,-to_float2(1,1),Q);
    
    swi2(Q,x,y) += A(swi2(Q,x,y)).xy;
    
    if (iFrame < 1) {
      Q = to_float4(_floor(0.5f+U),0,0);
    }


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel0
// Connect Buffer D 'Previsualization: Buffer C' to iChannel2
// Connect Buffer D 'Previsualization: Buffer D' to iChannel3


__KERNEL__ void PathsThroughTimeFuse__Buffer_D(float4 Q, float2 U)
{

    Q = _mix(D(U),
            to_float4(1)*smoothstep(0.1f+10.0f*length(A(U).xy),0.0f,length(U-C(U).xy))
                              ,0.005f);


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer C' to iChannel2
// Connect Image 'Previsualization: Buffer D' to iChannel3


__KERNEL__ void PathsThroughTimeFuse(float4 Q, float2 U)
{

    Q = 1.0f-_fmaxf(_sinf(3.0f*D(U)*(1.0f+0.2f*to_float4(1,2,3,4))),0.0f);


  SetFragmentShaderComputedColor(Q);
}