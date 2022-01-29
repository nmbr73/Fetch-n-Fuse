

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Approximating caustic intensity with Laplacian of water surface
// valid for gently curved surfaces
// https://doi.org/10.1088/2040-8986/aa6c4e
// https://michaelberryphysics.files.wordpress.com/2013/06/berry497.pdf

#define R iResolution

float height(float x, float y) {
    return texture(iChannel0, vec2(x,y)/iResolution.xy).x;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    float x = fragCoord.x, y = fragCoord.y;
    float i = 4., z = 200.;

    // view refraction
    float dx = height(x+i,y) - height(x-i,y);
    float dy = height(x,y+i) - height(x,y-i);
    x += 5. * clamp(50. * dx, -1., 1.) + 100. * (x/R.x - 0.5);
    y += 5. * clamp(50. * dy, -1., 1.) + 100. * (y/R.y - 0.5);

    float laplacian = -3. * height(x, y)
        + .5 * (height(x+i, y) + height(x, y+i) + height(x-i, y) + height(x, y-i))
        + .25 * (height(x+i, y+i) + height(x-i, y+i) + height(x-i, y-i) + height(x+i, y-i));
    vec3 n = vec3(1.2, 1.3, 1.4); // refractive index, with chromatic aberration
    fragColor.rgb = 1. - (n - 1.) * z * laplacian; // caustic intensity
    fragColor.rgb *= .3;

    // border
    fragColor = mix(fragColor, vec4(1), smoothstep(15., 5., x));
    fragColor = mix(fragColor, vec4(1), smoothstep(15., 5., y));
    fragColor = mix(fragColor, vec4(1), smoothstep(R.x - 15., R.x - 5., x));
    fragColor = mix(fragColor, vec4(1), smoothstep(R.y - 15., R.y - 5., y));

    fragColor.rgb *= vec3(.8,.9,1); // water absorption
    fragColor.rgb += vec3(1, .9, .8) * clamp(10. * (dx + dy), 0., 1.); // reflection

    fragColor = pow(fragColor, vec4(1./2.2)); // gamma
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
//map distribution functions to texture coordinates
//4 texels are used to store the 9 distribution functions in one cell
#define f0(x,y) texture(iChannel0, (vec2(2*x,2*y)+0.5)/iResolution.xy).r;
#define f1(x,y) texture(iChannel0, (vec2(2*x,2*y)+0.5)/iResolution.xy).g;
#define f2(x,y) texture(iChannel0, (vec2(2*x,2*y)+0.5)/iResolution.xy).b;
#define f3(x,y) texture(iChannel0, (vec2(2*x+1,2*y)+0.5)/iResolution.xy).r;
#define f4(x,y) texture(iChannel0, (vec2(2*x+1,2*y)+0.5)/iResolution.xy).g;
#define f5(x,y) texture(iChannel0, (vec2(2*x+1,2*y)+0.5)/iResolution.xy).b;
#define f6(x,y) texture(iChannel0, (vec2(2*x,2*y+1)+0.5)/iResolution.xy).r;
#define f7(x,y) texture(iChannel0, (vec2(2*x,2*y+1)+0.5)/iResolution.xy).g;
#define f8(x,y) texture(iChannel0, (vec2(2*x,2*y+1)+0.5)/iResolution.xy).b;

#define G 0.01

float fbm( in vec3 x )
{
    float H = .5;
    float f = 1.0;
    float a = 1.0;
    float t = 0.0;
    for( int i=0; i<10; i++ )
    {
        t += a*simplex3d(f*x);
        f *= 2.0;
        a *= exp2(-H);
    }
    return t;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    //relaxation time
    float w = 1.95;
    //fragColor=texture(iChannel0, fragCoord/iResolution.xy);
    int LatSizeX = int(iResolution.x/2.0);
    int LatSizeY = int(iResolution.y/2.0);
    //int LatSizeX = 200;
    //int LatSizeY = 200;
    //4 texels per voxel
    //all 4 pixels do the same computations
    int ix = int(floor(fragCoord.x/2.0));
    int iy = int(floor(fragCoord.y/2.0));
    if( ix >= LatSizeX || iy >= LatSizeY )
    {
        return;
    }
    int itx = int(fragCoord.x) - 2*ix;
    int ity = int(fragCoord.y) - 2*iy;
    float f0,f1,f2,f3,f4,f5,f6,f7,f8; //distribution functions
    float rho, vx, vy; //moments
    f0 = f0(ix,iy); //if 0, reinitialise
    if( (iFrame==0) || (f0==0.0) ) //initialisation
    {
        rho = 1.0;
        rho += .05 * fbm(vec3(fragCoord/iResolution.x, 0)*8.0+8.0);
        vx = vy = 0.0;
        //add a small disk near the entrance
      //  if( distance(vec2(LatSizeX/2,LatSizeY/2),vec2(ix,iy)) < 10.0 )
      //      rho = 1.1;
        float sq_term = (vx*vx+vy*vy);
        f0 = rho*(1.-10./12.*G*rho-2./3.*sq_term);
        f1 = rho*(1./6. *G*rho+1./3. *vx     +vx*vx/2.             -sq_term/6.);
        f2 = rho*(1./6. *G*rho-1./3. *vx     +vx*vx/2.             -sq_term/6.);
        f3 = rho*(1./6. *G*rho+1./3. *vy     +vy*vy/2.             -sq_term/6.);
        f4 = rho*(1./6. *G*rho-1./3. *vy     +vy*vy/2.             -sq_term/6.);
        f5 = rho*(1./24.*G*rho+1./12.*(vx+vy)+1./8.*(vx+vy)*(vx+vy)-sq_term/24.);
        f6 = rho*(1./24.*G*rho-1./12.*(vx+vy)+1./8.*(vx+vy)*(vx+vy)-sq_term/24.);
        f7 = rho*(1./24.*G*rho-1./12.*(vx-vy)+1./8.*(vx-vy)*(vx-vy)-sq_term/24.);
        f8 = rho*(1./24.*G*rho+1./12.*(vx-vy)+1./8.*(vx-vy)*(vx-vy)-sq_term/24.);
    }
    else //normal time-step
    {
        //=== STREAMING STEP (PERIODIC) =======================
        int xplus  = ((ix==LatSizeX-1) ? (0) : (ix+1));
        int xminus = ((ix==0) ? (LatSizeX-1) : (ix-1));
        int yplus  = ((iy==LatSizeY-1) ? (0) : (iy+1));
        int yminus = ((iy==0) ? (LatSizeY-1) : (iy-1));
        //f0 = f0( ix    ,iy    );
        f1 = f1( xminus,iy    );
        f2 = f2( xplus ,iy    );
        f3 = f3( ix    ,yminus);
        f4 = f4( ix    ,yplus );
        f5 = f5( xminus,yminus);
        f6 = f6( xplus ,yplus );
        f7 = f7( xplus ,yminus);
        f8 = f8( xminus,yplus );

        //=== COMPUTE MOMENTS =================================
        //density
        rho = f0+f1+f2+f3+f4+f5+f6+f7+f8;
        //velocity
        vx = 1./rho*(f1-f2+f5-f6-f7+f8);
        vy = 1./rho*(f3-f4+f5-f6+f7-f8);
        //velocity cap for stability
        float norm = sqrt(vx*vx+vy*vy);
        if(norm>0.2)
        {
            vx *= 0.2/norm;
            vy *= 0.2/norm;
        }

        //mouse motion
        float t = 1.5 + .0002 * iTime;
        for(int i = 0; i < 10; i++) {
            vec2 m = iResolution.xy * fract(vec2(fbm(vec3(t,1,i)), fbm(vec3(1,t,i))) * 2. + .5);
            if(i == 0 && iMouse.z>0.) m = iMouse.xy;
            vec2 dx = m/2. - vec2(ix,iy);
            float h = exp(-dot(dx,dx)/16.);
            rho = mix(rho, .8, h);
            w = mix(w, 1., h);
        }

        float sq_term = (vx*vx+vy*vy);
        float f0eq = rho*(1.-10./12.*G*rho-2./3.*sq_term);
        float f1eq = rho*(1./6. *G*rho+1./3. *vx     +vx*vx/2.             -sq_term/6.);
        float f2eq = rho*(1./6. *G*rho-1./3. *vx     +vx*vx/2.             -sq_term/6.);
        float f3eq = rho*(1./6. *G*rho+1./3. *vy     +vy*vy/2.             -sq_term/6.);
        float f4eq = rho*(1./6. *G*rho-1./3. *vy     +vy*vy/2.             -sq_term/6.);
        float f5eq = rho*(1./24.*G*rho+1./12.*(vx+vy)+1./8.*(vx+vy)*(vx+vy)-sq_term/24.);
        float f6eq = rho*(1./24.*G*rho-1./12.*(vx+vy)+1./8.*(vx+vy)*(vx+vy)-sq_term/24.);
        float f7eq = rho*(1./24.*G*rho-1./12.*(vx-vy)+1./8.*(vx-vy)*(vx-vy)-sq_term/24.);
        float f8eq = rho*(1./24.*G*rho+1./12.*(vx-vy)+1./8.*(vx-vy)*(vx-vy)-sq_term/24.);
        //=== RELAX TOWARD EQUILIBRIUM ========================
        f0 = (1.-w) * f0 + w * f0eq;
        f1 = (1.-w) * f1 + w * f1eq;
        f2 = (1.-w) * f2 + w * f2eq;
        f3 = (1.-w) * f3 + w * f3eq;
        f4 = (1.-w) * f4 + w * f4eq;
        f5 = (1.-w) * f5 + w * f5eq;
        f6 = (1.-w) * f6 + w * f6eq;
        f7 = (1.-w) * f7 + w * f7eq;
        f8 = (1.-w) * f8 + w * f8eq;
    }
    if(itx==0&&ity==0)//stores f0,f1,f2
        fragColor.rgb = vec3(f0,f1,f2);
        //fragColor.rgb = vec3(1.0,0.0,0.0);
    else if(itx==1&&ity==0)//stores f3,f4,f5
        fragColor.rgb = vec3(f3,f4,f5);
        //fragColor.rgb = vec3(0.0,1.0,0.0);
    else if(itx==0&&ity==1)//stores f6,f7,f8
        fragColor.rgb = vec3(f6,f7,f8);
        //fragColor.rgb = vec3(0.0,0.0,1.0);
    else //stores rho,vx,vy
        fragColor.rgb = vec3(rho,vx,vy);

}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
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

/* const matrices for 3d rotation */
const mat3 rot1 = mat3(-0.37, 0.36, 0.85,-0.14,-0.93, 0.34,0.92, 0.01,0.4);
const mat3 rot2 = mat3(-0.55,-0.39, 0.74, 0.33,-0.91,-0.24,0.77, 0.12,0.63);
const mat3 rot3 = mat3(-0.71, 0.52,-0.47,-0.08,-0.72,-0.68,-0.7,-0.45,0.56);

/* directional artifacts can be reduced by rotating each octave */
float simplex3d_fractal(vec3 m) {
    return   0.5333333*simplex3d(m*rot1)
			+0.2666667*simplex3d(2.0*m*rot2)
			+0.1333333*simplex3d(4.0*m*rot3)
			+0.0666667*simplex3d(8.0*m);
}

// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// The MIT License
// Copyright © 2014 Inigo Quilez
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


/*
Comparison of three bicubic filter kernels. B=Spline, Catmull-Rom and "recommended", as described
in this article: http://http.developer.nvidia.com/GPUGems/gpugems_ch24.html

Done the naive way with 16 samples rather than the smart way of performing bilinear filters. For
the fast way to do it, see Dave Hoskins' shader: https://www.shadertoy.com/view/4df3Dn


// Mitchell Netravali Reconstruction Filter
// B = 1,   C = 0   - cubic B-spline
// B = 1/3, C = 1/3 - recommended
// B = 0,   C = 1/2 - Catmull-Rom spline
//
// ca = {  12 - 9*B - 6*C,  -18 + 12*B + 6*C, 0, 6 - 2*B  } / 6;
// cb = {  -B - 6*C, 6*B + 30*C, -12*B - 48*C, 8*B + 24*C } / 6;
*/

//-----------------------------------------------------------------------------------------

const vec4 BS_A = vec4(   3.0,  -6.0,   0.0,  4.0 ) /  6.0;
const vec4 BS_B = vec4(  -1.0,   6.0, -12.0,  8.0 ) /  6.0;
const vec4 RE_A = vec4(  21.0, -36.0,   0.0, 16.0 ) / 18.0;
const vec4 RE_B = vec4(  -7.0,  36.0, -60.0, 32.0 ) / 18.0;
const vec4 CR_A = vec4(   3.0,  -5.0,   0.0,  2.0 ) /  2.0;
const vec4 CR_B = vec4(  -1.0,   5.0,  -8.0,  4.0 ) /  2.0;
        
vec4 powers( float x ) { return vec4(x*x*x, x*x, x, 1.0); }

vec4 ca = RE_A, cb = RE_B;

vec4 spline( float x, vec4 c0, vec4 c1, vec4 c2, vec4 c3 )
{
    // We could expand the powers and build a matrix instead (twice as many coefficients
    // would need to be stored, but it could be faster.
    return c0 * dot( cb, powers(x + 1.0)) + 
           c1 * dot( ca, powers(x      )) +
           c2 * dot( ca, powers(1.0 - x)) +
           c3 * dot( cb, powers(2.0 - x));
}

#define SAM(a,b) texelFetch(tex, 1 + 2 * (ivec2(i) + ivec2(a,b)), 0)

vec4 texture_Bicubic( sampler2D tex, vec2 t, vec2 res )
{
    vec2 p = res*t - 0.5;
    vec2 f = fract(p);
    vec2 i = floor(p);

    return spline( f.y, spline( f.x, SAM(-1,-1), SAM( 0,-1), SAM( 1,-1), SAM( 2,-1)),
                        spline( f.x, SAM(-1, 0), SAM( 0, 0), SAM( 1, 0), SAM( 2, 0)),
                        spline( f.x, SAM(-1, 1), SAM( 0, 1), SAM( 1, 1), SAM( 2, 1)),
                        spline( f.x, SAM(-1, 2), SAM( 0, 2), SAM( 1, 2), SAM( 2, 2)));
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    fragColor = texture_Bicubic(iChannel0, fragCoord/iResolution.xy, iResolution.xy/2.);
}