

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// I combined the shaders from the tutorial "HOWTO Get Started With Ray Marching"
// https://www.shadertoy.com/view/XllGW4 /with Blob physics (forked from LeWIZ)
// https://www.shadertoy.com/view/3lVfzR, which is a simplified version of 
// Refraction Blobs https://www.shadertoy.com/view/4ll3R7. I simplified the tutorial
// by taking out transformations and functions which were not relevant. 
// Supports texture backgrounds currently 
// Click on the shader to move the camera
// Edit content under float makeScene(vec3 pos) to change how the blobs look or add more blobs
// Disclaimer - im new to this. 

#ifdef GL_ES
precision mediump float;
#endif 

uniform vec2 u_resolution;
uniform float u_time;


#define AUTO_ROTATE     // uncomment to stop auto camera rotation
//#define BACKGROUND_BLUE // uncomment for blue background, else cubemap background
//#define VIEW_ZERO       // uncomment to default OpenGL look down z-axis view
//#define VIEW_ISOMETRIC  // Nice isometric camera angle
//#define LOW_Q // uncomment for low quality if your GPU is a potato

#ifdef LOW_Q
    #define MARCHSTEPS 25
#else
    #define MARCHSTEPS 50
    #define AMBIENT_OCCLUSION
    #define DOUBLE_SIDED_TRANSPARENCY
#endif

#define MAX_DIST 10.0

#define SPECULAR
#define REFLECTIONS
#define TRANSPARENCY
#define SHADOWS
#define FOG

#define DIRECTIONAL_LIGHT
#define DIRECTIONAL_LIGHT_FLARE

#define PI 3.141592654

#define kNt  -1.0 //no trans
#define kTt   1.0 //yes trans
#define kIt   0.0 //inverse trans

const float MATERIAL_1 = 1.0;
const float MATERIAL_2 = 2.0;
/* */ float gMaterial  = MATERIAL_1;


// rd Ray Direction
// rl Ray Length
struct sRay   { vec3 ro ; vec3  rd ; float sd; float rl; };
struct sHit   { vec3 hp ; float hd ; vec3 oid; };
struct sSurf  { vec3 nor; vec3  ref; vec3 tra; };
struct sMat   { vec3 ctc; float frs; float smt; vec2 par; float trs; float fri; };
struct sShade { vec3 dfs; vec3  spc; };
struct sLight { vec3 rd ; vec3  col; };





    vec4 opUt( vec4 a, vec4 b, float fts ){
        vec4 vScaled = vec4(b.x * (fts * 2.0 - 1.0), b.yzw);
        return mix(a, vScaled, step(vScaled.x, a.x) * step(0.0, fts));
}



float sphere(vec3 pos) {
    float distanceFromCenter = length(pos);
    float sphereRadius = 1.0;
	return distanceFromCenter - sphereRadius; 
}

float makeBlobs(float s1, float s2, float s3, float s4) {
    float k = -6.0;
    return log( exp(k*s1) + exp(k*s2) + exp(k*s3) + exp(1.5*k*s4)) / k; // THE MAIN EQUATION THAT MAKES THE BLOBS BUD OFF OF EACH OTHER
}

float makeScene(vec3 pos) {
    float blobVelocity = 1.0;
    
    // BECAUSE THIS IS THE CENTER BLOB, THE POSITION IS LEFT EXACTLY AS IT IS
    vec3 posOffset0 = pos + blobVelocity * vec3(sin(iTime * 0.9),cos(iTime *1.9),cos(iTime * 2.0)+sin(0.5));
    float s1 = sphere(posOffset0);
    
    // BECAUSE THIS BLOB ROTATES AROUND THE CENTER BLOB, ITS POSITION IS OFFSET USING SIN/COS OSCILLATION.
    // IF THE CENTER OF THE NEW BLOB IS, FOR EXAMPLE, (2,1) AWAY FROM THE TRUE CENTER, THEN WE CALL THE
    // "SPHERE" FUNCTION AGAIN USING THE NEW OFFSET VALUE OF (2,1).
    vec3 posOffset = pos + 0.1*blobVelocity * vec3(cos(iTime * 0.7),cos(iTime *3.4),cos(iTime * 2.6));
    float s2 = sphere(posOffset);

    vec3 posOffset2 = pos/0.6 + 1.5*blobVelocity * vec3(sin(iTime * 0.6),cos(iTime *2.9),cos(iTime * 1.3)+0.3);
    float s3 = sphere(posOffset2);
    
    vec3 posOffset3 = pos/1.1 + blobVelocity * vec3(sin(iTime * 1.2),cos(iTime *1.9),cos(iTime * 1.2)+0.1);
    float s4 = sphere(posOffset3);
    
    return makeBlobs(s1, s2, s3, s4);
}


// ========================================
vec4 DE( vec3 hp, float fts ) {
    vec4 vResult = vec4(MAX_DIST, -1.0, 0.0, 0.0);
    vec4 vDist = vec4( makeScene(hp), MATERIAL_1, hp.xz);
    vDist.y = gMaterial; // v0.42 draw may over-ride material
    return opUt(vResult, vDist, fts);
}

// ========================================


sMat getMaterial( sHit hitInfo ) {
    sMat mat;
    if(hitInfo.oid.x == MATERIAL_1) {
        mat.frs = 0.31;
        mat.smt = 1.0;
        mat.trs = 1.0;
        mat.fri = 0.75;
        const float fExtinctionScale = 2.0;
        vec3 tc = vec3(0.93,0.96,1.0);        //tex/col
        mat.ctc = (vec3(1.0) - tc) * fExtinctionScale; 
    } else
    if(hitInfo.oid.x == MATERIAL_2) {
        mat.frs = 0.0;
        mat.smt = 1.0;
        mat.trs = 0.0;
        mat.fri = 0.0;
        mat.ctc = vec3(0.25,0.5,0.75); // Beautiful Baby Blue
    }
    return mat;
}

// ========================================
vec3 getBackground( vec3 rd ) {
#ifdef BACKGROUND_BLUE
    const vec3  tc = vec3(0.8824, 0.8824, 0.8824);
    const vec3  cc = tc * 0.5;
          float f  = clamp(rd.y, 0.0, 1.0);
    return mix(cc, tc, f);
#else
    return texture(iChannel0, rd.xy).xyz;
#endif
}

// ========================================
sLight getDirLight() {
    sLight result;
    result.rd  = normalize(vec3(-0.2, -0.3, 0.5));
    result.col = vec3(8.0, 7.5, 7.0);
    return result;
}

// ========================================
vec3 getAmbient( vec3 nor ) {
    return getBackground(nor);
}

// ========================================
vec3 normal( vec3 p, float fts ) {
    vec3 e = vec3(0.01,-0.01,0.0);
    return normalize( vec3(
        e.xyy*DE(p+e.xyy,fts).x +
        e.yyx*DE(p+e.yyx,fts).x +
        e.yxy*DE(p+e.yxy,fts).x +
        e.xxx*DE(p+e.xxx,fts).x)
    );
}
 
// ========================================
void march( sRay ray, out sHit res, int maxIter, float fts ) {
    res.hd = ray.sd;
    res.oid.x = 0.0;

    for( int i=0;i<=MARCHSTEPS;i++ ) {
        res.hp = ray.ro + ray.rd * res.hd;
        vec4 r = DE( res.hp, fts );
        res.oid = r.yzw;
        if((abs(r.x) <= 0.01) || (res.hd >= ray.rl) || (i > maxIter))
            break;
        res.hd = res.hd + r.x;
    }
    if(res.hd >= ray.rl) {
        res.hd = MAX_DIST;
        res.hp = ray.ro + ray.rd * res.hd;
        res.oid.x = 0.0;
    }
}

// ========================================
float getShadow( vec3 hp, vec3 nor, vec3 lrd, float d ) {
#ifdef SHADOWS
    sRay ray;
    ray.rd = lrd;
    ray.ro = hp;
    ray.sd = 0.05 / abs(dot(lrd, nor));
    ray.rl = d - ray.sd;
    sHit si;
    march(ray, si, 32, kNt);
    float s = step(0.0, si.hd) * step(d, si.hd );
    return s;
#else
    return 1.0;
#endif
}

// ========================================
float getAmbientOcclusion( sHit hi, sSurf s ) {
#ifdef AMBIENT_OCCLUSION
    vec3 hp = hi.hp;
    vec3 nor = s.nor;
    float ao = 1.0;

    float d = 0.0;
    for( int i=0; i<=5; i++ ) {
        d += 0.1;
        vec4 r = DE(hp + nor * d, kNt);
        ao *= 1.0 - max(0.0, (d - r.x) * 0.2 / d );
    }
    return ao;
#else
    return 1.0;
#endif
}

// ========================================
vec3 getFog( vec3 color, sRay ray, sHit hi ) {
#ifdef FOG
    float a = exp(hi.hd * - 0.05);
    vec3 fog = getBackground(ray.rd);

    #ifdef DIRECTIONAL_LIGHT_FLARE
        sLight lig = getDirLight();
        float f = clamp(dot(-lig.rd, ray.rd), 0.0, 1.0);
        fog += lig.col * pow(f, 10.0);
    #endif 

    color = mix(fog, color, a);
#endif

    return color;
}

// http://en.wikipedia.org/wiki/Schlick's_approximation
// Anisotropic scattering Schlick phase function
// "Interactive Manycore Photon Mapping"
// See: https://www.scss.tcd.ie/publications/tech-reports/reports.11/TCD-CS-2011-04.pdf
//
// More complex empirically motivated phase functions are efficiently approximated by the Schluck function [BLS93].
// ========================================
float getSchlick(vec3 nor, vec3 v, float frs, float sf) {
    float f = dot(nor, -v);
    f = clamp((1.0 - f), 0.0, 1.0);
    float fDotPow = pow(f, 5.0);
    return frs + (1.0 - frs) * fDotPow * sf;
}

// http://en.wikipedia.org/wiki/Fresnel_equations
// ========================================
vec3 getFresnel( vec3 dif, vec3 spe, vec3 nor, vec3 v, sMat m ) {
    float f = getSchlick(nor, v, m.frs, m.smt * 0.9 + 0.1);
    return mix(dif, spe, f);
}

// ========================================
float getPhong( vec3 ird, vec3 lrd, vec3 nor, float smt ) {
    vec3  v  = normalize(lrd - ird);
    float f  = max(0.0, dot(v, nor));
    float sp = exp2(4.0 + 6.0 * smt);
    float si = (sp + 2.0) * 0.125;
    return pow(f, sp) * si;
}

// ========================================
sShade setDirLight( sLight l, vec3 p, vec3 d, vec3 nor, sMat m ) {
    sShade s;
    vec3 lrd = -l.rd;
    float sf = getShadow( p, nor, lrd, 8.0 );
    vec3 il = l.col * sf * max(0.0, dot(lrd, nor));
    s.dfs = il;
    s.spc = getPhong( d, lrd, nor, m.smt ) * il;
    return s;
}

// ========================================
vec3 setColor( sRay ray, sHit hi, sSurf sc, sMat m ) {
    vec3 color;
    sShade s;
    s.dfs = vec3(0.0);
    s.spc = vec3(0.0);
    float ao = getAmbientOcclusion(hi, sc);
    vec3 al = getAmbient(sc.nor) * ao;
    s.dfs += al;
    s.spc += sc.ref;

#ifdef DIRECTIONAL_LIGHT
    sLight dl = getDirLight();
    sShade sh = setDirLight(dl, hi.hp, ray.rd, sc.nor, m);
    s.dfs += sh.dfs;
    s.spc += sh.spc;
#endif

    vec3 dr = s.dfs * m.ctc;

    dr = mix(dr, sc.tra, m.trs);

#ifdef SPECULAR
    color = getFresnel(dr , s.spc, sc.nor, ray.rd, m);
#else
    color = dr;
#endif

    return color;
}

// ========================================
vec3 getColor( sRay ray ) {
    sHit hi;
    march(ray, hi, 32, kNt);
    vec3 color;

    if(hi.oid.x < 0.5) {
        color = getBackground(ray.rd);
    } else {
        sSurf s;
        s.nor  = normal(hi.hp, kNt);
        sMat m = getMaterial( hi );
        s.ref  = getBackground(reflect(ray.rd, s.nor));
        m.trs  = 0.0;
        color  = setColor(ray, hi, s, m);
    }

    color = getFog(color, ray, hi);
    return color;
}

// ========================================
vec3 getReflection( sRay ray, sHit hitInfo, sSurf s ) {
#ifdef REFLECTIONS
    sRay rRay;
    rRay.rd = reflect(ray.rd, s.nor);
    rRay.ro = hitInfo.hp;
    rRay.rl = 16.0;
    rRay.sd = 0.1 / abs(dot(rRay.rd, s.nor));
    return getColor(rRay);
#else
    return getBackground(reflect(ray.rd, s.nor));
#endif
}

// ========================================
vec3 getTransparency( sRay ray, sHit hit, sSurf s, sMat m ) {
#ifdef TRANSPARENCY
    sRay rRay;
    rRay.rd = refract(ray.rd, s.nor, m.fri);
    rRay.ro = hit.hp;
    rRay.rl = 16.0;
    rRay.sd = 0.05 / abs(dot(rRay.rd, s.nor));

    #ifdef DOUBLE_SIDED_TRANSPARENCY
        sHit hit2;
        march(rRay, hit2, 32, kIt);
        vec3 nor = normal(hit2.hp, kIt);
            sRay rRay2;
            rRay2.rd = refract(rRay.rd, nor, 1.0 / m.fri);
            rRay2.ro = hit2.hp;
            rRay2.rl = 16.0;
            rRay2.sd = 0.0;
        float ed = hit2.hd;
        vec3 color = getColor( rRay2 );
    #else
        vec3 color = getColor( rRay );
        float ed = 0.5;
    #endif

    return color * clamp(exp(-(m.ctc * ed)),0.0,1.0);
#else
    return getBackground(reflect(ray.rd, s.nor));
#endif
}

// ========================================
vec3 getRayColor( sRay ray ) {
    sHit i;
    march(ray, i, MARCHSTEPS, kTt); //256

    vec3 color;
    if(i.oid.x < 0.5) {
        color = getBackground(ray.rd);
    } else  {
        sSurf s;
        s.nor  = normal(i.hp, kTt);
        sMat m = getMaterial( i );
        s.ref  = getReflection(ray, i, s);
        if(m.trs > 0.0) s.tra = getTransparency(ray, i, s, m);
        color  = setColor(ray, i, s, m);
    }

    getFog(color, ray, i); // BUG? Is this intentional that color is not updated??
    return color;
}

// ========================================
sRay setCameraRay( vec3 hp, vec3 i , vec2 fragCoord) {
    float fRatio = iResolution.x / iResolution.y; // Aspect Ratio

    vec3 f   = normalize(i - hp);
    vec3 vUp = vec3(0.0, 1.0, 0.0);
    vec2 vvc = 2.*fragCoord.xy/iResolution.xy-1.;
    vvc.y /= fRatio;

    sRay ray;
    ray.ro = hp;
    vec3 r = normalize(cross(f, vUp));
    vUp    = cross(r, f);
    ray.rd = normalize( r * vvc.x + vUp * vvc.y + f);
    ray.sd = 0.0;
    ray.rl = MAX_DIST;
    return ray;
}

// ========================================
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 m = vec2(0.0); // Default OpenGL camera: Look down -z axis

#ifdef VIEW_ISOMETRIC
    m = vec2( 3.5, 1.0 ) / PI; // fake isoemetric
#else
  #ifdef VIEW_ZERO
    // m.x = 0.0; // +z // h -> 0      =   0
    // m.x =+1.0;; //-z // h -> PI     = 180
    // m.x = 0.5; // +x // h -> PI  /2 =  90
    // m.x =-0.5; // -x // h -> PI*3/2 = 270
    //m.y = iMouse.y / iResolution.y; // uncomment to allow Y rotation
  #else
    m += 2.* iMouse.xy / iResolution.xy;
    m.x += 1.;
  #endif // ZERO
#endif // ISOMETRIC

    float nRotate = 0.0; // no rotation
#ifdef AUTO_ROTATE
    nRotate = iTime *0.05; // slow rotation
#endif

    //float h  = mix(0.0, PI , m.x - nRotate);
    float h  = PI * (m.x - nRotate);
    float e  = mix(0.0, 2.5, m.y                ); // eye
    // Hold down mouse button to zoom out & rotate the camera!
    float d  = mix(2.5, 2.5 + (iMouse.z > 0.0 ? 0.1 : 0.1), m.y); // eye distance

    // ro RayOrigin
    vec3 ro  = vec3(sin(h) *cos(e), sin(e), cos(h) * cos(e)) * d*2.0;
    vec3 ta  = vec3(0.0, 0.0, 0.0);

    sRay ray = setCameraRay( ta + ro, ta, fragCoord);
    vec3 col = getRayColor( ray );
    fragColor = vec4( col, 1.0 );
}