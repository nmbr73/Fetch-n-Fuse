
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution

__DEVICE__ float2 normz(float2 x) { return (x.x == 0.0f && x.y == 0.0f) ? to_float2_s(0) : normalize(x); }
__DEVICE__ float3 normz(float3 x) { return (x.x == 0.0f && x.y == 0.0f && x.z == 0.0f) ? to_float3_s(0) : normalize(x); }


__DEVICE__ float mul1 = 1.0f, mul2= 1.0f, mul3= 1.0f, mul4= 1.0f;

/* Texture Stencil Library https://www.shadertoy.com/view/ssBczm

The MIT License

Copyright (c) 2022 David A Roberts <https://davidar.io/>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

__DEVICE__ float4 texStencil(__TEXTURE2D__ ch, float2 uv, float coeff[9], float2 R) {
    //float2 texel = 1.0f / to_float2(textureSize(ch, 0));
    float2 texel = 1.0f / R;
    const float2 stencilOffset[9] = {
        to_float2(-1, 1), to_float2( 0, 1), to_float2( 1, 1),
        to_float2(-1, 0), to_float2( 0, 0), to_float2( 1, 0),
        to_float2(-1,-1), to_float2( 0,-1), to_float2( 1,-1)
    };
    float4 r = to_float4_s(0);
    for (int i = 0; i < 9; i++)
        r += coeff[i] * texture(ch, uv + texel * stencilOffset[i]);
    return r;
}


__DEVICE__ float *mul_ary_f(float ary[9], float mul)
{
   for ( int i = 0; i<9;i++)
   {
      ary[i] *= mul;
   }
   return ary;
}

// Gaussian/binomial blur
// https://bartwronski.com/2021/10/31/practical-gaussian-filter-binomial-filter-and-small-sigma-gaussians/
__DEVICE__ float4 texBlur(__TEXTURE2D__ ch, float2 uv, float2 R) {
    
    #define ORG
    #ifdef ORG
    float _ary[9] =  {
        0.0625f, 0.125f, 0.0625f,
        0.125f,  0.25f,  0.125f,
        0.0625f, 0.125f, 0.0625f
        };
    #else    
    float _ary[9] =  {
        0.0325f, 0.0125f, 0.0325f,
        0.0125f,  0.025f,  0.0125f,
        0.0325f, 0.0125f, 0.0325f 
        };        
    #endif         
    float *ary = mul_ary_f(_ary, mul1);
    return texStencil(ch, uv, ary, R);
}

// Laplacian, optimal 9-point stencil
// https://docs.lib.purdue.edu/cgi/viewcontent.cgi?article=1928&context=cstech
__DEVICE__ float4 texLapl(__TEXTURE2D__ ch, float2 uv, float2 R) {
    
    float ary[9] =  {
        1.0f,   4.0f, 1.0f,
        4.0f, -20.0f, 4.0f,
        1.0f,   4.0f, 1.0f
        };
      
    //ary*=mul2;
    return texStencil(ch, uv, ary, R ) / 6.0f;
}

// horizontal gradient (Sobel filter)
__DEVICE__ float4 texGradX(__TEXTURE2D__ ch, float2 uv, float2 R) {
    
    float ary[9] =  {
        -1.0f, 0.0f, 1.0f,
        -2.0f, 0.0f, 2.0f,
        -1.0f, 0.0f, 1.0f
        };
    //ary*=mul3;
    return texStencil(ch, uv, ary, R) / 8.0f;
}

// vertical gradient (Sobel filter)
__DEVICE__ float4 texGradY(__TEXTURE2D__ ch, float2 uv, float2 R) {
    
    float ary[9] =  {
         1.0f,  2.0f,  1.0f,
         0.0f,  0.0f,  0.0f,
        -1.0f, -2.0f, -1.0f
        };
    //ary*=mul4;      
    return texStencil(ch, uv, ary, R) / 8.0f;
}


// IQ's simplex noise:

// The MIT License
// Copyright © 2013 Inigo Quilez
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

__DEVICE__ float2 hash( float2 p ) // replace this by something better
{
  p = to_float2( dot(p,to_float2(127.1f,311.7f)),
                 dot(p,to_float2(269.5f,183.3f)) );

  return -1.0f + 2.0f*fract_f2(sin_f2(p)*43758.5453123f);
}

__DEVICE__ float noise( in float2 p )
{
    const float K1 = 0.366025404f; // (_sqrtf(3)-1)/2;
    const float K2 = 0.211324865f; // (3-_sqrtf(3))/6;

    float2 i = _floor( p + (p.x+p.y)*K1 );
  
    float2 a = p - i + (i.x+i.y)*K2;
    float2 o = step(swi2(a,y,x),swi2(a,x,y));    
    float2 b = a - o + K2;
    float2 c = a - 1.0f + 2.0f*K2;

    float3 h = _fmaxf( 0.5f-to_float3(dot(a,a), dot(b,b), dot(c,c) ), to_float3_s(0.0f) );

    float3 n = h*h*h*h*to_float3( dot(a,hash(i+0.0f)), dot(b,hash(i+o)), dot(c,hash(i+1.0f)));
float nnnnnnnnnnnnnnnnn;
    return dot( n, to_float3_s(70.0f) );
  
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
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1
// Connect Buffer A 'Previsualization: Buffer C' to iChannel2


#define TIMESTEPA 0.7f

#define PI 3.14159265359f

#define A iChannel0
#define B iChannel1
#define C iChannel2

__DEVICE__ float sigmoid(float x) {
    return 0.5f + 0.5f * _tanhf(-5.0f * (x - 1.0f));
}

__KERNEL__ void SuturedSatinFuse__Buffer_A(float4 r, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel2)
{
    CONNECT_CHECKBOX0(Reset, 0);
    
    CONNECT_SLIDER1(_mul1, -10.0f, 100000.0f, 1.0f);
    CONNECT_SLIDER2(_mul2, -10.0f, 10000000.0f, 1.0f);
    CONNECT_SLIDER3(_mul3, -10.0f, 1000000000.0f, 1.0f);
    CONNECT_SLIDER4(_mul4, -10.0f, 100000000.0f, 1.0f);
    
    mul1 = _mul1; 
    mul2 = _mul2;
    mul3 = _mul3;
    mul4 = _mul4;
    
    
    fragCoord+=0.5f; 
    
    float2 p = fragCoord / iResolution;
    float2 stepSize = 1.0f / iResolution;
    
    // initialize with noise
    if(iFrame < 10 || Reset) {
        float3 rnd = to_float3(noise(8.0f * p + 1.1f), noise(8.0f * p + 2.2f), noise(8.0f * p + 3.3f));
        r = to_float4_aw(rnd,0);
        SetFragmentShaderComputedColor(r);
        return;
    }
    
    float3 ma = to_float3_s(0);
    float gcurve = 0.0f;
    for (int i = 0; i < 12; i++) {
        float angle = (float)(i) * PI/6.0f;
        float2 offset = to_float2(_cosf(angle), _sinf(angle));
        float3 spring = to_float3_aw(offset, 0) + swi3(texture(C, p + offset / iResolution),x,y,z) - swi3(_tex2DVecN(C,p.x,p.y,15),x,y,z);
        ma += sigmoid(length(spring)) * spring;
        
        float angle1 = (float)(i+1) * PI/6.0f;
        float2 offset1 = to_float2(_cosf(angle1), _sinf(angle1));
        float3 spring1 = to_float3_aw(offset1, 0) + swi3(texture(C, p + offset1 / iResolution),x,y,z) - swi3(_tex2DVecN(C,p.x,p.y,15),x,y,z);
        gcurve += PI/6.0f - _acosf(dot(normz(spring), normz(spring1)));
    }
float AAAAAAAAAAAAAAA;    
    float3 dv = swi3(texBlur(A, p - stepSize * swi2(_tex2DVecN(A,p.x,p.y,15),x,y),R),x,y,z);
    dv += swi3(texLapl(A, p - stepSize * swi2(_tex2DVecN(A,p.x,p.y,15),x,y),R),x,y,z);
    dv = swi3(texBlur(A, p - stepSize * 48.0f * (2.87f + 1e4 * gcurve) * (swi2(_tex2DVecN(C,p.x,p.y,15),x,y) - swi2(dv,x,y)),R),x,y,z);
    dv += TIMESTEPA * ma;

    float t = 0.05f * iTime;
    float2 m = fract_f2(to_float2(noise(to_float2(t,0)), noise(to_float2(0,t)))) * iResolution;
    if (iMouse.z > 0.0f) m = swi2(iMouse,x,y);
    float2 d = fragCoord - m;
    dv += 5.0f * _expf(-length(d) / 50.0f) * normz(to_float3_aw(d, 0.0f));
    
    // hard clamping
    //dv = length(dv) > 1.0f ? normz(dv) : dv;
    // soft clamping
    dv -= 0.005f * _powf(length(dv), 3.0f) * normz(dv);
    //swi3S(r,x,y,z, _mix(swi3(_tex2DVecN(A,p.x,p.y,15),x,y,z), dv, 0.5f));

    r = to_float4_aw(_mix(swi3(_tex2DVecN(A,p.x,p.y,15),x,y,z), dv, 0.5f), 0.0f);

//r = _tex2DVecN(iChannel3,p.x,p.y,15);

  SetFragmentShaderComputedColor(r);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1


#define TIMESTEPB 0.1f

//#define A iChannel0
//#define B iChannel1

__KERNEL__ void SuturedSatinFuse__Buffer_B(float4 r, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0);
    fragCoord+=0.5f; 
    float2 p = fragCoord / iResolution;
    
    // initialize with noise
    if(iFrame < 10 || Reset) {
        float3 rnd = to_float3(noise(16.0f * p + 1.1f), noise(16.0f * p + 2.2f), noise(16.0f * p + 3.3f));
        r = to_float4_aw(rnd,0);
        SetFragmentShaderComputedColor(r);
        return;
    }
   
    float3 du = swi3(texBlur(B,p,R),x,y,z) + TIMESTEPB * swi3(texBlur(A,p,R),x,y,z);
    // hard clamping
    //du = length(du) > 1.0f ?  normz(du) : du;
    // soft clamping
    du -= 0.005f * _powf(length(du), 3.0f) * normz(du);
    float BBBBBBBBBBBBBBBBBB; 
    //swi3S(r,x,y,z, _mix(swi3(_tex2DVecN(B,p.x,p.y,15),x,y,z), du, 1.0f));
    r = to_float4_aw( _mix(swi3(_tex2DVecN(B,p.x,p.y,15),x,y,z), du, 1.0f),0.0f);
    //r.w = texBlur(A,p,R).w;
    r.w = texGradX(A,p,R).x + texGradY(A,p,R).y;

  SetFragmentShaderComputedColor(r);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


// based on Suture Fluid

__DEVICE__ float3 rotateAxis(float3 p, float3 axis, float angle) {
    return _mix(dot(axis, p) * axis, p, _cosf(angle)) + cross(axis, p) * _sinf(angle);
}

__KERNEL__ void SuturedSatinFuse__Buffer_C(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, int iFrame, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(Reset, 0);
    fragCoord+=0.5f; 
    float2 uv = fragCoord / iResolution;
float CCCCCCCCCCCCCCCCCCCC;
    if (iFrame < 10 || Reset) {
        fragColor = to_float4(noise(16.0f * uv + 1.1f), noise(16.0f * uv + 2.2f), noise(16.0f * uv + 3.3f), 0);
        SetFragmentShaderComputedColor(fragColor);
        return;
    }

    float divergence = _mix(_tex2DVecN(iChannel0,uv.x,uv.y,15).w, texLapl(iChannel0, uv,R).w, 0.25f); // divergence smoothing
    divergence = _mix(divergence, texGradX(iChannel0, uv,R).x + texGradY(iChannel0, uv,R).y, 1.0f); // divergence update

    float2 stepSize = 6.0f / iResolution;
    float3 velocity = swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z);
    float3 advected = swi3(texBlur(iChannel0, uv - stepSize * swi2(velocity,x,y),R),x,y,z);
    advected += 2.0f * swi3(texLapl(iChannel0, uv - stepSize * swi2(velocity,x,y),R),x,y,z);
    advected += 0.5f * swi3(texLapl(iChannel0, uv,R),x,y,z);
    advected -= 0.5f * swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z) * divergence;
    advected -= 0.8f * texLapl(iChannel0, uv,R).z * normz(velocity);
    advected -= clamp(iTime - 5.0f, 0.0f, 9.0f) * texBlur(iChannel0, uv,R).w * normz(velocity);

    float3 curl = to_float3(
                            texGradY(iChannel0, uv,R).z - 0.0f,
                            0.0f - texGradX(iChannel0, uv,R).z,
                            texGradX(iChannel0, uv,R).y - texGradY(iChannel0, uv,R).x);
    if (length(curl) > 0.0f)
        advected = rotateAxis(advected, normalize(curl), 10.0f * length(curl));
        
    advected += 1.5f * curl;

    if (length(advected) > 1.0f) advected = normalize(advected);
    divergence = clamp(divergence, -1.0f, 1.0f);
    fragColor = _mix(_tex2DVecN(iChannel0,uv.x,uv.y,15), to_float4_aw(advected, divergence), 0.2f); // update smoothing

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel0
// Connect Buffer D 'Previsualization: Buffer B' to iChannel1
// Connect Buffer D 'Previsualization: Buffer C' to iChannel2
// Connect Buffer D 'Previsualization: Buffer D' to iChannel3


// This convolves the Laplacian values with a specially-designed Poisson solver kernel.

__KERNEL__ void SuturedSatinFuse__Buffer_D(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel1, sampler2D iChannel3)
{
    CONNECT_CHECKBOX0(Reset, 0);
    fragCoord+=0.5f; 
    const float _K0 = 20.0f/6.0f; // center weight

    float2 uv = fragCoord / iResolution;
    float2 texel = 1.0f / iResolution;
 
    /* 
    Poisson solver kernel, computed using a custom tool. The curve ended up being very close
      to _expf(-x) times a constant (0.43757f*_expf(-1.0072f*x), R^2 = 0.9997f).
      The size of the kernel is truncated such that 99% of the summed kernel weight is accounted for. 
  */
    float a[121] = {
        1.2882849374994847E-4, 3.9883638750009155E-4, 9.515166750018973E-4, 0.0017727328875003466f, 0.0025830133546736567f, 0.002936729756271805f, 0.00258301335467621f, 0.0017727328875031007f, 9.515166750027364E-4, 3.988363875000509E-4, 1.2882849374998886E-4,
        3.988363875000656E-4, 0.00122005053750234f, 0.0029276701875229076f, 0.005558204850002636f, 0.008287002243739282f, 0.009488002668845403f, 0.008287002243717386f, 0.005558204850002533f, 0.002927670187515983f, 0.0012200505375028058f, 3.988363875001047E-4,
        9.515166750033415E-4, 0.0029276701875211478f, 0.007226947743770152f, 0.014378101312275642f, 0.02243013709214819f, 0.026345595431380788f, 0.02243013709216395f, 0.014378101312311218f, 0.007226947743759695f, 0.0029276701875111384f, 9.515166750008558E-4,
        0.0017727328875040689f, 0.005558204850002899f, 0.014378101312235814f, 0.030803252137257802f, 0.052905271651623786f, 0.06562027788638072f, 0.052905271651324026f, 0.03080325213733769f, 0.014378101312364885f, 0.005558204849979354f, 0.0017727328874979902f,
        0.0025830133546704635f, 0.008287002243679713f, 0.02243013709210261f, 0.052905271651950365f, 0.10825670746239457f, 0.15882720544362505f, 0.10825670746187367f, 0.05290527165080182f, 0.02243013709242713f, 0.008287002243769156f, 0.0025830133546869602f,
        0.00293672975627608f, 0.009488002668872716f, 0.026345595431503218f, 0.06562027788603421f, 0.15882720544151602f, 0.44102631192030745f, 0.15882720544590473f, 0.06562027788637015f, 0.026345595431065568f, 0.009488002668778417f, 0.0029367297562566848f,
        0.0025830133546700966f, 0.008287002243704267f, 0.022430137092024266f, 0.05290527165218751f, 0.10825670746234733f, 0.1588272054402839f, 0.1082567074615041f, 0.052905271651381314f, 0.022430137092484193f, 0.00828700224375486f, 0.002583013354686416f,
        0.0017727328875014527f, 0.005558204850013428f, 0.01437810131221156f, 0.03080325213737849f, 0.05290527165234342f, 0.06562027788535467f, 0.05290527165227899f, 0.03080325213731504f, 0.01437810131229074f, 0.005558204849973625f, 0.0017727328874977803f,
        9.515166750022218E-4, 0.002927670187526038f, 0.0072269477437592895f, 0.014378101312185454f, 0.02243013709218059f, 0.02634559543148722f, 0.0224301370922164f, 0.014378101312200022f, 0.007226947743773282f, 0.0029276701875125123f, 9.515166750016471E-4,
        3.988363875000695E-4, 0.0012200505375021846f, 0.002927670187525898f, 0.005558204849999022f, 0.008287002243689638f, 0.009488002668901728f, 0.008287002243695645f, 0.0055582048500028335f, 0.002927670187519828f, 0.0012200505375025872f, 3.988363874999818E-4,
        1.2882849374993535E-4, 3.9883638750004726E-4, 9.515166750034058E-4, 0.0017727328875029819f, 0.0025830133546718525f, 0.002936729756279661f, 0.002583013354672541f, 0.0017727328875033709f, 9.515166750023861E-4, 3.988363874999023E-4, 1.2882849374998856E-4
    };
    
    float b[121] = {
        8673174.0f, 1.5982146E7, 2.5312806E7, 3.4957296E7, 4.2280236E7, 4.5059652E7, 4.2280236E7, 3.4957296E7, 2.5312806E7, 1.5982146E7, 8673174.0f,
        1.5982146E7, 2.9347785E7, 4.6341531E7, 6.3895356E7, 7.7184405E7, 8.2245411E7, 7.7184405E7, 6.3895356E7, 4.6341531E7, 2.9347785E7, 1.5982146E7,
        2.5312806E7, 4.6341531E7, 7.2970173E7, 1.00453608E8, 1.21193181E8, 1.29118131E8, 1.21193181E8, 1.00453608E8, 7.2970173E7, 4.6341531E7, 2.5312806E7,
        3.4957296E7, 6.3895356E7, 1.00453608E8, 1.38192768E8, 1.66613346E8, 1.77507756E8, 1.66613346E8, 1.38192768E8, 1.00453608E8, 6.3895356E7, 3.4957296E7,
        4.2280236E7, 7.7184405E7, 1.21193181E8, 1.66613346E8, 2.00759625E8, 2.13875721E8, 2.00759625E8, 1.66613346E8, 1.21193181E8, 7.7184405E7, 4.2280236E7,
        4.5059652E7, 8.2245411E7, 1.29118131E8, 1.77507756E8, 2.13875721E8, 2.27856753E8, 2.13875721E8, 1.77507756E8, 1.29118131E8, 8.2245411E7, 4.5059652E7,
        4.2280236E7, 7.7184405E7, 1.21193181E8, 1.66613346E8, 2.00759625E8, 2.13875721E8, 2.00759625E8, 1.66613346E8, 1.21193181E8, 7.7184405E7, 4.2280236E7,
        3.4957296E7, 6.3895356E7, 1.00453608E8, 1.38192768E8, 1.66613346E8, 1.77507756E8, 1.66613346E8, 1.38192768E8, 1.00453608E8, 6.3895356E7, 3.4957296E7,
        2.5312806E7, 4.6341531E7, 7.2970173E7, 1.00453608E8, 1.21193181E8, 1.29118131E8, 1.21193181E8, 1.00453608E8, 7.2970173E7, 4.6341531E7, 2.5312806E7,
        1.5982146E7, 2.9347785E7, 4.6341531E7, 6.3895356E7, 7.7184405E7, 8.2245411E7, 7.7184405E7, 6.3895356E7, 4.6341531E7, 2.9347785E7, 1.5982146E7,
        8673174.0f, 1.5982146E7, 2.5312806E7, 3.4957296E7, 4.2280236E7, 4.5059652E7, 4.2280236E7, 3.4957296E7, 2.5312806E7, 1.5982146E7, 8673174.0
    };
    
    float4 ac = to_float4_s(0);
    float4 bc = to_float4_s(0);
    float4 bcw = to_float4_s(0);
    for (int i = -5; i <= 5; i++) {
        for (int j = -5; j <= 5; j++) {
            int index = (j + 5) * 11 + (i + 5);
            float4 tx0 = to_float4_s(texture(iChannel1, uv + texel * to_float2(i,j)).w);
            float4 tx1 = texture(iChannel3, (uv + texel * to_float2(i,j)));
            ac  += -a[index] * tx0;
            bcw +=  b[index];
            bc  +=  b[index] * tx1;
        }
    }
    
    bc /= bcw;
        
    fragColor = ac+bc;//+bc;
    
    if (iFrame < 10 || Reset) {
      fragColor = to_float4_s(0.0f);
    }

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: Forest_0' to iChannel1
// Connect Image 'Previsualization: Buffer D' to iChannel0


#define BUMP 0.3f

// dispersion amount
#define DISP_SCALE 0.3f

// minimum IOR
#define MIN_IOR 1.1f

// chromatic dispersion samples, higher values decrease banding
#define SAMPLES 9

// time scale
//#define TIME 0.1f*iTime

// sharpness of the sample weight distributions, higher values increase separation of colors
#define SHARP 15.0f

#define FILMIC
#ifdef FILMIC
// tweaked version of a filmic curve from paniq with a softer left knee
__DEVICE__ float3 contrast(float3 x) {
    x=log_f3(1.0f+exp_f3(x*10.0f-7.2f));
    return (x*(x*6.2f+0.5f))/(x*(x*6.2f+1.7f)+0.06f);
}
#else
#define SIGMOID_CONTRAST 8.0f
__DEVICE__ float3 contrast(float3 x) {
  return (1.0f / (1.0f + exp_f3(-SIGMOID_CONTRAST * (x - 0.5f))));    
}
#endif

__DEVICE__ float3 sampleWeights(float i) {
  return to_float3(_expf(-SHARP*_powf(i-0.25f,2.0f)), _expf(-SHARP*_powf(i-0.5f,2.0f)), _expf(-SHARP*_powf(i-0.75f,2.0f)));
}

__DEVICE__ mat3 cameraMatrix(float TIME) {
    float3 ro = to_float3(_sinf(TIME),0.0f,_cosf(TIME));
    float3 ta = to_float3(0,1.5f,0);  
    float3 w = normalize(ta - ro);
    float3 u = normalize(cross(w,to_float3(0,1,0)));
    float3 v = normalize(cross(u,w));
    return to_mat3_f3(u,v,w);
}

// same as the normal refract() but returns the coefficient
__DEVICE__ float3 refractK(float3 I, float3 N, float eta, out float *k) {
    *k = _fmaxf(0.0f,1.0f - eta * eta * (1.0f - dot(N, I) * dot(N, I)));
    if (*k <= 0.0f)
        return to_float3_s(0.0f);
    else
        return eta * I - (eta * dot(N, I) + _sqrtf(*k)) * N;
}

__DEVICE__ float3 sampleDisp(float2 uv, float3 disp, float TIME, __TEXTURE2D__ iChannel1) {
  float2 p = uv - 0.5f;

    // camera movement
    mat3 camMat = cameraMatrix(TIME);

    float3 rd = normz(mul_mat3_f3(camMat , to_float3_aw(p, 1.0f)));
    float3 norm = normz(mul_mat3_f3(camMat , disp));
    
    float3 col = to_float3_s(0);
    const float SD = 1.0f / (float)(SAMPLES);
    float wl = 0.0f;
    float3 denom = to_float3_s(0);
    for(int i = 0; i < SAMPLES; i++) {
        float3 sw = sampleWeights(wl);
        denom += sw;
        float k;
        float3 refr = refractK(rd, norm, MIN_IOR + wl * DISP_SCALE, &k);
        float3 refl = reflect(rd, norm);
        col += sw * _mix(swi3(decube_f3(iChannel1, refl),x,y,z), swi3(decube_f3(iChannel1,refr),x,y,z), k);
        wl  += SD;
    }
    
    return col / denom;
}

__KERNEL__ void SuturedSatinFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_SLIDER0(Debugging, -100.0f, 100.0f, 1.0f);
    CONNECT_SCREW0(Spiel, -10.0f, 10.0f, 1.0f);
  
    fragCoord+=0.5f; 
    
    float TIME = 0.1f*iTime; 
    
    float2 texel = 1.0f / iResolution;
    float2 uv = fragCoord / iResolution;

    float2 n  = to_float2(0.0f, texel.y);
    float2 e  = to_float2(texel.x, 0.0f);
    float2 s  = to_float2(0.0f, -texel.y);
    float2 w  = to_float2(-texel.x, 0.0f);

    float d   = _tex2DVecN(iChannel0,uv.x,uv.y,15).x;
    // uncomment to just render the heightmap
    //#define SIMPLE
    #ifdef SIMPLE
    fragColor = 0.5f+0.02f*to_float4_s(d);
    #else
    float d_n  = texture(iChannel0, (uv+n)  ).x;
    float d_e  = texture(iChannel0, (uv+e)  ).x;
    float d_s  = texture(iChannel0, (uv+s)  ).x;
    float d_w  = texture(iChannel0, (uv+w)  ).x; 
    float d_ne = texture(iChannel0, (uv+n+e)).x;
    float d_se = texture(iChannel0, (uv+s+e)).x;
    float d_sw = texture(iChannel0, (uv+s+w)).x;
    float d_nw = texture(iChannel0, (uv+n+w)).x; 

    float dxn[3];
    float dyn[3];
    float dcn[3];
    
    dcn[0] = 0.5f;
    dcn[1] = 1.0f; 
    dcn[2] = 0.5f;

    dyn[0] = d_nw - d_sw;
    dyn[1] = d_n  - d_s; 
    dyn[2] = d_ne - d_se;

    dxn[0] = d_ne - d_nw; 
    dxn[1] = d_e  - d_w; 
    dxn[2] = d_se - d_sw; 

    // The section below is an antialiased version of 
    // Shane's Bumped Sinusoidal Warp shadertoy here:
    // https://www.shadertoy.com/view/4l2XWK
  #define SRC_DIST 8.0
    float3 sp = to_float3_aw(uv-0.5f, 0);
    float3 light = to_float3(_cosf(iTime/2.0f)*0.5f, _sinf(iTime/2.0f)*0.5f, -SRC_DIST);
    float3 ld = light - sp;
    float lDist = _fmaxf(length(ld), 0.001f);
    ld /= lDist;
    float aDist = _fmaxf(distance_f3(to_float3(light.x, light.y, 0),sp) , 0.001f);
    float atten = _fminf(0.07f/(0.25f + aDist*0.5f + aDist*aDist*0.05f), 1.0f);
    float3 rd = normalize(to_float3_aw(uv - 0.5f, 1.0f));

    float spec = 0.0f;
    float den = 0.0f;
    
    float3 dispCol = to_float3_s(0);
    
    float _w; 
    // compute dispersion and specular with antialiasing
    float3 avd = to_float3_s(0);
    
    for(int i = 0; i < 3; i++) {
        for(int j = 0; j < 3; j++) {
            float2 dxy = to_float2(dxn[i], dyn[j]);
                    _w = dcn[i] * dcn[j];
            float3  bn = reflect(normalize(to_float3_aw(BUMP*dxy, -1.0f)), to_float3(0,1,0));
            avd += _w * bn;
            den += _w;
            dispCol += _w * sampleDisp(uv, bn, TIME, iChannel1); //* Debugging;//
            spec += _w * ggx(bn, to_float3(0,1,0), ld, 0.3f, 1.0f);
        }
    }

    avd /= den;
    spec /= den;
    dispCol /= den;
    
    // end bumpmapping section
    fragColor =  to_float4_aw(contrast(0.75f*dispCol),1) + 1.0f*to_float4(0.9f, 0.85f, 0.8f, 1)*spec;
    #endif

//fragColor = to_float4_aw(avd,1.0f);


  SetFragmentShaderComputedColor(fragColor);
}