
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
  //return texelFetch(iChannel0, p, 0 ).x;
  return texture(iChannel0, (to_float2(p.x,p.y)+0.5f)/iResolution ).x;
}

__DEVICE__ float hash1( float n )
{
	
    return fract(_sinf(n)*138.5453123f);
}

#ifdef XXX
__DEVICE__ bool key_down(int key) {
    return int(texelFetch(KEYBOARD, to_int2(key, 0), 0).x) == 1;
}
#endif

__DEVICE__ float S(float x)
{
	return step(0.5f,x);
}

__KERNEL__ void TreeAutomataJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
	CONNECT_CHECKBOX0(Reset, 0);
	CONNECT_CHECKBOX1(Tex, 0);
	CONNECT_SLIDER0(Level1, -1.0f, 5.0f, 1.0f);
	CONNECT_SLIDER1(Level2, -1.0f, 5.0f, 1.0f);
	CONNECT_SLIDER2(Level3, -1.0f, 5.0f, 1.0f);
	CONNECT_SLIDER3(Level4, -1.0f, 5.0f, 1.0f);
	CONNECT_SLIDER4(Level5, -1.0f, 5.0f, 1.0f);
	
	//Blending
  CONNECT_SLIDER5(Blend, 0.0f, 1.0f, 0.0f);
  CONNECT_SLIDER6(MulOffx, -10.0f, 10.0f, 1.0f);
  CONNECT_SLIDER7(MulOffy, -10.0f, 10.0f, 0.0f);
  CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
  CONNECT_POINT0(Par, 0.0f, 0.0f);
	
  fragCoord+=0.5f;

  int2 px = to_int2_cfloat( fragCoord );
    
   if (Reset || iFrame == 0 )
   {

	   
	   if (Tex)
	   {
		   float ratio = iResolution.x/iResolution.y;
		   float2 tuv = fragCoord/iResolution;
		   //tuv.x*=ratio;
		   float4 tex = texture(iChannel1, tuv);
		   if (tex.w > 0.0f) 
		   {
			   //fragColor = to_float4(tex.x+LevelTex,0,0,0);
			   //float2 U = fragCoord/iResolution.y ; 
			   //U.x -= 0.375f;
			   //U = step( fract_f2(3.0f * abs_f2(U-0.5f)),to_float2_s(3.0f / iResolution.y) );
			   float f = hash1(fragCoord.x*13.0f*Level1 + 10.131f*Level2 * iTime + hash1(fragCoord.y*73.1f*Level3));
			   fragColor = to_float4( step(0.5f,f), 0,0,0 );
		   }
	   }
	   else
	   {
		   float2 U = fragCoord/iResolution.y ; 
		   U.x -= 0.375f*Level4;
		   U = step( fract_f2(3.0f*Level5 * abs_f2(U-0.5f)),to_float2_s(3.0f*Level5 / iResolution.y) );
		   float f = _fmaxf(U.x,U.y) * hash1(fragCoord.x*13.0f*Level1 + 10.131f*Level2 * iTime + hash1(fragCoord.y*73.1f*Level3));
		   fragColor = to_float4( step(0.5f,f), 0,0,0 );
	   }
	   
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

	float tl = Cell(px + to_int2(-1,-1),iResolution,iChannel0);
	float tr = Cell(px + to_int2(1,-1),iResolution,iChannel0);
	float bl = Cell(px + to_int2(-1,1),iResolution,iChannel0);
	float br = Cell(px + to_int2(1,1),iResolution,iChannel0);

	float t2 = Cell(px + to_int2(0,-2),iResolution,iChannel0);
	float b2 = Cell(px + to_int2(0,2),iResolution,iChannel0);
	float l2 = Cell(px + to_int2(-2,0),iResolution,iChannel0);
	float r2 = Cell(px + to_int2(2,0),iResolution,iChannel0);   


	float q = hash1(fragCoord.x*13.0f+ 10.131f * iTime + hash1(fragCoord.y*73.1f));

	float2 uv = fragCoord/iResolution;
	float2 uv2 = _floor(1000.0f * uv) / 1000.0f;
	float q2 = hash1(uv2.x*13.0f + hash1(uv2.y*73.1f));

	if (q2 > 0.55f)
	{
		float sumNeighbours = S(t) + S(b) + S(l) + S(r) + S(tl) + S(tr) + S(bl) + S(br);

		if ( sumNeighbours == 1.0f && e < 1.0f)
			e =  step(0.5f, 0.6f * q);

		else if (e > 0.0f && e < 2.0f)
		{
			if (tl * tr > 0.0f || tl * bl > 0.0f || bl * br >0.0f || tr * br > 0.0f)
				e = 0.0f;
			else if (t * l > 0.0f || t * r > 0.0f || b * l > 0.0f || b * r > 0.0f)
				e = 0.0f;
			//else if (t2 * b2 > 0.0f || l2 * r2 > 0.0f)
			else if ((t2 > 0.0f && t <1.0f) || (b2 > 0.0f && b < 1.0f) || (r2 >0.0f && r < 1.0f) || (l2 >0.0f && l <1.0f))
				e = 0.0f;
			else if (sumNeighbours == 2.0f)
				e = 2.0f;
		}
	}
	else
		e = 0.0f;
	
	
	//Blending
	if (Blend > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(iChannel1,uv);

      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2)
		{    
	      float f = hash1(fragCoord.x*13.0f*Level1 + 10.131f*Level2 * iTime + hash1(fragCoord.y*73.1f*Level3));
          e = _mix(e,(f+MulOffy)*MulOffx,Blend);
		}

        if ((int)Modus&4)

          e = _mix(e,Par.x,Blend);
        
        
        if ((int)Modus&8)
          
          e = _mix( e,  (tex.x+MulOffy)*MulOffx, Blend);
          

        if ((int)Modus&16) 
          
          e = _mix(e,Par.y,Blend);
      }
      else
        if ((int)Modus&32) //Special
          {    
			  float f = hash1(fragCoord.x*13.0f*Level1 + 10.131f*Level2 * iTime + hash1(fragCoord.y*73.1f*Level3));
			  e = _mix(e,(f+MulOffy)*MulOffx,Blend);
		  }
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

__KERNEL__ void TreeAutomataJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
	CONNECT_COLOR0(Color, 3.0f, 3.0f, 3.0f, 1.0f);
	

	//fragColor = to_float4_aw( 1.0f - step(3.0f * texelFetch( iChannel0, to_int2(fragCoord), 0 ).x ,0.5f));
	//fragColor = to_float4_aw(3.0f * texelFetch( iChannel0, to_int2(fragCoord), 0 ).x );
	//fragColor = 3.0f * texture( iChannel0, (to_float2(to_int2_cfloat(fragCoord))+0.5f)/iResolution ).x;
	fragColor = Color * texture( iChannel0, (to_float2((int)fragCoord.x,(int)fragCoord.y)+0.5f)/iResolution ).x;

  SetFragmentShaderComputedColor(fragColor);
}