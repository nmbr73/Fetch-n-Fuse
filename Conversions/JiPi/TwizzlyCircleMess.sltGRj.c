
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: Uffizi Gallery_0' to iChannel0
// Connect Image 'Cubemap: Uffizi Gallery Blurred_0' to iChannel1


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

// set these really low so my computer can handle it + dont mind the glitchy look
#define MAX_STEPS 40
#define MAX_DIST 20.0f
#define SURF_DIST 0.05f

#define S smoothstep
#define T iTime

__DEVICE__ mat2 Rot(float a) {
    float s=_sinf(a), c=_cosf(a);
    return to_mat2(c, -s, s, c);
} 

__DEVICE__ float h21 (float2 a) {
    return fract(_sinf(dot(swi2(a,x,y), to_float2(12.9898f, 78.233f))) * 43758.5453123f);
}

__DEVICE__ float thc(float a, float b) {
    return _tanhf(a * _cosf(b)) / _tanhf(a);
}

__DEVICE__ float ths(float a, float b) {
    return _tanhf(a * _sinf(b)) / _tanhf(a);
}

__DEVICE__ float GetDist(float3 p, float iTime) {

    float sd = length(p - to_float3(0,1.5f + 0.5f * _cosf(iTime),0)) 
               - 0.6f  + 0.1f * _cosf(2.0f * iTime);

    float a = _atan2f(p.x, p.z);
    float l = length(swi2(p,x,z));
    float lf = length(fract_f2(swi2(p,x,z))-0.5f);
    //   p.y += 0.2f * thc(2.0f, 5.0f * l * (0.5f + 0.5f * thc(2.0f, 1.0f * l - iTime)) + a + 1.0f * iTime) / _coshf(0.35f * l);
    
    // float2 ipos = _floor(swi2(p,x,z)) - 0.0f;
    //vec2 fpos = fract(swi2(p,x,z)) - 0.25f;
    
    //p.y += 1.0f/_coshf(_mix(12.0f,40.0f, 0.5f + 0.5f * thc(4.0f, iTime + 11.0f * h21(ipos))) * lf);
    p.y *= 1.0f / _coshf(0.5f * l + _cosf(a + iTime));
    p.y += _mix(0.0f, 0.15f, 0.5f + 0.5f * thc(2.0f, 4.0f * l +  iTime)) * thc(4.0f, 4.0f * l + iTime);
    float d = dot(p, to_float3(0,1,0));//length(p) - 1.5f;
       
    return  _fminf(d, 1.0f * sd);
}

__DEVICE__ float RayMarch(float3 ro, float3 rd, float iTime) {
    float dO=0.0f;
    
    for(int i=0; i<MAX_STEPS; i++) {
      float3 p = ro + rd*dO;
        float dS = GetDist(p,iTime);
        dO += dS;
        if(dO>MAX_DIST || _fabs(dS)<SURF_DIST) break;
    }
    
    return dO;
}

__DEVICE__ float3 GetNormal(float3 p, float iTime) {
  float d = GetDist(p, iTime);
    float2 e = to_float2(0.001f, 0);
    
    float3 n = d - to_float3(
                            GetDist(p-swi3(e,x,y,y),iTime),
                            GetDist(p-swi3(e,y,x,y),iTime),
                            GetDist(p-swi3(e,y,y,x),iTime));
    
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

__DEVICE__ float3 Bg(float3 rd) {
    float k = rd.y*0.5f + 0.5f;
    
    float3 col = _mix(to_float3(0.5f,0.0f,0.0f),to_float3_s(0.0f),k);
    return col;
}


__KERNEL__ void TwizzlyCircleMessFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_COLOR0(Color, 0.0f, 0.0f, 0.0f, 1.0f);
    CONNECT_SLIDER0(Brightness, -1.0f, 5.0f, 1.0f);

    float2 uv = (fragCoord-0.5f*iResolution)/iResolution.y;
    float2 m = swi2(iMouse,x,y)/iResolution;
    float time = 0.25f * iTime;
    float3 ro = to_float3(4.0f * thc(5.0f,time), _mix(2.0f, 5.0f, 0.5f + 0.5f * thc(3.0f, 1.5f * time)), 4.0f * ths(5.0f,time));
    swi2S(ro,y,z, mul_f2_mat2(swi2(ro,y,z) , Rot(-m.y*3.14f+1.0f)));
    swi2S(ro,x,z, mul_f2_mat2(swi2(ro,x,z) , Rot(-m.x*6.2831f)));
    
    float3 rd = GetRayDir(uv, ro, to_float3(0,0.0f,0), 1.0f);
    float3 col = to_float3_s(0);
   
    float d = RayMarch(ro, rd, iTime);

    if(d<MAX_DIST) {
        float3 p = ro + rd * d;
        float3 n = GetNormal(p,iTime);
        float3 r = reflect(rd, n);
        float3 rf = refract_f3(rd, n,0.1f);
        
        float dif = dot(n, normalize(to_float3(1,2,3)))*0.5f+0.5f;
        col = 0.42f * to_float3_s(dif);
        
        float b = 0.5f + 0.5f * _cosf(iTime);
        
        uv = fragCoord / iResolution;
        col += 0.9f * swi3(decube_f3(iChannel0, r / (0.5f + n) ),x,y,z);
         
        float a = _atan2f(rf.z, rf.x);
        float c = _atan2f(rf.z, rf.y);
        col.x += 0.15f + (0.25f+Color.x) * thc(2.0f,10.0f * rf.x + iTime - 3.1415f / 2.0f);
        col.y += 0.15f + (0.25f+Color.y) * thc(2.0f,10.0f * rf.y + iTime);
        col.z += 0.15f + (0.25f+Color.z) * thc(2.0f,10.0f * rf.z + iTime + 3.1415f / 2.0f);
        
        float l = length(p);
        float pa = _atan2f(p.x ,p.z);
        col *= 0.7f + 0.5f * Brightness * thc(6.0f + 6.0f * _cosf(20.0f * l + 10.0f * pa + iTime), 
                                              1.2f * l - 24.0f * pa - 0.5f * _cosf(l * 10.0f + 2.0f * pa + iTime) -  2.0f * iTime);
       // col *= 1.5f;
    } else {
        col = swi3(decube_f3(iChannel0,rd),x,y,z);
    }
    
    col.y = 0.5f * (col.x + col.z);
    
    col = pow_f3(col, to_float3_s(0.4545f));  // gamma correction
    
    fragColor = to_float4_aw(col,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}