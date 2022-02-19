
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

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

/* skew constants for 3d simplex functions */
#define F3   0.3333333f
#define G3   0.1666667f

/* 3d simplex noise */
__DEVICE__ float simplex3d(float3 p) {
   /* 1.0f find current tetrahedron T and it's four vertices */
   /* s, s+i1, s+i2, s+1.0f - absolute skewed (integer) coordinates of T vertices */
   /* x, x1, x2, x3 - unskewed coordinates of p relative to each of T vertices*/
   
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



/* directional artifacts can be reduced by rotating each octave */
__DEVICE__ float simplex3d_fractal(float3 m) {
    /* const matrices for 3d rotation */
    const mat3 rot1 = to_mat3(-0.37f, 0.36f, 0.85f,-0.14f,-0.93f, 0.34f,0.92f, 0.01f,0.4f);
    const mat3 rot2 = to_mat3(-0.55f,-0.39f, 0.74f, 0.33f,-0.91f,-0.24f,0.77f, 0.12f,0.63f);
    const mat3 rot3 = to_mat3(-0.71f, 0.52f,-0.47f,-0.08f,-0.72f,-0.68f,-0.7f,-0.45f,0.56f);
  
    return   0.5333333f*simplex3d(mul_f3_mat3( m,rot1))
            +0.2666667f*simplex3d(mul_f3_mat3( 2.0f*m,rot2))
            +0.1333333f*simplex3d(mul_f3_mat3( 4.0f*m,rot3))
            +0.0666667f*simplex3d(8.0f*m);
}

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0


//map distribution functions to texture coordinates
//4 texels are used to store the 9 distribution functions in one cell
#define f0(_x,_y) texture(iChannel0, (to_float2(2*_x,2*_y)+0.5f)/iResolution).x;
#define f1(_x,_y) texture(iChannel0, (to_float2(2*_x,2*_y)+0.5f)/iResolution).y;
#define f2(_x,_y) texture(iChannel0, (to_float2(2*_x,2*_y)+0.5f)/iResolution).z;
#define f3(_x,_y) texture(iChannel0, (to_float2(2*_x+1,2*_y)+0.5f)/iResolution).x;
#define f4(_x,_y) texture(iChannel0, (to_float2(2*_x+1,2*_y)+0.5f)/iResolution).y;
#define f5(_x,_y) texture(iChannel0, (to_float2(2*_x+1,2*_y)+0.5f)/iResolution).z;
#define f6(_x,_y) texture(iChannel0, (to_float2(2*_x,2*_y+1)+0.5f)/iResolution).x;
#define f7(_x,_y) texture(iChannel0, (to_float2(2*_x,2*_y+1)+0.5f)/iResolution).y;
#define f8(_x,_y) texture(iChannel0, (to_float2(2*_x,2*_y+1)+0.5f)/iResolution).z;

#define G 0.01

__DEVICE__ float fbm( in float3 x )
{
    float H = 0.5f;
    float f = 1.0f;
    float a = 1.0f;
    float t = 0.0f;
    for( int i=0; i<10; i++ )
    {
        t += a*simplex3d(f*x);
        f *= 2.0f;
        a *= _exp2f(-H);
    }
    return t;
}

__KERNEL__ void CausticLbmFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{

    //relaxation time
    float w = 1.95f;
    //fragColor=texture(iChannel0, fragCoord/iResolution);
    int LatSizeX = (int)(iResolution.x/2.0f);
    int LatSizeY = (int)(iResolution.y/2.0f);
    //int LatSizeX = 200;
    //int LatSizeY = 200;
    //4 texels per voxel
    //all 4 pixels do the same computations
    int ix = (int)(_floor(fragCoord.x/2.0f));
    int iy = (int)(_floor(fragCoord.y/2.0f));
    if( ix >= LatSizeX || iy >= LatSizeY )
    {
        SetFragmentShaderComputedColor(fragColor);
        return;
    }
    int itx = (int)(fragCoord.x) - 2*ix;
    int ity = (int)(fragCoord.y) - 2*iy;
    float f0,f1,f2,f3,f4,f5,f6,f7,f8; //distribution functions
    float rho, vx, vy; //moments
    f0 = f0(ix,iy); //if 0, reinitialise
    if( (iFrame==0) || (f0==0.0f) ) //initialisation
    {
        rho = 1.0f;
        rho += 0.05f * fbm(to_float3_aw(fragCoord/iResolution.x, 0)*8.0f+8.0f);
        vx = vy = 0.0f;
        //add a small disk near the entrance
      //  if( distance(to_float2(LatSizeX/2,LatSizeY/2),to_float2(ix,iy)) < 10.0f )
      //      rho = 1.1f;
        float sq_term = (vx*vx+vy*vy);
        f0 = rho*(1.0f-10.0f/12.0f*G*rho-2.0f/3.0f*sq_term);
        f1 = rho*(1.0f/6.0f *G*rho+1.0f/3.0f *vx     +vx*vx/2.0f             -sq_term/6.0f);
        f2 = rho*(1.0f/6.0f *G*rho-1.0f/3.0f *vx     +vx*vx/2.0f             -sq_term/6.0f);
        f3 = rho*(1.0f/6.0f *G*rho+1.0f/3.0f *vy     +vy*vy/2.0f             -sq_term/6.0f);
        f4 = rho*(1.0f/6.0f *G*rho-1.0f/3.0f *vy     +vy*vy/2.0f             -sq_term/6.0f);
        f5 = rho*(1.0f/24.0f*G*rho+1.0f/12.0f*(vx+vy)+1.0f/8.0f*(vx+vy)*(vx+vy)-sq_term/24.0f);
        f6 = rho*(1.0f/24.0f*G*rho-1.0f/12.0f*(vx+vy)+1.0f/8.0f*(vx+vy)*(vx+vy)-sq_term/24.0f);
        f7 = rho*(1.0f/24.0f*G*rho-1.0f/12.0f*(vx-vy)+1.0f/8.0f*(vx-vy)*(vx-vy)-sq_term/24.0f);
        f8 = rho*(1.0f/24.0f*G*rho+1.0f/12.0f*(vx-vy)+1.0f/8.0f*(vx-vy)*(vx-vy)-sq_term/24.0f);
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
        vx = 1.0f/rho*(f1-f2+f5-f6-f7+f8);
        vy = 1.0f/rho*(f3-f4+f5-f6+f7-f8);
        //velocity cap for stability
        float norm = _sqrtf(vx*vx+vy*vy);
        if(norm>0.2f)
        {
            vx *= 0.2f/norm;
            vy *= 0.2f/norm;
        }

        //mouse motion
        float t = 1.5f + 0.0002f * iTime;
        for(int i = 0; i < 10; i++) {
            float2 m = iResolution * fract_f2(to_float2(fbm(to_float3(t,1,i)), fbm(to_float3(1,t,i))) * 2.0f + 0.5f);
            if(i == 0 && iMouse.z>0.0f) m = swi2(iMouse,x,y);
            float2 dx = m/2.0f - to_float2(ix,iy);
            float h = _expf(-dot(dx,dx)/16.0f);
            rho = _mix(rho, 0.8f, h);
            w = _mix(w, 1.0f, h);
        }

        float sq_term = (vx*vx+vy*vy);
        float f0eq = rho*(1.0f-10.0f/12.0f*G*rho-2.0f/3.0f*sq_term);
        float f1eq = rho*(1.0f/6.0f *G*rho+1.0f/3.0f *vx     +vx*vx/2.0f             -sq_term/6.0f);
        float f2eq = rho*(1.0f/6.0f *G*rho-1.0f/3.0f *vx     +vx*vx/2.0f             -sq_term/6.0f);
        float f3eq = rho*(1.0f/6.0f *G*rho+1.0f/3.0f *vy     +vy*vy/2.0f             -sq_term/6.0f);
        float f4eq = rho*(1.0f/6.0f *G*rho-1.0f/3.0f *vy     +vy*vy/2.0f             -sq_term/6.0f);
        float f5eq = rho*(1.0f/24.0f*G*rho+1.0f/12.0f*(vx+vy)+1.0f/8.0f*(vx+vy)*(vx+vy)-sq_term/24.0f);
        float f6eq = rho*(1.0f/24.0f*G*rho-1.0f/12.0f*(vx+vy)+1.0f/8.0f*(vx+vy)*(vx+vy)-sq_term/24.0f);
        float f7eq = rho*(1.0f/24.0f*G*rho-1.0f/12.0f*(vx-vy)+1.0f/8.0f*(vx-vy)*(vx-vy)-sq_term/24.0f);
        float f8eq = rho*(1.0f/24.0f*G*rho+1.0f/12.0f*(vx-vy)+1.0f/8.0f*(vx-vy)*(vx-vy)-sq_term/24.0f);
        //=== RELAX TOWARD EQUILIBRIUM ========================
        f0 = (1.0f-w) * f0 + w * f0eq;
        f1 = (1.0f-w) * f1 + w * f1eq;
        f2 = (1.0f-w) * f2 + w * f2eq;
        f3 = (1.0f-w) * f3 + w * f3eq;
        f4 = (1.0f-w) * f4 + w * f4eq;
        f5 = (1.0f-w) * f5 + w * f5eq;
        f6 = (1.0f-w) * f6 + w * f6eq;
        f7 = (1.0f-w) * f7 + w * f7eq;
        f8 = (1.0f-w) * f8 + w * f8eq;
    }
    
    
    if(itx==0 && ity==0)//stores f0,f1,f2
        swi3S(fragColor,x,y,z, to_float3(f0,f1,f2))
        //swi3(fragColor,x,y,z) = to_float3(1.0f,0.0f,0.0f);
    else if(itx==1 && ity==0)//stores f3,f4,f5
        swi3S(fragColor,x,y,z, to_float3(f3,f4,f5))
        //swi3(fragColor,x,y,z) = to_float3(0.0f,1.0f,0.0f);
    else if(itx==0 && ity==1)//stores f6,f7,f8
        swi3S(fragColor,x,y,z, to_float3(f6,f7,f8))
        //swi3(fragColor,x,y,z) = to_float3(0.0f,0.0f,1.0f);
    else //stores rho,vx,vy
        swi3S(fragColor,x,y,z, to_float3(rho,vx,vy))

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


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


__DEVICE__ float4 powers( float x ) { return to_float4(x*x*x, x*x, x, 1.0f); }



__DEVICE__ float4 spline( float x, float4 c0, float4 c1, float4 c2, float4 c3 )
{
    const float4 BS_A = to_float4(   3.0f,  -6.0f,   0.0f,  4.0f ) /  6.0f;
    const float4 BS_B = to_float4(  -1.0f,   6.0f, -12.0f,  8.0f ) /  6.0f;
    const float4 RE_A = to_float4(  21.0f, -36.0f,   0.0f, 16.0f ) / 18.0f;
    const float4 RE_B = to_float4(  -7.0f,  36.0f, -60.0f, 32.0f ) / 18.0f;
    const float4 CR_A = to_float4(   3.0f,  -5.0f,   0.0f,  2.0f ) /  2.0f;
    const float4 CR_B = to_float4(  -1.0f,   5.0f,  -8.0f,  4.0f ) /  2.0f;

    float4 ca = RE_A, cb = RE_B;
        
    // We could expand the powers and build a matrix instead (twice as many coefficients
    // would need to be stored, but it could be faster.
    return c0 * dot( cb, powers(x + 1.0f)) + 
           c1 * dot( ca, powers(x      )) +
           c2 * dot( ca, powers(1.0f - x)) +
           c3 * dot( cb, powers(2.0f - x));
}

//#define SAM(a,b) texelFetch(tex, 1 + 2 * (to_int2(i) + to_int2(a,b)), 0)
#define SAM(a,b) texture(tex, (make_float2(1 + 2 * (to_int2_cfloat(i) + to_int2(a,b)))+0.5f)/R )

__DEVICE__ float4 texture_Bicubic( __TEXTURE__ tex, float2 t, float2 res, float2 R )
{
    float2 p = res*t - 0.5f;
    float2 f = fract_f2(p);
    float2 i = _floor(p);

    return spline( f.y, spline( f.x, SAM(-1,-1), SAM( 0,-1), SAM( 1,-1), SAM( 2,-1)),
                        spline( f.x, SAM(-1, 0), SAM( 0, 0), SAM( 1, 0), SAM( 2, 0)),
                        spline( f.x, SAM(-1, 1), SAM( 0, 1), SAM( 1, 1), SAM( 2, 1)),
                        spline( f.x, SAM(-1, 2), SAM( 0, 2), SAM( 1, 2), SAM( 2, 2)));
}

__KERNEL__ void CausticLbmFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

    fragColor = texture_Bicubic(iChannel0, fragCoord/iResolution, iResolution/2.0f, iResolution);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer B' to iChannel0


// Approximating caustic intensity with Laplacian of water surface
// valid for gently curved surfaces
// https://doi.org/10.1088f/2040-8986/aa6c4e
// https://michaelberryphysics.files.wordpress.com/2013/06/berry497.pdf

#define R iResolution

__DEVICE__ float height(__TEXTURE__ iChannel0, float x, float y, float2 R) {
    return texture(iChannel0, to_float2(x,y)/R).x;
}

__KERNEL__ void CausticLbmFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

    float _x = fragCoord.x, _y = fragCoord.y;
    float i = 4.0f, _z = 200.0f;

    // view refraction
    float dx = height(iChannel0,_x+i,_y,iResolution) - height(iChannel0,_x-i,_y,iResolution);
    float dy = height(iChannel0,_x,_y+i,iResolution) - height(iChannel0,_x,_y-i,iResolution);
    _x += 5.0f * clamp(50.0f * dx, -1.0f, 1.0f) + 100.0f * (_x/R.x - 0.5f);
    _y += 5.0f * clamp(50.0f * dy, -1.0f, 1.0f) + 100.0f * (_y/R.y - 0.5f);

    float laplacian = -3.0f * height(iChannel0,_x, _y,iResolution)
                     + 0.5f * (height(iChannel0,_x+i, _y,iResolution) + height(iChannel0,_x, _y+i,iResolution) + height(iChannel0,_x-i, _y,iResolution) + height(iChannel0,_x, _y-i,iResolution))
                     + 0.25f * (height(iChannel0,_x+i, _y+i,iResolution) + height(iChannel0,_x-i, _y+i,iResolution) + height(iChannel0,_x-i, _y-i,iResolution) + height(iChannel0,_x+i, _y-i,iResolution));
    float3 n = to_float3(1.2f, 1.3f, 1.4f); // refractive index, with chromatic aberration

    swi3S(fragColor,x,y,z, (1.0f - (n - 1.0f) * _z * laplacian)*0.3f); // caustic intensity
    //swi3(fragColor,x,y,z) *= 0.3f;
    

    // border
    fragColor = _mix(fragColor, to_float4_s(1), smoothstep(15.0f, 5.0f, _x));
    fragColor = _mix(fragColor, to_float4_s(1), smoothstep(15.0f, 5.0f, _y));
    fragColor = _mix(fragColor, to_float4_s(1), smoothstep(R.x - 15.0f, R.x - 5.0f, _x));
    fragColor = _mix(fragColor, to_float4_s(1), smoothstep(R.y - 15.0f, R.y - 5.0f, _y));

    //swi3(fragColor,x,y,z) *= to_float3(0.8f,0.9f,1); // water absorption
    fragColor.x *= 0.8f;
    fragColor.y *= 0.9f;
    fragColor.z *= 1.0f;

    swi3S(fragColor,x,y,z, swi3(fragColor,x,y,z) + to_float3(1, 0.9f, 0.8f) * clamp(10.0f * (dx + dy), 0.0f, 1.0f)); // reflection

    fragColor = pow_f4(fragColor, to_float4_s(1.0f/2.2f)); // gamma


  SetFragmentShaderComputedColor(fragColor);
}