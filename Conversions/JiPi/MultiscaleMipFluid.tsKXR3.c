


BufferB verwendet textureLod - keine KOnvertierung möglich !!!!!!!!!!!!!!!!!!!!!!!!




// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution

/*
  Number of scales to use in computation of each value. Lowering these will change the 
    result drastically, also note that the heightmap is used for rendering, so changing 
    POISSON_SCALES will alter the appearance of lighting/shadows. Weighting functions
    for each scale are defined below.
*/
#define TURBULENCE_SCALES 11
#define VORTICITY_SCALES 11
#define POISSON_SCALES 11



// If defined, recalculate the advection offset at every substep. Otherwise, subdivide the offset.
// Leaving this undefined is much cheaper for large ADVECTION_STEPS but yields worse results; this
// can be improved by defining the BLUR_* options below.
#define RECALCULATE_OFFSET
// Number of advection substeps to use. Useful to increase this for large ADVECTION_SCALE. Must be >= 1
#define ADVECTION_STEPS 3
// Advection distance multiplier.
#define ADVECTION_SCALE 40.0f
// Scales the effect of turbulence on advection.
#define ADVECTION_TURBULENCE 1.0f
// Scales the effect of turbulence on velocity. Use small values.
#define VELOCITY_TURBULENCE 0.0000f
// Scales the effect of vorticity confinement on velocity.
#define VELOCITY_CONFINEMENT 0.01f
// Scales diffusion.
#define VELOCITY_LAPLACIAN 0.02f
// Scales the effect of vorticity confinement on advection.
#define ADVECTION_CONFINEMENT 0.6f
// Scales the effect of divergence on advection.
#define ADVECTION_DIVERGENCE  0.0f
// Scales the effect of velocity on advection.
#define ADVECTION_VELOCITY -0.05f
// Amount of divergence minimization. Too much will cause instability.
#define DIVERGENCE_MINIMIZATION 0.1f
// If 0.0f, compute the gradient at (0,0). If 1.0f, compute the gradient at the advection distance.
#define DIVERGENCE_LOOKAHEAD 1.0f
// If 0.0f, compute the laplacian at (0,0). If 1.0f, compute the laplacian at the advection distance.
#define LAPLACIAN_LOOKAHEAD 1.0f
// Scales damping force.
#define DAMPING 0.0001f
// Overall velocity multiplier
#define VELOCITY_SCALE 1.0f
// Mixes the previous velocity with the new velocity (range 0..1).
#define UPDATE_SMOOTHING 0.0f



// These control the (an)isotropy of the various kernels
#define TURB_ISOTROPY 0.9f  // [0..2.0]
#define CURL_ISOTROPY 0.6f  // >= 0
#define CONF_ISOTROPY 0.25f // [0..0.5]
#define POIS_ISOTROPY 0.16f // [0..0.5]



// If defined, multiply curl by vorticity, then accumulate. If undefined, accumulate, then multiply.
#define PREMULTIPLY_CURL



// These apply a gaussian blur to the various values used in the velocity/advection update. More expensive when defined.
//#define BLUR_TURBULENCE
//#define BLUR_CONFINEMENT
//#define BLUR_VELOCITY



// These define weighting functions applied at each of the scales, i=0 being the finest detail.
//#define TURB_W_FUNCTION 1.0f/float(i+1)
#define TURB_W_FUNCTION 1.0f
//#define TURB_W_FUNCTION float(i+1)

//#define CURL_W_FUNCTION 1.0f/float(1 << i)
#define CURL_W_FUNCTION 1.0f/(float)(i+1)
//#define CURL_W_FUNCTION 1.0

//#define CONF_W_FUNCTION 1.0f/float(i+1)
#define CONF_W_FUNCTION 1.0f
//#define CONF_W_FUNCTION float(i+1)
//#define CONF_W_FUNCTION float(1 << i)

//#define POIS_W_FUNCTION 1.0
#define POIS_W_FUNCTION 1.0f/(float)(i+1)
//#define POIS_W_FUNCTION 1.0f/float(1 << i)
//#define POIS_W_FUNCTION float(i+1)
//#define POIS_W_FUNCTION float(1 << i)



// These can help reduce mipmap artifacting, especially when POIS_W_FUNCTION emphasizes large scales.
//#define USE_PRESSURE_ADVECTION
// Scales pressure advection distance.
#define PRESSURE_ADVECTION 0.0002f // higher values more likely to cause blowup if laplacian > 0.0
// Pressure diffusion.
#define PRESSURE_LAPLACIAN 0.1f // [0..0.3] higher values more likely to cause blowup
// Mixes the previous pressure with the new pressure.
#define PRESSURE_UPDATE_SMOOTHING 0.0f // [0..1]



// Scales mouse interaction effect
#define MOUSE_AMP 0.05f
// Scales mouse interaction radius
#define MOUSE_RADIUS 0.001f



// If defined, "pump" velocity in the center of the screen. If undefined, alternate pumping from the sides of the screen.
//#define CENTER_PUMP
// Amplitude and cycle time for the "pump" at the center of the screen.
#define PUMP_SCALE 0.001f
#define PUMP_CYCLE 0.2f


__DEVICE__ float4 normz(float4 _x) {
  //return swi3(x,x,y,z) == to_float3(0) ? to_float4(0,0,0,x.w) : to_float4(normalize(swi3(x,x,y,z)),0);
  if (_x.x==0.0f&&_x.y==0.0f&&_x.z==0.0f) return to_float4(0,0,0,x.w);
  return to_float4(normalize(swi3(x,x,y,z)),0);
}

__DEVICE__ float3 normz(float3 _x) {
  return (_x.x == 0.0f&&_x.y == 0.0f&&_x.z == 0.0f) ? to_float3_s(0) : normalize(_x);
}

__DEVICE__ float2 normz(float2 _x) {
  return (_x.x == 0.0f&&_x.y == 0.0f) ? to_float2_s(0) : normalize(_x);
}


// Only used for rendering, but useful helpers
__DEVICE__ float softmax(float a, float b, float k) {
  return _logf(exp(k*a)+_expf(k*b))/k;    
}

__DEVICE__ float softmin(float a, float b, float k) {
  return -_logf(exp(-k*a)+_expf(-k*b))/k;    
}

__DEVICE__ float4 softmax(float4 a, float4 b, float k) {
  return log_f4(exp_f4(k*a)+exp_f4(k*b))/k;    
}

__DEVICE__ float4 softmin(float4 a, float4 b, float k) {
  return -log_f4(exp_f4(-k*a)+exp_f4(-k*b))/k;    
}

__DEVICE__ float softclamp(float a, float b, float x, float k) {
  return (softmin(b,softmax(a,x,k),k) + softmax(a,softmin(b,x,k),k)) / 2.0f;    
}

__DEVICE__ float4 softclamp(float4 a, float4 b, float4 x, float k) {
  return (softmin(b,softmax(a,x,k),k) + softmax(a,softmin(b,x,k),k)) / 2.0f;    
}

__DEVICE__ float4 softclamp(float a, float b, float4 x, float k) {
  return (softmin(to_float4_s(b),softmax(to_float4_s(a),x,k),k) + softmax(to_float4_s(a),softmin(to_float4_s(b),x,k),k)) / 2.0f;    
}




// GGX from Noby's Goo shader https://www.shadertoy.com/view/lllBDM

// MIT License: https://opensource.org/licenses/MIT
__DEVICE__ float G1V(float dnv, float k){
    return 1.0f/(dnv*(1.0f-k)+k);
}

__DEVICE__ float ggx(float3 n, float3 v, float3 l, float rough, float f0){
    float alpha = rough*rough;
    float3 h = normalize(v+l);
    float dnl = clamp(dot(n,l), 0.0f, 1.0f);
    float dnv = clamp(dot(n,v), 0.0f, 1.0f);
    float dnh = clamp(dot(n,h), 0.0f, 1.0f);
    float dlh = clamp(dot(l,h), 0.0f, 1.0f);
    float f, d, vis;
    float asqr = alpha*alpha;
    const float pi = 3.14159f;
    float den = dnh*dnh*(asqr-1.0f)+1.0f;
    d = asqr/(pi * den * den);
    dlh = _powf(1.0f-dlh, 5.0f);
    f = f0 + (1.0f-f0)*dlh;
    float k = alpha/1.0f;
    vis = G1V(dnl, k)*G1V(dnv, k);
    float spec = dnl * d * f * vis;
    return spec;
}
// End Noby's GGX


// Modified from Shane's Bumped Sinusoidal Warp shadertoy here:
// https://www.shadertoy.com/view/4l2XWK
__DEVICE__ float3 light(float2 uv, float BUMP, float SRC_DIST, float2 dxy, float iTime, inout float3 avd) {
    float3 sp = to_float3_aw(uv-0.5f, 0);
    float3 light = to_float3(_cosf(iTime/2.0f)*0.5f, _sinf(iTime/2.0f)*0.5f, -SRC_DIST);
    float3 ld = light - sp;
    float lDist = _fmaxf(length(ld), 0.001f);
    ld /= lDist;
    avd = reflect(normalize(to_float3_aw(BUMP*dxy, -1.0f)), to_float3(0,1,0));
    return ld;
}
// End Shane's bumpmapping section


// The MIT License
// Copyright © 2017 Inigo Quilez
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
__DEVICE__ float hash1( uint n ) 
{
    // integer hash copied from Hugo Elias
    n = (n << 13U) ^ n;
    n = n * (n * n * 15731U + 789221U) + 1376312589U;
    return (float)( n & make_uint3(0x7fffffffU))/(float)(0x7fffffff);
}

__DEVICE__ float3 hash3( uint n ) 
{
    // integer hash copied from Hugo Elias
    n = (n << 13U) ^ n;
float zzzzzzzzzzzzzzzzzzzzzzzzzzzzz;    
    n = n * (n * n * 15731U + 789221U) + 1376312589U;
    uint3 k = n * make_uint3(n,n*16807U,n*48271U);
    return to_float3( k & make_uint3(0x7fffffffU))/(float)(0x7fffffff);
}

// a simple modification for this shader to get a float4
__DEVICE__ float4 rand4( float2 fragCoord, float2 iResolution, int iFrame ) {
    uint2 p = make_uint2(fragCoord);
    uint2 r = make_uint2(iResolution);
    uint c = p.x + r.x*p.y + r.x*r.y*(uint)(iFrame);
  return to_float4_aw(hash3(c),hash1(c + 75132895U));   
}

// End IQ's integer hash
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel3
// Connect Buffer A 'Previsualization: Buffer C' to iChannel2
// Connect Buffer A 'Previsualization: Buffer D' to iChannel1


#define TURBULENCE_SAMPLER iChannel3
#define CONFINEMENT_SAMPLER iChannel2
#define POISSON_SAMPLER iChannel1
#define VELOCITY_SAMPLER iChannel0

#define V(d) swi2(texture(TURBULENCE_SAMPLER, fract_f2(uv+(d+0.0f))),x,y)

__DEVICE__ float2 gaussian_turbulence(float2 uv, float2 R) {
    float2 texel = 1.0f/iResolution;
    float4 t = to_float4(texel.x, texel.y, -texel.y, 0);

    float2 d =    V( swi2(t,w,w)); float2 d_n =  V( swi2(t,w,y)); float2 d_e =  V( swi2(t,x,w));
    float2 d_s =  V( swi2(t,w,z)); float2 d_w =  V(-swi2(t,x,w)); float2 d_nw = V(-swi2(t,x,z));
    float2 d_sw = V(-swi2(t,x,y)); float2 d_ne = V( swi2(t,x,y)); float2 d_se = V( swi2(t,x,z));
    
    return 0.25f * d + 0.125f * (d_e + d_w + d_n + d_s) + 0.0625f * (d_ne + d_nw + d_se + d_sw);
}

#define C(d) swi2(texture(CONFINEMENT_SAMPLER, fract_f2(uv+(d+0.0f))),x,y)

__DEVICE__ float2 gaussian_confinement(float2 uv, float2 R) {
    float2 texel = 1.0f/iResolution;
    float4 t = to_float4(texel.x, texel.y, -texel.y, 0);

    float2 d =    C( swi2(t,w,w)); float2 d_n =  C( swi2(t,w,y)); float2 d_e =  C( swi2(t,x,w));
    float2 d_s =  C( swi2(t,w,z)); float2 d_w =  C(-swi2(t,x,w)); float2 d_nw = C(-swi2(t,x,z));
    float2 d_sw = C(-swi2(t,x,y)); float2 d_ne = C( swi2(t,x,y)); float2 d_se = C( swi2(t,x,z));
    
    return 0.25f * d + 0.125f * (d_e + d_w + d_n + d_s) + 0.0625f * (d_ne + d_nw + d_se + d_sw);
}

#define D(d) texture(POISSON_SAMPLER, fract_f2(uv+d)).x

__DEVICE__ float2 diff(float2 uv, float2 R) {
    float2 texel = 1.0f/iResolution;
    float4 t = to_float4(texel.x, texel.y, -texel.y, 0);

    float d =    D( swi2(t,w,w)); float d_n =  D( swi2(t,w,y)); float d_e =  D( swi2(t,x,w));
    float d_s =  D( swi2(t,w,z)); float d_w =  D(-swi2(t,x,w)); float d_nw = D(-swi2(t,x,z));
    float d_sw = D(-swi2(t,x,y)); float d_ne = D( swi2(t,x,y)); float d_se = D( swi2(t,x,z));
    
    return to_float2(
        0.5f * (d_e - d_w) + 0.25f * (d_ne - d_nw + d_se - d_sw),
        0.5f * (d_n - d_s) + 0.25f * (d_ne + d_nw - d_se - d_sw)
    );
}

#define N(d) texture(VELOCITY_SAMPLER, fract(uv+(d+0.0f)))

__DEVICE__ float4 gaussian_velocity(float2 uv, float2 R) {
    float2 texel = 1.0f/iResolution;
    float4 t = to_float4(texel.x, texel.y, -texel.y, 0);

    float4 d =    N( swi2(t,w,w)); float4 d_n =  N( swi2(t,w,y)); float4 d_e =  N( swi2(t,x,w));
    float4 d_s =  N( swi2(t,w,z)); float4 d_w =  N(-swi2(t,x,w)); float4 d_nw = N(-swi2(t,x,z));
    float4 d_sw = N(-swi2(t,x,y)); float4 d_ne = N( swi2(t,x,y)); float4 d_se = N( swi2(t,x,z));
    
    return 0.25f * d + 0.125f * (d_e + d_w + d_n + d_s) + 0.0625f * (d_ne + d_nw + d_se + d_sw);
}

__DEVICE__ float2 vector_laplacian(float2 uv, float2 R) {
    const float _K0 = -20.0f/6.0f, _K1 = 4.0f/6.0f, _K2 = 1.0f/6.0f;
    float2 texel = 1.0f/iResolution;
    float4 t = to_float4(texel.x, texel.y, -texel.y, 0);

    float4 d =    N( swi2(t,w,w)); float4 d_n =  N( swi2(t,w,y)); float4 d_e =  N( swi2(t,x,w));
    float4 d_s =  N( swi2(t,w,z)); float4 d_w =  N(-swi2(t,x,w)); float4 d_nw = N(-swi2(t,x,z));
    float4 d_sw = N(-swi2(t,x,y)); float4 d_ne = N( swi2(t,x,y)); float4 d_se = N( swi2(t,x,z));
    
    return swi2((_K0 * d + _K1 * (d_e + d_w + d_n + d_s) + _K2 * (d_ne + d_nw + d_se + d_sw)),x,y);
}

    


__KERNEL__ void MultiscaleMipFluidFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{

    fragCoord+=0.5f;

    float2 uv = fragCoord/iResolution;
    float2 tx = 1.0f/iResolution;

    
    float2 turb, confine, div, delta_v, offset, lapl = to_float2_s(0);
    float4 vel, adv = to_float4_s(0);
    float4 init = N(0.0f);

    #ifdef RECALCULATE_OFFSET
        for (int i = 0; i < ADVECTION_STEPS; i++) {
            #ifdef BLUR_TURBULENCE
            turb = gaussian_turbulence(uv + tx * offset,R);
            #else
            turb = V(tx * offset);
            #endif

            #ifdef BLUR_CONFINEMENT
            confine = gaussian_confinement(uv + tx * offset,R);
            #else
            confine = C(tx * offset);
            #endif

            #ifdef BLUR_VELOCITY
            vel = gaussian_velocity(uv + tx * offset,R);
            #else
            vel = N(tx * offset);
            #endif

            // an alternative, but seems to give less smooth results:
            // offset += (1.0f / float(ADVECTION_STEPS)) * ...
            offset = ((float)(i+1) / (float)(ADVECTION_STEPS)) * - ADVECTION_SCALE * (ADVECTION_VELOCITY * swi2(vel,x,y) + ADVECTION_TURBULENCE * turb - ADVECTION_CONFINEMENT * confine + ADVECTION_DIVERGENCE * div);

            div = diff(uv + tx * DIVERGENCE_LOOKAHEAD * offset,R);

            lapl = vector_laplacian(uv + tx * LAPLACIAN_LOOKAHEAD * offset);

            adv += N(tx * offset);

            delta_v += VELOCITY_LAPLACIAN * lapl + VELOCITY_TURBULENCE * turb + VELOCITY_CONFINEMENT * confine - DAMPING * swi2(vel,x,y) - DIVERGENCE_MINIMIZATION * div;
        }
        adv /= (float)(ADVECTION_STEPS);
        delta_v /= (float)(ADVECTION_STEPS);
    #else
        #ifdef BLUR_TURBULENCE
        turb = gaussian_turbulence(uv,R);
        #else
        turb = V(0.0f);
        #endif

        #ifdef BLUR_CONFINEMENT
        confine = gaussian_confinement(uv,R);
        #else
        confine = C(0.0f);
        #endif

        #ifdef BLUR_VELOCITY
        vel = gaussian_velocity(uv,R);
        #else
        vel = N(0.0f);
        #endif
    
      offset = - ADVECTION_SCALE * (ADVECTION_VELOCITY * swi2(vel,x,y) + ADVECTION_TURBULENCE * turb - ADVECTION_CONFINEMENT * confine + ADVECTION_DIVERGENCE * div);
        
      div = diff(uv + tx * DIVERGENCE_LOOKAHEAD * offset,R);
        
      lapl = vector_laplacian(uv + tx * LAPLACIAN_LOOKAHEAD * offset);
      
      delta_v += VELOCITY_LAPLACIAN * lapl + VELOCITY_TURBULENCE * turb + VELOCITY_CONFINEMENT * confine - DAMPING * swi2(vel,x,y) - DIVERGENCE_MINIMIZATION * div;
    
        for (int i = 0; i < ADVECTION_STEPS; i++) {
            adv += N(((float)(i+1) / (float)(ADVECTION_STEPS)) * tx * offset);   
        }   
        adv /= (float)(ADVECTION_STEPS);
    #endif
    

    
    // define a pump, either at the center of the screen,
    // or alternating at the sides of the screen.
    float2 pq = 2.0f*(uv*2.0f-1.0f) * to_float2(1,tx.x/tx.y);
    #ifdef CENTER_PUMP
      float2 pump = _sinf(PUMP_CYCLE*iTime)*PUMP_SCALE*swi2(pq,x,y) / (dot(pq,pq)+0.01f);
    #else
      float2 pump = to_float2_s(0);
      #define AMP 15.0f
      #define SCL -50.0f
        float uvy0 = _expf(SCL*_powf(pq.y,2.0f));
        float uvx0 = _expf(SCL*_powf(uv.x,2.0f));
        pump += -AMP*to_float2(_fmaxf(0.0f,_cosf(PUMP_CYCLE*iTime))*PUMP_SCALE*uvx0*uvy0,0);
    
        float uvy1 = _expf(SCL*_powf(pq.y,2.0f));
        float uvx1 = _expf(SCL*_powf(1.0f - uv.x,2.0f));
        pump += AMP*to_float2(_fmaxf(0.0f,_cosf(PUMP_CYCLE*iTime + 3.1416f))*PUMP_SCALE*uvx1*uvy1,0);

        float uvy2 = _expf(SCL*_powf(pq.x,2.0f));
        float uvx2 = _expf(SCL*_powf(uv.y,2.0f));
        pump += -AMP*to_float2(0,_fmaxf(0.0f,_sinf(PUMP_CYCLE*iTime))*PUMP_SCALE*uvx2*uvy2);
    
        float uvy3 = _expf(SCL*_powf(pq.x,2.0f));
        float uvx3 = _expf(SCL*_powf(1.0f - uv.y,2.0f));
        pump += AMP*to_float2(0,_fmaxf(0.0f,_sinf(PUMP_CYCLE*iTime + 3.1416f))*PUMP_SCALE*uvx3*uvy3);
    #endif
    
    fragColor = _mix(adv + to_float4_aw(VELOCITY_SCALE * (delta_v + pump), offset), init, UPDATE_SMOOTHING);
    
    if (iMouse.z > 0.0f) {
        float4 mouseUV = iMouse / swi4(iResolution,x,y,x,y);
        float2 delta = normz(swi2(mouseUV,z,w) - swi2(mouseUV,x,y));
        float2 md = (swi2(mouseUV,x,y) - uv) * to_float2(1.0f,tx.x/tx.y);
        float amp = _expf(_fmaxf(-12.0f,-dot(md,md)/MOUSE_RADIUS));
        swi2S(fragColor,x,y, swi2(fragColor,x,y) + VELOCITY_SCALE * MOUSE_AMP * clamp(amp * delta,-1.0f,1.0f));
    }
    
    // Adding a very small amount of noise on init fixes subtle numerical precision blowup problems
    if (iFrame==0) fragColor=1e-6*rand4(fragCoord, iResolution, iFrame);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


#define TURB_CH xy
#define TURB_SAMPLER iChannel0
#define DEGREE TURBULENCE_SCALES

#define DB(d) swi2(texture(TURB_SAMPLER, fract_f2(uv+d)),x,y)

__DEVICE__ void tex(float2 uv, inout mat3 mx, inout mat3 my, int degree) {
    float2 texel = 1.0f/iResolution;
    float stride = float(1 << degree);
    float mip = float(degree);
    float4 t = stride * to_float4(texel, -texel.y, 0);

    float2 d =    D( swi2(t,w,w)); float2 d_n =  D( swi2(t,w,y)); float2 d_e =  D( swi2(t,x,w));
    float2 d_s =  D( swi2(t,w,z)); float2 d_w =  D(-swi2(t,x,w)); float2 d_nw = D(-swi2(t,x,z));
    float2 d_sw = D(-swi2(t,x,y)); float2 d_ne = D( swi2(t,x,y)); float2 d_se = D( swi2(t,x,z));
    
    mx =  mat3(d_nw.x, d_n.x, d_ne.x,
               d_w.x,  d.x,   d_e.x,
               d_sw.x, d_s.x, d_se.x);
    
    my =  mat3(d_nw.y, d_n.y, d_ne.y,
               d_w.y,  d.y,   d_e.y,
               d_sw.y, d_s.y, d_se.y);
}

__DEVICE__ float reduce(mat3 a, mat3 b) {
    mat3 p = matrixCompMult(a, b);
    return p[0][0] + p[0][1] + p[0][2] +
           p[1][0] + p[1][1] + p[1][2] +
           p[2][0] + p[2][1] + p[2][2];
}

__DEVICE__ void turbulence(float2 fragCoord, inout float2 turb, inout float curl)
{
  float2 uv = fragCoord / iResolution;
     
    mat3 turb_xx = (2.0f - TURB_ISOTROPY) * mat3(
        0.125f,  0.25f, 0.125f,
       -0.25f,  -0.5f, -0.25f,
        0.125f,  0.25f, 0.125
    );

    mat3 turb_yy = (2.0f - TURB_ISOTROPY) * mat3(
        0.125f, -0.25f, 0.125f,
        0.25f,  -0.5f,  0.25f,
        0.125f, -0.25f, 0.125
    );

    mat3 turb_xy = TURB_ISOTROPY * mat3(
       0.25f, 0.0f, -0.25f,  
       0.0f,  0.0f,  0.0f,
      -0.25f, 0.0f,  0.25
    );
    
    const float norm = 8.8f / (4.0f + 8.0f * CURL_ISOTROPY);  // 8.8f takes the isotropy as 0.6
    float c0 = CURL_ISOTROPY;
    
    mat3 curl_x = mat3(
        c0,   1.0f,  c0,
        0.0f,  0.0f,  0.0f,
       -c0,  -1.0f, -c0
    );

    mat3 curl_y = mat3(
        c0, 0.0f, -c0,
       1.0f, 0.0f, -1.0f,
        c0, 0.0f, -c0
    );
    
    mat3 mx, my;
    float2 v = to_float2(0);
    float turb_wc, curl_wc = 0.0f;
    curl = 0.0f;
    for (int i = 0; i < DEGREE; i++) {
        tex(uv, mx, my, i);
        float turb_w = TURB_W_FUNCTION;
        float curl_w = CURL_W_FUNCTION;
      v += turb_w * to_float2(reduce(turb_xx, mx) + reduce(turb_xy, my), reduce(turb_yy, my) + reduce(turb_xy, mx));
        curl += curl_w * (reduce(curl_x, mx) + reduce(curl_y, my));
        turb_wc += turb_w;
        curl_wc += curl_w;
    }

    turb = float(DEGREE) * v / turb_wc;
    curl = norm * curl / curl_wc;
}

__KERNEL__ void MultiscaleMipFluidFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel0)
{

    float2 turb;
    float curl;
    turbulence(fragCoord, turb, curl);
    fragColor = to_float4(turb,0,curl);
    // Adding a very small amount of noise on init fixes subtle numerical precision blowup problems
    if (iFrame==0) fragColor=1e-6*rand4(fragCoord, iResolution, iFrame);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


#define CURL_CH w
#define CURL_SAMPLER iChannel0
#define DEGREE VORTICITY_SCALES

#define CURL(d) texture(CURL_SAMPLER, fract(uv+(d+0.0f))).CURL_CH
#define D(d) _fabs(texture(CURL_SAMPLER, fract(uv+d)).CURL_CH)

__DEVICE__ void tex(float2 uv, inout mat3 mc, inout float curl, int degree) {
    float2 texel = 1.0f/iResolution;
    float stride = float(1 << degree);
    float mip = float(degree);
    float4 t = stride * to_float4(texel, -texel.y, 0);

    float d =    D( swi2(t,w,w)); float d_n =  D( swi2(t,w,y)); float d_e =  D( swi2(t,x,w));
    float d_s =  D( swi2(t,w,z)); float d_w =  D(-swi2(t,x,w)); float d_nw = D(-swi2(t,x,z));
    float d_sw = D(-swi2(t,x,y)); float d_ne = D( swi2(t,x,y)); float d_se = D( swi2(t,x,z));
    
    mc =  mat3(d_nw, d_n, d_ne,
               d_w,  d,   d_e,
               d_sw, d_s, d_se);
    
    curl = CURL();
    
}

__DEVICE__ float reduce(mat3 a, mat3 b) {
    mat3 p = matrixCompMult(a, b);
    return p[0][0] + p[0][1] + p[0][2] +
           p[1][0] + p[1][1] + p[1][2] +
           p[2][0] + p[2][1] + p[2][2];
}

__DEVICE__ float2 confinement(float2 fragCoord)
{
  float2 uv = fragCoord / iResolution;
    
    float k0 = CONF_ISOTROPY;
    float k1 = 1.0f - 2.0f*(CONF_ISOTROPY);

    mat3 conf_x = mat3(
       -k0, -k1, -k0,
        0.0f, 0.0f, 0.0f,
        k0,  k1,  k0
    );

    mat3 conf_y = mat3(
       -k0,  0.0f,  k0,
       -k1,  0.0f,  k1,
       -k0,  0.0f,  k0
    );
    
    mat3 mc;
    float2 v = to_float2(0);
    float curl;
    
    float cacc = 0.0f;
    float2 nacc = to_float2(0);
    float wc = 0.0f;
    for (int i = 0; i < DEGREE; i++) {
        tex(uv, mc, curl, i);
        float w = CONF_W_FUNCTION;
        float2 n = w * normz(to_float2(reduce(conf_x, mc), reduce(conf_y, mc)));
        v += curl * n;
        cacc += curl;
        nacc += n;
        wc += w;
    }

    #ifdef PREMULTIPLY_CURL
        return v / wc;
    #else
      return nacc * cacc / wc;
    #endif

}

__KERNEL__ void MultiscaleMipFluidFuse__Buffer_C(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel0)
{

    fragColor = to_float4(confinement(fragCoord),0,0);
    // Adding a very small amount of noise on init fixes subtle numerical precision blowup problems
    if (iFrame==0) fragColor=1e-6*rand4(fragCoord, iResolution, iFrame);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel0
// Connect Buffer D 'Previsualization: Buffer D' to iChannel1


#define VORT_CH xy
#define VORT_SAMPLER iChannel0
#define POIS_SAMPLER iChannel1
#define POIS_CH x
#define DEGREE POISSON_SCALES

#define D(d) textureLod(VORT_SAMPLER, fract(uv+d), mip).VORT_CH
#define P(d) textureLod(POIS_SAMPLER, fract(uv+d), mip).POIS_CH

__DEVICE__ float laplacian_poisson(float2 fragCoord) {
    const float _K0 = -20.0f/6.0f, _K1 = 4.0f/6.0f, _K2 = 1.0f/6.0f;
    float2 texel = 1.0f/iResolution;
    float2 uv = fragCoord * texel;
    float4 t = to_float4(texel, -texel.y, 0);
    float mip = 0.0f;

    float p =    P( swi2(t,w,w)); float p_n =  P( swi2(t,w,y)); float p_e =  P( swi2(t,x,w));
    float p_s =  P( swi2(t,w,z)); float p_w =  P(-swi2(t,x,w)); float p_nw = P(-swi2(t,x,z));
    float p_sw = P(-swi2(t,x,y)); float p_ne = P( swi2(t,x,y)); float p_se = P( swi2(t,x,z));
    
    return _K0 * p + _K1 * (p_e + p_w + p_n + p_s) + _K2 * (p_ne + p_nw + p_se + p_sw);
}

__DEVICE__ void tex(float2 uv, inout mat3 mx, inout mat3 my, inout mat3 mp, int degree) {
    float2 texel = 1.0f/iResolution;
    float stride = float(1 << degree);
    float mip = float(degree);
    float4 t = stride * to_float4(texel, -texel.y, 0);

    float2 d =    D( swi2(t,w,w)); float2 d_n =  D( swi2(t,w,y)); float2 d_e =  D( swi2(t,x,w));
    float2 d_s =  D( swi2(t,w,z)); float2 d_w =  D(-swi2(t,x,w)); float2 d_nw = D(-swi2(t,x,z));
    float2 d_sw = D(-swi2(t,x,y)); float2 d_ne = D( swi2(t,x,y)); float2 d_se = D( swi2(t,x,z));
    
    float p =    P( swi2(t,w,w)); float p_n =  P( swi2(t,w,y)); float p_e =  P( swi2(t,x,w));
    float p_s =  P( swi2(t,w,z)); float p_w =  P(-swi2(t,x,w)); float p_nw = P(-swi2(t,x,z));
    float p_sw = P(-swi2(t,x,y)); float p_ne = P( swi2(t,x,y)); float p_se = P( swi2(t,x,z));
    
    mx =  mat3(d_nw.x, d_n.x, d_ne.x,
               d_w.x,  d.x,   d_e.x,
               d_sw.x, d_s.x, d_se.x);
    
    my =  mat3(d_nw.y, d_n.y, d_ne.y,
               d_w.y,  d.y,   d_e.y,
               d_sw.y, d_s.y, d_se.y);
    
    mp =  mat3(p_nw, p_n, p_ne,
               p_w,  p,   p_e,
               p_sw, p_s, p_se);
}

__DEVICE__ float reduce(mat3 a, mat3 b) {
    mat3 p = matrixCompMult(a, b);
    return p[0][0] + p[0][1] + p[0][2] +
           p[1][0] + p[1][1] + p[1][2] +
           p[2][0] + p[2][1] + p[2][2];
}

__DEVICE__ float2 pois(float2 fragCoord)
{
  float2 uv = fragCoord / iResolution;
    
    float k0 = POIS_ISOTROPY;
    float k1 = 1.0f - 2.0f*(POIS_ISOTROPY);
    
    mat3 pois_x = mat3(
        k0,  0.0f, -k0,
        k1,  0.0f, -k1,
        k0,  0.0f, -k0
    );
     
    mat3 pois_y = mat3(
       -k0,  -k1,  -k0,
        0.0f,  0.0f,  0.0f,
        k0,   k1,   k0
    );

    mat3 gauss = mat3(
       0.0625f, 0.125f, 0.0625f,  
       0.125f,  0.25f,  0.125f,
       0.0625f, 0.125f, 0.0625
    );
    
    mat3 mx, my, mp;
    float2 v = to_float2(0);
    
    float wc = 0.0f;
    for (int i = 0; i < DEGREE; i++) {
        tex(uv, mx, my, mp, i);
        float w = POIS_W_FUNCTION;
        wc += w;
      v += w * to_float2(reduce(pois_x, mx) + reduce(pois_y, my), reduce(gauss, mp));
    }

    return v / wc;

}

#define V(d) textureLod(VORT_SAMPLER, fract(uv+d), mip).zw

__KERNEL__ void MultiscaleMipFluidFuse__Buffer_D(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{


    float2 p = pois(fragCoord);
    #ifdef USE_PRESSURE_ADVECTION
        float mip = 0.0f;
        float2 tx = 1.0f / iResolution;
        float2 uv = fragCoord * tx;
        float prev = P(0.0002f * PRESSURE_ADVECTION * tx * V(to_float2(0)));
        fragColor = to_float4_aw(_mix(p.x + p.y, prev + PRESSURE_LAPLACIAN * laplacian_poisson(fragCoord), PRESSURE_UPDATE_SMOOTHING));
    #else
      fragColor = to_float4(p.x + p.y);
    #endif
    // Adding a very small amount of noise on init fixes subtle numerical precision blowup problems
    if (iFrame==0) fragColor=1e-6*rand4(fragCoord, iResolution, iFrame);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel2
// Connect Image 'Previsualization: Buffer C' to iChannel3
// Connect Image 'Previsualization: Buffer D' to iChannel1


/* 
  Created by Cornus Ammonis (2019)
  Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.
*/

/*
  This is a mipmap-based approach to multiscale fluid dynamics.

  Check the Common tab for lots of configurable parameters.

  Click to interact with your mouse. I'd recommend turning off the "pump" by
  setting PUMP_SCALE to 0.0f on line 113 of the Common tab to play around with
  just mouse interaction.

  Buffer B is a multiscale method for computing turbulence along the lines of 
  the Large Eddy Simulation method; multiscale curl is also computed in Buffer B, 
    to be passed along to Buffer C.
  
  Buffer C is a fairly conventional Vorticity Confinement method, also multiscale, 
    using the curl computed in Buffer B. It probably makes more sense to compute 
    each curl scale separately before accumulating, but for the sake of efficiency 
    and simplicity (a larger kernel would be required), I haven't done that here.

  Buffer D is a multiscale Poisson solver, which converges rapidly but not to an 
    exact solution - this nonetheless works well for the purposes of divergence 
    minimization since we only need the gradient, with allowances for the choice of
    scale weighting. 

  Buffer A computes subsampled advection and velocity update steps, sampling
    from Buffers B, C, and D with a variety of smoothing options.

  There are a number of options included to make this run faster.

  Using mipmaps in this way has a variety of advantages:

  1.0f The scale computations have no duplicative or dependent reads, we only need 
       that for advection.
  2.0f No randomness or stochastic sampling is involved.
  3.0f The total number of reads can be greatly reduced for a comparable level of 
       fidelity to some other methods.
  4.0f We can easily sample the entire buffer in one pass (on average).
  5.0f The computational complexity is deferred to mipmap generation (though with
       a large coefficient), because: 
  6.0f The algorithm itself is O(n) with a fixed number of scales (or we could 
       potentially do scale calculations in parallel with mipmap generation, 
       equalling mipmap generation complexity at O(nlogn))

  Notable downsides:

  1.0f Using mipmaps introduces a number of issues, namely:
       a. Mipmaps can introduce artifacts due to interpolation and downsampling. 
          Using Gaussian pyramids, or some other lowpass filtering method would 
          be better. 
       b. Using higher-order sampling of the texture buffer (e.g. bicubic) would 
          also be better, but that would limit our performance gains. 
       c. NPOT textures are problematic (as always). They can introduce weird 
          anisotropy issues among other things.
  2.0f Stochastic or large-kernel methods are a better approximation to the true
       sampling distribution approximated here, for a large-enough number of
       samples.
    3.0f We're limited in how we construct our scale-space. Is a power-of-two stride 
       length on both axes always ideal, even along diagonals? I'm not particularly 
       sure. There are clever wavelet methods out there for Navier-Stokes solvers, 
       and LES in particular, too.

*/


#define BUMP 3200.0

#define D(d) -textureLod(iChannel1, fract(uv+(d+0.0f)), mip).w

__DEVICE__ float2 diff(float2 uv, float mip) {
    float2 texel = 1.0f/iResolution;
    float4 t = float(1<<int(mip))*to_float4(texel, -texel.y, 0);

    float d =    D( swi2(t,w,w)); float d_n =  D( swi2(t,w,y)); float d_e =  D( swi2(t,x,w));
    float d_s =  D( swi2(t,w,z)); float d_w =  D(-swi2(t,x,w)); float d_nw = D(-swi2(t,x,z));
    float d_sw = D(-swi2(t,x,y)); float d_ne = D( swi2(t,x,y)); float d_se = D( swi2(t,x,z));
    
    return to_float2(
        0.5f * (d_e - d_w) + 0.25f * (d_ne - d_nw + d_se - d_sw),
        0.5f * (d_n - d_s) + 0.25f * (d_ne + d_nw - d_se - d_sw)
    );
}

__DEVICE__ float4 contrast(float4 col, float x) {
  return x * (col - 0.5f) + 0.5f;
}

__KERNEL__ void MultiscaleMipFluidFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{

    float2 uv = fragCoord / iResolution;

    float2 dxy = to_float2(0);
    float occ, mip = 0.0f;
    float d   = D();
    
    // blur the gradient to reduce appearance of artifacts,
    // and do cheap occlusion with mipmaps
    #define STEPS 10.0
    #define ODIST 2.0
    for(mip = 1.0f; mip <= STEPS; mip += 1.0f) {   
        dxy += (1.0f/_powf(2.0f,mip)) * diff(uv, mip-1.0f);  
      occ += softclamp(-ODIST, ODIST, d - D(),1.0f)/(_powf(1.5f,mip));
    }
    dxy /= float(STEPS);
    
    // I think this looks nicer than using smoothstep
    occ = _powf(_fmaxf(0.0f,softclamp(0.2f,0.8f,100.0f*occ + 0.5f,1.0f)),0.5f);
 
    float3 avd;
    float3 ld = light(uv, BUMP, 0.5f, dxy, iTime, avd);
    
    float spec = ggx(avd, to_float3(0,1,0), ld, 0.1f, 0.1f);
    
    #define LOG_SPEC 1000.0
    spec = (_logf(LOG_SPEC+1.0f)/LOG_SPEC)*_logf(1.0f+LOG_SPEC*spec);    
    
    #define VIEW_VELOCITY
    
    #ifdef VIEW_VELOCITY
    float4 diffuse = softclamp(0.0f,1.0f,6.0f*to_float4(_tex2DVecN(iChannel0,uv.x,uv.y,15).xy,0,0)+0.5f,2.0f);    
    #elif defined(VIEW_CURL)
    float4 diffuse = _mix(to_float4(1,0,0,0),to_float4(0,0,1,0),softclamp(0.0f,1.0f,0.5f+2.0f*_tex2DVecN(iChannel2,uv.x,uv.y,15).w,2.0f));    
    #elif defined(VIEW_ADVECTION)
    float4 diffuse = softclamp(0.0f,1.0f,0.0004f*to_float4(_tex2DVecN(iChannel0,uv.x,uv.y,15).zw,0,0)+0.5f,2.0f); 
    #elif defined(VIEW_GRADIENT)
      float4 diffuse = softclamp(0.0f,1.0f,10.0f*to_float4_aw(diff(uv,0.0f),0,0)+0.5f,4.0f); 
    #else // Vorticity confinement vectors
      float4 diffuse = softclamp(0.0f,1.0f,4.0f*to_float4(_tex2DVecN(iChannel3,uv.x,uv.y,15).xy,0,0)+0.5f,4.0f);
    #endif
    
    
    fragColor = (diffuse + 4.0f*_mix(to_float4(spec),1.5f*diffuse*spec,0.3f));
    fragColor = _mix(1.0f,occ,0.7f) * (softclamp(0.0f,1.0f,contrast(fragColor,4.5f),3.0f));
    
    //fragColor = to_float4(occ);
    //fragColor = to_float4(spec);
    //fragColor = diffuse;
    //fragColor = to_float4(diffuse+(occ-0.5f));


  SetFragmentShaderComputedColor(fragColor);
}