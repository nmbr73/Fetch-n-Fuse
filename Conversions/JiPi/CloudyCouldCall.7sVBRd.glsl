

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<

// Cloudy Could Call
//
// - inadequat volumetric rendering
// - incorrect ambient occlusion
// - blue noise cache-misère
// - chiaroscuro simulacrum
// - nimitz protean clouds wannabe
//
// "Atmosphère ! Atmosphère ! Est-ce que j'ai une gueule d'atmosphère ?"
// - Arletty (1938)

// Buffer A : cloud rendering
// Buffer B : temporal anti aliasing

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;
    fragColor = texture(iChannel0, uv);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<

// global
float noise;

// spicy fbm gyroid noise
float gyroid (vec3 seed) { return dot(sin(seed),cos(seed.yzx)); }
float fbm (vec3 seed) {
    float result = 0.;
    float a = .5;
    for (int i = 0; i < 6; ++i) {
        seed.z -= iTime*.1+result*.1;
        result += abs(gyroid(seed/a))*a;
        a /= 2.;
    }
    return result;
}

// signed distance function
float map(vec3 p)
{
    noise = fbm(p*.5);
    float dist = -length(p.xy)+2. - noise*noise;
    return dist*.5;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{    
    vec3 color = vec3(0);
    
    // coordinates
    vec2 uv = (fragCoord-iResolution.xy/2.)/iResolution.y;
    vec3 pos = vec3(0,0,5);
    vec3 ray = normalize(vec3(uv,-1));
    
    // animated blue noise by Alan Wolfe
    // https://www.shadertoy.com/view/XsVBDR
    const float c_goldenRatioConjugate = 0.61803398875;
    vec4 rng = texture(iChannel0, fragCoord / vec2(1024.0));
    rng = fract(rng + float(iFrame%256) * c_goldenRatioConjugate);
    
    // raymarch
    float maxDist = 10.;
    const float count = 20.;
    float total = 0.;
    float dense = 0.;
    for (float steps = count; steps > 0.; --steps) {
        float dist = map(pos);
        dist *= 0.7+0.3*rng.x;
        // sort of volumetric march
        if (dist < .1*rng.z) {
            dense += .2;
            dist = .02;
        }
        total += dist;
        if (dense >= 1. || total > maxDist) break;
        pos += ray * dist;
    }
    
    // cloud color
    float n = noise;
    #define getAO(dir,k) smoothstep(-k,k,map(pos+dir*k)-map(pos-dir*k))
    float ao = getAO(vec3(0,0,1),(.1*rng.x+.1));
    color = .5+.5*cos(vec3(1,2,3)*.5+ao+pos.z*.3-uv.y);
    color *= .2+.8*n;
    
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