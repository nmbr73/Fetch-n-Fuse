

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
const int MAX_STEPS = 150;
const float MAX_DIST = 20.0;
const float MIN_DIST = 0.01;

float Random1(float x, float y, float z) {
    return (fract(sin((x * 12.9898) + (y * 78.233) + (z * 195.1533)) * 43758.5453123) * 2.0) - 1.0;
}

float SmoothNoise1(vec3 p) {
    float ix0 = floor(p.x),
          iy0 = floor(p.y),
          iz0 = floor(p.z),
          ix1 = ix0 + 1.0,
          iy1 = iy0 + 1.0,
          iz1 = iz0 + 1.0,
          fx = p.x - ix0,
          fy = p.y - iy0,
          fz = p.z - iz0;

    fx *= fx * (3.0 - (fx * 2.0));
    fy *= fy * (3.0 - (fy * 2.0));
    fz *= fz * (3.0 - (fz * 2.0));

    return mix(mix(mix(Random1(ix0, iy0, iz0), Random1(ix1, iy0, iz0), fx),
                   mix(Random1(ix0, iy1, iz0), Random1(ix1, iy1, iz0), fx), fy),
               mix(mix(Random1(ix0, iy0, iz1), Random1(ix1, iy0, iz1), fx),
                   mix(Random1(ix0, iy1, iz1), Random1(ix1, iy1, iz1), fx), fy), fz);
}

float FractalSmoothNoise1(vec3 p) {
    float y = 0.0;
    
    float amplitude = 0.5;
    float frequency = 1.0;
    
    float gain = 0.5;
    float lacunarity = 2.0;
    for(int i = 0; i < 8; i++) {
        y += SmoothNoise1(p * frequency) * amplitude;
        frequency *= lacunarity;
        amplitude *= gain;
    }
    
    return y;
}

float Map(vec3 p) {
    return ((FractalSmoothNoise1(p + vec3(0.5)) * 0.5) + 0.5) - 0.3;
}

float March(vec3 ro, vec3 rd) {
    float d = 0.0;
    for(int i = 0; i < MAX_STEPS; i++) {
        float sd = Map(ro + (rd * d));
        d += sd;
        if(d > MAX_DIST) return MAX_DIST;
        if(abs(sd) < MIN_DIST) break;
    }
    return d;
}

void mainImage(out vec4 o, vec2 i) {
    vec2 uv = (i - (0.5 *  iResolution.xy)) / min(iResolution.x, iResolution.y);

    vec3 col = vec3(0.0);

    vec3 ro = vec3(0.0, 0.0, iTime * 3.0);
    vec3 rd = normalize(vec3(uv, 1.0));

    float d = March(ro, rd);
    vec3 p = ro + (rd * d);

    col += abs(d) / MAX_DIST;

    o = vec4(col,1.0);
}
