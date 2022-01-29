

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Library code is in the Common tab
// Demo of using it to reimplement Suture Fluid is in Buffer A

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord.xy / iResolution.xy;
    // apply Sobel filter to Buffer B
    vec4 dx = texGradX(iChannel0, uv);
    vec4 dy = texGradY(iChannel0, uv);
    fragColor = 10. * sqrt(dx*dx + dy*dy);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Suture Fluid

vec2 normz(vec2 x) {
	return x == vec2(0) ? vec2(0) : normalize(x);
}

mat2 rotate2d(float a) {
    return mat2(cos(a),-sin(a),
                sin(a),cos(a));
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord / iResolution.xy;

    if (iFrame < 10) {
        fragColor = vec4(noise(16.0 * uv + 1.1), noise(16.0 * uv + 2.2), noise(16.0 * uv + 3.3), 0);
        return;
    }

    float divergence = texture(iChannel0, uv).z;
    divergence += .3 * texLapl(iChannel0, uv).z; // divergence smoothing
    divergence += .2 * (texGradX(iChannel0, uv).x + texGradY(iChannel0, uv).y); // divergence update

    vec2 stepSize = 6. / iResolution.xy;
    vec2 velocity = texture(iChannel0, uv).xy;
    vec2 advected = texBlur(iChannel0, uv - stepSize * velocity).xy;
    advected += .05*texLapl(iChannel0, uv).xy;
    advected -= .05*texture(iChannel0, uv).xy * divergence;
    advected -= .80*texLapl(iChannel0, uv).z  * normz(velocity);

    float curl = texGradX(iChannel0, uv).y - texGradY(iChannel0, uv).x;
    advected *= rotate2d(2.4 * curl);

    if (length(advected) > 1.) advected = normalize(advected);
    divergence = clamp(divergence, -1., 1.);
    fragColor.xyz = mix(texture(iChannel0, uv).xyz, vec3(advected, divergence), .2); // update smoothing
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





// IQ's simplex noise:

// The MIT License
// Copyright © 2013 Inigo Quilez
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

vec2 hash( vec2 p ) // replace this by something better
{
	p = vec2( dot(p,vec2(127.1,311.7)),
			  dot(p,vec2(269.5,183.3)) );

	return -1.0 + 2.0*fract(sin(p)*43758.5453123);
}

float noise( in vec2 p )
{
    const float K1 = 0.366025404; // (sqrt(3)-1)/2;
    const float K2 = 0.211324865; // (3-sqrt(3))/6;

	vec2 i = floor( p + (p.x+p.y)*K1 );
	
    vec2 a = p - i + (i.x+i.y)*K2;
    vec2 o = step(a.yx,a.xy);    
    vec2 b = a - o + K2;
	vec2 c = a - 1.0 + 2.0*K2;

    vec3 h = max( 0.5-vec3(dot(a,a), dot(b,b), dot(c,c) ), 0.0 );

	vec3 n = h*h*h*h*vec3( dot(a,hash(i+0.0)), dot(b,hash(i+o)), dot(c,hash(i+1.0)));

    return dot( n, vec3(70.0) );
	
}

// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// Visualization of the system in Buffer A

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord / iResolution.xy;
    vec3 norm = normalize(texture(iChannel0, uv).xyz);
    fragColor.xyz = .5 + .6 * cross(norm, vec3(.5,-.4,.5)) + .1 * norm.z;
}