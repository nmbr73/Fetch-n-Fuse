

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
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
#define RAY_MAX_DISTANCE 100.0
#define RAY_SURF_DISTANCE 0.0001

#define RINGS 6.0

#define PI 3.1415926535897932384626433832795
#define TAU (PI * 2.0)

float sine(float t) { return sin(t * PI); }
float cose(float t) { return cos(t * PI); }

mat2 rot(float r) {
    float s = sin(r), c = cos(r);
    return mat2(c, -s, s, c);
}

mat2 rote(float r) {
    return rot(r * PI);
}

mat3 rotX(float r) {
    float s = sin(r), c = cos(r);
    return mat3(
        1.0, 0.0, 0.0,
        0.0, c  , -s ,
        0.0, s  , c
    );
}

mat3 rotY(float r) {
    float s = sin(r), c = cos(r);
    return mat3(
        c  , 0.0, -s ,
        0.0, 1.0, 0.0,
        s  , 0.0, c
    );
}

mat3 rotZ(float r) {
    float s = sin(r), c = cos(r);
    return mat3(
        c  , -s , 0.0,
        s  , c  , 0.0,
        0.0, 0.0, 1.0
    );
}

mat3 roteX(float r) {
    return rotX(r * PI);
}

mat3 roteY(float r) {
    return rotY(r * PI);
}

mat3 roteZ(float r) {
    return rotZ(r * PI);
}

float sdPlane(vec3 p, float y) {
    return p.y - y;
}

float sdSphere(vec3 p, float r) {
    return length(p) - r;
}

float sdTorus(vec3 p, float t, float r) {
  return length(vec2(length(p.xz) - t, p.y)) - r;
}

float sdCappedCylinder(vec3 p, float h, float r) {
    vec2 d = abs(vec2(length(p.xz), p.y)) - vec2(h, r);
    return min(max(d.x, d.y), 0.0) + length(max(d, 0.0));
}

struct mapresult {
    float d;
    int m;
};

mapresult map(vec3 p) {
    float d = RAY_MAX_DISTANCE;
    int m = 0;
    
    float sd = sdCappedCylinder(p - vec3(0.0, -20.0, 0.0), 2.5, 20.0);
    if(sd < d) {
        d = sd;
        m = 1;
    }
    
    float tsk = iTime / (RINGS * 0.5);
    sd = sdSphere(p - vec3(sine(tsk) * 2.0, abs(cose(iTime)) + 0.25, cose(tsk) * 2.0), 0.25);
    if(sd < d) {
        d = sd;
        m = 2;
    }
    
    float r = atan(p.x, p.z);
    r /= TAU;
    r = fract(r + (0.5 / RINGS)) - (0.5 / RINGS);
    r = round(r * RINGS) / RINGS;
    float y = abs(sine((iTime / RINGS) - r));
    vec2 po = p.xz * rote(r * 2.0);
    
    sd = -sdCappedCylinder(vec3(po.x, p.y, po.y) - vec3(0.0, -0.015, 2.0), 0.0625, 0.03);
    if(sd >= d) {
        d = sd;
        m = 1;
    }
    
    sd = min(
        sdTorus(vec3(po.y, po.x, p.y) - vec3(2.0, 0.0, 1.25 + y), 0.25 + 0.0625, 0.0625),
        sdCappedCylinder(vec3(po.x, p.y, po.y) - vec3(0.0, (0.5 - 0.0625) + y, 2.0), 0.0625, 0.5)
    );
    
    if(sd < d) {
        d = sd;
        m = 3;
    }
    
    return mapresult(d, m);
}

mapresult march(vec3 ro, vec3 rd) {
    float d = 0.0;
    int m = 0;
    for(int i = 0; i < RAY_MAX_ITERATIONS; i++) {
        mapresult mr = map(ro + (rd * d));
        float sd = mr.d;
        d += sd;
        m = mr.m;
        if(d > RAY_MAX_DISTANCE) return mapresult(RAY_MAX_DISTANCE, 0);
        if(abs(sd) < RAY_SURF_DISTANCE) break;
    }
    return mapresult(d, m);
}

vec3 normal(vec3 p) {
    vec2 e = vec2(RAY_SURF_DISTANCE * 2.0, 0.0);
    return normalize(vec3(
        map(p + e.xyy).d - map(p - e.xyy).d,
        map(p + e.yxy).d - map(p - e.yxy).d,
        map(p + e.yyx).d - map(p - e.yyx).d
    ));
}

float shadow(vec3 origin, vec3 dir) {
    return step(RAY_MAX_DISTANCE, march(origin, dir).d);
}

float directionalLightShaded(vec3 origin, vec3 direction, vec3 normal) {
    return dot(normal, direction) * shadow(origin + ((normal * RAY_SURF_DISTANCE) * 2.0), direction);
}

vec3 sky(vec3 rd, vec3 sunDir) {
    float sun = dot(rd, sunDir);
    float sunk = (sun * 0.5) + 0.5;
    float suna = pow(sunk, 4.0);
    float sunb = pow(suna, 32.0);

    vec3 skyColorOut = mix(vec3(0.5, 0.6, 0.7), vec3(3.0, 1.5, 0.7), suna);
    skyColorOut = mix(skyColorOut, vec3(5.0, 4.0, 2.5), sunb);
    skyColorOut = mix(skyColorOut, vec3(8.0, 6.0, 4.0), smoothstep(0.9997, 0.9998, sunk));

    float m = 1.0 - pow(1.0 - max(rd.y, 0.0), 4.0);

    return skyColorOut;
}

vec3 aces(vec3 x) {
    return clamp((x * ((2.51 * x) + 0.03)) / (x * ((2.43 * x) + 0.59) + 0.14), 0.0, 1.0);
}

void mainImage(out vec4 fragColor, vec2 fragCoord) {
    float aspect = max(iResolution.x / iResolution.y, iResolution.y / iResolution.x);
    vec2 uv = (fragCoord / iResolution.xy) - 0.5;
    uv.x *= aspect;
    uv *= 2.0;
    
    vec3 ro = vec3(0.0, 0.5, -5.0);
    vec3 rd = normalize(vec3(uv, 1.6));

    mat3 rotation =
        roteX(0.125) *
        roteY(iTime * 0.0625 * 0.25);
    
    ro *= rotation;
    rd *= rotation;

    mapresult mr = march(ro, rd);
    float d = mr.d;
    int m = mr.m;
    vec3 p = ro + (rd * d);
    vec3 n = normal(p);

    vec3 col = vec3(0.0, 0.0, 0.0);
    
    float skyFactor = d / RAY_MAX_DISTANCE;
    
    vec3 sunDir = normalize(vec3(0.5, 0.25, 0.5));
    
    vec3 skyColor = sky(rd, sunDir);
    
    vec3 surfaceColor = vec3(0.0);

    // I know this is some strange stuff, but it looks good, so don't touch it.
    if(m == 1) {
        surfaceColor = mix(vec3(0.0, 0.0, 0.0), vec3(0.7, 0.7, 0.7), fract((floor(p.x) + floor(p.z)) * 0.5));
        
        vec3 surfaceReflColor = vec3(0.0);
        for(int i = 0; i < 30; i++) {
            float seed = texture(iChannel3, vec2(iTime, float(i) * 0.62842)).x;
            vec3 rdRefl = reflect(rd, n);
            rdRefl = normalize(rdRefl + ((texture(iChannel3, vec2(p.x + p.z, p.y + p.z) + seed).xyz - 0.5) * 2.0) * 3.0);
            surfaceReflColor += sky(rdRefl, sunDir);
        }
        surfaceReflColor /= 30.0;
        surfaceColor += surfaceReflColor;
    }
    else if(m == 2) {
        surfaceColor = vec3(2.0);
        surfaceColor += sky(reflect(rd, n), sunDir);
    }
    else if(m == 3) {
        surfaceColor = vec3(-0.25);
        
        vec3 surfaceReflColor = vec3(0.0);
        for(int i = 0; i < 5; i++) {
            float seed = texture(iChannel3, vec2(iTime, float(i) * 0.62842)).x;
            vec3 rdRefl = reflect(rd, n);
            rdRefl = normalize(rdRefl + ((texture(iChannel3, vec2(p.x + p.z, p.y + p.z) + seed).xyz - 0.5) * 2.0) * 0.75);
            surfaceReflColor += sky(rdRefl, sunDir);
        }
        surfaceReflColor /= 5.0;
        surfaceColor += surfaceReflColor;
    }
    
    surfaceColor *= mix(directionalLightShaded(p, sunDir, n), 1.0, 0.5);
    
    col = mix(
        surfaceColor,
        skyColor,
        skyFactor
    );
    
    col = aces(col);
    
    fragColor = vec4(col, 1.0);
}