

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// raymarching based from https://www.shadertoy.com/view/wdGGz3
#define MAX_STEPS 256
#define MAX_DIST 256.
#define SURF_DIST .0005
#define Rot(a) mat2(cos(a),-sin(a),sin(a),cos(a))
#define antialiasing(n) n/min(iResolution.y,iResolution.x)
#define S(d,b) smoothstep(antialiasing(1.0),b,d)
#define B(p,s) max(abs(p).x-s.x,abs(p).y-s.y)
#define MATERIAL 0

#define ZERO (min(iFrame,0))

vec3 N33(vec3 p) {
    vec3 a = fract(p*vec3(123.34,234.34,345.65));
    a+=dot(a,a+34.45);
    return fract(vec3(a.x*a.y,a.y*a.z,a.z*a.x));
}

float smin( float a, float b, float k ) {
    float h = clamp( 0.5+0.5*(b-a)/k, 0., 1. );
    return mix( b, a, h ) - k*h*(1.0-h);
}

float hash(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

float metaball(vec3 p, float i, float t) {
    vec3 n = N33(vec3(i));
    vec3 p2 = sin(n*t)*0.2;
    vec3 spp = p-p2;
    float sp = length(spp)-0.01;
    return sp;
}

vec2 GetDist(vec3 p) {

    float k = 0.7;
    float d = 10.0;
    float t = iTime*3.0;

    d = smin(d,metaball(p,0.3, t),k); 
    d = smin(d,metaball(p,0.6, t),k); 
    d = smin(d,metaball(p,0.9, t),k); 

    vec2 model = vec2(d,MATERIAL);
    return model;
}

vec2 RayMarch(vec3 ro, vec3 rd, float side, int stepnum) {
    vec2 dO = vec2(0.0);
    
    for(int i=0; i<stepnum; i++) {
        vec3 p = ro + rd*dO.x;
        vec2 dS = GetDist(p);
        dO.x += dS.x*side;
        dO.y = dS.y;
        
        if(dO.x>MAX_DIST || abs(dS.x)<SURF_DIST) break;
    }
    
    return dO;
}

vec3 GetNormal(vec3 p) {
    float d = GetDist(p).x;
    vec2 e = vec2(.001, 0);
    
    vec3 n = d - vec3(
        GetDist(p-e.xyy).x,
        GetDist(p-e.yxy).x,
        GetDist(p-e.yyx).x);
    
    return normalize(n);
}

vec3 R(vec2 uv, vec3 p, vec3 l, float z) {
    vec3 f = normalize(l-p),
        r = normalize(cross(vec3(0,1,0), f)),
        u = cross(f,r),
        c = p+f*z,
        i = c + uv.x*r + uv.y*u,
        d = normalize(i-p);
    return d;
}

// thx iq! https://iquilezles.org/articles/distfunctions2d/
float sdBox( in vec2 p, in vec2 b )
{
    vec2 d = abs(p)-b;
    return length(max(d,0.0)) + min(max(d.x,d.y),0.0);
}


float charS(vec2 p){
    vec2 prevP = p;
    p.y*=1.5;
    float d = abs(length(p-vec2(-0.02,0.06))-0.06)-0.02;
    float d2 = B(p-vec2(0.03,0.02),vec2(0.045,0.04));
    d = max(-d2,d);
    
    d2 = abs(length(p-vec2(-0.02,-0.06))-0.06)-0.02;
    float d3 = B(p-vec2(-0.06,-0.02),vec2(0.045,0.04));
    d2 = max(-d3,d2);
    
    d = min(d,d2);
    return d;
}

float charH(vec2 p){
    vec2 prevP = p;
    p.y*=1.5;
    p.x = abs(p.x)-0.06;
    float d = B(p,vec2(0.02,0.14));
    p = prevP;
    float d2 = B(p,vec2(0.08,0.02));
    d = min(d,d2);
    return d;
}

float charA(vec2 p){
    vec2 prevP = p;
    p.y*=1.5;
    p.x = abs(p.x)-0.04;
    p*=Rot(radians(-15.0));
    float d = B(p,vec2(0.02,0.16));
    p = prevP;
    float d2 = B(p-vec2(0.0,-0.03),vec2(0.05,0.02));
    d = min(d,d2);
    d = max((abs(p.y)-0.09),d);
    p = prevP;
    p*=Rot(radians(22.0));
    d2 =  B(p-vec2(-0.037,-0.12),vec2(0.019,0.12));
    return d;
}

float charD(vec2 p){
    vec2 prevP = p;
    p.y*=1.5;
    float d = abs(sdBox(p,vec2(0.02,0.075))-0.04)-0.02;
    d = max(-p.x-0.03,d);
    float d2 = B(p-vec2(-0.05,0.0),vec2(0.02,0.135));
    d = min(d,d2);
    return d;
}

float charE(vec2 p){
    vec2 prevP = p;
    p.y*=1.5;
    float d = B(p,vec2(0.065,0.13));
    p.y = abs(p.y)-0.055;
    float d2 = B(p-vec2(0.03,0.0),vec2(0.065,0.03));
    d = max(-d2,d);
    return d;
}

float charR(vec2 p){
    vec2 prevP = p;
    p.y*=1.5;
    float d = abs(sdBox(p-vec2(-0.01,0.05),vec2(0.03,0.022))-0.04)-0.023;
    d = max(-p.x-0.03,d);
    float d2 = B(p-vec2(-0.05,0.0),vec2(0.02,0.135));
    d = min(d,d2);
    p*=Rot(radians(-20.0));
    d2 = B(p-vec2(0.02,-0.14),vec2(0.02,0.13));
    p*=Rot(radians(20.0));
    d2 = max(-p.y-0.132,d2);
    
    d = min(d,d2);
    return d;
}

float shaderText(vec2 p){
    vec2 prevP = p;
    p.y*=0.9;
    float d = charS(p-vec2(-0.5,0.0));
    float d2 = charH(p-vec2(-0.34,0.0));
    d = min(d,d2);
    d2 = charA(p-vec2(-0.14,0.0));
    d = min(d,d2);
    d2 = charD(p-vec2(0.05,0.0));
    d = min(d,d2);
    d2 = charE(p-vec2(0.22,0.0));
    d = min(d,d2);
    d2 = charR(p-vec2(0.38,0.0));
    d = min(d,d2);
    return d;
}

vec3 drawBg(vec2 p, vec3 col){
    float d = shaderText(p*1.7);
    col = mix(col,vec3(0.0),S(d,0.0));
    return col;
}

vec3 reflectMaterial(vec3 p, vec3 rd, vec3 n) {
    vec3 r = reflect(rd,n);
    
    vec3 refTex = drawBg(p.xy,vec3(max(0.95,r.y)))+(r*sin(iTime)*0.5);

    return refTex;
}

vec3 materials(int mat, vec3 n, vec3 rd, vec3 p, vec3 col){
    if(mat == MATERIAL){
        col = reflectMaterial(p,rd,n);
    }
    return col;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (fragCoord-.5*iResolution.xy)/iResolution.y;
    vec2 prevUV = uv;
    vec2 m =  iMouse.xy/iResolution.xy;
    
    vec3 ro = vec3(0, 0, -1.0);
    
    vec3 rd = R(uv, ro, vec3(0,0.0,0.0), 1.0);
    vec2 d = RayMarch(ro, rd, 1.,MAX_STEPS);
    vec3 col = vec3(1.0);
    
    if(d.x<MAX_DIST) {
        vec3 p = ro + rd * d.x;
        vec3 n = GetNormal(p);
        int mat = int(d.y);
        col = materials(mat,n,rd,p,col);
    } else {
        col = drawBg(uv,col);
    }
    
    // gamma correction
    col = pow( col, vec3(0.9545) );    

    fragColor = vec4(col,1.0);
}