

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define NUM_STARS (1 << 11)

float rand(float x) {
    return fract(sin(x) * 123.456);
}

float cross2(vec2 a, vec2 b) {
    return a.x * b.y - a.y * b.x;
}

float getDistanceLP(vec2 s, vec2 t, vec2 p) {
    return abs(cross2(t - s, p - s) / distance(t, s));
}

float getDistanceSP(vec2 s, vec2 t, vec2 p) {
    if (dot(t - s, p - s) < 0.) {
        return distance(p, s);
    }
    if (dot(s - t, p - t) < 0.) {
        return distance(p, t);
    }
    return getDistanceLP(s, t, p);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (fragCoord - .5 * iResolution.xy) / iResolution.y;
    
    vec3 col = vec3(0);
    
    // Switching forward/backward transition
    const float cycle = 7.0;
    float t = mod(iTime, cycle);
    if (t > cycle * 0.5) {
       t = cycle - t;
    }
    // 1.5 sec delay timer
    float td = max(t - 1.5, 0.);
    
    
    for (int i = 0; i < NUM_STARS; i++) {
        // Create random star
        float x = rand(float(i*3)*12.34) * 2. - 1.;
        float y = rand(float(i*3+1)*23.45) * 2. - 1.;
        vec2 c = vec2(x, y);
        float r = rand(float(i*3+2)) * 0.0018;
        
        // Create line segment of star and get distance from uv coordinates
        vec2 n = c * (exp(t*4.)-1.) * 0.002;
        float d = (getDistanceSP(c, c + n, uv.xy)-r);
        
        // Line segment color
        col += vec3(exp(-800.*d)) * vec3(0.7, 0.8, 1.0);
        
        // Environmental lighting
        col += vec3(0.7, 0.8, 1.0) * ((exp(t*0.00008)-1.) + (exp(td*td*0.01)-1.) * (0.04/length(uv.xy)));
    }
    
    fragColor = vec4(col,1.0);
}