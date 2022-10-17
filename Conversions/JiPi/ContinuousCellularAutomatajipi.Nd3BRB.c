
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Preset: Keyboard' to iChannel1
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define KEYBOARD iChannel1
#define KEY_RESET 82


__DEVICE__ float Cell( in int2 p, float2 iResolution, __TEXTURE2D__ iChannel0 )
{
  // do wrapping
  //int2 r = to_int2(textureSize(iChannel0, 0));
	int2 r = to_int2_cfloat(iResolution);
  p = to_int2((p.x+r.x) % r.x, (p.y+r.y) % r.y);
    
  // fetch texel
  // return (texelFetch(iChannel0, p, 0 ).x > 0.5f ) ? 1 : 0;
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
  float a = 0.5f*_powf(2.0f*((x<0.5f)?x:1.0f-x), k);
  return (x<0.5f)?a:1.0f-a;
}

// not in use
__DEVICE__ float p4(float x)
{
  return (0.9f + 0.1f * _cosf( 2.0f * 3.14159f * x)) * x;
}



__KERNEL__ void ContinuousCellularAutomatajipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_CHECKBOX1(Reset2, 0);
	  CONNECT_CHECKBOX2(Tex, 0);

    fragCoord+=0.5f;

    int2 px = to_int2_cfloat( fragCoord );
    
    if (Reset2) {
      // if you want a random reset, uncomment
      /*
       float f = hash1(fragCoord.x*13.0f+ 10.131f * iTime + hash1(fragCoord.y*73.1f));
      if (f > 0.1f)
      f = 0.0f;
      */
      float x= fragCoord.x / iResolution.y - 0.375f;
      float y = fragCoord.y / iResolution.y;
      float2 dir = to_float2(x,y) - 0.5f;
      float d = length(dir);
      
      float tex = texture(iChannel1, fragCoord/iResolution).w;
      
      
      
      float f =(1.0f - 1.0f/ 0.15f * d) * step(d,0.15f);

      if (!Tex || (Tex && tex))   
        fragColor = to_float4( f, 0.0f, 0.0f, 0.0f );

          if (Tex)  fragColor = to_float4( tex, 0.0f, 0.0f, 0.0f );

      SetFragmentShaderComputedColor(fragColor);
      return;
    }
    
    // center cell
    float e = Cell(px,iResolution,iChannel0); 

    // neighbour cells
    float t = Cell(px + to_int2(0,-1),iResolution,iChannel0);
    float b = Cell(px + to_int2(0,1),iResolution,iChannel0);
    float l = Cell(px + to_int2(-1,0),iResolution,iChannel0);
    float r = Cell(px + to_int2(1,0),iResolution,iChannel0);   

    float h = 0.5f * (l+r);
    float v = 0.5f * (t + b);  
    float k = _fmaxf(h,v); // "average" of neighbours
    //float k = 0.5f *(h+v);

    // difference between center and average
    float j = _fabs(e - k);

	// if center is below average, increase
	// if center is above average, decrease
	if (e < k)
	 e += 0.18f * k;
	else if (e > k)
	 e -= 0.1f * k;

	float c = 0.01f; //0.02f, 0.03f look cool too
	if (j < c)
		e = k - c * sign_f(e-k) ; // keep center and average seperated if they're close
	else
		e = 0.9f * e + 0.1f * k; // lerp center to average if not close (usually true)


	if (e <= 0.05f && k > 0.05f) // expand black bits into white bits (really important)
		e = k + 0.1f; // e = ...; works fine too
	else if (e >= 0.95f && k < 0.95f) // slow values from converging to 1.
		e = k - 0.1f; // e *= 0.9f; // value not very important
	else if (e > k)
		e -= 0.5f * j; // 0.45f looks cool too, also slows values converging to 1

	e = _fmaxf(_fminf(e,1.0f),0.0f); // probably not necessary - cap values in between 0.0f and 1.

	float f = e;
	if( iFrame==0 || Reset) 
	{   
		float x= fragCoord.x / iResolution.y - 0.375f;
		float y = fragCoord.y / iResolution.y;
		float2 dir = to_float2(x,y) - 0.5f;
		float d = length(dir);
		f =(1.0f - 1.0f/ 0.15f * d) * step(d,0.15f);
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

__KERNEL__ void ContinuousCellularAutomatajipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

    
  float3 col =  to_float3_s(1.2f * texture( iChannel0, fragCoord/iResolution).x);

  fragColor = to_float4_aw( col, 1.0f );

  SetFragmentShaderComputedColor(fragColor);
}