

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Preset: Keyboard' to iChannel1
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

//#define KEYBOARD iChannel1
//#define KEY_RESET 82


__DEVICE__ float Cell( in int2 p, float2 R, __TEXTURE2D__ iChannel0 )
{
    // do wrapping
    //int2 r = to_int2(textureSize(iChannel0, 0));
    int2 r = to_int2_cfloat(R);
    
    p = to_int2((p.x+r.x) % r.x,(p.y+r.y) % r.y);
    
    // fetch texel
   // return (texelFetch(iChannel0, p, 0 ).x > 0.5f ) ? 1 : 0;
   //return texelFetch(iChannel0, p, 0 ).x;
   return texture(iChannel0, (make_float2(p.x,p.y)+0.5f)/R).x;
}

__DEVICE__ float hash1( float n )
{
    return fract(_sinf(n)*138.5453123f);
}

//__DEVICE__ bool key_down(int key) {
//    return int(texelFetch(KEYBOARD, to_int2(key, 0), 0).x) == 1;
//}


// goes through 0.5f,0.5f, 0 derivative at 0,0  and 1,1
// not in use
__DEVICE__ float p(float _x)
{
  return (1.0f / 9.0f) * (-4.0f * _x * _x + 8.0f * _x + 5.0f) * _x * _x * (2.0f-_x) * (2.0f-_x);
}

// not in use
__DEVICE__ float gain(float _x, float k)
{
  float a = 0.5f*_powf(2.0f*((_x<0.5f)?_x:1.0-_x), k);
  return (_x<0.5f)?a:1.0-a;
}

// not in use
__DEVICE__ float p4(float _x)
{
return (0.9f + 0.1f * _cosf( 2.0f * 3.14159f * _x)) * _x;
}



__KERNEL__ void BurstingbubblesJipi977Fuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
  
  CONNECT_CHECKBOX0(Reset, 0); 

  fragCoord+=0.5f;    

  int2 px = to_int2_cfloat( fragCoord );
  
  if (Reset || iFrame == 0) {
  // if you want a random reset, uncomment

    float f = hash1(fragCoord.x*13.0f + 0.01f * iTime + hash1( fragCoord.y*73.1f));
    fragColor = to_float4( f, 0.0f, 0.0f, 0.0f );  
    SetFragmentShaderComputedColor(fragColor);
    return;
  }
  
  // center cell
  float e = Cell(px,R,iChannel0); 

  // neighbour cells
  float t = Cell(px + to_int2(0,-1),R,iChannel0);
  float b = Cell(px + to_int2(0,1),R,iChannel0);
  float l = Cell(px + to_int2(-1,0),R,iChannel0);
  float r = Cell(px + to_int2(1,0),R,iChannel0);

  float h = 0.5f * (l+r);
  float v = 0.5f * (t + b);  
  //float k = _fmaxf(h,v); // "average" of neighbours
  float k = 0.5f *(h+v);

  // difference between center and average
  float j = _fabs(e - k);

  if (fract(4.0f * (e-k)) < 0.26f) //0.05f, 0.28f are also interesting. (only values <= 0.3f ish)
  {
  if (e > k - 0.05f )
    e = 4.0f * j;
    e += 0.01f;
    //float n = 0.0f;
    float m =0.5f; //n * e + (1.0f-n) * 0.5f;
    float p = 0.01f;
    e = m * e + (1.0f-m) * _fmaxf(e + 0.01f, k - 0.01f);
  }
  else if ( j > 0.1f)
    e = k;
  else if (_fabs(v-h) < 0.2f)
  {
    e = k + 0.01f +  0.01f * step(0.9f,j) * sign_f(k-e);
  }

  if ( e > 0.9f)
    e = 0.0f;

  e = _fmaxf(min(e,1.0f),0.0f); // probably not necessary - cap values in between 0.0f and 1.

  fragColor = to_float4( e, 0.0f, 0.0f, 0.0f );


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


// Code forked from Inigo Quilez's game of life shader
// https://www.shadertoy.com/view/XstGRf
// Reset code stolen from somewhere else - sorry!
// (Press R to reset shader)

__KERNEL__ void BurstingbubblesJipi977Fuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
  CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f); 
  Color -= 0.5f;

  //float x = (1.0f/ 0.9f) * texelFetch( iChannel0, to_int2(fragCoord), 0 ).x;
  float x = (1.0f/ 0.9f) * texture( iChannel0, (make_float2(to_int2_cfloat(fragCoord))+0.5f)/iResolution ).x;
  //x = 16.0f * x * x * (1.0f-x) * (1.0f-x);
  fragColor = to_float4_s( x ) + Color;
  fragColor.w = Color.w;

  SetFragmentShaderComputedColor(fragColor);
}
