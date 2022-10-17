
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer C' to iChannel0
// Connect Buffer A 'Previsualization: Buffer D' to iChannel2

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution

#define swi4S(a,b,c,d,e,f) {float4 tmp = f; (a).b = tmp.x; (a).c = tmp.y; (a).d = tmp.z; (a).e = tmp.w;}

//Fluid Algorithm 
//float2 R;
__DEVICE__ float ln (float2 p, float2 a, float2 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f));
}
//__DEVICE__ float4 T ( float2 U ) {return texture(iChannel0,U/R);}
#define T(uv) _tex2DVecN(iChannel0, (uv).x/(R).x, (uv).y/(R).y, 15)

__DEVICE__ float X (float2 U, float2 u, inout float4 *Q, in float2 r, float2 R, __TEXTURE2D__ iChannel0) {

    float2 V = U + r, v = swi2(T(V),x,y);
    float4 t = T(V-v);
    swi2S(*Q,x,y, swi2(*Q,x,y) - 0.25f*r*(t.z-(*Q).z+(*Q).w*t.w*(t.w-(*Q).w)));
    return 0.5f*(length(r-v+u)-length(r+v-u))+t.z;
}

__KERNEL__ void WeirdScienceIiiFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel2)
{
   U+=0.5f;
   float2 u = swi2(T(U),x,y), e = to_float2(1,0);
   float P = 0.0f; Q = T(U-u);
   Q.z = 0.25f*(
       X (U,u,&Q, swi2(e,x,y),R,iChannel0)+
       X (U,u,&Q,-1.0f*swi2(e,x,y),R,iChannel0)+
       X (U,u,&Q, swi2(e,y,x),R,iChannel0)+
       X (U,u,&Q,-1.0f*swi2(e,y,x),R,iChannel0)
    );
   float l = length(swi2(Q,x,y));if(l>0.0f)    swi2S(Q,x,y, _mix(swi2(Q,x,y),Q.w*swi2(Q,x,y)/l,0.0001f));
   if (iFrame < 1){if (length(U-0.5f*R)<7.0f)  Q=to_float4_s(1); else Q = to_float4_s(0);}
   float4 mo = texture(iChannel2,to_float2_s(0));
   l = ln(U,swi2(mo,x,y),swi2(mo,z,w));
   if (mo.z > 0.0f && l < 5.0f)                swi4S(Q,x,y,z,w, swi4(Q,x,y,z,w) + to_float4((5.0f-l)*((mo.x)-(mo.z))/700.0f,(5.0f-l)*((mo.y)-(mo.w))/700.0f,0,1.0f-Q.w));

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer D' to iChannel2


//Fluid Algorithm 
#ifdef XXX
float2 R;
__DEVICE__ float ln (float2 p, float2 a, float2 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f));
}
__DEVICE__ float4 T ( float2 U ) {return texture(iChannel0,U/R);}
__DEVICE__ float X (float2 U, float2 u, inout float4 Q, in float2 r) {
    float2 V = U + r, v = T(V).xy;
    float4 t = T (V-v);
    swi2(Q,x,y) -= 0.25f*r*(t.z-Q.z+Q.w*t.w*(t.w-Q.w));
    return 0.5f*(length(r-v+u)-length(r+v-u))+t.z;
}
#endif

__KERNEL__ void WeirdScienceIiiFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel2)
{
   U+=0.5f;
float BBBBBBBBBBBBBBBBBBBBBBBBB;   
   float2 u = swi2(T(U),x,y), e = to_float2(1,0);
   float P = 0.0f; Q = T(U-u);
   Q.z = 0.25f*(
       X (U,u,&Q, swi2(e,x,y),R,iChannel0)+
       X (U,u,&Q,-1.0f*swi2(e,x,y),R,iChannel0)+
       X (U,u,&Q, swi2(e,y,x),R,iChannel0)+
       X (U,u,&Q,-1.0f*swi2(e,y,x),R,iChannel0)
    );
   float l = length(swi2(Q,x,y));if(l>0.0f)   swi2S(Q,x,y, _mix(swi2(Q,x,y),Q.w*swi2(Q,x,y)/l,0.0001f));
   if (iFrame < 1){if (length(U-0.5f*R)<7.0f) Q = to_float4_s(1); else Q = to_float4_s(0);}
   float4 mo = texture(iChannel2,to_float2_s(0));
   l = ln(U,swi2(mo,x,y),swi2(mo,z,w));
   if (mo.z > 0.0f && l < 5.0f)               swi4S(Q,x,y,z,w, swi4(Q,x,y,z,w) + to_float4((5.0f-l)*((mo.x)-(mo.z))/700.0f,(5.0f-l)*((mo.y)-(mo.w))/700.0f,0,1.0f-Q.w));
 
  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0
// Connect Buffer C 'Previsualization: Buffer D' to iChannel2


//Fluid Algorithm 
#ifdef XXX
float2 R;
__DEVICE__ float ln (float2 p, float2 a, float2 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f));
}
__DEVICE__ float4 T ( float2 U ) {return texture(iChannel0,U/R);}
__DEVICE__ float X (float2 U, float2 u, inout float4 Q, in float2 r) {
    float2 V = U + r, v = T(V).xy;
    float4 t = T (V-v);
    swi2(Q,x,y) -= 0.25f*r*(t.z-Q.z+Q.w*t.w*(t.w-Q.w));
    return 0.5f*(length(r-v+u)-length(r+v-u))+t.z;
}
#endif

__KERNEL__ void WeirdScienceIiiFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel2)
{
   U+=0.5f;
float CCCCCCCCCCCCCCCCCCCCCCCCCCCCCC;   
   float2 u = swi2(T(U),x,y), e = to_float2(1,0);
   float P = 0.0f; Q = T(U-u);
   Q.z = 0.25f*(
       X (U,u,&Q, swi2(e,x,y),R,iChannel0)+
       X (U,u,&Q,-1.0f*swi2(e,x,y),R,iChannel0)+
       X (U,u,&Q, swi2(e,y,x),R,iChannel0)+
       X (U,u,&Q,-1.0f*swi2(e,y,x),R,iChannel0)
    );
   float l = length(swi2(Q,x,y));if(l>0.0f)    swi2S(Q,x,y, _mix(swi2(Q,x,y),Q.w*swi2(Q,x,y)/l,0.0001f));
   if (iFrame < 1){if (length(U-0.5f*R)<7.0f)  Q=to_float4_s(1); else Q = to_float4_s(0);}
   float4 mo = texture(iChannel2,to_float2_s(0));
   l = ln(U,swi2(mo,x,y),swi2(mo,z,w));
   if (mo.z > 0.0f && l < 5.0f) swi4S(Q,x,y,z,w, swi4(Q,x,y,z,w) + to_float4((5.0f-l)*((mo.x)-(mo.z))/700.0f,(5.0f-l)*((mo.y)-(mo.w))/700.0f,0,1.0f-Q.w));

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer D' to iChannel0


// keep track of mouse
__KERNEL__ void WeirdScienceIiiFuse__Buffer_D(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
    fragCoord+=0.5f; 
     
    float4 p = texture(iChannel0,fragCoord/iResolution);
    if (iMouse.z>0.0f) {
        if (p.z>0.0f) fragColor =  to_float4_f2f2(swi2(iMouse,x,y),swi2(p,x,y));
      else fragColor =  to_float4_f2f2(swi2(iMouse,x,y),swi2(iMouse,x,y));
    }
    else fragColor = to_float4_f2f2(-iResolution,-iResolution);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: St Peters Basilica_0' to iChannel1
// Connect Image 'Previsualization: Buffer A' to iChannel0

//__DEVICE__ float4 T ( float2 U ) {return texture(iChannel0,U/R);}
__KERNEL__ void WeirdScienceIiiFuse(float4 C, float2 U, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

    U+=0.5f;
    
    float4 
        a = T(U+to_float2(1,0)),
        b = T(U-to_float2(1,0)),
        c = T(U+to_float2(0,1)),
        d = T(U-to_float2(0,1));
        
    float4 g = to_float4_f2f2(swi2(a,z,w)-swi2(b,z,w),swi2(c,z,w)-swi2(d,z,w));
    float2 dz = swi2(g,x,z);
    float2 dw = swi2(g,y,w);
    float4 v = T(U-10.0f*dz);
    swi3S(C,x,y,z, abs_f3(sin_f3(v.z*v.z+0.5f+5.0f*(v.w-length(dw))*to_float3(1.1f,1.2f,1.3f))));
    float3 n = normalize(to_float3_aw(dz,0.05f));
    float4 tx = decube_f3(iChannel1,reflect(to_float3(0,0,1),n));
    C = (n.x*0.3f+0.9f)*(C)*(0.9f+0.1f*tx);
    C *= sqrt_f4(C);

  SetFragmentShaderComputedColor(C);
}