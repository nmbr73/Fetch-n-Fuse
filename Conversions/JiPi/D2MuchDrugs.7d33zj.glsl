

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Fork of "Another Mess with Dispersion" by xenn. https://shadertoy.com/view/Ndc3zj
// 2021-08-28 16:03:28

// Fork of "A Handsome Mess with Dispersion" by xenn. https://shadertoy.com/view/fdc3zj
// 2021-08-28 10:49:54

// created by florian berger (flockaroo) - 2016
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

// single pass CFD
// ---------------
// this is some "computational flockarooid dynamics" ;)
// the self-advection is done purely rotational on all scales. 
// therefore i dont need any divergence-free velocity field. 
// with stochastic sampling i get the proper "mean values" of rotations 
// over time for higher order scales.
//
// try changing "RotNum" for different accuracies of rotation calculation
// for even RotNum uncomment the line #define SUPPORT_EVEN_ROTNUM

float getVal(vec2 uv)
{
    return length(texture(iChannel0,uv).xyz);
}
    
vec2 getGrad(vec2 uv,float delta)
{
    vec2 d=vec2(delta,0);
    return vec2(
        getVal(uv+d.xy)-getVal(uv-d.xy),
        getVal(uv+d.yx)-getVal(uv-d.yx)
    )/delta;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord.xy / iResolution.xy;
    vec3 n = vec3(getGrad(uv,1.0/iResolution.y),999.0);
    //n *= n;
    n=normalize(n);
    fragColor=vec4(n,1);
    vec3 light = normalize(vec3(1,1,2));
    float diff=clamp(dot(n,light),0.5,1.0);
    float spec=clamp(dot(reflect(light,n),vec3(0,0,-1)),0.0,1.0);
    spec=pow(spec,36.0)*2.5;
    //spec=0.0;
	fragColor = texture(iChannel2,uv)*vec4(diff)+vec4(spec);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
/*
    A fracturing dynamical system
	see: https://www.shadertoy.com/view/MsyXRW
*/

#define _G0 0.25
#define _G1 0.125
#define _G2 0.0625
#define W0 -3.0
#define W1 0.5
#define TIMESTEP 0.1
#define ADVECT_DIST 2.0
#define DV 0.70710678

// nonlinearity
float nl(float x) {
    return 1.0 / (1.0 + exp(W0 * (W1 * x - 0.5))); 
}

vec4 gaussian(vec4 x, vec4 x_nw, vec4 x_n, vec4 x_ne, vec4 x_w, vec4 x_e, vec4 x_sw, vec4 x_s, vec4 x_se) {
    return _G0*x + _G1*(x_n + x_e + x_w + x_s) + _G2*(x_nw + x_sw + x_ne + x_se);
}

bool reset() {
    return texture(iChannel3, vec2(32.5/256.0, 0.5) ).x > 0.5;
}

vec2 normz(vec2 x) {
	return x == vec2(0.0, 0.0) ? vec2(0.0, 0.0) : normalize(x);
}

vec4 advect(vec2 ab, vec2 vUv, vec2 step) {
    
    vec2 aUv = vUv - ab * ADVECT_DIST * step;
    
    vec2 n  = vec2(0.0, step.y);
    vec2 ne = vec2(step.x, step.y);
    vec2 e  = vec2(step.x, 0.0);
    vec2 se = vec2(step.x, -step.y);
    vec2 s  = vec2(0.0, -step.y);
    vec2 sw = vec2(-step.x, -step.y);
    vec2 w  = vec2(-step.x, 0.0);
    vec2 nw = vec2(-step.x, step.y);

    vec4 u =    texture(iChannel0, fract(aUv));
    vec4 u_n =  texture(iChannel0, fract(aUv+n));
    vec4 u_e =  texture(iChannel0, fract(aUv+e));
    vec4 u_s =  texture(iChannel0, fract(aUv+s));
    vec4 u_w =  texture(iChannel0, fract(aUv+w));
    vec4 u_nw = texture(iChannel0, fract(aUv+nw));
    vec4 u_sw = texture(iChannel0, fract(aUv+sw));
    vec4 u_ne = texture(iChannel0, fract(aUv+ne));
    vec4 u_se = texture(iChannel0, fract(aUv+se));
    
    return gaussian(u, u_nw, u_n, u_ne, u_w, u_e, u_sw, u_s, u_se);
}

#define SQRT_3_OVER_2 0.86602540378
#define SQRT_3_OVER_2_INV 0.13397459621

vec2 diagH(vec2 x, vec2 x_v, vec2 x_h, vec2 x_d) {
    return 0.5 * ((x + x_v) * SQRT_3_OVER_2_INV + (x_h + x_d) * SQRT_3_OVER_2);
}

vec2 diagV(vec2 x, vec2 x_v, vec2 x_h, vec2 x_d) {
    return 0.5 * ((x + x_h) * SQRT_3_OVER_2_INV + (x_v + x_d) * SQRT_3_OVER_2);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 vUv = fragCoord.xy / iResolution.xy;
    vec2 texel = 1. / iResolution.xy;
    
    vec2 n  = vec2(0.0, 1.0);
    vec2 ne = vec2(1.0, 1.0);
    vec2 e  = vec2(1.0, 0.0);
    vec2 se = vec2(1.0, -1.0);
    vec2 s  = vec2(0.0, -1.0);
    vec2 sw = vec2(-1.0, -1.0);
    vec2 w  = vec2(-1.0, 0.0);
    vec2 nw = vec2(-1.0, 1.0);

/*    vec4 u =    texture(iChannel0, fract(vUv));
    vec4 u_n =  texture(iChannel0, fract(vUv+texel*n));
    vec4 u_e =  texture(iChannel0, fract(vUv+texel*e));
    vec4 u_s =  texture(iChannel0, fract(vUv+texel*s));
    vec4 u_w =  texture(iChannel0, fract(vUv+texel*w));
    vec4 u_nw = texture(iChannel0, fract(vUv+texel*nw));
    vec4 u_sw = texture(iChannel0, fract(vUv+texel*sw));
    vec4 u_ne = texture(iChannel0, fract(vUv+texel*ne));
    vec4 u_se = texture(iChannel0, fract(vUv+texel*se));
*/    
    vec4 u =    texture(iChannel2, fract(vUv));
    vec4 u_n =  texture(iChannel2, fract(vUv+texel*n));
    vec4 u_e =  texture(iChannel2, fract(vUv+texel*e));
    vec4 u_s =  texture(iChannel2, fract(vUv+texel*s));
    vec4 u_w =  texture(iChannel2, fract(vUv+texel*w));
    vec4 u_nw = texture(iChannel2, fract(vUv+texel*nw));
    vec4 u_sw = texture(iChannel2, fract(vUv+texel*sw));
    vec4 u_ne = texture(iChannel2, fract(vUv+texel*ne));
    vec4 u_se = texture(iChannel2, fract(vUv+texel*se));
    
    
    const float vx = 0.5;
    const float vy = SQRT_3_OVER_2;
    const float hx = SQRT_3_OVER_2;
    const float hy = 0.5;

    float di_n  = nl(distance(u_n.xy + n, u.xy));
    float di_w  = nl(distance(u_w.xy + w, u.xy));
    float di_e  = nl(distance(u_e.xy + e, u.xy));
    float di_s  = nl(distance(u_s.xy + s, u.xy));
    
    float di_nne = nl(distance((diagV(u.xy, u_n.xy, u_e.xy, u_ne.xy) + vec2(+ vx, + vy)), u.xy));
    float di_ene = nl(distance((diagH(u.xy, u_n.xy, u_e.xy, u_ne.xy) + vec2(+ hx, + hy)), u.xy));
    float di_ese = nl(distance((diagH(u.xy, u_s.xy, u_e.xy, u_se.xy) + vec2(+ hx, - hy)), u.xy));
    float di_sse = nl(distance((diagV(u.xy, u_s.xy, u_e.xy, u_se.xy) + vec2(+ vx, - vy)), u.xy));    
    float di_ssw = nl(distance((diagV(u.xy, u_s.xy, u_w.xy, u_sw.xy) + vec2(- vx, - vy)), u.xy));
    float di_wsw = nl(distance((diagH(u.xy, u_s.xy, u_w.xy, u_sw.xy) + vec2(- hx, - hy)), u.xy));
    float di_wnw = nl(distance((diagH(u.xy, u_n.xy, u_w.xy, u_nw.xy) + vec2(- hx, + hy)), u.xy));
    float di_nnw = nl(distance((diagV(u.xy, u_n.xy, u_w.xy, u_nw.xy) + vec2(- vx, + vy)), u.xy));

    vec2 xy_n  = u_n.xy + n - u.xy;
    vec2 xy_w  = u_w.xy + w - u.xy;
    vec2 xy_e  = u_e.xy + e - u.xy;
    vec2 xy_s  = u_s.xy + s - u.xy;
    
    vec2 xy_nne = (diagV(u.xy, u_n.xy, u_e.xy, u_ne.xy) + vec2(+ vx, + vy)) - u.xy;
    vec2 xy_ene = (diagH(u.xy, u_n.xy, u_e.xy, u_ne.xy) + vec2(+ hx, + hy)) - u.xy;
    vec2 xy_ese = (diagH(u.xy, u_s.xy, u_e.xy, u_se.xy) + vec2(+ hx, - hy)) - u.xy;
    vec2 xy_sse = (diagV(u.xy, u_s.xy, u_e.xy, u_se.xy) + vec2(+ vx, - vy)) - u.xy;
    vec2 xy_ssw = (diagV(u.xy, u_s.xy, u_w.xy, u_sw.xy) + vec2(- vx, - vy)) - u.xy;
    vec2 xy_wsw = (diagH(u.xy, u_s.xy, u_w.xy, u_sw.xy) + vec2(- hx, - hy)) - u.xy;
    vec2 xy_wnw = (diagH(u.xy, u_n.xy, u_w.xy, u_nw.xy) + vec2(- hx, + hy)) - u.xy;
    vec2 xy_nnw = (diagV(u.xy, u_n.xy, u_w.xy, u_nw.xy) + vec2(- vx, + vy)) - u.xy;

    vec2 ma = di_nne * xy_nne + di_ene * xy_ene + di_ese * xy_ese + di_sse * xy_sse + di_ssw * xy_ssw + di_wsw * xy_wsw + di_wnw * xy_wnw + di_nnw * xy_nnw + di_n * xy_n + di_w * xy_w + di_e * xy_e + di_s * xy_s;

    vec4 u_blur = gaussian(u, u_nw, u_n, u_ne, u_w, u_e, u_sw, u_s, u_se);
    
    vec4 au = advect(u.xy, vUv, texel);
    vec4 av = advect(u.zw, vUv, texel);
    
    vec2 dv = av.zw + TIMESTEP * ma;
    vec2 du = au.xy + TIMESTEP * dv;

    if (iMouse.z > 0.0) {
    	vec2 d = fragCoord.xy - iMouse.xy;
        float m = exp(-length(d) / 50.0);
        du += 0.2 * m * normz(d);
    }
    
    vec2 init = texture(iChannel1, vUv, 4.0).xy;
    // initialize with noise
    if((length(u) < 0.001 && length(init) > 0.001) || reset()) {
        fragColor = 8.0 * (vec4(-0.5) + vec4(init.xy, init.xy));
    } else {
        du = length(du) > 1.0 ? normz(du) : du;
        fragColor = vec4(du, dv);
    }
    

}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
/*
    A fluid-like dynamical system
	see: https://www.shadertoy.com/view/XddSRX
*/

vec2 normz(vec2 x) {
	return x == vec2(0.0, 0.0) ? vec2(0.0, 0.0) : normalize(x);
}

// reverse advection
vec4 advect(vec2 ab, vec2 vUv, vec2 step, float sc) {
    
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

    vec4 uv =    texture(iChannel0, fract(aUv));
    vec4 uv_n =  texture(iChannel0, fract(aUv+n));
    vec4 uv_e =  texture(iChannel0, fract(aUv+e));
    vec4 uv_s =  texture(iChannel0, fract(aUv+s));
    vec4 uv_w =  texture(iChannel0, fract(aUv+w));
    vec4 uv_nw = texture(iChannel0, fract(aUv+nw));
    vec4 uv_sw = texture(iChannel0, fract(aUv+sw));
    vec4 uv_ne = texture(iChannel0, fract(aUv+ne));
    vec4 uv_se = texture(iChannel0, fract(aUv+se));
    
    return _G0*uv + _G1*(uv_n + uv_e + uv_w + uv_s) + _G2*(uv_nw + uv_sw + uv_ne + uv_se);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    const float _K0 = -20.0/6.0; // center weight
    const float _K1 = 4.0/6.0;   // edge-neighbors
    const float _K2 = 1.0/6.0;   // vertex-neighbors
    const float cs = -3.0;  // curl scale
    const float ls = 3.0;  // laplacian scale
    const float ps = 0.0;  // laplacian of divergence scale
    const float ds = -12.0; // divergence scale
    const float dp = -6.0; // divergence update scale
    const float pl = 0.3;   // divergence smoothing
    const float ad = 6.0;   // advection distance scale
    const float pwr = 1.0;  // power when deriving rotation angle from curl
    const float amp = 1.0;  // self-amplification
    const float upd = 0.99;  // update smoothing
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

 /*   vec4 uv =    texture(iChannel0, fract(vUv));
    vec4 uv_n =  texture(iChannel0, fract(vUv+n));
    vec4 uv_e =  texture(iChannel0, fract(vUv+e));
    vec4 uv_s =  texture(iChannel0, fract(vUv+s));
    vec4 uv_w =  texture(iChannel0, fract(vUv+w));
    vec4 uv_nw = texture(iChannel0, fract(vUv+nw));
    vec4 uv_sw = texture(iChannel0, fract(vUv+sw));
    vec4 uv_ne = texture(iChannel0, fract(vUv+ne));
    vec4 uv_se = texture(iChannel0, fract(vUv+se));
  */  
     vec4 uv =    texture(iChannel2, fract(vUv));
    vec4 uv_n =  texture(iChannel2, fract(vUv+n));
    vec4 uv_e =  texture(iChannel2, fract(vUv+e));
    vec4 uv_s =  texture(iChannel2, fract(vUv+s));
    vec4 uv_w =  texture(iChannel2, fract(vUv+w));
    vec4 uv_nw = texture(iChannel2, fract(vUv+nw));
    vec4 uv_sw = texture(iChannel2, fract(vUv+sw));
    vec4 uv_ne = texture(iChannel2, fract(vUv+ne));
    vec4 uv_se = texture(iChannel2, fract(vUv+se));
    
    // uv.x and uv.y are the x and y components, uv.z and uv.w accumulate divergence 

    // laplacian of all components
    vec4 lapl  = _K0*uv + _K1*(uv_n + uv_e + uv_w + uv_s) + _K2*(uv_nw + uv_sw + uv_ne + uv_se);
    
    // calculate curl
    // vectors point clockwise about the center point
    float curl = uv_n.x - uv_s.x - uv_e.y + uv_w.y + sq2 * (uv_nw.x + uv_nw.y + uv_ne.x - uv_ne.y + uv_sw.y - uv_sw.x - uv_se.y - uv_se.x);
    
    // compute angle of rotation from curl
    float sc = cs * sign(curl) * pow(abs(curl), pwr);
    
    // calculate divergence
    // vectors point inwards towards the center point
    float div  = uv_s.y - uv_n.y - uv_e.x + uv_w.x + sq2 * (uv_nw.x - uv_nw.y - uv_ne.x - uv_ne.y + uv_sw.x + uv_sw.y + uv_se.y - uv_se.x);
    
    vec2 norm = normz(uv.xy);
    
    float sdx = uv.z + dp * uv.x * div + pl * lapl.z;
    float sdy = uv.w + dp * uv.y * div + pl * lapl.w;

    vec2 ab = advect(vec2(uv.x, uv.y), vUv, texel, ad).xy;
    
    // temp values for the update rule
    float ta = amp * ab.x + ls * lapl.x + norm.x * ps * lapl.z + ds * sdx;
    float tb = amp * ab.y + ls * lapl.y + norm.y * ps * lapl.w + ds * sdy;

    // rotate
    float a = ta * cos(sc) - tb * sin(sc);
    float b = ta * sin(sc) + tb * cos(sc);
    
    vec4 abd = upd * uv + (1.0 - upd) * vec4(a,b,sdx,sdy);
    
    fragColor = vec4(abd);
    
    abd.xy = clamp(length(abd.xy) > 1.0 ? normz(abd.xy) : abd.xy, -1.0, 1.0);
    fragColor = vec4(abd);
    
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// created by florian berger (flockaroo) - 2016
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

// single pass CFD
// ---------------
// this is some "computational flockarooid dynamics" ;)
// the self-advection is done purely rotational on all scales. 
// therefore i dont need any divergence-free velocity field. 
// with stochastic sampling i get the proper "mean values" of rotations 
// over time for higher order scales.
//
// try changing "RotNum" for different accuracies of rotation calculation
// for even RotNum uncomment the line #define SUPPORT_EVEN_ROTNUM

#define RotNum 5
//#define SUPPORT_EVEN_ROTNUM

#define Res  iChannelResolution[0]
#define Res1 iChannelResolution[1]

#define keyTex iChannel3
#define KEY_I texture(keyTex,vec2((105.5-32.0)/256.0,(0.5+0.0)/3.0)).x

const float ang = 2.0*3.1415926535/float(RotNum);
mat2 m = mat2(cos(ang),sin(ang),-sin(ang),cos(ang));
mat2 mh = mat2(cos(ang*0.5),sin(ang*0.5),-sin(ang*0.5),cos(ang*0.5));

vec4 randS(vec2 uv)
{
    return texture(iChannel1,uv*Res.xy/Res1.xy)-vec4(0.5);
}

float getRot(vec2 pos, vec2 b)
{
    vec2 p = b;
    float rot=0.0;
    for(int i=0;i<RotNum;i++)
    {
        rot+=dot(texture(iChannel0,fract((pos+p)/Res.xy)).xy-vec2(0.5),p.yx*vec2(1,-1));
        p = m*p;
    }
    return rot/float(RotNum)/dot(b,b);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 pos = fragCoord.xy;
    float rnd = randS(vec2(float(iFrame)/Res.x,0.5/Res1.y)).x;
    
    vec2 b = vec2(cos(ang*rnd),sin(ang*rnd));
    vec2 v=vec2(0);
    float bbMax=0.7*Res.y; bbMax*=bbMax;
    for(int l=0;l<20;l++)
    {
        if ( dot(b,b) > bbMax ) break;
        vec2 p = b;
        for(int i=0;i<RotNum;i++)
        {
#ifdef SUPPORT_EVEN_ROTNUM
            v+=p.yx*getRot(pos+p,-mh*b);
#else
            // this is faster but works only for odd RotNum
            v+=p.yx*getRot(pos+p,b);
#endif
            p = m*p;
        }
        b*=2.0;
    }
    
    fragColor=texture(iChannel2,fract((pos+v*vec2(-1,1)*2.0)/Res.xy));
    
    // add a little "motor" in the center
    vec2 scr=(fragCoord.xy/Res.xy)*2.0-vec2(1.0);
    fragColor.xy += (0.001*scr.xy / (dot(scr,scr)/0.1+0.3));
    
    if(iFrame<=4 || KEY_I>0.5) fragColor=texture(iChannel2,fragCoord.xy/Res.xy);
}

// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
// Mashup Fork of "Displacement with Dispersion" by cornusammonis. https://shadertoy.com/view/4ldGDB
// 2021-08-28 10:34:29
// & this https://www.shadertoy.com/view/MsGSRd by flockeroo

// displacement amount
#define DISP_SCALE 2.0

// chromatic dispersion samples
#define SAMPLES 24

// contrast
#define SIGMOID_CONTRAST 12.0

// channels to use for displacement, either xy or zw
#define CH xy


vec3 contrast(vec3 x) {
	return 1.0 / (1.0 + exp(-SIGMOID_CONTRAST * (x - 0.5)));    
}

vec2 normz(vec2 x) {
	return x == vec2(0) ? vec2(0) : normalize(x);
}

/*
	This function supplies a weight vector for each color channel.
	It's analogous to (but not a physically accurate model of)
	the response curves for each of the 3 cone types in the human eye.
	The three functions for red, green, and blue have the same integral
    over [0, 1], which is 1/3.
    Here are some other potential terms for the green weight that 
	integrate to 1/3:
        2.0*(1-x)*x
        10.0*((1-x)*x)^2
        46.667*((1-i)*i)^3
        210.0*((1-x)*x)^4
        924.0*((1-x)*x)^5
    By the way, this series of coefficients is OEIS A004731 divided by 3,
    which is a pretty interesting series: https://oeis.org/A002457
*/
vec3 sampleWeights(float i) {
	return vec3(i * i, 46.6666*pow((1.0-i)*i,3.0), (1.0 - i) * (1.0 - i));
}

vec3 sampleDisp(vec2 uv, vec2 dispNorm, float disp) {
    vec3 col = vec3(0);
    const float SD = 1.0 / float(SAMPLES);
    float wl = 0.0;
    vec3 denom = vec3(0);
    for(int i = 0; i < SAMPLES; i++) {
        vec3 sw = sampleWeights(wl);
        denom += sw;
        col += sw * texture(iChannel1, uv + dispNorm * disp * wl).xyz;
        wl  += SD;
    }
    
    // For a large enough number of samples,
    // the return below is equivalent to 3.0 * col * SD;
    return col / denom;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord ){
    vec2 texel = 1. / iResolution.xy;
    vec2 uv = fragCoord.xy / iResolution.xy;

    vec2 n  = vec2(0.0, texel.y);
    vec2 e  = vec2(texel.x, 0.0);
    vec2 s  = vec2(0.0, -texel.y);
    vec2 w  = vec2(-texel.x, 0.0);

    vec2 d   = texture(iChannel0, uv).CH;
    vec2 d_n = texture(iChannel0, fract(uv+n)).CH;
    vec2 d_e = texture(iChannel0, fract(uv+e)).CH;
    vec2 d_s = texture(iChannel0, fract(uv+s)).CH;
    vec2 d_w = texture(iChannel0, fract(uv+w)).CH; 

    // antialias our vector field by blurring
    vec2 db = 0.4 * d + 0.15 * (d_n+d_e+d_s+d_w);

    float ld = length(db);
    vec2 ln = normz(db);

	vec3 col = sampleDisp(uv, ln, DISP_SCALE * ld);
    
    fragColor = vec4(contrast(col), 1.0);

}