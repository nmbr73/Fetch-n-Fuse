

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<

// Liquid toy by Leon Denise 2022-05-18
// Playing with shading with a fake fluid heightmap

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    
    // coordinates
    vec2 uv = fragCoord.xy / iResolution.xy;
    vec3 dither = texture(iChannel1, fragCoord.xy / 1024.).rgb;
    
    // value from buffer A
    vec4 data =  texture(iChannel0, uv);
    float gray = data.x;
    
    // gradient normal from gray value
    float range = 3.;
    vec3 unit = vec3(range/iResolution.xy,0);
    vec3 normal = normalize(vec3(
        TEX(uv + unit.xz)-TEX(uv - unit.xz),
        TEX(uv - unit.zy)-TEX(uv + unit.zy),
        gray*gray*gray));
        
    // backlight
    vec3 color = vec3(.3)*(1.-abs(dot(normal, vec3(0,0,1))));
    
    // specular light
    vec3 dir = normalize(vec3(0,1,2));
    float specular = pow(dot(normal, dir)*.5+.5,20.);
    color += vec3(.5)*ss(.2,1.,specular);
    
    // rainbow
    vec3 tint = .5+.5*cos(vec3(1,2,3)*1.+dot(normal, dir)*4.-uv.y*3.-3.);
    color += tint * smoothstep(.15,.0,gray);

    // dither
    color -= dither.x*.1;
    
    // background blend
    vec3 background = vec3(1);
    background *= smoothstep(1.5,-.5,length(uv-.5));
    color = mix(background, clamp(color, 0., 1.), ss(.01,.1,gray));
    
    // display layers when clic
    if (iMouse.z > 0.5 && iMouse.x/iResolution.x < .1)
    {
        if (uv.x < .33) color = vec3(gray);
        else if (uv.x < .66) color = normal*.5+.5;
        else color = vec3(tint);
    }

    fragColor = vec4(color, 1);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<

// Liquid toy by Leon Denise 2022-05-18
// Playing with shading with a fake fluid heightmap

const float speed = .01;
const float scale = .1;
const float falloff = 3.;
const float fade = .4;
const float strength = 1.;
const float range = 5.;

// fractal brownian motion (layers of multi scale noise)
vec3 fbm(vec3 p)
{
    vec3 result = vec3(0);
    float amplitude = 0.5;
    for (float index = 0.; index < 3.; ++index)
    {
        result += texture(iChannel0, p/amplitude).xyz * amplitude;
        amplitude /= falloff;
    }
    return result;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{

    // coordinates
    vec2 uv = (fragCoord.xy - iResolution.xy / 2.)/iResolution.y;
    
    // noise
    vec3 spice = fbm(vec3(uv*scale,iTime*speed));
    
    // draw circle at mouse or in motion
    float t = iTime*2.;
    vec2 mouse = (iMouse.xy - iResolution.xy / 2.)/iResolution.y;
    if (iMouse.z > .5) uv -= mouse;
    else uv -= vec2(cos(t),sin(t))*.3;
    float paint = trace(length(uv),.1);
    
    // expansion
    vec2 offset = vec2(0);
    uv = fragCoord.xy / iResolution.xy;
    vec4 data = texture(iChannel1, uv);
    vec3 unit = vec3(range/iResolution.xy,0);
    vec3 normal = normalize(vec3(
        TEX1(uv - unit.xz)-TEX1(uv + unit.xz),
        TEX1(uv - unit.zy)-TEX1(uv + unit.zy),
        data.x*data.x)+.001);
    offset -= normal.xy;
    
    // turbulence
    spice.x *= 6.28*2.;
    spice.x += iTime;
    offset += vec2(cos(spice.x),sin(spice.x));
    
    // sample buffer
    vec4 frame = texture(iChannel1, uv + strength * offset / iResolution.xy);
    
    // temporal fading buffer
    paint = max(paint, frame.x - iTimeDelta * fade);
    
    // print result
    fragColor = vec4(clamp(paint, 0., 1.));
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<

// shortcut to sample texture
#define TEX(uv) texture(iChannel0, uv).r
#define TEX1(uv) texture(iChannel1, uv).r
#define TEX2(uv) texture(iChannel2, uv).r
#define TEX3(uv) texture(iChannel3, uv).r

// shorcut for smoothstep uses
#define trace(edge, thin) smoothstep(thin,.0,edge)
#define ss(a,b,t) smoothstep(a,b,t)
