
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define pi _acosf(-1.0f)

#define sint(a) (asin(_sinf(a))*2.0f - 1.0f)
#define rot(a) to_mat2(_cosf(a),-_sinf(a),_sinf(a),_cosf(a))
#define pmod(p,d) mod_f(p - (d)*0.5f, (d)) - 0.5f*(d)

__DEVICE__ float r11(float i){ return fract(_sinf(i*12.126f)*12.6f);}

//#define xor(a,b,c) _fminf(max((a),-(b)), _fmaxf((b),-(a) - c)) 

__DEVICE__ float ss( float c, float power, float bias){
    c = clamp(c,-0.0f,1.0f);
    //c = smoothstep(0.0f,1.0f,c);
    
    c = _powf(c,1.0f + bias);
    
    float a = _powf( _fabs(c), power);
    float b = 1.0f-_powf( _fabs(c - 1.0f), power);
    
    return _mix(a,b,c);
}
__DEVICE__ float valueNoise(float i, float p){ return _mix(r11(_floor(i)),r11(_floor(i) + 1.0f), ss(fract(i), p,0.6f));}

__DEVICE__ float valueNoiseStepped(float i, float p, float steps){ return _mix(  _floor(r11(_floor(i))*steps)/steps, _floor(r11(_floor(i) + 1.0f)*steps)/steps, ss(fract(i), p,0.6f));}


// See: https://www.shadertoy.com/view/ls2Bz1
// Spectral Colour Schemes
// By Alan Zucconi
// Website: www.alanzucconi.com
// Twitter: @AlanZucconi

// Example of different spectral colour schemes
// to convert visible wavelengths of light (400-700 nm) to RGB colours.

// The function "spectral_zucconi6" provides the best approximation
// without including any branching.
// Its faster version, "spectral_zucconi", is advised for mobile applications.


// Read "Improving the Rainbow" for more information
// http://www.alanzucconi.com/?p=6703



__DEVICE__ float _saturatef (float _x)
{
    return _fminf(1.0f, _fmaxf(0.0f,_x));
}
__DEVICE__ float3 _saturatef (float3 _x)
{
    return _fminf(to_float3(1.0f,1.0f,1.0f), _fmaxf(to_float3(0.0f,0.0f,0.0f),_x));
}

// --- Spectral Zucconi --------------------------------------------
// By Alan Zucconi
// Based on GPU Gems: https://developer.nvidia.com/sites/all/modules/custom/gpugems/books/GPUGems/gpugems_ch08.html
// But with values optimised to match as close as possible the visible spectrum
// Fits this: https://commons.wikimedia.org/wiki/File:Linear_visible_spectrum.svg
// With weighter MSE (RGB weights: 0.3f, 0.59f, 0.11f)
__DEVICE__ float3 bump3y (float3 _x, float3 yoffset)
{
  float3 _y = to_float3(1.0f,1.0f,1.0f) - _x * _x;
  _y = _saturatef(_y-yoffset);
  return _y;
}

// --- Spectral Zucconi 6 --------------------------------------------

// Based on GPU Gems
// Optimised by Alan Zucconi
__DEVICE__ float3 spectral_zucconi6 (float _x)
{
  // w: [400, 700]
  // x: [0,   1]

  const float3 c1 = to_float3(3.54585104f, 2.93225262f, 2.41593945f);
  const float3 x1 = to_float3(0.69549072f, 0.49228336f, 0.27699880f);
  const float3 y1 = to_float3(0.02312639f, 0.15225084f, 0.52607955f);

  const float3 c2 = to_float3(3.90307140f, 3.21182957f, 3.96587128f);
  const float3 x2 = to_float3(0.11748627f, 0.86755042f, 0.66077860f);
  const float3 y2 = to_float3(0.84897130f, 0.88445281f, 0.73949448f);

  return
    bump3y(c1 * (_x - x1), y1) +
    bump3y(c2 * (_x - x2), y2) ;
}

// --- MATLAB Jet Colour Scheme ----------------------------------------
__DEVICE__ float3 spectral_jet(float w)
{
    // w: [400, 700]
  // x: [0,   1]
  float _x = _saturatef((w - 400.0f)/ 300.0f);
  float3 c;

  if (_x < 0.25f)
    c = to_float3(0.0f, 4.0f * _x, 1.0f);
  else if (_x < 0.5f)
    c = to_float3(0.0f, 1.0f, 1.0f + 4.0f * (0.25f - _x));
  else if (_x < 0.75f)
    c = to_float3(4.0f * (_x - 0.5f), 1.0f, 0.0f);
  else
    c = to_float3(1.0f, 1.0f + 4.0f * (0.75f - _x), 0.0f);

  // Clamp colour components in [0,1]
  return _saturatef(c);
}

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: RGBA Noise Small' to iChannel1
// Connect Buffer A 'Preset: Keyboard' to iChannel3
// Connect Buffer A 'Previsualization: Buffer C' to iChannel0


/*
    A fracturing dynamical system
  see: https://www.shadertoy.com/view/MsyXRW
*/

#define _G0 0.25f
#define _G1 0.125f
#define _G2 0.0625f
#define W0 -3.0f
#define W1 0.5f
#define TIMESTEP 0.1f
#define ADVECT_DIST 2.0f
#define DV 0.70710678f

// nonlinearity
__DEVICE__ float nl(float _x) {
    return 1.0f / (1.0f + _expf(W0 * (W1 * _x - 0.5f))); 
}

__DEVICE__ float4 gaussian(float4 _x, float4 x_nw, float4 x_n, float4 x_ne, float4 x_w, float4 x_e, float4 x_sw, float4 x_s, float4 x_se) {
    return _G0*_x + _G1*(x_n + x_e + x_w + x_s) + _G2*(x_nw + x_sw + x_ne + x_se);
}

//__DEVICE__ bool reset() {
//    return texture(iChannel3, to_float2(32.5f/256.0f, 0.5f) ).x > 0.5f;
//}

__DEVICE__ float2 normz(float2 _x) {
  //return _x == to_float2(0.0f, 0.0f) ? to_float2(0.0f, 0.0f) : normalize(_x);
  return (_x.x == 0.0f && _x.y == 0.0f)  ? to_float2(0.0f, 0.0f) : normalize(_x);
}

__DEVICE__ float4 advect(float2 ab, float2 vUv, float2 step, __TEXTURE2D__ iChannel0) {
    
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

#define SQRT_3_OVER_2 0.86602540378f
#define SQRT_3_OVER_2_INV 0.13397459621f

__DEVICE__ float2 diagH(float2 _x, float2 x_v, float2 x_h, float2 x_d) {
    return 0.5f * ((_x + x_v) * SQRT_3_OVER_2_INV + (x_h + x_d) * SQRT_3_OVER_2);
}

__DEVICE__ float2 diagV(float2 _x, float2 x_v, float2 x_h, float2 x_d) {
    return 0.5f * ((_x + x_h) * SQRT_3_OVER_2_INV + (x_v + x_d) * SQRT_3_OVER_2);
}

__KERNEL__ void ThroughTheTrippingGlassFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel3)
{
    CONNECT_CHECKBOX0(Reset, 0);
  
    fragCoord+=0.5f;

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

    float4 u =    texture(iChannel0, fract(vUv));
    float4 u_n =  texture(iChannel0, fract(vUv+texel*n));
    float4 u_e =  texture(iChannel0, fract(vUv+texel*e));
    float4 u_s =  texture(iChannel0, fract(vUv+texel*s));
    float4 u_w =  texture(iChannel0, fract(vUv+texel*w));
    float4 u_nw = texture(iChannel0, fract(vUv+texel*nw));
    float4 u_sw = texture(iChannel0, fract(vUv+texel*sw));
    float4 u_ne = texture(iChannel0, fract(vUv+texel*ne));
    float4 u_se = texture(iChannel0, fract(vUv+texel*se));
    
    const float vx = 0.5f;
    const float vy = SQRT_3_OVER_2;
    const float hx = SQRT_3_OVER_2;
    const float hy = 0.5f;

    float di_n  = nl(distance_f2(swi2(u_n,x,y) + n, swi2(u,x,y)));
    float di_w  = nl(distance_f2(swi2(u_w,x,y) + w, swi2(u,x,y)));
    float di_e  = nl(distance_f2(swi2(u_e,x,y) + e, swi2(u,x,y)));
    float di_s  = nl(distance_f2(swi2(u_s,x,y) + s, swi2(u,x,y)));
    
    float di_nne = nl(distance_f2((diagV(swi2(u,x,y), swi2(u_n,x,y), swi2(u_e,x,y), swi2(u_ne,x,y)) + to_float2(+ vx, + vy)), swi2(u,x,y)));
    float di_ene = nl(distance_f2((diagH(swi2(u,x,y), swi2(u_n,x,y), swi2(u_e,x,y), swi2(u_ne,x,y)) + to_float2(+ hx, + hy)), swi2(u,x,y)));
    float di_ese = nl(distance_f2((diagH(swi2(u,x,y), swi2(u_s,x,y), swi2(u_e,x,y), swi2(u_se,x,y)) + to_float2(+ hx, - hy)), swi2(u,x,y)));
    float di_sse = nl(distance_f2((diagV(swi2(u,x,y), swi2(u_s,x,y), swi2(u_e,x,y), swi2(u_se,x,y)) + to_float2(+ vx, - vy)), swi2(u,x,y)));    
    float di_ssw = nl(distance_f2((diagV(swi2(u,x,y), swi2(u_s,x,y), swi2(u_w,x,y), swi2(u_sw,x,y)) + to_float2(- vx, - vy)), swi2(u,x,y)));
    float di_wsw = nl(distance_f2((diagH(swi2(u,x,y), swi2(u_s,x,y), swi2(u_w,x,y), swi2(u_sw,x,y)) + to_float2(- hx, - hy)), swi2(u,x,y)));
    float di_wnw = nl(distance_f2((diagH(swi2(u,x,y), swi2(u_n,x,y), swi2(u_w,x,y), swi2(u_nw,x,y)) + to_float2(- hx, + hy)), swi2(u,x,y)));
    float di_nnw = nl(distance_f2((diagV(swi2(u,x,y), swi2(u_n,x,y), swi2(u_w,x,y), swi2(u_nw,x,y)) + to_float2(- vx, + vy)), swi2(u,x,y)));

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
    
    float4 au = advect(swi2(u,x,y), vUv, texel, iChannel0);
    float4 av = advect(swi2(u,z,w), vUv, texel, iChannel0);
    
    float2 dv = swi2(av,z,w) + TIMESTEP * ma;
    float2 du = swi2(au,x,y) + TIMESTEP * dv;

    if (iMouse.z > 0.0f) {
        float2 d = fragCoord - swi2(iMouse,x,y);
        float m = _expf(-length(d) / 50.0f);
        du += 0.2f * m * normz(d);
    }
    
    float2 init = swi2(texture(iChannel1, vUv),x,y);
    // initialize with noise
    if((length(u) < 0.001f && length(init) > 0.001f) || Reset) {
        fragColor = 8.0f * (to_float4_s(-0.5f) + to_float4_f2f2(swi2(init,x,y), swi2(init,x,y)));
    } else {
        du = length(du) > 1.0f ? normz(du) : du;
        fragColor = to_float4_f2f2(du, dv);
    }
    
    if (iFrame < 1 || Reset) fragColor = to_float4_s(0.0f);
    
  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer D' to iChannel0


/*
    A fluid-like dynamical system
  see: https://www.shadertoy.com/view/XddSRX
*/

//__DEVICE__ float2 normz(float2 _x) {
//  return _x == to_float2(0.0f, 0.0f) ? to_float2(0.0f, 0.0f) : normalize(_x);
//}

// reverse advection
__DEVICE__ float4 advectB(float2 ab, float2 vUv, float2 step, float sc, __TEXTURE2D__ iChannel0) {
    
    float2 aUv = vUv - ab * sc * step;

    const float G0 = 0.25f; // center weight
    const float G1 = 0.125f; // edge-neighbors
    const float G2 = 0.0625f; // vertex-neighbors
    
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
    
    return G0*uv + G1*(uv_n + uv_e + uv_w + uv_s) + G2*(uv_nw + uv_sw + uv_ne + uv_se);
}

__KERNEL__ void ThroughTheTrippingGlassFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    fragCoord+=0.5f;
float BBBBBBBBBBBBBBBBBBBBBBBBBBBBBB;
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

    float4 uv =    texture(iChannel0, fract(vUv));
    float4 uv_n =  texture(iChannel0, fract(vUv+n));
    float4 uv_e =  texture(iChannel0, fract(vUv+e));
    float4 uv_s =  texture(iChannel0, fract(vUv+s));
    float4 uv_w =  texture(iChannel0, fract(vUv+w));
    float4 uv_nw = texture(iChannel0, fract(vUv+nw));
    float4 uv_sw = texture(iChannel0, fract(vUv+sw));
    float4 uv_ne = texture(iChannel0, fract(vUv+ne));
    float4 uv_se = texture(iChannel0, fract(vUv+se));
    
    // uv.x and uv.y are the x and y components, uv.z and uv.w accumulate divergence 

    // laplacian of all components
    float4 lapl  = _K0*uv + _K1*(uv_n + uv_e + uv_w + uv_s) + _K2*(uv_nw + uv_sw + uv_ne + uv_se);
    
    // calculate curl
    // vectors point clockwise about the center point
    float curl = uv_n.x - uv_s.x - uv_e.y + uv_w.y + sq2 * (uv_nw.x + uv_nw.y + uv_ne.x - uv_ne.y + uv_sw.y - uv_sw.x - uv_se.y - uv_se.x);
    
    // compute angle of rotation from curl
    float sc = cs * sign_f(curl) * _powf(_fabs(curl), pwr);
    
    // calculate divergence
    // vectors point inwards towards the center point
    float div  = uv_s.y - uv_n.y - uv_e.x + uv_w.x + sq2 * (uv_nw.x - uv_nw.y - uv_ne.x - uv_ne.y + uv_sw.x + uv_sw.y + uv_se.y - uv_se.x);
    
    float2 norm = normz(swi2(uv,x,y));
    
    float sdx = uv.z + dp * uv.x * div + pl * lapl.z;
    float sdy = uv.w + dp * uv.y * div + pl * lapl.w;

    float2 ab = swi2(advectB(to_float2(uv.x, uv.y), vUv, texel, ad, iChannel0),x,y);
    
    // temp values for the update rule
    float ta = amp * ab.x + ls * lapl.x + norm.x * ps * lapl.z + ds * sdx;
    float tb = amp * ab.y + ls * lapl.y + norm.y * ps * lapl.w + ds * sdy;

    // rotate
    float a = ta * _cosf(sc) - tb * _sinf(sc);
    float b = ta * _sinf(sc) + tb * _cosf(sc);
    
    float4 abd = upd * uv + (1.0f - upd) * to_float4(a,b,sdx,sdy);
    
    fragColor = abd;
    
    swi2S(abd,x,y, clamp(length(swi2(abd,x,y)) > 1.0f ? normz(swi2(abd,x,y)) : swi2(abd,x,y), -1.0f, 1.0f));
    fragColor = abd;
    
  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Texture: London' to iChannel2
// Connect Buffer C 'Previsualization: Buffer C' to iChannel0
// Connect Buffer C 'Previsualization: Buffer D' to iChannel3
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

#define RotNum 9
//#define SUPPORT_EVEN_ROTNUM

#define Res  iResolution
#define Res1 iResolution

//#define keyTex iChannel3
//#define KEY_I texture(keyTex,to_float2((105.5f-32.0f)/256.0f,(0.5f+0.0f)/3.0f)).x



__DEVICE__ float4 randS(float2 uv, __TEXTURE2D__ iChannel1)
{
    return texture(iChannel1,uv)-to_float4_s(0.5f);
}

__DEVICE__ float getRot(float2 pos, float2 b, __TEXTURE2D__ iChannel0, mat2 m, float2 R)
{
    //const float ang = 2.0f*3.1415926535f/(float)(RotNum);
    //mat2 m = to_mat2(_cosf(ang),_sinf(ang),-_sinf(ang),_cosf(ang));
    //mat2 mh = to_mat2(_cosf(ang*0.5f),_sinf(ang*0.5f),-_sinf(ang*0.5f),_cosf(ang*0.5f));
  
    float2 p = b;
    float rot=0.0f;
    for(int i=0;i<RotNum;i++)
    {
        rot+=dot(swi2(texture(iChannel0,fract_f2((pos+p)/swi2(Res,x,y))),x,y)-to_float2_s(0.5f),swi2(p,y,x)*to_float2(1,-1));
        p = mul_mat2_f2(m,p);
    }
    return rot/(float)(RotNum)/dot(b,b);
}

__KERNEL__ void ThroughTheTrippingGlassFuse__Buffer_C(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, float3 iChannelResolution[], sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
    CONNECT_CHECKBOX0(Reset, 0);

    fragCoord+=0.5f;
    
    const float ang = 2.0f*3.1415926535f/(float)(RotNum);
    mat2 m = to_mat2(_cosf(ang),_sinf(ang),-_sinf(ang),_cosf(ang));
    mat2 mh = to_mat2(_cosf(ang*0.5f),_sinf(ang*0.5f),-_sinf(ang*0.5f),_cosf(ang*0.5f));
    

    float2 pos = fragCoord;
    float rnd = randS(to_float2((float)(iFrame)/Res.x,0.5f/Res1.y), iChannel1).x;
    
    float2 b = to_float2(_cosf(ang*rnd),_sinf(ang*rnd));
    float2 v=to_float2_s(0);
    float bbMax=0.7f*Res.y; bbMax*=bbMax;
    for(int l=0;l<20;l++)
    {
        if ( dot(b,b) > bbMax ) break;
        float2 p = b;
        for(int i=0;i<RotNum;i++)
        {
#ifdef SUPPORT_EVEN_ROTNUM
            v+=swi2(p,y,x)*getRot(pos+p,-mul_mat2_f2(mh,b), iChannel0, m, R);
#else
            // this is faster but works only for odd RotNum
            v+=swi2(p,y,x)*getRot(pos+p,b, iChannel0, m, R);
#endif
            p = mul_mat2_f2(m,p);
        }
        b*=2.0f;
    }
    
  float2 uv = fragCoord/iResolution; // Normalized pixel coordinates (from 0 to 1)

  float4 col = texture(iChannel0,fract((pos+v*to_float2(-1,1)*2.0f)/swi2(Res,x,y)));
  float4 col2 = texture(iChannel3,fract((pos+v*to_float2(-1,1)*2.0f)/swi2(Res,x,y)));
  float4 blend = _mix(col,col2,(0.5f) );
  
  fragColor=blend;
  //  fragColor=texture(iChannel0,fract((pos+v*to_float2(-1,1)*2.0f)/swi2(Res,x,y)));
    
  // add a little "motor" in the center
  float2 scr=(fragCoord/swi2(Res,x,y))*2.0f-to_float2_s(1.0f);
  swi2S(fragColor,x,y, swi2(fragColor,x,y) + (0.001f*swi2(scr,x,y) / (dot(scr,scr)/0.1f+0.3f)));
    
  //   if(iFrame<=4 || KEY_I>0.5f) fragColor=texture(iChannel2,fragCoord/swi2(Res,x,y));

  if (iFrame < 1 || Reset) fragColor = to_float4_s(0.0f);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0
// Connect Buffer D 'Previsualization: Buffer C' to iChannel2
// Connect Buffer D 'Previsualization: Buffer D' to iChannel1
// Connect Buffer D 'Previsualization: Buffer D' to iChannel3


/*
  Normally it would be provided by texture parameters, but on Mac/iOS the texture REPEAT
  seems to work only for power-of-2 texture sizes.
 */
__DEVICE__ float4 repeatedTexture(__TEXTURE2D__ channel, in float2 uv) {
    return texture(channel, mod_f(uv, 1.0f));
}

__DEVICE__ float drawBlob(
    in float2 st,
    in float2 center,
    in float radius,
    in float edgeSmoothing,
    in float iBlobEdgeSmoothing
) {
    float dist = length((st - center) / radius);
    return dist * smoothstep(1.0f, 1.0f - iBlobEdgeSmoothing, dist);
}


__KERNEL__ void ThroughTheTrippingGlassFuse__Buffer_D(float4 fragColor, float2 fragCoord, float iTime, int iFrame, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
    CONNECT_CHECKBOX0(Reset, 0);

    fragCoord+=0.5f;
    
    // In the original shader-web-background these values are provided as uniforms
    // feel free to play with them and if you will find something prettier than
    // the equilibrium I established, please send it back to me :)
    const float2  iFeedbackZoomCenter     = to_float2(0.0f, 0.0f);
    const float iFeedbackZoomRate         = 0.001f;
    const float iFeedbackFadeRate         = 0.999f;
    const float iFeedbackColorShiftZoom   = 0.015f;
    const float iFeedbackColorShiftImpact = -0.02f;
    const float2  iDrawCenter             = to_float2(0.0f, 0.0f);
    const float iDrawIntensity            = 0.05f;
    const float iBlobEdgeSmoothing        = 0.02f;
    const float iBlob1Radius              = 0.69f;
    const float iBlob1PowFactor           = 30.0f;
    const float iBlob1ColorPulseSpeed     = 0.024f;
    const float iBlob2Radius              = 0.7f;
    const float iBlob2PowFactor           = 20.0f;
    const float iBlob2ColorPulseSpeed     = 0.15f;
    const float iBlob2ColorPulseShift     = 0.0f;
    const float iColorShiftOfRadius       = 0.5f;
    const float iFeedbackMouseShiftFactor = 0.03f;
    
     
    // in shader-web-background provided as uniforms: start
    float iMinDimension = _fminf(iResolution.x, iResolution.y);
    float2 iScreenRatioHalf =
            (iResolution.x >= iResolution.y)
            ? to_float2(iResolution.y / iResolution.x * 0.5f, 0.5f)
            : to_float2(0.5f, iResolution.x / iResolution.y);
    float3 iBlob1Color = spectral_zucconi6(
           mod_f(iTime * iBlob1ColorPulseSpeed, 1.0f)
           );
    
    float3 iBlob2Color = spectral_zucconi6 (
           mod_f(iTime * iBlob2ColorPulseSpeed + iBlob2ColorPulseShift, 1.0f)
           );
    float2 iFeedbackShiftVector =
           (iMouse.x > 0.0f && iMouse.y > 0.0f)
           ? (swi2(iMouse,x,y) * 2.0f - iResolution) / iMinDimension * iFeedbackMouseShiftFactor
           : to_float2_s(0);
    // in shader-web-background provided as uniforms: end
            
    
    float2 uv = fragCoord / iResolution;
    float2 st = (fragCoord * 2.0f - iResolution) / iMinDimension;

    float2  drawDelta = st - iDrawCenter;
    float drawAngle = _atan2f(drawDelta.x, drawDelta.y);
    float drawDist = length(drawDelta);

    float3 feedbk = swi3(repeatedTexture(iChannel1, uv - st),x,y,z);
    float3 colorShift = swi3(repeatedTexture(
                        iChannel0,
                        uv - st * iFeedbackColorShiftZoom * iScreenRatioHalf),x,y,z);

    float2 stShift = to_float2_s(0);
    stShift += iFeedbackZoomRate * (st - iFeedbackZoomCenter);
    stShift += (swi2(feedbk,z,y)/swi2(colorShift,x,z) - 0.5f) * iFeedbackColorShiftImpact  * (0.1f*_fabs(_cosf(iTime*0.1231f))) ;
    stShift += iFeedbackShiftVector ;
    stShift *= iScreenRatioHalf;

    float3 prevColor = swi3(repeatedTexture(iChannel2, uv - stShift),x,y,z);
    prevColor *= iFeedbackFadeRate;

    float3 drawColor = to_float3_s(0);
    float3 extraColor = swi3(repeatedTexture(iChannel3, uv - stShift),x,y,z);
    extraColor *= iFeedbackFadeRate;

    float radius = 1.0f + (colorShift.x + colorShift.y + colorShift.z) * iColorShiftOfRadius;
    drawColor +=   _powf( drawBlob(st, iDrawCenter, radius * (iBlob1Radius ) , iBlobEdgeSmoothing, iBlobEdgeSmoothing),
                          iBlob1PowFactor
                          ) * iBlob1Color;
    drawColor += _powf( drawBlob(st, iDrawCenter, radius * (iBlob2Radius ), iBlobEdgeSmoothing, iBlobEdgeSmoothing),
                        iBlob2PowFactor
                          ) * iBlob2Color;

    float3 color = to_float3_s(0);
    drawColor *= iDrawIntensity;
    prevColor *= iFeedbackFadeRate;
    color += prevColor;
    color += drawColor;
    color *  (extraColor * extraColor);

//    color -   clamp(color, 0.0f, 2.5f );   //?????????????
    //   fragColor = to_float4_aw(color, 1.0f);
 
    float4 col = _tex2DVecN(iChannel2,uv.x,uv.y,15);
    float4 col2 = _tex2DVecN(iChannel3,uv.x,uv.y,15);
    float4 blend = _mix(col,col2,(0.125f) );

//    color -   clamp(color, 0.0f, 2.5f ); //??????????????
    fragColor = _mix((to_float4_aw(color, 1.0f)),blend, 0.25f);

    if (iFrame < 1 || Reset) fragColor = to_float4_s(0.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: London' to iChannel2
// Connect Image 'Previsualization: Buffer D' to iChannel0
// Connect Image 'Texture: Blue Noise' to iChannel1


// Fork of "Another Mess with Dispersion" by xenn. https://shadertoy.com/view/Ndc3zj
// 2021-09-01 22:13:25

// Mashup Fork of "Displacement with Dispersion" by cornusammonis. https://shadertoy.com/view/4ldGDB
// 2021-08-28 10:34:29
// & this https://www.shadertoy.com/view/MsGSRd by flockeroo

// displacement amount
#define DISP_SCALE 2.0f

// chromatic dispersion samples
#define SAMPLES 64

// contrast
#define SIGMOID_CONTRAST 12.0f

// channels to use for displacement, either xy or zw
#define CH xy


__DEVICE__ float3 contrast(float3 _x) {
  return 1.0f / (1.0f + exp_f3(-SIGMOID_CONTRAST * (_x - 0.5f)));    
}

//__DEVICE__ float2 normz(float2 _x) {
//  return (_x.x == 0.0f && _x.y == 0.0f) ? to_float2_s(0) : normalize(_x);
//}

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
  return to_float3(i * i, 46.6666f*_powf((1.0f-i)*i,3.0f), (1.0f - i) * (1.0f - i));
}

__DEVICE__ float3 sampleDisp(float2 uv, float2 dispNorm, float disp, __TEXTURE2D__ iChannel1) {
    float3 col = to_float3_s(0);
    const float SD = 1.0f / (float)(SAMPLES);
    float wl = 0.0f;
    float3 denom = to_float3_s(0);
    for(int i = 0; i < SAMPLES; i++) {
        float3 sw = sampleWeights(wl);
        denom += sw;
        col += sw * swi3(texture(iChannel1, uv + dispNorm * disp * wl),x,y,z);
        wl  += SD;
    }
    
    // For a large enough number of samples,
    // the return below is equivalent to 3.0f * col * SD;
    return col / denom;
}

__KERNEL__ void ThroughTheTrippingGlassFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

    fragCoord+=0.5f;
    
    float2 texel = 1.0f / iResolution;
    float2 uv = fragCoord / iResolution;

    float2 n  = to_float2(0.0f, texel.y);
    float2 e  = to_float2(texel.x, 0.0f);
    float2 s  = to_float2(0.0f, -texel.y);
    float2 w  = to_float2(-texel.x, 0.0f);

    float2 d   = swi2(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y);
    float2 d_n = swi2(texture(iChannel0, fract(uv+n)),x,y);
    float2 d_e = swi2(texture(iChannel0, fract(uv+e)),x,y);
    float2 d_s = swi2(texture(iChannel0, fract(uv+s)),x,y);
    float2 d_w = swi2(texture(iChannel0, fract(uv+w)),x,y); 

    // antialias our vector field by blurring
    float2 db = 0.4f * d + 0.15f * (d_n+d_e+d_s+d_w);

    float ld = length(db);
    float2 ln = normz(db);

    //float3 col = sampleDisp(uv, ln, DISP_SCALE * ld, iChannel1);
    float3 col = sampleDisp(uv, ln, DISP_SCALE * ld, iChannel2);  //Umsortiert
    
    fragColor = to_float4_aw(contrast(col), 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}