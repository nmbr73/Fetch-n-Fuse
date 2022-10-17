

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<

// Sinusoidal Painting
// when you let sine paint

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec4 color = vec4(0, 0, 0, 1);
    vec2 uv = fragCoord/iResolution.xy;
    vec4 data = texture(iChannel0, uv);
    float mask = data.r;
    
    if (mask > .001)
    {
        // lighting
        vec3 normal = texture(iChannel1, uv).rgb;
        vec3 light = normalize(vec3(0,1,1));
        float timestamp = data.b;
        float shade = dot(normal, light)*.5+.5;
        vec3 palette = .5+.5*cos(vec3(1,2,3)*5.+timestamp*3.);
        color.rgb = palette * shade;
        color += pow(shade,  50.);

    }
    else
    {
        // background
        color.rgb = vec3(1) * smoothstep(2., -2., length(uv-.5));

        // shadow
        float sdf = data.g;
        color *= smoothstep(-.3, .2,  sdf);
    }
    
    fragColor = color;
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<

// move brush
vec2 move(float t)
{
    vec2 pos = vec2(0);
    
    // random targets
    float jitter = .5;
    float time = t*3.;
    float index = floor(time);
    float anim = fract(time);
    vec2 rng = mix(hash21(index), hash21(index+1.), anim);
    pos += (rng*2.-1.)*jitter;
    
    // translate to right
    pos.x += .5;
    
    // twist it
    float angle = t;
    float radius = .1;
    pos += vec2(cos(angle),sin(angle))*radius;
    
    // fbm gyroid noise
    angle = fbm(vec3(pos,t))*6.28;
    radius = .2;
    pos += vec2(cos(angle),sin(angle))*radius;
    return pos;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // coordinates
    vec2 uv = fragCoord/R.xy;
    vec2 p = 1.5*(fragCoord-R.xy/2.)/R.y;
    
    // scroll
    uv.x += 1./R.x;
    
    // framebuffer
    vec4 frame = texture(iChannel0, uv);
    float mask = frame.r;
    float sdf = frame.g;
    
    // interaction
    if (iMouse.z > 0.)
    {
        vec2 mouse = iMouse.xy;
        vec4 prev = texture(iChannel0, vec2(0));
        vec3 dither = hash(uvec3(fragCoord, iFrame)); 
        mouse = prev.z > 0. ? mix(mouse, prev.xy, dither.x) : mouse;
        mouse = 1.5*(mouse-R.xy/2.)/R.y;
        float thin = .04+.03*sin(iTime*20.);
        float dist = length(p-mouse);
        float msk = smoothstep(thin,.0,dist);
        if (msk > .001) frame.b = iTime;
        sdf = sdf < .001 ? dist : min(sdf, dist);
        mask += msk;
    }
    else
    {
        // accumulate noisy results
        for (float frames = 20.; frames > 0.; --frames)
        {
            // cursor timeline with noise offset
            float f = float(iFrame) + frames * 200.;
            vec3 rng = hash(uvec3(fragCoord, f));
            float cursor = rng.x*.03+iTime;

            // brush
            float thin = .04+.03*sin(cursor*20.);
            float dist = length(p-move(cursor));
            float msk = smoothstep(thin,.0,dist);

            // timestamp
            if (msk > .001) frame.b = iTime;

            // distance
            sdf = sdf < .001 ? dist : min(sdf, dist);

            // accumulate
            mask += msk;
        }
    }

    // save data
    frame.r = mask;
    frame.g = sdf;
    fragColor = frame;
    
    // avoid glitch after disabling fullscreen
    if (fragCoord.x > R.x-1.) fragColor = vec4(0,0,0,1);
    
    if (fragCoord.x < 1. && fragCoord.y < 1.)
    {
        fragColor = iMouse;
    }
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<

#define R iResolution
#define T(uv) texture(iChannel0, uv).r
float gyroid (vec3 seed) { return dot(sin(seed),cos(seed.yzx)); }
float fbm (vec3 seed) {
    float result = 0.;
    float a = .5;
    for (int i = 0; i < 3; ++i) {
        result += gyroid(seed/a)*a;
        a /= 3.;
    }
    return result;
}

// Dave Hoskins https://www.shadertoy.com/view/4djSRW
vec2 hash21(float p)
{
	vec3 p3 = fract(vec3(p) * vec3(.1031, .1030, .0973));
	p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.xx+p3.yz)*p3.zy);
}

// Victor Shepardson + Inigo Quilez 
// https://www.shadertoy.com/view/XlXcW4
const uint k = 1103515245U;  // GLIB C
vec3 hash( uvec3 x )
{
    x = ((x>>8U)^x.yzx)*k;
    x = ((x>>8U)^x.yzx)*k;
    x = ((x>>8U)^x.yzx)*k;
    return vec3(x)*(1.0/float(0xffffffffU));
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{

    vec2 uv = fragCoord/iResolution.xy;
    
    // compute normal
    vec4 color = texture(iChannel0, uv);
    vec3 unit = vec3(1./iResolution.xy, 0);
    vec3 normal = normalize(vec3(
                            T(uv+unit.xz)-T(uv-unit.xz),
                            T(uv-unit.zy)-T(uv+unit.zy),
                            color.r));
                            
    fragColor = vec4(normal, 1.);
}