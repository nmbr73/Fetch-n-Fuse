

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord.xy / iResolution.xy;
    vec2 pixelSize = 1. / iResolution.xy;
    vec2 aspect = vec2(1.,iResolution.y/iResolution.x);

    vec4 noise = texture(iChannel3, fragCoord.xy / iChannelResolution[3].xy + fract(vec2(42,56)*iTime));
    
	vec2 lightSize=vec2(4.);

    // get the gradients from the blurred image
	vec2 d = pixelSize*2.;
	vec4 dx = (texture(iChannel2, uv + vec2(1,0)*d) - texture(iChannel2, uv - vec2(1,0)*d))*0.5;
	vec4 dy = (texture(iChannel2, uv + vec2(0,1)*d) - texture(iChannel2, uv - vec2(0,1)*d))*0.5;

	// add the pixel gradients
	d = pixelSize*1.;
	dx += texture(iChannel0, uv + vec2(1,0)*d) - texture(iChannel0, uv - vec2(1,0)*d);
	dy += texture(iChannel0, uv + vec2(0,1)*d) - texture(iChannel0, uv - vec2(0,1)*d);

	vec2 displacement = vec2(dx.x,dy.x)*lightSize; // using only the red gradient as displacement vector
	float light = pow(max(1.-distance(0.5+(uv-0.5)*aspect*lightSize + displacement,0.5+(iMouse.xy*pixelSize-0.5)*aspect*lightSize),0.),4.);

	// recolor the red channel
	vec4 rd = vec4(texture(iChannel0,uv+vec2(dx.x,dy.x)*pixelSize*8.).x)*vec4(0.7,1.5,2.0,1.0)-vec4(0.3,1.0,1.0,1.0);

    // and add the light map
    fragColor = mix(rd,vec4(8.0,6.,2.,1.), light*0.75*vec4(1.-texture(iChannel0,uv+vec2(dx.x,dy.x)*pixelSize*8.).x)); 
	
	//fragColor = texture(iChannel0, uv); // bypass    
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