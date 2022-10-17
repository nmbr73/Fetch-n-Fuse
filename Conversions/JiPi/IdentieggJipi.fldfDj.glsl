

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Random dyed egg generator inspired by WordPress/Gravatar identicon.

// Common: random egg generation
// Image: rendering

// Interactive Javascript version:
// https://harry7557558.github.io/art/dyed-egg/index.html

// =============================== end of description


// MODELING

vec3 uEggParameters = vec3(0.0499, 1.1173, -0.1602);
mat3 uEggOrientation = mat3(1,0,0,0,0.0,1.0,0,-1.0,0.0);
vec3 uEggTranslation = vec3(0,0,1);

float eggEquationPolar(float t) {
    float s = sin(t);
    vec3 r = (s*s) * vec3(1.0, exp(-t), cos(t));
    return 1.0 - dot(r, uEggParameters);
}

float eggSDF(vec3 p) {
    p = uEggOrientation * (p - uEggTranslation);
    float d = length(p.xy), z = p.z;
    float a = atan(abs(d), z);
    float r = length(vec2(d, z));
    return r - eggEquationPolar(a);
}

vec3 getEggNormal(vec3 p) {
    const float eps = 0.01;
    return normalize(vec3(
        eggSDF(p+vec3(eps,0,0))-eggSDF(p-vec3(eps,0,0)),
        eggSDF(p+vec3(0,eps,0))-eggSDF(p-vec3(0,eps,0)),
        eggSDF(p+vec3(0,0,eps))-eggSDF(p-vec3(0,0,eps))));
}


bool intersectSphere(vec3 ce, float r, in vec3 ro, in vec3 rd, out float t) {
    vec3 p = ro-ce;
    float b = dot(p,rd), c = dot(p,p)-r*r;
    float delta = b*b-c; if (delta<=0.0) return false;
    delta = sqrt(delta);
    t = -b-delta;
    return true;
}

bool raymarch(in vec3 ro, in vec3 rd, out float t) {
    const float eps = 0.001;
    if (!intersectSphere(uEggTranslation, 1.0+eps, ro, rd, t)) return false;
    t = max(t, 0.0);
    ro = ro + rd*t;
    float t0 = t;
    t = eps;
    for (int i=0; i<64; i++) {
        float dt = 0.9*eggSDF(ro+rd*t);
        t += dt;
        if (dt < eps) {
            vec3 p = ro+rd*t;
            t += t0;
            return true;
        }
        if (dt>2.0) break;
    }
    return false;
}


#define ID_PLANE 0
#define ID_EGG 1

bool intersectScene(in vec3 ro, in vec3 rd, out float min_t, out vec3 min_n, out vec3 fcol, out int intersect_id) {
    float t;
    vec3 n;
    min_t = 1e+6;
    intersect_id = -1;

    // intersect with the egg
    if (raymarch(ro, rd, t)) {
        min_t = t, min_n = getEggNormal(ro+rd*t);
        vec3 p = uEggOrientation*(ro+rd*t-uEggTranslation);
        fcol = eggTexture(p);
        fcol *= vec3(0.99, 0.95, 0.81);
        intersect_id = ID_EGG;
    }

    // intersect with the plane
    t = -(ro.z+0.0)/rd.z;
    if (t > 0.0 && t < min_t) {
        min_t = t, min_n = vec3(0, 0, 1);
        fcol = vec3(1.0, 1.0, 0.9);
        vec2 p = ro.xy+rd.xy*t;
        if (mod(floor(p.x)+floor(p.y),2.0)==0.0) fcol*=0.9;
        intersect_id = ID_PLANE;
    }

    if (dot(rd, min_n) > 0.0) min_n = -min_n;

    return intersect_id != -1;
}


// RENDERING

const vec3 light = vec3(3, 3, 10);


float calcSoftShadow(vec3 ro, float k) {
    vec3 rd = light - ro;
    float col = 1.0;
    float t = 0.1;
    float maxt = length(rd);
    for (int i=0; i<8; i++){
        float h = eggSDF(ro + rd*t);
        col = min(col, smoothstep(0.0, 1.0, k*h/t));
        t += clamp(h, 0.01, 0.2);
        if (h<0. || t>maxt) break;
    }
    return max(col, 0.);
}

float calcAO(vec3 p, vec3 n) {
    float t = 0.12;
    vec3 q = p+n*t;
    float sd = min(eggSDF(q), q.z);
    float occ = (t - sd)*2.0;
    return smoothstep(0.0, 1.0, 1.0-occ);
}


vec3 getShade(vec3 pos, vec3 rd, vec3 n, vec3 fcol, int intersect_id) {
    vec3 lightdir = light - pos;
    vec3 ambient = (0.5+0.2*dot(n,vec3(0.5,0.5,0.5)))*vec3(1.0,1.0,1.0)*fcol / (0.02*dot(pos,pos)+1.0);
    vec3 direct = 3.0*max(dot(n,lightdir)/dot(lightdir,lightdir), 0.0) * fcol;
    vec3 specular = (intersect_id==ID_EGG?0.05:0.1) * vec3(1.0,0.95,0.9)*pow(max(dot(rd, normalize(lightdir)), 0.0), (intersect_id==ID_EGG?5.0:40.0));
    float shadow = calcSoftShadow(pos, 0.2);
    float ao = calcAO(pos, n);
    return ao*(ambient+shadow*(direct+specular));
}

vec3 traceRay(vec3 ro, vec3 rd) {

    float t;
    vec3 n;
    vec3 fcol;
    int intersect_id;
    if (!intersectScene(ro, rd, t, n, fcol, intersect_id)) {
        return vec3(0.0);
    }

    ro = ro + rd*t;
    vec3 refl = rd - 2.0*dot(rd, n)*n;
    vec3 col_direct = getShade(ro, refl, n, fcol, intersect_id);
    vec3 n0 = n;

    if (intersect_id==ID_EGG) return col_direct;

    vec3 col_refl = vec3(0.0);
    if (intersectScene(ro+0.01*refl, refl, t, n, fcol, intersect_id)) {
        col_refl = getShade(ro+refl*t, refl-2.0*dot(refl,n)*n, n, fcol, intersect_id);
    }
    else col_refl = vec3(pow(max(dot(refl, normalize(light-ro)), 0.0), 2.0)/(0.02*dot(ro,ro)+1.0));

    return mix(col_direct, col_refl, 0.3+0.2*pow(1.0-abs(dot(refl,n0)),5.0));
}



void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    uint seed = uint(iTime);
    if (seed>1u) {
        // random egg texture
        calcTextureParameters(seed);
    
        // random egg shape
        // the Javascript version has a physically-based (but probably buggy) shape orientation calculation
        uEggParameters = randomEggShape(seed);
    }
    uEggTranslation = vec3(0,0,eggEquationPolar(.5*PI));

    // between -1 and 1
    vec2 uv = 2. * fragCoord/iResolution.xy - 1.;

    // barrel distortion from https://www.shadertoy.com/view/wslcDS by Shane
    float r = dot(uv, uv);
    uv *= 1. + vec2(.03,.02)*(r*r + r);

    // calculate projection
    float rx = iMouse.w>=0. ? 0.2 : 1.7*iMouse.y/iResolution.y-0.1;
    float rz = iMouse.w>=0. ? -0.5 : 6.3*iMouse.x/iResolution.x-2.8;
    vec3 w = vec3(cos(rx)*vec2(cos(rz),sin(rz)), sin(rx));
    vec3 u = vec3(-sin(rz),cos(rz),0);
    vec3 v = cross(w,u);

    // camera position
    vec3 cam = 6.*w + vec3(0, 0, 0.6);
    if (cam.z < 0.) {
        cam -= w * (cam.z/w.z+1e-3);  // prevent below horizon
    }

    // generate ray
    vec3 rd = normalize(mat3(u,v,-w)*vec3(uv*iResolution.xy, 2.*length(iResolution)));
    vec3 col = traceRay(cam, rd);

    // adjustment
    float gamma = 1.2;
    col = vec3(pow(col.x,gamma), pow(col.y,gamma), pow(col.z,gamma));
    col = 1.2*col-0.05;
    fragColor = vec4(col, 1);
}

// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define PI 3.1415926

// egg texture parameters
int uCapShape = 11;
ivec4 uCapShapeOrient = ivec4(0,1,1,0);
vec3 uCapCol1 = vec3(0.9,0.5,0.5);
vec3 uCapCol2 = vec3(0.5,0.9,0.9);
int uRingShape = 21;
ivec4 uRingShapeOrient = ivec4(1,1,0,1);
vec3 uRingCol1 = vec3(0.5,0.5,0.9);
vec3 uRingCol2 = vec3(0.5,0.9,0.5);
int uBodyShape = 43;
ivec4 uBodyShapeOrient = ivec4(1,1,1,0);
vec3 uBodyCol1 = vec3(0.9,0.9,0.5);
vec3 uBodyCol2 = vec3(0.5,0.9,0.9);
int uMidShape = 28;
ivec4 uMidShapeOrient = ivec4(0,0,0,1);
vec3 uMidCol1 = vec3(0.9,0.8,0.7);
vec3 uMidCol2 = vec3(0.7,0.5,1.0);

// random
uint randint(inout uint seed) {
    return seed = seed*1664525u+1013904223u;
}
float randfloat(inout uint seed) {
    return float(randint(seed))/4294967296.0;
}
ivec4 randvec4(inout uint seed) {
    uint k = randint(seed)>>4u;
    return ivec4(k&1u,(k>>1u)&1u,(k>>2u)&1u,(k>>3u)&1u);
}
float hue2rgb(float t) {
    if (t<0.0) t+=6.0;
    if (t>6.0) t-=6.0;
    if (t<1.0) return t;
    if (t<3.0) return 1.0;
    if (t<4.0) return 4.0-t;
    return 0.0;
}
vec3 hsl2rgb(float h, float s, float l) {
    h*=6.0;
    float mx = l<=0.5 ? l*(s+1.0) : l+s-l*s;
    float mn = 2.0*l-mx;
    float r = mn+(mx-mn)*hue2rgb(h+2.0);
    float g = mn+(mx-mn)*hue2rgb(h);
    float b = mn+(mx-mn)*hue2rgb(h-2.0);
    return vec3(r,g,b);
}
vec3 randcol(inout uint seed) {
    float hue = randfloat(seed);
    float sat = 0.7+0.3*randfloat(seed);
    float bri = 0.7+0.25*randfloat(seed);
    return hsl2rgb(hue, sat, bri);
}

// random texture parameters
void calcTextureParameters(inout uint seed) {
    randint(seed);
    uCapShape = int(randint(seed)%44u);
    uCapShapeOrient = randvec4(seed);
    uCapCol1 = randcol(seed);
    uCapCol2 = randcol(seed);
    uRingShape = int(randint(seed)%44u);
    uRingShapeOrient = randvec4(seed);
    uRingCol1 = randcol(seed);
    uRingCol2 = randcol(seed);
    uBodyShape = int(randint(seed)%44u);
    uBodyShapeOrient = randvec4(seed);
    uBodyCol1 = randcol(seed);
    uBodyCol2 = randcol(seed);
    uMidShape = int(randint(seed)%44u);
    uMidShapeOrient = randvec4(seed);
    uMidCol1 = randcol(seed);
    uMidCol2 = randcol(seed);
}


// Gravatar identicon components, total 44
// Reference: https://barro.github.io/2018/02/avatars-identicons-and-hash-visualization/

// return a signed number, -1 < u,v < 1
float identiconComponent_raw(int id, float u, float v) {
    if (id==0) return v;
    if (id==1) return u-v-2.0;
    if (id==2) return u+v;
    if (id==3) return abs(v)-0.5*(u+1.0);
    if (id==4) return abs(u)+abs(v)-1.0;
    if (id==5) return abs(u+v)-(u-v+2.0)/3.0;
    if (id==6) return max(abs(v)-0.5*(u+1.0),-max(abs(v)-0.5*(-u+1.0),-u));
    if (id==7) return max(abs(u+v)-(u-v+2.0)/3.0,u-v-1.0);
    if (id==8) return max(abs(u),abs(v))-0.5;
    if (id==9) return max(min(-u,v),u-v);
    if (id==10) return max(u,-v);
    if (id==11) return max(u+abs(v)-1.0,-u);
    if (id==12) return max(v-u,-v-u);
    if (id==13) return max(v-u-1.0,max(u,-v));
    if (id==14) return u-v+1.0;
    if (id==15) return u*v;
    if (id==16) return (v+u)*(v-u);
    if (id==17) return u-2.0*v+1.0;
    if (id==18) return min(u-2.0*v+1.0,max(u-2.0*v-1.0,v));
    if (id==19) return min(u-2.0*v+1.0,max(u-v,v));
    if (id==20) return max(-u-v,u+2.0*v-1.0);
    if (id==21) return min(max(min(-u,v),u-v),max(max(-u,v),u-v-1.0));
    if (id==22) return max(1.0-2.0*abs(v)-u,-1.0+2.0*abs(v)-u);
    if (id==23) return abs(u)-2.0*abs(v)+1.0;
    if (id==24) return min(abs(u)-2.0*abs(v)+1.0,abs(u)+2.0*abs(v)-1.0);
    if (id==25) return abs(u)+abs(v)-0.5;
    if (id==26) return min(1.0+abs(u)-2.0*abs(v),abs(v)-2.0*abs(u)+1.0);
    if (id==27) return min(max(abs(v)-u-1.0,u),abs(v)-u);
    if (id==28) return min(max(2.0*v-u-1.0,u-v),max(2.0*v+u-1.0,-u-v));
    if (id==29) return min(u-2.0*v+1.0,-u+2.0*v+1.0);
    if (id==30) return min(min(1.0+abs(u)-2.0*abs(v),abs(v)-2.0*abs(u)+1.0),abs(u)+abs(v)-0.5);
    if (id==31) return max(abs(abs(u)+abs(v)-0.75)-0.25,min(u,-v));
    if (id==32) return abs(u)+2.0*abs(v)-1.0;
    if (id==33) return min(u-v+1.0,min(max(u-v-1.0,max(-u,v)),max(min(-u,v),u-v)));
    if (id==34) return abs(abs(u)+abs(v)-0.75)-0.25;
    if (id==35) return min(max(u+2.0*v+1.0,u-2.0*v-1.0),min(max(v-2.0*u+1.0,v+2.0*u-1.0),max(-u-2.0*v+1.0,-u+2.0*v-1.0)));
    if (id==36) return min(max(u+2.0*v+1.0,u-2.0*v-1.0),max(v-2.0*u+1.0,v+2.0*u-1.0));
    if (id==37) return min(max(u+2.0*v+1.0,u-2.0*v-1.0),max(-u-2.0*v+1.0,-u+2.0*v-1.0));
    if (id==38) return max(-u+2.0*abs(v)-1.0,u-abs(v));
    if (id==39) return max(2.0*abs(v)-u-1.0,min(0.5-u,u-abs(v)));
    if (id==40) return min(u-v+1.0,v-u+1.0);
    if (id==41) return min(max(u-2.0*v+1.0,u),max(2.0*v-u+1.0,-u));
    if (id==42) return max(abs(u)+abs(v)-1.0,abs(u)-abs(v));
    if (id==43) return min(max(max(u-v,-u-v),v-u-1.0),max(max(v-u,u+v),u-v-1.0));
    return 0.0;
}

float identiconComponent(vec2 uv, int id, ivec4 orient) {
    float u = uv.x, v = uv.y;
    if (orient.x==1) u = uv.y, v = uv.x;
    if (orient.y==1) u = -u;
    if (orient.z==1) v = -v;
    return (orient.w==1?-1.0:1.0) * identiconComponent_raw(id, u, v);
}


// texture on different parts of the egg
// 0<u,v<1 for all functions except textureCap

vec3 textureCap(float u, float v) {
    float x = v*cos(4.0*u), y = v*sin(4.0*u);
    float sd = identiconComponent(vec2(x,y), uCapShape, uCapShapeOrient);
    return sd<0.0 ? uCapCol1 : uCapCol2;
    return sd<0.0 ? vec3(0.9, 0.5, 0.5) : vec3(0.5, 0.9, 0.9);
}

vec3 textureRing(float u, float v) {
    u = 2.0*u-1.0, v=2.0*v-1.0;
    float sd = identiconComponent(vec2(u,v), uRingShape, ivec4(0));
    return sd<0.0 ? uRingCol1 : uRingCol2;
    return sd<0.0 ? vec3(0.5, 0.5, 0.9) : vec3(0.5, 0.9, 0.5);
}

vec3 textureBody(float u, float v) {
    u = 2.0*u-1.0, v=2.0*v-1.0;
    float sd = identiconComponent(vec2(u,v), uBodyShape, ivec4(0));
    return sd<0.0 ? uBodyCol1 : uBodyCol2;
    return sd<0.0 ? vec3(0.9, 0.9, 0.5) :  vec3(0.5, 0.9, 0.9);
}

vec3 textureMiddle(float u, float v) {
    u = 2.0*u-1.0, v=2.0*v-1.0;
    vec3 k = abs(v)<0.4 ? vec3(1.0) : vec3(0.9);
    float sd = identiconComponent(vec2(u,v), uMidShape, ivec4(0));
    vec3 col = sd<0.0 ? vec3(0.9,0.8,0.7) : vec3(0.7,0.5,1.0);
    col = sd<0.0 ? uMidCol1 : uMidCol2;
    return k * col;
}

// final egg texture

vec3 eggTexture(vec3 p) {
    float u = atan(p.x, -p.y), v = (PI-atan(length(p.xy), p.z))/PI;
    float vn = min(v, 1.0-v);

    vec3 col = vec3(1.0);

    if (vn<0.1) {
        col = textureCap(v<0.5?u:1.0-u, vn/0.1);
    }
    else if (vn<0.2) {
        float un8r = acos(cos(4.0*u))/PI;
        col = textureRing(v<0.5?un8r:1.0-un8r, (vn-0.1)/(0.2-0.1));
    }
    else if (vn<0.38) {
        float un8 = fract(u * 4.0 / PI);
        col = textureBody(v<0.5?un8:1.0-un8, (vn-0.2)/(0.38-0.2));
    }
    else {
        float un8 = u * 4.0 + 0.5*PI;
        float vn = sin(un8)<0.0 ? v : 1.0-v;
        col = textureMiddle(fract(un8/PI), (vn-0.38)/(1.0-2.0*0.38));
    }

    return col;
}


// egg geometry

// generated by tracing+fitting real-world egg pictures and find the distribution of the parameters
// uniform random inside an ellipsoid, not quite perfect

// return three numbers a,b,c
// the equation of the section of the egg in polar coordinate is
// r(t) = 1-sin(t)²(a+b*exp(-t)+c*cos(t)), c(t)=r(t)*vec2(sin(t),cos(t))
vec3 randomEggShape(inout uint seed) {
    float u = 2.0*PI * randfloat(seed);
    float v = 2.0*randfloat(seed)-1.0;
    float w = pow(randfloat(seed), 1.0/3.0);
    float x = w * sqrt(1.0-v*v) * cos(u);
    float y = w * sqrt(1.0-v*v) * sin(u);
    float z = w * v;
    return vec3(0.050482,1.191725,-0.211361)
       + x*vec3(-0.112119,0.866281,-0.16593)
       + y*vec3(0.0514355,0.0009434,-0.0298296)
       + z*vec3(0.0212091,0.0098094,0.0368813);
}
