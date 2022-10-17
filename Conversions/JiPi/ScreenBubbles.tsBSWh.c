
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1
// Connect Buffer A 'Previsualization: Buffer C' to iChannel3

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
__DEVICE__ float4 A (float2 U, float2 R, __TEXTURE2D__ iChannel0) {return texture(iChannel0,U/R);}
__DEVICE__ float4 B (float2 U, float2 R, __TEXTURE2D__ iChannel1) {return texture(iChannel1,U/R);}
__DEVICE__ float4 C (float2 U, float2 R, __TEXTURE2D__ iChannel2) {return texture(iChannel2,U/R);}
__DEVICE__ float4 D (float2 U, float2 R, __TEXTURE2D__ iChannel0) {return texture(iChannel0,U/R);}
__DEVICE__ float4 T (float2 U, float2 R, __TEXTURE2D__ iChannel0) {
  return A(U-swi2(A(U, R,iChannel0),x,y), R,iChannel0);
}
__DEVICE__ float ln (float2 p, float2 a, float2 b) {return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f));}
__KERNEL__ void ScreenBubblesFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{

    CONNECT_SLIDER4(RX2, -100.0f, 100.0f, 2.0f);

    U+=0.5f;

    Q = T(U,R,iChannel0);
    float4 b = B(U,R,iChannel1),
        n = T(U+to_float2(0,1),R,iChannel0),
        e = T(U+to_float2(1,0),R,iChannel0),
        s = T(U-to_float2(0,1),R,iChannel0),
        w = T(U-to_float2(1,0),R,iChannel0);
   Q.x -= 0.25f*(e.z-w.z+Q.w*(n.w-s.w));
   Q.y -= 0.25f*(n.z-s.z+Q.w*(e.w-w.w));
   Q.z  = 0.25f*((s.y-n.y+w.x-e.x)+(n.z+e.z+s.z+w.z));
   Q.w  = 0.25f*((s.x-n.x+w.y-e.y)-Q.w);
   float p = smoothstep(2.0f,0.0f,length(U-swi2(b,x,y))-b.z);
   Q.z += 0.03f*p;
   Q.z*=0.975f;
   float4 mo = texture(iChannel3,to_float2_s(0));
   float l = ln(U,swi2(mo,x,y),swi2(mo,z,w));
   if (mo.z > 0.0f && length(swi2(mo,x,y)-swi2(mo,z,w))>0.0f) swi2S(Q,x,y, swi2(Q,x,y) + 0.01f*(swi2(mo,x,y)-swi2(mo,z,w))*smoothstep(40.0f,0.0f,l));

   if (U.x < 2.0f || U.y < 2.0f || R.x-U.x<RX2) swi2S(Q,x,y, _mix(swi2(Q,x,y),to_float2(-0.3f,0),0.99f));
   
   if (iFrame < 1) {
       Q = to_float4(-0.3f,0,0,0);
   }

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1

#ifdef XXX
#define R iResolution
__DEVICE__ float4 A (float2 U) {return texture(iChannel0,U/R);}
__DEVICE__ float4 B (float2 U) {return texture(iChannel1,U/R);}
__DEVICE__ float4 C (float2 U) {return texture(iChannel2,U/R);}
__DEVICE__ float4 D (float2 U) {return texture(iChannel0,U/R);}
#endif
__DEVICE__ void swap (float2 U, inout float4 *_A, float4 _B) {if (length(U-swi2(_B,x,y))-_B.z < length(U-swi2(*_A,x,y))-(*_A).z) *_A = _B;}

__KERNEL__ void ScreenBubblesFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    CONNECT_SLIDER1(RX, -100.0f, 100.0f, 1.0f);
    CONNECT_SLIDER2(QZ1, -1.0f, 1.0f, 0.5f);
    CONNECT_SLIDER3(QZ2, -10.0f, 10.0f, -1.5f);
    
    CONNECT_SLIDER5(ROffset, -1000.0f, 10.0f, -1.5f);

    U+=0.5f;
    
    Q = B(U,R,iChannel1);
    
    swap(U,&Q,B(U+to_float2(0,1),R,iChannel1));
    swap(U,&Q,B(U+to_float2(1,0),R,iChannel1));
    swap(U,&Q,B(U-to_float2(0,1),R,iChannel1));
    swap(U,&Q,B(U-to_float2(1,0),R,iChannel1));
    swap(U,&Q,B(U+to_float2(2,2),R,iChannel1));
    swap(U,&Q,B(U+to_float2(2,-2),R,iChannel1));
    swap(U,&Q,B(U-to_float2(2,-2),R,iChannel1));
    swap(U,&Q,B(U-to_float2(2,2),R,iChannel1));
    
    swi3S(Q,x,y,w, swi3(Q,x,y,w) + swi3(A(swi2(Q,x,y), R,iChannel0),x,y,w)*to_float3(1,1,6.2f));
    
    //if (R.x-U.x < 1.0f && mod_f((float)(iFrame) , 20.0f) == 1.0f) {
    if (R.x-U.x < RX && mod_f((float)(iFrame) , 20.0f) == 1.0f) {
        float _y = round((U.y+5.0f)/10.0f)*10.0f-5.0f;
        Q = to_float4(
                      R.x+ROffset, _y,
                      0.5f+QZ1*_sinf(_y+(_y+0.45f)*mod_f((float)(iFrame),1e3)), 0.0f
                     );
       Q.z = QZ2*_logf(1e-4+Q.z);
    }
    
   if (iFrame < 1) {
       Q = to_float4_s(0);
   }

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer C' to iChannel2


__KERNEL__ void ScreenBubblesFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, float4 iMouse, sampler2D iChannel2)
{

    float4 p = texture(iChannel2,U/iResolution);
     if (iMouse.z>0.0f) {
      if (p.z>0.0f) Q =  to_float4_f2f2(swi2(iMouse,x,y),swi2(p,x,y));
      else Q =  to_float4_f2f2(swi2(iMouse,x,y),swi2(iMouse,x,y));
     }else Q = to_float4_f2f2(-iResolution,-iResolution);

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'Previsualization: Buffer C' to iChannel2

#ifdef XXX
#define R iResolution
__DEVICE__ float ln (float3 p, float3 a, float3 b) {return length(p-a-(b-a)*dot(p-a,b-a)/dot(b-a,b-a));}
__DEVICE__ float4 A (float2 U) {return texture(iChannel0,U/R);}
__DEVICE__ float4 B (float2 U) {return texture(iChannel1,U/R);}
__DEVICE__ float4 C (float2 U) {return texture(iChannel2,U/R);}
#endif

__DEVICE__ float lnI (float3 p, float3 a, float3 b) {return length(p-a-(b-a)*dot(p-a,b-a)/dot(b-a,b-a));}

__KERNEL__ void ScreenBubblesFuse(float4 Q, float2 U, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{

    CONNECT_COLOR0(Color, 1.0f, 2.0f, 3.0f, 4.0f);
    CONNECT_SLIDER0(AW, -1.0f, 50.0f, 10.0f);
    CONNECT_SLIDER1(AZ, -1.0f, 1.0f, 0.3f);

    U+=0.5f;

    float3 light = to_float3_aw(2.5f*R,1e5);
    float3 me    = to_float3_aw(U,0);
    float3 r = to_float3_aw(U,100);
    float4 a = A(U,R,iChannel0);
    float4 b = B(U,R,iChannel1);
    float 
          n = A(U+to_float2(0,1),R,iChannel0).z,
          e = A(U+to_float2(1,0),R,iChannel0).z,
          s = A(U-to_float2(0,1),R,iChannel0).z,
          w = A(U-to_float2(1,0),R,iChannel0).z;
    float3 no = normalize(to_float3(e-w,n-s,-2.0f));
    float3 li = reflect((r-light),no);
    float o = lnI(me,r,li);
    float2 u = U-swi2(b,x,y);
    u = mul_f2_mat2(u , to_mat2(_cosf(b.w),-_sinf(b.w),_sinf(b.w),_cosf(b.w)));
    //Q = abs_f4(sin_f4(10.0f*a.w+0.3f*a.z*to_float4(1,2,3,4)+smoothstep(4.0f,3.0f,_fabs(length(U-swi2(b,x,y))-b.z))));
    Q = abs_f4(sin_f4(AW*a.w+AZ*a.z*Color+smoothstep(4.0f,3.0f,_fabs(length(U-swi2(b,x,y))-b.z))));
    Q *= smoothstep(-1.0f,-2.0f,length(U-swi2(b,x,y))-b.z);
    Q += _expf(-2.0f*o);
    Q *= 0.8f;

  SetFragmentShaderComputedColor(Q);
}