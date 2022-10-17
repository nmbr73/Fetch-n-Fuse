
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


////////////////////////////////////////////////////////////////////////////////
//
// Playing around with simplex noise and polar-coords with a lightning-themed
// scene.
//
// Copyright 2019 Mirco Müller
//
// Author(s):
//   Mirco "MacSlow" Müller <macslow@gmail.com>
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License version 3, as published
// by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranties of
// MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
// PURPOSE.  See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////////////

__DEVICE__ mat2 r2d (in float degree)
{
  float rad = radians (degree);
  float c = _cosf (rad);
  float s = _sinf (rad);
  return to_mat2 (c, s,-s, c);
}

// using a slightly adapted implementation of iq's simplex noise from
// https://www.shadertoy.com/view/Msf3WH with hash(), noise() and fbm()
__DEVICE__ float2 hash (in float2 p)
{
  p = to_float2 (dot (p, to_float2 (127.1f, 311.7f)),
                 dot (p, to_float2 (269.5f, 183.3f)));

  return -1.0f + 2.0f*fract_f2 (sin_f2(p)*43758.5453123f);
}

__DEVICE__ float noise (in float2 p)
{
  const float K1 = 0.366025404f;
  const float K2 = 0.211324865f;

  float2 i = _floor (p + (p.x + p.y)*K1);
  
  float2 a = p - i + (i.x + i.y)*K2;
  float2 o = step (swi2(a,y,x), swi2(a,x,y));    
  float2 b = a - o + K2;
  float2 c = a - 1.0f + 2.0f*K2;

  float3 h = _fmaxf (0.5f - to_float3(dot (a, a), dot (b, b), dot (c, c) ), to_float3_s(0.0f));

  float3 n = h*h*h*h*to_float3(dot (a, hash (i + 0.0f)),
                               dot (b, hash (i + o)),
                               dot (c, hash (i + 1.0f)));

    return dot(n, to_float3_s(70.0f));
}

__DEVICE__ float fbm (in float2 p)
{
  mat2 rot = r2d(27.5f);
    float d = noise (p);   p = mul_f2_mat2(p,rot);
    d += 0.5f*noise (p);   p = mul_f2_mat2(p,rot);
    d += 0.25f*noise (p);  p = mul_f2_mat2(p,rot);
    d += 0.125f*noise (p); p = mul_f2_mat2(p,rot);
    d += 0.0625f*noise (p);
    d /= (1.0f + 0.5f + 0.25f + 0.125f + 0.0625f);
  return 0.5f + 0.5f*d;
}

__DEVICE__ float2 mapToScreen (in float2 p, in float scale, float2 iResolution)
{
    float2 res = p;
    res = res * 2.0f - 1.0f;
    res.x *= iResolution.x / iResolution.y;
    res *= scale;
    
    return res;
}

__DEVICE__ float2 cart2polar (in float2 cart)
{
    float r = length (cart);
    float phi = _atan2f (cart.y, cart.x);
    return to_float2 (r, phi); 
}

__DEVICE__ float2 polar2cart (in float2 polar)
{
    float _x = polar.x*_cosf (polar.y);
    float _y = polar.x*_sinf (polar.y);
    return to_float2 (_x, _y); 
}

__KERNEL__ void SparkleSparkleLittleLightningFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{
    CONNECT_SLIDER0(Alpha, 0.0f, 1.0f, 1.0f);
    CONNECT_SLIDER1(UV_Factor, 0.0f, 1.0f, 1.0f);
    CONNECT_SLIDER2(Angle, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER3(thickness, 0.0f, 1.0f, 0.25f);
    CONNECT_SLIDER4(haze, 0.0f, 5.0f, 2.5f);
    CONNECT_POINT0(UV_Offset, 0.0f, 0.0f);
    
    CONNECT_SLIDER5(D1, -10.0f, 10.0f, 1.25f);
    CONNECT_SLIDER6(D2, -10.0f, 10.0f, -1.5f);
    CONNECT_SLIDER7(D3, -10.0f, 10.0f, -2.0f);
    CONNECT_SLIDER8(D4, -10.0f, 10.0f, 2.5f);
    
    
    CONNECT_COLOR0(Color1,0.1f,0.8f,2.0f,1.0f);
    CONNECT_COLOR1(Color2,2.0f,0.1f,0.8f,1.0f);
    CONNECT_COLOR2(Color3,0.8f,2.0f,0.1f,1.0f);

    float ratio = iResolution.x/iResolution.y;
    float2 uv = mapToScreen (fragCoord/iResolution, 2.5f, iResolution);

    uv = mul_f2_mat2(uv,r2d(12.0f*iTime+Angle));
    float len = length (uv);
    //float thickness = 0.25f;
    //float haze = 2.5f;

    // distort UVs a bit
    uv = cart2polar (uv);
    uv.y += 0.2f*(0.5f + 0.5f*_sinf(cos (uv.x)*len));
    uv = polar2cart (uv);
#ifdef ORG
    float d1 = _fabs ((uv.x*haze)*thickness / (uv.x + fbm (uv + 1.25f*iTime)));
    float d2 = _fabs ((uv.y*haze)*thickness / (uv.y + fbm (uv - 1.5f*iTime)));
    float d3 = _fabs ((uv.x*uv.y*haze)*thickness / (uv.x*uv.y + fbm (uv - 2.0f*iTime)));
    float d4 = _fabs ((uv.x*uv.y*haze)*thickness / (uv.x*uv.y + fbm (uv + 2.5f*iTime)));    
#endif

    float d1 = _fabs (((uv.x+D1)*haze)*thickness / ((uv.x-D1) + fbm (uv + 1.25f*iTime)));
    float d2 = _fabs (((uv.y+D2)*haze)*thickness / ((uv.y-D2) + fbm (uv - 1.5f*iTime)));
    float d3 = _fabs ((((uv.x*uv.y)+D3)*haze)*thickness / (((uv.x*uv.y)-D3) + fbm (uv - 2.0f*iTime)));
    float d4 = _fabs ((((uv.x*uv.y)+D4)*haze)*thickness / (((uv.x*uv.y)-D4) + fbm (uv + 2.5f*iTime)));    

    
    float3 col = to_float3_s (0.0f);
    float2 tuv = to_float2(uv.x/ratio,uv.y)*UV_Factor + UV_Offset;
    col = swi3(_tex2DVecN(iChannel0,tuv.x,tuv.y,15),x,y,z);
    float size = 0.075f;
    col += d1*size*swi3(Color1,x,y,z);//to_float3 (0.1f, 0.8f, 2.0f);
    col += d2*size*swi3(Color2,x,y,z);//to_float3 (2.0f, 0.1f, 0.8f);
    col += d3*size*swi3(Color3,x,y,z);//to_float3 (0.8f, 2.0f, 0.1f);
    col += d4*size*swi3(Color3,x,y,z);//to_float3 (0.8f, 2.0f, 0.1f);

    fragColor = to_float4_aw (col, Alpha);

  SetFragmentShaderComputedColor(fragColor);
}