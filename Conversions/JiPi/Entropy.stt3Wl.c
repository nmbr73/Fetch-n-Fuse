
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Font 1' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
// ---------------------- utils from https://www.shadertoy.com/view/llySRh
#define H(p) ( 2.0f* fract_f2(sin_f2(mul_f2_mat2((p),to_mat2(127.1f,311.7f, 269.5f,183.3f))) *43758.5453123f) -1.0f )

__DEVICE__ float4 _char(float2 p, int c, float2 R, __TEXTURE2D__ iChannel0) {
   // float2 dFdx = dFdx(p/16.0f), dFdy = dFdy(p/16.0f);
   float2 dFdx = to_float2(3.0f/R.y/16.0f,0), dFdy = swi2(dFdx,y,x) ;
   if (p.x<0.0f|| p.x>1.0f || p.y<0.0f|| p.y>1.0f) return to_float4(0,0,0,1e5);
   //return textureGrad( iChannel0, p/16.0f + fract( to_float2(c, 15-c/16) / 16.0f ), dFdx, dFdy );
   return texture( iChannel0, p/16.0f + fract_f2( to_float2(c, 15-c/16) / 16.0f ));
}
#define spc  U.x-=0.5f;
#define C(c) spc O += _char(U,64+32+c,R, iChannel0).x;
// ----------------------

__KERNEL__ void EntropyFuse(float4 O, float2 u, float2 iResolution, sampler2D iChannel0)
{

CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);
CONNECT_COLOR1(ColorBKG, 0.0f, 0.0f, 0.0f, 1.0f);
CONNECT_SLIDER0(Jitter, -20.0f, 10.0f, 0.2f);

CONNECT_POINT0(PosXY, 0.0f, 0.0f );

CONNECT_CHECKBOX0(Modus1, 0);

    float Alpha = 1.0f;

    float2 U = u / R.y, 
           p = _floor(u/2.0f);
         
    //float e = _fmaxf(0.0f,0.12f*(U.x-Jitter));//0.2f));      // jitter
    float e = _fmaxf(-10.0f,0.12f*(U.x-Jitter));//0.2f));      // jitter
    if ( H(p+0.5f ).x < e ) U += e * H(p);
    
       
    O -= O;
    U *= 2.5f; U -= to_float2(-0.3f,0.8f)+PosXY;        // draw text
    C(5)C(14)C(20)C(18)C(15)C(16)C(25)                  // "entropy"
    
    if(O.x==0.0f&&O.y==0.0f&&O.z==0.0f) 
    {
      Alpha = Color.w;
      O=ColorBKG;
    }
    else
      O = Color;
    
    if(Modus1 == false)
      O = to_float4_s(1.0f)-O;

  O.w = Alpha;

  SetFragmentShaderComputedColor(O);
}