

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<

// Inter Membrane Space

// Buffer A : raymarching and lighting
// Buffer B : temporal anti aliasing

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;
    fragColor = texture(iChannel0, uv);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<

float details;

float map(vec3 p)
{
    // spicy fbm cyclic gyroid noise
    details = sin(iTime*.2-fbm(p)+length(p));
    return max(abs(details*.05), p.z+2.);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // salt
    vec3 rng = hash33(vec3(fragCoord, iFrame));
    
    // coordinates
    vec2 uv = (fragCoord-iResolution.xy/2.)/iResolution.y;
    vec3 color = vec3(0);
    vec3 ray = normalize(vec3(uv, -1.));
    ray.xy *= rot(-.7);
    vec3 pos = ray*(.5+.5*rng.z);
    
    // raymarch
    float maxDist = 5.;
    const float count = 100.;
    float steps = 0.;
    float total = 0.;
    for (steps = count; steps > 0.; --steps) {
        float dist = map(pos);
        if (dist < total/iResolution.y || total > maxDist) break;
        dist *= 0.9+0.1*rng.x;
        pos += ray * dist;
        total += dist;
    }
    
    // lighting
    float shade = steps/count;
    if (shade > .001 && total < maxDist) {
        vec2 noff = vec2(.001,0); // NuSan https://www.shadertoy.com/view/3sBGzV
        vec3 normal = normalize(map(pos)-vec3(map(pos-noff.xyy), map(pos-noff.yxy), map(pos-noff.yyx)));
        float top = dot(reflect(ray, normal), vec3(0,1,0))*.5+.5;
        vec3 tint = .5+.5*cos(vec3(1,2,3)+pos.y+details*6.);
        color = vec3(0.2) + vec3(.8)*top;
        color += tint * .5;
        color *= shade*shade;
    }
    
    fragColor = vec4(color, 1);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// Temporal Anti Aliasing from:
// https://www.elopezr.com/temporal-aa-and-the-quest-for-the-holy-trail/

// but only the color clamping...

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord.xy / iResolution.xy;
    vec3 color = texture(iChannel0, uv).rgb;
    vec3 temporal = texture(iChannel1, uv).rgb;
    vec3 minColor = vec3(9999.), maxColor = vec3(-9999.);
    for(int x = -1; x <= 1; ++x){
        for(int y = -1; y <= 1; ++y){
            vec3 c = texture(iChannel0, uv + vec2(x, y) / iResolution.xy).rgb;
            minColor = min(minColor, c);
            maxColor = max(maxColor, c);
        }
    }
    temporal = clamp(temporal, minColor, maxColor);
    fragColor.rgb = mix(color, temporal, 0.9);
    fragColor.a = 1.0;
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<

// fbm gyroid noise
float gyroid (vec3 seed) { return dot(sin(seed),cos(seed.yzx)); }
float fbm (vec3 seed) {
    float result = 0.;
    float a = .5;
    for (int i = 0; i < 5; ++i) {
        result += gyroid(seed/a+result/a)*a;
        a /= 2.;
    }
    return result;
}

mat2 rot(float a) {
    float c = cos(a), s = sin(a);
    return mat2(c,-s,s,c);
}

// Dave Hoskins
// https://www.shadertoy.com/view/4djSRW
vec3 hash33(vec3 p3) {
	p3 = fract(p3 * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yxz+33.33);
    return fract((p3.xxy + p3.yxx)*p3.zyx);
}
