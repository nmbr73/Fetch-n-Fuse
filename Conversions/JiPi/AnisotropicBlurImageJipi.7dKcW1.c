
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: RGBA Noise Medium' to iChannel1
// Connect Buffer A 'Preset: Keyboard' to iChannel3
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0


#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

/*
    Generates a vector field using a dynamical system. 
  To see it in action on its own see this shadertoy:
    https://www.shadertoy.com/view/XddSRX
*/

//__DEVICE__ bool reset() {
//    return texture(iChannel3, to_float2(32.5f/256.0f, 0.5f) ).x > 0.5f;
//}

__DEVICE__ float2 normz(float2 x) {
  //return x == to_float2(0.0f, 0.0f) ? to_float2(0.0f, 0.0f) : normalize(x);
  return (x.x == 0.0f && x.y == 0.0f) ? to_float2(0.0f, 0.0f) : normalize(x);
}

// reverse advection
__DEVICE__ float3 advect(float2 ab, float2 vUv, float2 step, float sc, __TEXTURE2D__ iChannel0) {
    
    float2 aUv = vUv - ab * sc * step;
float zzzzzzzzzzzzzzzzzzzz;    
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

    float3 uv =    swi3(texture(iChannel0, fract(aUv)),x,y,z);
    float3 uv_n =  swi3(texture(iChannel0, fract(aUv+n)),x,y,z);
    float3 uv_e =  swi3(texture(iChannel0, fract(aUv+e)),x,y,z);
    float3 uv_s =  swi3(texture(iChannel0, fract(aUv+s)),x,y,z);
    float3 uv_w =  swi3(texture(iChannel0, fract(aUv+w)),x,y,z);
    float3 uv_nw = swi3(texture(iChannel0, fract(aUv+nw)),x,y,z);
    float3 uv_sw = swi3(texture(iChannel0, fract(aUv+sw)),x,y,z);
    float3 uv_ne = swi3(texture(iChannel0, fract(aUv+ne)),x,y,z);
    float3 uv_se = swi3(texture(iChannel0, fract(aUv+se)),x,y,z);
    
    return _G0*uv + _G1*(uv_n + uv_e + uv_w + uv_s) + _G2*(uv_nw + uv_sw + uv_ne + uv_se);
}

__KERNEL__ void AnisotropicBlurImageJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel3)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_CHECKBOX1(OrgPar, 0);
  
    CONNECT_SLIDER0(MouseSize, -1.0f, 50.0f, 20.0f); 
    //CONNECT_SLIDER1(ampMul, -1.0f, 1.0f, 0.00051f);
    
    CONNECT_SLIDER2(_K0, -10.0f, 10.0f, -3.3333f);
    CONNECT_SLIDER3(_K1, -1.0f, 1.0f, 0.66667f);
    CONNECT_SLIDER4(_K2, -1.0f, 1.0f, 0.16667f);
    
    CONNECT_SLIDER5(cs, -1.0f, 1.0f, -0.6f);
    CONNECT_SLIDER6(ls, -1.0f, 1.0f, 0.05f);
    
    CONNECT_SLIDER7(ps, -1.0f, 1.0f, -0.8f);
    CONNECT_SLIDER8(ds, -1.0f, 1.0f, -0.05f);
    
    CONNECT_SLIDER9(dp, -1.0f, 1.0f, -0.04f);
    CONNECT_SLIDER10(pl, -1.0f, 1.0f, 0.3f);
    
    CONNECT_SLIDER11(ad, -1.0f, 10.0f, 6.0f);
    CONNECT_SLIDER12(pwr, -2.0f, 2.0f, 1.0f);
    CONNECT_SLIDER13(amp, -2.0f, 2.0f, 1.0f);
    CONNECT_SLIDER14(upd, -2.0f, 2.0f, 0.8f);
    CONNECT_SLIDER15(sq2, -2.0f, 2.0f, 0.6f);
    
    fragCoord+=0.5f;
 
    
    if(OrgPar)
    {
       _K0 = -20.0f/6.0f; // center weight
       _K1 = 4.0f/6.0f;   // edge-neighbors
       _K2 = 1.0f/6.0f;   // vertex-neighbors
       cs = -0.6f;  // curl scale
       ls = 0.05f;  // laplacian scale
       ps = -0.8f;  // laplacian of divergence scale
       ds = -0.05f; // divergence scale
       dp = -0.04f; // divergence update scale
       pl = 0.3f;   // divergence smoothing
       ad = 6.0f;   // advection distance scale
       pwr = 1.0f;  // power when deriving rotation angle from curl
       amp = 1.0f;  // self-amplification
       upd = 0.8f;  // update smoothing
       sq2 = 0.6f;  // diagonal weight
    }


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

    float3 uv =    swi3(texture(iChannel0, fract(vUv)),x,y,z);
    float3 uv_n =  swi3(texture(iChannel0, fract(vUv+n)),x,y,z);
    float3 uv_e =  swi3(texture(iChannel0, fract(vUv+e)),x,y,z);
    float3 uv_s =  swi3(texture(iChannel0, fract(vUv+s)),x,y,z);
    float3 uv_w =  swi3(texture(iChannel0, fract(vUv+w)),x,y,z);
    float3 uv_nw = swi3(texture(iChannel0, fract(vUv+nw)),x,y,z);
    float3 uv_sw = swi3(texture(iChannel0, fract(vUv+sw)),x,y,z);
    float3 uv_ne = swi3(texture(iChannel0, fract(vUv+ne)),x,y,z);
    float3 uv_se = swi3(texture(iChannel0, fract(vUv+se)),x,y,z);
float AAAAAAAAAAAAAAAAA;    
    // uv.x and uv.y are the x and y components, uv.z is divergence 

    // laplacian of all components
    float3 lapl  = _K0*uv + _K1*(uv_n + uv_e + uv_w + uv_s) + _K2*(uv_nw + uv_sw + uv_ne + uv_se);
    float sp = ps * lapl.z;
    
    // calculate curl
    // vectors point clockwise about the center point
    float curl = uv_n.x - uv_s.x - uv_e.y + uv_w.y + sq2 * (uv_nw.x + uv_nw.y + uv_ne.x - uv_ne.y + uv_sw.y - uv_sw.x - uv_se.y - uv_se.x);
    
    // compute angle of rotation from curl
    float sc = cs * sign_f(curl) * _powf(_fabs(curl), pwr);
    
    // calculate divergence
    // vectors point inwards towards the center point
    float div  = uv_s.y - uv_n.y - uv_e.x + uv_w.x + sq2 * (uv_nw.x - uv_nw.y - uv_ne.x - uv_ne.y + uv_sw.x + uv_sw.y + uv_se.y - uv_se.x);
    float sd = uv.z + dp * div + pl * lapl.z;

    float2 norm = normz(swi2(uv,x,y));
    
    float3 ab = advect(to_float2(uv.x, uv.y), vUv, texel, ad, iChannel0);
    
    // temp values for the update rule
    float ta = amp * ab.x + ls * lapl.x + norm.x * sp + uv.x * ds * sd;
    float tb = amp * ab.y + ls * lapl.y + norm.y * sp + uv.y * ds * sd;

    // rotate
    float a = ta * _cosf(sc) - tb * _sinf(sc);
    float b = ta * _sinf(sc) + tb * _cosf(sc);
    
    float3 abd = upd * uv + (1.0f - upd) * to_float3(a,b,sd);
    
    if (iMouse.z > 0.0f) {
      float2 d = fragCoord - swi2(iMouse,x,y);
      float m = _expf(-length(d) / MouseSize);//20.0f);
      swi2S(abd,x,y, swi2(abd,x,y) + m * normz(d));
    }
    
    float3 init = swi3(texture(iChannel1, fragCoord / iResolution),x,y,z);
    // initialize with noise
    //if((uv == to_float3_s(0.0f) && init != to_float3_s(0.0f)) || reset()) {
      if((uv.x == 0.0f && uv.y == 0.0f && uv.z == 0.0f && (init.x != 0.0f || init.y != 0.0f || init.z != 0.0f)) || Reset) {
        fragColor = to_float4_aw(init - 0.5f, 0.0f);
    } else {
        abd.z = clamp(abd.z, -1.0f, 1.0f);
        swi2S(abd,x,y, clamp(length(swi2(abd,x,y)) > 1.0f ? normz(swi2(abd,x,y)) : swi2(abd,x,y), -1.0f, 1.0f));
        fragColor = to_float4_aw(abd, 0.0f);
    }
    
  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Preset: Keyboard' to iChannel3
// Connect Buffer B 'Texture: Abstract 3' to iChannel0
// Connect Buffer B 'Previsualization: Buffer A' to iChannel2
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1


/*
  Blurs each pixel with its neighbors according to the underlying
  vector field in Buffer A.
*/



#ifdef XXX
__DEVICE__ bool reset() {
    return texture(iChannel3, to_float2(32.5f/256.0f, 0.5f) ).x > 0.5f;
}

__DEVICE__ float2 normz(float2 x) {
  return x == to_float2(0.0f, 0.0f) ? to_float2(0.0f, 0.0f) : normalize(x);
}
#endif

__KERNEL__ void AnisotropicBlurImageJipiFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
    CONNECT_CHECKBOX0(Reset, 0);
    
    CONNECT_CHECKBOX2(GAMMA_CORRECT, 1);
    //#define GAMMA_CORRECT    // use gamma-corrected blending
    CONNECT_SLIDER16(Gamma, -1.0f, 10.0f, 2.2f);
    float3 GAMMA = to_float3_s(Gamma);
    //#define GAMMA to_float3_s(2.2f)     // gamma

    CONNECT_CHECKBOX3(NORMALIZE_AB, 1);
    //#define NORMALIZE_AB    // normalize the vector value
    CONNECT_SLIDER17(BLUR_RATIO, -1.0f, 10.0f, 0.2f);
    //#define BLUR_RATIO 0.2f    // ratio of the original pixel value to the blurred value
    CONNECT_SLIDER18(SHARPNESS, -1.0f, 10.0f, 1.0f);
    //#define SHARPNESS 1.0f    // sharpness of the blur kernel, 0.0f gives a uniform distribution
    CONNECT_SLIDER19(VECTOR_SHARPEN, -1.0f, 30.0f, 12.0f);
    //#define VECTOR_SHARPEN 12.0f // sharpens the vector field
    
    
    fragCoord+=0.5f;
    
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
    
    float2 ab =    swi2(texture(iChannel2, fract(vUv)),x,y);
    float2 ab_n =  swi2(texture(iChannel2, fract(vUv+n)),x,y);
    float2 ab_e =  swi2(texture(iChannel2, fract(vUv+e)),x,y);
    float2 ab_s =  swi2(texture(iChannel2, fract(vUv+s)),x,y);
    float2 ab_w =  swi2(texture(iChannel2, fract(vUv+w)),x,y);
    float2 ab_nw = swi2(texture(iChannel2, fract(vUv+nw)),x,y);
    float2 ab_sw = swi2(texture(iChannel2, fract(vUv+sw)),x,y);
    float2 ab_ne = swi2(texture(iChannel2, fract(vUv+ne)),x,y);
    float2 ab_se = swi2(texture(iChannel2, fract(vUv+se)),x,y);
    
    const float _K0 = -20.0f/6.0f; // center weight
    const float _K1 = 4.0f/6.0f;   // edge-neighbors
    const float _K2 = 1.0f/6.0f;   // vertex-neighbors
    
    // laplacian
    float2 lapl  = _K0*ab + _K1*(ab_n + ab_e + ab_w + ab_s) + _K2*(ab_nw + ab_sw + ab_ne + ab_se);
float BBBBBBBBBBBBBBBBBB;    
    ab += -VECTOR_SHARPEN * lapl;
    
    //#ifdef NORMALIZE_AB
    if(NORMALIZE_AB)
      ab = normz(ab);
    //#endif
    
    
    float3 im,im_n,im_e,im_s,im_w,im_nw,im_sw,im_ne,im_se;
    
    //#ifdef GAMMA_CORRECT
    if(GAMMA_CORRECT)
    {
         im =    pow_f3(swi3(texture(iChannel1, fract(vUv)),x,y,z), GAMMA);
         im_n =  pow_f3(swi3(texture(iChannel1, fract(vUv+n)),x,y,z), GAMMA);
         im_e =  pow_f3(swi3(texture(iChannel1, fract(vUv+e)),x,y,z), GAMMA);
         im_s =  pow_f3(swi3(texture(iChannel1, fract(vUv+s)),x,y,z), GAMMA);
         im_w =  pow_f3(swi3(texture(iChannel1, fract(vUv+w)),x,y,z), GAMMA);
         im_nw = pow_f3(swi3(texture(iChannel1, fract(vUv+nw)),x,y,z), GAMMA);
         im_sw = pow_f3(swi3(texture(iChannel1, fract(vUv+sw)),x,y,z), GAMMA);
         im_ne = pow_f3(swi3(texture(iChannel1, fract(vUv+ne)),x,y,z), GAMMA);
         im_se = pow_f3(swi3(texture(iChannel1, fract(vUv+se)),x,y,z), GAMMA);
    }
    else //#else
    {  
         im =    swi3(texture(iChannel1, fract(vUv)),x,y,z);
         im_n =  swi3(texture(iChannel1, fract(vUv+n)),x,y,z);
         im_e =  swi3(texture(iChannel1, fract(vUv+e)),x,y,z);
         im_s =  swi3(texture(iChannel1, fract(vUv+s)),x,y,z);
         im_w =  swi3(texture(iChannel1, fract(vUv+w)),x,y,z);
         im_nw = swi3(texture(iChannel1, fract(vUv+nw)),x,y,z);
         im_sw = swi3(texture(iChannel1, fract(vUv+sw)),x,y,z);
         im_ne = swi3(texture(iChannel1, fract(vUv+ne)),x,y,z);
         im_se = swi3(texture(iChannel1, fract(vUv+se)),x,y,z);
    }
    //#endif

    // a gaussian centered around the point at 'ab'
    #define e(x,y) _expf(-SHARPNESS * dot(to_float2(x,y) - ab, to_float2(x,y) - ab))
    
    float D_c =  e( 0.0f, 0.0f);
    float D_e =  e( 1.0f, 0.0f);
    float D_w =  e(-1.0f, 0.0f);
    float D_n =  e( 0.0f, 1.0f);
    float D_s =  e( 0.0f,-1.0f);
    float D_ne = e( 1.0f, 1.0f);
    float D_nw = e(-1.0f, 1.0f);
    float D_se = e( 1.0f,-1.0f);
    float D_sw = e(-1.0f,-1.0f);
    
    // normalize the blur kernel
    float dn = D_c + D_e + D_w + D_n + D_s + D_ne + D_nw + D_se + D_sw;

    float3 blur_im = (D_c*im 
        + im_n*D_n + im_ne*D_ne 
        + im_e*D_e + im_se*D_se 
        + im_s*D_s + im_sw*D_sw 
        + im_w*D_w + im_nw*D_nw) / dn;
    
    //#ifdef GAMMA_CORRECT
    if(GAMMA_CORRECT)
    {
      blur_im = pow_f3(blur_im, 1.0f / GAMMA);
      im = pow_f3(im, 1.0f / GAMMA);
    }
    //#endif

    // initialize with image
    float4 init = texture(iChannel0, fragCoord / iResolution);
    //if((im == to_float3_s(0.0f) && init != to_float4_s(0.0f)) || reset()) {
      if((im.x == 0.0f && im.y == 0.0f && im.z == 0.0f && (init.x != 0.0f || init.y != 0.0f || init.z != 0.0f)) || Reset) {
        fragColor = init;
    } else {
        fragColor = to_float4_aw(clamp(BLUR_RATIO * im + (1.0f - BLUR_RATIO) * blur_im, 0.0f, 1.0f), 0.0f);
    }

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer B' to iChannel0


//#define SIGMOID_CONTRAST 12.0f

__DEVICE__ float4 _contrast(float4 _x, float s) {
  return to_float4_s(1.0f) / (to_float4_s(1.0f) + exp_f4(-s * (_x - 0.5f)));    
}

__KERNEL__ void AnisotropicBlurImageJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    CONNECT_SLIDER1(Alpha, 0.0f, 1.0f, 1.0f );
  
  CONNECT_SCREW0(SIGMOID_CONTRAST, -1.0f, 30.0f, 12.0f);
  
    fragCoord+=0.5f;   

    float2 texel = 1.0f / iResolution;
    float2 uv = fragCoord / iResolution;
    fragColor = _contrast(_tex2DVecN(iChannel0,uv.x,uv.y,15), SIGMOID_CONTRAST);

fragColor = _tex2DVecN(iChannel0,uv.x,uv.y,15);

    fragColor.w = Alpha;

  SetFragmentShaderComputedColor(fragColor);
}