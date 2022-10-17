
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer C' to iChannel0
// Connect Buffer A 'Previsualization: Buffer D' to iChannel1

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

//vec2 R;
__DEVICE__ float4 T ( float2 U, float2 R, __TEXTURE2D__ iChannel0 ) {return texture(iChannel0,U/R);}
__DEVICE__ float4 P ( float2 U, float2 R, __TEXTURE2D__ iChannel1 ) {return texture(iChannel1,U/R);}
__DEVICE__ float X (float2 U, float2 u, inout float4 *Q, in float2 r, inout float4 mu, float2 R, __TEXTURE2D__ iChannel0 ) {
    float2 V = U + r, v = swi2(T(V,R,iChannel0),x,y);
    float4 t = T (V-v,R,iChannel0);
    swi2S(*Q,x,y, swi2(*Q,x,y) - 0.25f*r*(t.z-(*Q).z-t.w+(*Q).w));
    mu += 0.25f*t;
    return (0.5f*(length(r-v+u)-length(r+v-u))+t.z);
}

__KERNEL__ void ThermalParticlesIvFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
   U+=0.5f;
   float2 R = iResolution;
   float2 u = swi2(T(U,R,iChannel0),x,y), e = to_float2(1,0);
   Q = T(U-u,R,iChannel0);
   float4 p = P(U-u,R,iChannel1),mu=to_float4_s(0);
   Q.z = 0.25f*(X(U,u,&Q,e,mu,R,iChannel0)+X(U,u,&Q,-1.0f*e,mu,R,iChannel0)+X(U,u,&Q,swi2(e,y,x),mu,R,iChannel0)+X(U,u,&Q,-1.0f*swi2(e,y,x),mu,R,iChannel0));
   float r = smoothstep(3.0f,1.0f,length(U-abs_f2(swi2(p,x,y))));
   Q.w =  _mix(0.9f*mu.w,sign_f(p.x),r);
   swi2S(Q,x,y, _mix(swi2(Q,x,y)*0.9f,swi2(p,z,w),r));
   Q.z*= 0.995f;
   if (iMouse.z > 0.0f) Q.w = _mix(Q.w,100.0f,_expf(-0.1f*dot(U-swi2(iMouse,x,y),U-swi2(iMouse,x,y))));
   if (iFrame < 1) Q = to_float4_s(0);
   if (U.x < 1.0f||U.y < 1.0f||R.x-U.x < 1.0f||R.y-U.y < 1.0f) Q.x=0.0f,Q.y=0.0f; //swi2(Q,x,y) = to_float2(0);

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer D' to iChannel1

/*
__DEVICE__ float4 T ( float2 U ) {return texture(iChannel0,U/R);}
__DEVICE__ float4 P ( float2 U ) {return texture(iChannel1,U/R);}
__DEVICE__ float X (float2 U, float2 u, inout float4 Q, in float2 r, inout float4 mu) {
    float2 V = U + r, v = T(V).xy;
    float4 t = T (V-v);
    swi2(Q,x,y) -= 0.25f*r*(t.z-Q.z-t.w+Q.w);
    mu += 0.25f*t;
    return (0.5f*(length(r-v+u)-length(r+v-u))+t.z);
}
*/
__KERNEL__ void ThermalParticlesIvFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
   U+=0.5f;
   float2 R = iResolution;
   float2 u = swi2(T(U,R,iChannel0),x,y), e = to_float2(1,0);
   Q = T(U-u,R,iChannel0);
   float4 p = P(U-u,R,iChannel1),mu=to_float4_s(0);
   Q.z = 0.25f*(X(U,u,&Q,e,mu,R,iChannel0)+X(U,u,&Q,-1.0f*e,mu,R,iChannel0)+X(U,u,&Q,swi2(e,y,x),mu,R,iChannel0)+X(U,u,&Q,-1.0f*swi2(e,y,x),mu,R,iChannel0));
   float r = smoothstep(3.0f,1.0f,length(U-abs_f2(swi2(p,x,y))));
   Q.w =  _mix(0.9f*mu.w,sign_f(p.x),r);
   swi2S(Q,x,y, _mix(swi2(Q,x,y)*0.9f,swi2(p,z,w),r));
   Q.z*= 0.995f;
   if (iMouse.z > 0.0f) Q.w = _mix(Q.w,100.0f,_expf(-0.1f*dot(U-swi2(iMouse,x,y),U-swi2(iMouse,x,y))));
   if (iFrame < 1) Q = to_float4_s(0);
   if (U.x < 1.0f||U.y < 1.0f||R.x-U.x < 1.0f||R.y-U.y < 1.0f) Q.x=0.0f,Q.y=0.0f; //swi2(Q,x,y) = to_float2(0);

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0
// Connect Buffer C 'Previsualization: Buffer D' to iChannel1


/*
__DEVICE__ float4 T ( float2 U ) {return texture(iChannel0,U/R);}
__DEVICE__ float4 P ( float2 U ) {return texture(iChannel1,U/R);}
__DEVICE__ float X (float2 U, float2 u, inout float4 Q, in float2 r, inout float4 mu) {
    float2 V = U + r, v = T(V).xy;
    float4 t = T (V-v);
    swi2(Q,x,y) -= 0.25f*r*(t.z-Q.z-t.w+Q.w);
    mu += 0.25f*t;
    return (0.5f*(length(r-v+u)-length(r+v-u))+t.z);
}
*/
__KERNEL__ void ThermalParticlesIvFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
   U+=0.5f;
   float2 R = iResolution;
   float2 u = swi2(T(U,R,iChannel0),x,y), e = to_float2(1,0);
   Q = T(U-u,R,iChannel0);
   float4 p = P(U-u,R,iChannel1),mu=to_float4_s(0);
   Q.z = 0.25f*(X(U,u,&Q,e,mu,R,iChannel0)+X(U,u,&Q,-1.0f*e,mu,R,iChannel0)+X(U,u,&Q,swi2(e,y,x),mu,R,iChannel0)+X(U,u,&Q,-1.0f*swi2(e,y,x),mu,R,iChannel0));
   float r = smoothstep(3.0f,1.0f,length(U-abs_f2(swi2(p,x,y))));
   Q.w =  _mix(0.9f*mu.w,sign_f(p.x),r);
   swi2S(Q,x,y, _mix(swi2(Q,x,y)*0.9f,swi2(p,z,w),r));
   Q.z*= 0.995f;
   if (iMouse.z > 0.0f) Q.w = _mix(Q.w,100.0f,_expf(-0.1f*dot(U-swi2(iMouse,x,y),U-swi2(iMouse,x,y))));
   if (iFrame < 1) Q = to_float4_s(0);
   if (U.x < 1.0f||U.y < 1.0f||R.x-U.x < 1.0f||R.y-U.y < 1.0f) Q.x=0.0f,Q.y=0.0f; //swi2(Q,x,y) = to_float2(0);

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel0
// Connect Buffer D 'Previsualization: Buffer D' to iChannel1


// Voronoi based particle tracking
/*
float2 R;float N;
__DEVICE__ float4 T ( float2 U ) {return texture(iChannel0,U/R);}//sample fluid
__DEVICE__ float4 P ( float2 U ) {return texture(iChannel1,U/R);}//sample particles
*/
__DEVICE__ void swap (float2 U, inout float4 *Q, float2 u, float2 R, __TEXTURE2D__ iChannel1) {
    float4 p = P(U+u,R,iChannel1);
    float dl = length(U-abs_f2(swi2(*Q,x,y))) - length(U-abs_f2(swi2(p,x,y)));
    *Q = _mix(*Q,p,(float)(dl>=0.0f));
}

__KERNEL__ void ThermalParticlesIvFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
   U+=0.5f;
   float2 R = iResolution;
   Q = P(U,R,iChannel1);
   swap(U,&Q,to_float2(1,0),R,iChannel1);
   swap(U,&Q,to_float2(0,1),R,iChannel1);
   swap(U,&Q,to_float2(0,-1),R,iChannel1);
   swap(U,&Q,to_float2(-1,0),R,iChannel1);
   swap(U,&Q,to_float2(2,0),R,iChannel1);
   swap(U,&Q,to_float2(0,-2),R,iChannel1);
   swap(U,&Q,to_float2(-2,0),R,iChannel1);
   swap(U,&Q,to_float2(0,2),R,iChannel1);
   float4 t = T(swi2(Q,x,y),R,iChannel0);
   float2 e = to_float2(3.5f,0);
   float4 
        a = T(swi2(Q,x,y)+swi2(e,x,y),R,iChannel0),
        b = T(swi2(Q,x,y)+swi2(e,y,x),R,iChannel0),
        c = T(swi2(Q,x,y)-swi2(e,x,y),R,iChannel0),
        d = T(swi2(Q,x,y)-swi2(e,y,x),R,iChannel0);
   swi2S(Q,z,w, swi2(Q,z,w)-0.06f*to_float2(a.z-c.z,b.z-d.z)-0.1f*to_float2(a.w-c.w,b.w-d.w));
   swi2S(Q,z,w, clamp(swi2(Q,z,w),-0.9f,0.9f));
   swi2S(Q,x,y, sign_f2(swi2(Q,x,y))*(abs_f2(swi2(Q,x,y))+swi2(Q,z,w)));
   
   if (iFrame < 1 && U.x>20.0f&&R.x-U.x>20.0f&&U.y>20.0f&&R.y-U.y>20.0f) {
       Q = to_float4(_floor(U.x/10.0f+0.5f)*10.0f,_floor(U.y/10.0f+0.5f)*10.0f,0,0);
    }
   
   if (_fabs(Q.x)<10.0f)       Q.z=0.9f*_fabs(Q.z);
   if (R.x-_fabs(Q.x)<10.0f)   Q.z=-0.9f*_fabs(Q.z);
   if (_fabs(Q.y)<10.0f)       Q.w=0.9f*_fabs(Q.w);
   if (R.y-_fabs(Q.y)<10.0f)   Q.w=-0.9f*_fabs(Q.w);

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: Forest_0' to iChannel2
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer D' to iChannel1

/*
vec2 R;
__DEVICE__ float4 T ( float2 U ) {return texture(iChannel0,U/R);}
__DEVICE__ float4 P ( float2 U ) {return texture(iChannel1,U/R);}
*/
__KERNEL__ void ThermalParticlesIvFuse(float4 C, float2 U, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    U+=0.5f;
    float2 R = iResolution;
    float4 
        a = T(U+to_float2(1,0),R,iChannel0),
        b = T(U-to_float2(1,0),R,iChannel0),
        c = T(U+to_float2(0,1),R,iChannel0),
        d = T(U-to_float2(0,1),R,iChannel0);
        
    float4 g = to_float4_f2f2(swi2(a,z,w)-swi2(b,z,w),swi2(c,z,w)-swi2(d,z,w));
    float2 dz = swi2(g,x,z);
    float2 dw = swi2(g,y,w);
    float4 v = T(U,R,iChannel0);
    float r = smoothstep(0.0f,2.0f,length(U-abs_f2(swi2(P(U,R,iChannel1),x,y))));
    float3 n = normalize(to_float3_aw(dz+dw,0.1f));
    float3 t = swi3(_tex2DVecN(iChannel2,n.x,n.y,15),x,y,z);
    swi3S(C,x,y,z, (t*0.2f+0.9f)*(0.5f+0.5f*(r*2.0f-1.0f)*sin_f3(0.9f+r+v.w+0.6f*v.z*to_float3(1,2,3))));

  SetFragmentShaderComputedColor(C);
}