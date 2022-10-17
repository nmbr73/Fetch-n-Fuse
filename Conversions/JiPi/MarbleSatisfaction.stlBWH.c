
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Blue Noise' to iChannel3

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

/*
    MIT License

    Copyright (c) 2022 shyshokayu

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the Software), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, andor sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED AS IS, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#define RAY_MAX_ITERATIONS 100
#define RAY_MAX_DISTANCE 100.0f
#define RAY_SURF_DISTANCE 0.0001f

#define RINGS 6.0f

#define PI 3.1415926535897932384626433832795f
#define TAU (PI * 2.0f)

__DEVICE__ float sine(float t) { return _sinf(t * PI); }
__DEVICE__ float cose(float t) { return _cosf(t * PI); }

__DEVICE__ mat2 rot(float r) {
    float s = _sinf(r), c = _cosf(r);
    return to_mat2(c, -s, s, c);
}

__DEVICE__ mat2 rote(float r) {
    return rot(r * PI);
}

__DEVICE__ mat3 rotX(float r) {
    float s = _sinf(r), c = _cosf(r);
    return to_mat3(
        1.0f, 0.0f, 0.0f,
        0.0f, c  , -s ,
        0.0f, s  , c
    );
}

__DEVICE__ mat3 rotY(float r) {
    float s = _sinf(r), c = _cosf(r);
    return to_mat3(
        c  , 0.0f, -s ,
        0.0f, 1.0f, 0.0f,
        s  , 0.0f, c
    );
}

__DEVICE__ mat3 rotZ(float r) {
    float s = _sinf(r), c = _cosf(r);
    return to_mat3(
        c  , -s , 0.0f,
        s  , c  , 0.0f,
        0.0f, 0.0f, 1.0
    );
}

__DEVICE__ mat3 roteX(float r) {
    return rotX(r * PI);
}

__DEVICE__ mat3 roteY(float r) {
    return rotY(r * PI);
}

__DEVICE__ mat3 roteZ(float r) {
    return rotZ(r * PI);
}

__DEVICE__ float sdPlane(float3 p, float y) {
    return p.y - y;
}

__DEVICE__ float sdSphere(float3 p, float r) {
    return length(p) - r;
}

__DEVICE__ float sdTorus(float3 p, float t, float r) {
  return length(to_float2(length(swi2(p,x,z)) - t, p.y)) - r;
}

__DEVICE__ float sdCappedCylinder(float3 p, float h, float r) {
    float2 d = abs_f2(to_float2(length(swi2(p,x,z)), p.y)) - to_float2(h, r);
    return _fminf(max(d.x, d.y), 0.0f) + length(_fmaxf(d, to_float2_s(0.0f)));
}

struct mapresult {
    float d;
    int m;
};

__DEVICE__ mapresult map(float3 p, float iTime) {
    float d = RAY_MAX_DISTANCE;
    int m = 0;
    
    float sd = sdCappedCylinder(p - to_float3(0.0f, -20.0f, 0.0f), 2.5f, 20.0f);
    if(sd < d) {
        d = sd;
        m = 1;
    }
    
    float tsk = iTime / (RINGS * 0.5f);
    sd = sdSphere(p - to_float3(sine(tsk) * 2.0f, _fabs(cose(iTime)) + 0.25f, cose(tsk) * 2.0f), 0.25f);
    if(sd < d) {
        d = sd;
        m = 2;
    }
    
    float r = _atan2f(p.x, p.z);
    r /= TAU;
    r = fract(r + (0.5f / RINGS)) - (0.5f / RINGS);
    r = round(r * RINGS) / RINGS;
    float y = _fabs(sine((iTime / RINGS) - r));
    float2 po = mul_f2_mat2(swi2(p,x,z) , rote(r * 2.0f));
    
    sd = -sdCappedCylinder(to_float3(po.x, p.y, po.y) - to_float3(0.0f, -0.015f, 2.0f), 0.0625f, 0.03f);
    if(sd >= d) {
        d = sd;
        m = 1;
    }
    
    sd = _fminf(
        sdTorus(to_float3(po.y, po.x, p.y) - to_float3(2.0f, 0.0f, 1.25f + y), 0.25f + 0.0625f, 0.0625f),
        sdCappedCylinder(to_float3(po.x, p.y, po.y) - to_float3(0.0f, (0.5f - 0.0625f) + y, 2.0f), 0.0625f, 0.5f)
    );
    
    if(sd < d) {
        d = sd;
        m = 3;
    }
    
    mapresult ret = {d,m};
    return ret;//mapresult(d, m);
}

__DEVICE__ mapresult march(float3 ro, float3 rd, float iTime) {
    float d = 0.0f;
    int m = 0;
    
        
    for(int i = 0; i < RAY_MAX_ITERATIONS; i++) {
        mapresult mr = map(ro + (rd * d),iTime);
        float sd = mr.d;
        d += sd;
        m = mr.m;
        if(d > RAY_MAX_DISTANCE) { mapresult ret = {RAY_MAX_DISTANCE,0};  return ret; } //mapresult(RAY_MAX_DISTANCE, 0);
        if(_fabs(sd) < RAY_SURF_DISTANCE) break;
    }
    mapresult ret = {d,m};
    return ret;//mapresult(d, m);
}

__DEVICE__ float3 normal(float3 p, float iTime) {
    float2 e = to_float2(RAY_SURF_DISTANCE * 2.0f, 0.0f);
    return normalize(to_float3(
        map(p + swi3(e,x,y,y), iTime).d - map(p - swi3(e,x,y,y), iTime).d,
        map(p + swi3(e,y,x,y), iTime).d - map(p - swi3(e,y,x,y), iTime).d,
        map(p + swi3(e,y,y,x), iTime).d - map(p - swi3(e,y,y,x), iTime).d
    ));
}

__DEVICE__ float shadow(float3 origin, float3 dir, float iTime) {
    return step(RAY_MAX_DISTANCE, march(origin, dir,iTime).d);
}

__DEVICE__ float directionalLightShaded(float3 origin, float3 direction, float3 normal, float iTime) {
    return dot(normal, direction) * shadow(origin + ((normal * RAY_SURF_DISTANCE) * 2.0f), direction, iTime);
}

__DEVICE__ float3 sky(float3 rd, float3 sunDir) {
    float sun = dot(rd, sunDir);
    float sunk = (sun * 0.5f) + 0.5f;
    float suna = _powf(sunk, 4.0f);
    float sunb = _powf(suna, 32.0f);

    float3 skyColorOut = _mix(to_float3(0.5f, 0.6f, 0.7f), to_float3(3.0f, 1.5f, 0.7f), suna);
    skyColorOut = _mix(skyColorOut, to_float3(5.0f, 4.0f, 2.5f), sunb);
    skyColorOut = _mix(skyColorOut, to_float3(8.0f, 6.0f, 4.0f), smoothstep(0.9997f, 0.9998f, sunk));

    float m = 1.0f - _powf(1.0f - _fmaxf(rd.y, 0.0f), 4.0f);

    return skyColorOut;
}

__DEVICE__ float3 aces(float3 _x) {
    return clamp((_x * ((2.51f * _x) + 0.03f)) / (_x * ((2.43f * _x) + 0.59f) + 0.14f), 0.0f, 1.0f);
}

__KERNEL__ void MarbleSatisfactionFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel3)
{
    CONNECT_SLIDER0(SkyReflect, -1.0f, 10.0f, 1.0f);

    float aspect = _fmaxf(iResolution.x / iResolution.y, iResolution.y / iResolution.x);
    float2 uv = (fragCoord / iResolution) - 0.5f;
    uv.x *= aspect;
    uv *= 2.0f;
    
    float3 ro = to_float3(0.0f, 0.5f, -5.0f);
    float3 rd = normalize(to_float3_aw(uv, 1.6f));

    mat3 rotation = mul_mat3_mat3(roteX(0.125f) , roteY(iTime * 0.0625f * 0.25f));
    
    ro = mul_f3_mat3(ro,rotation);
    rd = mul_f3_mat3(rd,rotation);

    mapresult mr = march(ro, rd, iTime);
    float d = mr.d;
    int m = mr.m;
    float3 p = ro + (rd * d);
    float3 n = normal(p, iTime);

    float3 col = to_float3(0.0f, 0.0f, 0.0f);
    
    float skyFactor = d / RAY_MAX_DISTANCE;
    
    float3 sunDir = normalize(to_float3(0.5f, 0.25f, 0.5f));
    
    float3 skyColor = sky(rd, sunDir);
    
    float3 surfaceColor = to_float3_s(0.0f);

    // I know this is some strange stuff, but it looks good, so don't touch it.
    if(m == 1) {
        surfaceColor = _mix(to_float3(0.0f, 0.0f, 0.0f), to_float3(0.7f, 0.7f, 0.7f), fract((_floor(p.x) + _floor(p.z)) * 0.5f));
        
        float3 surfaceReflColor = to_float3_s(0.0f);
        for(int i = 0; i < 30; i++) {
            float seed = texture(iChannel3, to_float2(iTime, (float)(i) * 0.62842f)).x;
            float3 rdRefl = reflect(rd, n);
            rdRefl = normalize(rdRefl + ((swi3(texture(iChannel3, to_float2(p.x + p.z, p.y + p.z) + seed),x,y,z) - 0.5f) * 2.0f) * 3.0f);
            surfaceReflColor += sky(rdRefl, sunDir);
        }
        surfaceReflColor /= 30.0f;
        surfaceColor += surfaceReflColor;
    }
    else if(m == 2) {
        //surfaceColor = to_float3_s(2.0f);
        //surfaceColor = swi3(texture(iChannel0, swi2(n,x,y)),x,y,z);//vec3(2.0);
        surfaceColor = swi3(texture(iChannel0, to_float2(n.x/aspect,n.y)),x,y,z);//vec3(2.0);       
      
        surfaceColor += sky(reflect(rd, n), sunDir)*SkyReflect;
    }
    else if(m == 3) {
        surfaceColor = to_float3_s(-0.25f);
        
        float3 surfaceReflColor = to_float3_s(0.0f);
        for(int i = 0; i < 5; i++) {
            float seed = texture(iChannel3, to_float2(iTime, (float)(i) * 0.62842f)).x;
            float3 rdRefl = reflect(rd, n);
            rdRefl = normalize(rdRefl + ((swi3(texture(iChannel3, to_float2(p.x + p.z, p.y + p.z) + seed),x,y,z) - 0.5f) * 2.0f) * 0.75f);
            surfaceReflColor += sky(rdRefl, sunDir);
        }
        surfaceReflColor /= 5.0f;
        surfaceColor += surfaceReflColor;
    }
    
    surfaceColor *= _mix(directionalLightShaded(p, sunDir, n, iTime), 1.0f, 0.5f);
    
    col = _mix(
        surfaceColor,
        skyColor,
        skyFactor
    );
    
    col = aces(col);
    
    fragColor = to_float4_aw(col, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
