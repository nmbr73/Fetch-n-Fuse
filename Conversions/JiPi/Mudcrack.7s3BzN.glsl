

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// colour reference https://fineartamerica.com/featured/crack-aidong-ning.html
const vec4 yellow = vec4(0.96, 0.86, 0.59, 1);
const vec4 orange = vec4(0.86, 0.62, 0.45, 1);
const vec4 blue = vec4(0.41, 0.46, 0.55, 1);
const vec4 black = vec4(0.03, 0.05, 0.09, 1);
const vec4 brown = vec4(0.47, 0.26, 0.18, 1);

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord / iResolution.xy;
    
    vec2 displacement = texture(iChannel0, uv).zw;
    vec3 normal = normalize(vec3(-displacement/3., 1));
    float lighting = dot(normal, normalize(vec3(1,1,0)));
    
    vec2 gradDisp = vec2(texGradX(iChannel0, uv).z, texGradY(iChannel0, uv).w);
    float crackMask = length(max(gradDisp, vec2(0)));
    
    fragColor = yellow;
    fragColor = mix(fragColor, orange, smoothstep(-1., 0., lighting));
    fragColor = mix(fragColor, blue,   smoothstep( 0., 1., lighting));
    fragColor = mix(fragColor, brown,  smoothstep( 0., .4, crackMask));
    fragColor = mix(fragColor, black,  smoothstep( .4, .7, crackMask));
}

// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
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

vec4 texStencil(sampler2D ch, vec2 uv, float coeff[9]) {
    vec2 texel = 1. / vec2(textureSize(ch, 0));
    const vec2 stencilOffset[9] = vec2[](
        vec2(-1, 1), vec2( 0, 1), vec2( 1, 1),
        vec2(-1, 0), vec2( 0, 0), vec2( 1, 0),
        vec2(-1,-1), vec2( 0,-1), vec2( 1,-1)
    );
    vec4 r = vec4(0);
    for (int i = 0; i < 9; i++)
        r += coeff[i] * texture(ch, uv + texel * stencilOffset[i]);
    return r;
}

// Gaussian/binomial blur
// https://bartwronski.com/2021/10/31/practical-gaussian-filter-binomial-filter-and-small-sigma-gaussians/
vec4 texBlur(sampler2D ch, vec2 uv) {
    return texStencil(ch, uv, float[](
        .0625, .125, .0625,
        .125,  .25,  .125,
        .0625, .125, .0625
    ));
}

// Laplacian, optimal 9-point stencil
// https://docs.lib.purdue.edu/cgi/viewcontent.cgi?article=1928&context=cstech
vec4 texLapl(sampler2D ch, vec2 uv) {
    return texStencil(ch, uv, float[](
        1.,   4., 1.,
        4., -20., 4.,
        1.,   4., 1.
    )) / 6.;
}

// horizontal gradient (Sobel filter)
vec4 texGradX(sampler2D ch, vec2 uv) {
    return texStencil(ch, uv, float[](
        -1., 0., 1.,
        -2., 0., 2.,
        -1., 0., 1.
    )) / 8.;
}

// vertical gradient (Sobel filter)
vec4 texGradY(sampler2D ch, vec2 uv) {
    return texStencil(ch, uv, float[](
         1.,  2.,  1.,
         0.,  0.,  0.,
        -1., -2., -1.
    )) / 8.;
}





/* https://www.shadertoy.com/view/XsX3zB
 *
 * The MIT License
 * Copyright © 2013 Nikita Miropolskiy
 * 
 * ( license has been changed from CCA-NC-SA 3.0 to MIT
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

/* discontinuous pseudorandom uniformly distributed in [-0.5, +0.5]^3 */
vec3 random3(vec3 c) {
	float j = 4096.0*sin(dot(c,vec3(17.0, 59.4, 15.0)));
	vec3 r;
	r.z = fract(512.0*j);
	j *= .125;
	r.x = fract(512.0*j);
	j *= .125;
	r.y = fract(512.0*j);
	return r-0.5;
}

/* skew constants for 3d simplex functions */
const float F3 =  0.3333333;
const float G3 =  0.1666667;

/* 3d simplex noise */
float simplex3d(vec3 p) {
	 /* 1. find current tetrahedron T and it's four vertices */
	 /* s, s+i1, s+i2, s+1.0 - absolute skewed (integer) coordinates of T vertices */
	 /* x, x1, x2, x3 - unskewed coordinates of p relative to each of T vertices*/
	 
	 /* calculate s and x */
	 vec3 s = floor(p + dot(p, vec3(F3)));
	 vec3 x = p - s + dot(s, vec3(G3));
	 
	 /* calculate i1 and i2 */
	 vec3 e = step(vec3(0.0), x - x.yzx);
	 vec3 i1 = e*(1.0 - e.zxy);
	 vec3 i2 = 1.0 - e.zxy*(1.0 - e);
	 	
	 /* x1, x2, x3 */
	 vec3 x1 = x - i1 + G3;
	 vec3 x2 = x - i2 + 2.0*G3;
	 vec3 x3 = x - 1.0 + 3.0*G3;
	 
	 /* 2. find four surflets and store them in d */
	 vec4 w, d;
	 
	 /* calculate surflet weights */
	 w.x = dot(x, x);
	 w.y = dot(x1, x1);
	 w.z = dot(x2, x2);
	 w.w = dot(x3, x3);
	 
	 /* w fades from 0.6 at the center of the surflet to 0.0 at the margin */
	 w = max(0.6 - w, 0.0);
	 
	 /* calculate surflet components */
	 d.x = dot(random3(s), x);
	 d.y = dot(random3(s + i1), x1);
	 d.z = dot(random3(s + i2), x2);
	 d.w = dot(random3(s + 1.0), x3);
	 
	 /* multiply d by w^4 */
	 w *= w;
	 w *= w;
	 d *= w;
	 
	 /* 3. return the sum of the four surflets */
	 return dot(d, vec4(52.0));
}

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// location of mass in given cell
vec2 location(vec2 cell) {
    return cell + texture(iChannel0, cell / iResolution.xy).zw;
}

// spring fails when stretched too far
float stiffness(float dist, vec2 uv) {
    return mix(.05, .2, uv.x) * smoothstep(1.4, .6, dist);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord / iResolution.xy;

    // init
    if (iFrame < 10) {
        fragColor = vec4(simplex3d(vec3(10.*uv, 1)), simplex3d(vec3(10.*uv, 2)), simplex3d(vec3(10.*uv, 3)), simplex3d(vec3(10.*uv, 4)));
        return;
    }
    
    // diffusion of internal stress
    vec2 stress = texBlur(iChannel0, uv).xy;
    
    // spring forces from neighbouring cells
    for (int i = -1; i <= 1; i++) for (int j = -1; j <= 1; j++) {
        if (i == 0 && j == 0) continue;
        vec2 offset = normalize(vec2(i,j));
        vec2 disp = location(fragCoord + offset) - location(fragCoord);
        vec2 force = stiffness(length(disp), uv) * disp; // Hooke's law
        stress += force;
    }
    
    // interaction
    vec4 m = iMouse;
    float t = float(iFrame) / 200.;
    if (t < 1.) {
        m.x = iResolution.x * t;
        m.y = iResolution.y * (.5 + .25 * sin(10. * t));
        m.zw = m.xy;
    }
    if (m.z > 0.) {
        vec2 dx = (fragCoord.xy - m.xy) / iResolution.y;
        dx *= 10.;
        stress += dx * exp(-dot(dx,dx));
    }
    
    // clamp stress
    if (length(stress) > 1.) stress = normalize(stress);
    
    // update mass locations
    vec2 displacement = texBlur(iChannel0, uv).zw; // diffusion
    displacement *= 1. - .01 * length(displacement); // pull towards centre of cell
    displacement += .1 * stress; // push by spring forces
    
    fragColor = vec4(stress, displacement);
}
