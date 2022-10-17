

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<

// Silky Storm
// gyroidisticly tunneled

// main code is in Buffer A
// Buffer B is a minimal temporal anti aliasing
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;
    fragColor = texture(iChannel0, uv);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<

// fractal brownian motion
// https://thebookofshaders.com/13/
float fbm (vec3 p)
{
    float result = 0.;
    float a = .5;
    for (float i = 0.; i < 4.; ++i) {
        result += sin(gyroid(p/a)*3.14+.1*iTime/a)*a;
        a /= 2.;
    }
    return result;
}

// signed distance function
float map(vec3 p)
{
    // tunnel
    float dist = max(0., -length(p.xy)+.5);
    
    // displace with gyroid noise
    float t = iTime * .1;
    vec3 s = p * 1.;
    s.z -= t;
    float noise = fbm(s);
    dist -= .1*noise;
    
    // filaments
    dist = min(dist, abs(noise)+max(0.,-p.z)*.003);
    
    return dist;
}

void coloring (inout vec3 color, in vec3 pos, in vec3 normal, in vec3 ray, in vec2 uv, in float shade)
{
    // Inigo Quilez color palette
    // https://iquilezles.org/www/articles/palettes/palettes.htm
    vec3 tint = .5+.5*cos(vec3(0,.3,.6)*6.283+iTime*.2+uv.y*2.);

    // lighting
    vec3 rf = reflect(ray, normal);
    float top = dot(rf, vec3(0,1,0))*.5+.5;
    float glow = dot(normal, ray)*.5+.5;
    color = vec3(0.5) * pow(dot(normal, -normalize(pos))*.5+.5, 0.5);
    color += vec3(.2)*clamp(top,0.,1.);
    color += tint*glow;
    color *= pow(shade,.5);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec3 color = vec3(0);
    
    // coordinates
    vec2 uv = (fragCoord-iResolution.xy/2.)/iResolution.y;
    vec3 pos = vec3(0,0,1);
    vec3 at = vec3(0);
    pos.xz *= rot(cos(iTime*.1)*.2);
    pos.zy *= rot(sin(iTime*.2)*.1);
    vec3 ray = lookAt(pos, at, uv, 1.);
    
    // noise
    vec3 blue = texture(iChannel0, fragCoord/1024.).xyz;
    vec3 white = hash33(vec3(fragCoord, iFrame));
    
    // start ahead
    pos += ray * white.z * .2;
    
    // blur edges
    float dof = .2*smoothstep(.5, 2., length(uv));
    ray.xy += vec2(cos(blue.x*6.28),sin(blue.x*6.28))*blue.z*dof;
    
    // raymarch
    float maxDist = 8.;
    const float count = 50.;
    float steps = 0.;
    float total = 0.;
    for (steps = count; steps > 0.; --steps) {
        float dist = map(pos);
        if (dist < total/iResolution.y || total > maxDist) break;
        dist *= 0.9+0.1*blue.z;
        ray += white * total*.002;
        pos += ray * dist;
        total += dist;
    }
    
    // coloring
    float shade = steps/count;
    if (shade > .001 && total < maxDist) {
        // NuSan
        // https://www.shadertoy.com/view/3sBGzV
        vec2 noff = vec2(.01,0);
        vec3 normal = normalize(map(pos)-vec3(map(pos-noff.xyy), map(pos-noff.yxy), map(pos-noff.yyx)));
        coloring(color, pos, normal, ray, uv, shade);
    }
    
    fragColor = vec4(color, 1);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<

#define repeat(p,r) (mod(p,r)-r/2.)
mat2 rot(float a) { return mat2(cos(a),-sin(a),sin(a),cos(a)); }
vec3 lookAt (vec3 from, vec3 at, vec2 uv, float fov)
{
  vec3 z = normalize(at-from);
  vec3 x = normalize(cross(z, vec3(0,1,0)));
  vec3 y = normalize(cross(x, z));
  return normalize(z * fov + uv.x * x + uv.y * y);
}
float gyroid (vec3 s)
{
    return dot(sin(s),cos(s.yzx));
}

// Dave Hoskins
// https://www.shadertoy.com/view/4djSRW
vec3 hash33(vec3 p3) {
	p3 = fract(p3 * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yxz+33.33);
    return fract((p3.xxy + p3.yxx)*p3.zyx);
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