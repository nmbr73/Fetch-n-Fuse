
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: RGBA Noise Small' to iChannel1
// Connect Buffer A 'Cubemap: Forest Blurred_0' to iChannel2
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


__DEVICE__ float3 _refract_f3(float3 I, float3 N, float eta, float refmul, float refoff) {
   float dotNI = dot(N, I);
   float k = 1.0f - eta * eta * (1.0f - dotNI * dotNI);
   if (k < 0.0f) {
     return to_float3_s(0.0);
   }
   return eta * I - (eta * dotNI * _sqrtf(k)) * N * refmul + refoff; //+0.5f;   * -01.50f;(MarchingCubes)  - 0.15f; (GlassDuck)
}


// Begin IQ's simplex noise:

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

    float3 h = _fmaxf( to_float3_s(0.5f)-to_float3(dot(a,a), dot(b,b), dot(c,c) ), to_float3_s(0.0f) );

    float3 n = h*h*h*h*to_float3( dot(a,hash(i+0.0f)), dot(b,hash(i+o)), dot(c,hash(i+1.0f)));
float zzzzzzzzzzzzzzzzz;
    return dot( n, to_float3_s(70.0f) );
  
}

// End IQ's simplex noise


__DEVICE__ float2 normz(float2 _x) {
  return (_x.x == 0.0f && _x.y == 0.0f) ? to_float2(0.0f, 0.0f) : normalize(_x);
}

// reverse advection
__DEVICE__ float3 advect(float2 ab, float2 vUv, float2 step, float sc, __TEXTURE2D__ iChannel0) {
    
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


__KERNEL__ void RayMarchingExperiment71JipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel3)
{
    CONNECT_CHECKBOX0(Reset, 0); 
float AAAAAAAAAAAAAAAAAA;   
    fragCoord+=0.5f;
    const float _K0 = -20.0f/6.0f; // center weight
    const float _K1 = 4.0f/6.0f;   // edge-neighbors
    const float _K2 = 1.0f/6.0f;   // vertex-neighbors
    const float cs = -0.6f;  // curl scale
    const float ls = 0.05f;  // laplacian scale
    const float ps = -0.8f;  // laplacian of divergence scale
    const float ds = -0.05f; // divergence scale
    const float dp = -0.04f; // divergence update scale
    const float pl = 0.3f;   // divergence smoothing
    const float ad = 6.0f;   // advection distance scale
    const float pwr = 1.0f;  // power when deriving rotation angle from curl
    const float amp = 1.0f;  // self-amplification
    const float upd = 0.8f;  // update smoothing
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

    float3 uv =    swi3(texture(iChannel0, fract(vUv)),x,y,z);
    float3 uv_n =  swi3(texture(iChannel0, fract(vUv+n)),x,y,z);
    float3 uv_e =  swi3(texture(iChannel0, fract(vUv+e)),x,y,z);
    float3 uv_s =  swi3(texture(iChannel0, fract(vUv+s)),x,y,z);
    float3 uv_w =  swi3(texture(iChannel0, fract(vUv+w)),x,y,z);
    float3 uv_nw = swi3(texture(iChannel0, fract(vUv+nw)),x,y,z);
    float3 uv_sw = swi3(texture(iChannel0, fract(vUv+sw)),x,y,z);
    float3 uv_ne = swi3(texture(iChannel0, fract(vUv+ne)),x,y,z);
    float3 uv_se = swi3(texture(iChannel0, fract(vUv+se)),x,y,z);
    
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
    
    /*if (iMouse.z > 0.0f) {
      float2 d = fragCoord - swi2(iMouse,x,y);
        float m = _expf(-length(d) / 10.0f);
        swi2(abd,x,y) += m * normz(d);
    }*/
    
    // initialize with noise
    if(iFrame<180 || Reset) {
        float3 rnd = to_float3(noise(16.0f * vUv + 1.1f), noise(16.0f * vUv + 2.2f), noise(16.0f * vUv + 3.3f));
        fragColor = to_float4_aw(rnd, 0);
        fragColor = -0.5f + texture(iChannel1, fragCoord / iResolution);
    } else {
        //fragColor = clamp(to_float4_aw(abd,0.0f), -1.0f, 1.0f);
        abd.z = clamp(abd.z, -1.0f, 1.0f);
        swi2S(abd,x,y, clamp(length(swi2(abd,x,y)) > 1.0f ? normz(swi2(abd,x,y)) : swi2(abd,x,y), -1.0f, 1.0f));
        fragColor = to_float4_aw(abd, 0.0f);
    }

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


// Visualization of the system in Buffer A

__KERNEL__ void RayMarchingExperiment71JipiFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    fragCoord+=0.5f;
float BBBBBBBBBBBBBBBBBB;
    float2 texel = 1.0f / iResolution;
    float2 uv = fragCoord / iResolution;
    float3 c = swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z);
    float3 norm = normalize(c);
    
    float3 div = to_float3_s(0.1f) * norm.z;    
    float3 rbcol = 0.5f + 0.6f * cross(swi3(norm,x,y,z), to_float3(0.5f, -0.4f, 0.5f));
    
    fragColor = to_float4_aw(rbcol + div, 0.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: RGBA Noise Small' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'Cubemap: Forest Blurred_0' to iChannel2


// Created by Stephane Cuillerdier - @Aiekick/2017
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.

//float dispSize;

__DEVICE__ float3 effect(float2 p, __TEXTURE2D__ iChannel1) 
{
    return swi3(texture(iChannel1, (p+16.0f)*0.02f),x,y,z);
}


__DEVICE__ float4 displacement(float3 p, float dispSize, __TEXTURE2D__ iChannel1)
{
    float2 g = swi2(p,x,z);
    float3 col = 1.0f-clamp(effect(g*5.0f, iChannel1),0.0f,1.0f);
    //vec3 music = texture(iChannel2, to_float2( 0.15f, 0.25f )).rgb*2.0f;
    float dist = dot(col,to_float3_s(dispSize));
    return to_float4(dist,col.x,col.y,col.z);
}

__DEVICE__ float4 map(float3 p, float dispSize, __TEXTURE2D__ iChannel1)
{
    float4 disp = displacement(p, dispSize, iChannel1);
    return to_float4(length(p) - 4.0f - (disp.x), disp.y,disp.z,disp.w);
}

__DEVICE__ float3 calcNormal( in float3 pos, float dispSize, __TEXTURE2D__ iChannel1 )
{
  float3 eps = to_float3( 0.03f, 0.0f, 0.0f );
  float3 nor = to_float3(
      map(pos+swi3(eps,x,y,y), dispSize, iChannel1).x - map(pos-swi3(eps,x,y,y), dispSize, iChannel1).x,
      map(pos+swi3(eps,y,x,y), dispSize, iChannel1).x - map(pos-swi3(eps,y,x,y), dispSize, iChannel1).x,
      map(pos+swi3(eps,y,y,x), dispSize, iChannel1).x - map(pos-swi3(eps,y,y,x), dispSize, iChannel1).x );
  return normalize(nor);
}

__DEVICE__ float march(float3 ro, float3 rd, float rmPrec, float maxd, float mapPrec, float dispSize, __TEXTURE2D__ iChannel1)
{
  
    float s = rmPrec;
    float d = 0.0f;
    for(int i=0;i<180;i++)
    {      
        if (d/s>1e5||d>maxd) break;
        s = map(ro+rd*d, dispSize, iChannel1).x;
        d += s*0.2f;
    }
    return d;
}


__KERNEL__ void RayMarchingExperiment71JipiFuse(float4 f, float2 g, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    CONNECT_COLOR0(Color, 1.0f, 0.8f, 0.2f, 1.0f); 
  
    CONNECT_SLIDER1(refmul, -1.0f, 10.0f, 1.0f); 
    CONNECT_SLIDER2(refoff, -1.0f, 10.0f, 0.0f);
    
    CONNECT_POINT0(BKG, 0.0f, 0.0f);
    CONNECT_SLIDER3(BKGZ, -1.0f, 10.0f, 0.0f);
  
    g+=0.5f;
    
    float dispSize = 0.15f;
    
    float time = iTime*0.25f;
    float cam_a = time; 

    float cam_e = 5.52f; 
    float cam_d = 1.88f; 
    
    float2 s = iResolution;
    float2 uv = (g+g-s)/s.y;
    
    if (iMouse.z > 0.0f)
        dispSize = iMouse.x / s.x * 2.0f - 1.0f;
    
    
    float3 col = to_float3_s(0.0f);
    
    float3 ro = to_float3(-_sinf(cam_a)*cam_d, cam_e+1.0f, _cosf(cam_a)*cam_d); //
    float3 rov = normalize(-ro);
    float3 u = normalize(cross(to_float3(0,1,0),rov));
    float3 v = cross(rov,u);
    float3 rd = normalize(rov + uv.x*u + uv.y*v);
    
    float b = 0.35f;
    
    float d = march(ro, rd, 1e-5, 50.0f, 0.5f, dispSize, iChannel1);
    
    rd+= to_float3_aw(BKG,BKGZ);
    
    if (d<50.0f)
    {
        float2 e = to_float2(-1.0f, 1.0f)*0.005f; 
        float3 p = ro+rd*d;
        float3 n = calcNormal(p, dispSize, iChannel1);
        
        float3 reflRay = reflect(rd, n);
        float3 refrRay = _refract_f3(rd, n, 0.7f, refmul, refoff);
        
        float3 cubeRefl = swi3(decube_f3(iChannel2,reflRay),x,y,z) * 0.5f;
        float3 cubeRefr = swi3(decube_f3(iChannel2,refrRay),x,y,z) * 0.8f;
        
        col = cubeRefl + cubeRefr + _powf(b, 15.0f);
        
        float3  lig = normalize( to_float3(-0.6f, 0.7f, -0.5f) );
        float dif = clamp( dot( n, lig ), 0.0f, 1.0f );
        float spe = _powf(clamp( dot( reflRay, lig ), 0.0f, 1.0f ),16.0f);

        float3 brdf = to_float3_s(0);
        brdf += 1.2f*dif*to_float3_s(1);
        //brdf += 0.5f*spe*to_float3(1,0.8f,0.2f)*dif;
        brdf += 0.5f*spe*swi3(Color,x,y,z)*dif;
    
        col = _mix(brdf, swi3(map(p, dispSize, iChannel1),y,z,w), 0.5f);
    }
    else
    {
        col = swi3(decube_f3(iChannel2,rd),x,y,z);
    }
    
  f = to_float4_aw(col, Color.w);


  SetFragmentShaderComputedColor(f);
}