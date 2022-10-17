
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: Uffizi Gallery Blurred_0' to iChannel1
// Connect Image 'Cubemap: Forest_0' to iChannel0


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
#ifdef XXX
__DEVICE__ float3 _refract_f3(float3 I, float3 N, float eta, float justage) {
   float dotNI = dot(N, I);
   float k = 1.0f - eta * eta * (1.0f - dotNI * dotNI);
   if (k < 0.0f) {
     return to_float3_s(0.0);
   }
   return eta * I - (eta * dotNI * _sqrtf(k)) * N * justage; //+0.5f;   * -01.50f;(MarchingCubes)  - 0.15f; (GlassDuck)
}
#endif
__DEVICE__ float3 _refract_f3(float3 I, float3 N, float ior, float justage) {
    //float cosi = clamp(dot(N,I), -1.0f,1.0f);  //clamp(-1, 1, I.dot(N));
    float cosi = clamp( -1.0f,1.0f,dot(N,I));    //clamp(-1, 1, I.dot(N));
    float etai = 01.0f, etat = ior*1.0f* justage;
    float3 n = N;
    if (cosi < 0) {
        cosi = -cosi;
    } else {
        float temp = etai;
        etai = etat;
        etat = temp;
        n = -N;
    }
    float eta = etai / etat;
    float k = 1.0f - (eta * eta) * (1.0f - (cosi * cosi));
    if (k <= 0) {
        return to_float3_s(0.0f);
    } else {
        //return I.multiply(eta).add(n.multiply(((eta * cosi) - Math.sqrt(k))));
    return I*eta+n*((eta*cosi* justage)-_sqrtf(k));  //!!
	  //return eta * I + (eta * cosi - _sqrtf(1.0f-k)) * N;
    }
}

#define MAX_STEPS 100
#define MAX_DIST 100.0f
#define SURF_DIST 0.001f

#define S smoothstep
#define T iTime

__DEVICE__ mat2 Rot(float a) {
    float s=_sinf(a), c=_cosf(a);
    return to_mat2(c, -s, s, c);
} 

__DEVICE__ float sdBox(float3 p, float3 s, float iTime) {
    float a = _atan2f(p.z, p.x);
    float b = _atan2f(p.z, p.y);
    p = 1.1f + 0.1f * cos_f3(p  + p.x * 8.0f + iTime)-s;
    //swi2(p,x,z) += 0.5f + 0.5f * _cosf(4.0f * p.y + iTime);

  return length(_fmaxf(p, to_float3_s(0.0f)))+_fminf(max(p.x, _fmaxf(p.y, p.z)), 0.0f);
}


__DEVICE__ float myLength(float2 u, float iTime) {
    float a = _atan2f(u.x, u.y);
    float b = 0.5f + 0.5f * _cosf(iTime);
    float n = 3.0f - 18.0f * a * a;
    u *= to_float2(_cosf(n * a + iTime), _sinf(n * a + iTime));
    return length(u * _cosf(a));
}

__DEVICE__ float GetDist(float3 p, float iTime) {
   // float d = sdBox(p, to_float3_s(1),iTime);
    //d = _mix(length(p) - 0.2f, d, 0.5f );
    float b = 0.5f + 0.5f * _cosf(iTime);
    float a = _atan2f(p.x,p.z);
    float r1 = 0.9f;
    float r2 = 0.9f;
    float d1 = length(swi2(p,x,z)) - r1;
    float d = p.y * (2.0f + _cosf(3.0f * p.y * b - 18.0f * a)) * myLength(to_float2(d1,p.y),iTime) - r2;   
    
    return 0.05f * d;
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
  float d = GetDist(p,iTime);
    float2 e = to_float2(0.01f, 0);
    
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


__KERNEL__ void TwizzlyFlowerFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{

    CONNECT_SLIDER0(RefractJustage, -10.0f, 10.0f, 0.0f);

    float2 uv = (fragCoord-0.5f*iResolution)/iResolution.y;
    float2 m = swi2(iMouse,x,y)/iResolution;

    //float time
    float3 ro = to_float3(6.0f * _cosf(0.314159f * iTime), 4.0f, 6.0f * _sinf(0.2f * iTime));
    swi2S(ro,y,z, mul_f2_mat2(swi2(ro,y,z) , Rot(-m.y*3.14f+1.0f)));
    swi2S(ro,x,z, mul_f2_mat2(swi2(ro,x,z) , Rot(-m.x*6.2831f)));
    
    float3 rd = GetRayDir(uv, ro, to_float3(0,1.0f,0), 2.2f);
    float3 col = to_float3_s(0);
float IIIIIIIIIIIIIIII;   
    float d = RayMarch(ro, rd,iTime);

    if(d < MAX_DIST) {
        float3 p = ro + rd * d;
        float3 n = GetNormal(p,iTime);
        float3 r = reflect(rd, n);
        float3 rf = _refract_f3(rd, n,0.1f, RefractJustage);
        
        float dif = dot(n, normalize(to_float3(1,2,3)))*0.5f+0.5f;
        col -= 0.2f *to_float3_s(dif);
        
        float b = 0.5f + 0.5f * _cosf(iTime);
        
        uv = fragCoord / iResolution;
        col += 1.5f * swi3(decube_f3(iChannel0,r / (0.5f + n)),x,y,z);
         
        float a = _atan2f(rf.z, rf.x);
        float c = _atan2f(rf.z, rf.y);
        float k = 0.25f;
        col.x += 0.15f + k * _cosf(10.0f * rf.y + 4.0f * iTime - 3.1415f / 2.0f);
        col.y += 0.15f + k * _cosf(10.0f * rf.y + 4.0f * iTime);
        col.z += 0.15f + k * _cosf(10.0f * rf.y + 4.0f * iTime + 3.1415f / 2.0f);
        
        float3 col2 = col;
        col.x *= col2.y;
        col.y *= col2.z;
        col.z *= col2.x;
        
        col /= 1.0f-_powf(_fabs(_sinf(0.2f * length(swi2(p,x,z)) + 0.5f * p.y - 1.0f * iTime)),10.0f);
        col *= (1.0f- 0.1f * length(swi2(p,x,z))) * 0.25f * col;
    }
    
    //col.x =0.02f;
    col = pow_f3(col, to_float3_s(0.4545f));  // gamma correction
    
    fragColor = to_float4_aw(col,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}