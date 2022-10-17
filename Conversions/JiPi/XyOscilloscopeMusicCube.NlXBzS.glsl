

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
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

#define VISUAL_LINE_COLOR vec3(0.45, 1.0, 0.4)
#define VISUAL_LINE_BLUR 2.0
#define VISUAL_LINE_BRIGHTNESS 1000.0

float sliderPointLine(in vec2 a, in vec2 b, in vec2 p) {
    vec2 ab = b - a;
    return dot(p - a, ab) / dot(ab, ab);
}

vec2 closestPointLine(in vec2 a, in vec2 b, in float d) {
    return mix(a, b, saturate(d));
}

vec2 closestPointLine(in vec2 a, in vec2 b, in vec2 p) {
    return closestPointLine(a, b, sliderPointLine(a, b, p));
}

float distToLineSqr(in vec2 a, in vec2 b, in vec2 p, in float k) {
    vec2 d = p - closestPointLine(a, b, k);
    return dot(d, d);
}

float distToLineSqr(in vec2 a, in vec2 b, in vec2 p) {
    vec2 d = p - closestPointLine(a, b, p);
    return dot(d, d);
}

float distToLine(in vec2 a, in vec2 b, in vec2 p) {
    return sqrt(distToLineSqr(a, b, p));
}

vec2 pointTex(in float t) {
    // Estimate location based on texture coordinate
    t *= float(VISUAL_ITERATIONS);
    
    // Convert to int for texelFetch and for more reliable arithmetic
    int i = int(t);
    int j = i + 1;
    
    // Interpolation between two sampled points
    ivec2 ip1 = ivec2(i % int(iChannelResolution[0].x), i / int(iChannelResolution[0].x));
    ivec2 ip2 = ivec2(j % int(iChannelResolution[0].x), j / int(iChannelResolution[0].x));
    vec2 p1 = texelFetch(iChannel0, ip1, 0).xy;
    vec2 p2 = texelFetch(iChannel0, ip2, 0).xy;
    
    vec2 p = mix(p1, p2, fract(t));

    return p;
}

vec2 pointTexi(in int i) {
    int r = int(iChannelResolution[0].x);
    return texelFetch(iChannel0, ivec2(i % r, i / r), 0).xy;
}

float imageVectorScopeLine(in vec2 a, in vec2 b, in vec2 p) {
    float d = 0.01 + distanceSqr(a - b); // Emit less light if line is longer
    float s = saturate(sliderPointLine(a.xy, b.xy, p));
    float ld = distToLineSqr(a.xy, b.xy, p, s);
    return min((0.00000004 / ((ld + (VISUAL_LINE_BLUR * 0.00001)) * d)), 0.2) * VISUAL_LINE_BRIGHTNESS;
}

float imageVectorScopeLines(in vec2 uv) {
    float v = 0.0; // Total value
    float lv = 0.0; // Last value
    float cv = 0.0; // Current value
    
    vec2 cp;
    vec2 lp = pointTexi(0);
    
    float fv = 1.0 / float(VISUAL_ITERATIONS);
    float k;
    
    for(int i = 1; i < VISUAL_ITERATIONS; i++) {
        cp = pointTexi(i); // Get the point
        k = float(i) * fv;
        cv = imageVectorScopeLine(lp, cp, uv) * (1.0 - k); // Set current value and multiply by time
        v += max(lv, cv); // Eliminate circles between the lines
        lv = cv; // Set last value to current value
        lp = cp; // Set last point to current point
    }
    
    return v * fv;
}

vec3 imageVectorScope(in vec2 uv) {
    vec3 col = vec3(0.0);

    // Lines
    float v = imageVectorScopeLines(uv);
    vec3 emitCol = pow(v * 0.5, 0.5) * VISUAL_LINE_COLOR;

    // Grid
    vec3 surfaceCol = vec3(1.0);
    float gridV =  0.25 * (max(max(step(fract(uv.x * 4.0), 0.01), step(1.0 - 0.01, fract(uv.x * 4.0))), max(step(fract(uv.y * 4.0), 0.01), step(1.0 - 0.01, fract(uv.y * 4.0)))));
    surfaceCol = mix(surfaceCol, vec3(2.0), gridV);

    // Ambient light                                                           Inner display tube light absorption?
    vec3 lightCol = vec3(0.045) * max(0.0, dot(uv + 0.5, vec2(0.12, 0.15) * 4.0)) * linearstep(-0.5, 3.0, length(uv));

    // Color compositing
    col = surfaceCol * lightCol;
    col += emitCol * (1.0 - gridV);

    // Cut out to make a square view
    col *= step(abs(uv.x), 1.0) * step(abs(uv.y), 1.0);

    return col;
}

float imageOscilloscopeLine(in vec2 a, in vec2 b, in vec2 p) {
    const float ta = 0.0;
    const float tb = 0.004;
    const float dta = ta * ta;
    const float dtb = tb * tb;
    float s = saturate(sliderPointLine(a.xy, b.xy, p));
    float ld = distToLineSqr(a.xy, b.xy, p, s);
    return linearstep(dtb, dta, ld);
}

vec3 oscilloscopePoint(float x) {
    return vec3(x, pointTex(1.0 - ((x * 0.5) + 0.5)));
}

vec3 imageOscilloscope(in vec2 uv) {
    float un = 1.0 / min(iResolution.x, iResolution.y);
    
    float scale = 2.0;
    un *= scale;
    
    const float segments = 250.0;
    const float thickness = 1.0 / segments;
    const float gridThickness = 0.001;
    const float gridInterval = 2.0;
    float ip = round(uv.x * segments);
    float unit = 1.0 / segments;

    vec3 p0 = oscilloscopePoint((ip - 2.0) * unit),
         p1 = oscilloscopePoint((ip - 1.0) * unit),
         p2 = oscilloscopePoint((ip) * unit),
         p3 = oscilloscopePoint((ip + 1.0) * unit),
         p4 = oscilloscopePoint((ip + 2.0) * unit);

    float dist1 = min(min(distToLine(p0.xy, p1.xy, uv), distToLine(p1.xy, p2.xy, uv)),
                      min(distToLine(p2.xy, p3.xy, uv), distToLine(p3.xy, p4.xy, uv))) - thickness;
    
    float dist2 = min(min(distToLine(p0.xz, p1.xz, uv), distToLine(p1.xz, p2.xz, uv)),
                      min(distToLine(p2.xz, p3.xz, uv), distToLine(p3.xz, p4.xz, uv))) - thickness;
    
    vec3 col = vec3(0.0);
    
    col += vec3(0.8, 0.2, 0.2) * smoothstep(un, -un, dist1);
    col += vec3(0.2, 0.2, 0.8) * smoothstep(un, -un, dist2);

    // Cut out to make a square view
    col *= step(abs(uv.x), 1.0) * step(abs(uv.y), 1.0);
    
    return col;
}

void mainImage(out vec4 o, in vec2 u) {
    vec2 uv = (u - (0.5 * iResolution.xy)) / min(iResolution.x, iResolution.y);
    uv *= 2.0;
    
    vec3 col = mix(
        mix(
            imageVectorScope(uv),
            imageOscilloscope(uv),
            texelFetch(iChannel3, ivec2(69, 2), 0).x // nice
        ),
        ((texture(iChannel0, (u / iResolution.xy)).xyz * 0.5) + 0.5), // Show cached points visually
        texelFetch(iChannel3, ivec2(81, 2), 0).x
    );

    col = pow(col, vec3(1.0 / 1.3));

    // Full depth dithering, a way to make your images less bandy in low color ranges.
    // Since we're using floats here, we can use that as an opportunity to dither that to the common color format, 32bit rgba.
    float depth = 256.0;
    vec3 cd = col * depth;
    vec3 di = floor(cd);
    vec3 df = cd - di;
    vec3 ditheredCol = (step(texture(iChannel2, u * 0.125).x + 0.00001, df) + di) / depth;
    
    o = vec4(ditheredCol, 1.0);

    // Just uncomment this line and see how much of a difference this dithering makes in the dark areas.
    //o = vec4(col, 1.0);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
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

#define VISUAL_ITERATIONS 300 // Number of iterations to draw the whole line for the current frame
#define VISUAL_DURATION 0.02 // How long the line should last for a frame

// Mathematical functions
#define PI 3.1415926535897932384626433832795
#define TAU (PI * 2.0)

#define map(a, b, x) (((x) - (a)) / ((b) - (a)))
#define saturate(x) clamp(x, 0.0, 1.0)
#define linearstep(a, b, x) saturate(map(a, b, x))
#define cmix(a, b, x) mix(a, b, saturate(x))

#define steprange(a, b, t) (step(a, t) * step(t, b))

#define distanceSqr(v) dot(v, v)

mat2 rot(float r) {
    float s = sin(r), c = cos(r);
    return mat2(c, -s, s, c);
}

#define rote(r) rot(r * PI)

mat3 rotX(float r) {
    float s = sin(r), c = cos(r);
    return mat3(
        1.0, 0.0, 0.0,
        0.0, c  , -s ,
        0.0, s  , c
    );
}

mat3 rotY(float r) {
    float s = sin(r), c = cos(r);
    return mat3(
        c  , 0.0, -s ,
        0.0, 1.0, 0.0,
        s  , 0.0, c
    );
}

mat3 rotZ(float r) {
    float s = sin(r), c = cos(r);
    return mat3(
        c  , -s , 0.0,
        s  , c  , 0.0,
        0.0, 0.0, 1.0
    );
}

mat3 roteX(float r) {
    return rotX(r * PI);
}

mat3 roteY(float r) {
    return rotY(r * PI);
}

mat3 roteZ(float r) {
    return rotZ(r * PI);
}

mat4 matrixTransform(float x, float y, float z) {
    return mat4(
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        x  , y  , z  , 1.0
    );
}

mat4 matrixTransform(vec3 p) {
    return mat4(
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        p.x, p.y, p.z, 1.0
    );
}

mat4 matrixScale(float x, float y, float z) {
    return mat4(
        x  , 0.0, 0.0, 0.0,
        0.0, y  , 0.0, 0.0,
        0.0, 0.0, z  , 0.0,
        0.0, 0.0, 0.0, 1.0
    );
}

mat4 matrixScale(vec3 p) {
    return mat4(
        p.x, 0.0, 0.0, 0.0,
        0.0, p.y, 0.0, 0.0,
        0.0, 0.0, p.z, 0.0,
        0.0, 0.0, 0.0, 1.0
    );
}
float easeIn(float x) {
    return x * x;
}

float easeOut(float x) {
    return 1.0 - easeIn(1.0 - x);
}

float easeInOut(float x) {
    return mix(easeIn(x), easeOut(x), x);
}

float easeIn(float a, float b, float x) {
    return easeIn(linearstep(a, b, x));
}

float easeOut(float a, float b, float x) {
    return easeOut(linearstep(a, b, x));
}

float easeInOut(float a, float b, float x) {
    return easeInOut(linearstep(a, b, x));
}

float randomnoise11(float p) {
    return fract(sin(p * 12.9898) * 43758.5453123);
}

vec2 randomnoise12(float p) {
    float x = randomnoise11(p);
    return vec2(x, randomnoise11(p + x));
}

// Musical functions
#define pitch(x) (pow(1.059460646483, x) * 440.0)

#define sine(x) sin((x) * PI)
#define cosine(x) cos((x) * PI)
#define square(x) ((fract((x) * 0.5) > 0.5) ? -1.0 : 1.0)
#define cosquare(x) ((fract(((x) + 0.5) * 0.5) > 0.5) ? -1.0 : 1.0)
#define saw(x) ((fract((x) * 0.5) * 2.0) - 1.0)
#define cosaw(x) ((fract(((x) + 0.5) * 0.5) * 2.0) - 1.0)
#define cotriangle(x) (-(abs(fract((x) * 0.5) - 0.5) * 4.0) + 1.0)
#define triangle(x) cotriangle((x) + 0.5)

// Some musical functions made specifically for this preview
// Probably stupidly impractical but whatever...
vec3 box(float t) {
    const vec3[] points = vec3[](
        vec3(-1.0, -1.0, -1.0),
        vec3(-1.0, -1.0, 1.0),
        vec3(-1.0, -1.0, 1.0),
        vec3(-1.0, 1.0, 1.0),
        vec3(-1.0, 1.0, 1.0),
        vec3(-1.0, 1.0, -1.0),
        vec3(-1.0, 1.0, -1.0),
        vec3(-1.0, -1.0, -1.0),
        
        vec3(1.0, -1.0, -1.0),
        vec3(1.0, -1.0, 1.0),
        vec3(1.0, -1.0, 1.0),
        vec3(1.0, 1.0, 1.0),
        vec3(1.0, 1.0, 1.0),
        vec3(1.0, 1.0, -1.0),
        vec3(1.0, 1.0, -1.0),
        vec3(1.0, -1.0, -1.0),
        
        vec3(-1.0, -1.0, -1.0),
        vec3(1.0, -1.0, -1.0),
        vec3(1.0, -1.0, 1.0),
        vec3(-1.0, -1.0, 1.0),
        vec3(-1.0, 1.0, 1.0),
        vec3(1.0, 1.0, 1.0),
        vec3(1.0, 1.0, -1.0),
        vec3(-1.0, 1.0, -1.0)
    );
    
    int i = (int(t) * 2) % (points.length());
    return mix(points[i], points[i + 1], fract(t));
}

vec3 tetrisT(float t) {
    const vec3[] points = vec3[](
        vec3(1.5, -1.0, -0.5),
        vec3(-1.5, -1.0, -0.5),
        vec3(-1.5, -1.0, -0.5),
        vec3(-1.5, 0.0, -0.5),
        vec3(-1.5, 0.0, -0.5),
        vec3(-0.5, 0.0, -0.5),
        vec3(-0.5, 0.0, -0.5),
        vec3(-0.5, 1.0, -0.5),
        vec3(-0.5, 1.0, -0.5),
        vec3(0.5, 1.0, -0.5),
        vec3(0.5, 1.0, -0.5),
        vec3(0.5, 0.0, -0.5),
        vec3(0.5, 0.0, -0.5),
        vec3(1.5, 0.0, -0.5),
        vec3(1.5, 0.0, -0.5),
        vec3(1.5, -1.0, -0.5),
        
        vec3(1.5, -1.0, 0.5),
        vec3(-1.5, -1.0, 0.5),
        vec3(-1.5, -1.0, 0.5),
        vec3(-1.5, 0.0, 0.5),
        vec3(-1.5, 0.0, 0.5),
        vec3(-0.5, 0.0, 0.5),
        vec3(-0.5, 0.0, 0.5),
        vec3(-0.5, 1.0, 0.5),
        vec3(-0.5, 1.0, 0.5),
        vec3(0.5, 1.0, 0.5),
        vec3(0.5, 1.0, 0.5),
        vec3(0.5, 0.0, 0.5),
        vec3(0.5, 0.0, 0.5),
        vec3(1.5, 0.0, 0.5),
        vec3(1.5, 0.0, 0.5),
        vec3(1.5, -1.0, 0.5),
        
        
        vec3(1.5, -1.0, -0.5),
        vec3(1.5, -1.0, 0.5),
        vec3(-1.5, -1.0, 0.5),
        vec3(-1.5, -1.0, -0.5),
        vec3(-1.5, 0.0, -0.5),
        vec3(-1.5, 0.0, 0.5),
        vec3(-0.5, 0.0, 0.5),
        vec3(-0.5, 0.0, -0.5),
        vec3(-0.5, 1.0, -0.5),
        vec3(-0.5, 1.0, 0.5),
        vec3(0.5, 1.0, 0.5),
        vec3(0.5, 1.0, -0.5),
        vec3(0.5, 0.0, -0.5),
        vec3(0.5, 0.0, 0.5),
        vec3(1.5, 0.0, 0.5),
        vec3(1.5, 0.0, -0.5)
    );
    
    int i = (int(t) * 2) % (points.length());
    return mix(points[i], points[i + 1], fract(t));
}

vec3 hihats(float t) {
    vec3 p = vec3(0.0);

    p.xy = ((randomnoise12(t) * 2.0) - 1.0) * 0.5 * fract(-t + 0.25);
    
    p.xy *= step(fract(t * 2.0), 0.125) * step(fract(t * 0.5), 0.85);
    
    return p;
}

vec3 kick(float t) {
    float tf = (t * 300.0) - (20.0 * (fract(-t * 0.5) * easeOut(0.0, 0.1, fract(t * 3.0))));
    vec3 p = vec3(sine(tf), cosine(tf), 0.0);
    p *= 3.0 * fract(-t * 3.0);
    p *= smoothstep(0.0625, 0.0, t);
    p *= step(0.0, t);
    return p;
}

vec3 kicks(float t) {
    vec3 p;
    
    float t2 = fract(t * 0.125) * 8.0;
    
    p += kick(t2);
    p += kick(t2 - 2.5);
    
    p += kick(t2 - 4.0);
    p += kick(t2 - 6.5);
    
    return p;
}

vec3 snare(float t) {
    float tf = (t * 400.0) - (20.0 * (fract(-t * 0.5) * easeOut(-0.015, 0.04, fract(t * 3.0))));
    vec3 p = vec3(sine(tf), cosine(tf), 0.0);
    p.xy += ((randomnoise12(t) * 2.0) - 1.0) * 2.0 * fract(t * 3.0);
    p *= 3.0 * fract(-t * 4.0);
    p *= smoothstep(0.0625 * 4.0, 0.0, t);
    p *= step(0.0, t);
    return p;
}

vec3 snare2(float t) {
    float tf = (t * 1200.0) - (20.0 * (fract(-t * 0.5) * easeOut(-0.015, 0.04, fract(t * 3.0))));
    vec3 p = vec3(sine(tf), cosine(tf), 0.0);
    p.xy += ((randomnoise12(t) * 2.0) - 1.0) * 2.0 * fract(t * 3.0);
    p *= 3.0 * fract(-t * 4.0);
    p *= smoothstep(0.0625 * 3.0 * 0.25, 0.0, t);
    p *= step(0.0, t);
    return p;
}

vec3 snares(float t) {
    vec3 p;
    
    float t2 = fract((t * 0.25) + 0.25) * 4.0;
    
    p += snare(t2);
    p += snare(t2 - 2.0);
    
    return p;
}

vec3 drums(float t) {
    vec3 p;
    
    p += kicks(t);
    p += snares(t);
    
    return p;
}

float freqArpeggiator1(float t) {
    t = fract(t * 0.25) * 4.0;
    float s = ((triangle(floor(fract(t * 1.5) * 4.0) * 0.25) * 0.5) + 0.5) * 4.0;
    s += ((triangle(floor(t) * 0.5) * 0.5) + 0.5) * 10.0;
    s += 1.0;
    
    return pitch(s);
}

// This is your sandbox:
vec2 point(float t) {
    vec3 p = vec3(0.0);
    
    t = fract(t / 24.0) * 24.0; // Loop
    
    float freq = freqArpeggiator1(t) * 3.0;
    float tf = t * freq;
    
    p += box(tf);
    // What the hell
    p = mix(p, vec3(
        (easeInOut(easeInOut(easeInOut(easeInOut(easeInOut((p.x * 0.5) + 0.5))))) * 2.0) - 1.0,
        (easeInOut(easeInOut(easeInOut(easeInOut(easeInOut((p.y * 0.5) + 0.5))))) * 2.0) - 1.0,
        (easeInOut(easeInOut(easeInOut(easeInOut(easeInOut((p.z * 0.5) + 0.5))))) * 2.0) - 1.0
    ), step(8.0, t) * ((sine(t * 6.0) * 0.5) + 0.5));
    
    float k = floor(fract(t * 6.0) * 2.0) + 1.0;
    p = mix(p, round(p * k) / k, step(4.0, fract(t * 0.125) * 8.0));
    
    p *= 0.6;
    p *= easeOut(-0.25, 0.5, fract(t));
    
    p = inverse(rotX(t) * rotY(t)) * p;
    
    p -= vec3(0.0, 0.0, -1.5);
    
    p.xy /= p.z;
    
    {
        vec3 k1 = round(p * 16.0) / 16.0;
        vec3 k2 = ceil(p * 16.0) / 16.0;
        vec3 kf = p - k1;
        p = mix(p, mix(k1, k2, kf), step(2.0, fract(t * 0.25) * 4.0));
    }
    
    p += step(8.0, t) * hihats(t);
    p += step(16.0, t) * drums(t);
    
    p *= 1.0 - (easeOut(7.75, 8.0, fract(t * 0.125) * 8.0) * fract(floor(t * 16.0) * 0.5));
    
    p *= easeOut(0.0, 0.5, t);
    p *= easeOut(24.0, 23.5, t);
    
    return clamp(p.xy, vec2(-1.0), vec2(1.0));
}

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Samples coordinates every frame instead of sampling every iteration for every pixel for every frame
void mainImage(out vec4 o, in vec2 u) {
    // Estimate time based on pixel and calculate point
    u.y -= 0.5; // For some reason this fixes the error where t > VISUAL_ITERATIONS is wrong
    float t = u.x + (u.y * iResolution.x);
    
    // Skip samples if outside used region
    if(t > float(VISUAL_ITERATIONS)) { // Not perfect but saves a bit of performance (not sure because of SIMD)
        o = vec4(0.0, 0.0, 0.5, 1.0);
        return;
    }
    
    t /= float(VISUAL_ITERATIONS);
    t *= VISUAL_DURATION;
    
    // Output point result
    o.xy = point((iTime - t) + iTimeDelta);
    o.zw = vec2(0.0, 1.0);
}

// >>> ___ GLSL:[Sound] ____________________________________________________________________ <<<
vec2 mainSound(int s, float t) {
    vec2 v = point(t);

    // Some background noise (optional)
    v += (((texture(iChannel0, vec2(t * 1.4236, t * 1.2267)).xy * 2.0) - 1.0) * 0.0625 * 0.25);

    return v * 0.25; // User convenience, we don't want to destroy any ears
}
