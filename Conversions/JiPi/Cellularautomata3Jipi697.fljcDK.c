
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Preset: Keyboard' to iChannel1
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)
#define R iResolution

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
    return texture(iChannel0, (make_float2(p)+0.5f)/iResolution).x; 
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
__DEVICE__ float p(float x)
{
  return (1.0f / 9.0f) * (-4.0f * x * x + 8.0f * x + 5.0f) * x * x * (2.0f-x) * (2.0f-x);
}

// not in use
__DEVICE__ float gain(float x, float k)
{
  float a = 0.5f*_powf(2.0f*((x<0.5f)?x:1.0-x), k);
  return (x<0.5f)?a:1.0-a;
}

// not in use
__DEVICE__ float p4(float x)
{
  return (0.9f + 0.1f * _cosf( 2.0f * 3.14159f * x)) * x;
}



__KERNEL__ void Cellularautomata3Jipi697Fuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_CHECKBOX1(ResetTextur, 0);
    CONNECT_CHECKBOX2(Average, 0);
    CONNECT_CHECKBOX3(Variante, 0);
    
    CONNECT_SLIDER0(Blend, 0.0f, 1.0f, 0.0f);
  
    fragCoord+=0.5f;

    int2 px = to_int2_cfloat( fragCoord );
    
    if (Reset) {
      // if you want a random reset, uncomment
      //float f = hash1(fragCoord.x*13.0f+ 10.131f * iTime + hash1(fragCoord.y*73.1f));
      
      float x= fragCoord.x / iResolution.y - 0.375f;
      float y = fragCoord.y / iResolution.y;
      float2 dir = to_float2(x,y) - 0.5f;
      float d = length(dir);
      float f = step(d,0.4f);

      fragColor = to_float4( f, 0.0f, 0.0f, 0.0f );
      
      SetFragmentShaderComputedColor(fragColor);
      return;
    }
    
    if (ResetTextur) {
      
      fragColor = texture(iChannel1, fragCoord/iResolution)/4.0f; 
      
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
  if (Average) k = _fmaxf(h,v); // "average" of neighbours


  // difference between center and average
  float j = _fabs(e - k);

  if (fract(4.0f * (e-k)) < 0.565f)
  {
    if (j < 0.34f) // change this value for fun times ( change e>=0.3f below too) // 0.2f, 0.33f, 0.5
    {
      float m = 0.001f;
      if (e > k )
      e = k + m;
      else
      e = k -m;
    }
  }
  else 
  {
    if (j < 0.5f)
    {
      if (e > k )
        e = k - 0.09f;
      else
        e = k + 0.008f;
    }
  }

//  if (0)
    if (e >= 0.9f && k <= 0.8925f && Variante) // also is good
      e = 0.0f;
//  else    
    if (e >= 0.3f && !Variante)
      e = 0.0f;

  e = _fmaxf(_fminf(e,1.0f),0.0f); // probably not necessary - cap values in between 0.0f and 1.

  float f = e;
  if( iFrame==0 ) 
  {   
     f = hash1(fragCoord.x*13.0f + hash1(fragCoord.y*73.1f));
  }

  //Blending
  if(Blend > 0.0f) {
    
    float4 tex = texture(iChannel1, fragCoord/iResolution)/4.0f; 
    
    if (tex.w > 0.0f)
      f = _mix(f, tex.x, Blend);
    
  }

    
  fragColor = to_float4( f, 0.0f, 0.0f, 1.0f );

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

__KERNEL__ void Cellularautomata3Jipi697Fuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);
    fragCoord+=0.5f;

    //fragColor = to_float4_aw( 4.0f* texelFetch( iChannel0, to_int2(fragCoord), 0 ).x );
    fragColor = to_float4_s( 4.0f* texture( iChannel0, (make_float2(to_int2_cfloat(fragCoord))+0.5f)/iResolution ).x ); //Original
    
    
    float bufA = 4.0f* texture( iChannel0, (make_float2(to_int2_cfloat(fragCoord))+0.5f)/iResolution ).x;
    Color+=0.5f;
    fragColor = to_float4_aw(swi3(Color,x,y,z)*bufA,Color.w);
    //fragColor.w=Color.w;

  SetFragmentShaderComputedColor(fragColor);
}