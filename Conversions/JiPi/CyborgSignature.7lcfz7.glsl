

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<

// Cyborg Signature,
// when you have to sign that check for your ai bot therapist

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec4 color = vec4(0, 0, 0, 1);
    vec2 uv = fragCoord/iResolution.xy;
    float timeline = fract(iTime*speed);
    
    // data readability unpacking
    vec4 data = texture(iChannel0, uv);
    float mask = data.r;
    float timestamp = data.b;
    float dist = data.g;
    float material = data.a;
    float glow = texture(iChannel1, uv).a;
    vec3 normal = texture(iChannel1, uv).rgb;
    
    // background
    color.rgb = vec3(1) * smoothstep(2., -2., length(uv-.5));
    
    // ambient occlusion
    if (.01 < timeline) 
        color *= smoothstep(-.5,.2,dist);
    
    if (mask > .001)
    {
        // lighting
        vec3 light = normalize(vec3(0,1,1));
        float shade = dot(normal, light)*.5+.5;
        color *= material;
        color += glow;
        color += pow(shade, 10.);

    // debug g-buffer
    } else if (false) {
    
        uv *= 4.;
        if (inside(uv))
        {
            // data pack
            vec4 d = texture(iChannel0, uv);
            color = fract(d.grba*3.);
        }
        uv.x -= 1.;
        if (inside(uv))
        {
            // normal and glow
            vec4 d = texture(iChannel1, uv);
            if (d.r > .001)
                color += d;
            color += d.aaaa;
        }
    }
    
    // shine
    vec3 tint = .5+.5*cos(vec3(1,2,3)*5.+uv.x*6.);
    color.rgb += tint*glow;
    
    fragColor = color;
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<

// move horizontally, with circles and random offset
vec2 move(float t)
{
    vec2 pos = vec2(0);
    float angle = t*10.;
    float radius = .1;
    float jitter = .1;
    float time = t*5.;
    float index = floor(time);
    float anim = fract(time);
    float scroll = fract(t*speed);
    vec2 rng = mix(hash21(index), hash21(index+1.), anim);
    pos += (rng*2.-1.)*jitter;
    pos.x += scroll*2.-1.;
    pos.y += pow(abs(sin(time*.2)), 20.)*.5;
    pos.y -= pow(abs(sin(time*.1)), 50.)*.4;
    pos += vec2(cos(angle),sin(angle*1.5))*radius;
    return pos;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;
    vec2 pos = 1.5*(fragCoord-iResolution.xy/2.)/iResolution.y;
    
    // shape
    float thin = .02+.01*sin(iTime*10.);
    float time = iTime;
    float dist = sdSegment(pos, move(time-iTimeDelta), move(time));
    float mask = smoothstep(thin,.0,dist);
    
    // frame buffer
    vec4 frame = texture(iChannel0, uv);
    if (frame.g > .0) dist = min(dist, frame.g);
    float timestamp = mix(frame.b, iTime, step(.0001,mask));
    mask = max(mask*.1,frame.r);
    float material = step(threshold,fract(timestamp*cycle));
    
    // pack
    fragColor = vec4(mask, dist, timestamp, material);
    
    // wipe
    float timeline = fract(iTime*speed);
    fragColor *= step(.01, timeline);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<

#define speed 0.1
#define cycle 2.
#define threshold 0.95

#define T(uv) texture(iChannel0, uv).r
#define inside(uv) (abs(uv.x-.5) < 0.5 && abs(uv.y-.5) < 0.5)

// Inigo Quilez https://iquilezles.org/articles/distfunctions2d/
float sdSegment( in vec2 p, in vec2 a, in vec2 b )
{
    vec2 pa = p-a, ba = b-a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
    return length( pa - ba*h );
}

// Dave Hoskins https://www.shadertoy.com/view/4djSRW
float hash13(vec3 p3)
{
	p3  = fract(p3 * .1031);
    p3 += dot(p3, p3.zyx + 31.32);
    return fract((p3.x + p3.y) * p3.z);
}
vec2 hash21(float p)
{
	vec3 p3 = fract(vec3(p) * vec3(.1031, .1030, .0973));
	p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.xx+p3.yz)*p3.zy);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{

    vec2 uv = fragCoord/iResolution.xy;
    float timeline = fract(iTime*speed);
    
    // compute normal
    vec4 color = texture(iChannel0, uv);
    vec3 unit = vec3(1./iResolution.xy, 0);
    vec3 normal = normalize(vec3(
                            T(uv+unit.xz)-T(uv-unit.xz),
                            T(uv-unit.zy)-T(uv+unit.zy),
                            color.r));
    
    // glow diffusion
    float glow = 0.;
    vec4 blue = texture(iChannel2, fragCoord/1024.+iTime)*2.-1.;
    uv += 5.*blue.xy/iResolution.xy;
    float gold = texture(iChannel1, uv).a;
    glow = max(gold, color.a*.35);
    glow *= step(.01, timeline);
    
    fragColor = vec4(normal, glow);
}