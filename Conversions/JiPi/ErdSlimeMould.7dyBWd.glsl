

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<

// shortcut to sample texture
#define TEX(uv) texture(iChannel0, uv).r


// Fork of " expansive reaction-diffusion" by Flexi. https://shadertoy.com/view/4dcGW2
// 2022-07-28 10:46:34

// and

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
    vec3 tint = .5+.5*cos(vec3(1,2,3)*(1.+(.5*sin(iTime/3.)))+gray*5.+uv.x*5.);
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
// main reaction-diffusion loop

// actually the diffusion is realized as a separated two-pass Gaussian blur kernel and is stored in buffer C

#define pi2_inv 0.159154943091895335768883763372

vec2 complex_mul(vec2 factorA, vec2 factorB){
    return vec2( factorA.x*factorB.x - factorA.y*factorB.y, factorA.x*factorB.y + factorA.y*factorB.x);
}

vec2 spiralzoom(vec2 domain, vec2 center, float n, float spiral_factor, float zoom_factor, vec2 pos){
    vec2 uv = domain - center;
    float d = length(uv);
    return vec2( atan(uv.y, uv.x)*n*pi2_inv + d*spiral_factor, -log(d)*zoom_factor) + pos;
}

vec2 complex_div(vec2 numerator, vec2 denominator){
    return vec2( numerator.x*denominator.x + numerator.y*denominator.y,
                numerator.y*denominator.x - numerator.x*denominator.y)/
        vec2(denominator.x*denominator.x + denominator.y*denominator.y);
}

float circle(vec2 uv, vec2 aspect, float scale){
    return clamp( 1. - length((uv-0.5)*aspect*scale), 0., 1.);
}

float sigmoid(float x) {
    return 2./(1. + exp2(-x)) - 1.;
}

float smoothcircle(vec2 uv, vec2 aspect, float radius, float ramp){
    return 0.5 - sigmoid( ( length( (uv - 0.5) * aspect) - radius) * ramp) * 0.5;
}

float conetip(vec2 uv, vec2 pos, float size, float min)
{
    vec2 aspect = vec2(1.,iResolution.y/iResolution.x);
    return max( min, 1. - length((uv - pos) * aspect / size) );
}

float warpFilter(vec2 uv, vec2 pos, float size, float ramp)
{
    return 0.5 + sigmoid( conetip(uv, pos, size, -16.) * ramp) * 0.5;
}

vec2 vortex_warp(vec2 uv, vec2 pos, float size, float ramp, vec2 rot)
{
    vec2 aspect = vec2(1.,iResolution.y/iResolution.x);

    vec2 pos_correct = 0.5 + (pos - 0.5);
    vec2 rot_uv = pos_correct + complex_mul((uv - pos_correct)*aspect, rot)/aspect;
    float _filter = warpFilter(uv, pos_correct, size, ramp);
    return mix(uv, rot_uv, _filter);
}

vec2 vortex_pair_warp(vec2 uv, vec2 pos, vec2 vel)
{
    vec2 aspect = vec2(1.,iResolution.y/iResolution.x);
    float ramp = 5.;

    float d = 0.2;

    float l = length(vel);
    vec2 p1 = pos;
    vec2 p2 = pos;

    if(l > 0.){
        vec2 normal = normalize(vel.yx * vec2(-1., 1.))/aspect;
        p1 = pos - normal * d / 2.;
        p2 = pos + normal * d / 2.;
    }

    float w = l / d * 2.;

    // two overlapping rotations that would annihilate when they were not displaced.
    vec2 circle1 = vortex_warp(uv, p1, d, ramp, vec2(cos(w),sin(w)));
    vec2 circle2 = vortex_warp(uv, p2, d, ramp, vec2(cos(-w),sin(-w)));
    return (circle1 + circle2) / 2.;
}

vec2 mouseDelta(){
    vec2 pixelSize = 1. / iResolution.xy;
    float eighth = 1./8.;
    vec4 oldMouse = texture(iChannel2, vec2(7.5 * eighth, 2.5 * eighth));
    vec4 nowMouse = vec4(iMouse.xy / iResolution.xy, iMouse.zw / iResolution.xy);
    if(oldMouse.z > pixelSize.x && oldMouse.w > pixelSize.y && 
       nowMouse.z > pixelSize.x && nowMouse.w > pixelSize.y)
    {
        return nowMouse.xy - oldMouse.xy;
    }
    return vec2(0.);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord.xy / iResolution.xy;
    vec2 pixelSize = 1. / iResolution.xy;
    

    vec2 mouseV = mouseDelta();
    vec2 aspect = vec2(1.,iResolution.y/iResolution.x);
    uv = vortex_pair_warp(uv, iMouse.xy*pixelSize, mouseV*aspect*1.4);

    vec4 blur1 = texture(iChannel1, uv);
    
    vec4 noise = texture(iChannel3, fragCoord.xy / iChannelResolution[3].xy + fract(vec2(42,56)*iTime));

    // get the gradients from the blurred image
	vec2 d = pixelSize*4.;
	vec4 dx = (texture(iChannel1, fract(uv + vec2(1,0)*d)) - texture(iChannel1, fract(uv - vec2(1,0)*d))) * 0.5;
	vec4 dy = (texture(iChannel1, fract(uv + vec2(0,1)*d)) - texture(iChannel1, fract(uv - vec2(0,1)*d))) * 0.5;
    
    vec2 uv_red = uv + vec2(dx.x, dy.x)*pixelSize*8.; // add some diffusive expansion
    
    float new_red = texture(iChannel0, fract(uv_red)).x + (noise.x - 0.5) * 0.0025 - 0.002; // stochastic decay
	new_red -= (texture(iChannel1, fract(uv_red + (noise.xy-0.5)*pixelSize)).x -
				texture(iChannel0, fract(uv_red + (noise.xy-0.5)*pixelSize))).x * 0.047; // reaction-diffusion
        
    if(iFrame<10)
    {
        fragColor = noise; 
    }
    else
    {
        fragColor.x = clamp(new_red, 0., 1.);
    }

//    fragColor = noise; // need a restart?
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// horizontal Gaussian blur pass

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 pixelSize = 1./ iChannelResolution[0].xy;
    vec2 uv = fragCoord.xy * pixelSize;
    
    float h = pixelSize.x;
	vec4 sum = vec4(0.0);
	sum += texture(iChannel0, fract(vec2(uv.x - 4.0*h, uv.y)) ) * 0.05;
	sum += texture(iChannel0, fract(vec2(uv.x - 3.0*h, uv.y)) ) * 0.09;
	sum += texture(iChannel0, fract(vec2(uv.x - 2.0*h, uv.y)) ) * 0.12;
	sum += texture(iChannel0, fract(vec2(uv.x - 1.0*h, uv.y)) ) * 0.15;
	sum += texture(iChannel0, fract(vec2(uv.x + 0.0*h, uv.y)) ) * 0.16;
	sum += texture(iChannel0, fract(vec2(uv.x + 1.0*h, uv.y)) ) * 0.15;
	sum += texture(iChannel0, fract(vec2(uv.x + 2.0*h, uv.y)) ) * 0.12;
	sum += texture(iChannel0, fract(vec2(uv.x + 3.0*h, uv.y)) ) * 0.09;
	sum += texture(iChannel0, fract(vec2(uv.x + 4.0*h, uv.y)) ) * 0.05;
    
    fragColor.xyz = sum.xyz/0.98; // normalize
	fragColor.a = 1.;
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// vertical Gaussian blur pass

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 pixelSize = 1./ iChannelResolution[0].xy;
    vec2 uv = fragCoord.xy * pixelSize;

    float v = pixelSize.y;
	vec4 sum = vec4(0.0);
	sum += texture(iChannel0, fract(vec2(uv.x, uv.y - 4.0*v)) ) * 0.05;
	sum += texture(iChannel0, fract(vec2(uv.x, uv.y - 3.0*v)) ) * 0.09;
	sum += texture(iChannel0, fract(vec2(uv.x, uv.y - 2.0*v)) ) * 0.12;
	sum += texture(iChannel0, fract(vec2(uv.x, uv.y - 1.0*v)) ) * 0.15;
	sum += texture(iChannel0, fract(vec2(uv.x, uv.y + 0.0*v)) ) * 0.16;
	sum += texture(iChannel0, fract(vec2(uv.x, uv.y + 1.0*v)) ) * 0.15;
	sum += texture(iChannel0, fract(vec2(uv.x, uv.y + 2.0*v)) ) * 0.12;
	sum += texture(iChannel0, fract(vec2(uv.x, uv.y + 3.0*v)) ) * 0.09;
	sum += texture(iChannel0, fract(vec2(uv.x, uv.y + 4.0*v)) ) * 0.05;
    
    fragColor.xyz = sum.xyz/0.98; // normalize
	fragColor.a = 1.;
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
// not used (yet), but hooray for 8 channel feedback

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord.xy / iResolution.xy;
    vec2 pixelSize = 1. / iResolution.xy;
    float eighth = 1./8.;
    if(uv.x > 7.*eighth && uv.x < 8.*eighth && uv.y > 2.*eighth && uv.y < 3.*eighth)
    {
        fragColor = vec4(iMouse.xy / iResolution.xy, iMouse.zw / iResolution.xy);
    }
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<

// rotation matrix
mat2 rot (float a) { return mat2(cos(a),-sin(a),sin(a),cos(a)); }

// generated with discrete Fourier transform
vec2 cookie(float t) {
	return vec2(0.08+cos(t-1.58)*0.23+cos(t*2.-1.24)*0.14+cos(t*3.-1.12)*0.09+cos(t*4.-0.76)*0.06+cos(t*5.-0.59)*0.05+cos(t*6.+0.56)*0.03+cos(t*7.-2.73)*0.03+cos(t*8.-1.26)*0.02+cos(t*9.-1.44)*0.02+cos(t*10.-2.09)*0.03+cos(t*11.-2.18)*0.01+cos(t*12.-1.91)*0.02,cos(3.14)*0.05+cos(t+0.35)*0.06+cos(t*2.+0.54)*0.09+cos(t*3.+0.44)*0.03+cos(t*4.+1.02)*0.07+cos(t*6.+0.39)*0.03+cos(t*7.-1.48)*0.02+cos(t*8.-3.06)*0.02+cos(t*9.-0.39)*0.07+cos(t*10.-0.39)*0.03+cos(t*11.-0.03)*0.04+cos(t*12.-2.08)*0.02);
}
