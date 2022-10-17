

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
vec3 pal( in float t, in vec3 a, in vec3 b, in vec3 c, in vec3 d ) {
    // https://iquilezles.org/articles/palettes
    return a + b*cos( 6.28318*(c*t+d) );
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord/iResolution.xy;

    // Time varying pixel color
    vec3 col = 0.5 + 0.5*cos(iTime+uv.xyx+vec3(0,2,4));
    
    float d = texture(iChannel0, uv).z;
    //d = mix(-1., 1., d);
    
    col = pal( d, vec3(0.5,0.5,0.5),vec3(0.5,0.5,0.5),vec3(1.0,1.0,1.0),vec3(0.0,0.10,0.20) );

    // Output to screen
    fragColor = vec4(col, 1.);
    //fragColor = texture(iChannel2, uv);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
/*
Buffers B and C calculate fluid pressure using the two-pass Poisson solver described in
    "Fast Eulerian Fluid Simulation In Games Using Poisson Filters"
    https://www.shahinrabbani.ca/torch2pd.html

Solutions are initialised with a Gaussian blur of the previous solution
*/

// https://youtu.be/_3eyPUyqluc?t=355
const float poisson_filter[7] = float[](
    .57843719174,
    .36519596949,
    .23187988879,
    .14529589353,
    .08816487385,
    .05184872885,
    .02906462467
);

float gaussian(float w, float s) {
    return exp(-(w*w) / (2.*s*s)) / (s * sqrt(radians(360.)));
}



// Hash without Sine
// MIT License...
/* Copyright (c)2014 David Hoskins.

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
SOFTWARE.*/

//----------------------------------------------------------------------------------------
//  1 out, 1 in...
float hash11(float p)
{
    p = fract(p * .1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

//----------------------------------------------------------------------------------------
//  1 out, 2 in...
float hash12(vec2 p)
{
	vec3 p3  = fract(vec3(p.xyx) * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

//----------------------------------------------------------------------------------------
//  1 out, 3 in...
float hash13(vec3 p3)
{
	p3  = fract(p3 * .1031);
    p3 += dot(p3, p3.zyx + 31.32);
    return fract((p3.x + p3.y) * p3.z);
}

//----------------------------------------------------------------------------------------
//  2 out, 1 in...
vec2 hash21(float p)
{
	vec3 p3 = fract(vec3(p) * vec3(.1031, .1030, .0973));
	p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.xx+p3.yz)*p3.zy);

}

//----------------------------------------------------------------------------------------
///  2 out, 2 in...
vec2 hash22(vec2 p)
{
	vec3 p3 = fract(vec3(p.xyx) * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx+33.33);
    return fract((p3.xx+p3.yz)*p3.zy);

}

//----------------------------------------------------------------------------------------
///  2 out, 3 in...
vec2 hash23(vec3 p3)
{
	p3 = fract(p3 * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx+33.33);
    return fract((p3.xx+p3.yz)*p3.zy);
}

//----------------------------------------------------------------------------------------
//  3 out, 1 in...
vec3 hash31(float p)
{
   vec3 p3 = fract(vec3(p) * vec3(.1031, .1030, .0973));
   p3 += dot(p3, p3.yzx+33.33);
   return fract((p3.xxy+p3.yzz)*p3.zyx); 
}


//----------------------------------------------------------------------------------------
///  3 out, 2 in...
vec3 hash32(vec2 p)
{
	vec3 p3 = fract(vec3(p.xyx) * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yxz+33.33);
    return fract((p3.xxy+p3.yzz)*p3.zyx);
}

//----------------------------------------------------------------------------------------
///  3 out, 3 in...
vec3 hash33(vec3 p3)
{
	p3 = fract(p3 * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yxz+33.33);
    return fract((p3.xxy + p3.yxx)*p3.zyx);

}

//----------------------------------------------------------------------------------------
// 4 out, 1 in...
vec4 hash41(float p)
{
	vec4 p4 = fract(vec4(p) * vec4(.1031, .1030, .0973, .1099));
    p4 += dot(p4, p4.wzxy+33.33);
    return fract((p4.xxyz+p4.yzzw)*p4.zywx);
    
}

//----------------------------------------------------------------------------------------
// 4 out, 2 in...
vec4 hash42(vec2 p)
{
	vec4 p4 = fract(vec4(p.xyxy) * vec4(.1031, .1030, .0973, .1099));
    p4 += dot(p4, p4.wzxy+33.33);
    return fract((p4.xxyz+p4.yzzw)*p4.zywx);

}

//----------------------------------------------------------------------------------------
// 4 out, 3 in...
vec4 hash43(vec3 p)
{
	vec4 p4 = fract(vec4(p.xyzx)  * vec4(.1031, .1030, .0973, .1099));
    p4 += dot(p4, p4.wzxy+33.33);
    return fract((p4.xxyz+p4.yzzw)*p4.zywx);
}

//----------------------------------------------------------------------------------------
// 4 out, 4 in...
vec4 hash44(vec4 p4)
{
	p4 = fract(p4  * vec4(.1031, .1030, .0973, .1099));
    p4 += dot(p4, p4.wzxy+33.33);
    return fract((p4.xxyz+p4.yzzw)*p4.zywx);
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

/* skew constants for 3d simplex functions */
const float F3 = 1./3.;
const float G3 = 1./6.;

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
	 d.x = dot(hash33(s) - 0.5, x);
	 d.y = dot(hash33(s + i1) - 0.5, x1);
	 d.z = dot(hash33(s + i2) - 0.5, x2);
	 d.w = dot(hash33(s + 1.0) - 0.5, x3);
	 
	 /* multiply d by w^4 */
	 w *= w;
	 w *= w;
	 d *= w;
	 
	 /* 3. return the sum of the four surflets */
	 return dot(d, vec4(52.0));
}



// https://iquilezles.org/articles/fbm

float fbm( in vec3 x, in float G, in int numOctaves )
{    
    float f = 1.0;
    float a = 1.0;
    float t = 0.0;
    for( int i=0; i<numOctaves; i++ )
    {
        t += a*simplex3d(f*x);
        f *= 2.0;
        a *= G;
    }
    return t;
}



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

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void interaction(in vec2 uv, inout vec2 velocity, inout float ink) {
    // this function describes any external forces on the fluid, for example:

    vec3 seed = vec3(uv, iTime / 10.);
    velocity += .05 * fbm(20. * seed, .5, 8);
    velocity *= .99;
    ink += (fbm(seed, .5, 8) + 1.) * 7e-3;
    ink = clamp(.99 * ink, 0., 1.);

    if (iMouse.z > 0.) {
        vec2 p = iMouse.xy / iResolution.xy;
        vec4 iMousePrev = texelFetch(iChannel3, ivec2(0,0), 0);
        vec2 v = iMouse.xy - iMousePrev.xy;
        if (length(v) > 1.) v = normalize(v);
        velocity += v * exp(-1000. * dot(uv-p, uv-p));
    }
}

vec3 fetch(vec2 uv) {
    // subtract pressure gradient from velocity
    return vec3(
        texture(iChannel0, uv).x - texGradX(iChannel2, uv).x,
        texture(iChannel0, uv).y - texGradY(iChannel2, uv).x,
        texture(iChannel0, uv).z);
}

vec3 advect(vec2 uv) {
    return fetch(uv - fetch(uv).xy / iResolution.xy);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord / iResolution.xy;
    vec3 r = advect(uv);
    interaction(uv, r.xy, r.z);
    fragColor = vec4(r, 1.);
}

// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// Poisson solver - horizontal pass

float rhs(vec2 pos) { // rhs of the poisson equation
    vec2 uv = pos / iResolution.xy;
    float divergence = texGradX(iChannel0, uv).x + texGradY(iChannel0, uv).y;
    return -divergence;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    float p = 0., g = 0.;
    for (int i = -6; i <= 6; i++) {
        vec2 pos = fragCoord + vec2(i,0);
        p += poisson_filter[abs(i)] * rhs(pos);
        g += gaussian(float(i), 3.) * texelFetch(iChannel2, ivec2(pos), 0).x;
    }
	fragColor = vec4(p, g, 0, 0);
}

// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// Poisson solver - vertical pass

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    float p = 0., g = 0.;
    for (int j = -6; j <= 6; j++) {
        vec4 pass1 = texelFetch(iChannel1, ivec2(fragCoord) + ivec2(0,j), 0);
        p += poisson_filter[abs(j)] * pass1.x;
        g += gaussian(float(j), 3.) * pass1.y;
    }
    fragColor = vec4(g + p);
}

// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    fragColor = iMouse;
}