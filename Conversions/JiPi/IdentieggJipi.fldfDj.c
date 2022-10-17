
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


#define PI 3.1415926

// egg texture parameters
int uCapShape = 11;
int4 uCapShapeOrient = to_int4(0,1,1,0);
float3 uCapCol1 = to_float3(0.9f,0.5f,0.5f);
float3 uCapCol2 = to_float3(0.5f,0.9f,0.9f);
int uRingShape = 21;
int4 uRingShapeOrient = to_int4(1,1,0,1);
float3 uRingCol1 = to_float3(0.5f,0.5f,0.9f);
float3 uRingCol2 = to_float3(0.5f,0.9f,0.5f);
int uBodyShape = 43;
int4 uBodyShapeOrient = to_int4(1,1,1,0);
float3 uBodyCol1 = to_float3(0.9f,0.9f,0.5f);
float3 uBodyCol2 = to_float3(0.5f,0.9f,0.9f);
int uMidShape = 28;
int4 uMidShapeOrient = to_int4(0,0,0,1);
float3 uMidCol1 = to_float3(0.9f,0.8f,0.7f);
float3 uMidCol2 = to_float3(0.7f,0.5f,1.0f);

// random
uint randint(inout uint seed) {
    return seed = seed*1664525u+1013904223u;
}
__DEVICE__ float randfloat(inout uint seed) {
    return float(randint(seed))/4294967296.0f;
}
int4 randto_float4_aw(inout uint seed) {
    uint k = randint(seed)>>4u;
    return to_int4(k&1u,(k>>1u)&1u,(k>>2u)&1u,(k>>3u)&1u);
}
__DEVICE__ float hue2rgb(float t) {
    if (t<0.0f) t+=6.0f;
    if (t>6.0f) t-=6.0f;
    if (t<1.0f) return t;
    if (t<3.0f) return 1.0f;
    if (t<4.0f) return 4.0f-t;
    return 0.0f;
}
__DEVICE__ float3 hsl2rgb(float h, float s, float l) {
    h*=6.0f;
    float mx = l<=0.5f ? l*(s+1.0f) : l+s-l*s;
    float mn = 2.0f*l-mx;
    float r = mn+(mx-mn)*hue2rgb(h+2.0f);
    float g = mn+(mx-mn)*hue2rgb(h);
    float b = mn+(mx-mn)*hue2rgb(h-2.0f);
    return to_float3(r,g,b);
}
__DEVICE__ float3 randcol(inout uint seed) {
    float hue = randfloat(seed);
    float sat = 0.7f+0.3f*randfloat(seed);
    float bri = 0.7f+0.25f*randfloat(seed);
    return hsl2rgb(hue, sat, bri);
}

// random texture parameters
__DEVICE__ void calcTextureParameters(inout uint seed) {
    randint(seed);
    uCapShape = int(randint(seed)%44u);
    uCapShapeOrient = randto_float4(seed);
    uCapCol1 = randcol(seed);
    uCapCol2 = randcol(seed);
    uRingShape = int(randint(seed)%44u);
    uRingShapeOrient = randto_float4(seed);
    uRingCol1 = randcol(seed);
    uRingCol2 = randcol(seed);
    uBodyShape = int(randint(seed)%44u);
    uBodyShapeOrient = randto_float4(seed);
    uBodyCol1 = randcol(seed);
    uBodyCol2 = randcol(seed);
    uMidShape = int(randint(seed)%44u);
    uMidShapeOrient = randto_float4(seed);
    uMidCol1 = randcol(seed);
    uMidCol2 = randcol(seed);
}


// Gravatar identicon components, total 44
// Reference: https://barro.github.io/2018/02/avatars-identicons-and-hash-visualization/

// return a signed number, -1 < u,v < 1
__DEVICE__ float identiconComponent_raw(int id, float u, float v) {
    if (id==0) return v;
    if (id==1) return u-v-2.0f;
    if (id==2) return u+v;
    if (id==3) return _fabs(v)-0.5f*(u+1.0f);
    if (id==4) return _fabs(u)+_fabs(v)-1.0f;
    if (id==5) return _fabs(u+v)-(u-v+2.0f)/3.0f;
    if (id==6) return _fmaxf(_fabs(v)-0.5f*(u+1.0f),-_fmaxf(_fabs(v)-0.5f*(-u+1.0f),-u));
    if (id==7) return _fmaxf(_fabs(u+v)-(u-v+2.0f)/3.0f,u-v-1.0f);
    if (id==8) return _fmaxf(_fabs(u),_fabs(v))-0.5f;
    if (id==9) return _fmaxf(min(-u,v),u-v);
    if (id==10) return _fmaxf(u,-v);
    if (id==11) return _fmaxf(u+_fabs(v)-1.0f,-u);
    if (id==12) return _fmaxf(v-u,-v-u);
    if (id==13) return _fmaxf(v-u-1.0f,_fmaxf(u,-v));
    if (id==14) return u-v+1.0f;
    if (id==15) return u*v;
    if (id==16) return (v+u)*(v-u);
    if (id==17) return u-2.0f*v+1.0f;
    if (id==18) return _fminf(u-2.0f*v+1.0f,_fmaxf(u-2.0f*v-1.0f,v));
    if (id==19) return _fminf(u-2.0f*v+1.0f,_fmaxf(u-v,v));
    if (id==20) return _fmaxf(-u-v,u+2.0f*v-1.0f);
    if (id==21) return _fminf(max(_fminf(-u,v),u-v),_fmaxf(max(-u,v),u-v-1.0f));
    if (id==22) return _fmaxf(1.0f-2.0f*_fabs(v)-u,-1.0f+2.0f*_fabs(v)-u);
    if (id==23) return _fabs(u)-2.0f*_fabs(v)+1.0f;
    if (id==24) return _fminf(_fabs(u)-2.0f*_fabs(v)+1.0f,_fabs(u)+2.0f*_fabs(v)-1.0f);
    if (id==25) return _fabs(u)+_fabs(v)-0.5f;
    if (id==26) return _fminf(1.0f+_fabs(u)-2.0f*_fabs(v),_fabs(v)-2.0f*_fabs(u)+1.0f);
    if (id==27) return _fminf(max(_fabs(v)-u-1.0f,u),_fabs(v)-u);
    if (id==28) return _fminf(max(2.0f*v-u-1.0f,u-v),_fmaxf(2.0f*v+u-1.0f,-u-v));
    if (id==29) return _fminf(u-2.0f*v+1.0f,-u+2.0f*v+1.0f);
    if (id==30) return _fminf(min(1.0f+_fabs(u)-2.0f*_fabs(v),_fabs(v)-2.0f*_fabs(u)+1.0f),_fabs(u)+_fabs(v)-0.5f);
    if (id==31) return _fmaxf(_fabs(abs(u)+_fabs(v)-0.75f)-0.25f,_fminf(u,-v));
    if (id==32) return _fabs(u)+2.0f*_fabs(v)-1.0f;
    if (id==33) return _fminf(u-v+1.0f,_fminf(max(u-v-1.0f,_fmaxf(-u,v)),_fmaxf(min(-u,v),u-v)));
    if (id==34) return _fabs(abs(u)+_fabs(v)-0.75f)-0.25f;
    if (id==35) return _fminf(max(u+2.0f*v+1.0f,u-2.0f*v-1.0f),_fminf(max(v-2.0f*u+1.0f,v+2.0f*u-1.0f),_fmaxf(-u-2.0f*v+1.0f,-u+2.0f*v-1.0f)));
    if (id==36) return _fminf(max(u+2.0f*v+1.0f,u-2.0f*v-1.0f),_fmaxf(v-2.0f*u+1.0f,v+2.0f*u-1.0f));
    if (id==37) return _fminf(max(u+2.0f*v+1.0f,u-2.0f*v-1.0f),_fmaxf(-u-2.0f*v+1.0f,-u+2.0f*v-1.0f));
    if (id==38) return _fmaxf(-u+2.0f*_fabs(v)-1.0f,u-_fabs(v));
    if (id==39) return _fmaxf(2.0f*_fabs(v)-u-1.0f,_fminf(0.5f-u,u-_fabs(v)));
    if (id==40) return _fminf(u-v+1.0f,v-u+1.0f);
    if (id==41) return _fminf(max(u-2.0f*v+1.0f,u),_fmaxf(2.0f*v-u+1.0f,-u));
    if (id==42) return _fmaxf(_fabs(u)+_fabs(v)-1.0f,_fabs(u)-_fabs(v));
    if (id==43) return _fminf(max(_fmaxf(u-v,-u-v),v-u-1.0f),_fmaxf(max(v-u,u+v),u-v-1.0f));
    return 0.0f;
}

__DEVICE__ float identiconComponent(float2 uv, int id, int4 orient) {
    float u = uv.x, v = uv.y;
    if (orient.x==1) u = uv.y, v = uv.x;
    if (orient.y==1) u = -u;
    if (orient.z==1) v = -v;
    return (orient.w==1?-1.0:1.0) * identiconComponent_raw(id, u, v);
}


// texture on different parts of the egg
// 0<u,v<1 for all functions except textureCap

__DEVICE__ float3 textureCap(float u, float v) {
    float x = v*_cosf(4.0f*u), y = v*_sinf(4.0f*u);
    float sd = identiconComponent(to_float2(x,y), uCapShape, uCapShapeOrient);
    return sd<0.0f ? uCapCol1 : uCapCol2;
    return sd<0.0f ? to_float3(0.9f, 0.5f, 0.5f) : to_float3(0.5f, 0.9f, 0.9f);
}

__DEVICE__ float3 textureRing(float u, float v) {
    u = 2.0f*u-1.0f, v=2.0f*v-1.0f;
    float sd = identiconComponent(to_float2(u,v), uRingShape, to_int4(0));
    return sd<0.0f ? uRingCol1 : uRingCol2;
    return sd<0.0f ? to_float3(0.5f, 0.5f, 0.9f) : to_float3(0.5f, 0.9f, 0.5f);
}

__DEVICE__ float3 textureBody(float u, float v) {
    u = 2.0f*u-1.0f, v=2.0f*v-1.0f;
    float sd = identiconComponent(to_float2(u,v), uBodyShape, to_int4(0));
    return sd<0.0f ? uBodyCol1 : uBodyCol2;
    return sd<0.0f ? to_float3(0.9f, 0.9f, 0.5f) :  to_float3(0.5f, 0.9f, 0.9f);
}

__DEVICE__ float3 textureMiddle(float u, float v) {
    u = 2.0f*u-1.0f, v=2.0f*v-1.0f;
    float3 k = _fabs(v)<0.4f ? to_float3_s(1.0f) : to_float3_s(0.9f);
    float sd = identiconComponent(to_float2(u,v), uMidShape, to_int4(0));
    float3 col = sd<0.0f ? to_float3(0.9f,0.8f,0.7f) : to_float3(0.7f,0.5f,1.0f);
    col = sd<0.0f ? uMidCol1 : uMidCol2;
    return k * col;
}

// final egg texture

__DEVICE__ float3 eggTexture(float3 p) {
    float u = _atan2f(p.x, -p.y), v = (PI-_atan2f(length(swi2(p,x,y)), p.z))/PI;
    float vn = _fminf(v, 1.0f-v);

    float3 col = to_float3_s(1.0f);

    if (vn<0.1f) {
        col = textureCap(v<0.5?u:1.0-u, vn/0.1f);
    }
    else if (vn<0.2f) {
        float un8r = _acosf(cos(4.0f*u))/PI;
        col = textureRing(v<0.5?un8r:1.0-un8r, (vn-0.1f)/(0.2f-0.1f));
    }
    else if (vn<0.38f) {
        float un8 = fract(u * 4.0f / PI);
        col = textureBody(v<0.5?un8:1.0-un8, (vn-0.2f)/(0.38f-0.2f));
    }
    else {
        float un8 = u * 4.0f + 0.5f*PI;
        float vn = _sinf(un8)<0.0f ? v : 1.0f-v;
        col = textureMiddle(fract(un8/PI), (vn-0.38f)/(1.0f-2.0f*0.38f));
    }

    return col;
}


// egg geometry

// generated by tracing+fitting real-world egg pictures and find the distribution of the parameters
// uniform random inside an ellipsoid, not quite perfect

// return three numbers a,b,c
// the equation of the section of the egg in polar coordinate is
// r(t) = 1-_sinf(t)²(a+b*_expf(-t)+c*_cosf(t)), c(t)=r(t)*to_float2(_sinf(t),_cosf(t))
__DEVICE__ float3 randomEggShape(inout uint seed) {
    float u = 2.0f*PI * randfloat(seed);
    float v = 2.0f*randfloat(seed)-1.0f;
    float w = _powf(randfloat(seed), 1.0f/3.0f);
    float x = w * _sqrtf(1.0f-v*v) * _cosf(u);
    float y = w * _sqrtf(1.0f-v*v) * _sinf(u);
    float z = w * v;
    return to_float3(0.050482f,1.191725f,-0.211361f)
       + x*to_float3(-0.112119f,0.866281f,-0.16593f)
       + y*to_float3(0.0514355f,0.0009434f,-0.0298296f)
       + z*to_float3(0.0212091f,0.0098094f,0.0368813f);
}

// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


// Random dyed egg generator inspired by WordPress/Gravatar identicon.

// Common: random egg generation
// Image: rendering

// Interactive Javascript version:
// https://harry7557558.github.io/art/dyed-egg/index.html

// =============================== end of description


// MODELING

float3 uEggParameters = to_float3(0.0499f, 1.1173f, -0.1602f);
mat3 uEggOrientation = mat3(1,0,0,0,0.0f,1.0f,0,-1.0f,0.0f);
float3 uEggTranslation = to_float3(0,0,1);

__DEVICE__ float eggEquationPolar(float t) {
    float s = _sinf(t);
    float3 r = (s*s) * to_float3_aw(1.0f, _expf(-t), _cosf(t));
    return 1.0f - dot(r, uEggParameters);
}

__DEVICE__ float eggSDF(float3 p) {
    p = uEggOrientation * (p - uEggTranslation);
    float d = length(swi2(p,x,y)), z = p.z;
    float a = _atan2f(_fabs(d), z);
    float r = length(to_float2(d, z));
    return r - eggEquationPolar(a);
}

__DEVICE__ float3 getEggNormal(float3 p) {
    const float eps = 0.01f;
    return normalize(to_float3(
        eggSDF(p+to_float3(eps,0,0))-eggSDF(p-to_float3(eps,0,0)),
        eggSDF(p+to_float3(0,eps,0))-eggSDF(p-to_float3(0,eps,0)),
        eggSDF(p+to_float3(0,0,eps))-eggSDF(p-to_float3(0,0,eps))));
}


__DEVICE__ bool intersectSphere(float3 ce, float r, in float3 ro, in float3 rd, out float t) {
    float3 p = ro-ce;
    float b = dot(p,rd), c = dot(p,p)-r*r;
    float delta = b*b-c; if (delta<=0.0f) return false;
    delta = _sqrtf(delta);
    t = -b-delta;
    return true;
}

__DEVICE__ bool raymarch(in float3 ro, in float3 rd, out float t) {
    const float eps = 0.001f;
    if (!intersectSphere(uEggTranslation, 1.0f+eps, ro, rd, t)) return false;
    t = _fmaxf(t, 0.0f);
    ro = ro + rd*t;
    float t0 = t;
    t = eps;
    for (int i=0; i<64; i++) {
        float dt = 0.9f*eggSDF(ro+rd*t);
        t += dt;
        if (dt < eps) {
            float3 p = ro+rd*t;
            t += t0;
            return true;
        }
        if (dt>2.0f) break;
    }
    return false;
}


#define ID_PLANE 0
#define ID_EGG 1

__DEVICE__ bool intersectScene(in float3 ro, in float3 rd, out float min_t, out float3 min_n, out float3 fcol, out int intersect_id) {
    float t;
    float3 n;
    min_t = 1e+6;
    intersect_id = -1;

    // intersect with the egg
    if (raymarch(ro, rd, t)) {
        min_t = t, min_n = getEggNormal(ro+rd*t);
        float3 p = uEggOrientation*(ro+rd*t-uEggTranslation);
        fcol = eggTexture(p);
        fcol *= to_float3(0.99f, 0.95f, 0.81f);
        intersect_id = ID_EGG;
    }

    // intersect with the plane
    t = -(ro.z+0.0f)/rd.z;
    if (t > 0.0f && t < min_t) {
        min_t = t, min_n = to_float3(0, 0, 1);
        fcol = to_float3(1.0f, 1.0f, 0.9f);
        float2 p = swi2(ro,x,y)+swi2(rd,x,y)*t;
        if (mod_f(_floor(p.x)+_floor(p.y),2.0f)==0.0f) fcol*=0.9f;
        intersect_id = ID_PLANE;
    }

    if (dot(rd, min_n) > 0.0f) min_n = -min_n;

    return intersect_id != -1;
}


// RENDERING

const float3 light = to_float3(3, 3, 10);


__DEVICE__ float calcSoftShadow(float3 ro, float k) {
    float3 rd = light - ro;
    float col = 1.0f;
    float t = 0.1f;
    float maxt = length(rd);
    for (int i=0; i<8; i++){
        float h = eggSDF(ro + rd*t);
        col = _fminf(col, smoothstep(0.0f, 1.0f, k*h/t));
        t += clamp(h, 0.01f, 0.2f);
        if (h<0.0f || t>maxt) break;
    }
    return _fmaxf(col, 0.0f);
}

__DEVICE__ float calcAO(float3 p, float3 n) {
    float t = 0.12f;
    float3 q = p+n*t;
    float sd = _fminf(eggSDF(q), q.z);
    float occ = (t - sd)*2.0f;
    return smoothstep(0.0f, 1.0f, 1.0f-occ);
}


__DEVICE__ float3 getShade(float3 pos, float3 rd, float3 n, float3 fcol, int intersect_id) {
    float3 lightdir = light - pos;
    float3 ambient = (0.5f+0.2f*dot(n,to_float3(0.5f,0.5f,0.5f)))*to_float3(1.0f,1.0f,1.0f)*fcol / (0.02f*dot(pos,pos)+1.0f);
    float3 direct = 3.0f*_fmaxf(dot(n,lightdir)/dot(lightdir,lightdir), 0.0f) * fcol;
    float3 specular = (intersect_id==ID_EGG?0.05:0.1) * to_float3(1.0f,0.95f,0.9f)*_powf(_fmaxf(dot(rd, normalize(lightdir)), 0.0f), (intersect_id==ID_EGG?5.0:40.0));
    float shadow = calcSoftShadow(pos, 0.2f);
    float ao = calcAO(pos, n);
    return ao*(ambient+shadow*(direct+specular));
}

__DEVICE__ float3 traceRay(float3 ro, float3 rd) {

    float t;
    float3 n;
    float3 fcol;
    int intersect_id;
    if (!intersectScene(ro, rd, t, n, fcol, intersect_id)) {
        return to_float3_s(0.0f);
    }

    ro = ro + rd*t;
    float3 refl = rd - 2.0f*dot(rd, n)*n;
    float3 col_direct = getShade(ro, refl, n, fcol, intersect_id);
    float3 n0 = n;

    if (intersect_id==ID_EGG) return col_direct;

    float3 col_refl = to_float3_s(0.0f);
    if (intersectScene(ro+0.01f*refl, refl, t, n, fcol, intersect_id)) {
        col_refl = getShade(ro+refl*t, refl-2.0f*dot(refl,n)*n, n, fcol, intersect_id);
    }
    else col_refl = to_float3_aw(_powf(_fmaxf(dot(refl, normalize(light-ro)), 0.0f), 2.0f)/(0.02f*dot(ro,ro)+1.0f));

    return _mix(col_direct, col_refl, 0.3f+0.2f*_powf(1.0f-_fabs(dot(refl,n0)),5.0f));
}



__KERNEL__ void IdentieggJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse)
{

    uint seed = uint(iTime);
    if (seed>1u) {
        // random egg texture
        calcTextureParameters(seed);
    
        // random egg shape
        // the Javascript version has a physically-based (but probably buggy) shape orientation calculation
        uEggParameters = randomEggShape(seed);
    }
    uEggTranslation = to_float3(0,0,eggEquationPolar(0.5f*PI));

    // between -1 and 1
    float2 uv = 2.0f * fragCoord/iResolution - 1.0f;

    // barrel distortion from https://www.shadertoy.com/view/wslcDS by Shane
    float r = dot(uv, uv);
    uv *= 1.0f + to_float2(0.03f,0.02f)*(r*r + r);

    // calculate projection
    float rx = iMouse.w>=0.0f ? 0.2f : 1.7f*iMouse.y/iResolution.y-0.1f;
    float rz = iMouse.w>=0.0f ? -0.5f : 6.3f*iMouse.x/iResolution.x-2.8f;
    float3 w = to_float3_aw(_cosf(rx)*to_float2(_cosf(rz),_sinf(rz)), _sinf(rx));
    float3 u = to_float3_aw(-_sinf(rz),_cosf(rz),0);
    float3 v = cross(w,u);

    // camera position
    float3 cam = 6.0f*w + to_float3(0, 0, 0.6f);
    if (cam.z < 0.0f) {
        cam -= w * (cam.z/w.z+1e-3);  // prevent below horizon
    }

    // generate ray
    float3 rd = normalize(mat3(u,v,-w)*to_float3_aw(uv*iResolution, 2.0f*length(iResolution)));
    float3 col = traceRay(cam, rd);

    // adjustment
    float gamma = 1.2f;
    col = to_float3_aw(_powf(col.x,gamma), _powf(col.y,gamma), _powf(col.z,gamma));
    col = 1.2f*col-0.05f;
    fragColor = to_float4_aw(col, 1);


  SetFragmentShaderComputedColor(fragColor);
}