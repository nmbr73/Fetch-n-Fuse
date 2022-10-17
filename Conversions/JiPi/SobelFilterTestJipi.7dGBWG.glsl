

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define STRENGTH 100.

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    fragColor = pF(iChannel1);
}


// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    //fragColor = texture(iChannel0, UV);
    fragColor = texture(iChannel0, UV-0.05*cos(iTime)*sin(iTime));
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
// Utilities
    // macros
#define RES iResolution.xy
#define P fragCoord.xy
#define UV (fragCoord.xy/iResolution.xy)
#define MUV (iMouse.xy/iResolution.xy)

#define pF(c) texelFetch(c, ivec2(P), 0)
#define sF(c) texture(c, UV)

    // screen
vec2 wrap(in vec2 p, in vec2 res) {
    if (p.x > res.x) p.x = mod(p.x, res.x);
    else if (p.x < 0.) p.x = res.x + p.x;
    
    if (p.y > res.y) p.y = mod(p.y, res.y);
    else if (p.y < 0.) p.y = res.y + p.y;
    
    return p;
}

#define SCALE(v, mx, a, b) (a + (v * (b - a) / mx))
vec2 scale(vec2 mn, vec2 mx, mat2 bounds) {
    return vec2(SCALE(mn.x, mx.x, bounds[0][0], bounds[0][1]),
                SCALE(mn.y, mx.y, bounds[1][0], bounds[1][1]));
}

// math
    //Generic 3x3 filter - vec3(center, edges, diagonals)
#define GAUSSIAN vec3(.204, .124, .075)
#define LAPLACIAN vec3(-1., .2, .05)
vec4 filter3x3(in vec2 pos, in vec3 kernel, in sampler2D channel, in vec2 reso) {
    vec4 sum = vec4(0.);
    
    for(int i=-1; i<=1; i++) {
        for(int j=-1; j<=1; j++) {
            float weight = (i==0 && j==0) ? kernel[0] : (abs(i-j) == 1 ? kernel[1] : kernel[2]);
            
            sum += weight * texelFetch(channel, ivec2(wrap(pos + vec2(i, j), reso)), 0);
        }
    }
    
    return sum;
}


    // Sobel
#define SOBEL_EDGE_COLOR vec4(0.753,0.380,0.796,1.)
vec4 sobel(in vec2 pos, in sampler2D channel, in vec2 reso) {
    // 
    mat3 SX = mat3( 1.0,  2.0,  1.0, 
                    0.0,  0.0,  0.0, 
                   -1.0, -2.0, -1.0);
    mat3 SY = mat3(1.0, 0.0, -1.0, 
                   2.0, 0.0, -2.0, 
                   1.0, 0.0, -1.0);

    vec4 T = texelFetch(channel, ivec2(pos), 0);

    mat3 M = mat3(0.);
    for(int i=0; i<3; i++) {
        for(int j=0; j<3; j++) {
            vec4 A = texelFetch(channel, ivec2(pos + vec2(i-1, j-1)), 0);
            M[i][j] = length(A);
        }
    }
    
    float gx = dot(SX[0], M[0]) + dot(SX[1], M[1]) + dot(SX[2], M[2]);
    float gy = dot(SY[0], M[0]) + dot(SY[1], M[1]) + dot(SY[2], M[2]);
    
    
    // TODO factor into float sobel() and move this to a buffer pass.
    float g = sqrt(gx*gx + gy*gy);
    g = smoothstep(0.15, 0.98, g);

    return mix(T, SOBEL_EDGE_COLOR, g);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec4 blurred = filter3x3(P, GAUSSIAN, iChannel0, RES); 
    fragColor = vec4((blurred.x + blurred.y + blurred.z) * 0.3333);
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Only want the sobel from this source
    vec4 T = texture(iChannel1, UV);//* filter3x3(P, GAUSSIAN, iChannel0, RES)) * 0.25;
    vec4 sob = sobel(P, iChannel0, RES);
    
    fragColor = sob*sob;
}