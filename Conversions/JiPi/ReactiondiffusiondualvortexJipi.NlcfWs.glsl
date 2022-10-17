

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define _Smooth(p,r,s) smoothstep(-s, s, p-(r))

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord.xy / iResolution.xy;
    vec4 state = texture(iChannel0,uv);
	fragColor =  vec4(0.,state.y,state.y/state.x,1.);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
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
    float ramp = 8.;

    float d = 0.1;

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
    vec4 nowMouse = vec4(iMouse.xy * pixelSize.xy, iMouse.zw * pixelSize.xy);
    if(oldMouse.z > pixelSize.x && oldMouse.w > pixelSize.y && 
       nowMouse.z > pixelSize.x && nowMouse.w > pixelSize.y)
    {
        return nowMouse.xy - oldMouse.xy;
    }
    return vec2(0.);
}

// below code is forked from https://www.shadertoy.com/view/MlKXDw
#define sample(x,y) texture(iChannel0, (uv + vec2(x,y) / iResolution.xy))

vec2 difRate = vec2(1.,.25);

#define FEED .0367;
#define KILL .0649;

float zoom = .9997;

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord / iResolution.xy;
    vec2 texel = 1. / iResolution.xy;

// begin Felix' edit to the original by Cornus Ammonis
    vec2 mouseV = mouseDelta();
    vec2 aspect = vec2(1.,texel.x/texel.y);
    if(length(mouseV)==0. && iFrame > 4*1024){
        fragColor = texture(iChannel0, uv);
        return;
    }
    uv = vortex_pair_warp(uv, iMouse.xy*texel, mouseV*aspect*1.);
// end      
    
    uv =( uv - vec2(.5)) * zoom + vec2(.5);
    vec4 current = sample(0.,0.);
    
    vec4 cumul = current * -1.;
    
    cumul += (   sample( 1., 0.) 
               + sample(-1., 0.) 
               + sample( 0., 1.) 
               + sample( 0.,-1.)
             ) * .2;

    cumul += (
        sample( 1, 1) +
        sample( 1,-1) +
        sample(-1, 1) +
        sample(-1,-1) 
       )*.05;
    
    
    float feed = FEED;
    float kill = KILL;
    
    float dist = distance(uv,vec2(.5)) - .34;
    kill = kill + step(0.,dist) * dist*.25;
    
    vec4 lap =  cumul;
    float newR = current.r + (difRate.r * lap.r - current.r * current.g * current.g + feed * (1. - current.r));
    float newG = current.g + (difRate.g * lap.g + current.r * current.g * current.g - (kill + feed) * current.g);
    
    newR = clamp(newR,0.,1.);
    newG = clamp(newG,0.,1.);
    
    current = vec4(newR,newG,0.,1.);
    
    
        uv = (fragCoord / iResolution.y) -  vec2(iResolution.x /iResolution.y * .5,.5);
    	float f = step(length(uv),.25) - step(length(uv),.24);
    	f *=  .25 + fract(atan(uv.y,uv.x)*.5 + iTime*.5) * .25 * sin(iTime*.1);
        current = max(current, vec4(0.,1.,0.,1.) * f);
    
    if(iMouse.z > .5)
    {
        uv = (fragCoord - iMouse.xy) / iResolution.xy;
//        current = max(current,vec4(1.) * step(dot(uv,uv),.001225));
    }
    
    fragColor = current;
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord.xy / iResolution.xy;
    vec2 pixelSize = 1. / iResolution.xy;
    float eighth = 1./8.;
    if(uv.x > 7.*eighth && uv.x < 8.*eighth && uv.y > 2.*eighth && uv.y < 3.*eighth)
    {
        fragColor = vec4(iMouse.xy * pixelSize.xy, iMouse.zw * pixelSize.xy);
    }
}