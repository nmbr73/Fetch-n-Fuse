
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution

#define Mach_Number 1.0f

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer D' to iChannel0


// Fluid
__DEVICE__ float4 T (float2 U, float2 R, __TEXTURE2D__ iChannel0) { return texture(iChannel0,U/R);}
__DEVICE__ float4 X (float2 U, in float4 C, float2 r, float2 R, __TEXTURE2D__ iChannel0) {
    float4 n = T(U+r,R,iChannel0); // neighbor
    float2 rp = to_float2(-r.y,r.x); // perpiduclar to r
    float2 first_term = r *(n.z-C.z) + // pressure term
                        rp*(n.w*C.w) + // spin term 
                        _mix(swi2(C,x,y),swi2(n,x,y),0.3f); 
                     
    return to_float4( first_term.x,first_term.y,
                      dot(r ,swi2(n,x,y)-swi2(C,x,y))+n.z,   // pressure calculation
                      dot(rp,swi2(n,x,y)-swi2(C,x,y))-(n.w) );// spin calculation
    
}
__KERNEL__ void SuperSonicFuse__Buffer_A(float4 C, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{

   U+=0.5f;
   C = T(U,R,iChannel0);
   float r2 = _sqrtf(2.0f)*0.5f; // without renormalization, the neighborhood needs to be equidistant to the cell
   // calculate the sum of all neighbor interactions
   C = X(U,C,to_float2( 1, 0),R,iChannel0)+ 
       X(U,C,to_float2( 0, 1),R,iChannel0)+
       X(U,C,to_float2(-1, 0),R,iChannel0)+
       X(U,C,to_float2( 0,-1),R,iChannel0)+
       X(U,C,to_float2( r2, r2),R,iChannel0)+
       X(U,C,to_float2(-r2, r2),R,iChannel0)+
       X(U,C,to_float2(-r2,-r2),R,iChannel0)+
       X(U,C,to_float2( r2,-r2),R,iChannel0);
   C /= 8.0f; // divide by the neighborhood size

    
   if (iFrame < 1||U.x < 4.0f||R.x-U.x < 4.0f)
       C = to_float4(Mach_Number,0,0,0);
   if (U.y < 4.||R.y-U.y < 4.0f) C.w = 0.0f;
   if (iFrame < 1) C.x = 0.0f;
   float t = 0.5f+0.3f*_sinf(iTime);
   if (iMouse.z > 0.0f) t = 0.5f+0.3f*(iMouse.y/R.y*2.0f-1.0f);
   float si = _sinf(t), co = _cosf(t);
   mat2 ro = to_mat2(co,-si,si,co);
   U = mul_f2_mat2((U-to_float2(0.25f,0.5f)*R),ro);
   U.x *= 0.1f;
   U.y -= 20.0f*_expf(-3e-2*U.x*U.x);
   if (length(U) < 6.0f)  C.x*=0.0f, C.y*=0.0f;//swi2(C,x,y) *= 0.0f;


  SetFragmentShaderComputedColor(C);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


// Advection Step

//__DEVICE__ float4 T (float2 U) {return texture(iChannel0,U/R);}
__KERNEL__ void SuperSonicFuse__Buffer_B(float4 C, float2 U, float2 iResolution, sampler2D iChannel0)
{

   U+=0.5f;
   #define N 16.
   for (float i = 0.0f; i < N; i+=1.0f)
       U -= swi2(T(U,R,iChannel0),x,y)/N;
   C = T(U,R,iChannel0);

  SetFragmentShaderComputedColor(C);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


// Fluid

/*
__DEVICE__ float4 T (float2 U) { return texture(iChannel0,U/R);}
__DEVICE__ float4 X (float2 U, in float4 C, float2 r) {
  float4 n = T(U+r); // neighbor
    float2 rp = to_float2(-r.y,r.x); // perpiduclar to r
    return to_float4(
           r *(n.z-C.z) + // pressure term
             rp*(n.w*C.w) + // spin term 
           _mix(swi2(C,x,y),swi2(n,x,y),0.3f),//viscous term
        dot(r ,swi2(n,x,y)-swi2(C,x,y))+n.z, // pressure calculation
      dot(rp,swi2(n,x,y)-swi2(C,x,y))-(n.w));// spin calculation
    
}
*/
__KERNEL__ void SuperSonicFuse__Buffer_C(float4 C, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{

   U+=0.5f;
   C = T(U,R,iChannel0);
   float r2 = _sqrtf(2.0f)*0.5f; // without renormalization, the neighborhood needs to be equidistant to the cell
   // calculate the sum of all neighbor interactions
   C = X(U,C,to_float2( 1, 0),R,iChannel0) + 
       X(U,C,to_float2( 0, 1),R,iChannel0)+
       X(U,C,to_float2(-1, 0),R,iChannel0)+
       X(U,C,to_float2( 0,-1),R,iChannel0)+
       X(U,C,to_float2( r2, r2),R,iChannel0)+
       X(U,C,to_float2(-r2, r2),R,iChannel0)+
       X(U,C,to_float2(-r2,-r2),R,iChannel0)+
       X(U,C,to_float2( r2,-r2),R,iChannel0);
   C /= 8.0f; // divide by the neighborhood size
   
    
   if (iFrame < 1||U.x < 4.0f||R.x-U.x < 4.0f)
       C = to_float4(Mach_Number,0,0,0);
   if (U.y < 4.||R.y-U.y < 4.0f) C.w = 0.0f;
   if (iFrame < 1) C.x = 0.0f;
   float t = 0.5f+0.3f*_sinf(iTime);
   if (iMouse.z > 0.0f) t = 0.5f+0.3f*(iMouse.y/R.y*2.0f-1.0f);
   float si = _sinf(t), co = _cosf(t);
   mat2 ro = to_mat2(co,-si,si,co);
   U = mul_f2_mat2((U-to_float2(0.25f,0.5f)*R),ro);
   U.x *= 0.1f;
   U.y -= 20.0f*_expf(-3e-2*U.x*U.x);
   if (length(U) < 6.0f)  C.x*=0.0f, C.y*=0.0f;//swi2(C,x,y) *= 0.0f;

  SetFragmentShaderComputedColor(C);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


// Advection Step

//__DEVICE__ float4 T (float2 U) {return texture(iChannel0,U/R);}

__KERNEL__ void SuperSonicFuse__Buffer_D(float4 C, float2 U, float2 iResolution, sampler2D iChannel0)
{

   U+=0.5f;
   #define N 16.
   for (float i = 0.0f; i < N; i+=1.0f)
       U -= swi2(T(U,R,iChannel0),x,y)/N;
   C = T(U,R,iChannel0);

  SetFragmentShaderComputedColor(C);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: Forest_0' to iChannel2
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer C' to iChannel1

/*
__DEVICE__ float2 _fwidth(float2 inp, float2 iR){
    //simulate fwidth
    float uvx = inp.x + 1.0f/iR.x;
    float ddx = uvx * uvx - inp.x * inp.x;

    float uvy = inp.y + 1.0f/iR.y;
    float ddy = uvy * uvy - inp.y * inp.y;

    return to_float2(_fabs(ddx), _fabs(ddy));
}
*/
__DEVICE__ float _fwidth(float inp, float2 iR){
    //simulate fwidth
    float uvx = inp + 1.0f/iR.x;
    float ddx = uvx * uvx - inp * inp;

    float uvy = inp + 1.0f/iR.y;
    float ddy = uvy * uvy - inp * inp;

    return _fabs(ddx) + _fabs(ddy);
}

//__DEVICE__ float4 T ( float2 U ) {return texture(iChannel0,U/R);}

__KERNEL__ void SuperSonicFuse(float4 C, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel2)
{
    CONNECT_SLIDER5(Alpha, 0.0f, 1.0f, 1.0f);
    CONNECT_SLIDER6(Par1, -10.0f, 10.0f, 1.2f);

    U+=0.5f;
    float4 
        o = T(U,R,iChannel0),
        a = T(U+to_float2(1,0),R,iChannel0),
        b = T(U-to_float2(1,0),R,iChannel0),
        c = T(U+to_float2(0,1),R,iChannel0),
        d = T(U-to_float2(0,1),R,iChannel0);
        
    float4 g = to_float4_f2f2(swi2(a,z,w)-swi2(b,z,w),swi2(c,z,w)-swi2(d,z,w));
    float2 dz = swi2(g,x,z);
    float2 dw = swi2(g,y,w);
    float3 n = normalize(to_float3_aw(dz,0.05f));
    float4 tx = decube_f3(iChannel2,reflect(to_float3(0,0,1),n));
    C = abs_f4(cos_f4(o.w*to_float4(140,162,175,20)))*(0.7f+0.3f*tx);
    float2 u = U;
    for (int i = 0; i < 50; i++) {
        U -= swi2(T(U,R,iChannel0),x,y);
    }
    U.x-= (float)(iFrame)*Mach_Number;
    U = sin_f2(U*0.2f);
    float2 w=to_float2_s(Par1);//_fwidth(U,R)*1.2f;
    //w=to_float2(_fwidth(U.x,R),_fwidth(U.y,R))*Par1;
    
    C *= smoothstep(-w.x,w.x,_fabs(U.x))*smoothstep(-w.y,w.y,_fabs(U.y));
    U = u;
    float t = 0.5f+0.3f*_sinf(iTime);
    if (iMouse.z > 0.0f) t = 0.5f+0.3f*(iMouse.y/R.y*2.0f-1.0f);
    float si = _sinf(t), co = _cosf(t);
    mat2 ro = to_mat2(co,-si,si,co);
    U = mul_f2_mat2((U-to_float2(0.25f,0.5f)*R),ro);
    U.x *= 0.1f;
    U.y -= 20.0f*_expf(-3e-2*U.x*U.x);
    if (length(U) < 6.0f) {
      C = abs_f4(sin_f4(10.0f*o.w+o.z*to_float4(1,2,5,4)));
    }
   C.w=Alpha; 

   SetFragmentShaderComputedColor(C);
}