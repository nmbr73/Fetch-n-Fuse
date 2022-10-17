
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Preset: Keyboard' to iChannel1
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
  //   return texelFetch(iChannel0, p, 0 ).x;
   return texture(iChannel0, (to_float2(p.x,p.y)+0.5f)/iResolution ).x;
}

__DEVICE__ float hash1( float n )
{
  return fract(_sinf(n)*138.5453123f);
}


__DEVICE__ float f(float x)
{
	return 16.0f * x * x * (1.0f - x ) * (1.0f - x);
}

__KERNEL__ void D2DWalkAutomataJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
  

	CONNECT_CHECKBOX0(Reset, 0);
	
  fragCoord+=0.5f;
  
  
	int2 px = to_int2_cfloat( fragCoord );

	/*
	if (iFrame == 0)
	{    
	float f = 0.0f;
	if (fragCoord.x > 0.5f * iResolution.x && fragCoord.x < 0.5f * iResolution.x + 1.
	 && fragCoord.y > 0.5f * iResolution.y && fragCoord.y < 0.5f * iResolution.y + 1.0f)
	f = 3.0f;

	fragColor = to_float4( f, 0.0f, 0.0f, 0.0f );
	return;
	}
	*/

	if (iFrame == 0 || Reset)
	{
		// change the 0.99f to get different amounts of particles spawning!
		float f = hash1(fragCoord.x*13.0f + 1.0f + 0.1f * iTime + hash1(fragCoord.y*73.1f));
		if (f > 0.99f && mod_f(_floor(fragCoord.x),2.0f) == 0.0f && mod_f(_floor(fragCoord.y),2.0f) == 0.0f)
			f = 3.0f;
		else 
			f = 0.0f;
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

	float t2 = Cell(px + to_int2(0,-2), iResolution, iChannel0);
	float b2 = Cell(px + to_int2(0,2), iResolution, iChannel0);
	float l2 = Cell(px + to_int2(-2,0), iResolution, iChannel0);
	float r2 = Cell(px + to_int2(2,0), iResolution, iChannel0);   

	float tl = Cell(px + to_int2(-1,-1), iResolution, iChannel0);
	float tr = Cell(px + to_int2(1,-1), iResolution, iChannel0);
	float bl = Cell(px + to_int2(-1,1), iResolution, iChannel0);
	float br = Cell(px + to_int2(1,1), iResolution, iChannel0);

	float sum = t + b + l + r;

	// 2 up, 3 right, 4 down, 5 left

	/*
	if (iFrame % 60 == 0 && fragCoord.x > 0.5f * iResolution.x && fragCoord.x < 0.5f * iResolution.x + 1.
	 && fragCoord.y > 0.5f * iResolution.y && fragCoord.y < 0.5f * iResolution.y + 1.0f)
	e = 3.0f;
	else  */
	if (fragCoord.y > iMouse.y && fragCoord.y < iMouse.y + 1.0f
	 && fragCoord.x > iMouse.x && fragCoord.x < iMouse.x + 1.0f)
		e = 3.0f;
	else if (e > 1.0f || (((b == 2.0f && sum == 2.0f) || (l == 3.0f && sum == 3.0f)
	|| (t == 4.0f && sum == 4.0f) || (r == 5.0f && sum == 5.0f))))// && tl + tr + bl + br == 0.0f))
		e = 1.0f;
	else if (b2 == 2.0f)
		e = 2.0f;
	else if (l2 == 3.0f)
		e = 3.0f;
	else if (t2 == 4.0f)
		e = 4.0f;
	else if (r2 == 5.0f)
		e = 5.0f;
	else if (e <= 1.0f) // (A) get rid of me to keep the trail alive forever
		e -= 0.005f;

	float n = 0.0f;
	if (t > 0.0f && t <= 1.0f)
		n++;
	if (r > 0.0f && r <= 1.0f)
		n++;
	if (l > 0.0f && l <= 1.0f)
		n++;
	if (b > 0.0f && b <= 1.0f)
		n++;

	/*
	if (n == 0.0f && e > 1.0f)
	{
	int m = iFrame % 7;
	if (m < 2)
	e = 2.0f;
	else if ( m < 4)
	e = 5.0f;
	else if (m < 6)
	e = 4.0f;
	else 
	e = 3.0f;
	}
	*/

	if (n == 0.0f && e > 1.0f)
	{
		float e2 = 0.0f;
		//int L = int(100.0f * (1.0f - f(fragCoord.x / iResolution.x) * f(fragCoord.y / iResolution.y))); // change this value for bigger squares

		int L = (int)(250.0f * (1.0f - 0.5f * _cosf(0.25f * iTime)) * (1.0f - f(fract(2.0f * (fragCoord.x / iResolution.y - 0.375f))) * f(fract(2.0f * fragCoord.y / iResolution.y)))); // change this value for bigger squares
		//int L = 2;
		//int L = iFrame % 60; // 5
		int m = iFrame % (4 * L);
		if (m < L)
			e2 = 2.0f;
		else if ( m < 2 *L )
			e2 = 5.0f;
		else if (m < 3 * L)
			e2 = 4.0f;
		else
			e2 = 3.0f;

		if (mod_f(e - e2,2.0f) != 0.0f)
			e = e2;
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

__KERNEL__ void D2DWalkAutomataJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{
	CONNECT_CHECKBOX1(Option, 0);
  CONNECT_COLOR0(Color, 0.0f, 0.0f, 0.0f, 1.0f);
  CONNECT_SLIDER0(Level0, -1.0f, 1.0f, 0.8f);
  CONNECT_SLIDER1(Level1, -1.0f, 1.0f, 0.1f);
  
	fragCoord+=0.5f;

  //float x = texelFetch( iChannel0, to_int2(fragCoord), 0 ).x;
  float x = texture( iChannel0, fragCoord/iResolution ).x;
  //float x = texture( iChannel0, (make_float2(to_int2_cfloat(fragCoord))+0.5f)/iResolution ).x;

  if (x > 1.0f)
  {
    fragColor = to_float4(x,x,x,1.0f);
  }
  else
  {
    if(Option)
      fragColor = (Level0 + Level1 * _cosf( iTime)) * x * Color;
    else
      fragColor = (0.8f + 0.1f * _cosf( iTime)) * to_float4(x,0.0f,0.0f,Color.w);
  }
  fragColor.w = Color.w;
  
  SetFragmentShaderComputedColor(fragColor);
}