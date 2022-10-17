
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: RGBA Noise Small' to iChannel1
// Connect Buffer A 'Preset: Keyboard' to iChannel3
// Connect Buffer A 'Previsualization: Buffer C' to iChannel0
// Connect Buffer A 'Previsualization: Buffer C' to iChannel2


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
__DEVICE__ float nl(float x) {
    return 1.0f / (1.0f + _expf(W0 * (W1 * x - 0.5f))); 
}

__DEVICE__ float4 gaussian(float4 x, float4 x_nw, float4 x_n, float4 x_ne, float4 x_w, float4 x_e, float4 x_sw, float4 x_s, float4 x_se) {
    return _G0*x + _G1*(x_n + x_e + x_w + x_s) + _G2*(x_nw + x_sw + x_ne + x_se);
}

__DEVICE__ bool reset() {
    return texture(iChannel3, to_float2(32.5f/256.0f, 0.5f) ).x > 0.5f;
}

__DEVICE__ float2 normz(float2 x) {
  return x == to_float2(0.0f, 0.0f) ? to_float2(0.0f, 0.0f) : normalize(x);
}

__DEVICE__ float4 advect(float2 ab, float2 vUv, float2 step) {
    
    float2 aUv = vUv - ab * ADVECT_DIST * step;
    
    float2 n  = to_float2(0.0f, step.y);
    float2 ne = to_float2(step.x, step.y);
    float2 e  = to_float2(step.x, 0.0f);
    float2 se = to_float2(step.x, -step.y);
    float2 s  = to_float2(0.0f, -step.y);
    float2 sw = to_float2(-step.x, -step.y);
    float2 w  = to_float2(-step.x, 0.0f);
    float2 nw = to_float2(-step.x, step.y);

    float4 u =    texture(iChannel0, fract(aUv));
    float4 u_n =  texture(iChannel0, fract(aUv+n));
    float4 u_e =  texture(iChannel0, fract(aUv+e));
    float4 u_s =  texture(iChannel0, fract(aUv+s));
    float4 u_w =  texture(iChannel0, fract(aUv+w));
    float4 u_nw = texture(iChannel0, fract(aUv+nw));
    float4 u_sw = texture(iChannel0, fract(aUv+sw));
    float4 u_ne = texture(iChannel0, fract(aUv+ne));
    float4 u_se = texture(iChannel0, fract(aUv+se));
    
    return gaussian(u, u_nw, u_n, u_ne, u_w, u_e, u_sw, u_s, u_se);
}

#define SQRT_3_OVER_2 0.86602540378
#define SQRT_3_OVER_2_INV 0.13397459621

__DEVICE__ float2 diagH(float2 x, float2 x_v, float2 x_h, float2 x_d) {
    return 0.5f * ((x + x_v) * SQRT_3_OVER_2_INV + (x_h + x_d) * SQRT_3_OVER_2);
}

__DEVICE__ float2 diagV(float2 x, float2 x_v, float2 x_h, float2 x_d) {
    return 0.5f * ((x + x_h) * SQRT_3_OVER_2_INV + (x_v + x_d) * SQRT_3_OVER_2);
}

__KERNEL__ void D2MuchDrugsFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{

    float2 vUv = fragCoord / iResolution;
    float2 texel = 1.0f / iResolution;
    
    float2 n  = to_float2(0.0f, 1.0f);
    float2 ne = to_float2(1.0f, 1.0f);
    float2 e  = to_float2(1.0f, 0.0f);
    float2 se = to_float2(1.0f, -1.0f);
    float2 s  = to_float2(0.0f, -1.0f);
    float2 sw = to_float2(-1.0f, -1.0f);
    float2 w  = to_float2(-1.0f, 0.0f);
    float2 nw = to_float2(-1.0f, 1.0f);

/*    float4 u =    texture(iChannel0, fract(vUv));
    float4 u_n =  texture(iChannel0, fract(vUv+texel*n));
    float4 u_e =  texture(iChannel0, fract(vUv+texel*e));
    float4 u_s =  texture(iChannel0, fract(vUv+texel*s));
    float4 u_w =  texture(iChannel0, fract(vUv+texel*w));
    float4 u_nw = texture(iChannel0, fract(vUv+texel*nw));
    float4 u_sw = texture(iChannel0, fract(vUv+texel*sw));
    float4 u_ne = texture(iChannel0, fract(vUv+texel*ne));
    float4 u_se = texture(iChannel0, fract(vUv+texel*se));
*/    
    float4 u =    texture(iChannel2, fract(vUv));
    float4 u_n =  texture(iChannel2, fract(vUv+texel*n));
    float4 u_e =  texture(iChannel2, fract(vUv+texel*e));
    float4 u_s =  texture(iChannel2, fract(vUv+texel*s));
    float4 u_w =  texture(iChannel2, fract(vUv+texel*w));
    float4 u_nw = texture(iChannel2, fract(vUv+texel*nw));
    float4 u_sw = texture(iChannel2, fract(vUv+texel*sw));
    float4 u_ne = texture(iChannel2, fract(vUv+texel*ne));
    float4 u_se = texture(iChannel2, fract(vUv+texel*se));
    
    
    const float vx = 0.5f;
    const float vy = SQRT_3_OVER_2;
    const float hx = SQRT_3_OVER_2;
    const float hy = 0.5f;

    float di_n  = nl(distance(swi2(u_n,x,y) + n, swi2(u,x,y)));
    float di_w  = nl(distance(swi2(u_w,x,y) + w, swi2(u,x,y)));
    float di_e  = nl(distance(swi2(u_e,x,y) + e, swi2(u,x,y)));
    float di_s  = nl(distance(swi2(u_s,x,y) + s, swi2(u,x,y)));
    
    float di_nne = nl(distance((diagV(swi2(u,x,y), swi2(u_n,x,y), swi2(u_e,x,y), swi2(u_ne,x,y)) + to_float2(+ vx, + vy)), swi2(u,x,y)));
    float di_ene = nl(distance((diagH(swi2(u,x,y), swi2(u_n,x,y), swi2(u_e,x,y), swi2(u_ne,x,y)) + to_float2(+ hx, + hy)), swi2(u,x,y)));
    float di_ese = nl(distance((diagH(swi2(u,x,y), swi2(u_s,x,y), swi2(u_e,x,y), swi2(u_se,x,y)) + to_float2(+ hx, - hy)), swi2(u,x,y)));
    float di_sse = nl(distance((diagV(swi2(u,x,y), swi2(u_s,x,y), swi2(u_e,x,y), swi2(u_se,x,y)) + to_float2(+ vx, - vy)), swi2(u,x,y)));    
    float di_ssw = nl(distance((diagV(swi2(u,x,y), swi2(u_s,x,y), swi2(u_w,x,y), swi2(u_sw,x,y)) + to_float2(- vx, - vy)), swi2(u,x,y)));
    float di_wsw = nl(distance((diagH(swi2(u,x,y), swi2(u_s,x,y), swi2(u_w,x,y), swi2(u_sw,x,y)) + to_float2(- hx, - hy)), swi2(u,x,y)));
    float di_wnw = nl(distance((diagH(swi2(u,x,y), swi2(u_n,x,y), swi2(u_w,x,y), swi2(u_nw,x,y)) + to_float2(- hx, + hy)), swi2(u,x,y)));
    float di_nnw = nl(distance((diagV(swi2(u,x,y), swi2(u_n,x,y), swi2(u_w,x,y), swi2(u_nw,x,y)) + to_float2(- vx, + vy)), swi2(u,x,y)));

    float2 xy_n  = swi2(u_n,x,y) + n - swi2(u,x,y);
    float2 xy_w  = swi2(u_w,x,y) + w - swi2(u,x,y);
    float2 xy_e  = swi2(u_e,x,y) + e - swi2(u,x,y);
    float2 xy_s  = swi2(u_s,x,y) + s - swi2(u,x,y);
    
    float2 xy_nne = (diagV(swi2(u,x,y), swi2(u_n,x,y), swi2(u_e,x,y), swi2(u_ne,x,y)) + to_float2(+ vx, + vy)) - swi2(u,x,y);
    float2 xy_ene = (diagH(swi2(u,x,y), swi2(u_n,x,y), swi2(u_e,x,y), swi2(u_ne,x,y)) + to_float2(+ hx, + hy)) - swi2(u,x,y);
    float2 xy_ese = (diagH(swi2(u,x,y), swi2(u_s,x,y), swi2(u_e,x,y), swi2(u_se,x,y)) + to_float2(+ hx, - hy)) - swi2(u,x,y);
    float2 xy_sse = (diagV(swi2(u,x,y), swi2(u_s,x,y), swi2(u_e,x,y), swi2(u_se,x,y)) + to_float2(+ vx, - vy)) - swi2(u,x,y);
    float2 xy_ssw = (diagV(swi2(u,x,y), swi2(u_s,x,y), swi2(u_w,x,y), swi2(u_sw,x,y)) + to_float2(- vx, - vy)) - swi2(u,x,y);
    float2 xy_wsw = (diagH(swi2(u,x,y), swi2(u_s,x,y), swi2(u_w,x,y), swi2(u_sw,x,y)) + to_float2(- hx, - hy)) - swi2(u,x,y);
    float2 xy_wnw = (diagH(swi2(u,x,y), swi2(u_n,x,y), swi2(u_w,x,y), swi2(u_nw,x,y)) + to_float2(- hx, + hy)) - swi2(u,x,y);
    float2 xy_nnw = (diagV(swi2(u,x,y), swi2(u_n,x,y), swi2(u_w,x,y), swi2(u_nw,x,y)) + to_float2(- vx, + vy)) - swi2(u,x,y);

    float2 ma = di_nne * xy_nne + di_ene * xy_ene + di_ese * xy_ese + di_sse * xy_sse + di_ssw * xy_ssw + di_wsw * xy_wsw + di_wnw * xy_wnw + di_nnw * xy_nnw + di_n * xy_n + di_w * xy_w + di_e * xy_e + di_s * xy_s;

    float4 u_blur = gaussian(u, u_nw, u_n, u_ne, u_w, u_e, u_sw, u_s, u_se);
    
    float4 au = advect(swi2(u,x,y), vUv, texel);
    float4 av = advect(swi2(u,z,w), vUv, texel);
    
    float2 dv = swi2(av,z,w) + TIMESTEP * ma;
    float2 du = swi2(au,x,y) + TIMESTEP * dv;

    if (iMouse.z > 0.0f) {
      float2 d = fragCoord - swi2(iMouse,x,y);
        float m = _expf(-length(d) / 50.0f);
        du += 0.2f * m * normz(d);
    }
    
    float2 init = texture(iChannel1, vUv, 4.0f).xy;
    // initialize with noise
    if((length(u) < 0.001f && length(init) > 0.001f) || reset()) {
        fragColor = 8.0f * (to_float4(-0.5f) + to_float4(swi2(init,x,y), swi2(init,x,y)));
    } else {
        du = length(du) > 1.0f ? normz(du) : du;
        fragColor = to_float4_aw(du, dv);
    }
    



  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer B' to iChannel0
// Connect Buffer B 'Previsualization: Buffer C' to iChannel2
// Connect Buffer B 'Previsualization: Buffer D' to iChannel3


/*
    A fluid-like dynamical system
  see: https://www.shadertoy.com/view/XddSRX
*/

__DEVICE__ float2 normz(float2 x) {
  return x == to_float2(0.0f, 0.0f) ? to_float2(0.0f, 0.0f) : normalize(x);
}

// reverse advection
__DEVICE__ float4 advect(float2 ab, float2 vUv, float2 step, float sc) {
    
    float2 aUv = vUv - ab * sc * step;
    
    const float _G0 = 0.25f; // center weight
    const float _G1 = 0.125f; // edge-neighbors
    const float _G2 = 0.0625f; // vertex-neighbors
    
    // 3x3 neighborhood coordinates
    float step_x = step.x;
    float step_y = step.y;
    float2 n  = to_float2(0.0f, step_y);
    float2 ne = to_float2(step_x, step_y);
    float2 e  = to_float2(step_x, 0.0f);
    float2 se = to_float2(step_x, -step_y);
    float2 s  = to_float2(0.0f, -step_y);
    float2 sw = to_float2(-step_x, -step_y);
    float2 w  = to_float2(-step_x, 0.0f);
    float2 nw = to_float2(-step_x, step_y);

    float4 uv =    texture(iChannel0, fract(aUv));
    float4 uv_n =  texture(iChannel0, fract(aUv+n));
    float4 uv_e =  texture(iChannel0, fract(aUv+e));
    float4 uv_s =  texture(iChannel0, fract(aUv+s));
    float4 uv_w =  texture(iChannel0, fract(aUv+w));
    float4 uv_nw = texture(iChannel0, fract(aUv+nw));
    float4 uv_sw = texture(iChannel0, fract(aUv+sw));
    float4 uv_ne = texture(iChannel0, fract(aUv+ne));
    float4 uv_se = texture(iChannel0, fract(aUv+se));
    
    return _G0*uv + _G1*(uv_n + uv_e + uv_w + uv_s) + _G2*(uv_nw + uv_sw + uv_ne + uv_se);
}

__KERNEL__ void D2MuchDrugsFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel2)
{

    const float _K0 = -20.0f/6.0f; // center weight
    const float _K1 = 4.0f/6.0f;   // edge-neighbors
    const float _K2 = 1.0f/6.0f;   // vertex-neighbors
    const float cs = -3.0f;  // curl scale
    const float ls = 3.0f;  // laplacian scale
    const float ps = 0.0f;  // laplacian of divergence scale
    const float ds = -12.0f; // divergence scale
    const float dp = -6.0f; // divergence update scale
    const float pl = 0.3f;   // divergence smoothing
    const float ad = 6.0f;   // advection distance scale
    const float pwr = 1.0f;  // power when deriving rotation angle from curl
    const float amp = 1.0f;  // self-amplification
    const float upd = 0.99f;  // update smoothing
    const float sq2 = 0.6f;  // diagonal weight
    
    float2 vUv = fragCoord / iResolution;
    float2 texel = 1.0f / iResolution;
    
    // 3x3 neighborhood coordinates
    float step_x = texel.x;
    float step_y = texel.y;
    float2 n  = to_float2(0.0f, step_y);
    float2 ne = to_float2(step_x, step_y);
    float2 e  = to_float2(step_x, 0.0f);
    float2 se = to_float2(step_x, -step_y);
    float2 s  = to_float2(0.0f, -step_y);
    float2 sw = to_float2(-step_x, -step_y);
    float2 w  = to_float2(-step_x, 0.0f);
    float2 nw = to_float2(-step_x, step_y);

 /*   float4 uv =    texture(iChannel0, fract(vUv));
    float4 uv_n =  texture(iChannel0, fract(vUv+n));
    float4 uv_e =  texture(iChannel0, fract(vUv+e));
    float4 uv_s =  texture(iChannel0, fract(vUv+s));
    float4 uv_w =  texture(iChannel0, fract(vUv+w));
    float4 uv_nw = texture(iChannel0, fract(vUv+nw));
    float4 uv_sw = texture(iChannel0, fract(vUv+sw));
    float4 uv_ne = texture(iChannel0, fract(vUv+ne));
    float4 uv_se = texture(iChannel0, fract(vUv+se));
  */  
     float4 uv =    texture(iChannel2, fract(vUv));
    float4 uv_n =  texture(iChannel2, fract(vUv+n));
    float4 uv_e =  texture(iChannel2, fract(vUv+e));
    float4 uv_s =  texture(iChannel2, fract(vUv+s));
    float4 uv_w =  texture(iChannel2, fract(vUv+w));
    float4 uv_nw = texture(iChannel2, fract(vUv+nw));
    float4 uv_sw = texture(iChannel2, fract(vUv+sw));
    float4 uv_ne = texture(iChannel2, fract(vUv+ne));
    float4 uv_se = texture(iChannel2, fract(vUv+se));
    
    // uv.x and uv.y are the x and y components, uv.z and uv.w accumulate divergence 

    // laplacian of all components
    float4 lapl  = _K0*uv + _K1*(uv_n + uv_e + uv_w + uv_s) + _K2*(uv_nw + uv_sw + uv_ne + uv_se);
    
    // calculate curl
    // vectors point clockwise about the center point
    float curl = uv_n.x - uv_s.x - uv_e.y + uv_w.y + sq2 * (uv_nw.x + uv_nw.y + uv_ne.x - uv_ne.y + uv_sw.y - uv_sw.x - uv_se.y - uv_se.x);
    
    // compute angle of rotation from curl
    float sc = cs * sign(curl) * _powf(_fabs(curl), pwr);
    
    // calculate divergence
    // vectors point inwards towards the center point
    float div  = uv_s.y - uv_n.y - uv_e.x + uv_w.x + sq2 * (uv_nw.x - uv_nw.y - uv_ne.x - uv_ne.y + uv_sw.x + uv_sw.y + uv_se.y - uv_se.x);
    
    float2 norm = normz(swi2(uv,x,y));
    
    float sdx = uv.z + dp * uv.x * div + pl * lapl.z;
    float sdy = uv.w + dp * uv.y * div + pl * lapl.w;

    float2 ab = advect(to_float2(uv.x, uv.y), vUv, texel, ad).xy;
    
    // temp values for the update rule
    float ta = amp * ab.x + ls * lapl.x + norm.x * ps * lapl.z + ds * sdx;
    float tb = amp * ab.y + ls * lapl.y + norm.y * ps * lapl.w + ds * sdy;

    // rotate
    float a = ta * _cosf(sc) - tb * _sinf(sc);
    float b = ta * _sinf(sc) + tb * _cosf(sc);
    
    float4 abd = upd * uv + (1.0f - upd) * to_float4(a,b,sdx,sdy);
    
    fragColor = to_float4(abd);
    
    swi2(abd,x,y) = clamp(length(swi2(abd,x,y)) > 1.0f ? normz(swi2(abd,x,y)) : swi2(abd,x,y), -1.0f, 1.0f);
    fragColor = to_float4(abd);
    


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel2
// Connect Buffer C 'Previsualization: Buffer C' to iChannel0
// Connect Buffer C 'Texture: Blue Noise' to iChannel1


// created by florian berger (flockaroo) - 2016
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.

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
#define KEY_I texture(keyTex,to_float2((105.5f-32.0f)/256.0f,(0.5f+0.0f)/3.0f)).x

const float ang = 2.0f*3.1415926535f/float(RotNum);
mat2 m = mat2(_cosf(ang),_sinf(ang),-_sinf(ang),_cosf(ang));
mat2 mh = mat2(_cosf(ang*0.5f),_sinf(ang*0.5f),-_sinf(ang*0.5f),_cosf(ang*0.5f));

__DEVICE__ float4 randS(float2 uv)
{
    return texture(iChannel1,uv*Res.xy/swi2(Res1,x,y))-to_float4_s(0.5f);
}

__DEVICE__ float getRot(float2 pos, float2 b)
{
    float2 p = b;
    float rot=0.0f;
    for(int i=0;i<RotNum;i++)
    {
        rot+=dot(texture(iChannel0,fract((pos+p)/swi2(Res,x,y))).xy-to_float2_s(0.5f),swi2(p,y,x)*to_float2(1,-1));
        p = m*p;
    }
    return rot/float(RotNum)/dot(b,b);
}

__KERNEL__ void D2MuchDrugsFuse__Buffer_C(float4 fragColor, float2 fragCoord, int iFrame, float3 iChannelResolution[], sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{

    float2 pos = fragCoord;
    float rnd = randS(to_float2(float(iFrame)/Res.x,0.5f/Res1.y)).x;
    
    float2 b = to_float2(_cosf(ang*rnd),_sinf(ang*rnd));
    float2 v=to_float2(0);
    float bbMax=0.7f*Res.y; bbMax*=bbMax;
    for(int l=0;l<20;l++)
    {
        if ( dot(b,b) > bbMax ) break;
        float2 p = b;
        for(int i=0;i<RotNum;i++)
        {
#ifdef SUPPORT_EVEN_ROTNUM
            v+=swi2(p,y,x)*getRot(pos+p,-mh*b);
#else
            // this is faster but works only for odd RotNum
            v+=swi2(p,y,x)*getRot(pos+p,b);
#endif
            p = m*p;
        }
        b*=2.0f;
    }
    
    fragColor=texture(iChannel2,fract((pos+v*to_float2(-1,1)*2.0f)/swi2(Res,x,y)));
    
    // add a little "motor" in the center
    float2 scr=(fragCoord/swi2(Res,x,y))*2.0f-to_float2_s(1.0f);
    swi2(fragColor,x,y) += (0.001f*swi2(scr,x,y) / (dot(scr,scr)/0.1f+0.3f));
    
    if(iFrame<=4 || KEY_I>0.5f) fragColor=texture(iChannel2,fragCoord/swi2(Res,x,y));


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Texture: London' to iChannel1
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


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


__DEVICE__ float3 contrast(float3 x) {
  return 1.0f / (1.0f + _expf(-SIGMOID_CONTRAST * (x - 0.5f)));    
}

__DEVICE__ float2 normz(float2 x) {
  return x == to_float2(0) ? to_float2(0) : normalize(x);
}

/*
  This function supplies a weight vector for each color channel.
  It's analogous to (but not a physically accurate model of)
  the response curves for each of the 3 cone types in the human eye.
  The three functions for red, green, and blue have the same integral
    over [0, 1], which is 1/3.
    Here are some other potential terms for the green weight that 
  integrate to 1/3:
        2.0f*(1-x)*x
        10.0f*((1-x)*x)^2
        46.667f*((1-i)*i)^3
        210.0f*((1-x)*x)^4
        924.0f*((1-x)*x)^5
    By the way, this series of coefficients is OEIS A004731 divided by 3,
    which is a pretty interesting series: https://oeis.org/A002457
*/
__DEVICE__ float3 sampleWeights(float i) {
  return to_float3_aw(i * i, 46.6666f*_powf((1.0f-i)*i,3.0f), (1.0f - i) * (1.0f - i));
}

__DEVICE__ float3 sampleDisp(float2 uv, float2 dispNorm, float disp) {
    float3 col = to_float3_aw(0);
    const float SD = 1.0f / float(SAMPLES);
    float wl = 0.0f;
    float3 denom = to_float3(0);
    for(int i = 0; i < SAMPLES; i++) {
        float3 sw = sampleWeights(wl);
        denom += sw;
        col += sw * texture(iChannel1, uv + dispNorm * disp * wl).xyz;
        wl  += SD;
    }
    
    // For a large enough number of samples,
    // the return below is equivalent to 3.0f * col * SD;
    return col / denom;
}

__KERNEL__ void D2MuchDrugsFuse__Buffer_D(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

    float2 texel = 1.0f / iResolution;
    float2 uv = fragCoord / iResolution;

    float2 n  = to_float2(0.0f, texel.y);
    float2 e  = to_float2(texel.x, 0.0f);
    float2 s  = to_float2(0.0f, -texel.y);
    float2 w  = to_float2(-texel.x, 0.0f);

    float2 d   = _tex2DVecN(iChannel0,uv.x,uv.y,15).CH;
    float2 d_n = texture(iChannel0, fract(uv+n)).CH;
    float2 d_e = texture(iChannel0, fract(uv+e)).CH;
    float2 d_s = texture(iChannel0, fract(uv+s)).CH;
    float2 d_w = texture(iChannel0, fract(uv+w)).CH; 

    // antialias our vector field by blurring
    float2 db = 0.4f * d + 0.15f * (d_n+d_e+d_s+d_w);

    float ld = length(db);
    float2 ln = normz(db);

  float3 col = sampleDisp(uv, ln, DISP_SCALE * ld);
    
    fragColor = to_float4_aw(contrast(col), 1.0f);



  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Abstract 1' to iChannel1
// Connect Image 'Previsualization: Buffer C' to iChannel3
// Connect Image 'Previsualization: Buffer D' to iChannel0
// Connect Image 'Previsualization: Buffer D' to iChannel2


// Fork of "Another Mess with Dispersion" by xenn. https://shadertoy.com/view/Ndc3zj
// 2021-08-28 16:03:28

// Fork of "A Handsome Mess with Dispersion" by xenn. https://shadertoy.com/view/fdc3zj
// 2021-08-28 10:49:54

// created by florian berger (flockaroo) - 2016
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.

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

__DEVICE__ float getVal(float2 uv)
{
    return length(_tex2DVecN(iChannel0,uv.x,uv.y,15).xyz);
}
    
__DEVICE__ float2 getGrad(float2 uv,float delta)
{
    float2 d=to_float2(delta,0);
    return to_float2(
        getVal(uv+swi2(d,x,y))-getVal(uv-swi2(d,x,y)),
        getVal(uv+swi2(d,y,x))-getVal(uv-swi2(d,y,x))
    )/delta;
}

__KERNEL__ void D2MuchDrugsFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel2)
{

  float2 uv = fragCoord / iResolution;
    float3 n = to_float3_aw(getGrad(uv,1.0f/iResolution.y),999.0f);
    //n *= n;
    n=normalize(n);
    fragColor=to_float4_aw(n,1);
    float3 light = normalize(to_float3(1,1,2));
    float diff=clamp(dot(n,light),0.5f,1.0f);
    float spec=clamp(dot(reflect(light,n),to_float3(0,0,-1)),0.0f,1.0f);
    spec=_powf(spec,36.0f)*2.5f;
    //spec=0.0f;
  fragColor = _tex2DVecN(iChannel2,uv.x,uv.y,15)*to_float4(diff)+to_float4(spec);


  SetFragmentShaderComputedColor(fragColor);
}