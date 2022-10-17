

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Taste of Noise 18 by Leon Denise 2022-05-17
// variation of https://www.shadertoy.com/view/fljBWK

// A very distorted volume
// Playing with a 3D FBM noise

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Render result of Buffer A
    vec2 uv = fragCoord.xy / iResolution.xy;
    fragColor = texture(iChannel0, uv);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Taste of Noise 18 by Leon Denise 2022-05-17
// variation of https://www.shadertoy.com/view/fljBWK

// A very distorted volume
// Playing with a 3D FBM noise

float details;

// rotation matrix
mat2 rot (float a) { return mat2(cos(a),-sin(a),sin(a),cos(a)); }

// shortcut for lighting
#define dt(rn,v,p) pow(dot(rn,normalize(v))*.5+.5,p)

#define ss(a,b,t) smoothstep(a,b,t)

// https://iquilezles.org/articles/distfunctions/
float smin(float d1, float d2, float k) { float h = clamp( 0.5 + 0.5*(d2-d1)/k, 0.0, 1.0 ); return mix( d2, d1, h ) - k*h*(1.0-h); }

// transform linear value into cyclic absolute value
vec3 bend(vec3 v)
{
    return abs(sin(v*6.28*4.-iTime * .2));
}

// fractal brownian motion (layers of multi scale noise)
vec3 fbm(vec3 p)
{
    vec3 result = vec3(0);
    float falloff = 0.5;
    for (float index = 0.; index < 3.; ++index)
    {
        result += bend(texture(iChannel0, p/falloff).xyz) * falloff;
        falloff /= 2.;
    }
    return result;
}

// signed distance function
float map(vec3 p)
{
    float d = 0.;
    d = length(p)-1.0;
    
    // FBM animated noise
    vec3 ps = p * .04;
    ps.z += iTime*.0005+.2;
    vec3 spicy = fbm(ps);
    details = spicy.x;
    spicy = spicy * 2. - 1.;
    
    // displace volume
    d += spicy.x * .9;
    
    // volume to surface
    d = abs(d);
    
    return d * .025;
}

// Antoine Zanuttini
// https://www.shadertoy.com/view/3sBGzV
vec3 getNormal (vec3 pos)
{
    vec2 noff = vec2(0.01,0);
    return normalize(map(pos)-vec3(map(pos-noff.xyy), map(pos-noff.yxy), map(pos-noff.yyx)));
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // coordinates
    vec2 uv = (fragCoord.xy - iResolution.xy / 2.)/iResolution.y;
    vec3 noise = texture(iChannel1, fragCoord.xy/1024.+iTime).rgb;
    vec3 ray = normalize(vec3(uv, .5));
    vec3 pos = vec3(0,0,0);
    
    // init variables
    vec3 color, normal, tint, dir, refl;
    float index, shade, light;
    const float count = 70.;

    // ray marching
    for (index = count; index > 0.; --index)
    {
        float dist = map(pos);
        if (dist < .001) break;
        dist *= .9+.1*noise.z;
        pos += ray*dist;
    }
    
    // lighting
    shade = index/count;
    normal = getNormal(pos);
    refl = reflect(ray, normal);
    tint = .5+.5*cos(vec3(1,2,3)+details*5. + .5);
    color += tint * dt(refl, vec3(0,1,-1), .3);
    color += vec3(0.459,0.102,0.173)*dt(refl, vec3(0,-1,.5), .5);
    color = mix(color, vec3(1), ss(.4,1.,dot(normal, -ray)));
    color = clamp(color * shade * 1.5, 0., 1.);
    
    // temporal buffer
    uv = fragCoord.xy / iResolution.xy;
    vec3 frame = texture(iChannel2, uv).rgb;
    color = mix(color, frame, .9);
    
    fragColor = vec4(color, 1.);
}