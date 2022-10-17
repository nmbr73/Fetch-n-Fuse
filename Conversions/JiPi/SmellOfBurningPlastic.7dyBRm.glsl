

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<

// Smell of Burning Plastic
// revisiting Liquid Toy https://www.shadertoy.com/view/fljBWc
// with gyroid turbulences and a burning floor context

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;
    vec3 blue = texture(iChannel1, fragCoord/1024.).rgb;
    vec3 color = vec3(0);
    
    // masks
    float shade = texture(iChannel0, uv).r;
    float flame = pow(shade, 6.);
    float smoke = pow(shade, .5);
    float height = flame;
    
    // normal
    float range = 30.*blue.x;
    vec3 unit = vec3(range/iResolution.xy,0);
    vec3 normal = normalize(vec3(
        TEX(uv + unit.xz)-TEX(uv - unit.xz),
        TEX(uv - unit.zy)-TEX(uv + unit.zy),
        height));
        
    // lighting
    vec3 tint = .5+.5*cos(vec3(0,.3,.6)*6.28-flame*4.-4.);
    vec3 dir = normalize(vec3(0,-.5,0.5));
    float light = dot(normal, dir)*.5+.5;
    light = pow(light,.5);
    light *= (uv.y+.5); 
    color += tint * flame;
    color += vec3(.5) * light;
    color *= smoke;
    color -= .1*blue.x;
    color += smoothstep(.1,.0,1.-shade);
    
    // show layers
    if (iMouse.z > 0.5) {
        if (iMouse.x < 20.) {
            if (uv.x < .25) {
                color = vec3(shade);
            } else if (uv.x < .5) {
                color = normal;
            } else if (uv.x < .75) {
                color = tint;
            } else {
                color = vec3(.5)*light;
            }
        }
    }
    
    fragColor = vec4(color,1.0);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<

// fbm gyroid cyclic noise
float gyroid (vec3 seed) { return dot(sin(seed),cos(seed.yzx)); }
float fbm (vec3 seed) {
    float result = 0.;
    float a = .5;
    for (int i = 0; i < 4; ++i) {
        result += sin(gyroid(seed/a)*3.14+iTime/a)*a;
        a /= 2.;
    }
    return result;
}

// the fluidish simulacre
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;
    vec2 p = (fragCoord-iResolution.xy/2.)/iResolution.y;
    vec3 blue = texture(iChannel1, fragCoord/1024.).rgb;
    
    float dt = iTimeDelta;
    float current = texture(iChannel0, uv).r;
    
    // shape
    float shape = p.y+.5;
    
    // mouse interaction
    if (iMouse.z > 0.5) {
        vec2 mouse = (iMouse.xy-iResolution.xy/2.)/iResolution.y;
        shape = min(shape, length(p-mouse)-.01);
    }
    
    
    // masks
    float shade = smoothstep(.01,.0,shape);
    float smoke = pow(uv.y,.5);
    float flame = 1.-uv.y;
    float steam = pow(current, 0.2);
    float cycle = .5 + .5 * sin(iTime-uv.x*3.);
    
    vec2 offset = vec2(0);
    
    // gravity
    offset += vec2(0,-1) * flame * cycle * steam;
    
    // wind
    //offset.x += sin(iTime*.2);
    //offset += 3.*normalize(p*rot(.1)-p) * smoothstep(.1,.0,abs(length(p)-.5));
    
    // expansion
    vec4 data = texture(iChannel0, uv);
    vec3 unit = vec3(20.*blue.x/iResolution.xy,0);
    vec3 normal = normalize(vec3(
        TEX(uv - unit.xz)-TEX(uv + unit.xz),
        TEX(uv - unit.zy)-TEX(uv + unit.zy),
        data.x*data.x*data.x)+.001);
    offset -= normal.xy * (smoke + cycle) * steam;
    
    // turbulence
    vec3 seed = vec3(p*2.,p.y);
    float angle = fbm(seed)*6.28*2.;
    offset += vec2(cos(angle),sin(angle)) * flame;
    
    // energy loss
    vec4 frame = texture(iChannel0, uv+offset/iResolution.xy);
    shade = max(shade, frame.r-dt*.2);
    fragColor = vec4(shade);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<

#define TEX(uv) texture(iChannel0, uv).r
mat2 rot (float a) { return mat2(cos(a),-sin(a),sin(a),cos(a)); }

