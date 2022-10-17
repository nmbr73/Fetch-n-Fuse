
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer B' to iChannel0

#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define k 0.25f
#define l 0.7f
#define m 0.0f

__DEVICE__ float4 T ( float2 U, float2 R, __TEXTURE2D__ iChannel0 ) {return texture(iChannel0,U/R);}

__KERNEL__ void NavierStokesJipiFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0)
{
   CONNECT_CHECKBOX0(Reset, 0);
   CONNECT_CHECKBOX1(Textur, 0);
   U+=0.5f;
  
   //R = iResolution;
   
   float2 O = U,A = U+to_float2(1,0),B = U+to_float2(0,1),C = U+to_float2(-1,0),D = U+to_float2(0,-1);
   float4 u,a,b,c,d;
   float ds = 0.0f;
   float2 g = to_float2_s(0);
   float2 vdv = to_float2_s(0);
   
   float s = 0.5f;
   
   u = T(U,R,iChannel0); U -= swi2(u,x,y)*s;
   a = T(A,R,iChannel0); b = T(B,R,iChannel0); c = T(C,R,iChannel0); d = T(D,R,iChannel0);
   A -=swi2(a,x,y)*s; B -=swi2(b,x,y)*s; C -=swi2(c,x,y)*s; D -=swi2(d,x,y)*s;     
   g += to_float2(a.z-c.z,b.z-d.z);
   //vdv += to_float2(u.x*(c.x-a.x)+u.y*(d.x-b.x), u.x*(c.y-a.y)+u.y*(d.y-b.y))/2.0f;   
   Q = T(U,R,iChannel0);
   float4 N = 0.25f*(a+b+c+d);
   float div = 0.25f*((c.x-a.x)+(d.y-b.y));
   Q.z = N.z-k*div;
   //swi2(Q,x,y) += g*l;
   Q.x += g.x*l;
   Q.y += g.y*l;
   //swi2(Q,x,y) -= vdv*m;
   //Q *= 0.9999f;
   if (iFrame < 1 || Reset)                                                                 Q = to_float4(0.5f,0,0,0);
   
   if (Textur)
   {
     float tex = texture(iChannel1,U/R).w;
     if (tex>0.0f) Q.x=0.0f,Q.y=0.0f,Q.w=1.0f;//swi3S(Q,x,y,w, to_float3_aw(swi2(Q,x,y)*0.5f+0.5f*to_float2(-1.0f,0),1.0f));
   }
   else{
     if (length(U-to_float2(0.075f,0.15f)*R) < 4.0f)                                          swi3S(Q,x,y,w, to_float3_aw(swi2(Q,x,y)*0.5f+0.5f*to_float2(-1.0f,0),1.0f));
     if (length(U-to_float2(0.075f,0.5f)*R) < 16.0f)                                          Q.x=0.0f,Q.y=0.0f,Q.w=1.0f;//Q.xyw=to_float3(0.0f,0.0f,1.0f);
     if (length(U-to_float2(0.115f,0.75f)*R)+ length(U-to_float2(0.055f,0.82f)*R)< R.x/12.0f) Q.x=0.0f,Q.y=0.0f,Q.w=1.0f;//Q.xyw=to_float3(0.0f,0.0f,1.0f);
   }
   
   if (U.x<1.0f||U.y<1.0f||R.x-U.x<1.0f||R.y-U.y<1.0f)                                      Q.x=0.5f,Q.y=0.0f;//Q.xy=to_float2(0.5f,0);

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


//#define k 0.25
//#define l .7
//#define m 0.0
//float2 R;
//__DEVICE__ float4 T ( float2 U ) {return texture(iChannel0,U/R);}
__KERNEL__ void NavierStokesJipiFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0)
{
   CONNECT_CHECKBOX0(Reset, 0);
   CONNECT_CHECKBOX1(Textur, 0);
   CONNECT_BUTTON0(Modus, 0, BTN1,BTN2,BTN3,BTN4,BTN5);
   U+=0.5f;
   //R = iResolution;
   
   float2 O = U,A = U+to_float2(1,0),B = U+to_float2(0,1),C = U+to_float2(-1,0),D = U+to_float2(0,-1);
   float4 u,a,b,c,d;
   float ds = 0.0f;
   float2 g = to_float2_s(0);
   float2 vdv = to_float2_s(0);
   
   float s = 0.5f;
   
   u = T(U,R,iChannel0); U -= swi2(u,x,y)*s;
   a = T(A,R,iChannel0); b = T(B,R,iChannel0); c = T(C,R,iChannel0); d = T(D,R,iChannel0);
   A -=swi2(a,x,y)*s; B -=swi2(b,x,y)*s; C -=swi2(c,x,y)*s; D -=swi2(d,x,y)*s;     
   g += to_float2(a.z-c.z,b.z-d.z);
   //vdv += to_float2(u.x*(c.x-a.x)+u.y*(d.x-b.x), u.x*(c.y-a.y)+u.y*(d.y-b.y))/2.0f;   
   Q = T(U,R,iChannel0);
   float4 N = 0.25f*(a+b+c+d);
   float div = 0.25f*((c.x-a.x)+(d.y-b.y));
   Q.z = N.z-k*div;
   //   swi2(Q,x,y) += g*l;
   Q.x += g.x*l;
   Q.y += g.y*l;
   
   //swi2(Q,x,y) -= vdv*m;
   //Q *= 0.9999f;
   if (iFrame < 1 || Reset)                                                                 Q = to_float4(0.5f,0,0,0);
   
      if (Textur)
   {
     float4 tex = texture(iChannel1,U/R);
     if (tex.w>0.0f) 
      {
        //if (Modus = 1) Q.x=0.0f,Q.y=0.0f,Q.w=1.0f;//swi3S(Q,x,y,w, to_float3_aw(swi2(Q,x,y)*0.5f+0.5f*to_float2(-1.0f,0),1.0f));
        //if (Modus = 2) swi3S(Q,x,y,w, to_float3_aw(swi2(Q,x,y)*0.5f+0.5f*to_float2(-1.0f,0),1.0f));
        //if (Modus = 3) Q.x=tex.x,Q.y=tex.y,Q.w=tex.z;//swi3S(Q,x,y,w, to_float3_aw(swi2(Q,x,y)*0.5f+0.5f*to_float2(-1.0f,0),1.0f));
      }
   }
   else{
     if (length(U-to_float2(0.075f,0.15f)*R) < 4.0f)                                          swi3S(Q,x,y,w, to_float3_aw(swi2(Q,x,y)*0.5f+0.5f*to_float2(-1.0f,0),1.0f));
     if (length(U-to_float2(0.075f,0.5f)*R) < 16.0f)                                          Q.x=0.0f,Q.y=0.0f,Q.w=1.0f;//Q.xyw=to_float3(0.0f,0.0f,1.0f);
     if (length(U-to_float2(0.115f,0.75f)*R)+ length(U-to_float2(0.055f,0.82f)*R)< R.x/12.0f) Q.x=0.0f,Q.y=0.0f,Q.w=1.0f;//Q.xyw=to_float3(0.0f,0.0f,1.0f);
   }
   
   if (U.x<1.0f||U.y<1.0f||R.x-U.x<1.0f||R.y-U.y<1.0f)                                      Q.x=0.5f,Q.y=0.0f;//Q.xy=to_float2(0.5f,0);

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


//based on https://www.shadertoy.com/view/4lyyzc by wyatt

//__DEVICE__ float4 T ( float2 U ) {return texture(iChannel0,U/R);}
__KERNEL__ void NavierStokesJipiFuse(float4 C, float2 U, float2 iResolution, sampler2D iChannel0)
{
    CONNECT_COLOR0(Color, 1.0f, 2.0f, 3.0f, 1.0f);
    
    
    CONNECT_SLIDER0(Par1, -10.0f, 10.0f, 1.5f);
    CONNECT_SLIDER1(Par2, -10.0f, 10.0f, 5.0f);
    CONNECT_POINT0(NDZ, 0.0f, 0.0f);
    CONNECT_POINT1(DZ, 0.0f, 0.0f);
    
    U+=0.5f;
    //R = iResolution;
    
    float4 v = T(U,R,iChannel0);
    float2 dz = swi2(v,x,y)-(to_float2(0.5f,0)+DZ);
    //swi3S(C,x,y,z, _fmaxf(to_float3_s(0),sin_f3(1.5f+5.0f*(v.w)*to_float3(1,2,3))));
    swi3S(C,x,y,z, _fmaxf(to_float3_s(0),sin_f3(Par1+Par2*(v.w)*swi3(Color,x,y,z))));
    float ndz = length(dz);
    C *= ((0.7f+NDZ.x)+(0.5f+NDZ.y)*ndz);
    C.w=Color.w;

  SetFragmentShaderComputedColor(C);
}