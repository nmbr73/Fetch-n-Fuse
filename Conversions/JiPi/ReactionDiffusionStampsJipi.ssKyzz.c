
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


  __DEVICE__ inline mat3 multi( float B, mat3 A)  
  {  
  return to_mat3_f3(A.r0 * B, A.r1 * B, A.r2 * B);  
  }  

  __DEVICE__ inline mat3 inverse( mat3 A)  
  {  
   mat3 R;  
   float result[3][3];  
   float a[3][3] = {{A.r0.x, A.r0.y, A.r0.z},  
					{A.r1.x, A.r1.y, A.r1.z},  
					{A.r2.x, A.r2.y, A.r2.z}};  
     
   float det = a[0][0] * a[1][1] * a[2][2]  
			 + a[0][1] * a[1][2] * a[2][0]  
			 + a[0][2] * a[1][0] * a[2][1]  
			 - a[2][0] * a[1][1] * a[0][2]  
			 - a[2][1] * a[1][2] * a[0][0]  
			 - a[2][2] * a[1][0] * a[0][1];  
   if( det != 0.0 )  
   {  
	   result[0][0] = a[1][1] * a[2][2] - a[1][2] * a[2][1];  
	   result[0][1] = a[2][1] * a[0][2] - a[2][2] * a[0][1];  
	   result[0][2] = a[0][1] * a[1][2] - a[0][2] * a[1][1];  
	   result[1][0] = a[2][0] * a[1][2] - a[1][0] * a[2][2];  
	   result[1][1] = a[0][0] * a[2][2] - a[2][0] * a[0][2];  
	   result[1][2] = a[1][0] * a[0][2] - a[0][0] * a[1][2];  
	   result[2][0] = a[1][0] * a[2][1] - a[2][0] * a[1][1];  
	   result[2][1] = a[2][0] * a[0][1] - a[0][0] * a[2][1];  
	   result[2][2] = a[0][0] * a[1][1] - a[1][0] * a[0][1];  
		 
	   R = to_mat3_f3(make_float3(result[0][0], result[0][1], result[0][2]),   
	                  make_float3(result[1][0], result[1][1], result[1][2]), 
                    make_float3(result[2][0], result[2][1], result[2][2]));  
	   return multi( 1.0f / det, R);  
   }  
   R = to_mat3_f3(make_float3(1.0f, 0.0f, 0.0f), make_float3(0.0f, 1.0f, 0.0f), make_float3(0.0f, 0.0f, 1.0f));  
   return R;  
  } 


/*
 * Reaction-Diffusion-Stamps
 * 
 * Copyright (C) 2022  Alexander Kraus <nr4@z10.info>
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
#ifdef XXX
__DEVICE__ float f = 0.0545f,
    k = 0.062f,
    bpm = 15.0f,
    spb = 60.0f/15.0f,
    stepTime,
    nbeats,
    scale,
//    pi = 3.14159f,
//    fsaa = 144.0f,
    hardBeats,
    time = 0.0f;
__DEVICE__ int frame = 0;
__DEVICE__ float2 unit,
    r = {1.0f,0.5f},   //to_float2(1.0f,0.5f),
    uv,
    resolution;
//__DEVICE__ float3 c = {1.0f,0.0f,-1.0f};//to_float3(1.0f,0.0f,-1.0f);
__DEVICE__ float4 mouse;
#endif


#define pi 3.14159f
#define fsaa 144.0f

// Creative Commons Attribution-ShareAlike 4.0f International Public License
// Created by David Hoskins.
// See https://www.shadertoy.com/view/4djSRW
__DEVICE__ float2 hash22(float2 p)
{
  float3 p3 = fract_f3((swi3(p,x,y,x)) * to_float3(0.1031f, 0.1030f, 0.0973f));
  p3 += dot(p3, swi3(p3,y,z,x)+33.33f);
  return fract_f2((swi2(p3,x,x)+swi2(p3,y,z))*swi2(p3,z,y));
}

// Creative Commons Attribution-ShareAlike 4.0f International Public License
// Created by David Hoskins.
// See https://www.shadertoy.com/view/4djSRW
__DEVICE__ float hash12(float2 p)
{
  float3 p3  = fract_f3(swi3(p,x,y,x) * 0.1031f);
    p3 += dot(p3, swi3(p3,y,z,x) + 33.33f);
    return fract((p3.x + p3.y) * p3.z);
}

__DEVICE__ float lfnoise(float2 t)
{
    float3 c = {1.0f,0.0f,-1.0f};
  
    float2 i = _floor(t);
    t = smoothstep(swi2(c,y,y), swi2(c,x,x), fract(t));
    float2 v1 = to_float2(hash12(i), hash12(i+swi2(c,x,y))),
    v2 = to_float2(hash12(i+swi2(c,y,x)), hash12(i+swi2(c,x,x)));
    v1 = swi2(c,z,z)+2.0f*_mix(v1, v2, t.y);
    return _mix(v1.x, v1.y, t.x);
}

__DEVICE__ float dbox3(float3 x, float3 b)
{
  b = abs_f3(x) - b;
  return length(_fmaxf(b,to_float3_s(0.0f)))
       + _fminf(_fmaxf(b.x,_fmaxf(b.y,b.z)),0.0f);
}

// Distance to star
__DEVICE__ float dstar(float2 x, float N, float2 R)
{
  float3 c = {1.0f,0.0f,-1.0f}; 
  
    float d = pi/N,
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

__DEVICE__ float m(float2 x)
{
    return _fmaxf(x.x,x.y);
}

__DEVICE__ float d210(float2 x)
{
    return _fminf(_fmaxf(_fmaxf(_fmaxf(_fmaxf(_fminf(_fmaxf(_fmaxf(m(abs_f2(to_float2(_fabs(abs(x.x)-0.25f)-0.25f, x.y))-to_float2_s(0.2f)), 
                                                                  -m(abs_f2(to_float2(x.x+0.5f, _fabs(_fabs(x.y)-0.05f)-0.05f))-to_float2(0.12f,0.02f))), 
                                                                  -m(abs_f2(to_float2(_fabs(x.x+0.5f)-0.1f, x.y-0.05f*sign_f(x.x+0.5f)))-to_float2(0.02f,0.07f))),
                                                                   m(abs_f2(to_float2(x.x+0.5f,x.y+0.1f))-to_float2(0.08f,0.04f))), 
                                                                  -m(abs_f2(to_float2(x.x, x.y-0.04f))-to_float2(0.02f, 0.08f))), 
                                                                  -m(abs_f2(to_float2(x.x, x.y+0.1f))-to_float2_s(0.02f))), 
                                                                  -m(abs_f2(to_float2(x.x-0.5f, x.y))-to_float2(0.08f,0.12f))), 
                                                                  -m(abs_f2(to_float2(x.x-0.5f, x.y-0.05f))-to_float2(0.12f, 0.07f))), 
                                                                   m(abs_f2(to_float2(x.x-0.5f, x.y))-to_float2(0.02f, 0.08f)));
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
    return cc.z * _mix(swi3(K,x,x,x), clamp(p - swi3(K,x,x,x), 0.0f, 1.0f), cc.y);
}

__DEVICE__ float2 rgb2sv(float3 cc)
{
    float4 K = to_float4(0.0f, -1.0f / 3.0f, 2.0f / 3.0f, -1.0f),
        p = _mix(to_float4_f2f2(swi2(cc,z,y), swi2(K,w,z)), to_float4_f2f2(swi2(cc,y,z), swi2(K,x,y)), step(cc.z, cc.y)),
        q = _mix(to_float4_aw(swi3(p,x,y,w), cc.x), to_float4(cc.x, p.y,p.z,p.x), step(p.x, cc.x));
    return to_float2((q.x - _fminf(q.w, q.y)) / (q.x + 1.e-10), q.x);
}

__DEVICE__ float3 scene(float3 x, __TEXTURE2D__ buffer, float2 resolution, float nbeats)
{
    float3 c = {1.0f,0.0f,-1.0f};
  
    float3 k = swi3(texture(buffer, mod_f(0.5f*(swi2(x,x,y)+0.5f*resolution/resolution.y),swi2(resolution,x,y))),x,y,z);
    return to_float3(3.0f+8.0f*(k.x+k.y)+0.1f*k.z*k.x*k.y+nbeats+0.5f*x.x*x.y, x.z+0.015f-0.65f*_sqrtf(_fabs(lfnoise(0.1f*swi2(x,x,y)+0.66f)*lfnoise(nbeats*swi2(c,x,x)+0.31f)))*k.y, 0.6f)*to_float3(1.0f,0.25f,1.0f);
}

__DEVICE__ float3 palette(float scale)
{
    const int N = 4;
    float3 colors[N] = {
        to_float3(0.16f,0.22f,0.24f),
        to_float3(0.90f,0.29f,0.37f),
        to_float3(1.00f,0.51f,0.49f),
        to_float3(1.00f,0.80f,0.67f)
                       };
    float i = mod_f(_floor(scale), (float)(N)),
        ip1 = mod_f(i + 1.0f, (float)(N));
    return _mix(colors[(int)(i)], colors[(int)(ip1)], fract(scale));
}

__DEVICE__ bool ray(inout float3 *col, out float3 *x, float d, float3 dir, out float3 *s, float3 o, float3 l, out float3 *n, __TEXTURE2D__ buffer, float2 resolution, float nbeats)
{
    float3 c = {1.0f,0.0f,-1.0f};
  
    for(int i=0; i<250; ++i)
    {
        *x = o + d * dir;
        *s = scene(*x, buffer,resolution, nbeats);

        if(_fabs((*x).z)>0.15f) break;
float zzzzzzzzzzzzzzzzz;        
        if((*s).y < 1.e-4)
        {
            // Blinn-Phong Illumination
            float dx = 5.e-5;
            *n = normalize(to_float3(
                scene(*x+dx*swi3(c,x,y,y), buffer,resolution,nbeats).y, 
                scene(*x+dx*swi3(c,y,x,y), buffer,resolution,nbeats).y, 
                scene(*x+dx*swi3(c,y,y,x), buffer,resolution,nbeats).y
            )-(*s).y);

            *col = palette((*s).x);

            *col = 0.2f * *col
                + *col*_fmaxf(dot(normalize(l- *x),*n),0.0f)
                + 0.7f * *col*_powf(_fmaxf(dot(reflect(normalize(l- *x),*n),dir),0.0f),2.0f);
            
            if((*x).z < -0.01f)
            {
                float cc = 0.035f;
                float2 a = mod_f2(swi2(*x,x,y),cc)-0.5f*cc,
                    ai = swi2(*x,x,y)-mod_f(swi2(*x,x,y)+0.5f*cc,cc), 
                    y = abs_f2(a)-0.002f;
                *col = _mix(*col, 0.5f* *col, smoothstep(1.5f/resolution.y, -1.5f/resolution.y, _fminf(y.x,y.y)))+0.06f*hash12(ai*1.e2);
            }

            return true;
        }
        d += _fabs((*s).y);
    }
    return false;
}

#ifdef XXX
__DEVICE__ void setup(float2 fragCoord, float2 res, float tm, int frm, float4 ms)
{
    time = tm;
    frame = frm;
    resolution = res;
    stepTime = mod_f(time+0.5f*spb, spb)-0.5f*spb;
    nbeats = (11.0f+((time-stepTime+0.5f*spb)/spb + smoothstep(-0.2f*spb, 0.2f*spb, stepTime)))*0.33f;
    scale = smoothstep(-0.3f*spb, 0.0f, stepTime)*smoothstep(0.3f*spb, 0.0f, stepTime);
    hardBeats = round((time-stepTime)/spb);
    uv = fragCoord/resolution;
    unit = swi2(c,x,x)/resolution;
    mouse = ms;
}
#endif

__DEVICE__ void simulate(float2 fragCoord, __TEXTURE2D__ buffer, out float4 *fragColor, float2 resolution, float nbeats, float hardBeats, float2 unit, float4 mouse, inout float *k, inout float *f, float time, float scale, int frame, float2 r)
{
    float3 c = {1.0f,0.0f,-1.0f};
  
    float2 
    uv0 = 3.0f*(fragCoord-0.5f*swi2(resolution,x,y))/resolution.y,
    uva = uv0-3.0f*(hash22(hardBeats*swi2(c,x,x))-0.5f);

    float3 v = swi3(texture(buffer, fragCoord*unit),x,y,z);
    float2 u = swi2(v,x,y);
    float s = hash12(hardBeats*swi2(c,x,x)),
         sdf;

    (*fragColor).z = v.z;

    // Boundary conditions
    *k += 0.01f*lfnoise(0.1f*uv0+ 2.131f + nbeats*swi2(c,x,x));
    *f += 0.01f*lfnoise(0.1f*uv0 + nbeats*swi2(c,x,x)+1.31f);

    // Mouse
    if(mouse.x != 0.0f && mouse.y != 0.0f)
    {
        float2 uv1 = (swi2(mouse,z,w)-0.5f*swi2(resolution,x,y))/resolution.y;
        float3 
            o = swi3(c,y,z,x),
            dir = normalize(uv1.x * swi3(c,x,y,y) + uv1.y * cross(swi3(c,x,y,y),normalize(-o))-o);
        float2 uve = swi2((o - (o.z)/dir.z * dir),x,y);
        uve = 0.5f*(swi2(uve,x,y)+0.5f*resolution/resolution.y);
        float la = length(fragCoord*unit - uve) - 0.1f;
        if(la < 0.0f)
        {
            sdf = _fabs(la)-0.005f;
            u = _fmaxf(u, 0.85f*smoothstep(35.0f*unit.x, -35.0f*unit.x, sdf)*swi2(c,x,x));
            if(sdf < 0.0f)
            {
                (*fragColor).z = time;
            }
        }
    }

    if(scale > 0.95f)
    {
        if(s < 0.1f)
            sdf = _fabs(dstar(uva, 5.0f, to_float2(0.2f,0.5f)))-0.05f;
        else if(s < 0.25f)
            sdf = _fabs(length(uva)-0.3f)-0.1f-0.13f*_sinf(_atan2f(uva.y,uva.x)*4.0f*pi);
        else if(s < 0.4f)
            sdf = _fabs(length(uva)-0.4f)-0.09f;
        else if(s < 0.5f)
            sdf = _fabs(dbox3(to_float3_aw(uva+swi2(uva,y,x)*swi2(c,x,z),0), 0.4f*swi3(c,x,x,x)))-0.09f;
        else if(s < 0.55f)
            sdf = 2.0f*d210(0.5f*uva);
        else if(s < 0.7f)
            sdf = _fabs(dhexagonpattern(2.0f*uva)/2.0f)-0.005f;
        else if(s < 0.95f)
            sdf = _fabs(mod_f(uva.x, 0.8f)-0.4f+_fabs(uva.y)-0.4f)-0.005f;
        else swi2S(*fragColor,x,y, swi2(c,y,y));

        u = _fmaxf(u, 0.85f*smoothstep(35.0f*unit.x, -35.0f*unit.x, sdf)*swi2(c,x,x));
        if(sdf < 0.0f)
        {
            (*fragColor).z = time;
        }
    }
    
    if(frame < 10)
    {
        sdf = _fabs(length(uv0+0.3f)-0.3f)-0.1f-0.13f*_sinf(_atan2f(uv0.y+0.3f,uv0.x+0.3f)*4.0f*pi);
        swi2S(*fragColor,x,y, _fmaxf(u, 0.85f*smoothstep(35.0f*unit.x, -35.0f*unit.x, sdf)*swi2(c,x,x)));
    }
    else
    {
        float2 l = swi2(c,y,y);
        float3 wc = to_float3(0.05f,0.2f,-1.0f);
        //mat3 w = mat3(swi3(wc,x,y,x), swi3(wc,y,z,y), swi3(wc,x,y,x));

        float w[3][3] = {{wc.x,wc.y,wc.x},{wc.y,wc.z,wc.y},{wc.x,wc.y,wc.x}};
float wwwwwwwwwwwwwwwwww;        
        // Laplace operator
        for(int i=0; i<3; ++i)
            for(int j=0; j<3; ++j)
                l += w[i][j]*swi2(texture(buffer, (fragCoord + (1.5f+0.5f*lfnoise(nbeats*swi2(c,x,x)))*make_float2(to_int2(i,j)-1))*unit),x,y);

        // Reaction-Diffusion system
        swi2S(*fragColor,x,y, u + r*l + swi2(c,z,x)*u.x*u.y*u.y + to_float2(*f*(1.0f-u.x), -(*f+ *k)*u.y)+0.0005f*hash22(uv0*1.e2));
        swi2S(*fragColor,x,y, clamp(swi2(*fragColor,x,y), -1.0f,1.0f));

    }
}

__DEVICE__ float3 shifthue(float3 rgb, float hue)
{
    float cc = _cosf(hue),
          cs = _sinf(hue);
    mat3 yiq = to_mat3(
                      0.3f,0.6f,0.21f,
                      0.59f,-0.27f,-0.52f,
                      0.11f,-0.32f,0.31f
                      );
    rgb = mul_mat3_f3(yiq , rgb);
    swi2S(rgb,y,z, mul_f2_mat2(swi2(rgb,y,z) , to_mat2(cc,cs,-cs,cc)));
    return  mul_mat3_f3(inverse(yiq) , rgb);
}

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer D' to iChannel0


/*
 * Reaction-Diffusion-Stamps
 * 
 * Copyright (C) 2022  Alexander Kraus <nr4@z10.info>
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

__KERNEL__ void ReactionDiffusionStampsJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    fragCoord+=0.5f;
float AAAAAAAAAAAAAAAAA;
    //setup(fragCoord, iResolution, iTime, iFrame, iMouse);
    
    float3 c = {1.0f,0.0f,-1.0f};
    
    float f = 0.0545f,
          k = 0.062f,
          bpm = 15.0f,
          spb = 60.0f/15.0f;
    float2  r = to_float2(1.0f,0.5f);
    
    float time = iTime;//tm;
    int frame = iFrame;//frm;
    float2 resolution = iResolution;//res;
    float stepTime = mod_f(time+0.5f*spb, spb)-0.5f*spb;
    float nbeats = (11.0f+((time-stepTime+0.5f*spb)/spb + smoothstep(-0.2f*spb, 0.2f*spb, stepTime)))*0.33f;
    float scale = smoothstep(-0.3f*spb, 0.0f, stepTime)*smoothstep(0.3f*spb, 0.0f, stepTime);
    float hardBeats = round((time-stepTime)/spb);
    float2 uv = fragCoord/resolution;
    float2 unit = swi2(c,x,x)/resolution;
    float4 mouse = iMouse;//ms;
   
    simulate(fragCoord, iChannel0, &fragColor, resolution, nbeats, hardBeats, unit, mouse, &k, &f, time, scale, frame, r);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1
// Connect Buffer B 'Previsualization: Buffer D' to iChannel0


/*
 * Reaction-Diffusion-Stamps
 * 
 * Copyright (C) 2022  Alexander Kraus <nr4@z10.info>
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

__DEVICE__ void pixelB( out float4 *fragColor, in float2 fragCoord, float2 iResolution, float iTime, int iFrame, float4 iMouse, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1 )
{
       
    //setup(fragCoord, iResolution, iTime, iFrame, iMouse);
    
    float3 c = {1.0f,0.0f,-1.0f};
    
    float f = 0.0545f,
          k = 0.062f,
          bpm = 15.0f,
          spb = 60.0f/15.0f;
    float2  r = to_float2(1.0f,0.5f);
    
    float time = iTime;//tm;
    int frame = iFrame;//frm;
    float2 resolution = iResolution;//res;
    float stepTime = mod_f(time+0.5f*spb, spb)-0.5f*spb;
    float nbeats = (11.0f+((time-stepTime+0.5f*spb)/spb + smoothstep(-0.2f*spb, 0.2f*spb, stepTime)))*0.33f;
    float scale = smoothstep(-0.3f*spb, 0.0f, stepTime)*smoothstep(0.3f*spb, 0.0f, stepTime);
    float hardBeats = round((time-stepTime)/spb);
    float2 uv = fragCoord/resolution;
    float2 unit = swi2(c,x,x)/resolution;
    float4 mouse = iMouse;//ms;

    
    float2 uv1 = (fragCoord-0.5f*iResolution)/iResolution.y;
    float3 
        o = swi3(c,y,z,x),
        col,
        c1,
        x,
        x1,
        n,
        dir = normalize(uv1.x * swi3(c,x,y,y) + uv1.y * cross(swi3(c,x,y,y),normalize(-o))-o),
        l = swi3(c,z,z,x)-0.5f*swi3(c,y,y,x),
        s,
        s1;

    // Material ray
    if(ray(&col, &x, -(o.z-0.06f)/dir.z, dir, &s, o, l, &n, iChannel0, resolution, nbeats))
    {
        // Reflections
        if(ray(&c1, &x1, 2.e-3, reflect(dir,n), &s1, x, l, &n, iChannel0, resolution, nbeats))
            col = _mix(col, c1, s.z);

        // Hard Shadow
        if(ray(&c1, &x1, 1.e-2, normalize(l-x), &s1, x, l, &n, iChannel0, resolution, nbeats) && length(l-x1) < length(l-x))
            col *= 0.5f;
    }
float bpbpbpbpbpbpbpbpbpbp;
    // Gamma
    col += col*col + col*col*col;
    col *= 0.75f;
    col = _mix(col, shifthue(col, 2.0f*pi*lfnoise(0.1f*nbeats*swi2(c,x,x))), 0.5f);
    col = _mix(length(col)/_sqrtf(3.0f)*swi3(c,x,x,x), col, clamp(_fabs(x.z*100.0f),0.0f,1.0f));
    *fragColor = _mix(_tex2DVecN(iChannel1,uv.x,uv.y,15), to_float4_aw(clamp(col,0.0f,1.0f),1.0f), 0.5f);
}


__KERNEL__ void ReactionDiffusionStampsJipiFuse__Buffer_B(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{

    fragCoord+=0.5f;
float BBBBBBBBBBBBBBBBBBBB;
    float ssaa = 1.0f;
    float4 col = to_float4_s(0.0f);
    float bound = _sqrtf(ssaa)-1.0f;
        for(float i = -0.5f*bound; i<=0.5f*bound; i+=1.0f)
            for(float j=-0.5f*bound; j<=0.5f*bound; j+=1.0f)
            {
                float4 c1;
                float r = pi/4.0f;
                mat2 R = to_mat2(_cosf(r),_sinf(r),-_sinf(r),_cosf(r));
                pixelB(&c1, fragCoord+mul_mat2_f2(R,(to_float2(i,j)* 1.0f/_fmaxf(bound, 1.0f))),iResolution,iTime,iFrame,iMouse,iChannel0,iChannel1);
                col += c1;
            }
    col /= ssaa;
    fragColor = col;


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0


/*
 * Reaction-Diffusion-Stamps
 * 
 * Copyright (C) 2022  Alexander Kraus <nr4@z10.info>
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

__KERNEL__ void ReactionDiffusionStampsJipiFuse__Buffer_C(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{

    fragCoord+=0.5f;

    //setup(fragCoord, iResolution, iTime, iFrame, iMouse);
    float3 c = {1.0f,0.0f,-1.0f};
    
        float f = 0.0545f,
          k = 0.062f,
          bpm = 15.0f,
          spb = 60.0f/15.0f;
    float2  r = to_float2(1.0f,0.5f);
    
    float time = iTime;//tm;
    int frame = iFrame;//frm;
    float2 resolution = iResolution;//res;
    float stepTime = mod_f(time+0.5f*spb, spb)-0.5f*spb;
    float nbeats = (11.0f+((time-stepTime+0.5f*spb)/spb + smoothstep(-0.2f*spb, 0.2f*spb, stepTime)))*0.33f;
    float scale = smoothstep(-0.3f*spb, 0.0f, stepTime)*smoothstep(0.3f*spb, 0.0f, stepTime);
    float hardBeats = round((time-stepTime)/spb);
    float2 uv = fragCoord/resolution;
    float2 unit = swi2(c,x,x)/resolution;
    float4 mouse = iMouse;//ms;
    
    simulate(fragCoord, iChannel0, &fragColor, resolution, nbeats, hardBeats, unit, mouse, &k, &f, time, scale, frame, r);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


/*
 * Reaction-Diffusion-Stamps
 * 
 * Copyright (C) 2022  Alexander Kraus <nr4@z10.info>
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

__KERNEL__ void ReactionDiffusionStampsJipiFuse__Buffer_D(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{

    fragCoord+=0.5f;

    //setup(fragCoord, iResolution, iTime, iFrame, iMouse);
    float3 c = {1.0f,0.0f,-1.0f};
    
    float f = 0.0545f,
          k = 0.062f,
          bpm = 15.0f,
          spb = 60.0f/15.0f;
    float2  r = to_float2(1.0f,0.5f);
    
    float time = iTime;//tm;
    int frame = iFrame;//frm;
    float2 resolution = iResolution;//res;
    float stepTime = mod_f(time+0.5f*spb, spb)-0.5f*spb;
    float nbeats = (11.0f+((time-stepTime+0.5f*spb)/spb + smoothstep(-0.2f*spb, 0.2f*spb, stepTime)))*0.33f;
    float scale = smoothstep(-0.3f*spb, 0.0f, stepTime)*smoothstep(0.3f*spb, 0.0f, stepTime);
    float hardBeats = round((time-stepTime)/spb);
    float2 uv = fragCoord/resolution;
    float2 unit = swi2(c,x,x)/resolution;
    float4 mouse = iMouse;//ms;
  
    simulate(fragCoord, iChannel0, &fragColor, resolution, nbeats, hardBeats, unit, mouse, &k, &f, time, scale, frame, r);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer B' to iChannel0


/*
 * Reaction-Diffusion-Stamps
 * 
 * Copyright (C) 2022  Alexander Kraus <nr4@z10.info>
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

__DEVICE__ void pixelI( out float4 *fragColor, in float2 fragCoord, float2 iResolution, float iTime, int iFrame, float4 iMouse, __TEXTURE2D__ iChannel0 )
{
    float3 c = {1.0f,0.0f,-1.0f};
  
    //setup(fragCoord, iResolution, iTime, iFrame, iMouse);
    
    float f = 0.0545f,
          k = 0.062f,
          bpm = 15.0f,
          spb = 60.0f/15.0f;
    float2  r = to_float2(1.0f,0.5f);
    
    float time = iTime;//tm;
    int frame = iFrame;//frm;
    float2 resolution = iResolution;//res;
    float stepTime = mod_f(time+0.5f*spb, spb)-0.5f*spb;
    float nbeats = (11.0f+((time-stepTime+0.5f*spb)/spb + smoothstep(-0.2f*spb, 0.2f*spb, stepTime)))*0.33f;
    float scale = smoothstep(-0.3f*spb, 0.0f, stepTime)*smoothstep(0.3f*spb, 0.0f, stepTime);
    float hardBeats = round((time-stepTime)/spb);
    float2 uv = fragCoord/resolution;
    float2 unit = swi2(c,x,x)/resolution;
    float4 mouse = iMouse;//ms;
    
    
    // SSAA
    float3 col = swi3(c,y,y,y);
    float bound = _sqrtf(fsaa)-1.0f;
    for(float i = -0.5f*bound; i<=0.5f*bound; i+=1.0f)
        for(float j=-0.5f*bound; j<=0.5f*bound; j+=1.0f)
            col += swi3(texture(iChannel0, uv+to_float2(i,j)*1.5f/_fmaxf(bound, 1.0f)*unit),x,y,z);
    col /= fsaa;

    // edge glow
    float4 col11 = texture(iChannel0, uv - unit),
        col13 = texture(iChannel0, uv + unit*swi2(c,x,z)),
        col31 = texture(iChannel0 , uv + unit*swi2(c,z,x)),
        col33 = texture(iChannel0, uv + unit),
        x = col33 -col11 -3.0f* texture(iChannel0, uv + unit*swi2(c,y,z)) -col13 + col31 + 3.0f*texture(iChannel0, uv + unit*swi2(c,y,x)),
        y = col33 -col11 -3.0f* texture(iChannel0, uv + unit*swi2(c,z,y)) -col31 + col13 + 3.0f*texture(iChannel0, uv + unit*swi2(c,x,y));
    *fragColor = to_float4_aw(_mix(col, 1.5f*swi3((abs_f3(swi3(y,x,y,z)) + abs_f3(swi3(x,x,y,z))),x,y,z), 0.3f),1.0f);

float pipipipipipipipipi;
    // Vignette
    uv *=  1.0f - swi2(uv,y,x);
    *fragColor *= _powf(uv.x*uv.y * 15.0f, 0.2f);
}

__KERNEL__ void ReactionDiffusionStampsJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{

    fragCoord+=0.5f;
float IIIIIIIIIIIIIIIIIIIIIIII;
    float ssaa = 1.0f;
    float4 col = to_float4_s(0.0f);
    float bound = _sqrtf(ssaa)-1.0f;
        for(float i = -0.5f*bound; i<=0.5f*bound; i+=1.0f)
            for(float j=-0.5f*bound; j<=0.5f*bound; j+=1.0f)
            {
                float4 c1;
                float r = pi/4.0f;
                mat2 R = to_mat2(_cosf(r),_sinf(r),-_sinf(r),_cosf(r));
                pixelI(&c1, fragCoord+mul_mat2_f2(R,(to_float2(i,j)*1.0f/_fmaxf(bound, 1.0f))), iResolution,iTime,iFrame,iMouse,iChannel0);
                col += c1;
            }
    col /= ssaa;
    fragColor = col;


  SetFragmentShaderComputedColor(fragColor);
}