
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


#define R iResolution
#define A(U) _tex2DVecN(iChannel0, (U).x/R.x,(U).y/R.y,15)
#define B(U) _tex2DVecN(iChannel1, (U).x/R.x,(U).y/R.y,15)
#define C(U) _tex2DVecN(iChannel2, (U).x/R.x,(U).y/R.y,15)
#define D(U) _tex2DVecN(iChannel3, (U).x/R.x,(U).y/R.y,15)
#define E(U) _tex2DVecN(iChannel4, (U).x/R.x,(U).y/R.y,15)

#define N 3.0f

#define PRECIPITATION 1.0f
#define EVAPORATION 0.0001f
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer B' to iChannel0
// Connect Buffer A 'Previsualization: Buffer D' to iChannel2


// Calculate forces and pressure
__KERNEL__ void LandOfWaterFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, int iFrame)
{
    U+=0.5f;
    
    Q = A(U);
    float4 
        n = A(U+to_float2(0,1)),
        e = A(U+to_float2(1,0)),
        s = A(U-to_float2(0,1)),
        w = A(U-to_float2(1,0)),
        c = C(U),
        nc = C(U+to_float2(0,1)),
        ec = C(U+to_float2(1,0)),
        sc = C(U-to_float2(0,1)),
        wc = C(U-to_float2(1,0));
    swi2S(Q,x,y, swi2(Q,x,y) - 1.0f/(1.0f+2.0f*_sqrtf(c.y))*(
                // slope force
                0.25f*(0.5f*to_float2(ec.x-wc.x,nc.x-sc.x)+to_float2(ec.y-wc.y,nc.y-sc.y))+
                // pressure force
                0.25f*to_float2(e.z-w.z,n.z-s.z)+
                // magnus force
                0.25f*Q.w*to_float2(n.w-s.w,e.w-w.w)));
    //swi2(Q,x,y) *= _fminf(1.0f,c.y);
    Q.x *= _fminf(1.0f,c.y);
    Q.y *= _fminf(1.0f,c.y);
    // divergence
    Q.z  = 0.25f*(s.y-n.y+w.x-e.x+n.z+e.z+s.z+w.z);
    // curl
    Q.w = 0.25f*(s.x-n.x+w.y-e.y);
    if (length(swi2(Q,x,y)) > 0.8f) swi2S(Q,x,y, 0.8f*normalize(swi2(Q,x,y)));
    
    //Boundary conditions
    if (iFrame<1) Q = to_float4_s(0);
    
  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


// Advect along velocity and curl feild
__KERNEL__ void LandOfWaterFuse__Buffer_B(float4 Q, float2 U, float2 iResolution)
{
    U+=0.5f;
    
    for (float i = 0.0f; i< N;i+=1.0f) {
        Q = A(U);
        float co = _cosf(Q.w/N), si = _sinf(Q.w/N);
        U -= mul_f2_mat2(swi2(Q,x,y) , to_mat2(co,-si,si,co))/N;
    }
    Q = A(U);

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer D' to iChannel2


// Advect along velocity and curl feild
__KERNEL__ void LandOfWaterFuse__Buffer_C(float4 Q, float2 U, float2 iResolution)
{
    U+=0.5f;
    
    for (float i = 0.0f; i< N;i+=1.0f) {
        Q = A(U);
        float co = _cosf(Q.w/N), si = _sinf(Q.w/N);
        U -= mul_f2_mat2(swi2(Q,x,y) , to_mat2(co,-si,si,co))/N;
    }
    Q = C(U);

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Texture: Abstract 1' to iChannel1
// Connect Buffer D 'Previsualization: Buffer A' to iChannel0
// Connect Buffer D 'Previsualization: Buffer C' to iChannel2
// Connect Buffer D 'Previsualization: Buffer D' to iChannel3


__KERNEL__ void LandOfWaterFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_SLIDER1(StartTex, 0.1f, 10.0f, 0.1f);
   
  
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1_Thr, 0.0f, 10.0f, 4.0f);
  
    CONNECT_CHECKBOX0(TexBurn, 0);
    CONNECT_SLIDER4(TexBurnStrength, -10.0f, 10.0f, 1.0f);
  
  
    U+=0.5f;
   
    Q = C(U);
    // neighborhood
    float4 
        a = A(U),
        n = A(U+to_float2(0,1)),
        e = A(U+to_float2(1,0)),
        s = A(U-to_float2(0,1)),
        w = A(U-to_float2(1,0)),
        nc = C(U+to_float2(0,1)),
        ec = C(U+to_float2(1,0)),
        sc = C(U-to_float2(0,1)),
        wc = C(U-to_float2(1,0));
    Q = _mix(Q,0.25f*(nc+ec+sc+wc),to_float4(0.0f,0.1f,0,0));
    // divergence 
    Q += 0.25f*(s.y*sc-n.y*nc+w.x*wc-e.x*ec);
    
    // x : height y : water, z : sediment 
    float m = 0.25f*(D(U+to_float2(0,1)).x+D(U+to_float2(1,0)).x+D(U-to_float2(1,0)).x+D(U-to_float2(0,1)).x);
    float me = D(U).x;
    float l = m-me;
    Q.x = me + 0.01f*l*l*l;
    if (iMouse.z>0.0f)  Q.x += 0.5f/(1.0f+0.01f*dot(U-swi2(iMouse,x,y),U-swi2(iMouse,x,y)));
    float _x = 0.05f*(Q.y*length(swi2(a,x,y))*(1.0f-Q.z)-0.1f*Q.z);
    Q.z += _x;
    Q.x -= _x;
    Q.y = Q.y*(1.0f-EVAPORATION) + PRECIPITATION/R.x;
    Q = _fmaxf(Q, to_float4_s(0.0f));
    
    
        //Textureblending
    if (Blend1 > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = B(U);

      if (tex.w > 0.0f)
      {
        Q.x = _mix(Q.x,tex.x,Blend1);
      }
    }
    
    if(TexBurn)
    {
      float4 tex = B(U);
      if (tex.w > 0.0f)
        Q.x += 0.5f/TexBurnStrength;
    }
    
    // boundary conditions
    //if (iFrame<5)                                        Q = to_float4(10.0f+1.0f-U.y/R.y+0.1f*B(U).x+2.0f*_expf(-length(U-0.5f*R)/R.y),0.3f,0,0);
    if (iFrame<5)                                        Q = to_float4(10.0f+1.0f-U.y/R.y+StartTex*B(U).x+2.0f*_expf(-length(U-0.5f*R)/R.y),0.3f,0,0);
    if (U.x<2.0f||U.y<2.0f||R.y-U.y<2.0f||R.x-U.x<2.0f)  Q*=0.0f;

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer C' to iChannel2
// Connect Image 'Previsualization: Buffer D' to iChannel3


// Controls in Common

__KERNEL__ void LandOfWaterFuse(float4 Q, float2 U, float2 iResolution)
{
  
    CONNECT_SLIDER0(Alpha,0.0f,1.0f,1.0f);
    U+=0.5f;
    float4 
        n = (C(U+to_float2(0,1))),
        e = (C(U+to_float2(1,0))),
        s = (C(U-to_float2(0,1))),
        w = (C(U-to_float2(1,0)));
    float4 c = C(U);
    float3 no = normalize(to_float3(e.x-w.x+e.y-w.y,n.x-s.x+n.y-s.y,0.5f));
    Q = abs_f4(sin_f4(1.0f+3.0f*c.z+_sqrtf(c.y)*(1.0f+0.5f*to_float4(1,2,3,4))));
    float a = 0.5f;
    swi2S(no,z,y, mul_f2_mat2(swi2(no,z,y) , to_mat2(_cosf(a),-_sinf(a),_sinf(a),_cosf(a))));
    swi2S(no,z,x, mul_f2_mat2(swi2(no,z,x) , to_mat2(_cosf(a),-_sinf(a),_sinf(a),_cosf(a))));
    Q*=_fmaxf(0.0f,0.2f+0.8f*no.z);
    
    Q.w=Alpha;
  
  SetFragmentShaderComputedColor(Q);
}