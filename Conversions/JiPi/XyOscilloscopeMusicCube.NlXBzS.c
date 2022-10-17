
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


/*
    MIT License

    Copyright (c) 2022 shyshokayu

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the Software), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, andor sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED AS IS, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

/*
    Feel free to make your own music/art with this shader,
    but don't forget to provide credit to me for making this framework.
*/

#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define ceil_f3(a) to_float3(_ceil((a).x), _ceil((a).y), _ceil((a).z))


  __DEVICE__ inline mat3 mul_f_mat3( float B, mat3 A)  
  {  
  return to_mat3_f3(A.r0 * B, A.r1 * B, A.r2 * B);  
  } 

  __DEVICE__ inline mat3 inverse( mat3 A)  
  {  
   mat3 R;  
   float result[3][3];  
   float a[3][3] = {{A.r0.x, A.r0.y, A.r0.z},  
					{A.r1.x, A.r1.y, A.r1.z},  
					{A.r2.x, A.r2.y, A.r2.z}};  
     
   float det = a[0][0] * a[1][1] * a[2][2]  
			 + a[0][1] * a[1][2] * a[2][0]  
			 + a[0][2] * a[1][0] * a[2][1]  
			 - a[2][0] * a[1][1] * a[0][2]  
			 - a[2][1] * a[1][2] * a[0][0]  
			 - a[2][2] * a[1][0] * a[0][1];  
   if( det != 0.0 )  
   {  
	   result[0][0] = a[1][1] * a[2][2] - a[1][2] * a[2][1];  
	   result[0][1] = a[2][1] * a[0][2] - a[2][2] * a[0][1];  
	   result[0][2] = a[0][1] * a[1][2] - a[0][2] * a[1][1];  
	   result[1][0] = a[2][0] * a[1][2] - a[1][0] * a[2][2];  
	   result[1][1] = a[0][0] * a[2][2] - a[2][0] * a[0][2];  
	   result[1][2] = a[1][0] * a[0][2] - a[0][0] * a[1][2];  
	   result[2][0] = a[1][0] * a[2][1] - a[2][0] * a[1][1];  
	   result[2][1] = a[2][0] * a[0][1] - a[0][0] * a[2][1];  
	   result[2][2] = a[0][0] * a[1][1] - a[1][0] * a[0][1];  
		 
	   R = to_mat3_f3(make_float3(result[0][0], result[0][1], result[0][2]),   
	   make_float3(result[1][0], result[1][1], result[1][2]), make_float3(result[2][0], result[2][1], result[2][2]));  
	   return mul_f_mat3( 1.0f / det, R);  
   }  
   R = to_mat3_f3(make_float3(1.0f, 0.0f, 0.0f), make_float3(0.0f, 1.0f, 0.0f), make_float3(0.0f, 0.0f, 1.0f));  
   return R;  
  } 



#define VISUAL_ITERATIONS 300 // Number of iterations to draw the whole line for the current frame
#define VISUAL_DURATION 0.02f // How long the line should last for a frame

// Mathematical functions
#define PI 3.1415926535897932384626433832795f
#define TAU (PI * 2.0f)

#define map(a, b, x) (((x) - (a)) / ((b) - (a)))
#define _saturatef(x) clamp(x, 0.0f, 1.0f)
#define linearstep(a, b, x) _saturatef(map(a, b, x))
#define cmix(a, b, x) _mix(a, b, _saturatef(x))

#define steprange(a, b, t) (step(a, t) * step(t, b))

#define distanceSqr(v) dot(v, v)

__DEVICE__ mat2 rot(float r) {
    float s = _sinf(r), c = _cosf(r);
    return to_mat2(c, -s, s, c);
}

#define rote(r) rot(r * PI)

__DEVICE__ mat3 rotX(float r) {
    float s = _sinf(r), c = _cosf(r);
    return to_mat3(
        1.0f, 0.0f, 0.0f,
        0.0f, c  , -s ,
        0.0f, s  , c
    );
}

__DEVICE__ mat3 rotY(float r) {
    float s = _sinf(r), c = _cosf(r);
    return to_mat3(
        c  , 0.0f, -s ,
        0.0f, 1.0f, 0.0f,
        s  , 0.0f, c
    );
}

__DEVICE__ mat3 rotZ(float r) {
    float s = _sinf(r), c = _cosf(r);
    return to_mat3(
        c  , -s , 0.0f,
        s  , c  , 0.0f,
        0.0f, 0.0f, 1.0
    );
}

__DEVICE__ mat3 roteX(float r) {
    return rotX(r * PI);
}

__DEVICE__ mat3 roteY(float r) {
    return rotY(r * PI);
}

__DEVICE__ mat3 roteZ(float r) {
    return rotZ(r * PI);
}

__DEVICE__ mat4 matrixTransform(float x, float y, float z) {
    return to_mat4(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        x  , y  , z  , 1.0
    );
}

__DEVICE__ mat4 matrixTransform(float3 p) {
    return to_mat4(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        p.x, p.y, p.z, 1.0
    );
}

__DEVICE__ mat4 matrixScale(float x, float y, float z) {
    return to_mat4(
        x  , 0.0f, 0.0f, 0.0f,
        0.0f, y  , 0.0f, 0.0f,
        0.0f, 0.0f, z  , 0.0f,
        0.0f, 0.0f, 0.0f, 1.0
    );
}

__DEVICE__ mat4 matrixScale(float3 p) {
    return to_mat4(
        p.x, 0.0f, 0.0f, 0.0f,
        0.0f, p.y, 0.0f, 0.0f,
        0.0f, 0.0f, p.z, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0
    );
}
__DEVICE__ float easeIn(float _x) {
    return _x * _x;
}

__DEVICE__ float easeOut(float _x) {
    return 1.0f - easeIn(1.0f - _x);
}

__DEVICE__ float easeInOut(float _x) {
    return _mix(easeIn(_x), easeOut(_x), _x);
}

__DEVICE__ float easeIn(float a, float b, float _x) {
    return easeIn(linearstep(a, b, _x));
}

__DEVICE__ float easeOut(float a, float b, float _x) {
    return easeOut(linearstep(a, b, _x));
}

__DEVICE__ float easeInOut(float a, float b, float _x) {
    return easeInOut(linearstep(a, b, _x));
}

__DEVICE__ float randomnoise11(float p) {
    return fract(_sinf(p * 12.9898f) * 43758.5453123f);
}

__DEVICE__ float2 randomnoise12(float p) {
    float _x = randomnoise11(p);
    return to_float2(_x, randomnoise11(p + _x));
}

// Musical functions
#define pitch(x) (_powf(1.059460646483f, x) * 440.0f)

#define sine(x) _sinf((x) * PI)
#define cosine(x) _cosf((x) * PI)
#define square(x) ((fract((x) * 0.5f) > 0.5f) ? -1.0f : 1.0f)
#define cosquare(x) ((fract(((x) + 0.5f) * 0.5f) > 0.5f) ? -1.0f : 1.0f)
#define saw(x) ((fract((x) * 0.5f) * 2.0f) - 1.0f)
#define cosaw(x) ((fract(((x) + 0.5f) * 0.5f) * 2.0f) - 1.0f)
#define cotriangle(x) (-(_fabs(fract((x) * 0.5f) - 0.5f) * 4.0f) + 1.0f)
#define triangle(x) cotriangle((x) + 0.5f)

// Some musical functions made specifically for this preview
// Probably stupidly impractical but whatever...
__DEVICE__ float3 box(float t) {
    const float3 points[] = {
        to_float3(-1.0f, -1.0f, -1.0f),
        to_float3(-1.0f, -1.0f, 1.0f),
        to_float3(-1.0f, -1.0f, 1.0f),
        to_float3(-1.0f, 1.0f, 1.0f),
        to_float3(-1.0f, 1.0f, 1.0f),
        to_float3(-1.0f, 1.0f, -1.0f),
        to_float3(-1.0f, 1.0f, -1.0f),
        to_float3(-1.0f, -1.0f, -1.0f),
        
        to_float3(1.0f, -1.0f, -1.0f),
        to_float3(1.0f, -1.0f, 1.0f),
        to_float3(1.0f, -1.0f, 1.0f),
        to_float3(1.0f, 1.0f, 1.0f),
        to_float3(1.0f, 1.0f, 1.0f),
        to_float3(1.0f, 1.0f, -1.0f),
        to_float3(1.0f, 1.0f, -1.0f),
        to_float3(1.0f, -1.0f, -1.0f),
        
        to_float3(-1.0f, -1.0f, -1.0f),
        to_float3(1.0f, -1.0f, -1.0f),
        to_float3(1.0f, -1.0f, 1.0f),
        to_float3(-1.0f, -1.0f, 1.0f),
        to_float3(-1.0f, 1.0f, 1.0f),
        to_float3(1.0f, 1.0f, 1.0f),
        to_float3(1.0f, 1.0f, -1.0f),
        to_float3(-1.0f, 1.0f, -1.0f)
    };
    
    //int i = ((int)(t) * 2) % (points.length());
    int i = ((int)(t) * 2) % (sizeof(points)/12);
    
    return _mix(points[i], points[i + 1], fract(t));
}

__DEVICE__ float3 tetrisT(float t) {
    const float3 points[] = {
        to_float3(1.5f, -1.0f, -0.5f),
        to_float3(-1.5f, -1.0f, -0.5f),
        to_float3(-1.5f, -1.0f, -0.5f),
        to_float3(-1.5f, 0.0f, -0.5f),
        to_float3(-1.5f, 0.0f, -0.5f),
        to_float3(-0.5f, 0.0f, -0.5f),
        to_float3(-0.5f, 0.0f, -0.5f),
        to_float3(-0.5f, 1.0f, -0.5f),
        to_float3(-0.5f, 1.0f, -0.5f),
        to_float3(0.5f, 1.0f, -0.5f),
        to_float3(0.5f, 1.0f, -0.5f),
        to_float3(0.5f, 0.0f, -0.5f),
        to_float3(0.5f, 0.0f, -0.5f),
        to_float3(1.5f, 0.0f, -0.5f),
        to_float3(1.5f, 0.0f, -0.5f),
        to_float3(1.5f, -1.0f, -0.5f),
        
        to_float3(1.5f, -1.0f, 0.5f),
        to_float3(-1.5f, -1.0f, 0.5f),
        to_float3(-1.5f, -1.0f, 0.5f),
        to_float3(-1.5f, 0.0f, 0.5f),
        to_float3(-1.5f, 0.0f, 0.5f),
        to_float3(-0.5f, 0.0f, 0.5f),
        to_float3(-0.5f, 0.0f, 0.5f),
        to_float3(-0.5f, 1.0f, 0.5f),
        to_float3(-0.5f, 1.0f, 0.5f),
        to_float3(0.5f, 1.0f, 0.5f),
        to_float3(0.5f, 1.0f, 0.5f),
        to_float3(0.5f, 0.0f, 0.5f),
        to_float3(0.5f, 0.0f, 0.5f),
        to_float3(1.5f, 0.0f, 0.5f),
        to_float3(1.5f, 0.0f, 0.5f),
        to_float3(1.5f, -1.0f, 0.5f),
        
        
        to_float3(1.5f, -1.0f, -0.5f),
        to_float3(1.5f, -1.0f, 0.5f),
        to_float3(-1.5f, -1.0f, 0.5f),
        to_float3(-1.5f, -1.0f, -0.5f),
        to_float3(-1.5f, 0.0f, -0.5f),
        to_float3(-1.5f, 0.0f, 0.5f),
        to_float3(-0.5f, 0.0f, 0.5f),
        to_float3(-0.5f, 0.0f, -0.5f),
        to_float3(-0.5f, 1.0f, -0.5f),
        to_float3(-0.5f, 1.0f, 0.5f),
        to_float3(0.5f, 1.0f, 0.5f),
        to_float3(0.5f, 1.0f, -0.5f),
        to_float3(0.5f, 0.0f, -0.5f),
        to_float3(0.5f, 0.0f, 0.5f),
        to_float3(1.5f, 0.0f, 0.5f),
        to_float3(1.5f, 0.0f, -0.5f)
    };
    
    //int i = ((int)(t) * 2) % (points.length());
    int i = ((int)(t) * 2) % (sizeof(points)/12);
    return _mix(points[i], points[i + 1], fract(t));
}

__DEVICE__ float3 hihats(float t) {
    float3 p = to_float3_s(0.0f);

    swi2S(p,x,y, ((randomnoise12(t) * 2.0f) - 1.0f) * 0.5f * fract(-t + 0.25f));
    swi2S(p,x,y, swi2(p,x,y) * step(fract(t * 2.0f), 0.125f) * step(fract(t * 0.5f), 0.85f));
    
    return p;
}

__DEVICE__ float3 kick(float t) {
    float tf = (t * 300.0f) - (20.0f * (fract(-t * 0.5f) * easeOut(0.0f, 0.1f, fract(t * 3.0f))));
    float3 p = to_float3(sine(tf), cosine(tf), 0.0f);
    p *= 3.0f * fract(-t * 3.0f);
    p *= smoothstep(0.0625f, 0.0f, t);
    p *= step(0.0f, t);
    return p;
}

__DEVICE__ float3 kicks(float t) {
    float3 p;
    
    float t2 = fract(t * 0.125f) * 8.0f;
    
    p += kick(t2);
    p += kick(t2 - 2.5f);
    
    p += kick(t2 - 4.0f);
    p += kick(t2 - 6.5f);
    
    return p;
}

__DEVICE__ float3 snare(float t) {
    float tf = (t * 400.0f) - (20.0f * (fract(-t * 0.5f) * easeOut(-0.015f, 0.04f, fract(t * 3.0f))));
    float3 p = to_float3(sine(tf), cosine(tf), 0.0f);
    swi2S(p,x,y, swi2(p,x,y) + ((randomnoise12(t) * 2.0f) - 1.0f) * 2.0f * fract(t * 3.0f));
    p *= 3.0f * fract(-t * 4.0f);
    p *= smoothstep(0.0625f * 4.0f, 0.0f, t);
    p *= step(0.0f, t);
    return p;
}

__DEVICE__ float3 snare2(float t) {
    float tf = (t * 1200.0f) - (20.0f * (fract(-t * 0.5f) * easeOut(-0.015f, 0.04f, fract(t * 3.0f))));
    float3 p = to_float3(sine(tf), cosine(tf), 0.0f);
    swi2S(p,x,y, swi2(p,x,y) + ((randomnoise12(t) * 2.0f) - 1.0f) * 2.0f * fract(t * 3.0f));
    p *= 3.0f * fract(-t * 4.0f);
    p *= smoothstep(0.0625f * 3.0f * 0.25f, 0.0f, t);
    p *= step(0.0f, t);
    return p;
}

__DEVICE__ float3 snares(float t) {
    float3 p;
    
    float t2 = fract((t * 0.25f) + 0.25f) * 4.0f;
    
    p += snare(t2);
    p += snare(t2 - 2.0f);
    
    return p;
}

__DEVICE__ float3 drums(float t) {
    float3 p;
    
    p += kicks(t);
    p += snares(t);
    
    return p;
}

__DEVICE__ float freqArpeggiator1(float t) {
    t = fract(t * 0.25f) * 4.0f;
    float s = ((triangle(_floor(fract(t * 1.5f) * 4.0f) * 0.25f) * 0.5f) + 0.5f) * 4.0f;
    s += ((triangle(_floor(t) * 0.5f) * 0.5f) + 0.5f) * 10.0f;
    s += 1.0f;
    
    return pitch(s);
}

// This is your sandbox:
__DEVICE__ float2 point(float t) {
    float3 p = to_float3_s(0.0f);
    
    t = fract(t / 24.0f) * 24.0f; // Loop
    
    float freq = freqArpeggiator1(t) * 3.0f;
    float tf = t * freq;
    
    p += box(tf);
    // What the hell
    p = _mix(p, to_float3(
        (easeInOut(easeInOut(easeInOut(easeInOut(easeInOut((p.x * 0.5f) + 0.5f))))) * 2.0f) - 1.0f,
        (easeInOut(easeInOut(easeInOut(easeInOut(easeInOut((p.y * 0.5f) + 0.5f))))) * 2.0f) - 1.0f,
        (easeInOut(easeInOut(easeInOut(easeInOut(easeInOut((p.z * 0.5f) + 0.5f))))) * 2.0f) - 1.0
    ), step(8.0f, t) * ((sine(t * 6.0f) * 0.5f) + 0.5f));
    
    float k = _floor(fract(t * 6.0f) * 2.0f) + 1.0f;
    p = _mix(p, round(p * k) / k, step(4.0f, fract(t * 0.125f) * 8.0f));
    
    p *= 0.6f;
    p *= easeOut(-0.25f, 0.5f, fract(t));
    
    p = mul_mat3_f3(inverse(mul_mat3_mat3(rotX(t) , rotY(t))) , p);
    
    p -= to_float3(0.0f, 0.0f, -1.5f);
    
    //swi2(p,x,y) /= p.z;
    p.x /= p.z;
    p.y /= p.z;
    
    {
        float3 k1 = round(p * 16.0f) / 16.0f;
        float3 k2 = ceil_f3(p * 16.0f) / 16.0f;
        float3 kf = p - k1;
        p = _mix(p, mix_f3(k1, k2, kf), step(2.0f, fract(t * 0.25f) * 4.0f));
    }
float zzzzzzzzzzzzzzzzzzzzzz;    
    p += step(8.0f, t) * hihats(t);
    p += step(16.0f, t) * drums(t);
    
    p *= 1.0f - (easeOut(7.75f, 8.0f, fract(t * 0.125f) * 8.0f) * fract(_floor(t * 16.0f) * 0.5f));
    
    p *= easeOut(0.0f, 0.5f, t);
    p *= easeOut(24.0f, 23.5f, t);
    
    return clamp(swi2(p,x,y), to_float2_s(-1.0f), to_float2_s(1.0f));
}

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------


// Samples coordinates every frame instead of sampling every iteration for every pixel for every frame
__KERNEL__ void XyOscilloscopeMusicCubeFuse__Buffer_A(float4 o, float2 u, float iTime, float2 iResolution, float iTimeDelta)
{
  
    u+=0.5f;

    // Estimate time based on pixel and calculate point
    u.y -= 0.5f; // For some reason this fixes the error where t > VISUAL_ITERATIONS is wrong
    float t = u.x + (u.y * iResolution.x);

    // Skip samples if outside used region
    if(t > float(VISUAL_ITERATIONS)) { // Not perfect but saves a bit of performance (not sure because of SIMD)
        o = to_float4(0.0f, 0.0f, 0.5f, 1.0f);
        SetFragmentShaderComputedColor(o);
        return;
    }
    
    t /= (float)(VISUAL_ITERATIONS);
    t *= VISUAL_DURATION;
    
    // Output point result
    swi2S(o,x,y, point((iTime - t) + iTimeDelta));
    swi2S(o,z,w, to_float2(0.0f, 1.0f));


  SetFragmentShaderComputedColor(o);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Bayer' to iChannel2
// Connect Image 'Preset: Keyboard' to iChannel3
// Connect Image 'Previsualization: Buffer A' to iChannel0


/*
    MIT License

    Copyright (c) 2022 shyshokayu

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the Software), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, andor sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED AS IS, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

/*
    Feel free to make your own music/art with this shader,
    but don't forget to provide credit to me for making this.
*/

/*
    ---| Go to Common tab to play with sound |---
*/

#define VISUAL_LINE_COLOR to_float3(0.45f, 1.0f, 0.4f)
#define VISUAL_LINE_BLUR 2.0f
#define VISUAL_LINE_BRIGHTNESS 1000.0f

__DEVICE__ float sliderPointLine(in float2 a, in float2 b, in float2 p) {
    float2 ab = b - a;
    return dot(p - a, ab) / dot(ab, ab);
}

__DEVICE__ float2 closestPointLine(in float2 a, in float2 b, in float d) {
    return _mix(a, b, _saturatef(d));
}

__DEVICE__ float2 closestPointLine(in float2 a, in float2 b, in float2 p) {
    return closestPointLine(a, b, sliderPointLine(a, b, p));
}

__DEVICE__ float distToLineSqr(in float2 a, in float2 b, in float2 p, in float k) {
    float2 d = p - closestPointLine(a, b, k);
    return dot(d, d);
}

__DEVICE__ float distToLineSqr(in float2 a, in float2 b, in float2 p) {
    float2 d = p - closestPointLine(a, b, p);
    return dot(d, d);
}

__DEVICE__ float distToLine(in float2 a, in float2 b, in float2 p) {
    return _sqrtf(distToLineSqr(a, b, p));
}

__DEVICE__ float2 pointTex(in float t, float2 R, __TEXTURE2D__ iChannel0) {
    // Estimate location based on texture coordinate
    t *= (float)(VISUAL_ITERATIONS);
    
    // Convert to int for texelFetch and for more reliable arithmetic
    int i = (int)(t);
    int j = i + 1;
    
    // Interpolation between two sampled points
    //int2 ip1 = to_int2(i % (int)(iChannelResolution[0].x), i / (int)(iChannelResolution[0].x));
    //int2 ip2 = to_int2(j % (int)(iChannelResolution[0].x), j / (int)(iChannelResolution[0].x));
    int2 ip1 = to_int2(i % (int)(R.x), i / (int)(R.x));
    int2 ip2 = to_int2(j % (int)(R.x), j / (int)(R.x));
    float2 p1 = swi2(texture(iChannel0, (make_float2(ip1)+0.5f)/R),x,y);
    float2 p2 = swi2(texture(iChannel0, (make_float2(ip2)+0.5f)/R),x,y);
    
    float2 p = _mix(p1, p2, fract(t));

    return p;
}

__DEVICE__ float2 pointTexi(in int i, float2 R, __TEXTURE2D__ iChannel0) {
    //int r = int(iChannelResolution[0].x);
    int r = (int)(R.x);
    return swi2(texture(iChannel0, (make_float2(make_int2(i % r, i / r))+0.5f)/R),x,y);
}

__DEVICE__ float imageVectorScopeLine(in float2 a, in float2 b, in float2 p) {
    float d = 0.01f + distanceSqr(a - b); // Emit less light if line is longer
    float s = _saturatef(sliderPointLine(swi2(a,x,y), swi2(b,x,y), p));
    float ld = distToLineSqr(swi2(a,x,y), swi2(b,x,y), p, s);
    return _fminf((0.00000004f / ((ld + (VISUAL_LINE_BLUR * 0.00001f)) * d)), 0.2f) * VISUAL_LINE_BRIGHTNESS;
}

__DEVICE__ float imageVectorScopeLines(in float2 uv, float2 R, __TEXTURE2D__ iChannel0) {
    float v = 0.0f; // Total value
    float lv = 0.0f; // Last value
    float cv = 0.0f; // Current value
    
    float2 cp;
    float2 lp = pointTexi(0,R,iChannel0);
    
    float fv = 1.0f / (float)(VISUAL_ITERATIONS);
    float k;
    
    for(int i = 1; i < VISUAL_ITERATIONS; i++) {
        cp = pointTexi(i,R,iChannel0); // Get the point
        k = (float)(i) * fv;
        cv = imageVectorScopeLine(lp, cp, uv) * (1.0f - k); // Set current value and multiply by time
        v += _fmaxf(lv, cv); // Eliminate circles between the lines
        lv = cv; // Set last value to current value
        lp = cp; // Set last point to current point
    }
    
    return v * fv;
}

__DEVICE__ float3 imageVectorScope(in float2 uv, float2 R, __TEXTURE2D__ iChannel0) {
    float3 col = to_float3_s(0.0f);

    // Lines
    float v = imageVectorScopeLines(uv,R,iChannel0);
    float3 emitCol = _powf(v * 0.5f, 0.5f) * VISUAL_LINE_COLOR;

    // Grid
    float3 surfaceCol = to_float3_s(1.0f);
    float gridV =  0.25f * (_fmaxf(max(step(fract(uv.x * 4.0f), 0.01f), step(1.0f - 0.01f, fract(uv.x * 4.0f))), _fmaxf(step(fract(uv.y * 4.0f), 0.01f), step(1.0f - 0.01f, fract(uv.y * 4.0f)))));
    surfaceCol = _mix(surfaceCol, to_float3_s(2.0f), gridV);

    // Ambient light                                                           Inner display tube light absorption?
    float3 lightCol = to_float3_s(0.045f) * _fmaxf(0.0f, dot(uv + 0.5f, to_float2(0.12f, 0.15f) * 4.0f)) * linearstep(-0.5f, 3.0f, length(uv));

    // Color compositing
    col = surfaceCol * lightCol;
    col += emitCol * (1.0f - gridV);

    // Cut out to make a square view
    col *= step(_fabs(uv.x), 1.0f) * step(_fabs(uv.y), 1.0f);
float uuuuuuuuuuuuuuuuuu;
    return col;
}

__DEVICE__ float imageOscilloscopeLine(in float2 a, in float2 b, in float2 p) {
    const float ta = 0.0f;
    const float tb = 0.004f;
    const float dta = ta * ta;
    const float dtb = tb * tb;
    float s = _saturatef(sliderPointLine(swi2(a,x,y), swi2(b,x,y), p));
    float ld = distToLineSqr(swi2(a,x,y), swi2(b,x,y), p, s);
    return linearstep(dtb, dta, ld);
}

__DEVICE__ float3 oscilloscopePoint(float x, float2 R, __TEXTURE2D__ iChannel0) {
    return to_float3(x, pointTex(1.0f - ((x * 0.5f) + 0.5f),R,iChannel0).x,pointTex(1.0f - ((x * 0.5f) + 0.5f),R,iChannel0).y);
}

__DEVICE__ float3 imageOscilloscope(in float2 uv, float2 R, __TEXTURE2D__ iChannel0) {
    float un = 1.0f / _fminf(iResolution.x, iResolution.y);
    
    float scale = 2.0f;
    un *= scale;
    
    const float segments = 250.0f;
    const float thickness = 1.0f / segments;
    const float gridThickness = 0.001f;
    const float gridInterval = 2.0f;
    float ip = round(uv.x * segments);
    float unit = 1.0f / segments;

    float3 p0 = oscilloscopePoint((ip - 2.0f) * unit,R,iChannel0),
         p1 = oscilloscopePoint((ip - 1.0f) * unit,R,iChannel0),
         p2 = oscilloscopePoint((ip) * unit,R,iChannel0),
         p3 = oscilloscopePoint((ip + 1.0f) * unit,R,iChannel0),
         p4 = oscilloscopePoint((ip + 2.0f) * unit,R,iChannel0);

    float dist1 = _fminf(min(distToLine(swi2(p0,x,y), swi2(p1,x,y), uv), distToLine(swi2(p1,x,y), swi2(p2,x,y), uv)),
                      _fminf(distToLine(swi2(p2,x,y), swi2(p3,x,y), uv), distToLine(swi2(p3,x,y), swi2(p4,x,y), uv))) - thickness;
    
    float dist2 = _fminf(min(distToLine(swi2(p0,x,z), swi2(p1,x,z), uv), distToLine(swi2(p1,x,z), swi2(p2,x,z), uv)),
                      _fminf(distToLine(swi2(p2,x,z), swi2(p3,x,z), uv), distToLine(swi2(p3,x,z), swi2(p4,x,z), uv))) - thickness;
    
    float3 col = to_float3_s(0.0f);
    
    col += to_float3(0.8f, 0.2f, 0.2f) * smoothstep(un, -un, dist1);
    col += to_float3(0.2f, 0.2f, 0.8f) * smoothstep(un, -un, dist2);

    // Cut out to make a square view
    col *= step(_fabs(uv.x), 1.0f) * step(_fabs(uv.y), 1.0f);
    
    return col;
}


__KERNEL__ void XyOscilloscopeMusicCubeFuse(float4 o, float2 u, float2 iResolution, float3 iChannelResolution[], sampler2D iChannel0, sampler2D iChannel2, sampler2D iChannel3)
{
    CONNECT_CHECKBOX0(VarE, 0);
    CONNECT_CHECKBOX1(VarQ, 0);
   
    u+=0.5f;

    float2 uv = (u - (0.5f * iResolution)) / _fminf(iResolution.x, iResolution.y);
    uv *= 2.0f;
    
    float3 col = _mix(
        _mix(
            imageVectorScope(uv,R,iChannel0),
            imageOscilloscope(uv,R,iChannel0),
            VarE //texelFetch(iChannel3, to_int2(69, 2), 0).x // nice
        ),
        ((swi3(texture(iChannel0, (u / iResolution)),x,y,z) * 0.5f) + 0.5f), // Show cached points visually
        VarQ //texelFetch(iChannel3, to_int2(81, 2), 0).x
    );

    col = pow_f3(col, to_float3_s(1.0f / 1.3f));

    // Full depth dithering, a way to make your images less bandy in low color ranges.
    // Since we're using floats here, we can use that as an opportunity to dither that to the common color format, 32bit rgba.
    float depth = 256.0f;
    float3 cd = col * depth;
    float3 di = _floor(cd);
    float3 df = cd - di;
    float3 ditheredCol = (step(texture(iChannel2, u * 0.125f).x + 0.00001f, df.x) + di) / depth;
    
    o = to_float4_aw(ditheredCol, 1.0f);

    // Just uncomment this line and see how much of a difference this dithering makes in the dark areas.
    //o = to_float4_aw(col, 1.0f);

  SetFragmentShaderComputedColor(o);
}
