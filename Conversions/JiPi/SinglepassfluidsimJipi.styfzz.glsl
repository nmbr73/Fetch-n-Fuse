

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Based on "spilled" by florian berger (flockaroo) https://www.shadertoy.com/view/MsGSRd
// Click and drag to inject color
// Press I to pick a different start seed

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
	vec2 uv = fragCoord.xy / iResolution.xy;
	fragColor = texture(iChannel0,uv);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Based on "spilled" by florian berger (flockaroo) https://www.shadertoy.com/view/MsGSRd
// Click and drag to inject color
// Press I to pick a different start seed

// RotNum has to be an odd integer
#define RotNum 5

#define keyTex iChannel1
#define KEY_I texture(keyTex,vec2((105.5-32.0)/256.0,(0.5+0.0)/3.0)).x

const float third = 1.0 / 3.0;
const float sixth = 1.0 / 6.0;
const float rotFloat = float(RotNum);
const float iRotFloat = 1.0 / rotFloat;
const float imf = 1.0 / float(0xFFFFFFFFU);
const float ang = 2.0 * 3.1415926535 / float(RotNum);
mat2 m = mat2( cos(ang), sin(ang),
              -sin(ang), cos(ang));

vec4 permute(vec4 x) {
    vec4 xm = mod(x, 289.0);
    return mod(((xm * 34.0) + 10.0) * xm, 289.0);
}

// Stefan Gustavson's and Ian McEwan's implementation of simplex noise (patent is now expired)
// https://github.com/stegu/psrdnoise
vec3 psrdnoise(vec3 x) {
    vec3 uvw = x + dot(x, vec3(third));
    vec3 i0 = floor(uvw);
    vec3 f0 = fract(uvw);
    vec3 g_ = step(f0.xyx, f0.yzz);
    vec3 l_ = 1.0 - g_;
    vec3 g = vec3(l_.z, g_.xy);
    vec3 l = vec3(l_.xy, g_.z);
    vec3 o1 = min(g, l);
    vec3 o2 = max(g, l);
    vec3 i1 = i0 + o1;
    vec3 i2 = i0 + o2;
    vec3 i3 = i0 + 1.0;
    vec3 v0 = i0 - dot(i0, vec3(sixth));
    vec3 v1 = i1 - dot(i1, vec3(sixth));
    vec3 v2 = i2 - dot(i2, vec3(sixth));
    vec3 v3 = i3 - dot(i3, vec3(sixth));
    vec3 x0 = x - v0;
    vec3 x1 = x - v1;
    vec3 x2 = x - v2;
    vec3 x3 = x - v3;
    vec4 hash = permute(permute(permute(
                  vec4(i0.z, i1.z, i2.z, i3.z))
                + vec4(i0.y, i1.y, i2.y, i3.y))
                + vec4(i0.x, i1.x, i2.x, i3.x));
    vec4 theta = hash * 3.883222077;
    vec4 sz = hash * -0.006920415 + 0.996539792;
    vec4 psi = hash * 0.108705628;
    vec4 Ct = cos(theta);
    vec4 St = sin(theta);
    vec4 sz_prime = sqrt(1.0 - sz * sz);
    vec4 gx = Ct * sz_prime;
    vec4 gy = St * sz_prime;
    vec3 g0 = vec3(gx.x, gy.x, sz.x);
    vec3 g1 = vec3(gx.y, gy.y, sz.y);
    vec3 g2 = vec3(gx.z, gy.z, sz.z);
    vec3 g3 = vec3(gx.w, gy.w, sz.w);
    vec4 w = 0.5 - vec4(dot(x0, x0), dot(x1, x1), dot(x2, x2), dot(x3, x3));
    w = max(w, 0.0);
    vec4 w2 = w * w;
    vec4 w3 = w2 * w;
    vec4 gdotx = vec4(dot(g0, x0), dot(g1, x1), dot(g2, x2), dot(g3, x3));
    vec4 dw = -6.0 * w2 * gdotx;
    vec3 dn0 = w3.x * g0 + dw.x * x0;
    vec3 dn1 = w3.y * g1 + dw.y * x1;
    vec3 dn2 = w3.z * g2 + dw.z * x2;
    vec3 dn3 = w3.w * g3 + dw.w * x3;
    return 39.5 * (dn0 + dn1 + dn2 + dn3);
}

// Chris Wellons' and TheIronBorn's best 32-bit two-round integer hash
// https://github.com/skeeto/hash-prospector
// flockaroo used 2D noise, but it was only sampled along one dimension
// and only used one output value, so I replaced it with 1D noise
float hash32(int x) {
    // since the frame number is an int seed, convert to uint for bitwise ops
    uint p = uint(x);
    p ^= p >> 16;
    p *= 0x21F0AAADU;
    p ^= p >> 15;
    p *= 0xD35A2D97U;
    p ^= p >> 15;
    // normalize float and shift range to -0.5, 0.5 to cover whole period with ang
    return float(p) * imf - 0.5;
}

float circleSDF(vec2 p, vec2 c, float r) {
    return length(p + c) - r;
}

// basically tried to eliminate as many dot products and divisions as possible
float getRot(vec2 invRes, vec2 pos, vec2 b, float idb) {
    vec2 p = b;
    float rot = 0.0;
    for(int i = 0; i < RotNum; i++) {
        rot += dot(texture(iChannel0, fract((pos + p) * invRes)).xy - 0.5,
                   vec2(p.y, -p.x));
        p = m * p;
    }
    return rot * iRotFloat * idb;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
    // use screen resolution in all calculations
    // since buffer and image resolution are the same
    
    // eliminate division where possible
    vec2 invRes = 1.0 / iResolution.xy;
    
    // proportionally squared and zero-centered uvs
    // to keep simplex seed color grid coords from stretching
    vec2 uv = (fragCoord - 0.5 * iResolution.xy) * invRes.y;
    float rnd = hash32(iFrame);
    
    // do ang * rnd once instead of twice
    float angrnd = ang * rnd;
    vec2 b = vec2(cos(angrnd), sin(angrnd));
    
    // calculate dot product and its inverse only once
    float db = dot(b, b);
    float idb = 1.0 / db;
    vec2 v = vec2(0.0);
    
    // abort loop later for bigger vortices (for long-term stability)
    // this makes it less dependent on certain resolutions for stability
    float bbMax = iResolution.y;
    bbMax *= bbMax;
    
    // reduced the number of rounds the velocity is summed over
    for(int l = 0; l < 9; l++) {
        if(db > bbMax) break;
        vec2 p = b;
        for(int i = 0; i < RotNum; i++) {
            v += p.yx * getRot(invRes, fragCoord.xy + p, b, idb);
            p = m * p;
        }
        b *= 2.0;
        // do multiplications for dot product optimizations
        db *= 4.0;
        idb *= 0.25;
    }
    
    fragColor=texture(iChannel0,fract((fragCoord.xy + v * vec2(-1, 1) * 2.0) / iResolution.xy));

    vec2 scr = fragCoord.xy * invRes - 0.5;
    // added an extra blue color "fountain"
    // since velocity is driven by red and green channels, blue is initially stationary (like black)
    // but starts moving when red and green accumulate in the blue pixels
    // slowed down color accumulation a bit to accomodate for an extra channel being used
    fragColor.xyz += (0.009 * vec3(scr.xy, -0.5 * (scr.x + scr.y)) / (dot(scr, scr) * 10.0 + 0.3));
    
    if(iFrame < 1 || KEY_I > 0.5) {
        float srv = iDate.x + iDate.y + iDate.z + iDate.w; 
        vec3 n = psrdnoise(vec3(uv * 4.0, srv));
        n = normalize(n) * 0.5 + 0.5;
        fragColor = vec4(n, 1.0);
    }
    
    if(iMouse.z > 0.0) {
        float t = iTime;
        vec2 ppos = -(iMouse.xy - 0.5 * iResolution.xy) * invRes.y;
        vec3 pcol = 0.5 * (sin(vec3(t, t + 2.1, t + 4.2)) + 1.0);
        float dist = circleSDF(uv, ppos, 0.1);
        if(dist < 0.0) fragColor = vec4(pcol, 1.0);
    }
}