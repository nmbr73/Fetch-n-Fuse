
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
	
    p = (p+r) % r;
    
    // fetch texel
   // return (texelFetch(iChannel0, p, 0 ).x > 0.5f ) ? 1 : 0;
   //return texelFetch(iChannel0, p, 0 ).x;
   return texture(iChannel0, (to_float2((float)p.x,(float)p.y))/iResolution ).x;
}

__DEVICE__ float hash1( float n )
{
    return fract(_sinf(n)*138.5453123f);
}

__DEVICE__ float S(float x)
{
	return step(0.5f,x);
}

__KERNEL__ void InteractivAutomataJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
	
	CONNECT_CHECKBOX0(Reset, 0);
	
	fragCoord+=0.5f;

	int2 px = to_int2_cfloat( fragCoord );
		
	if (Reset || iFrame == 0)
	{    
		float d = length(fragCoord / iResolution.y - to_float2(0.875f, 0.5f));
		float g = hash1(fragCoord.x * 13.0f  + 10.131f * iTime + 100.19414f + hash1(fragCoord.y*73.1f + 1931.1f));
		g = step(0.995f,g);
		fragColor = to_float4(g, 0.0f, 0.0f, 0.0f );
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

	// "average" of neighbours
	float k = _fmaxf(0.5f * (t + b), 0.5f * (l + r));

	// difference between "average" and center
	float j = _fabs(e - k);

	// this stuff makes a completely different automata
	/*
	float  count = 2.0f * (step(e,t) + step(e,b) + step(e,l) + step(e,r))  -4.0f;
	if (count >= -1.0f)
	e += 8.0f * k * e * e * j; // change 8.0f and 2.0f for better results
	else if (count < 0.0f)
	e -= 2.0f * j;
	*/

	// slightly different pattern:
	// if (e <= 0.05f) // 0.04f works well too
	// e =  k +  (30.0f + 10.0f * hash1(e)) * e * j;
	float m = 0.0f;
	float olde = e;

	/*
	float c = 0.5f * (1.0f + _cosf(1.0f * iTime));
	float ux = fragCoord.x / iResolution.y;
	float uy = fragCoord.y / iResolution.y;
	ux = 4.0f * ux * (1.0f-ux);
	uy = 4.0f * uy * (1.0f-uy);
	*/
	if (j < 0.15f && e < 0.9f)
		e = (7.2f + 0.2f * e) * j ;

	//e =  k * _fmaxf(e,1.0f - 16.0f * e * e * (1.0f-e) * (1.0f-e));
	e = k * _fmaxf(e, 1.0f - 8.0f * e * (1.0f-e));
	e = _fmaxf(1.001f * e, (0.88f + 1.4f * sign_f(k-e) * (0.5f + 0.9f * olde) * j) * k);

	e *= 1.0f - 0.9f * olde * e *  j * k;// 0.99f * (1.0f - 0.2f * e * j);

	float lth = length(fragCoord - swi2(iMouse,x,y));
	if (lth < 100.0f) // 200.0f * j // or k
		e *= _powf(lth / 100.0f,2.0f);
	 //e *= _powf(lth / (200.0f * j), 2.0f);

	e = _fmaxf(0.0f, _fminf(1.0f,e));
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

__KERNEL__ void InteractivAutomataJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

fragCoord+=0.5f;
	//fragColor = to_float4_aw( 1.0f - step(3.0f * texelFetch( iChannel0, to_int2(fragCoord), 0 ).x ,0.2f));
	//float x =  texelFetch( iChannel0, to_int2(fragCoord), 0 ).x;
	float x =  texture( iChannel0, (to_float2((int)fragCoord.x, (int)fragCoord.y)+0.5f)/iResolution ).x;

	x = _fminf(x,1.0f);
	x = 4.0f * x * (1.0f - x);
	float3 col = to_float3(34.0f,32.0f,52.0f) / 255.0f;
	float3 col2 = to_float3(69.0f,40.0f,60.0f) / 255.0f;
	float3 col3 = to_float3(172.0f,50.0f,50.0f) / 255.0f;
	float3 col4 = to_float3(223.0f,113.0f,38.0f) / 255.0f;
	float3 col5 = to_float3(255.0f,182.0f,45.0f) / 255.0f;
	float3 col6 = to_float3(251.0f,242.0f,54.0f) / 255.0f;

	//vec3 col6 = to_float3_s(1.0f);
	float m = 1.0f / 7.0f;
	if (x < m)
		fragColor = to_float4_aw(col,1.0f);
	else if (x < 2.0f * m)
		fragColor = to_float4_aw(col2,1.0f);
	else if (x < 3.0f * m)
		fragColor = to_float4_aw(col3,1.0f);
	else if (x < 4.0f * m)
		fragColor = to_float4_aw(col4,1.0f);
	else if (x < 5.0f * m)
		fragColor = to_float4_aw(col5,1.0f);
	else if (x < 6.0f * m)
		fragColor = to_float4_aw(col6,1.0f);
	else
		fragColor = to_float4_aw(col,1.0f);

	//fragColor = _fmaxf(fragColor,4.0f * x * (1.0f -x));
	//fragColor = to_float4(x,x,x,1.0f);


  SetFragmentShaderComputedColor(fragColor);
}