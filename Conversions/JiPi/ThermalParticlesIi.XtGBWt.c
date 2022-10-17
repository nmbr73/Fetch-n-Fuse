
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer C' to iChannel0
// Connect Buffer A 'Previsualization: Buffer D' to iChannel1


//vec2 R;
#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


// Fluid
__DEVICE__ float4 T (float2 U, float2 R, __TEXTURE2D__ iChannel0) { return texture(iChannel0,U/R);}
__DEVICE__ float4 P (float2 U, float2 R, __TEXTURE2D__ iChannel1) { return texture(iChannel1,U/R);}

__DEVICE__ float X (float2 U, float2 u, inout float4 *Q, in float2 r, float2 R, __TEXTURE2D__ iChannel0) {
    float2 V = U + r, v = swi2(T(V,R,iChannel0),x,y);
    float4 t = T (V-v,R,iChannel0);
    swi2S(*Q,x,y, swi2(*Q,x,y) - 0.25f*r*(t.z-(*Q).z+t.w-(*Q).w)); 
    return (0.5f*(length(r-v+u)-length(r+v-u))+t.z);
}

__KERNEL__ void ThermalParticlesIiFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
   CONNECT_CHECKBOX0(Reset, 0);
   
   U+=0.5f; 
   //R = iResolution;
   float2 u = swi2(T(U,R,iChannel0),x,y), e = to_float2(1,0);
   Q = T(U-u,R,iChannel0);
   float4 p = P(U-u,R,iChannel1);
   Q.z = 0.25f*(X(U,u,&Q,e,R,iChannel0)+X(U,u,&Q,-e,R,iChannel0)+X(U,u,&Q,swi2(e,y,x),R,iChannel0)+X(U,u,&Q,-1.0f*swi2(e,y,x),R,iChannel0));
   float r = smoothstep(6.0f,0.0f,length(U-abs_f2(swi2(p,x,y))));
   Q.w =  sign_f(p.x)*r;
   swi2S(Q,x,y,  _mix(swi2(Q,x,y),0.02f*swi2(p,z,w),r));
   //swi2(Q,x,y) *= 0.9f;
   Q.x*=0.9f;Q.y*=0.9f;
   Q.z  *= 0.995f;
   
   if (iMouse.z > 0.0f) Q.w = _mix(Q.w,4.0f,_expf(-0.1f*dot(U-swi2(iMouse,x,y),U-swi2(iMouse,x,y))));
   if (iFrame < 1|| Reset) Q = to_float4_s(0);
   if (U.x < 1.0f||U.y < 1.0f||R.x-U.x < 1.0f||R.y-U.y < 1.0f) Q.x=0.0f,Q.y=0.0f;//swi2(Q,x,y) = to_float2(0);

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
__DEVICE__ float X (float2 U, float2 u, inout float4 Q, in float2 r) {
    float2 V = U + r, v = T(V).xy;
    float4 t = T (V-v);
    swi2(Q,x,y) -= 0.25f*r*(t.z-Q.z+t.w-Q.w); 
    return (0.5f*(length(r-v+u)-length(r+v-u))+t.z);
}
*/
__KERNEL__ void ThermalParticlesIiFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
   CONNECT_CHECKBOX0(Reset, 0);
   
   U+=0.5f;
   float2 u = swi2(T(U,R,iChannel0),x,y), e = to_float2(1,0);
   Q = T(U-u,R,iChannel0);
   float4 p = P(U-u,R,iChannel1);
   Q.z = 0.25f*(X(U,u,&Q,e,R,iChannel0)+X(U,u,&Q,-e,R,iChannel0)+X(U,u,&Q,swi2(e,y,x),R,iChannel0)+X(U,u,&Q,-1.0f*swi2(e,y,x),R,iChannel0));
   float r = smoothstep(6.0f,0.0f,length(U-abs_f2(swi2(p,x,y))));
   Q.w =  sign_f(p.x)*r;
   swi2S(Q,x,y, _mix(swi2(Q,x,y),0.02f*swi2(p,z,w),r));
   //swi2(Q,x,y) *= 0.9f;
   Q.x*=0.9f;Q.y*=0.9f;
   Q.z  *= 0.995f;
   if (iMouse.z > 0.0f) Q.w = _mix(Q.w,4.0f,_expf(-0.1f*dot(U-swi2(iMouse,x,y),U-swi2(iMouse,x,y))));
   if (iFrame < 1 || Reset) Q = to_float4_s(0);
   if (U.x < 1.0f||U.y < 1.0f||R.x-U.x < 1.0f||R.y-U.y < 1.0f) Q.x=0.0f,Q.y=0.0f;//swi2(Q,x,y) = to_float2(0);

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
__DEVICE__ float X (float2 U, float2 u, inout float4 Q, in float2 r) {
    float2 V = U + r, v = T(V).xy;
    float4 t = T (V-v);
    swi2(Q,x,y) -= 0.25f*r*(t.z-Q.z+t.w-Q.w); 
    return (0.5f*(length(r-v+u)-length(r+v-u))+t.z);
}
*/

__KERNEL__ void ThermalParticlesIiFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
   CONNECT_CHECKBOX0(Reset, 0);
   
   U+=0.5f;
   float2 u = swi2(T(U,R,iChannel0),x,y), e = to_float2(1,0);
   Q = T(U-u,R,iChannel0);
   float4 p = P(U-u,R,iChannel1);
   Q.z = 0.25f*(X(U,u,&Q,e,R,iChannel0)+X(U,u,&Q,-e,R,iChannel0)+X(U,u,&Q,swi2(e,y,x),R,iChannel0)+X(U,u,&Q,-1.0f*swi2(e,y,x),R,iChannel0));
   float r = smoothstep(6.0f,0.0f,length(U-abs_f2(swi2(p,x,y))));
   Q.w =  sign_f(p.x)*r;
   swi2S(Q,x,y, _mix(swi2(Q,x,y),0.02f*swi2(p,z,w),r));
   //swi2(Q,x,y) *= 0.9f;
   Q.x*=0.9f;Q.y*=0.9f;
   Q.z  *= 0.995f;
   if (iMouse.z > 0.0f) Q.w = _mix(Q.w,4.0f,_expf(-0.1f*dot(U-swi2(iMouse,x,y),U-swi2(iMouse,x,y))));
   if (iFrame < 1 || Reset) Q = to_float4_s(0);
   if (U.x < 1.0f||U.y < 1.0f||R.x-U.x < 1.0f||R.y-U.y < 1.0f) Q.x=0.0f,Q.y=0.0f;//swi2(Q,x,y) = to_float2(0);

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
__KERNEL__ void ThermalParticlesIiFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
  CONNECT_CHECKBOX0(Reset, 0);
  CONNECT_CHECKBOX1(Textur, 0);
  
  CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
  CONNECT_SLIDER1(BlendZ_Thr, 0.0f, 10.0f, 1.0f);
  CONNECT_SLIDER2(BlendW_Thr, 0.0f, 10.0f, 1.0f);
  CONNECT_SLIDER3(BlendS_Thr, -10.0f, 10.0f, 0.0f);
  CONNECT_BUTTON0(Modus, 1, Z,  W, Particle, PS);
  
   U+=0.5f;
   Q = P(U,R,iChannel1);
   swap(U,&Q,to_float2(1,0),R,iChannel1);
   swap(U,&Q,to_float2(0,1),R,iChannel1);
   swap(U,&Q,to_float2(0,-1),R,iChannel1);
   swap(U,&Q,to_float2(-1,0),R,iChannel1);
   swap(U,&Q,to_float2(3,0),R,iChannel1);
   swap(U,&Q,to_float2(0,3),R,iChannel1);
   swap(U,&Q,to_float2(0,-3),R,iChannel1);
   swap(U,&Q,to_float2(-3,0),R,iChannel1);
   float4 t = T(swi2(Q,x,y),R,iChannel0);
   float2 e = to_float2(2,0);
   float4 
        a = T(swi2(Q,x,y)+swi2(e,x,y),R,iChannel0),
        b = T(swi2(Q,x,y)+swi2(e,y,x),R,iChannel0),
        c = T(swi2(Q,x,y)-swi2(e,x,y),R,iChannel0),
        d = T(swi2(Q,x,y)-swi2(e,y,x),R,iChannel0);
   swi2S(Q,z,w, swi2(Q,z,w)*0.999f-to_float2(a.z-c.z,b.z-d.z)-5.0f*to_float2(a.w-c.w,b.w-d.w));
   swi2S(Q,x,y, sign_f2(swi2(Q,x,y))*(abs_f2(swi2(Q,x,y)+0.02f*swi2(Q,z,w))));
   
   if(Reset) Q = to_float4_s(0.0f);
   
   if ((iFrame < 1 || Reset) && U.x>20.0f&&R.x-U.x>20.0f&&U.y>20.0f&&R.y-U.y>20.0f) 
   {
      Q = to_float4_s(0.0f);
       
      float tex = 1.0f;
       
      if(Textur)
        tex = texture(iChannel2, U/R).w;
         
      if (tex)      
        Q = to_float4(_floor(U.x/10.0f+0.5f)*10.0f,_floor(U.y/10.0f+0.5f)*10.0f,0,0);
   }
   

    //Textureblending
    if (Blend1 > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(iChannel2,U/R);

      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2)
          Q.z = _mix(Q.z,(tex.x-0.5f)*BlendZ_Thr, Blend1);

        if ((int)Modus&4)
          Q.w = _mix(Q.w,(tex.y-0.5f)*BlendW_Thr, Blend1);
        
        if ((int)Modus&8)
        {  
          //swi2S(Q,x,y, _mix(swi2(Q,x,y), U, Blenda));
          //if (U.x>0.1f && U.x<0.9f && U.y > 0.1f && U.y < 0.9f)
          float2 uv = U/R;  
          if (uv.x>0.01f && uv.x<0.99f && uv.y > 0.01f && uv.y < 0.99f)
             //Q.x=U.x, Q.y=U.y; //Q.z=tex.x; Q
             Q = to_float4(_floor(U.x/10.0f+0.5f)*10.0f,_floor(U.y/10.0f+0.5f)*10.0f,0,0);
        }
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
// Connect Image 'Cubemap: St Peters Basilica_0' to iChannel1
// Connect Image 'Previsualization: Buffer A' to iChannel0



//__DEVICE__ float4 T ( float2 U ) {return texture(iChannel0,U/R);}

__KERNEL__ void ThermalParticlesIiFuse(float4 C, float2 U, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_SLIDER9(Alpha, 0.0f, 1.0f, 1.0f);
    
    U+=0.5f;
    float4 
        a = T(U+to_float2(1,0),R,iChannel0),
        b = T(U-to_float2(1,0),R,iChannel0),
        c = T(U+to_float2(0,1),R,iChannel0),
        d = T(U-to_float2(0,1),R,iChannel0);
        
    float4 g = to_float4_f2f2(swi2(a,z,w)-swi2(b,z,w),swi2(c,z,w)-swi2(d,z,w));
    float2 dz = swi2(g,x,z);
    float2 dw = swi2(g,y,w);
    float4 v = T(U,R,iChannel0);
    swi3S(C,x,y,z, 0.5f-0.5f*cos_f3(2.0f*v.z+2.0f*v.w*to_float3(1,2,3)));
    float3 n = normalize(to_float3_aw(dz,0.05f));
    float4 tx = to_float4(0.6f,0.9f,1,1)*decube_f3(iChannel1,reflect(to_float3(0,0,1),n));
    C = (C*0.8f+0.2f)*(0.7f+0.3f*tx);
    C.w=Alpha;

  SetFragmentShaderComputedColor(C);
}