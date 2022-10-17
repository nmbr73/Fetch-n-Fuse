
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution

#define A(U) _tex2DVecN(iChannel0,(U).x/R.x,(U).y/R.y,15)
#define B(U) _tex2DVecN(iChannel1,(U).x/R.x,(U).y/R.y,15)
#define C(U) _tex2DVecN(iChannel2,(U).x/R.x,(U).y/R.y,15)
#define D(U) _tex2DVecN(iChannel3,(U).x/R.x,(U).y/R.y,15)

// lower to improve frame rate
#define N 6.0f

#define FORCE_RANGE to_float4(0,0,3,3)

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1
// Connect Buffer A 'Previsualization: Buffer C' to iChannel2
// Connect Buffer A 'Previsualization: Buffer D' to iChannel3


// Particle tracking
#ifdef XXX
float2 R;
__DEVICE__ float4 A (float2 U) {return texture(iChannel0,U/R);}
__DEVICE__ float4 B (float2 U) {return texture(iChannel1,U/R);}
__DEVICE__ float4 C (float2 U) {return texture(iChannel2,U/R);}
__DEVICE__ float4 D (float2 U) {return texture(iChannel3,U/R);}
#endif

__DEVICE__ float man (float2 U) {return _fabs(U.x)+_fabs(U.y);}
__DEVICE__ void X (float2 U, inout float4 *Q, float2 u, float2 R, __TEXTURE2D__ iChannel0) {
    float4 p = A(U+u);
    if (length(swi2(p,x,y) - U) < length(swi2(*Q,x,y)-U)) *Q = p;
    
}
__KERNEL__ void AGooeyMessFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
   U+=0.5f;
   Q = A(U);
   float4 c = C(U);
   // measure neighborhood
   for (int _x = -1; _x <= 1; _x++)
     for (int _y = -1; _y <= 1; _y++)
      X(U,&Q,to_float2(_x,_y),R,iChannel0);
    // if neares cell is far, make one up
    if (length(swi2(c,x,y))<0.1 && man(swi2(Q,x,y)-U)>2.0f) swi3S(Q,x,y,w, to_float3_aw(U,sign_f(_floor(3.0f*c.w+0.5f))));
        
    //swi2(Q,x,y) += D(swi2(Q,x,y)).xy;
    Q.x += D(swi2(Q,x,y)).x;
    Q.y += D(swi2(Q,x,y)).y;    

    // init
    if (iFrame < 1) {
        float2 u = U;
        U = _floor(u);
        //swi2(Q,x,y) = U;
        Q.x = U.x;
        Q.y = U.y;
        Q.z = 1.0f;
        Q.w = sign_f((U.y-0.5f*R.y));
    }

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1
// Connect Buffer B 'Previsualization: Buffer C' to iChannel2
// Connect Buffer B 'Previsualization: Buffer D' to iChannel3


// Blur pass 1
#ifdef XXX
float2 R;
__DEVICE__ float4 A (float2 U) {return texture(iChannel0,U/R);}
__DEVICE__ float4 B (float2 U) {return texture(iChannel1,U/R);}
__DEVICE__ float4 C (float2 U) {return texture(iChannel2,U/R);}
#endif

__KERNEL__ void AGooeyMessFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
  U+=0.5f;
  Q = to_float4_s(0);
   for (float i = -N ; i <= N ; i++) {
     float4 a = A(U+to_float2(i,0));
        float4 c = C(U+to_float2(i,0));
        float s = smoothstep(1.1f,0.9f,length(U+to_float2(i,0)-swi2(a,x,y)));
        float4 _x = to_float4(0,0,a.w*s,c.z);
        Q += _x*sqrt_f4(FORCE_RANGE)/FORCE_RANGE*exp_f4(-i*i*0.5f/FORCE_RANGE);
   }

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1
// Connect Buffer C 'Previsualization: Buffer C' to iChannel2
// Connect Buffer C 'Previsualization: Buffer D' to iChannel3


// Blur pass 2
#ifdef XXX
float2 R;
__DEVICE__ float4 A (float2 U) {return texture(iChannel0,U/R);}
__DEVICE__ float4 B (float2 U) {return texture(iChannel1,U/R);}
__DEVICE__ float4 C (float2 U) {return texture(iChannel2,U/R);}
#endif

__KERNEL__ void AGooeyMessFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
  U+=0.5f;
  Q = to_float4_s(0);
   for (float i = -N ; i <= N ; i++) {
     float4 c = B(U+to_float2(0,i));
        Q += c*sqrt_f4(FORCE_RANGE)/FORCE_RANGE*exp_f4(-i*i*0.5f/FORCE_RANGE);
   }
   float4 
        n = C(U+to_float2(0,1)),
        e = C(U+to_float2(1,0)),
        s = C(U+to_float2(0,-1)),
        w = C(U+to_float2(-1,0));
   Q = clamp(Q,-1.0f,1.0f);
   n = C(U+to_float2(0,1));
   e = C(U+to_float2(1,0));
   s = C(U-to_float2(0,1));
   w = C(U-to_float2(1,0));
   //swi2(Q,x,y) = to_float2(e.w-w.w,n.w-s.w);
   Q.x = (e.w-w.w);
   Q.y = (n.w-s.w);

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel0
// Connect Buffer D 'Previsualization: Buffer B' to iChannel1
// Connect Buffer D 'Previsualization: Buffer C' to iChannel2
// Connect Buffer D 'Previsualization: Buffer D' to iChannel3


// Fluid
#ifdef XXX
float2 R;
__DEVICE__ float4 A (float2 U) {return texture(iChannel0,U/R);}
__DEVICE__ float4 B (float2 U) {return texture(iChannel1,U/R);}
__DEVICE__ float4 C (float2 U) {return texture(iChannel2,U/R);}
__DEVICE__ float4 D (float2 U) {return texture(iChannel3,U/R);}
#endif
__DEVICE__ float4 T (float2 U, float2 R, __TEXTURE2D__ iChannel3) {
    return D(U-swi2(D(U),x,y));
}
__KERNEL__ void AGooeyMessFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{

   U+=0.5f;
   Q = T(U,R,iChannel3);
   float4 n,e,s,w;
   n = T(U+to_float2(0,1),R,iChannel3);
   e = T(U+to_float2(1,0),R,iChannel3);
   s = T(U-to_float2(0,1),R,iChannel3);
   w = T(U-to_float2(1,0),R,iChannel3);
   float4 c = C(U);
   // xy : velocity, z : pressure, w : spin
   Q.x -= 0.25f*(e.z-w.z+Q.w*(n.w-s.w));
   Q.y -= 0.25f*(n.z-s.z+Q.w*(e.w-w.w));
   Q.z  = 0.25f*((s.y-n.y+w.x-e.x)+(n.z+e.z+s.z+w.z));
   Q.w  = 0.25f*((n.x-s.x+w.y-e.y)-(n.w+e.w+s.w+w.w));
   swi2S(Q,x,y, _mix(swi2(Q,x,y)*0.999f,0.25f*swi2((n+e+s+w),x,y),0.2f));
   // blurred particles force
   //swi2(Q,x,y) += 0.001f*c.z*swi2(c,x,y);
   Q.x += 0.001f*c.z*c.x;
   Q.y += 0.001f*c.z*c.y;
   // gravity
   Q.y -= c.z/1000.0f;
   if (length(swi2(Q,x,y))>0.5f) swi2S(Q,x,y, 0.5f*normalize(swi2(Q,x,y)));
   // boundary conditions
   if( iFrame < 1) Q = to_float4_s(0);
   if (U.x<7.0f||R.x-U.x<7.0f||U.y<7.0f||R.y-U.y<7.0f) Q.x*=0.8f, Q.y*=0.8f;//swi2(Q,x,y) *= 0.8f;
   float2 m = (U-swi2(iMouse,x,y))/dot(U-swi2(iMouse,x,y),U-swi2(iMouse,x,y));
   if (iMouse.z > 0.0f) swi2S(Q,x,y, clamp(swi2(Q,x,y)+0.2f*(to_float2(-m.y,m.x))*_expf(-0.0001f*length(U-swi2(iMouse,x,y))),to_float2_s(-1),to_float2_s(1)));

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer C' to iChannel2
// Connect Image 'Previsualization: Buffer D' to iChannel3


__DEVICE__ float4 col (float2 U, float2 R, __TEXTURE2D__ iChannel2, __TEXTURE2D__ iChannel3) {
    float4 me = D(U);
    me = D(U);
    float4 c = C(U);
    float3 no = normalize(to_float3_aw(swi2(c,z,y),0.1f));
    return to_float4_s(0.5f)-0.5f*c.z*sin_f4((1.7f+1e2*me.w+me.z)*to_float4(1,2,3,4));

}
__KERNEL__ void AGooeyMessFuse(float4 Q, float2 U, float2 iResolution, sampler2D iChannel2, sampler2D iChannel3)
{

   U+=0.5f;
   Q = col(U,R,iChannel2,iChannel3);

  SetFragmentShaderComputedColor(Q);
}
