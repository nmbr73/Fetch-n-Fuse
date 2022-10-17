
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R    iResolution
//#define T(d) texelFetch(iChannel0, to_int2(d+U)%to_int2(R),0)
#define T(d) texture(iChannel0, (make_float2((int)((d).x+U.x)%(int)(R.x),(int)((d).y+U.y)%(int)(R.y))+0.5f)/R)

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: Abstract 1' to iChannel1
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0




__KERNEL__ void LiquidCrystalForestFuse__Buffer_A(float4 O, float2 U, float2 iResolution, int iFrame, sampler2D iChannel1)
{
  CONNECT_CHECKBOX0(Reset, 0);
  CONNECT_CHECKBOX1(OrgPar, 0);
  
  CONNECT_SLIDER0(ampOff, -1.0f, 10.0f, 0.888f); 
  CONNECT_SLIDER1(ampMul, -1.0f, 1.0f, 0.00051f);
  
  CONNECT_SLIDER2(_K0, -10.0f, 10.0f, -3.3333f);
  CONNECT_SLIDER3(_K1, -1.0f, 1.0f, 0.66667f);
  CONNECT_SLIDER4(_K2, -1.0f, 1.0f, 0.16667f);
  
  CONNECT_SLIDER5(cs, -1.0f, 1.0f, 0.12052f);
  CONNECT_SLIDER6(ls, -1.0f, 1.0f, 0.12052f);
  
  CONNECT_SLIDER7(ps, -1.0f, 1.0f, -0.06f);
  CONNECT_SLIDER8(ds, -1.0f, 1.0f, -0.08f);
  
  CONNECT_SLIDER9(pwr, -1.0f, 1.0f, 0.2f);
  CONNECT_SCREW0(sq2, -2.0f, 2.0f, 0.7f);
  
  U+=0.5f;

//float _K0 = -20.0f/6.0f, // center weight
//      _K1 =   4.0f/6.0f, // edge-neighbors
//      _K2 =   1.0f/6.0f, // vertex-neighbors
       cs +=   (U.x*0.0000051f);//,    // curl scale
       ls +=   (U.x*0.0000051f);//,    // laplacian scale
//       ps = -0.06f,    // laplacian of divergence scale
//       ds = -0.08f,    // divergence scale
//      pwr =  0.2f,     // power when deriving rotation angle from curl
      //amp = 0.888f+(U.y*0.00051f),      // self-amplification
float      amp = ampOff+(U.y*ampMul);//,      // self-amplification
//      sq2 =  0.7f;     // diagonal weight

if(OrgPar)
{
       _K0 = -20.0f/6.0f; // center weight
       _K1 =   4.0f/6.0f; // edge-neighbors
       _K2 =   1.0f/6.0f; // vertex-neighbors
       cs  =  0.12052f + (U.x*0.0000051f);//,    // curl scale
       ls  =  0.12052f + (U.x*0.0000051f);//,    // laplacian scale
       ps = -0.06f;    // laplacian of divergence scale
       ds = -0.08f;    // divergence scale
      pwr =  0.2f;     // power when deriving rotation angle from curl
//      amp = 0.888f+(U.y*0.00051f);      // self-amplification
      amp = ampOff+(U.y*ampMul);//,      // self-amplification
      sq2 =  0.7f;     // diagonal weight
  
}



// 3x3 neighborhood coordinates
    float4 uv = T(to_float2( 0,  0 )),
            n = T(to_float2( 0,  1 )),
            e = T(to_float2( 1,  0 )),
            s = T(to_float2( 0, -1 )),
            w = T(to_float2(-1,  0 )),
           nw = T(to_float2(-1,  1 )),
           sw = T(to_float2(-1, -1 )),
           ne = T(to_float2( 1,  1 )),
           se = T(to_float2( 1, -1 ));
    
    // uv.x and uv.y are our x and y components, uv.z is divergence 
float AAAAAAAAAAAAAAAAA;
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
     
    t = mul_f2_mat2(t,to_mat2(_cosf(a), -_sinf(a), _sinf(a), _cosf(a) ));
    if(iFrame<12 || Reset)
      O = -0.5f + texture(iChannel1, U/R), O.w=0.0f;
    else 
      O = clamp(to_float4(t.x,t.y,div,0), -1.0f, 1.0f);

  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


// WebGL2 cleaned-up version of "Viscous Fingering" by cornusammonis. https://shadertoy.com/view/Xst3Dj

__KERNEL__ void LiquidCrystalForestFuse(float4 O, float2 u, float2 iResolution)
{
    CONNECT_CHECKBOX2(Variante, 0);
    CONNECT_COLOR0(Color, 1.3f, 1.3f, 0.75f, 1.0f);
    u+=0.5f;

    float2 U = u;
    O =  normalize(T(to_float2(0.0f,0.0f))); 
    
    // Before Fabrice suggestion I had chosen values:
    if (Variante)
      O =  O/to_float4(2,4,4,1) + 0.5f*O.z;
    else
      // now I boost the blue and dial back the red and green
      //O =  O/to_float4(1.3f,1.3f,0.75f,1) + 0.4f *O.z; // 28.1f fps 
      O =  O/Color + 0.4f *O.z; // 28.1f fps 

    O.w = Color.w;

  SetFragmentShaderComputedColor(O);
}