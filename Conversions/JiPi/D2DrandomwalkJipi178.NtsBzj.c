
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Preset: Keyboard' to iChannel1
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


__DEVICE__ float Cell( in int2 p, float2 R, __TEXTURE2D__ iChannel0 )
{
    // do wrapping
   // int2 r = to_int2(textureSize(iChannel0, 0));
    int2 r = to_int2_cfloat(R);
    p = to_int2((p.x+r.x) % r.x, (p.y+r.y) % r.y);
    
   // fetch texel
   // return (texelFetch(iChannel0, p, 0 ).x > 0.5f ) ? 1 : 0;
   //return texelFetch(iChannel0, p, 0 ).x;
   return texture(iChannel0, (make_float2(p)+0.5f)/R ).x;
}

__DEVICE__ float hash1( float n )
{
    return fract(_sinf(n)*138.5453123f);
}

__DEVICE__ float S(float _x)
{
return step(0.5f,_x);
}

__KERNEL__ void D2DrandomwalkJipi178Fuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
  CONNECT_CHECKBOX0(Reset, 0); 
   
  fragCoord+=0.5f;

  int2 px = to_int2_cfloat( fragCoord );
      
  if (Reset || iFrame == 0)
  {    
    float f = 0.0f;
    if (fragCoord.x > 0.5f * iResolution.x && fragCoord.x < 0.5f * iResolution.x + 1.
     && fragCoord.y > 0.5f * iResolution.y && fragCoord.y < 0.5f * iResolution.y + 1.0f)
      f = 3.0f;

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


  // 2 up, 3 right, 4 down, 5 left
//#ifdef XXX
  if (fragCoord.y > iMouse.w && fragCoord.y < iMouse.w + 1.
   && fragCoord.x > iMouse.z && fragCoord.x < iMouse.z + 1.0f)
      e = 3.0f;
    else if (e > 1.0f)
      e = 1.0f;
    else if (b == 2.0f)
      e = 2.0f;
    else if (t == 4.0f)
      e = 4.0f;
    else if (l == 3.0f)
      e = 3.0f;
    else if (r == 5.0f)
      e = 5.0f;
    else 
      e -= 0.005f;
//#endif
  float q = hash1(fragCoord.x*13.0f + 0.1f * iTime + hash1(fragCoord.y*73.1f));
  if (q > 0.95f) // probability direction will change
  {
    // turn anticlockwise 
    // could easily be replaced with a function but I'm lazy atm
    if (e == 2.0f)
      e = 3.0f;
      else if (e == 3.0f)
      e = 4.0f;
      else if (e == 4.0f)
      e = 5.0f;
      else if (e == 5.0f)
    e = 2.0f;
  }


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

__KERNEL__ void D2DrandomwalkJipi178Fuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

    //fragColor = to_float4_aw( 1.0f - step(3.0f * texelFetch( iChannel0, to_int2(fragCoord), 0 ).x ,0.2f));
    //fragColor = to_float4_aw( 3.0f * texelFetch( iChannel0, to_int2(fragCoord), 0 ).x );
    fragColor = to_float4_s( 3.0f * texture( iChannel0,fragCoord/iResolution ).x );


  SetFragmentShaderComputedColor(fragColor);
}