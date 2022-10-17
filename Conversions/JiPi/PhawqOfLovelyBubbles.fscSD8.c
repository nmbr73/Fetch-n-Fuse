
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define iFeedbackColorShiftZoom 0.058f
//#define iFeedbackColorShiftImpact 0.001f
#define iBlob1ColorPulseSpeed 0.03456f
#define iBlob2ColorPulseSpeed -0.02345f
#define Margins 0.0f





/*
 * Conway Ticket
 * 
 * Copyright (C) 2021  Alexander Kraus <nr4@z10.info>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifdef MUELL 
int2 boardSize = to_int2(125),
    ci = to_int2(1,0);
float2 blockSize,
    xij;
float3 c = to_float3(1,0,-1);
float stepTimeDelta = 0.05f,
    fsaa = 144.0f,
    bpm = 90.0f,
    spb = 60.0f/90.0f,
    scale,
    nbeats,
    stepTime;
#endif

#define pi 3.14159f

// Creative Commons Attribution-ShareAlike 4.0f International Public License
// Created by David Hoskins.
// See https://www.shadertoy.com/view/4djSRW
__DEVICE__ float hash12(float2 p)
{

  float3 p3  = fract_f3(swi3(p,x,y,x) * 0.1031f);
  p3 += dot(p3, swi3(p3,y,z,x) + 33.33f);
  return fract((p3.x + p3.y) * p3.z);
}

__DEVICE__ float lfnoise(float y)
{

	
	float3 c = to_float3(1,0,-1);
  float2 t = y*swi2(c,x,x);
  float2 i = _floor(t);
  t = smoothstep(swi2(c,y,y), swi2(c,x,x), fract(t));
  float2 v1 = to_float2(hash12(i), hash12(i+swi2(c,x,y))),
  v2 = to_float2(hash12(i+swi2(c,y,x)), hash12(i+swi2(c,x,x)));
  v1 = swi2(c,z,z)+2.0f*_mix(v1, v2, t.y);
  return _mix(v1.x, v1.y, t.x);
}

__DEVICE__ float m(float2 x)
{
  return _fmaxf(x.x,x.y);
}

__DEVICE__ float d210(float2 x)
{
  return _fminf(_fmaxf(_fmaxf(_fmaxf(_fmaxf(_fminf(_fmaxf(_fmaxf(m(abs_f2(to_float2(_fabs(_fabs(x.x)-0.25f)-0.25f, x.y))-to_float2_s(0.2f)),
																-m(abs_f2(to_float2(x.x+0.5f, _fabs(_fabs(x.y)-0.05f)-0.05f))-to_float2(0.12f,0.02f))), 
																-m(abs_f2(to_float2(_fabs(x.x+0.5f)-0.1f, x.y-0.05f*sign_f(x.x+0.5f)))-to_float2(0.02f,0.07f))), 
																 m(abs_f2(to_float2(x.x+0.5f,x.y+0.1f))-to_float2(0.08f,0.04f))), 
																-m(abs_f2(to_float2(x.x, x.y-0.04f))-to_float2(0.02f, 0.08f))), 
																-m(abs_f2(to_float2(x.x, x.y+0.1f))-to_float2_s(0.02f))), 
																-m(abs_f2(to_float2(x.x-0.5f, x.y))-to_float2(0.08f,0.12f))), 
																-m(abs_f2(to_float2(x.x-0.5f, x.y-0.05f))-to_float2(0.12f, 0.07f))), 
																 m(abs_f2(to_float2(x.x-0.5f, x.y))-to_float2(0.02f, 0.08f)));
}

__DEVICE__ float dbox3(float3 x, float3 b)
{
  b = abs_f3(x) - b;
  return length(_fmaxf(b,to_float3_s(0.0f)))
              + _fminf(_fmaxf(b.x,_fmaxf(b.y,b.z)),0.0f);
}

__DEVICE__ float setStateF(int2 index, int2 where, float oldState, float newState)
{
  //return all(equal(index, where)) ? newState : oldState;
	return (index.x == where.x && index.y == where.y) ? newState : oldState;
}

// Distance to star
__DEVICE__ float dstar(float2 x, float N, float2 R)
{
    float3 c = to_float3(1,0,-1);
    float  d = pi/N,
          p0 = _acosf(x.x/length(x)),
           p = mod_f(p0, d);
    float2 a = _mix(R,swi2(R,y,x),mod_f(round((p-p0)/d),2.0f)),
          p1 = a.x*swi2(c,x,y),
          ff = a.y*to_float2(_cosf(d),_sinf(d))-p1;
    return dot(length(x)*to_float2(_cosf(p),_sinf(p))-p1,swi2(ff,y,x)*swi2(c,z,x))/length(ff);
}

__DEVICE__ float dhexagonpattern(float2 p) 
{
    float2 q = to_float2(p.x*1.2f, p.y + p.x*0.6f),
         qi = _floor(q),
         pf = fract_f2(q);
    float v = mod_f(qi.x + qi.y, 3.0f);
    
    return dot(step(swi2(pf,x,y),swi2(pf,y,x)), 1.0f-swi2(pf,y,x) + step(1.0f,v)*(pf.x+pf.y-1.0f) + step(2.0f,v)*(swi2(pf,y,x)-2.0f*swi2(pf,x,y)));
}

// x: material
// y: distance
// z: reflectivity
__DEVICE__ float3 add(float3 a, float3 b)
{
	
  if(a.y < b.y) return a;
  return b;
}

__DEVICE__ float3 hsv2rgb(float3 cc)
{
	
  float4 K = to_float4(1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);
  float3 p = abs_f3(fract_f3(swi3(cc,x,x,x) + swi3(K,x,y,z)) * 6.0f - swi3(K,w,w,w));
  //return cc.z * _mix(swi3(K,x,x,x), clamp(p - swi3(K,x,x,x), 0.0f, 1.0f), cc.y);
	return cc.z * _mix(swi3(K,x,x,x), clamp(p - swi3(K,x,x,x), to_float3_s(0.0f), to_float3_s(1.0f)), cc.y);
}

__DEVICE__ float2 rgb2sv(float3 cc)
{
    float4 K = to_float4(0.0f, -1.0f / 3.0f, 2.0f / 3.0f, -1.0f),
        p = _mix(to_float4_f2f2(swi2(cc,z,y), swi2(K,w,z)), to_float4_f2f2(swi2(cc,y,z), swi2(K,x,y)), step(cc.z, cc.y)),
        q = _mix(to_float4_aw(swi3(p,x,y,w), cc.x), to_float4(cc.x, p.y,p.z,p.x), step(p.x, cc.x));
    return to_float2((q.x - _fminf(q.w, q.y)) / (q.x + 1.e-10f), q.x);
}



//#define pi _acosf(-1.0f)


#define sint(a) (asin(_sinf(a))*2.0f - 1.0f)

#define rot(a) to_mat2(_cosf(a),-_sinf(a),_sinf(a),_cosf(a))

//#define pmod(p,d) mod_f(p - (d)*0.5f, (d)) - 0.5f*(d)

__DEVICE__ float r11(float i){ return fract(_sinf(i*12.126f)*12.6f);}

#define xor(a,b,c) _fminf(max((a),-(b)), _fmaxf((b),-(a) - c)) 

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



__DEVICE__ float __saturatef (float x)
{
    return _fminf(1.0f, _fmaxf(0.0f,x));
}
__DEVICE__ float3 __saturate_f3 (float3 x)
{
    return _fminf(to_float3(1.0f,1.0f,1.0f), _fmaxf(to_float3(0.0f,0.0f,0.0f),x));
}

// --- Spectral Zucconi --------------------------------------------
// By Alan Zucconi
// Based on GPU Gems: https://developer.nvidia.com/sites/all/modules/custom/gpugems/books/GPUGems/gpugems_ch08.html
// But with values optimised to match as close as possible the visible spectrum
// Fits this: https://commons.wikimedia.org/wiki/File:Linear_visible_spectrum.svg
// With weighter MSE (RGB weights: 0.3f, 0.59f, 0.11f)
__DEVICE__ float3 bump3y (float3 x, float3 yoffset)
{
  float3 y = to_float3(1.0f,1.0f,1.0f) - x * x;
  y = __saturate_f3(y-yoffset);
  return y;
}

// --- Spectral Zucconi 6 --------------------------------------------

// Based on GPU Gems
// Optimised by Alan Zucconi
__DEVICE__ float3 spectral_zucconi6 (float x)
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
    bump3y(c1 * (x - x1), y1) +
    bump3y(c2 * (x - x2), y2) ;
}

// Use a lookup texture in Buffer A for thin film interference instead of calculating it at every intersection
#define USE_THIN_FILM_LOOKUP 0



#define PI  3.141592654f

// used to prevent loop unrolling
// This will be zero but the compiler doesn't know that as iFrame is a uniform
#define ZERO 0 //_fminf(iFrame,0)

// https://en.wikipedia.org/wiki/Fresnel_equations
__DEVICE__ float FresnelS(float ni, float nt, float cosi, float cost)
{
    return ((nt * cosi) - (ni * cost)) / ((nt * cosi) + (ni * cost));
}

__DEVICE__ float FresnelP(float ni, float nt, float cosi, float cost)
{
    return ((ni * cosi) - (nt * cost)) / ((ni * cosi) + (nt * cost));
}

__DEVICE__ float Fresnel(float ni, float nt, float cosi, float cost )
{    
    float Rs = FresnelS( ni, nt, cosi, cost );
    float Rp = FresnelP( ni, nt, cosi, cost );

    return (Rs * Rs + Rp * Rp) * 0.5f;
}

__DEVICE__ float FresnelR0(float ni, float nt)
{
    float R0 = (ni-nt) / (ni+nt);
    R0 *= R0;
    return R0;
}

// https://en.wikipedia.org/wiki/Snell%27s_law
__DEVICE__ float GetCosT( float ni, float nt, float cosi )
{
    float n = ni/nt;
    float sinT2 = n*n*(1.0f-cosi*cosi);
    
    // Total internal reflection
    if (sinT2 >= 1.0f)
    {
        return 1.0f;
    } 

    float cost = _sqrtf(1.0f - sinT2);
    return cost;
}


// https://www.shadertoy.com/view/4djSRW
__DEVICE__ float hash13(float3 p3)
{
  p3  = fract_f3(p3 * 0.1031f);
  p3 += dot(p3, swi3(p3,z,y,x) + 31.32f);
  return fract((p3.x + p3.y) * p3.z);
}

__DEVICE__ float SmoothNoise3d(float3 p)
{
    float3 fl = _floor(p);
    float3 fr = p - fl;
    
    float3 ot = fr*fr*(3.0f-2.0f*fr);
    float3 zt = 1.0f - ot;
    
    
    float result = 0.0f;
    
    result += hash13(fl + to_float3(0,0,0)) * (zt.x * zt.y * zt.z);
    result += hash13(fl + to_float3(1,0,0)) * (ot.x * zt.y * zt.z);

    result += hash13(fl + to_float3(0,1,0)) * (zt.x * ot.y * zt.z);
    result += hash13(fl + to_float3(1,1,0)) * (ot.x * ot.y * zt.z);

    result += hash13(fl + to_float3(0,0,1)) * (zt.x * zt.y * ot.z);
    result += hash13(fl + to_float3(1,0,1)) * (ot.x * zt.y * ot.z);

    result += hash13(fl + to_float3(0,1,1)) * (zt.x * ot.y * ot.z);
    result += hash13(fl + to_float3(1,1,1)) * (ot.x * ot.y * ot.z);

    return result;
}



__DEVICE__ float Noise(float3 p, float o)
{
	const mat3 m3 = to_mat3( 0.00f,  0.80f,  0.60f,
          -0.80f,  0.36f, -0.48f,
          -0.60f, -0.48f,  0.64f );
	
    float result = 0.0f;
    float a = 1.0f;
    float t= 0.0f;
    float f = 0.5f;
    float s= 2.0f;
    
    p.x += o;
    result += SmoothNoise3d(p) * a; t+= a; p = mul_mat3_f3(m3 , p) * s; a = a * f;
    p.x += o;
    result += SmoothNoise3d(p) * a; t+= a; p = mul_mat3_f3(m3 , p) * s; a = a * f;
    p.x += o;
    result += SmoothNoise3d(p) * a; t+= a; p = mul_mat3_f3(m3 , p) * s; a = a * f;
    p.x += o;
    result += SmoothNoise3d(p) * a; t+= a; p = mul_mat3_f3(m3 , p) * s; a = a * f;
    result = result / t;
    
    return result;
}



// Spectrum to xyz approx function from http://jcgt.org/published/0002/02/01/paper.pdf
// Inputs:  Wavelength in nanometers
__DEVICE__ float xFit_1931( float wave )
{
    float t1 = (wave-442.0f)*((wave<442.0f)?0.0624:0.0374f),
          t2 = (wave-599.8f)*((wave<599.8f)?0.0264:0.0323f),
          t3 = (wave-501.1f)*((wave<501.1f)?0.0490:0.0382f);
    return 0.362f*_expf(-0.5f*t1*t1) + 1.056f*_expf(-0.5f*t2*t2)- 0.065f*_expf(-0.5f*t3*t3);
}
__DEVICE__ float yFit_1931( float wave )
{
    float t1 = (wave-568.8f)*((wave<568.8f)?0.0213:0.0247f),
          t2 = (wave-530.9f)*((wave<530.9f)?0.0613:0.0322f);
    return 0.821f*_expf(-0.5f*t1*t1) + 0.286f*_expf(-0.5f*t2*t2);
}
__DEVICE__ float zFit_1931( float wave )
{
    float t1 = (wave-437.0f)*((wave<437.0f)?0.0845:0.0278f),
          t2 = (wave-459.0f)*((wave<459.0f)?0.0385:0.0725f);
    return 1.217f*_expf(-0.5f*t1*t1) + 0.681f*_expf(-0.5f*t2*t2);
}

#define xyzFit_1931(w) to_float3( xFit_1931(w), yFit_1931(w), zFit_1931(w) ) 

__DEVICE__ float3 XYZtosRGB( float3 XYZ )
{
    // XYZ to sRGB
    // http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
   mat3 m = to_mat3 (
					 3.2404542f, -1.5371385f, -0.4985314f,
					-0.9692660f,  1.8760108f,  0.0415560f,
					 0.0556434f, -0.2040259f,  1.0572252f );
    
    return mul_f3_mat3(XYZ , m);
}

__DEVICE__ float3 WavelengthToXYZ( float f )
{    
    return xyzFit_1931( f );    
}



// from  https://github.com/amandaghassaei/SoapFlow/blob/main/python/Thin%20Film%20Interference.ipynb
__DEVICE__ float ThinFilmAmplitude( float wavelength, float thickness, float cosi, float N_Air, float N_Water )
{

    float ni = N_Air;
    float nt = N_Water;
 
    float cost = GetCosT( ni, nt, cosi );

    // # The wavelength inside a medium is scaled by the index of refraction.
    // wavelength_soap = wavelength / n_soap
    // wavelength_air = wavelength / n_air
    // # First calc phase shift of reflection at rear surface, based on film thickness.
    // phaseDelta = 2 * thickness / math._cosf(theta) * 2 * math.pi / wavelength_soap  
    // # There is an additional path to compute, the segment AJ from:
    // # https://www.glassner.com/wp-content/uploads/2014/04/CG-CGA-PDF-00-11-Soap-Bubbles-2-Nov00.pdf
    // phaseDelta -= 2 * thickness * math._tanf(theta) * math._sinf(incidentAngle) * 2 * math.pi / wavelength_air
    // Simplified to:
    float phaseDelta = 2.0f * thickness * nt * cost * 2.0f * PI / wavelength;
    
    // https://en.wikipedia.org/wiki/Reflection_phase_change
    if (ni < nt)
        phaseDelta -= PI;
    if (ni > nt)
        phaseDelta += PI;

    float front_refl_amp = Fresnel(cosi, cost, ni, nt);
    float front_trans_amp = 1.0f - front_refl_amp;
    float rear_refl_amp = front_trans_amp * Fresnel(cost, cosi, nt, ni);
    
    rear_refl_amp /= front_refl_amp;
    front_refl_amp = 1.0f;
        
    // http://scipp.ucsc.edu/~haber/ph5B/addsine.pdf
    return _sqrtf(front_refl_amp * front_refl_amp + rear_refl_amp * rear_refl_amp + 2.0f * front_refl_amp * rear_refl_amp * _cosf(phaseDelta));
}

__DEVICE__ float3 GetThinFilmColour( float cosi, float thicknessN, float N_Air, float N_Water )
{
    float thicknessMin = 100.0f;//1.0f;
    float thicknessMax = 1500.0f;//2500.0f;
    
    float thickness = _mix(thicknessMin, thicknessMax, thicknessN);

    float3 result = to_float3_s(0.0f);
    
    float t = 0.0f;
    
    float3 white = to_float3_s(0.0f);
    
    for (float wavelength = 380.0f; wavelength<=780.0f; wavelength += 50.0f)
    {
        float amplitude = ThinFilmAmplitude( wavelength, thickness, cosi, N_Air, N_Water );
        
        float3 XYZ = WavelengthToXYZ( wavelength );
    
        white += XYZ;
    
        result += XYZ * amplitude;
        t += 1.0f;
    }

    result = XYZtosRGB( result );
      
    result /= t;
    //result /= white;
    //result = to_float3_s(1.0f);
    
    return result;
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Cubemap: St Peters Basilica_0' to iChannel1
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0




__KERNEL__ void PhawqOfLovelyBubblesFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel0)
{

fragCoord+=0.5f;

#if USE_THIN_FILM_LOOKUP
    {
		float N_Air = 1.0f;
        float N_Water = 1.33f;
        int segmentCount = 32;
        int segment = iFrame % segmentCount;
        int currSegment = (int)(_floor((fragCoord.y * (float)(segmentCount) / iResolution.y)));
        
        if ( segment != currSegment )
        {
          //fragColor = texelFetch( iChannel0, to_int2_cfloat(fragCoord), 0 );
			    fragColor = texture( iChannel0, (to_float2(to_int2_cfloat(fragCoord))+0.5f)/iResolution );
			    SetFragmentShaderComputedColor(fragColor);
          return;
        }
    }
float AAAAAAAAAAAAAAAA;
    float2 uv = fragCoord/iResolution;
        
    float3 result = GetThinFilmColour(uv.x, uv.y, N_Air,N_Water);  

    fragColor = to_float4_aw(result,1.0f);
#else
	
    //discard;
#endif


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Cubemap: Textur' to iChannel2
// Connect Buffer B 'Cubemap: Uffizi Gallery_0' to iChannel1
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


// Lovely Bubbles
// by @P_Malin
// https://www.shadertoy.com/view/Nl2SRc
//
// Some lovely shadertoy bubbles.
// I've wanted to implement something with thin film interference for a while.


// CAMERA

__DEVICE__ float2 GetWindowCoord( float2 uv, float2 iResolution )
{
  float2 window = uv * 2.0f - 1.0f;
  window.x *= iResolution.x / iResolution.y;

  return window;  
}

__DEVICE__ float3 GetCameraRayDir( float2 window, float3 cameraPos, float3 cameraTarget, float fov )
{
  float3 forward = normalize( cameraTarget - cameraPos );
  float3 right = normalize( cross( to_float3(0.0f, 1.0f, 0.0f), forward ) );
  float3 up = normalize( cross( forward, right ) );
                
  float3 dir = normalize(window.x * right + window.y * up + forward * fov);

  return dir;
}


// POSTFX

__DEVICE__ float Vignette( float2 uv, float size )
{
    float d = length( (uv - 0.5f) * 2.0f ) / length(to_float2_s(1.0f));
    
    d /= size;

    float s = d * d * ( 3.0f - 2.0f * d );
    
    float v = _mix ( d, s, 0.6f );
    
    return _fmaxf(0.0f, 1.0f - v);
}

__DEVICE__ float3 ApplyTonemap( float3 linearCol )
{
  const float kExposure = 0.5f;
  
    float a = 0.010f;
    float b = 0.132f;
    float c = 0.010f;
    float d = 0.163f;
    float e = 0.101f;

    float3 x = linearCol * kExposure;

    return ( x * ( a * x + b ) ) / ( x * ( c * x + d ) + e );    
}

__DEVICE__ float3 ApplyGamma( float3 linearCol )
{
  const float kGamma = 2.2f;

  return pow_f3( linearCol, to_float3_s(1.0f/kGamma) );  
}

__DEVICE__ float3 ApplyPostFX( float2 uv, float3 col )
{    
  col *= 1.3f;

  col *= 0.1f + 0.9f * Vignette( uv, 1.0f );

  col *= to_float3(1.0f, 0.95f, 0.8f); // warmer
  
  col = ApplyTonemap(col);
  col = ApplyGamma(col);
    
  return col;
}
  


// Scene

//float speed = 1.0f;

__DEVICE__ float BubbleOriginForward( float t, float speed, float iTime )
{
    t = t * 30.0f;
    if ( t > 0.0f)
    {
        t = t / (1.0f+t/10.0f);

    }
    return t + iTime * speed;
}

__DEVICE__ float BubbleOriginInverse( float r, float speed, float iTime )
{
    r = r- iTime * speed;
    if( r > 0.0f)
    {
        r = -10.0f * r / (r - 10.0f);
    }
    r = r / 30.0f;
    return r;
}

__DEVICE__ float Scene_Distance(float3 pos, float speed, float iTime)
{

    float3 vPos = pos;
    vPos.x += 3.0f;

    float scale = 50.0f;
    
    vPos /= scale;

    // wobble
    float3 offset = to_float3_s(0);
    offset += sin_f3( swi3(pos,y,z,x) * 8.91f + iTime * 10.0f ) * 0.001f;
    offset += sin_f3( swi3(pos,z,x,y) * 7.89f + iTime * 10.0f ) * 0.001f;    
    offset *= 0.08f;
    
    float f = BubbleOriginForward( vPos.x, speed, iTime );
    
    f = _floor(f);
    
    float minD = 1000000.0f;
    
    for (float b=-1.0f; b<=2.0f; b+=1.0f)
    {
        float p = f + b;
        float3 o = vPos;
        o.x = BubbleOriginInverse( p, speed,iTime );
                
        o.x -= vPos.x;

         float spreadBlend = 1.0f - clamp( vPos.x * 3.0f + 0.2f, 0.0f, 1.0f);
         
         float spread = spreadBlend;
         
         spread *= 0.05f;

         o.y += _sinf(p * 123.3456f) * spread;
         o.z += _sinf(p * 234.5678f) * spread;
         
         o += offset;
           
         float rad = _sinf( p * 456.8342f ) * 0.5f + 0.5f;
                             
         float d = length(o) - 0.005f - rad * rad * 0.02f;
         
         minD = _fminf( minD, d );
    }
    
     return minD * scale;
}

union A2F
 {
   float4  F;    //32bit float
   float  A[4];  //32bit unsigend integer
 };

__DEVICE__ float3 Scene_GetNormal( float3 pos, float speed, float iTime )
{
    const float delta = 0.0001f;
    
    A2F samples; //float4 samples;
    for( int i=ZERO; i<=4; i++ )
    {
        //float4 offset = to_float4_s(0);
        A2F offset;
        offset.F = to_float4_s(0);
        offset.A[i] = delta;
        samples.A[i] = Scene_Distance( pos + swi3(offset.F,x,y,z), speed, iTime );
    }
    
    float3 normal = swi3((samples.F),x,y,z) - swi3((samples.F),w,w,w);    
    return normalize( normal );
}    

__DEVICE__ float Scene_Trace( float3 rayOrigin, float3 rayDir, float minDist, float maxDist, float side, float speed, float iTime )
{
  float t = minDist;

  const int kRaymarchMaxIter = 128;
  for(int i=0; i<kRaymarchMaxIter; i++)
  {    
    float epsilon = 0.0001f * t;
    float d = Scene_Distance( rayOrigin + rayDir * t, speed, iTime ) * side;
    if ( _fabs(d) < epsilon )
    {
      break;
    }
        if ( t > maxDist )
        {
          t = maxDist + 1.0f;
          break;
        }       
        t += d;        
  }
  return t;
}

__DEVICE__ float3 GetSkyColour( float3 dir, __TEXTURE2D__ iChannel1 )
{
    float3 result = to_float3_s(0.0f);
  
    float3 envMap = swi3(decube_f3(iChannel1,dir),x,y,z);//.rgb;
    envMap = envMap * envMap;
    float kEnvmapExposure = 0.99999f;
    //result = -_log2f(1.0f - envMap * kEnvmapExposure);
    result = -1.0f*log2_f3(1.0f - envMap * kEnvmapExposure);

    return result;  
}

__DEVICE__ float FilmThickness( float3 pos, float iTime )
{
    return Noise(pos * 0.3f, iTime * 0.5f);
}

__DEVICE__ void Shade( inout float3 *colour, inout float3 *remaining, float3 pos, float3 rayDir, float3 normal, float N_Air, float N_Water, float iTime, __TEXTURE2D__ iChannel1 )
{
    float NdotV = _fmaxf( dot(normal, -rayDir), 0.0f );

    float filmThickness = FilmThickness(pos, iTime);

    float3 reflection = GetSkyColour( reflect( rayDir, normal ), iChannel1 );
    
#if 1
    // Extra highlight
    float3 LightColour = to_float3(1,0.9f,0.7f) * 0.8f;
    float3 L = normalize(to_float3(1.0f, 2.0f, 0.0f));
    float NdotL = _fmaxf( dot( normal, L ), 0.0f );
    float NdotH = _fmaxf( dot( normal, normalize(L-rayDir) ), 0.0f );
    reflection += (_powf(NdotH,10000.0f) * 10000.0f) * NdotL * LightColour;
    //vReflection += (_powf(NdotH,1000.0f) * 2000.0f) * NdotL * LightColour;
    reflection += (_powf(NdotH,100.0f) * 200.0f) * NdotL * LightColour;
    reflection += (_powf(NdotH,10.0f) * 20.0f) * NdotL * LightColour;
#endif     
     
    float ni = N_Air;
    float nt = N_Water;     
    
    float cosi = NdotV;
    float cost = GetCosT( ni, nt, cosi );
    float fresnelA = Fresnel( ni, nt, cosi, cost );
    float fresnelB = Fresnel( nt, ni, cost, cosi );

    float fresnelFactor = 1.0f - (1.0f - fresnelA) * (1.0f - fresnelB);
    
    float3 fresnel = to_float3_s(fresnelFactor);

    float3 thinFilmColour;
#if USE_THIN_FILM_LOOKUP
    thinFilmColour = swi3(texture(iChannel0, to_float2(NdotV, filmThickness) ),x,y,z);//.rgb;
#else
    thinFilmColour = GetThinFilmColour(NdotV, filmThickness, N_Air, N_Water);
#endif
    fresnel *= thinFilmColour;
    
    *colour += reflection * fresnel * *remaining;
    *remaining *= (1.0f - fresnel);


#if 0
    float fGlassThickness = 0.5f;
    float3 vGlassColor = to_float3(1,0.5f, 0.25f);

    float fOpticalDepth = fGlassThickness / NdotV;
    float3 vExtinction = _exp2f( -fOpticalDepth * (1.0f - vGlassColor) ); 
    *remaining *= vExtinction;
#endif    
}


__DEVICE__ float3 GetSceneColour( float3 rayOrigin, float3 rayDir, float iTime, float N_Air, float N_Water, __TEXTURE2D__ iChannel1, float speed, float3 colour )
{    

	  float kFarClip = 200.0f;

    //float3 colour = to_float3_s(0);
    float3 remaining = to_float3_s(1);
    
    float side = 1.0f;

    float minDist = 0.0f;
    
    for( int i=0; i<10; i++ )
    {
        float t = Scene_Trace( rayOrigin, rayDir, minDist, kFarClip, side, speed, iTime );
        
        if ( t>=kFarClip )
        {
            break;
        }
        
        minDist = t + 0.1f;
        
        float3 hitPos = rayOrigin + rayDir * t;

        float3 normal = Scene_GetNormal( hitPos, speed, iTime );

        Shade(&colour, &remaining, hitPos, rayDir, normal * side, N_Air, N_Water, iTime, iChannel1 );
        
        side = side * -1.0f;
    }
    
 //   colour += GetSkyColour(rayDir) * remaining; 
  
  return colour;
}

__KERNEL__ void PhawqOfLovelyBubblesFuse__Buffer_B(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
  fragCoord+=0.5f;        
 
  float N_Air = 1.0f;
  float N_Water = 1.33f;
  
  float speed = 1.0f;

  float2 uv = fragCoord / iResolution;

float3 colour = swi3(texture(iChannel2,uv),x,y,z); //to_float3_s(0);

  float heading = 0.3f + _sinf(iTime * 0.3f) * 0.1f;

  float elevation = 1.8f + _sinf(iTime * 0.134f) * 0.1f;

  float fov = 2.5f + _sinf( iTime * 0.234f) * 0.5f;

  float cameraDist = 10.0f;
  float3 cameraPos = to_float3(_sinf(heading) * _sinf(-elevation), _cosf(-elevation), _cosf(heading) * _sinf(-elevation)) * cameraDist;
  float3 cameraTarget = to_float3(_sinf(iTime * 0.1542f) * 3.0f, 0.0f, 0.0f);

  float3 rayOrigin = cameraPos;
  float3 rayDir = GetCameraRayDir( GetWindowCoord(uv, iResolution), cameraPos, cameraTarget, fov );
  
  float3 sceneCol = GetSceneColour( rayOrigin, rayDir, iTime, N_Air, N_Water, iChannel1, speed, colour );
  
  float3 final = ApplyPostFX( uv, sceneCol );
  
  fragColor = to_float4_aw(final, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1
// Connect Buffer C 'Previsualization: Buffer C' to iChannel2
// Connect Buffer C 'Previsualization: Buffer D' to iChannel3


                                                                                                                                                                                                                                                                                        // See Image tab for details, also visit:
//
// https://xemantic.github.io/shader-web-background/
//


/*
  Normally it would be provided by texture parameters, but on Mac/iOS the texture REPEAT
  seems to work only for power-of-2 texture sizes.
 */
__DEVICE__ float4 repeatedTexture(in __TEXTURE2D__ channel, in float2 uv) {
    return texture(channel, mod_f2(uv, 1.0f));
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


__KERNEL__ void PhawqOfLovelyBubblesFuse__Buffer_C(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{

    fragCoord+=0.5f;

	// In the original shader-web-background these values are provided as uniforms
	// feel free to play with them and if you will find something prettier than
	// the equilibrium I established, please send it back to me :)
	const float2  iFeedbackZoomCenter       = to_float2(0.0f, 0.0f);
	const float iFeedbackZoomRate         = 0.001f;
	const float iFeedbackFadeRate         = 0.999f;
	//const float iFeedbackColorShiftZoom   = 0.05f;
	const float iFeedbackColorShiftImpact = 0.0015f;
	const float2  iDrawCenter             = to_float2(0.0f, 0.0f);
	const float iDrawIntensity            = 2.5f;
	const float iBlobEdgeSmoothing        = 0.02f;
	const float iBlob1Radius              = 0.65f;
	const float iBlob1PowFactor           = 20.0f;
	//const float iBlob1ColorPulseSpeed     = 0.042f;
	const float iBlob2Radius              = 0.7f;
	const float iBlob2PowFactor           = 20.0f;
	//const float iBlob2ColorPulseSpeed     = 0.0234f;
	const float iBlob2ColorPulseShift     = 0.0f;
	const float iColorShiftOfRadius       = 0.05f;
	const float iFeedbackMouseShiftFactor = 0.003f;


    // in shader-web-background provided as uniforms: start
    float iMinDimension = _fminf(iResolution.x, iResolution.y);
    float2 iScreenRatioHalf =
        (iResolution.x >= iResolution.y)
            ? to_float2(iResolution.y / iResolution.x * 0.5f, 0.5f)
            : to_float2(0.5f, iResolution.x / iResolution.y);
    float3 iBlob1Color = spectral_zucconi6(
        mod_f(iTime * (iBlob1ColorPulseSpeed / -1.5f), 1.0f)
    );
    
    float3 iBlob2Color = spectral_zucconi6(
        mod_f(iTime * (iBlob2ColorPulseSpeed / -1.5f) + iBlob2ColorPulseShift, 1.0f)
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

    float3 feedbk = swi3(repeatedTexture(iChannel1, uv - st),x,y,z);//.rgb;
    float3 colorShift = swi3(repeatedTexture(
											iChannel0,
											uv - st * iFeedbackColorShiftZoom * iScreenRatioHalf
							                ),x,y,z);//.rgb;

    float2 stShift = to_float2_s(0);
    stShift += iFeedbackZoomRate * (st - iFeedbackZoomCenter);
    stShift += (swi2(feedbk,z,y)/swi2(colorShift,y,x) - 0.5f) * iFeedbackColorShiftImpact;
    stShift += iFeedbackShiftVector;
    stShift *= iScreenRatioHalf;

    float3 prevColor = swi3(repeatedTexture(iChannel2, uv - stShift),x,y,z);//.rgb;
    prevColor *= iFeedbackFadeRate;
    
    float3 prevColor2 = swi3(repeatedTexture(iChannel3, uv - stShift),x,y,z);//.rgb;
    prevColor2 *= iFeedbackFadeRate;
    
 //   prevColor = _mix(prevColor, prevColor2, 0.0995f);
    prevColor = _fminf(prevColor, prevColor2);

    float3 drawColor = to_float3_s(0);
   

    float radius =
        1.0f
        + (colorShift.x + colorShift.y + colorShift.z) * iColorShiftOfRadius;
    drawColor +=
        _powf(
          drawBlob(st, iDrawCenter, radius * iBlob1Radius, iBlobEdgeSmoothing, iBlobEdgeSmoothing),
          iBlob1PowFactor
        ) * iBlob1Color;
    drawColor +=
        _powf(
          drawBlob(st, iDrawCenter, radius * iBlob2Radius, iBlobEdgeSmoothing, iBlobEdgeSmoothing),
          iBlob2PowFactor
        ) * iBlob2Color;

    float3 color = to_float3_s(0);
    drawColor *= iDrawIntensity;
    prevColor *= iFeedbackFadeRate;
    color += prevColor;
    color += drawColor;

    color = clamp(color, 0.0f, 1.0f);
    fragColor = to_float4_aw(color, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer B' to iChannel0
// Connect Buffer D 'Previsualization: Buffer B' to iChannel2
// Connect Buffer D 'Previsualization: Buffer D' to iChannel1
// Connect Buffer D 'Previsualization: Buffer D' to iChannel3


// This buffer is the feedback loop

// iq noise

__DEVICE__ float hash( float n )
{
    return fract(_sinf(n)*43758.5453123f);
}

__DEVICE__ float noise( in float2 x )
{
    float2 p = _floor(x);
    float2 f = fract_f2(x);

    f = f*f*(3.0f-2.0f*f);

    float n = p.x + p.y*157.0f;

    return _mix(_mix( hash(n+  0.0f), hash(n+  1.0f),f.x),
                _mix( hash(n+157.0f), hash(n+158.0f),f.x),f.y);
}

// hue by netgrind(?)

__DEVICE__ float3 hue(float3 color, float shift) {

    const float3  kRGBToYPrime = to_float3 (0.299f, 0.587f, 0.114f);
    const float3  kRGBToI     = to_float3 (0.596f, -0.275f, -0.321f);
    const float3  kRGBToQ     = to_float3 (0.212f, -0.523f, 0.311f);

    const float3  kYIQToR   = to_float3 (1.0f, 0.956f, 0.621f);
    const float3  kYIQToG   = to_float3 (1.0f, -0.272f, -0.647f);
    const float3  kYIQToB   = to_float3 (1.0f, -1.107f, 1.704f);

    // Convert to YIQ
    float   YPrime  = dot (color, kRGBToYPrime);
    float   I      = dot (color, kRGBToI);
    float   Q      = dot (color, kRGBToQ);

    // Calculate the hue and chroma
    float   hue     = _atan2f (Q, I);
    float   chroma  = _sqrtf (I * I + Q * Q);

    // Make the user's adjustments
    hue += shift;

    // Convert back to YIQ
    Q = chroma * _sinf (hue);
    I = chroma * _cosf (hue);

    // Convert back to RGB
    float3    yIQ   = to_float3 (YPrime, I, Q);
    color.x = dot (yIQ, kYIQToR);
    color.y = dot (yIQ, kYIQToG);
    color.z = dot (yIQ, kYIQToB);

    return color;
}

__DEVICE__ float fractalNoise(float2 pos) {
  float n = 0.0f;
  float scale = 1.0f / 1.5f;
  for (int i = 0; i < 5; i += 1) {
    n += noise(pos) * scale;
    scale *= 0.5f;
    pos *= 2.0f;
  }
  return n;
}

__KERNEL__ void PhawqOfLovelyBubblesFuse__Buffer_D(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
	  fragCoord+=0.5f;

    float2 uv = fragCoord / iResolution;

    // Convert the uv's to polar coordinates to scale up  
    float2 polarUv = (uv * 2.0f - 1.0f);
    float angle = _atan2f(polarUv.y, polarUv.x);
    
    // Scale up the length of the vector by a noise function feeded by the angle and length of the vector
    float ll = length(polarUv)*0.5f - fractalNoise(to_float2(_sinf(angle*4.0f + iTime*2.0f) + length(uv)*10.0f, length(uv)*20.0f + _sinf(angle*4.0f)))*0.005f ;
    
    float3 base = swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z);//.rgb;
    
    // Convert the scaled coordinates back to cartesian
    float2 offs = to_float2(_cosf(angle)*ll + 0.5f, _sinf(angle)*ll + 0.5f);
    
    // sample the last texture with uv's slightly scaled up
    float3 overlay = swi3(texture(iChannel1, swi2(offs,x,y)),x,y,z);//.rgb;
    
    // Since the colors of the iChannel0 are monochrome, set a color channel to zero to do a hue shift
    //   base.z = 0.0f;
    
    // Apply a hue shift to the overlaid image so it cascades in the feedback loop
    overlay = hue(overlay, 0.5f);
    overlay += (base + base) * (overlay / 0.50f);
    
    // Additively blend the colors together
    float4 col = to_float4_aw(clamp(to_float3_s(0.0f),to_float3_s(1.0f),base + overlay*0.99f), 1.0f);
    
    // col *= (col + col) * (overlay,1.0f);
    //(overlay,1.0f);
    
    fragColor = col ;

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer B' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel2
// Connect Image 'Previsualization: Buffer D' to iChannel1


// Fork of "Lovely Bubbles Fawq" by xenn. https://shadertoy.com/view/fddSz7
// 2021-09-30 10:00:20

__KERNEL__ void PhawqOfLovelyBubblesFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
	  fragCoord+=0.5f;
    float2 uv = fragCoord / iResolution;

    float3 ob, nb, a;
    ob = swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z);//.rgb;
    nb = swi3(_tex2DVecN(iChannel1,uv.x,uv.y,15),x,y,z);//.rgb;
    float3 xb = swi3(_tex2DVecN(iChannel2,uv.x,uv.y,15),x,y,z);//.rgb;
    xb += (xb * xb) - xb;
 
    //a = col * len
    a = to_float3(0.5f * (_fabs(_sinf(iTime / 3.0f))), 0.25f, 0.5f * (1.1f * (_fabs(_cosf(iTime / 3.0f))))) * 0.5f;

    //fragColor = min(vec4(mix(ob, nb, (a / 0.50)), 1.),(xb,1.));

    fragColor = _fminf(to_float4_aw(mix_f3(ob, nb, (a / 0.50f)), 1.0f), to_float4_aw(xb,1.0f));

    //fragColor = to_float4_aw(xb,1.0f); // Test

  SetFragmentShaderComputedColor(fragColor);
}