

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<

// popping shapes in a turbo rainbow dissolver

// main code is in Buffer A
// Buffer B is a minimal temporal anti aliasing
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;
    fragColor = texture(iChannel0, uv);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<

// fractal brownian motion https://thebookofshaders.com/13/
// with a "abs(sin(value))" twist 
vec3 fbm (vec3 p)
{
    vec3 result = vec3(.0);
    float a = .5;
    for (float i = 0.; i < 3.; ++i) {
        result += abs(sin(texture(iChannel1, p/a).xyz*6.))*a;
        a /= 2.;
    }
    return result;
}

// signed distance function
float map(vec3 p)
{
    float dist = 100.;
    
    // timing
    float time = iTime;
    float anim = fract(time);
    float index = floor(time);
    
    // noise animation
    float scale = .1-anim*.05;
    vec3 seed = p * scale + index * .12344;
    vec3 noise = fbm(seed);
    
    // shapes and distortions
    float size = .5*pow(anim,.2);
    float type = mod(index, 3.);
    dist = type > 1.5 ? sdTorus(p, vec2(size, .1)) :
           type > .5 ? sdBox(p,vec3(size*.7)) :
           length(p)-size;
    dist -= anim * noise.x * .2;
    dist += pow(anim, 3.) * noise.y;
    
    // scale field when highly distorted to avoid artefacts
    return dist * (1.-anim*.7);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (fragCoord-iResolution.xy/2.)/iResolution.y;
    
    // background
    vec3 color = vec3(.5)*smoothstep(2.,.5,length(uv));
    
    // coordinates
    vec3 pos = vec3(0,0,-1.5);
    pos.xz *= rot(iTime*.1);
    pos.zy *= rot(sin(iTime*.2));
    vec3 ray = lookAt(pos, vec3(0), uv, 1.5);
    
    // noise
    vec3 blue = texture(iChannel0, fragCoord/1024.).xyz;
    
    // raymarch
    const float count = 30.;
    float steps = 0.;
    float total = 0.;
    for (steps = count; steps > 0.; --steps) {
        float dist = map(pos);
        if (dist < total/iResolution.y || total > 3.) break;
        dist *= 0.9+0.1*blue.z;
        pos += ray * dist;
        total += dist;
    }
    
    // coloring
    float shade = steps/count;
    if (shade > .001 && total < 3.) {
    
        // NuSan https://www.shadertoy.com/view/3sBGzV
        vec2 noff = vec2(.02,0);
        vec3 normal = normalize(map(pos)-vec3(map(pos-noff.xyy), map(pos-noff.yxy), map(pos-noff.yyx)));
        
        color = vec3(.1);
        float light = dot(reflect(ray, normal), vec3(0,1,0))*.5+.5;
        float rainbow = dot(normal, -normalize(pos))*.5+.5;
        color += vec3(0.5)*pow(light, 4.5);
        
        // Inigo Quilez color palette https://iquilezles.org/articles/palettes/
        color += (.5+.5*cos(vec3(0.,.3,.6)*6.+uv.y*3.+iTime))*pow(rainbow,4.);
        color *= pow(shade,.5);
    }
    
    fragColor = vec4(color, 1);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<

mat2 rot(float a) { return mat2(cos(a),-sin(a),sin(a),cos(a)); }
vec3 lookAt (vec3 from, vec3 at, vec2 uv, float fov)
{
  vec3 z = normalize(at-from);
  vec3 x = normalize(cross(z, vec3(0,1,0)));
  vec3 y = normalize(cross(x, z));
  return normalize(z * fov + uv.x * x + uv.y * y);
}

// Inigo Quilez
// https://iquilezles.org/articles/distfunctions/
float sdBox( vec3 p, vec3 b )
{
  vec3 q = abs(p) - b;
  return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
}
float sdTorus( vec3 p, vec2 t )
{
  vec2 q = vec2(length(p.xz)-t.x,p.y);
  return length(q)-t.y;
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// Temporal Anti Aliasing from:
// https://www.elopezr.com/temporal-aa-and-the-quest-for-the-holy-trail/

// but only the color clamping...
// it's very subtle but I like it...

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