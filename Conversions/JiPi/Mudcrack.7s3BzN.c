
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

/* Texture Stencil Library https://www.shadertoy.com/view/ssBczm

The MIT License

Copyright (c) 2022 David A Roberts <https://davidar.io/>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

__DEVICE__ float4 texStencil(__TEXTURE2D__ ch, float2 uv, float coeff[9], float2 iResolution) {
    //float2 texel = 1.0f / to_float2(textureSize(ch, 0));
	float2 texel = 1.0f / iResolution;
    const float2 stencilOffset[9] = {
									to_float2(-1, 1), to_float2( 0, 1), to_float2( 1, 1),
									to_float2(-1, 0), to_float2( 0, 0), to_float2( 1, 0),
									to_float2(-1,-1), to_float2( 0,-1), to_float2( 1,-1)
                                    };
    float4 r = to_float4_s(0);
    for (int i = 0; i < 9; i++)
        r += coeff[i] * texture(ch, uv + texel * stencilOffset[i]);
    return r;
}

// Gaussian/binomial blur
// https://bartwronski.com/2021/10/31/practical-gaussian-filter-binomial-filter-and-small-sigma-gaussians/
__DEVICE__ float4 texBlur(__TEXTURE2D__ ch, float2 uv, float2 iResolution) {
    
	float par[9] = {
		        0.0625f, 0.125f, 0.0625f,
				0.125f,  0.25f,  0.125f,
				0.0625f, 0.125f, .0625f
                   };
	return texStencil(ch, uv, par,iResolution);
}

// Laplacian, optimal 9-point stencil
// https://docs.lib.purdue.edu/cgi/viewcontent.cgi?article=1928&context=cstech
__DEVICE__ float4 texLapl(__TEXTURE2D__ ch, float2 uv, float2 iResolution) {
    float par[9] = {
		        1.0f,   4.0f, 1.0f,
				4.0f, -20.0f, 4.0f,
				1.0f,   4.0f, 1.0f
                   };
	return texStencil(ch, uv, par,iResolution) / 6.0f;
}

// horizontal gradient (Sobel filter)
__DEVICE__ float4 texGradX(__TEXTURE2D__ ch, float2 uv, float2 iResolution) {
    float par[9] = {
		        -1.0f, 0.0f, 1.0f,
				-2.0f, 0.0f, 2.0f,
				-1.0f, 0.0f, 1.0f
                   };
	
	return texStencil(ch, uv, par,iResolution) / 8.0f;
}

// vertical gradient (Sobel filter)
__DEVICE__ float4 texGradY(__TEXTURE2D__ ch, float2 uv, float2 iResolution) {
    
	float par[9] = {
		         1.0f,  2.0f,  1.0f,
				 0.0f,  0.0f,  0.0f,
				-1.0f, -2.0f, -1.0f
                   };
	
	return texStencil(ch, uv, par,iResolution) / 8.0f;
}





/* https://www.shadertoy.com/view/XsX3zB
 *
 * The MIT License
 * Copyright © 2013 Nikita Miropolskiy
 * 
 * ( license has been changed from CCA-NC-SA 3.0f to MIT
 *
 *   but thanks for attributing your source code when deriving from this sample 
 *   with a following link: https://www.shadertoy.com/view/XsX3zB )
 *
 * ~
 * ~ if you're looking for procedural noise implementation examples you might 
 * ~ also want to look at the following shaders:
 * ~ 
 * ~ Noise Lab shader by candycat: https://www.shadertoy.com/view/4sc3z2
 * ~
 * ~ Noise shaders by iq:
 * ~     Value    Noise 2D, Derivatives: https://www.shadertoy.com/view/4dXBRH
 * ~     Gradient Noise 2D, Derivatives: https://www.shadertoy.com/view/XdXBRH
 * ~     Value    Noise 3D, Derivatives: https://www.shadertoy.com/view/XsXfRH
 * ~     Gradient Noise 3D, Derivatives: https://www.shadertoy.com/view/4dffRH
 * ~     Value    Noise 2D             : https://www.shadertoy.com/view/lsf3WH
 * ~     Value    Noise 3D             : https://www.shadertoy.com/view/4sfGzS
 * ~     Gradient Noise 2D             : https://www.shadertoy.com/view/XdXGW8
 * ~     Gradient Noise 3D             : https://www.shadertoy.com/view/Xsl3Dl
 * ~     Simplex  Noise 2D             : https://www.shadertoy.com/view/Msf3WH
 * ~     Voronoise: https://www.shadertoy.com/view/Xd23Dh
 * ~ 
 *
 */

/* discontinuous pseudorandom uniformly distributed in [-0.5f, +0.5]^3 */
__DEVICE__ float3 random3(float3 c) {
  float j = 4096.0f*_sinf(dot(c,to_float3(17.0f, 59.4f, 15.0f)));
  float3 r;
  r.z = fract(512.0f*j);
  j *= 0.125f;
  r.x = fract(512.0f*j);
  j *= 0.125f;
  r.y = fract(512.0f*j);
  return r-0.5f;
}



/* 3d simplex noise */
__DEVICE__ float simplex3d(float3 p) {
   /* 1.0f find current tetrahedron T and it's four vertices */
   /* s, s+i1, s+i2, s+1.0f - absolute skewed (integer) coordinates of T vertices */
   /* x, x1, x2, x3 - unskewed coordinates of p relative to each of T vertices*/
   
   /* skew constants for 3d simplex functions */
   const float F3 =  0.3333333f;
   const float G3 =  0.1666667f;
	   
   /* calculate s and x */
   float3 s = _floor(p + dot(p, to_float3_s(F3)));
   float3 x = p - s + dot(s, to_float3_s(G3));
   
   /* calculate i1 and i2 */
   float3 e = step(to_float3_s(0.0f), x - swi3(x,y,z,x));
   float3 i1 = e*(1.0f - swi3(e,z,x,y));
   float3 i2 = 1.0f - swi3(e,z,x,y)*(1.0f - e);
     
   /* x1, x2, x3 */
   float3 x1 = x - i1 + G3;
   float3 x2 = x - i2 + 2.0f*G3;
   float3 x3 = x - 1.0f + 3.0f*G3;
   
   /* 2.0f find four surflets and store them in d */
   float4 w, d;
   
   /* calculate surflet weights */
   w.x = dot(x, x);
   w.y = dot(x1, x1);
   w.z = dot(x2, x2);
   w.w = dot(x3, x3);
   
   /* w fades from 0.6f at the center of the surflet to 0.0f at the margin */
   w = _fmaxf(to_float4_s(0.6f) - w, to_float4_s(0.0f));
   
   /* calculate surflet components */
   d.x = dot(random3(s), x);
   d.y = dot(random3(s + i1), x1);
   d.z = dot(random3(s + i2), x2);
   d.w = dot(random3(s + 1.0f), x3);
   
   /* multiply d by w^4 */
   w *= w;
   w *= w;
   d *= w;

   /* 3.0f return the sum of the four surflets */
   return dot(d, to_float4_s(52.0f));
}

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0


// location of mass in given cell
__DEVICE__ float2 location(float2 cell, float2 iResolution,__TEXTURE2D__ iChannel0) {
    return cell + swi2(texture(iChannel0, cell / iResolution),z,w);
}

// spring fails when stretched too far
__DEVICE__ float stiffness(float dist, float2 uv) {
    return _mix(0.05f, 0.2f, uv.x) * smoothstep(1.4f, 0.6f, dist);
}

__KERNEL__ void MudcrackFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
	CONNECT_CHECKBOX0(Reset, 0);
	fragCoord+=0.5f;

    float2 uv = fragCoord / iResolution;

    // init
    if (iFrame < 10 || Reset) {
        fragColor = to_float4(simplex3d(to_float3_aw(10.0f*uv, 1)), simplex3d(to_float3_aw(10.0f*uv, 2)), simplex3d(to_float3_aw(10.0f*uv, 3)), simplex3d(to_float3_aw(10.0f*uv, 4)));
		SetFragmentShaderComputedColor(fragColor);
        return;
    }
    
    // diffusion of internal stress
    float2 stress = swi2(texBlur(iChannel0, uv,iResolution),x,y);
    
    // spring forces from neighbouring cells
    for (int i = -1; i <= 1; i++) for (int j = -1; j <= 1; j++) {
        if (i == 0 && j == 0) continue;
        float2 offset = normalize(to_float2(i,j));
        float2 disp = location(fragCoord + offset,iResolution,iChannel0) - location(fragCoord,iResolution,iChannel0);
        float2 force = stiffness(length(disp), uv) * disp; // Hooke's law
        stress += force;
    }
    
    // interaction
    float4 m = iMouse;
    float t = (float)(iFrame) / 200.0f;
    if (t < 1.0f) {
        m.x = iResolution.x * t;
        m.y = iResolution.y * (0.5f + 0.25f * _sinf(10.0f * t));
        swi2S(m,z,w, swi2(m,x,y));
    }
    if (m.z > 0.0f) {
        float2 dx = (fragCoord - swi2(m,x,y)) / iResolution.y;
        dx *= 10.0f;
        stress += dx * _expf(-dot(dx,dx));
    }
    
    // clamp stress
    if (length(stress) > 1.0f) stress = normalize(stress);
    
    // update mass locations
    float2 displacement = swi2(texBlur(iChannel0, uv,iResolution),z,w); // diffusion
    displacement *= 1.0f - 0.01f * length(displacement); // pull towards centre of cell
    displacement += 0.1f * stress; // push by spring forces
    
    fragColor = to_float4_f2f2(stress, displacement);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0



__KERNEL__ void MudcrackFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

	// colour reference https://fineartamerica.com/featured/crack-aidong-ning.html
	const float4 yellow = to_float4(0.96f, 0.86f, 0.59f, 1);
	const float4 orange = to_float4(0.86f, 0.62f, 0.45f, 1);
	const float4 blue = to_float4(0.41f, 0.46f, 0.55f, 1);
	const float4 black = to_float4(0.03f, 0.05f, 0.09f, 1);
	const float4 brown = to_float4(0.47f, 0.26f, 0.18f, 1);


    float2 uv = fragCoord / iResolution;
    
    float2 displacement = swi2(_tex2DVecN(iChannel0,uv.x,uv.y,15),z,w);
    float3 normal = normalize(to_float3_aw(-displacement/3.0f, 1));
    float lighting = dot(normal, normalize(to_float3(1,1,0)));
    
    float2 gradDisp = to_float2(texGradX(iChannel0, uv,iResolution).z, texGradY(iChannel0, uv,iResolution).w);
    float crackMask = length(_fmaxf(gradDisp, to_float2_s(0)));
    
    fragColor = yellow;
    fragColor = _mix(fragColor, orange, smoothstep(-1.0f, 0.0f, lighting));
    fragColor = _mix(fragColor, blue,   smoothstep( 0.0f, 1.0f, lighting));
    fragColor = _mix(fragColor, brown,  smoothstep( 0.0f, 0.4f, crackMask));
    fragColor = _mix(fragColor, black,  smoothstep( 0.4f, 0.7f, crackMask));


  SetFragmentShaderComputedColor(fragColor);
}