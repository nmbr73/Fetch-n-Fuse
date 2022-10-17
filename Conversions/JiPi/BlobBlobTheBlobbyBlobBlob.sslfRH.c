
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


#define pi 3.14159f

#define thc(a,b) _tanhf(a*_cosf(b))/_tanhf(a)
#define ths(a,b) _tanhf(a*_sinf(b))/_tanhf(a)
#define sabs(x) _sqrtf(x*x+1e-2)

__DEVICE__ float3 pal( in float t, in float3 a, in float3 b, in float3 c, in float3 d )
{
    return a + b*cos_f3( 6.28318f*(c*t+d) );
}

__DEVICE__ float h21 (float2 a) {
    return fract(_sinf(dot(swi2(a,x,y), to_float2(12.9898f, 78.233f))) * 43758.5453123f);
}

__DEVICE__ float mlength(float2 uv) {
    return _fmaxf(_fabs(uv.x), _fabs(uv.y));
}

__DEVICE__ float mlength(float3 uv) {
    return _fmaxf(max(_fabs(uv.x), _fabs(uv.y)), _fabs(uv.z));
}

// (SdSmoothMin) stolen from here: https://www.shadertoy.com/view/MsfBzB
__DEVICE__ float smin(float a, float b)
{
    float k = 0.12f;
    float h = clamp(0.5f + 0.5f * (b-a) / k, 0.0f, 1.0f);
    return _mix(b, a, h) - k * h * (1.0f - h);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


// "RayMarching starting point" 
// by Martijn Steinrucken aka The Art of Code/BigWings - 2020
// The MIT License
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// Email: countfrolic@gmail.com
// Twitter: @The_ArtOfCode
// YouTube: youtube.com/TheArtOfCodeIsCool
// Facebook: https://www.facebook.com/groups/theartofcode/
//
// You can use this shader as a template for ray marching shaders

#define MAX_STEPS 400
#define MAX_DIST 10.0f
#define SURF_DIST 0.001f

#define S smoothstep
#define T iTime

__DEVICE__ mat2 Rot(float a) {
    float s=_sinf(a), c=_cosf(a);
    return to_mat2(c, -s, s, c);
}

__DEVICE__ float sdBox(float3 p, float3 s) {
    p = abs_f3(p)-s;
  return length(_fmaxf(p, to_float3_s(0.0f)))+_fminf(max(p.x, _fmaxf(p.y, p.z)), 0.0f);
}


__DEVICE__ float sdBox(float4 p, float4 s) {
    p = abs_f4(p)-s;
  return length(_fmaxf(p, to_float4_s(0.0f)))+_fminf( _fmaxf(max(p.x, p.y), _fmaxf(p.z, p.w)), 0.0f );
}

#define pi 3.14159f

__DEVICE__ float shape(float4 q, float iTime, float par[4]) {
    float as = 2.0f;//par[0]; //2.0f; 
    float ls = 0.5f;//par[1]; //0.5f;
    float t = 0.5f;//par[2];  //0.5f;
    
    //swi2(q,x,w) *= Rot(as * _atan2f(q.x, q.w) + ls * length(swi2(q,x,w)) - 0.0f * iTime);// - 2.0f * pi / 3.0f);
    //swi2(q,y,w) *= Rot(as * _atan2f(q.y, q.w) + ls * length(swi2(q,y,w)) - 0.0f * iTime);
    //swi2(q,z,w) *= Rot(as * _atan2f(q.z, q.w) + ls * length(swi2(q,z,w)) - 0.0f * iTime);// + 2.0f * pi / 3.0f);
    float m = par[3];  //1.0f;
    // fractal q.w (no idea what this does)
    for (int i = 0; i<5; i++) {
        q.w = sabs(q.w) - m;
        m *=   0.5f; //par[3];
    }
    swi2S(q,x,y, mul_f2_mat2(swi2(q,x,y) , Rot(as * smin(q.w, q.z) + t * iTime)));
    swi2S(q,y,z, mul_f2_mat2(swi2(q,y,z) , Rot(as * smin(q.x, q.w) + t * iTime)));
    swi2S(q,z,w, mul_f2_mat2(swi2(q,z,w) , Rot(as * smin(q.y, q.x) + t * iTime)));
    swi2S(q,w,x, mul_f2_mat2(swi2(q,w,x) , Rot(as * smin(q.z, q.y) + t * iTime)));
    
    // torus looks trippy as fuck but is buggy
    /*
    float r1 = 0.5f;
    float r2 = 0.3f;
    float d1 = length(swi2(q,x,z)) - r1;
    float d2 = length(to_float2(q.w, d1)) - r2;
    */
    // sharper box makes curves more distinguished
    float b = 0.5f - 0.5f * thc(4.0f, 0.5f * q.w + 0.25f * iTime);
    //float b = 0.5f - 0.5f * thc(par[0], par[1] * q.w + par[2] * iTime);
    
    float m1 = _mix(0.2f, 0.4f, b);
    float m2 = _mix(0.7f, 0.1f, b);
    float d = sdBox(q, to_float4_s(m1)) - m2; 
    return d;
}

__DEVICE__ float GetDist(float3 p, float iTime, float par[4]) {
    float w = length(p) * 2.0f + 0.25f * iTime;

    // tried using blacklemori's technique with q.w
    // couldnt get it working outside of the sphere
    float center = _floor(w) + 0.5f;
    // float neighbour = center + ((w < center) ? -1.0f : 1.0f);

    float4 q = to_float4_aw(p, w);
    //float a = _atan2f(p.x, p.z);
    
    float me = shape(q - to_float4(0,0,0,center),iTime, par);
    //float next = shape(q - to_float4(0,0,0,neighbour),iTime); // incorrect but looks okay
    float d = me;//smin(me, next);
    
    float d2 = length(p) - 1.0f;
    // d = -smin(d, -d2);
    //d = _fmaxf(d, d2);
    return 0.7f * d;
}

__DEVICE__ float RayMarch(float3 ro, float3 rd, float z, float iTime, float par[4]) {
    float dO=0.0f;
    
    for(int i=0; i<MAX_STEPS; i++) {
      float3 p = ro + rd*dO;
        float dS = z * GetDist(p,iTime,par);
        dO += dS;
        if(dO>MAX_DIST || _fabs(dS)<SURF_DIST) break;
    }
    
    return dO;
}

__DEVICE__ float3 GetNormal(float3 p, float iTime, float par[4]) {
    float d = GetDist(p,iTime,par);
    float2 e = to_float2(0.001f, 0);
    
    float3 n = d - to_float3(
        GetDist(p-swi3(e,x,y,y),iTime,par),
        GetDist(p-swi3(e,y,x,y),iTime,par),
        GetDist(p-swi3(e,y,y,x),iTime,par));
    
    return normalize(n);
}

__DEVICE__ float3 GetRayDir(float2 uv, float3 p, float3 l, float z) {
    float3 f = normalize(l-p),
        r = normalize(cross(to_float3(0,1,0), f)),
        u = cross(f,r),
        c = f*z,
        i = c + uv.x*r + uv.y*u,
        d = normalize(i);
    return d;
}

__KERNEL__ void BlobBlobTheBlobbyBlobBlobFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse)
{

    CONNECT_COLOR0(Color, 0.0f, 0.33f, 0.66f, 1.0f);
    CONNECT_SLIDER1(as,-10.0f, 10.0f, 2.0f);
    CONNECT_SLIDER2(ls,-10.0f, 10.0f, 0.5f);
    CONNECT_SLIDER3(t,-10.0f, 10.0f, 0.5f);
    CONNECT_SLIDER4(_m,-10.0f, 10.0f, 1.0f);
    CONNECT_SLIDER0(_Alpha, 0.0f, 1.0f, 1.0f);

    float alpha = Color.w;
 
    float par[4] = {as,ls,t,_m};
 
    float2 uv = (fragCoord-0.5f*iResolution)/iResolution.y;
    float2 m = swi2(iMouse,x,y)/iResolution;

    float3 ro = to_float3(0, 3, -3);
    swi2S(ro,y,z, mul_f2_mat2(swi2(ro,y,z) , Rot(-m.y*3.14f+1.0f)));
    swi2S(ro,x,z, mul_f2_mat2(swi2(ro,x,z) , Rot(-m.x*6.2831f)));
    
    float3 rd = GetRayDir(uv, ro, to_float3(0,0.0f,0), 1.8f);
    float3 col = to_float3_s(0);
   
    float d = RayMarch(ro, rd, 1.0f,iTime, par);

    float IOR = 1.05f;
    if(d<MAX_DIST) {
        float3 p = ro + rd * d;
        float3 n = GetNormal(p,iTime,par);
        float3 r = reflect(rd, n);

        float dif = dot(n, normalize(to_float3(1,2,3)))*0.5f+0.5f;
        col = to_float3_s(dif);
        
        float3 rdIn = refract_f3(rd, n, 1.0f/IOR);
        
        float3 pEnter = p - n*SURF_DIST*30.0f;
        float dIn = RayMarch(pEnter, rdIn, -1.0f,iTime,par); // inside the object
        
        float3 pExit = pEnter + rdIn * dIn; // 3d position of exit
        float3 nExit = -1.0f*GetNormal(pExit,iTime,par);
        
        float fresnel = _powf(1.0f+dot(rd, n), 3.0f);
        col = 2.5f * to_float3_s(fresnel);
        col *= 0.55f + 0.45f * cross(nExit, n);
        //col *= 1.0f - _expf(-0.1f * dIn);
        col = clamp(col, 0.0f, 1.0f);
        // col = 1.0f-col;
        float3 e = to_float3_s(1.0f);
        //col *= (p.y + 0.95f) * pal(1.0f, e, e, e, to_float3(0.0f,0.33f,0.66f)); //cba to lookup color
        col *= (p.y + 0.95f) * pal(1.0f, e, e, e, swi3(Color,x,y,z)); //cba to lookup color
        col *= 0.6f + 0.4f * n.y;
        alpha = 1.0f;
    }
    
    col = pow_f3(col, to_float3_s(0.4545f));  // gamma correction
    col += 0.04f;
    fragColor = to_float4_aw(col,alpha);

  SetFragmentShaderComputedColor(fragColor);
}