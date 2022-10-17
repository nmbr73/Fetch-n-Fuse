
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer B' to iChannel0
// Connect Buffer A 'Previsualization: Buffer D' to iChannel1
// Connect Buffer A 'Textur' to iChannel3

#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


__DEVICE__ float4 TexBlending(float2 uv, float4 C, float blend, float modus, float strength, float2 special, __TEXTURE2D__ iCh )
{
    
  if (blend>0.0f)
  {
    float4 tex = texture(iCh, uv);
    if (tex.w != 0.0f)    
    {
      if ((int)modus & 64)     // Image Einblendung
      {
         C = _mix(C,tex,blend)*strength; 
      }
      else if ((int)modus&128) // BufD Special
      {
         C = _mix(C,to_float4(special.x,special.y,0.0f,0.0f),blend);
      }
      else
      {
        tex = tex*2.0 - 1.0f;
        if ((int)modus & 2)  swi2S(C,x,y, _mix(swi2(C,x,y),swi2(tex,x,y)*strength,blend));// XY
        if ((int)modus & 4)  C = _mix(C,tex,blend)*strength;                              // All  
        if ((int)modus & 8)  C.z = _mix(C.z,tex.z,blend)*strength;                        // Z
        if ((int)modus & 16) C.w = _mix(C.w,tex.x,blend)*strength;                        // ZW_Strength
        if ((int)modus & 32) swi2S(C,z,w, _mix(swi2(C,z,w),to_float2_s(strength),blend)); // Erase
      }
    }  
  } 
  
  return C;
}


// Fluid

__DEVICE__ float ln (float2 p, float2 a, float2 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f));
}
__DEVICE__ float4 P (float2 U, float2 R, __TEXTURE2D__ iChannel1) { // access particle buffer
    return texture(iChannel1,U/R);}
__DEVICE__ float4 t (float2 U, float2 R, __TEXTURE2D__ iChannel0) { // access buffer
  return texture(iChannel0,U/R);
}
__DEVICE__ float4 T (float2 U, float2 R, __TEXTURE2D__ iChannel0) {
    // sample things where they were, not where they are
  U -= 0.5f*swi2(t(U,R,iChannel0),x,y);
  U -= 0.5f*swi2(t(U,R,iChannel0),x,y);
    return t(U,R,iChannel0);
}
__KERNEL__ void DeBroglieParticlesVFuse__Buffer_A(float4 C, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
   CONNECT_CHECKBOX0(Reset, 0); 

   CONNECT_SLIDER1(Blend1, 0.0f, 1.0f, 0.0f);
   CONNECT_SLIDER2(Strength, -1.0f, 10.0f, 1.0f);
   CONNECT_BUTTON0(Modus, 1, ModXY, All, ModZ, ModZW, Erase);
   CONNECT_BUTTON1(BufModus, 1, BufA, BufB, BufC, BufD, Image);

   U+=0.5f;
   C = T(U,R,iChannel0);
   float4 // neighborhood
        n = T(U+to_float2(0,1),R,iChannel0),
        e = T(U+to_float2(1,0),R,iChannel0),
        s = T(U-to_float2(0,1),R,iChannel0),
        w = T(U-to_float2(1,0),R,iChannel0),
         mu = n+e+s+w;
   // xy : velocity, z : pressure, w : spin
   C.x = 0.25f*(n.x+e.x+s.x+w.x+w.z-e.z+s.w*C.w-n.w*C.w);
   C.y = 0.25f*(n.y+e.y+s.y+w.y+s.z-n.z+w.w*C.w-e.w*C.w);
   C.z = 0.25f*(mu.z+s.y-n.y+w.x-e.x);
   C.w = 0.25f*(n.x-s.x+w.y-e.y-mu.w);

   //particle interaction
   float4 p = P(U,R,iChannel1);
   float r = smoothstep(5.0f,0.0f,length(U-abs_f2(swi2(p,x,y))));
   swi2S(C,z,w, _mix(swi2(C,z,w),to_float2(-1,0.5f*sign_f(p.x)),r));
   swi2S(C,x,y, _mix(swi2(C,x,y)*0.999f,swi2(p,z,w),r));
    
   //Blending
   if ((int)BufModus & 2) 
     C = TexBlending(U/R, C, Blend1, Modus, Strength, to_float2_s(0.0f), iChannel3 );       
    
    
   // Boundary Conditions
   if (iMouse.z > 0.0f) C.w = _mix(C.w,4.0f,_expf(-0.1f*dot(U-swi2(iMouse,x,y),U-swi2(iMouse,x,y))));
   if (iFrame < 1 || Reset) C = to_float4_s(0);
   if (U.x < 5.0f||U.y < 5.0f||R.x-U.x < 5.0f||R.y-U.y < 5.0f) C.x*=0.25f,C.z*=0.25f;//swi2(C,x,z)*=0.25f;
   if (U.x < 1.0f||U.y < 1.0f||R.x-U.x < 1.0f||R.y-U.y < 1.0f) C.x*=0.0f,C.z*=0.0f,C.w*=0.0f;//swi3(C,x,z,w)*=0.0f;


  SetFragmentShaderComputedColor(C);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer D' to iChannel1
// Connect Buffer B 'Textur' to iChannel3

// Fluid
/*
__DEVICE__ float ln (float2 p, float2 a, float2 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f));
}
__DEVICE__ float4 P (float2 U) { // access particle buffer
    return texture(iChannel1,U/R);}
__DEVICE__ float4 t (float2 U) { // access buffer
  return texture(iChannel0,U/R);
}
__DEVICE__ float4 T (float2 U) {
    // sample things where they were, not where they are
  U -= 0.5f*t(U).xy;
  U -= 0.5f*t(U).xy;
    return t(U);
}
*/
__KERNEL__ void DeBroglieParticlesVFuse__Buffer_B(float4 C, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
   CONNECT_CHECKBOX0(Reset, 0); 
   
   CONNECT_SLIDER1(Blend1, 0.0f, 1.0f, 0.0f);
   CONNECT_SLIDER2(Strength, -1.0f, 10.0f, 1.0f);
   CONNECT_BUTTON0(Modus, 1, ModXY, All, ModZ, ModZW, Erase);
   CONNECT_BUTTON1(BufModus, 1, BufA, BufB, BufC, BufD, Image);
   
   U+=0.5f;
   C = T(U,R,iChannel0);
   float4 // neighborhood
        n = T(U+to_float2(0,1),R,iChannel0),
        e = T(U+to_float2(1,0),R,iChannel0),
        s = T(U-to_float2(0,1),R,iChannel0),
        w = T(U-to_float2(1,0),R,iChannel0),
         mu = n+e+s+w;
   // xy : velocity, z : pressure, w : spin
   C.x = 0.25f*(n.x+e.x+s.x+w.x+w.z-e.z+s.w*C.w-n.w*C.w);
   C.y = 0.25f*(n.y+e.y+s.y+w.y+s.z-n.z+w.w*C.w-e.w*C.w);
   C.z = 0.25f*(mu.z+s.y-n.y+w.x-e.x);
   C.w = 0.25f*(n.x-s.x+w.y-e.y-mu.w);

   //particle interaction
   float4 p = P(U,R,iChannel1);
   float r = smoothstep(5.0f,0.0f,length(U-abs_f2(swi2(p,x,y))));
   swi2S(C,z,w, _mix(swi2(C,z,w),to_float2(-1,0.5f*sign_f(p.x)),r));
   swi2S(C,x,y, _mix(swi2(C,x,y)*0.999f,swi2(p,z,w),r));
    
    
   //Blending
   if ((int)BufModus & 4) 
     C = TexBlending(U/R, C, Blend1, Modus, Strength, to_float2_s(0.0f), iChannel3 );             
    
    // Boundary Conditions
   if (iMouse.z > 0.0f) C.w = _mix(C.w,4.0f,_expf(-0.1f*dot(U-swi2(iMouse,x,y),U-swi2(iMouse,x,y))));
   if (iFrame < 1 || Reset)      C = to_float4_s(0);
   
   if (U.x < 5.0f||U.y < 5.0f||R.x-U.x < 5.0f||R.y-U.y < 5.0f) C.x*=0.25f,C.z*=0.25f;//swi2(C,x,z)*=0.25f;
   if (U.x < 1.0f||U.y < 1.0f||R.x-U.x < 1.0f||R.y-U.y < 1.0f) C.x*=0.0f,C.z*=0.0f,C.w*=0.0f;//swi3(C,x,z,w)*=0.0f;

  SetFragmentShaderComputedColor(C);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer C' to iChannel0
// Connect Buffer C 'Previsualization: Buffer D' to iChannel1


  
__DEVICE__ float4 T2 ( float2 U, float2 R, __TEXTURE2D__ iChannel0 ) {return texture(iChannel0,U/R);}
//__DEVICE__ float4 P ( float2 U ) {return texture(iChannel1,U/R);}
__KERNEL__ void DeBroglieParticlesVFuse__Buffer_C(float4 C, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    
    CONNECT_SLIDER1(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER2(Strength, -1.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, ModXY, All, ModZ, ModZW, Erase);
    CONNECT_BUTTON1(BufModus, 1, BufA, BufB, BufC, BufD, Image);
    
    U+=0.5f;
    C = T2(U,R,iChannel0);
    float4 p = P(U,R,iChannel1);
    C = _fmaxf(
        C * (to_float4_s(1.0f)-to_float4(0.02f,0.01f,0.03f,0.01f)),
        4.0f*to_float4_s(smoothstep(3.0f,0.0f,length(abs_f2(swi2(p,x,y))-U)))
        );
        
        
    //Blending
    if ((int)BufModus & 8) 
      C = TexBlending(U/R, C, Blend1, Modus, Strength, to_float2_s(0.0f), iChannel3 );          
        
    if (iFrame < 1 || Reset) C = to_float4_s(0);

  SetFragmentShaderComputedColor(C);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel0
// Connect Buffer D 'Previsualization: Buffer D' to iChannel1


// Voronoi based particle tracking

//__DEVICE__ float4 T ( float2 U ) {return texture(iChannel0,U/R);}//sample fluid
//__DEVICE__ float4 P ( float2 U ) {return texture(iChannel1,U/R);}//sample particles
__DEVICE__ void swap (float2 U, inout float4 *Q, float2 u, float2 R, __TEXTURE2D__ iChannel1) {
    float4 p = P(U+u,R,iChannel1);
    float dl = length(U-abs_f2(swi2(*Q,x,y))) - length(U-abs_f2(swi2(p,x,y)));
    *Q = _mix(*Q,p,(float)(dl>=0.0f));
}
__KERNEL__ void DeBroglieParticlesVFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
   CONNECT_CHECKBOX0(Reset, 0); 
   
   CONNECT_CHECKBOX2(BufD_Special, 0); 
   
   CONNECT_SLIDER1(Blend1, 0.0f, 1.0f, 0.0f);
   CONNECT_SLIDER2(Strength, -1.0f, 10.0f, 1.0f);
   CONNECT_BUTTON0(Modus, 1, ModXY, All, ModZ, ModZW, Erase);
   CONNECT_BUTTON1(BufModus, 1, BufA, BufB, BufC, BufD, Image);
   
   
   U+=0.5f;
   Q = P(U,R,iChannel1);
   swap(U,&Q,to_float2(1,0),R,iChannel1);
   swap(U,&Q,to_float2(0,1),R,iChannel1);
   swap(U,&Q,to_float2(0,-1),R,iChannel1);
   swap(U,&Q,to_float2(-1,0),R,iChannel1);
   swap(U,&Q,to_float2(1,1),R,iChannel1);
   swap(U,&Q,to_float2(-1,1),R,iChannel1);
   swap(U,&Q,to_float2(1,-1),R,iChannel1);
   swap(U,&Q,to_float2(-1,-1),R,iChannel1);
   float4 t = T(abs_f2(swi2(Q,x,y)),R,iChannel0);
   float2 e = to_float2(2,0);
   float s = sign_f(Q.x);
   //swi2(Q,x,y) = _fabs(swi2(Q,x,y));
   Q.x = _fabs(Q.x);
   Q.y = _fabs(Q.y);
   float4 
        a = T(swi2(Q,x,y)+swi2(e,x,y),R,iChannel0),
        b = T(swi2(Q,x,y)+swi2(e,y,x),R,iChannel0),
        c = T(swi2(Q,x,y)-swi2(e,x,y),R,iChannel0),
        d = T(swi2(Q,x,y)-swi2(e,y,x),R,iChannel0);
   float2 z = to_float2(a.z-c.z,b.z-d.z);
   float2 w = to_float2(a.w-c.w,b.w-d.w);
   if (s > 0.0f) swi2S(Q,z,w, swi2(Q,z,w) + (-0.5f*z+s*swi2(w,y,x)*to_float2(-1,1)));
   //swi2(Q,z,w) = clamp(swi2(Q,z,w),-0.8f,0.8f);
   Q.z = clamp(Q.z,-0.8f,0.8f);
   Q.w = clamp(Q.w,-0.8f,0.8f);
   
   //swi2(Q,x,y)+=swi2(Q,z,w);
   Q.x+=Q.z;
   Q.y+=Q.w;

   //swi2(Q,x,y) *= s;
   Q.x *= s;
   Q.y *= s;
   
   
   //Blending
   if ((int)BufModus & 16) 
   {
     //float2 special = (length(U-0.5f*R)<0.33f*R.x ? 1.0f: -1.0f) * ceil_f2(U/15.0f-0.5f)*15.0f;
     float2 special = Strength * ceil_f2(U/15.0f-0.5f)*15.0f;
     if (BufD_Special) Modus=128;
     Q = TexBlending(U/R, Q, Blend1, Modus, Strength, special, iChannel3 );  
   }
   
   if (iFrame < 1 || Reset || (iMouse.z > 0.0f&& length(swi2(iMouse,x,y))<40.0f)){
      Q = to_float4_s(-100);
      if (U.x>20.0f&&R.x-U.x>20.0f&&U.y>20.0f&&R.y-U.y>20.0f)
      {
         float2 tmp = (length(U-0.5f*R)<0.33f*R.x ? 1.0f: -1.0f) * ceil_f2(U/15.0f-0.5f)*15.0f;
         Q = to_float4(tmp.x,tmp.y,0,0);
      }   
    }
   
   if (_fabs(Q.x)<10.0f)     Q.z=0.9f*_fabs(Q.z);
   if (R.x-_fabs(Q.x)<10.0f) Q.z=-0.9f*_fabs(Q.z);
   if (_fabs(Q.y)<10.0f)     Q.w=0.9f*_fabs(Q.w);
   if (R.y-_fabs(Q.y)<10.0f) Q.w=-0.9f*_fabs(Q.w);

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: Forest_0' to iChannel2
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer C' to iChannel3
// Connect Image 'Previsualization: Buffer D' to iChannel1
// Connect Image 'Textur' to iChannel4


//__DEVICE__ float4 T ( float2 U ) {return texture(iChannel0,U/R);}
//__DEVICE__ float4 P ( float2 U ) {return texture(iChannel1,U/R);}
__DEVICE__ float4 L ( float2 U,float2 R, __TEXTURE2D__ iChannel3 ) {return texture(iChannel3,U/R);}
__KERNEL__ void DeBroglieParticlesVFuse(float4 C, float2 U, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
    
    //CONNECT_CHECKBOX1(TexturImage, 0); 
    
    CONNECT_SLIDER1(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER2(Strength, -1.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, ModXY, All, ModZ, ModZW, Erase);
    CONNECT_BUTTON1(BufModus, 1, BufA, BufB, BufC, BufD, Image);
    
    CONNECT_SLIDER0(Alpha, 0.0f, 1.0f, 1.0f);
    U+=0.5f;
    float4 
        a = T(U+to_float2(1,0),R,iChannel0),
        b = T(U-to_float2(1,0),R,iChannel0),
        c = T(U+to_float2(0,1),R,iChannel0),
        d = T(U-to_float2(0,1),R,iChannel0),
        e = P(U+to_float2(1,0),R,iChannel1),
        f = P(U-to_float2(1,0),R,iChannel1),
        g = P(U+to_float2(0,1),R,iChannel1),
        h = P(U-to_float2(0,1),R,iChannel1),
        i = P(U+to_float2(1,0),R,iChannel1),
        j = P(U-to_float2(1,0),R,iChannel1),
        k = P(U+to_float2(0,1),R,iChannel1),
        l = P(U-to_float2(0,1),R,iChannel1),
        p = P(U,R,iChannel1);
    float o = 0.0f;
    o += length(abs_f2(swi2(p,x,y))-abs_f2(swi2(e,x,y)));
    o += length(abs_f2(swi2(p,x,y))-abs_f2(swi2(f,x,y)));
    o += length(abs_f2(swi2(p,x,y))-abs_f2(swi2(g,x,y)));
    o += length(abs_f2(swi2(p,x,y))-abs_f2(swi2(h,x,y)));
    o += length(abs_f2(swi2(p,x,y))-abs_f2(swi2(i,x,y)));
    o += length(abs_f2(swi2(p,x,y))-abs_f2(swi2(j,x,y)));
    o += length(abs_f2(swi2(p,x,y))-abs_f2(swi2(k,x,y)));
    o += length(abs_f2(swi2(p,x,y))-abs_f2(swi2(l,x,y)));
    float4 gr = to_float4_f2f2(swi2(a,z,w)-swi2(b,z,w),swi2(c,z,w)-swi2(d,z,w));
    float2 dz = swi2(gr,x,z);
    float2 dw = 10.0f*swi2(gr,y,w);
    float4 v = T(U-dz,R,iChannel0);
    swi3S(C,x,y,z, 0.5f-0.5f*sin_f3(0.005f*o*to_float3(2,1,0)+swi3(L(U,R,iChannel3),x,y,z)-length(swi2(v,x,y))*to_float3(1.2f,1.3f,1.2f)+v.w*to_float3(1,2,3)));
    float3 n = normalize(to_float3_aw(dz+dw,0.01f));
    float4 tx = to_float4(0.6f,0.9f,1,1)*decube_f3(iChannel2,reflect(to_float3(0,0,1),n));
    C = (C*0.8f+0.2f)*(0.9f+0.1f*tx);
    
    //float2 uv, float4 C, float blend, float modus, float strength, __TEXTURE2D__ iCh )
    //if(TexturImage)
    if ((int)BufModus & 32) 
      C = TexBlending(U/R,C, Blend1, 64, Strength, to_float2_s(0.0f), iChannel4);
    
    C.w=Alpha;

  SetFragmentShaderComputedColor(C);
}