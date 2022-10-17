

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<

// Brush toy by Leon Denise 2022-05-17

// I wanted to play further with shading and lighting from 2D heightmap.
// I started by generating a heightmap from noise, then shape and curves.
// Once the curve was drawing nice brush strokes, I wanted to add motion.
// Also wanted to add droplets of paints falling, but that will be
// for another sketch.

// This is the color pass
// Click on left edge to see layers

// The painting pass (Buffer A) is using FBM noise to simulate brush strokes
// The curve was generated with a discrete Fourier Transform,
// from https://www.shadertoy.com/view/3ljXWK

// Frame buffer sampling get offset from brush motion,
// and the mouse also interact with the buffer.


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec3 color = vec3(.0);
    
    // coordinates
    vec2 uv = fragCoord.xy / iResolution.xy;
    vec3 dither = texture(iChannel1, fragCoord.xy / 1024.).rgb;
    
    // value from noise buffer A
    vec3 noise = texture(iChannel0, uv).rgb;
    float gray = noise.x;
    
    // gradient normal from gray value
    vec3 unit = vec3(3./iResolution.xy,0);
    vec3 normal = normalize(vec3(
        TEX(uv + unit.xz)-TEX(uv - unit.xz),
        TEX(uv - unit.zy)-TEX(uv + unit.zy),
        gray*gray));
    
    
    // specular light
    vec3 dir = normalize(vec3(0,1,2.));
    float specular = pow(dot(normal, dir)*.5+.5,20.);
    color += vec3(.5)*specular;
    
    // rainbow palette
    vec3 tint = .5+.5*cos(vec3(1,2,3)*1.5+gray*5.+uv.x*5.);
    dir = normalize(vec3(uv-.5, 0.));
    color += tint*pow(dot(normal, -dir)*.5+.5, 0.5);
    
    // background blend
    vec3 background = vec3(.8)*smoothstep(1.5,0.,length(uv-.5));
    color = mix(background, clamp(color, 0., 1.), smoothstep(.2,.5,noise.x));
    
    // display layers when clic
    if (iMouse.z > 0.5 && iMouse.x/iResolution.x < .1)
    {
        if (uv.x < .33) color = vec3(gray);
        else if (uv.x < .66) color = normal*.5+.5;
        else color = vec3(.2+specular)*gray;
    }

    fragColor = vec4(color, 1);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<

// Brush toy by Leon Denise 2022-05-17

// The painting pass is using FBM noise to simulate brush strokes
// The curve was generated with a discrete Fourier Transform,
// from https://www.shadertoy.com/view/3ljXWK

// Frame buffer sampling get offset from brush motion,
// and the mouse also interact with the buffer.

const float speed = .01;
const float scale = 0.8;
const float falloff = 2.;

vec2 mouse;

// fractal brownian motion (layers of multi scale noise)
vec3 fbm(vec3 p)
{
    vec3 result = vec3(0);
    float amplitude = 0.5;
    for (float index = 0.; index < 3.; ++index)
    {
        result += (texture(iChannel0, p/amplitude).xyz) * amplitude;
        amplitude /= falloff;
    }
    return result;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{

    // coordinates
    vec2 uv = (fragCoord.xy - iResolution.xy / 2.)/iResolution.y;
    mouse = (iMouse.xy - iResolution.xy / 2.)/iResolution.y;
    
    // dithering
    vec3 dither = texture(iChannel2, fragCoord.xy / 1024.).rgb;
    
    // sample curve position
    float speed = 1.;
    float t = -iTime*speed+dither.x*.01;
    vec2 current = cookie(t);
    
    // velocity from current and next curve position
    vec2 next = cookie(t+.01);
    vec2 velocity = normalize(next-current);
    
    // move brush cursor along curve
    vec2 pos = uv-current*1.6;
    
    float paint = fbm(vec3(pos, 0.) * scale).x;
    
    // brush range
    float brush = smoothstep(.3,.0,length(pos));
    paint *= brush;
    
    // add circle shape to buffer
    paint += smoothstep(.05, .0, length(pos));
    
    // motion mask
    float push = smoothstep(.3, .5, paint);
    push *= smoothstep(.4, 1., brush);
    
    // direction and strength
    vec2 offset = 10.*push*velocity/iResolution.xy;
    
    // mouse interaction
    vec4 data = texture(iChannel1, vec2(0,0));
    bool wasNotPressing = data.w < 0.5;
    if (wasNotPressing && iMouse.z > .5) data.z = 0.;
    else data.z += iTimeDelta;
    data.z = clamp(data.z, 0., 1.);
    vec2 mousePrevious = data.xy;
    float erase = 0.;
    if (iMouse.z > 0.5)
    {
        uv = (fragCoord.xy - iResolution.xy / 2.)/iResolution.y;
        float mask = fbm(vec3(uv-mouse, 0.) * scale * .5).x;
        mask = smoothstep(.3,.6,mask);
        push = smoothstep(.2,.0,length(uv-mouse));
        push *= mask;
        vec2 dir = normalize(mousePrevious-mouse+.001);
        float fadeIn = smoothstep(.0, .5, data.z);
        float fadeInAndOut = sin(fadeIn*3.1415);
        offset += 10.*push*normalize(mouse-uv)/iResolution.xy*fadeInAndOut;
        erase = (.001 + .01*(1.-fadeIn)) * push;
        push *= 500.*length(mousePrevious-mouse)*fadeIn;
        offset += push*dir/iResolution.xy;
    }
    
    // sample frame buffer with motion
    uv = fragCoord.xy / iResolution.xy;
    vec4 frame = texture(iChannel1, uv + offset);
    
    // temporal fading buffer
    paint = max(paint, frame.x - .0005 - erase);
    
    // print result
    fragColor = vec4(clamp(paint, 0., 1.));
    
    // save mouse position for next frame
    if (fragCoord.x < 1. && fragCoord.y < 1.) fragColor = vec4(mouse, data.z, iMouse.z);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<

// shortcut to sample texture
#define TEX(uv) texture(iChannel0, uv).r

// rotation matrix
mat2 rot (float a) { return mat2(cos(a),-sin(a),sin(a),cos(a)); }

// generated with discrete Fourier transform
vec2 cookie(float t) {
	return vec2(0.08+cos(t-1.58)*0.23+cos(t*2.-1.24)*0.14+cos(t*3.-1.12)*0.09+cos(t*4.-0.76)*0.06+cos(t*5.-0.59)*0.05+cos(t*6.+0.56)*0.03+cos(t*7.-2.73)*0.03+cos(t*8.-1.26)*0.02+cos(t*9.-1.44)*0.02+cos(t*10.-2.09)*0.03+cos(t*11.-2.18)*0.01+cos(t*12.-1.91)*0.02,cos(3.14)*0.05+cos(t+0.35)*0.06+cos(t*2.+0.54)*0.09+cos(t*3.+0.44)*0.03+cos(t*4.+1.02)*0.07+cos(t*6.+0.39)*0.03+cos(t*7.-1.48)*0.02+cos(t*8.-3.06)*0.02+cos(t*9.-0.39)*0.07+cos(t*10.-0.39)*0.03+cos(t*11.-0.03)*0.04+cos(t*12.-2.08)*0.02);
}
