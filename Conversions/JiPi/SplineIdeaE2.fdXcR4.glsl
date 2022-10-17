

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;

    float x = texelFetch( iChannel0, ivec2(fragCoord), 0 ).x;
    x = 4. * x * (1.-x);
    x = x * x;
    vec3 e = vec3(1.);
    vec3 col2 = 2. * x * pal(mix(0.3, 0.35, 0.5 + 0.5 * thc(2., iTime)), e, e, e, vec3(0., 0.33, 0.66));
    fragColor = vec4(0.06 + col2, 1.);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
vec2 P(float time) {
    
    // Offset y values from x values (so it doesnt just move diagonally)
    float o = 0.01;
    // Scale point locations to fit to screen
    float sc = 0.8;
    
    // Next 4 points in sequence
    // (bad approach - has a bottom-left bias) (does it tho?)
    vec2 p0 = -sc * 0.5 + sc * vec2( h21(vec2(floor(time))),      h21(vec2(o + floor(time))) );
    vec2 p1 = -sc * 0.5 + sc * vec2( h21(vec2(floor(time + 1.))), h21(vec2(o + floor(time + 1.))) );
    vec2 p2 = -sc * 0.5 + sc * vec2( h21(vec2(floor(time + 2.))), h21(vec2(o + floor(time + 2.))) );
    vec2 p3 = -sc * 0.5 + sc * vec2( h21(vec2(floor(time + 3.))), h21(vec2(o + floor(time + 3.))) );

    float f = fract(time);

    float t = 0.8;
    mat4 M = mat4(   0,  1,     0,        0,
                    -t,  0,     t,        0,
                  2.*t,  t-3.,  3.-2.*t,  -t,
                    -t,  2.-t,  t-2.,     t);
    vec4 U = vec4(1., f, f*f, f*f*f);
    vec4 Px = vec4(p0.x, p1.x, p2.x, p3.x);
    vec4 Py = vec4(p0.y, p1.y, p2.y, p3.y);
    return vec2(dot(Px, M * U), dot(Py, M * U));
}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (fragCoord.xy - 0.5 * iResolution.xy) / iResolution.y;    
    
    vec2 uv2 = uv;
   float time = 0.005 * h21(uv) + 1.8 * cos(3.5 * length(uv) + 0.5 * iTime) + 0.25 * iTime;
    vec2 p = P(time);
   // vec2 p2 = P(time - 0.5 * 0.1);
    uv2 += p;
    

   // float th = pi/4. + atan(p.y - p2.y, p.x - p2.x);
    float th = 1. * iTime;
    mat2 R = mat2(cos(th), sin(th), -sin(th), cos(th));
    uv2 *= R;
    uv2 = abs(uv2);

    vec2 pt = 0.15 * (0.5 + 0.5 * ths(2., iTime)) / sqrt(2.) * vec2(1);
    //pt += 0.05 * vec2(cos(24. * uv.x + 4. * iTime), sin(24. * uv.y + 4. * iTime));
    float d = length(uv2 - pt);
    
    float k = 2. / iResolution.y;
    float s = smoothstep(-k, k, -d + 0.025);
    float x = texelFetch( iChannel0, ivec2(fragCoord), 0 ).x;
    x = 0.985 * clamp(x, 0., 1.);
    s = max(x, s);
    /*
    float d = length(uv - p);
    float k = 1. / iResolution.y;
    float s = smoothstep(-k, k, -d + 0.025);
    float x = texelFetch( iChannel0, ivec2(fragCoord), 0 ).x;
    x = 0.98 * clamp(x, 0., 1.);
    s = max(x, s);
    */
    
    //s = max(s, f * smoothstep(-k, k, -length(uv - p2) + 0.025));

    fragColor = vec4(s);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
// Collection of functions I use a lot:

#define pi 3.14159

float thc(float a, float b) {
    return tanh(a * cos(b)) / tanh(a);
}

float ths(float a, float b) {
    return tanh(a * sin(b)) / tanh(a);
}

vec2 thc(float a, vec2 b) {
    return tanh(a * cos(b)) / tanh(a);
}

vec2 ths(float a, vec2 b) {
    return tanh(a * sin(b)) / tanh(a);
}

vec3 pal( in float t, in vec3 a, in vec3 b, in vec3 c, in vec3 d )
{
    return a + b*cos( 6.28318*(c*t+d) );
}

float h21 (vec2 a) {
    return fract(sin(dot(a.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

float mlength(vec2 uv) {
    return max(abs(uv.x), abs(uv.y));
}


