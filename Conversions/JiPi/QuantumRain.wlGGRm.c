
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


#define R iResolution
#define A(U) _tex2DVecN(iChannel0,(U).x/R.x,(U).y/R.y,15)
#define B(U) _tex2DVecN(iChannel1,(U).x/R.x,(U).y/R.y,15)
#define C(U) _tex2DVecN(iChannel2,(U).x/R.x,(U).y/R.y,15)
#define D(U) _tex2DVecN(iChannel3,(U).x/R.x,(U).y/R.y,15)
#define E(U) _tex2DVecN(iChannel4,(U).x/R.x,(U).y/R.y,15)

//#define Main void mainImage( out float4 Q, in float2 U )

#define Them float4 n = A(U+to_float2(0,1)), e = A(U+to_float2(1,0)), s = A(U-to_float2(0,1)), w = A(U-to_float2(1,0)), m = 0.25f*(n+e+s+w);
#define dt 1.0f
#define F(Q) 0.5f*( 1.0f/(1.0f+length(Q)+dot(Q,Q)) + 1e-3*(U.y)*(iMouse.z>0.0f?0.0f:1.0f))
#define Loss 0.0f
#define Init if (length(U-0.5f*R) < 10.0f) Q = to_float4(1,1,1,-1);
#define Mouse if (iMouse.z>0.0f&&length(U-swi2(iMouse,x,y))<20.0f) Q += 0.03f*to_float4(1,0,0,-1);
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer D' to iChannel0


__KERNEL__ void QuantumRainFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    U+=0.5f;
    Q = A(U);
    Them;
    
    swi2S(Q,x,z, swi2(Q,x,z) + dt*swi2((m-Q+Q*F(Q)),y,w));
    
    Mouse;
    Init;
    
    SetFragmentShaderComputedColor(Q);        
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void QuantumRainFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    U+=0.5f;
    Q = A(U);
    Them;
    
    swi2S(Q,y,w, swi2(Q,y,w) - dt*swi2((m-Q+Q*F(Q)),x,z));
    Q += Loss*(m-Q);

    SetFragmentShaderComputedColor(Q);            
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


__KERNEL__ void QuantumRainFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    U+=0.5f;
    Q = A(U);
    Them;
    
    swi2S(Q,x,z, swi2(Q,x,z) + dt*swi2((m-Q+Q*F(Q)),y,w));
    
    Mouse;
    Init;
    
    SetFragmentShaderComputedColor(Q);            
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


__KERNEL__ void QuantumRainFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    U+=0.5f;
    Q = A(U);
    Them;
    
    swi2S(Q,y,w, swi2(Q,y,w) - dt*swi2((m-Q+Q*F(Q)),x,z));
    Q += Loss*(m-Q);
    
    SetFragmentShaderComputedColor(Q);            
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: Forest_0' to iChannel2
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void QuantumRainFuse(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    U+=0.5f;
    Them;
    float3 no = normalize(to_float3( length(e)-length(w),
                                     length(n)-length(s), 5)), 
           re = reflect(no,to_float3(0,0,1));
           
    Q = (0.5f+0.5f*decube_f3(iChannel2,re))*
        (sin_f4(_atan2f(length(A(U)), 1.0f)*to_float4(1,2,3,4)));
        
    SetFragmentShaderComputedColor(Q);                
}