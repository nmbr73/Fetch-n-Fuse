

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
vec4 cell(in ivec2 p) {
    ivec2 r = ivec2(textureSize(iChannel0, 0));
    p = (p+r) % r;
    return texelFetch(iChannel0, p, 0 );
} 

vec3 pal(float t) {
    vec3 d = 1. * vec3(0,1,2)/3.;
    return 1. + 2. * cos(6.28319 * (0.5 * t + d));
}

#define thc(a,b) tanh(a*cos(b))/tanh(a)

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;
    
    // Zoom + distort
    float mx = 0.5 + 0.5 * thc(2.5, length(uv) * 0.5 - 0.4 * iTime);
    float zm = mix(0.44, 0.6, mx);
    zm += 1.5 - 1.5 * tanh(0.04 * iTime * iTime);
    
    //zm -= 0.3 * tanh(24. - length(uv) * 50.);
    
    // Pixel + cells etc.
    vec2 res = 0.5 * floor(iResolution.xy);    
    ivec2 px = ivec2(zm * fragCoord + (1.-zm) * res);
    
    vec4 c = cell(px);
    vec4 b = cell(px - ivec2(0,1));
    vec4 t = cell(px + ivec2(0,1));  
    vec4 l = cell(px - ivec2(1,0));
    vec4 r = cell(px + ivec2(1,0));    
    vec4 sum = b + t + l + r;
      
    // Lighten right side
    float cn = 0.06 * smoothstep(-0.2, 0.2, uv.x); 
    
    // Cut colors into lines
    float fl = clamp(uv.y, -0.125, 0.125);
    
    // Background
    vec3 col = pal(0.05 * (uv.x + 5. * uv.y) + 0.2 * cn - 0.7);
    
    // Exterior outline
   // if (sum.r > 1.)
        col = c.g * pal(0.2 + uv.x * 0.2 + 0.3 * floor(c.g * 40.) /40.);  
    
    //col -= c.rgb;
    fragColor.rgb = col;
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
#define pi 3.14159

vec4 cell(in ivec2 p) {
    ivec2 r = ivec2(textureSize(iChannel0, 0));
    p = (p+r) % r;
    return texelFetch(iChannel0, p, 0 );
} 

// From iq
float smin( float d1, float d2, float k ) {
    float h = clamp( 0.5 + 0.5*(d2-d1)/k, 0.0, 1.0 );
    return mix( d2, d1, h ) - k*h*(1.0-h); 
}

void mainImage( out vec4 col, in vec2 f )
{
    ivec2 px = ivec2(f);
    vec4 test = cell(px);
    //px = ivec2(1.2 * (f-res));
    
    // Centre coords
    vec2 res = floor(0.5 * iResolution.xy);
    f -= res;
    
    px = ivec2(1.5 * f + 1. * iTime * vec2(1,0));
    
    // Speed of time
    float spd = 0.125;

    // Number of blobs
    float n = 40.;
    
    // Distance from blobs
    float d = 1e5;
    
    for (float i = 0.; i < n; i++) {
        // Offset each blob
        float io = 2. * pi * i / n;
        
        // Time
        float t = spd * iTime + 2. * pi * cos(0.5 * spd * iTime + io);
        
        // Motion of blobs (idk how this works)
        float c = 1. + 0.5 * cos(4. * t + io);
        d = smin(d, c * length(f - 120. * (c-0.5) * vec2(cos(t+io), sin(t+io))), 2.);  
     }
     
     // Harsh shape
     float r = step(d, 5.);
     
     // Soft shape (going inwards)
     float s = smoothstep(0., 5., -d + 5.);
     
     vec4 cl = cell(px);
     cl = mix(cl, cell(ivec2(f)), 0.95);
     col = vec4(s);
     
     col = mix(col + 0.99 * cl, 0.999 * abs(col-cl), 1.);//cl * 0.999;

     col = clamp(col, 0., 1.);
     //col *= 0.5;
}