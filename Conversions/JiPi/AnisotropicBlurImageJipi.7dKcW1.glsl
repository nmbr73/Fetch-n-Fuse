

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define SIGMOID_CONTRAST 12.0

vec4 contrast(vec4 x, float s) {
	return 1.0 / (1.0 + exp(-s * (x - 0.5)));    
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 texel = 1. / iResolution.xy;
    vec2 uv = fragCoord.xy / iResolution.xy;
    fragColor = contrast(texture(iChannel0, uv), SIGMOID_CONTRAST);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
/*
    Generates a vector field using a dynamical system. 
	To see it in action on its own see this shadertoy:
    https://www.shadertoy.com/view/XddSRX
*/

bool reset() {
    return texture(iChannel3, vec2(32.5/256.0, 0.5) ).x > 0.5;
}

vec2 normz(vec2 x) {
	return x == vec2(0.0, 0.0) ? vec2(0.0, 0.0) : normalize(x);
}

// reverse advection
vec3 advect(vec2 ab, vec2 vUv, vec2 step, float sc) {
    
    vec2 aUv = vUv - ab * sc * step;
    
    const float _G0 = 0.25; // center weight
    const float _G1 = 0.125; // edge-neighbors
    const float _G2 = 0.0625; // vertex-neighbors
    
    // 3x3 neighborhood coordinates
    float step_x = step.x;
    float step_y = step.y;
    vec2 n  = vec2(0.0, step_y);
    vec2 ne = vec2(step_x, step_y);
    vec2 e  = vec2(step_x, 0.0);
    vec2 se = vec2(step_x, -step_y);
    vec2 s  = vec2(0.0, -step_y);
    vec2 sw = vec2(-step_x, -step_y);
    vec2 w  = vec2(-step_x, 0.0);
    vec2 nw = vec2(-step_x, step_y);

    vec3 uv =    texture(iChannel0, fract(aUv)).xyz;
    vec3 uv_n =  texture(iChannel0, fract(aUv+n)).xyz;
    vec3 uv_e =  texture(iChannel0, fract(aUv+e)).xyz;
    vec3 uv_s =  texture(iChannel0, fract(aUv+s)).xyz;
    vec3 uv_w =  texture(iChannel0, fract(aUv+w)).xyz;
    vec3 uv_nw = texture(iChannel0, fract(aUv+nw)).xyz;
    vec3 uv_sw = texture(iChannel0, fract(aUv+sw)).xyz;
    vec3 uv_ne = texture(iChannel0, fract(aUv+ne)).xyz;
    vec3 uv_se = texture(iChannel0, fract(aUv+se)).xyz;
    
    return _G0*uv + _G1*(uv_n + uv_e + uv_w + uv_s) + _G2*(uv_nw + uv_sw + uv_ne + uv_se);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    const float _K0 = -20.0/6.0; // center weight
    const float _K1 = 4.0/6.0;   // edge-neighbors
    const float _K2 = 1.0/6.0;   // vertex-neighbors
    const float cs = -0.6;  // curl scale
    const float ls = 0.05;  // laplacian scale
    const float ps = -0.8;  // laplacian of divergence scale
    const float ds = -0.05; // divergence scale
    const float dp = -0.04; // divergence update scale
    const float pl = 0.3;   // divergence smoothing
    const float ad = 6.0;   // advection distance scale
    const float pwr = 1.0;  // power when deriving rotation angle from curl
    const float amp = 1.0;  // self-amplification
    const float upd = 0.8;  // update smoothing
    const float sq2 = 0.6;  // diagonal weight

    vec2 vUv = fragCoord.xy / iResolution.xy;
    vec2 texel = 1. / iResolution.xy;
    
    // 3x3 neighborhood coordinates
    float step_x = texel.x;
    float step_y = texel.y;
    vec2 n  = vec2(0.0, step_y);
    vec2 ne = vec2(step_x, step_y);
    vec2 e  = vec2(step_x, 0.0);
    vec2 se = vec2(step_x, -step_y);
    vec2 s  = vec2(0.0, -step_y);
    vec2 sw = vec2(-step_x, -step_y);
    vec2 w  = vec2(-step_x, 0.0);
    vec2 nw = vec2(-step_x, step_y);

    vec3 uv =    texture(iChannel0, fract(vUv)).xyz;
    vec3 uv_n =  texture(iChannel0, fract(vUv+n)).xyz;
    vec3 uv_e =  texture(iChannel0, fract(vUv+e)).xyz;
    vec3 uv_s =  texture(iChannel0, fract(vUv+s)).xyz;
    vec3 uv_w =  texture(iChannel0, fract(vUv+w)).xyz;
    vec3 uv_nw = texture(iChannel0, fract(vUv+nw)).xyz;
    vec3 uv_sw = texture(iChannel0, fract(vUv+sw)).xyz;
    vec3 uv_ne = texture(iChannel0, fract(vUv+ne)).xyz;
    vec3 uv_se = texture(iChannel0, fract(vUv+se)).xyz;
    
    // uv.x and uv.y are the x and y components, uv.z is divergence 

    // laplacian of all components
    vec3 lapl  = _K0*uv + _K1*(uv_n + uv_e + uv_w + uv_s) + _K2*(uv_nw + uv_sw + uv_ne + uv_se);
    float sp = ps * lapl.z;
    
    // calculate curl
    // vectors point clockwise about the center point
    float curl = uv_n.x - uv_s.x - uv_e.y + uv_w.y + sq2 * (uv_nw.x + uv_nw.y + uv_ne.x - uv_ne.y + uv_sw.y - uv_sw.x - uv_se.y - uv_se.x);
    
    // compute angle of rotation from curl
    float sc = cs * sign(curl) * pow(abs(curl), pwr);
    
    // calculate divergence
    // vectors point inwards towards the center point
    float div  = uv_s.y - uv_n.y - uv_e.x + uv_w.x + sq2 * (uv_nw.x - uv_nw.y - uv_ne.x - uv_ne.y + uv_sw.x + uv_sw.y + uv_se.y - uv_se.x);
    float sd = uv.z + dp * div + pl * lapl.z;

    vec2 norm = normz(uv.xy);
    
    vec3 ab = advect(vec2(uv.x, uv.y), vUv, texel, ad);
    
    // temp values for the update rule
    float ta = amp * ab.x + ls * lapl.x + norm.x * sp + uv.x * ds * sd;
    float tb = amp * ab.y + ls * lapl.y + norm.y * sp + uv.y * ds * sd;

    // rotate
    float a = ta * cos(sc) - tb * sin(sc);
    float b = ta * sin(sc) + tb * cos(sc);
    
    vec3 abd = upd * uv + (1.0 - upd) * vec3(a,b,sd);
    
    if (iMouse.z > 0.0) {
    	vec2 d = fragCoord.xy - iMouse.xy;
        float m = exp(-length(d) / 20.0);
        abd.xy += m * normz(d);
    }
    
    vec3 init = texture(iChannel1, fragCoord.xy / iResolution.xy).xyz;
    // initialize with noise
    if((uv == vec3(0.0) && init != vec3(0.0)) || reset()) {
        fragColor = vec4(init - 0.5, 0.0);
    } else {
        abd.z = clamp(abd.z, -1.0, 1.0);
        abd.xy = clamp(length(abd.xy) > 1.0 ? normz(abd.xy) : abd.xy, -1.0, 1.0);
        fragColor = vec4(abd, 0.0);
    }
    

}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
/*
	Blurs each pixel with its neighbors according to the underlying
	vector field in Buffer A.
*/

#define GAMMA_CORRECT		// use gamma-corrected blending
#define GAMMA vec3(2.2)     // gamma
#define NORMALIZE_AB		// normalize the vector value
#define BLUR_RATIO 0.2		// ratio of the original pixel value to the blurred value
#define SHARPNESS 1.0		// sharpness of the blur kernel, 0.0 gives a uniform distribution
#define VECTOR_SHARPEN 12.0 // sharpens the vector field

bool reset() {
    return texture(iChannel3, vec2(32.5/256.0, 0.5) ).x > 0.5;
}

vec2 normz(vec2 x) {
	return x == vec2(0.0, 0.0) ? vec2(0.0, 0.0) : normalize(x);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    
    vec2 vUv = fragCoord.xy / iResolution.xy;
    vec2 texel = 1. / iResolution.xy;
    
    // 3x3 neighborhood coordinates
    float step_x = texel.x;
    float step_y = texel.y;
    vec2 n  = vec2(0.0, step_y);
    vec2 ne = vec2(step_x, step_y);
    vec2 e  = vec2(step_x, 0.0);
    vec2 se = vec2(step_x, -step_y);
    vec2 s  = vec2(0.0, -step_y);
    vec2 sw = vec2(-step_x, -step_y);
    vec2 w  = vec2(-step_x, 0.0);
    vec2 nw = vec2(-step_x, step_y);
    
    vec2 ab =    texture(iChannel2, fract(vUv)).xy;
    vec2 ab_n =  texture(iChannel2, fract(vUv+n)).xy;
    vec2 ab_e =  texture(iChannel2, fract(vUv+e)).xy;
    vec2 ab_s =  texture(iChannel2, fract(vUv+s)).xy;
    vec2 ab_w =  texture(iChannel2, fract(vUv+w)).xy;
    vec2 ab_nw = texture(iChannel2, fract(vUv+nw)).xy;
    vec2 ab_sw = texture(iChannel2, fract(vUv+sw)).xy;
    vec2 ab_ne = texture(iChannel2, fract(vUv+ne)).xy;
    vec2 ab_se = texture(iChannel2, fract(vUv+se)).xy;
    
    const float _K0 = -20.0/6.0; // center weight
    const float _K1 = 4.0/6.0;   // edge-neighbors
    const float _K2 = 1.0/6.0;   // vertex-neighbors
    
    // laplacian
    vec2 lapl  = _K0*ab + _K1*(ab_n + ab_e + ab_w + ab_s) + _K2*(ab_nw + ab_sw + ab_ne + ab_se);
    
    ab += -VECTOR_SHARPEN * lapl;
    
    #ifdef NORMALIZE_AB
    	ab = normz(ab);
    #endif
    
    
    #ifdef GAMMA_CORRECT
        vec3 im =    pow(texture(iChannel1, fract(vUv)).xyz, GAMMA);
        vec3 im_n =  pow(texture(iChannel1, fract(vUv+n)).xyz, GAMMA);
        vec3 im_e =  pow(texture(iChannel1, fract(vUv+e)).xyz, GAMMA);
        vec3 im_s =  pow(texture(iChannel1, fract(vUv+s)).xyz, GAMMA);
        vec3 im_w =  pow(texture(iChannel1, fract(vUv+w)).xyz, GAMMA);
        vec3 im_nw = pow(texture(iChannel1, fract(vUv+nw)).xyz, GAMMA);
        vec3 im_sw = pow(texture(iChannel1, fract(vUv+sw)).xyz, GAMMA);
        vec3 im_ne = pow(texture(iChannel1, fract(vUv+ne)).xyz, GAMMA);
        vec3 im_se = pow(texture(iChannel1, fract(vUv+se)).xyz, GAMMA);
    #else
        vec3 im =    texture(iChannel1, fract(vUv)).xyz;
        vec3 im_n =  texture(iChannel1, fract(vUv+n)).xyz;
        vec3 im_e =  texture(iChannel1, fract(vUv+e)).xyz;
        vec3 im_s =  texture(iChannel1, fract(vUv+s)).xyz;
        vec3 im_w =  texture(iChannel1, fract(vUv+w)).xyz;
        vec3 im_nw = texture(iChannel1, fract(vUv+nw)).xyz;
        vec3 im_sw = texture(iChannel1, fract(vUv+sw)).xyz;
        vec3 im_ne = texture(iChannel1, fract(vUv+ne)).xyz;
        vec3 im_se = texture(iChannel1, fract(vUv+se)).xyz;
    #endif

    // a gaussian centered around the point at 'ab'
    #define e(x,y) exp(-SHARPNESS * dot(vec2(x,y) - ab, vec2(x,y) - ab))
    
    float D_c =  e( 0.0, 0.0);
    float D_e =  e( 1.0, 0.0);
    float D_w =  e(-1.0, 0.0);
    float D_n =  e( 0.0, 1.0);
    float D_s =  e( 0.0,-1.0);
    float D_ne = e( 1.0, 1.0);
    float D_nw = e(-1.0, 1.0);
    float D_se = e( 1.0,-1.0);
    float D_sw = e(-1.0,-1.0);
    
    // normalize the blur kernel
    float dn = D_c + D_e + D_w + D_n + D_s + D_ne + D_nw + D_se + D_sw;

    vec3 blur_im = (D_c*im 
        + im_n*D_n + im_ne*D_ne 
        + im_e*D_e + im_se*D_se 
        + im_s*D_s + im_sw*D_sw 
        + im_w*D_w + im_nw*D_nw) / dn;
    
    #ifdef GAMMA_CORRECT
    	blur_im = pow(blur_im, 1.0 / GAMMA);
    	im = pow(im, 1.0 / GAMMA);
    #endif

    // initialize with image
    vec4 init = texture(iChannel0, fragCoord.xy / iResolution.xy);
    if((im == vec3(0.0) && init != vec4(0.0)) || reset()) {
        fragColor = init;
    } else {
        fragColor = vec4(clamp(BLUR_RATIO * im + (1.0 - BLUR_RATIO) * blur_im, 0.0, 1.0), 0.0);
    }
}