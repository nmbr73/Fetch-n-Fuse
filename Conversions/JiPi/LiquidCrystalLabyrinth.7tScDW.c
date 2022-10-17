
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


#define R    iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

//#define T(d) texelFetch(iChannel0, to_int2(d+U)%to_int2(R),0)
//#define T(d) texture(iChannel0, (make_float2(to_int2_cfloat(d+U) % to_int2_cfloat(R) )+0.5)/R)

#define T(d) texture(iChannel0, (make_float2((int)(d.x+U.x) % (int)R.x, (int)(d.y+U.y) % (int)R.y  )+0.5f)/R)


__DEVICE__ float4 Blending( __TEXTURE2D__ channel, float2 uv, float4 Q, float Blend, float2 Par, float2 MulOff, int Modus, float2 U, float2 R)
{
   
    if (Blend > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(channel,uv);

      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2)
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
          //swi3S(Q,x,y,w, _mix(swi3(Q,x,y,w),(swi3(tex,x,y,z)+MulOff.y)*MulOff.x,Blend));

        if ((int)Modus&4)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par , Blend));
          //swi2S(Q,x,y, _mix( swi2(Q,x,y),  Par, Blend));
          //swi3S(Q,x,y,z, _mix(swi3(Q,x,y,z), (swi3(tex,x,y,z)+MulOff.y)*MulOff.x, Blend));  
          Q = _mix(Q,to_float4(Par.x,Par.y,(tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x),Blend);
        
        
        if ((int)Modus&8)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par, Blend));
          Q = _mix(Q,to_float4((tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x,Par.x,Par.y),Blend);
          //Q.z = _mix( Q.z,  (tex.x+MulOff.y)*MulOff.x, Blend);
          //swi2S(Q,z,w, _mix( swi2(Q,z,w), swi2(tex,x,y)*Par, Blend));

        if ((int)Modus&16) 
          //swi2S(Q,z,w, _mix(swi2(Q,z,w),  swi2(tex,x,y)*Par, Blend));
          Q = _mix(Q,to_float4(Par.x,Par.y,MulOff.x,MulOff.y),Blend);
      }
      else
        if ((int)Modus&32) //Special
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
    }
  
  return Q;
}




// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: London' to iChannel1
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0




__KERNEL__ void LiquidCrystalLabyrinthFuse__Buffer_A(float4 O, float2 U, float2 iResolution, int iFrame, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0);
  
    //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);

  U+=0.5f;

  float _K0 = -20.0f/6.0f, // center weight
        _K1 =   4.0f/6.0f, // edge-neighbors
        _K2 =   1.0f/6.0f, // vertex-neighbors
         cs =  0.12052f + (U.x*0.0000051f),    // curl scale
         ls =  0.12052f + (U.x*0.0000051f),    // laplacian scale
         ps = -0.06f,    // laplacian of divergence scale
         ds = -0.08f,    // divergence scale
        pwr =  0.2f,     // power when deriving rotation angle from curl
        amp = 0.999f+(U.y*0.000051f),      // self-amplification
        sq2 =  0.7f;     // diagonal weight

  // 3x3 neighborhood coordinates
  float4 uv = T(to_float2_s(0)),
          n = T(to_float2( 0,  1 )),
          e = T(to_float2( 1,  0 )),
          s = T(to_float2( 0, -1 )),
          w = T(to_float2(-1,  0 )),
         nw = T(to_float2(-1,  1 )),
         sw = T(to_float2_s(-1     )),
         ne = T(to_float2_s( 1     )),
         se = T(to_float2( 1, -1 ));
    
    // uv.x and uv.y are our x and y components, uv.z is divergence 

    // laplacian of all components
    float4 lapl  = _K0*uv + _K1*(n + e + w + s) 
                          + _K2*(nw + sw + ne + se);
    float sp = ps * lapl.z;
    
    // calculate curl
    // vectors point clockwise about the center point
    float curl = n.x - s.x - e.y + w.y 
               + sq2 * (nw.x + nw.y + ne.x - ne.y + sw.y - sw.x - se.y - se.x);
    
    // compute angle of rotation from curl
    float a = cs * sign_f(curl) * _powf(_fabs(curl), pwr);
    
    // calculate divergence
    // vectors point inwards towards the center point
    float div  = s.y - n.y - e.x + w.x 
        + sq2 * (nw.x - nw.y - ne.x - ne.y + sw.x + sw.y + se.y - se.x);
    float sd = ds * div;

    float2 norm = normalize(swi2(uv,x,y));
    
    // temp values for the update rule
     float2 t = swi2((amp * uv + ls * lapl + uv * sd),x,y) + norm * sp;
     float red = sd;
     float green = div;
     float blue = t.x;
     //O = to_float4(red,green, blue, 1.0f);
     //O = clamp(to_float4(t,div,0), -1.0f, 1.0f);
     
     t = mul_f2_mat2(t, to_mat2(_cosf(a), -_sinf(a), _sinf(a), _cosf(a) ));
     
     
     
     if(iFrame<10 || Reset)
        O = -0.5f + texture(iChannel1, U/R), O.w = 0.0f;
     else 
        O = clamp(to_float4(t.x,t.y,div,0), -1.0f, 1.0f);

     if (Blend1>0.0) O = Blending(iChannel2, U/R, O, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, U, R);


  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


// WebGL2 cleaned-up version of "Viscous Fingering" by cornusammonis. https://shadertoy.com/view/Xst3Dj

__KERNEL__ void LiquidCrystalLabyrinthFuse(float4 O, float2 u, float2 iResolution)
{
    CONNECT_CHECKBOX1(Invers, 0);
    CONNECT_CHECKBOX2(ApplyColor, 0);

    CONNECT_CHECKBOX3(Blending12, 0);

    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f); 
    CONNECT_SLIDER0(AlphaThres, 0.0f, 1.0f, 1.0f);
    
    
    //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER5(Blend2, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER6(Blend2Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER7(Blend2Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON1(Modus2, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT3(Par2, 0.0f, 0.0f);
    
if (Blending12) Blend2 = Blend1;

  u+=0.5f;

  float2 U = u;
    
    O.x += 0.5f  * normalize(T(to_float2_s(0))).x; 
    O.y += 0.25f * normalize(T(to_float2_s(0))).y; 
    O.z += 0.25f * normalize(T(to_float2_s(0))).z; 
    O += 0.5f * to_float4_s(normalize(T(to_float2_s(0))).z); 
       
    
    if (Invers) O = to_float4_s(1.0f) - O;
    if (ApplyColor)
    {
      O = (O + (Color-0.5f))*O.w;
      if (O.x <= AlphaThres)      O.w = Color.w;  
    }
    
    if (Blend2>0.0) O = Blending(iChannel1, U/R, O, Blend2, Par2, to_float2(Blend2Mul,Blend2Off), Modus2, U, R);
    
    

  SetFragmentShaderComputedColor(O);
}