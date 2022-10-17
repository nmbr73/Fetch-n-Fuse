
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer D' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define YA 6.5f
#define SA -0.7f

//float2 R;
__DEVICE__ float4 T ( float2 U, float2 R, __TEXTURE2D__ iChannel0 ) {return texture(iChannel0,U/R);}
__DEVICE__ float2 st (float p, float n, float p1, float n1, float2 v) {
  return (p*p1+n*n1-p*n1-n*p1)*normalize(v)/dot(v,v);
}
__DEVICE__ float2 fA (float2 U, float Y, float2 R, __TEXTURE2D__ iChannel0) {
  float4 me = T(U,R,iChannel0);
    float2 sf = to_float2_s(0);
    me.z+=1.0f;
    for (int _x = -2; _x <= 2; _x++) {
    for (int _y = -2; _y <= 2; _y++) {
       if (_x==0&&_y==0) continue;
       float2 u = to_float2(_x,_y)*Y;
       float4 o = T(U+u,R,iChannel0);
       o.z+=1.0f;
       sf += st (me.w*me.z,(1.0f-me.w)*me.z,o.w*o.z,(1.0f-o.w)*o.z,u);
    }
    }
    return sf;
}

__KERNEL__ void WeirdScienceIiFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0)
{
   U+=0.5f;

   float2 R = iResolution;
   float2 O = U,A = U+to_float2(1,0),B = U+to_float2(0,1),C = U+to_float2(-1,0),D = U+to_float2(0,-1);
   float4 u = T(U,R,iChannel0), a = T(A,R,iChannel0), b = T(B,R,iChannel0), c = T(C,R,iChannel0), d = T(D,R,iChannel0);
   float4 p = to_float4_s(0.0f);
   float2 g = to_float2_s(0);
   #define I 2
   for (int i = 0; i < I; i++) {
        U -=swi2(u,x,y); A -=swi2(a,x,y); B -=swi2(b,x,y); C -=swi2(c,x,y); D -=swi2(d,x,y); 
        p += to_float4(length(U-A),length(U-B),length(U-C),length(U-D))-1.0f;
        g += to_float2(a.z-c.z,b.z-d.z);
        u = T(U,R,iChannel0);a = T(A,R,iChannel0); b = T(B,R,iChannel0); c = T(C,R,iChannel0); d = T(D,R,iChannel0);
   }   
   Q = u; 
   float4 N = 0.25f*(a+b+c+d);
   Q = mix_f4(Q,N, to_float4(0,0,1,0.01f)); 
   //swi2(Q,x,y) -= g/10.0f/(float)(I); 
   Q.x -= g.x/10.0f/(float)(I); 
   Q.y -= g.y/10.0f/(float)(I); 
   
   Q.z += (p.x+p.y+p.z+p.w)/10.0f;
   
   swi2S(Q,x,y, swi2(Q,x,y) + SA*fA(U,YA,R,iChannel0));
    
   Q.z *= 0.999f;
   
   if (iFrame < 1)                               Q = to_float4_s(0);
   if (length(U-to_float2(0.1f,0.5f)*R) < 2.0f)  Q.x = 0.2f, Q.y = 0.0f, Q.w = 0.6f;//swi3(Q,x,y,w) = to_float3(0.2f,0,0.6f);
   if (U.x<1.0f||U.y<1.0f||R.x-U.x<1.0f||R.y-U.y<1.0f) Q.x=0.0f,Q.y=0.0f,Q.w=0.0f;

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


#define YB  4.5f
#define SB -0.4f

__DEVICE__ float2 fB (float2 U, float Y, float2 R, __TEXTURE2D__ iChannel0) {
  float4 me = T(U,R,iChannel0);
    float2 sf = to_float2_s(0);
    me.z+=1.0f;
    for (int _x = -2; _x <= 2; _x++) {
    for (int _y = -2; _y <= 2; _y++) {
       if (_x==0&&_y==0) continue;
       float2 u = to_float2(_x,_y)*Y;
       float4 o = T(U+u,R,iChannel0);
       o.z+=1.0f;
       sf += st (me.w*me.z,(1.0f-me.w)*me.z,o.w*o.z,(1.0f-o.w)*o.z,u);
    }
    }
    return sf;
}


__KERNEL__ void WeirdScienceIiFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0)
{
   U+=0.5f;

   float2 R = iResolution;
   float2 O = U,A = U+to_float2(1,0),B = U+to_float2(0,1),C = U+to_float2(-1,0),D = U+to_float2(0,-1);
   float4 u = T(U,R,iChannel0), a = T(A,R,iChannel0), b = T(B,R,iChannel0), c = T(C,R,iChannel0), d = T(D,R,iChannel0);
   float4 p = to_float4_s(0.0f);
   float2 g = to_float2_s(0);
   #define I 2
   for (int i = 0; i < I; i++) {
        U -=swi2(u,x,y); A -=swi2(a,x,y); B -=swi2(b,x,y); C -=swi2(c,x,y); D -=swi2(d,x,y); 
        p += to_float4(length(U-A),length(U-B),length(U-C),length(U-D))-1.0f;
        g += to_float2(a.z-c.z,b.z-d.z);
        u = T(U,R,iChannel0);a = T(A,R,iChannel0); b = T(B,R,iChannel0); c = T(C,R,iChannel0); d = T(D,R,iChannel0);
   }   
   Q = u; 
   float4 N = 0.25f*(a+b+c+d);
   Q = mix_f4(Q,N, to_float4(0,0,1,0.01f)); 
   //swi2(Q,x,y) -= g/10.0f/float(I); 
   Q.x -= g.x/10.0f/(float)(I); 
   Q.y -= g.y/10.0f/(float)(I); 
   
   Q.z += (p.x+p.y+p.z+p.w)/10.0f;
   
   swi2S(Q,x,y, swi2(Q,x,y) + SB*fB(U,YB,R,iChannel0));
   
    
   Q.z *= 0.999f;
   
   if (iFrame < 1)                                     Q = to_float4_s(0);
   if (length(U-to_float2(0.1f,0.5f)*R) < 2.0f)        Q.x = 0.2f, Q.y = 0.0f, Q.w = 0.6f;//swi3(Q,x,y,w) = to_float3(0.2f,0,0.6f);swi3(Q,x,y,w) = to_float3(0.2f,0,0.6f);
   if (U.x<1.0f||U.y<1.0f||R.x-U.x<1.0f||R.y-U.y<1.0f) Q.x=0.0f,Q.y=0.0f,Q.w=0.0f;//Q.xyw=to_float3(0,0,0);


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


#define YC 2.0f
#define SC 0.2f

__DEVICE__ float2 fC (float2 U, float Y, float2 R, __TEXTURE2D__ iChannel0) {
  float4 me = T(U,R,iChannel0);
    float2 sf = to_float2_s(0);
    for (int _x = -1; _x <= 1; _x++) {
    for (int _y = -1; _y <= 1; _y++) {
       if (_x==0&&_y==0) continue;
       float2 u = to_float2(_x,_y)*Y;
       float4 o = T(U+u,R,iChannel0);
       sf += st (me.w,(1.0f-me.w),o.w,(1.0f-o.w),u);
    }
    }
    return sf;
}


__KERNEL__ void WeirdScienceIiFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0)
{
   CONNECT_SLIDER1(SizeC, 0.1f, 5.0f, 2.0f);
   CONNECT_CHECKBOX2(PC, 0);
   CONNECT_POINT1(PointC, 0.1f, 0.5f);
   CONNECT_COLOR1(ParC, 0.2f, 0.0f, 0.6f, 0.0f);
  
   U+=0.5f; 

   float2 R = iResolution;
   float2 O = U,A = U+to_float2(1,0),B = U+to_float2(0,1),C = U+to_float2(-1,0),D = U+to_float2(0,-1);
   float4 u = T(U,R,iChannel0), a = T(A,R,iChannel0), b = T(B,R,iChannel0), c = T(C,R,iChannel0), d = T(D,R,iChannel0);
   float4 p = to_float4_s(0.0f);
   float2 g = to_float2_s(0);
   #define I 2
   for (int i = 0; i < I; i++) {
        U -=swi2(u,x,y); A -=swi2(a,x,y); B -=swi2(b,x,y); C -=swi2(c,x,y); D -=swi2(d,x,y); 
        p += to_float4(length(U-A),length(U-B),length(U-C),length(U-D))-1.0f;
        g += to_float2(a.z-c.z,b.z-d.z);
        u = T(U,R,iChannel0);a = T(A,R,iChannel0); b = T(B,R,iChannel0); c = T(C,R,iChannel0); d = T(D,R,iChannel0);
   }   
   Q = u; 
   float4 N = 0.25f*(a+b+c+d);
   Q = mix_f4(Q,N, to_float4(0,0,1,0.01f)); 
   //swi2(Q,x,y) -= g/10.0f/float(I); 
   Q.x -= g.x/10.0f/(float)(I); 
   Q.y -= g.y/10.0f/(float)(I); 
   
   Q.z += (p.x+p.y+p.z+p.w)/10.0f;
   
   swi2S(Q,x,y, swi2(Q,x,y) + SC*fC(U,YC,R,iChannel0));
    
   Q.z *= 0.999f;

   float2 m = to_float2(0.1f,0.5f); 
   if (PC) m = PointC; //-to_float2_s(0.5f);
   
     
   if (iFrame < 1)                               Q = to_float4_s(0);
   //if (length(U-to_float2(0.1f,0.5f)*R) < 2.0f)  Q.x = 0.2f, Q.y = 0.0f, Q.w = 0.6f;//swi3(Q,x,y,w) = to_float3(0.2f,0,0.6f);swi3(Q,x,y,w) = to_float3(0.2f,0,0.6f);swi3(Q,x,y,w) = to_float3(0.2f,0,0.6f);
   if (length(U-m*R) < SizeC)  Q.x=ParC.x,Q.y=ParC.y,Q.w=ParC.z;//  Q.x = 0.2f, Q.y = 0.0f, Q.w = 0.6f;//swi3(Q,x,y,w) = to_float3(0.2f,0,0.6f);
   if (U.x<1.0f||U.y<1.0f||R.x-U.x<1.0f||R.y-U.y<1.0f) Q.x=0.0f,Q.y=0.0f,Q.w=0.0f;//Q.xyw=to_float3(0,0,0);

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


#define YD 1.5f
#define SD 0.4f


__DEVICE__ float2 fD (float2 U, float Y, float2 R, __TEXTURE2D__ iChannel0) {
  float4 me = T(U,R,iChannel0);
    float2 sf = to_float2_s(0);
    for (int _x = -1; _x <= 1; _x++) {
    for (int _y = -1; _y <= 1; _y++) {
       if (_x==0&&_y==0) continue;
       float2 u = to_float2(_x,_y)*Y;
       float4 o = T(U+u,R,iChannel0);
       sf += st (me.w,(1.0f-me.w),o.w,(1.0f-o.w),u);
    }
    }
    return sf;
}


__KERNEL__ void WeirdScienceIiFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, int iFrame, float4 iMouse, sampler2D iChannel0)
{
   CONNECT_CHECKBOX0(Reset, 0);
   
   CONNECT_SLIDER0(SizeD, 0.1f, 5.0f, 2.0f);
   CONNECT_CHECKBOX1(PD, 0);
   CONNECT_POINT0(PointD, 0.1f, 0.5f);
   CONNECT_COLOR0(ParD, 0.2f, 0.0f, 0.6f, 0.0f);
  
   U+=0.5f; 

   float2 R = iResolution;
   float2 O = U,A = U+to_float2(1,0),B = U+to_float2(0,1),C = U+to_float2(-1,0),D = U+to_float2(0,-1);
   float4 u = T(U,R,iChannel0), a = T(A,R,iChannel0), b = T(B,R,iChannel0), c = T(C,R,iChannel0), d = T(D,R,iChannel0);
   float4 p = to_float4_s(0.0f);
   float2 g = to_float2_s(0);
   #define I 2
   for (int i = 0; i < I; i++) {
        U -=swi2(u,x,y); A -=swi2(a,x,y); B -=swi2(b,x,y); C -=swi2(c,x,y); D -=swi2(d,x,y); 
        p += to_float4(length(U-A),length(U-B),length(U-C),length(U-D))-1.0f;
        g += to_float2(a.z-c.z,b.z-d.z);
        u = T(U,R,iChannel0);a = T(A,R,iChannel0); b = T(B,R,iChannel0); c = T(C,R,iChannel0); d = T(D,R,iChannel0);
   }   
   Q = u; 
   float4 N = 0.25f*(a+b+c+d);
   Q = mix_f4(Q,N, to_float4(0,0,1,0.01f)); 
   //swi2(Q,x,y) -= g/10.0f/(float)(I); 
   Q.x -= g.x/10.0f/(float)(I); 
   Q.y -= g.y/10.0f/(float)(I);
   
   Q.z += (p.x+p.y+p.z+p.w)/10.0f;
   
   swi2S(Q,x,y, swi2(Q,x,y) + SD*fD(U,YD,R,iChannel0));
    
   Q.z *= 0.999f;
    
   float2 m = to_float2(0.1f,0.5f); 
   if (PD) m = PointD; //-to_float2_s(0.5f);
   
   
   if (iFrame < 1 || Reset)                           Q = to_float4_s(0);
   //if (length(U-to_float2(0.1f,0.5f)*R) < 2.0f)  Q.x = 0.2f, Q.y = 0.0f, Q.w = 0.6f;//swi3(Q,x,y,w) = to_float3(0.2f,0,0.6f);
   if (length(U-m*R) < SizeD)  Q.x=ParD.x,Q.y=ParD.y,Q.w=ParD.z;//  Q.x = 0.2f, Q.y = 0.0f, Q.w = 0.6f;//swi3(Q,x,y,w) = to_float3(0.2f,0,0.6f);
   if (U.x<1.0f||U.y<1.0f||R.x-U.x<1.0f||R.y-U.y<1.0f) Q.x=0.0f,Q.y=0.0f,Q.w=0.0f;//Q.xyw=to_float3(0,0,0);


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: Forest_0' to iChannel2
// Connect Image 'Previsualization: Buffer A' to iChannel0


__DEVICE__ float4 t (float2 v, float2 R, __TEXTURE2D__ iChannel0) {return texture(iChannel0,v/R);}

__KERNEL__ void WeirdScienceIiFuse(float4 C, float2 U, float2 iResolution, sampler2D iChannel0, sampler2D iChannel2)
{
    U+=0.5f;

    float2 R = iResolution;

    float4 me = t(U,R,iChannel0);
    float2 dw = to_float2(
      t(U+to_float2(1,0),R,iChannel0).w-t(U-to_float2(1,0),R,iChannel0).w,
      t(U+to_float2(0,1),R,iChannel0).w-t(U-to_float2(0,1),R,iChannel0).w
    );
    float3 n = normalize(to_float3_aw(dw,0.05f));
    C = sin_f4(to_float4(1.5f,2.5f,3,4)*me.w*2.0f);
    float l = dot(n,normalize(to_float3(3,1,0)));
    C *= 0.7f+0.4f*(0.5f-0.5f*l)*decube_f3(iChannel2, reflect(to_float3(0,0,-1),n));
    C = (0.8f+0.4f*l)*sqrt_f4(C)*sqrt_f4(sqrt_f4(C));
    
  SetFragmentShaderComputedColor(C);
}