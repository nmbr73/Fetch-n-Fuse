
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture' to iChannel1
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

__DEVICE__ float Cell( in int2 p, float2 iResolution, __TEXTURE2D__ iChannel0 )
{
  // do wrapping
  //int2 r = to_int2(textureSize(iChannel0, 0));
	int2 r = to_int2_cfloat(iResolution);
  p = to_int2((p.x+r.x) % r.x, (p.y+r.y) % r.y);

  // fetch texel
  // return (texelFetch(iChannel0, p, 0 ).x > 0.5f ) ? 1 : 0;
  //return texelFetch(iChannel0, p, 0 ).x;
  return texture(iChannel0, (to_float2(p.x,p.y)+0.5f)/iResolution).x;
}

__DEVICE__ float hash1( float n )
{
  return fract(_sinf(n)*138.5453123f);
}

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



__KERNEL__ void SpoonLifeJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
	  CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_CHECKBOX1(Reset2, 0);
    CONNECT_CHECKBOX2(Tex, 0);
    
    
	  fragCoord+=0.5f;

    int2 px = to_int2_cfloat( fragCoord );

    if (Reset2) {
		// if you want a random reset, uncomment
		
	  //  float f = hash1(fragCoord.x*13.0f+ 10.131f * iTime + hash1(fragCoord.y*73.1f));
		
		float x = fragCoord.x / iResolution.y - 0.375f;
		float y = fragCoord.y / iResolution.y;
		float2 dir = to_float2(x,y) - 0.5f;
		float d = length(dir);
		float m = 0.15f;
		float f = (1.0f - step(d,0.4f)) * hash1(fragCoord.x*13.0f+ 10.131f * iTime + hash1(fragCoord.y*73.1f));

    if (Tex)
    {
       float texw = texture(iChannel1, fragCoord/iResolution).w;
       
       if (texw > 0.0f)
         f = hash1(fragCoord.x*13.0f+ 10.131f * iTime + hash1(fragCoord.y*73.1f));
    }


		fragColor = to_float4( f, 0.0f, 0.0f, 0.0f );
		SetFragmentShaderComputedColor(fragColor);
		return;
    }
    
  // center cell
  float e = Cell(px, iResolution, iChannel0); 

  // neighbour cells
  float t = Cell(px + to_int2(0,-1), iResolution, iChannel0);
  float b = Cell(px + to_int2(0,1), iResolution, iChannel0);
  float l = Cell(px + to_int2(-1,0), iResolution, iChannel0);
  float r = Cell(px + to_int2(1,0), iResolution, iChannel0);   

  float h = 0.5f * (l+r);
  float v = 0.5f * (t + b);  
  //float k = _fmaxf(h,v); // "average" of neighbours
  float k = 0.5f *(h+v);

  // difference between center and average
  float j = _fabs(e - k);

 if ( e < k)
   e += 0.1f;
 else if (e > k)
   e -= 0.115f;

 if ( e < 0.5f)
   e -= 0.001f;

 if (e < 0.1f)
   e = k;// - 0.01f;// 0.01
 else if ( e > 0.9f && k > 0.5f)
   e = k;

 //e = 0.999f * e + 0.001f * k;
 e -= 0.0135f * step(k,0.1f);
 e = _fmaxf(min(e,1.0f),0.0f); // probably not necessary - cap values in between 0.0f and 1.

  float f = e;
  if( iFrame==0 || Reset ) 
  {   

    if (Tex)
    {
      float texw = texture(iChannel1, fragCoord/iResolution).w;
      
      if (texw > 0.0f)
        f = hash1(fragCoord.x*13.0f+ 10.131f * iTime + hash1(fragCoord.y*73.1f));
    }
    else
    {
      // f = hash1(fragCoord.x*13.0f + hash1(fragCoord.y*73.1f));
      float x = fragCoord.x / iResolution.y - 0.375f;
      float y = fragCoord.y / iResolution.y;
      float2 dir = to_float2(x,y) - 0.5f;
      float d = length(dir);
      float m =0.3f;
      f = (1.0f - d/m) *  step(d,m) * hash1(fragCoord.x*13.0f+ 10.131f * iTime + hash1(fragCoord.y*73.1f));
    }    

  }
    
  fragColor = to_float4( f, 0.0f, 0.0f, 0.0f );

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

__KERNEL__ void SpoonLifeJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

  //fragColor = to_float4_aw( 4.5f* texelFetch( iChannel0, to_int2(fragCoord), 0 ).x );
  fragColor = to_float4_s( 4.5f* texture( iChannel0, (to_float2((int)fragCoord.x,(int)fragCoord.y)+0.5f)/iResolution ).x );

  SetFragmentShaderComputedColor(fragColor);
}