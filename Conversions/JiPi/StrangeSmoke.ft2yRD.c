
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)
#define R iResolution


__DEVICE__ float4 Blending( __TEXTURE2D__ channel, float2 uv, float4 Q, float Blend, float2 Par, float2 MulOff, int Modus, float2 U, float2 R)
{
   
    if (Blend > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(channel,uv);

      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2)
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
          //swi3S(Q,x,y,w, _mix(swi3(Q,x,y,w),(swi3(tex,x,y,z)+MulOff.y)*MulOff.x,Blend));

        if ((int)Modus&4)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par , Blend));
          //swi2S(Q,x,y, _mix( swi2(Q,x,y),  Par, Blend));
          //swi3S(Q,x,y,z, _mix(swi3(Q,x,y,z), (swi3(tex,x,y,z)+MulOff.y)*MulOff.x, Blend));  
          Q = _mix(Q,to_float4(Par.x,Par.y,(tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x),Blend);
        
        
        if ((int)Modus&8)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par, Blend));
          Q = _mix(Q,to_float4((tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x,Par.x,Par.y),Blend);
          //Q.z = _mix( Q.z,  (tex.x+MulOff.y)*MulOff.x, Blend);
          //swi2S(Q,z,w, _mix( swi2(Q,z,w), swi2(tex,x,y)*Par, Blend));

        if ((int)Modus&16) 
          //swi2S(Q,z,w, _mix(swi2(Q,z,w),  swi2(tex,x,y)*Par, Blend));
          Q = _mix(Q,to_float4(tex.x*Par.x,tex.y*Par.y,tex.z+MulOff.x,tex.w*MulOff.y),Blend);
      }
      else
        if ((int)Modus&32) //Special
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
    }
  
  return Q;
}


//#define iFeedbackColorShiftZoom 0.2
//#define iFeedbackColorShiftImpact 0.001
//#define iBlob1ColorPulseSpeed 0.03456
//#define iBlob2ColorPulseSpeed 0.02345
#define Margins 0.0f

#define PI 3.14159265359f

__DEVICE__ float2 getFontSymbolSampleUV(int x, int y, float2 uv) {
  return uv * to_float2_s(1.0f/16.0f) + to_float2((float)(x) / 16.0f, (float)(y) / 16.0f);
}

__DEVICE__ float getHeight(float2 uv, __TEXTURE2D__ bumpMap, float maxHeight) {
    return _tex2DVecN(bumpMap,uv.x,uv.y,15).x * maxHeight;
}

__DEVICE__ float3 getSlope(float2 uv, float height, float2 axis, __TEXTURE2D__ bumpMap, float maxHeight, float2 texelSize) {
    float h1 = getHeight(uv+texelSize*axis, bumpMap, maxHeight);
    float h2 = getHeight(uv-texelSize*axis, bumpMap, maxHeight);
    return to_float3(1,((height-h2) + (h1-height)) / 2.0f,0);
}

__DEVICE__ float3 bump2Normal(float2 uv, __TEXTURE2D__ bumpMap, float maxHeight, float2 texelSize) {
    
    float height = getHeight(uv, bumpMap, maxHeight);
        
    float3 slopeX = swi3(getSlope(uv, height, to_float2(1.0f, 0.0f), bumpMap, maxHeight, texelSize),z,y,x);
    float3 slopeY = swi3(getSlope(uv, height, to_float2(0.0f, 1.0f), bumpMap, maxHeight, texelSize),x,y,z);
    return swi3(cross(slopeX, slopeY),z,y,x);
}

__DEVICE__ float3 normal2rgb(float3 normal) {
    return normalize(swi3(normal,x,z,y) * to_float3(0.5f, 0.5f, 0.5f) + to_float3(0.5f, 0.5f, 0.5f));
}

__DEVICE__ float sinWave(float v, float m){return (_sinf(v*m*PI*2.0f)+1.0f)/2.0f;}

//from https://github.com/hughsk/glsl-hsv2rgb
__DEVICE__ float3 hsv2rgb(float3 c) {
    float4 K = to_float4(1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);
    float3 p = abs_f3(fract_f3(swi3(c,x,x,x) + swi3(K,x,y,z)) * 6.0f - swi3(K,w,w,w));
    return c.z * _mix(swi3(K,x,x,x), clamp(p - swi3(K,x,x,x), 0.0f, 1.0f), c.y);
}

__DEVICE__ float4 xy2hue(float2 v){
    float h = (_atan2f(v.y, v.x)/(PI) + 1.0f)/2.0f;
    float b = length(v);
    return to_float4_aw(hsv2rgb(to_float3(h, 1.0f, b)), 1.0f);
}

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
    return _fminf(_fmaxf(_fmaxf(_fmaxf(_fmaxf(_fminf(_fmaxf(_fmaxf(m(abs_f2(to_float2(_fabs(abs(x.x)-0.25f)-0.25f, x.y))-to_float2_s(0.2f)), 
                                                                  -m(abs_f2(to_float2(x.x+0.5f, _fabs(abs(x.y)-0.05f)-0.05f))-to_float2(0.12f,0.02f))), 
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
    return (index.x == where.x && index.y == where.y ) ? newState : oldState;
}

// Distance to star
__DEVICE__ float dstar(float2 x, float N, float2 R)
{
    float3 c = to_float3(1,0,-1);
    float  d = PI/N,
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

//vec3 hsv2rgb(float3 cc)
//{
//    float4 K = to_float4(1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);
//    float3 p = _fabs(fract(swi3(cc,x,x,x) + swi3(K,x,y,z)) * 6.0f - swi3(K,w,w,w));
//    return cc.z * _mix(swi3(K,x,x,x), clamp(p - swi3(K,x,x,x), 0.0f, 1.0f), cc.y);
//}

__DEVICE__ float2 rgb2sv(float3 cc)
{
    float4 K = to_float4(0.0f, -1.0f / 3.0f, 2.0f / 3.0f, -1.0f),
        p = _mix(to_float4_f2f2(swi2(cc,z,y), swi2(K,w,z)), to_float4_f2f2(swi2(cc,y,z), swi2(K,x,y)), step(cc.z, cc.y)),
        q = _mix(to_float4_aw(swi3(p,x,y,w), cc.x), to_float4(cc.x, p.y, p.z, p.x), step(p.x, cc.x));
    return to_float2((q.x - _fminf(q.w, q.y)) / (q.x + 1.e-10), q.x);
}

#define rot(a) to_mat2(_cosf(a),-_sinf(a),_sinf(a),_cosf(a))


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



__DEVICE__ float _saturatef (float x)
{
    return _fminf(1.0f, _fmaxf(0.0f,x));
}
__DEVICE__ float3 _saturatef (float3 x)
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
  y = _saturatef(y-yoffset);
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

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer A' to iChannel1
// Connect Buffer A 'Previsualization: Buffer B' to iChannel2
// Connect Buffer A 'Previsualization: Buffer B' to iChannel3

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
    in float edgeSmoothing) 
{
    float dist = length((st - center) / radius);
    return dist * smoothstep(1.0f, 1.0f - edgeSmoothing, dist);
}


__KERNEL__ void StrangeSmokeFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
    CONNECT_CHECKBOX0(Reset, 0);
    
    //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);
  
    CONNECT_CHECKBOX1(TexBlob, 0);
    CONNECT_SLIDER5(iBlob1PowFactor, -1.0f, 50.0f, 20.0f);
    CONNECT_SLIDER6(iBlob2PowFactor, -1.0f, 50.0f, 20.0f);
    
    CONNECT_SLIDER7(iBlob2ColorPulseSpeed, -1.0f, 1.0f, 0.01234f);
    CONNECT_SLIDER8(iBlob2ColorPulseShift, -1.0f, 1.0f, 0.0f);
    CONNECT_SLIDER9(iFeedbackMouseShiftFactor, -1.0f, 1.0f, 0.003f);
  
    fragCoord+=0.5f;
    
    //
    // https://xemantic.github.io/shader-web-background/
    //
    // In the original shader-web-background these values are provided as uniforms
    // feel free to play with them and if you will find something prettier than
    // the equilibrium I established, please send it back to me :)
    const float2  iFeedbackZoomCenter     = to_float2(0.0f, 0.0f);
    const float iFeedbackZoomRate         = -0.001f;
    const float iFeedbackFadeRate         = 0.998f;
    const float iFeedbackColorShiftZoom   = 0.005f;
    const float iFeedbackColorShiftImpact = -0.0001f;
    const float2  iDrawCenter             = to_float2(0.0f, 0.0f);
    const float iDrawIntensity            = 0.025f;
    const float iBlobEdgeSmoothing        = 0.01f;
    const float iBlob1Radius              = 0.75f;
    //const float iBlob1PowFactor           = 20.0f;
    const float iBlob1ColorPulseSpeed     = 0.024f;
    const float iBlob2Radius              = 0.65f;
    //const float iBlob2PowFactor           = 20.0f;
    //const float iBlob2ColorPulseSpeed     = 0.01234f;
    //const float iBlob2ColorPulseShift     = 0.0f;
    const float iColorShiftOfRadius       = 0.5f;
    //const float iFeedbackMouseShiftFactor = 0.003f;
  

    // in shader-web-background provided as uniforms: start
    float iMinDimension = _fminf(iResolution.x, iResolution.y);
    float2 iScreenRatioHalf = (iResolution.x >= iResolution.y)
                              ? to_float2(iResolution.y / iResolution.x * 0.5f, 0.5f)
                              : to_float2(0.5f, iResolution.x / iResolution.y);
    float3 iBlob1Color = spectral_zucconi6( mod_f(iTime * iBlob1ColorPulseSpeed, 1.0f) );
    
    float3 iBlob2Color = spectral_zucconi6( mod_f(iTime * iBlob2ColorPulseSpeed + iBlob2ColorPulseShift, 1.0f) );
    float2 iFeedbackShiftVector = (iMouse.x > 0.0f && iMouse.y > 0.0f)
                                  ? (swi2(iMouse,x,y) * 2.0f - iResolution) / iMinDimension * iFeedbackMouseShiftFactor
                                  : to_float2_s(0);
    // in shader-web-background provided as uniforms: end
            
    
    float2 uv = fragCoord / iResolution;
    float2 st = (fragCoord * 2.0f - iResolution) / iMinDimension;

    //float2  drawDelta = st - iDrawCenter;
    //float drawAngle = _atan2f(drawDelta.x, drawDelta.y);
    //float drawDist = length(drawDelta);

    float3 feedbk = swi3(repeatedTexture(iChannel1, uv - st),x,y,z);
    float3 colorShift = swi3(repeatedTexture( iChannel0, uv - st * iFeedbackColorShiftZoom * iScreenRatioHalf ),x,y,z);

    float2 stShift = to_float2_s(0);
    stShift += iFeedbackZoomRate * (st - iFeedbackZoomCenter);
    stShift += (swi2(feedbk,z,x)/swi2(colorShift,y,x) - 0.5f) * iFeedbackColorShiftImpact;
    stShift += iFeedbackShiftVector;
    stShift *= iScreenRatioHalf;
    
    float3 prevColor = mix_f3(swi3(repeatedTexture(iChannel2, uv - stShift),x,y,z),swi3(repeatedTexture(iChannel3, uv - stShift),x,y,z),swi3(repeatedTexture(iChannel1, uv - stShift),x,y,z));
    float3 prevColor2 = swi3(repeatedTexture(iChannel2, uv - stShift),x,y,z)+swi3(repeatedTexture(iChannel3, uv - stShift),x,y,z) * swi3(repeatedTexture(iChannel1, uv - stShift),x,y,z)/512.0f;
    
    
    prevColor +=prevColor2/512.0f;
    prevColor *= iFeedbackFadeRate;

    float3 drawColor = to_float3_s(0);
   

    //Textur als Vorgabe
    if (TexBlob)
    {
      float tex = texture(iChannel4,uv).w;

      //drawColor = to_float3_s(0);
      drawColor += _powf(
                          tex,
                          iBlob1PowFactor
                        ) * iBlob1Color;
      drawColor += _powf(
                          tex,
                          iBlob2PowFactor
                        ) * iBlob2Color;
    }
    else
    {   
   
      //Original (Circle)
      float radius = 1.0f + (colorShift.x + colorShift.y + colorShift.z) * iColorShiftOfRadius;
      
      drawColor += _powf(
                          drawBlob(st, iDrawCenter, radius * iBlob1Radius, iBlobEdgeSmoothing),
                          iBlob1PowFactor
                        ) * iBlob1Color;
      drawColor += _powf(
                          drawBlob(st, iDrawCenter, radius * iBlob2Radius, iBlobEdgeSmoothing),
                          iBlob2PowFactor
                        ) * iBlob2Color;
    }



    float3 color = to_float3_s(0);
    drawColor *= iDrawIntensity;
    prevColor *= iFeedbackFadeRate;
    color += prevColor;
    color += drawColor;

    color = clamp(color, 0.0f, 1.0f);
    fragColor = to_float4_aw(color, 1.0f);
    
    if (Blend1>0.0) fragColor = Blending(iChannel4, uv, fragColor, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, fragCoord, R);

    if (iFrame<1 || Reset) fragColor = to_float4_s(0.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer A' to iChannel2
// Connect Buffer B 'Previsualization: Buffer A' to iChannel3


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

#define RotNum 7
#define SUPPORT_EVEN_ROTNUM

#define Res  iResolution
#define Res1 iResolution


__DEVICE__ float4 randS(float2 uv, __TEXTURE2D__ iChannel1)
{
    //return texture(iChannel1,uv*Res.xy/swi2(Res1,x,y))-to_float4_s(0.5f);
    return texture(iChannel1,uv)-to_float4_s(0.5f);
}

__DEVICE__ float getRot(float2 pos, float2 b, mat2 mu, float2 R, __TEXTURE2D__ iChannel0)
{
    float2 p = b;
    float rot=0.0f;
    for(int i=0;i<RotNum;i++)
    {
        rot+=dot(swi2(texture(iChannel0,fract_f2((pos+p)/swi2(Res,x,y))),x,y)-to_float2_s(0.5f),swi2(p,y,x)*to_float2(1,-1));
        p = mul_mat2_f2(mu,p);
    }
    return rot/(float)(RotNum)/dot(b,b);
}

__KERNEL__ void StrangeSmokeFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, float3 iChannelResolution[], sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
    fragCoord+=0.5f;    
    
    const float ang = 2.0f*3.1415926535f/(float)(RotNum);
    mat2 mu = to_mat2(_cosf(ang),_sinf(ang),-_sinf(ang),_cosf(ang));
    mat2 mh = to_mat2(_cosf(ang*0.5f),_sinf(ang*0.5f),-_sinf(ang*0.5f),_cosf(ang*0.5f));

    float2 pos = fragCoord;
    float rnd = randS(to_float2((float)(iFrame)/Res.x,0.5f/Res1.y),iChannel1).x;
    
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
            v+=swi2(p,y,x)*getRot(pos+p,-1.0f*mul_mat2_f2(mh,b),mu,R,iChannel0);
#else
            // this is faster but works only for odd RotNum
            v+=swi2(p,y,x)*getRot(pos+p,b,mu,R,iChannel0);
#endif
            p = mul_mat2_f2(mu,p);
        }
        b*=2.0f;
    }
    
     float2 uv = fragCoord/iResolution; // Normalized pixel coordinates (from 0 to 1)

//  float4 col = texture(iChannel0,fract((pos+v*to_float2(-1,1)*2.0f)/swi2(Res,x,y)));
//  float4 col2 = texture(iChannel3,fract((pos+v*to_float2(-1,1)*2.0f)/swi2(Res,x,y)));
//  float4 blend = _mix(col2,col,0.5f);
  
//  fragColor=blend;
  
  float4 col = texture(iChannel0,fract((pos-v*to_float2(-1,1)*2.0f)/swi2(Res,x,y)));
  float4 col2 = texture(iChannel2,fract((pos-v*to_float2(-1,1)*2.0f)/swi2(Res,x,y)));
  float4 col3 = texture(iChannel3,fract((pos+v*to_float2(-1,1)*2.0f)/swi2(Res,x,y)));
//  col = _fmaxf(col,col2);
//  col2= _fminf(col,col2);
  float4 blend = _mix(col,col2,col3);
 // blend = (blend*0.5f)* ((_fmaxf(col,blend))*0.5f)+ ((_fminf(col,blend))*0.5f);
  blend = clamp(blend, 0.0f, 1.0f);
  fragColor=blend;//,(fract((pos-v*to_float2(-1,1)*2.0f)/swi2(Res,x,y)));  //????????????????????????????????????
  //  fragColor=texture(iChannel0,fract((pos+v*to_float2(-1,1)*2.0f)/swi2(Res,x,y)));
    
  // add a little "motor" in the center
  // float2 scr=(fragCoord/swi2(Res,x,y))*2.0f-to_float2_s(1.0f);
  // swi2(fragColor,x,y) += (0.001f*swi2(scr,x,y) / (dot(scr,scr)/0.1f+0.3f));
    
  // if(iFrame<=4 || KEY_I>0.5f) fragColor=texture(iChannel2,fragCoord/swi2(Res,x,y));

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel3
// Connect Buffer C 'Previsualization: Buffer D' to iChannel0
// Connect Buffer C 'Previsualization: Buffer D' to iChannel1
// Connect Buffer C 'Previsualization: Buffer D' to iChannel2


__KERNEL__ void StrangeSmokeFuse__Buffer_C(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
    fragCoord+=0.5f;  

    //
    // https://xemantic.github.io/shader-web-background/
    //
    // In the original shader-web-background these values are provided as uniforms
    // feel free to play with them and if you will find something prettier than
    // the equilibrium I established, please send it back to me :)
    const float2  iFeedbackZoomCenter     = to_float2(0.0f, 0.0f);
    const float iFeedbackZoomRate         = 0.001f;
    const float iFeedbackFadeRate         = 0.996f;
    const float iFeedbackColorShiftZoom   = 0.005f;
    const float iFeedbackColorShiftImpact = 0.0001f;
    const float2  iDrawCenter             = to_float2(0.0f, 0.0f);
    const float iDrawIntensity            = 0.1f;
    const float iBlobEdgeSmoothing        = 0.1f;
    const float iBlob1Radius              = 0.85f;
    const float iBlob1PowFactor           = 20.0f;
    const float iBlob1ColorPulseSpeed     = -0.04f;
    const float iBlob2Radius              = 0.78f;
    const float iBlob2PowFactor           = 20.0f;
    const float iBlob2ColorPulseSpeed     = 0.1234f;
    const float iBlob2ColorPulseShift     = 0.0f;
    const float iColorShiftOfRadius       = 0.5f;
    const float iFeedbackMouseShiftFactor = 0.003f;

    // in shader-web-background provided as uniforms: start
    float iMinDimension = _fminf(iResolution.x, iResolution.y);
    float2 iScreenRatioHalf = (iResolution.x >= iResolution.y)
                              ? to_float2(iResolution.y / iResolution.x * 0.5f, 0.5f)
                              : to_float2(0.5f, iResolution.x / iResolution.y);
    float3 iBlob1Color = spectral_zucconi6(mod_f(iTime * iBlob1ColorPulseSpeed, 1.0f) );
    
    float3 iBlob2Color = spectral_zucconi6( mod_f(iTime * iBlob2ColorPulseSpeed + iBlob2ColorPulseShift, 1.0f) );
    float2 iFeedbackShiftVector = (iMouse.x > 0.0f && iMouse.y > 0.0f) 
                                  ? (swi2(iMouse,x,y) * 2.0f - iResolution) / iMinDimension * iFeedbackMouseShiftFactor
                                  : to_float2_s(0);
    // in shader-web-background provided as uniforms: end
            
    
    float2 uv = fragCoord / iResolution;
    float2 st = (fragCoord * 2.0f - iResolution) / iMinDimension;

    float2  drawDelta = st - iDrawCenter;
    float drawAngle = _atan2f(drawDelta.x, drawDelta.y);
    float drawDist = length(drawDelta);

    float3 feedbk = swi3(repeatedTexture(iChannel1, uv - st),x,y,z);
    float3 colorShift = swi3(repeatedTexture( iChannel0, uv - st * iFeedbackColorShiftZoom * iScreenRatioHalf ),x,y,z);

    float2 stShift = to_float2_s(0);
    stShift += iFeedbackZoomRate * (st - iFeedbackZoomCenter);
    stShift += (swi2(feedbk,z,x)/swi2(colorShift,y,x) - 0.5f) * iFeedbackColorShiftImpact;
    stShift += iFeedbackShiftVector;
    stShift *= iScreenRatioHalf;

    float3 prevColor = mix_f3(swi3(repeatedTexture(iChannel2, uv - stShift),x,y,z),swi3(repeatedTexture(iChannel3, uv - stShift),x,y,z),swi3(repeatedTexture(iChannel1, uv - stShift),x,y,z));
    float3 prevColor2 = swi3(repeatedTexture(iChannel2, uv - stShift),x,y,z)+swi3(repeatedTexture(iChannel3, uv - stShift),x,y,z) * swi3(repeatedTexture(iChannel1, uv - stShift),x,y,z)/512.0f;
    prevColor +=prevColor2/512.0f;
    prevColor *= iFeedbackFadeRate;

    float3 drawColor = to_float3_s(0);
   
    float radius =  1.0f + (colorShift.x + colorShift.y + colorShift.z) * iColorShiftOfRadius;
    drawColor += _powf(
                        drawBlob(st, iDrawCenter, radius * iBlob1Radius, iBlobEdgeSmoothing),
                        iBlob1PowFactor
                      ) * iBlob1Color;
    drawColor += _powf(
                        drawBlob(st, iDrawCenter, radius * iBlob2Radius, iBlobEdgeSmoothing),
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
// Connect Buffer D 'Previsualization: Buffer A' to iChannel2
// Connect Buffer D 'Previsualization: Buffer A' to iChannel3
// Connect Buffer D 'Previsualization: Buffer B' to iChannel1
// Connect Buffer D 'Previsualization: Buffer D' to iChannel0


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


__KERNEL__ void StrangeSmokeFuse__Buffer_D(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, float3 iChannelResolution[], sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
    fragCoord+=0.5f;
    
    const float ang = 2.0f*3.1415926535f/(float)(RotNum);
    mat2 mu = to_mat2(_cosf(ang),_sinf(ang),-_sinf(ang),_cosf(ang));
    mat2 mh = to_mat2(_cosf(ang*0.5f),_sinf(ang*0.5f),-_sinf(ang*0.5f),_cosf(ang*0.5f));
    
    float2 pos = fragCoord;
    float rnd = randS(to_float2((float)(iFrame)/Res.x,0.5f/Res1.y),iChannel1).x;

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
            v+=swi2(p,y,x)*getRot(pos+p,-1.0f*mul_mat2_f2(mh,b),mu,R,iChannel0);
#else
            // this is faster but works only for odd RotNum
            v+=swi2(p,y,x)*getRot(pos+p,b,mu,R,iChannel0);
#endif
            p = mul_mat2_f2(mu,p);
        }
        b*=2.0f;
    }
    
     float2 uv = fragCoord/iResolution; // Normalized pixel coordinates (from 0 to 1)

//  float4 col = texture(iChannel0,fract((pos+v*to_float2(-1,1)*2.0f)/swi2(Res,x,y)));
//  float4 col2 = texture(iChannel3,fract((pos+v*to_float2(-1,1)*2.0f)/swi2(Res,x,y)));
//  float4 blend = _mix(col2,col,0.5f);
  
//  fragColor=blend;
  
  float4 col = texture(iChannel0,fract_f2((pos+v*to_float2(-1,1)*2.0f)/swi2(Res,x,y)));
  float4 col2 = texture(iChannel2,fract_f2((pos+v*to_float2(-1,1)*2.0f)/swi2(Res,x,y)));
  float4 col3 = texture(iChannel3,fract_f2((pos+v*to_float2(-1,1)*2.0f)/swi2(Res,x,y)));
//  col = _fmaxf(col,col2);
//  col2= _fminf(col,col2);
  float4 blend = _mix(col,col2,col3);
 // blend = (blend*0.5f)* ((_fmaxf(col,blend))*0.5f)+ ((_fminf(col,blend))*0.5f);
  blend = clamp(blend, 0.0f, 1.0f);
  fragColor=blend;//,(fract((pos+v*to_float2(-1,1)*2.0f)/swi2(Res,x,y)));  //????????????????????????????????
 //   fragColor=texture(iChannel0,fract((pos+v*to_float2(-1,1)*2.0f)/swi2(Res,x,y)));
    
 //   add a little "motor" in the center
 //   float2 scr=(fragCoord/swi2(Res,x,y))*2.0f-to_float2_s(1.0f);
 //    swi2(fragColor,x,y) += (0.001f*swi2(scr,x,y) / (dot(scr,scr)/0.1f+0.3f));
    
 //   if(iFrame<=4 || KEY_I>0.5f) fragColor=texture(iChannel2,fragCoord/swi2(Res,x,y));

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer A' to iChannel2
// Connect Image 'Previsualization: Buffer D' to iChannel1


// Fork of a fork of a fork of a fork of a fork of a fork of a fork of a fork of a 

//Chromatic aberration, film grain and tone mapping



__DEVICE__ float randomFloat(float *NoiseSeed){
  *NoiseSeed = _sinf(*NoiseSeed) * 84522.13219145687f;
  return fract(*NoiseSeed);
}

__DEVICE__ float3 ACESFilm(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return (x*(a*x+b))/(x*(c*x+d)+e);
}

__KERNEL__ void StrangeSmokeFuse(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    fragCoord+=0.5f;
    
    float NoiseSeed;
    
    if(fragCoord.y / iResolution.y < Margins || fragCoord.y / iResolution.y > 1.0f-Margins){
        fragColor = to_float4_aw(ACESFilm(to_float3_s(0)), 1.0f);
        SetFragmentShaderComputedColor(fragColor);
        return;
    }

    NoiseSeed = (float)(iFrame)* 0.003186154f + fragCoord.y * 17.2986546543f + fragCoord.x;
    
    float2 uv = fragCoord/iResolution;
    
    float2 d = (uv-to_float2_s(0.5f)) * 0.0075f;
    float3 color = to_float3(texture(iChannel0, uv - 0.0f * d).x,
                             texture(iChannel0, uv - 1.0f * d).y,
                             texture(iChannel0, uv - 2.0f * d).z);
                                  
    float3 col = to_float3(texture(iChannel1, uv - 0.0f * d).x,
                           texture(iChannel1, uv - 1.0f * d).y,
                           texture(iChannel1, uv - 2.0f * d).z);
                      
        float3 col2 = to_float3(texture(iChannel2, uv - 0.0f * d).x,
                                texture(iChannel2, uv - 1.0f * d).y,
                                texture(iChannel2, uv - 2.0f * d).z);
                      
                //      col = _fminf(col,color);
                      color = _fminf(col2,col);
                //     col2 = _mix(col,color,0.5f);
                //     col2 = _fminf(col,color);
                      
                       
 //     color = _mix(col,color,col2);
    float noise = 0.9f + randomFloat(&NoiseSeed)*0.15f;
    fragColor = to_float4_aw(ACESFilm(((color * 0.5f)+ (mix_f3(col,color,col2)))*noise), 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}