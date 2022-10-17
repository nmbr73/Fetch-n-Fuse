
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R    iResolution
#define H(p) fract_f2(sin_f2( mul_f2_mat2((p), to_mat2(127.1f,311.7f, 269.5f,183.3f))) *43758.5453123f)
//#define T(U) texelFetch( iChannel0, to_int2(U), 0 )
#define T(_U) texture( iChannel0, (make_float2(to_int2_cfloat(_U))+0.5f)/R )
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void Entropy5Fuse__Buffer_A(float4 O, float2 u, float2 iResolution, float iTime, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0);  
    CONNECT_SLIDER0(Bomp, -1.0f, 1.0f, 0.3f);
    CONNECT_SLIDER2(Uix, -1.0f, 1.0f, 0.2f);
    u+=0.5f;

    float2 U = u/R.y;
    if (iFrame<1 || Reset) O = to_float4(U.x,U.y,0,0);
    else {
      
        for(int i=0; i<10; i++)
        {
         
          float2 P =  U - H(to_float2_s(iTime+(float)i/R.x))*R /R.y;
          float d = length(P);
          
          float temp = U.x - Uix;
          
          //swi2S(O,x,y, (swi2(T(u),x,y) + Bomp * _fmaxf(0.0f,U.x-Uix) *  P * smoothstep(0.1f,0.0f,d)));   // bomp uv space 
          
          float2 OTemp = (swi2(T(u),x,y) + Bomp * _fmaxf(0.0f,U.x-Uix) *  P * smoothstep(0.1f,0.0f,d));   // bomp uv space 
          swi2S(O,x,y, swi2(O,x,y) + OTemp);
        }
    }

  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Font 1' to iChannel1
// Connect Image 'Previsualization: Buffer A' to iChannel0


// variant of https://shadertoy.com/view/sld3Dl

// ---------------------- utils from https://www.shadertoy.com/view/llySRh
//#define H(p) fract(_sinf((p)*mat2(127.1f,311.7f, 269.5f,183.3f)) *43758.5453123f)

__DEVICE__ float4 _char(float2 p, int c, float2 R, __TEXTURE2D__ iChannel1) {
   // float2 dFdx = dFdx(p/16.0f), dFdy = dFdy(p/16.0f);
   float2 dFdx = to_float2(3.0f/R.y/16.0f,0), dFdy = swi2(dFdx,y,x) ;
   if (p.x<0.0f|| p.x>1.0f || p.y<0.0f|| p.y>1.0f)  return to_float4(0,0,0,1e5);
   //return textureGrad( iChannel1, p/16.0f + fract_f2( to_float2(c, 15-c/16) / 16.0f ), dFdx, dFdy );
   return texture( iChannel1, p/16.0f + fract_f2( to_float2(c, 15-c/16) / 16.0f ));
}
#define spc  U.x-=0.5f;
#define C(c) spc d = _fminf(d, _char(U,64+32+c,R,iChannel1).w );
// ----------------------

__KERNEL__ void Entropy5Fuse(float4 O, float2 u, float2 iResolution, sampler2D iChannel1)
{
    CONNECT_SLIDER1(d, -1.0f, 20.0f, 9.0f);
  
    u+=0.5f;
 
    float2 U = u / R.y, 
           p = (u/2.0f);
         
    //float d = 9.0f;
    U = swi2(T(u),x,y);
    U *= 2.5f; U -= to_float2(-0.3f,0.8f);            // draw text
    C(5)C(14)C(20)C(18)C(15)C(16)C(25)                // "entropy"
    O = to_float4_s( smoothstep(0.0f, 4.0f/R.y,d-0.5f) );

  SetFragmentShaderComputedColor(O);
}