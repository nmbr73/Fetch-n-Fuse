
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: London' to iChannel0


// I combined the shaders from the tutorial "HOWTO Get Started With Ray Marching"
// https://www.shadertoy.com/view/XllGW4 /with Blob physics (forked from LeWIZ)
// https://www.shadertoy.com/view/3lVfzR, which is a simplified version of 
// Refraction Blobs https://www.shadertoy.com/view/4ll3R7. I simplified the tutorial
// by taking out transformations and functions which were not relevant. 
// Supports texture backgrounds currently 
// Click on the shader to move the camera
// Edit content under float makeScene(float3 pos) to change how the blobs look or add more blobs
// Disclaimer - im new to this. 

#ifdef GL_ES
//precision mediump float;
#endif 

//uniform float2 u_resolution;
//uniform float u_time;


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

#define MAX_DIST 10.0f

#define SPECULAR
#define REFLECTIONS
#define TRANSPARENCY
#define SHADOWS
#define FOG

#define DIRECTIONAL_LIGHT
#define DIRECTIONAL_LIGHT_FLARE

#define PI 3.141592654f

#define kNt  -1.0f //no trans
#define kTt   1.0f //yes trans
#define kIt   0.0f //inverse trans
#define MATERIAL_1  1.0f
#define MATERIAL_2  2.0f
//float gMaterial  = MATERIAL_1;


// rd Ray Direction
// rl Ray Length
struct sRay   { float3 ro ; float3  rd ; float sd; float rl; };
struct sHit   { float3 hp ; float hd ; float3 oid; };
struct sSurf  { float3 nor; float3  ref; float3 tra; };
struct sMat   { float3 ctc; float frs; float smt; float2 par; float trs; float fri; };
struct sShade { float3 dfs; float3  spc; };
struct sLight { float3 rd ; float3  col; };





__DEVICE__ float4 opUt( float4 a, float4 b, float fts ){
        float4 vScaled = to_float4(b.x * (fts * 2.0f - 1.0f), b.y,b.z,b.w);
        return _mix(a, vScaled, step(vScaled.x, a.x) * step(0.0f, fts));
}



__DEVICE__ float sphere(float3 pos) {
    float distanceFromCenter = length(pos);
    float sphereRadius = 1.0f;
  return distanceFromCenter - sphereRadius; 
}

__DEVICE__ float makeBlobs(float s1, float s2, float s3, float s4) {
    float k = -6.0f;
    return _logf( _expf(k*s1) + _expf(k*s2) + _expf(k*s3) + _expf(1.5f*k*s4)) / k; // THE MAIN EQUATION THAT MAKES THE BLOBS BUD OFF OF EACH OTHER
}

__DEVICE__ float makeScene(float3 pos, float iTime) {
    float blobVelocity = 1.0f;
    
    // BECAUSE THIS IS THE CENTER BLOB, THE POSITION IS LEFT EXACTLY AS IT IS
    float3 posOffset0 = pos + blobVelocity * to_float3(_sinf(iTime * 0.9f),_cosf(iTime *1.9f),_cosf(iTime * 2.0f)+_sinf(0.5f));
    float s1 = sphere(posOffset0);
    
    // BECAUSE THIS BLOB ROTATES AROUND THE CENTER BLOB, ITS POSITION IS OFFSET USING SIN/COS OSCILLATION.
    // IF THE CENTER OF THE NEW BLOB IS, FOR EXAMPLE, (2,1) AWAY FROM THE TRUE CENTER, THEN WE CALL THE
    // "SPHERE" FUNCTION AGAIN USING THE NEW OFFSET VALUE OF (2,1).
    float3 posOffset = pos + 0.1f*blobVelocity * to_float3(_cosf(iTime * 0.7f),_cosf(iTime *3.4f),_cosf(iTime * 2.6f));
    float s2 = sphere(posOffset);

    float3 posOffset2 = pos/0.6f + 1.5f*blobVelocity * to_float3(_sinf(iTime * 0.6f),_cosf(iTime *2.9f),_cosf(iTime * 1.3f)+0.3f);
    float s3 = sphere(posOffset2);
    
    float3 posOffset3 = pos/1.1f + blobVelocity * to_float3(_sinf(iTime * 1.2f),_cosf(iTime *1.9f),_cosf(iTime * 1.2f)+0.1f);
    float s4 = sphere(posOffset3);
    
    return makeBlobs(s1, s2, s3, s4);
}


// ========================================
__DEVICE__ float4 DE( float3 hp, float fts, float iTime, float gMaterial ) {
    float4 vResult = to_float4(MAX_DIST, -1.0f, 0.0f, 0.0f);
    float4 vDist = to_float4( makeScene(hp, iTime), MATERIAL_1, hp.x,hp.z);
    vDist.y = gMaterial; // v0.42 draw may over-ride material
    return opUt(vResult, vDist, fts);
}

// ========================================


__DEVICE__ struct sMat getMaterial( struct sHit hitInfo ) {
    struct sMat mat;
    if(hitInfo.oid.x == MATERIAL_1) {
        mat.frs = 0.31f;
        mat.smt = 1.0f;
        mat.trs = 1.0f;
        mat.fri = 0.75f;
        const float fExtinctionScale = 2.0f;
        float3 tc = to_float3(0.93f,0.96f,1.0f);        //tex/col
        mat.ctc = (to_float3_s(1.0f) - tc) * fExtinctionScale; 
    } else
    if(hitInfo.oid.x == MATERIAL_2) {
        mat.frs = 0.0f;
        mat.smt = 1.0f;
        mat.trs = 0.0f;
        mat.fri = 0.0f;
        mat.ctc = to_float3(0.25f,0.5f,0.75f); // Beautiful Baby Blue
    }
    return mat;
}

// ========================================
__DEVICE__ float3 getBackground( float3 rd, float4 ratio, __TEXTURE2D__ iChannel0 ) {
#ifdef BACKGROUND_BLUE
    const float3  tc = to_float3(0.8824f, 0.8824f, 0.8824f);
    const float3  cc = tc * 0.5f;
          float f  = clamp(rd.y, 0.0f, 1.0f);
    return _mix(cc, tc, f);
#else
    return swi3(_tex2DVecN(iChannel0, (rd.x*ratio.w+ratio.x)*ratio.z,(rd.y+ratio.y)*ratio.z,15),x,y,z);
#endif
}

// ========================================
__DEVICE__ struct sLight getDirLight() {
    struct sLight result;
    result.rd  = normalize(to_float3(-0.2f, -0.3f, 0.5f));
    result.col = to_float3(8.0f, 7.5f, 7.0f);
    return result;
}

// ========================================
__DEVICE__ float3 getAmbient( float3 nor, float4 ratio, __TEXTURE2D__ iChannel0 ) {
    return getBackground(nor, ratio, iChannel0);
}

// ========================================
__DEVICE__ float3 normal( float3 p, float fts, float iTime, float gMaterial ) {
    float3 e = to_float3(0.01f,-0.01f,0.0f);
    return normalize( (
        swi3(e,x,y,y)*DE(p+swi3(e,x,y,y),fts,iTime, gMaterial).x +
        swi3(e,y,y,x)*DE(p+swi3(e,y,y,x),fts,iTime, gMaterial).x +
        swi3(e,y,x,y)*DE(p+swi3(e,y,x,y),fts,iTime, gMaterial).x +
        swi3(e,x,x,x)*DE(p+swi3(e,x,x,x),fts,iTime, gMaterial).x)
    );
}
 
// ========================================
__DEVICE__ struct sHit march( struct sRay ray, struct sHit res, int maxIter, float fts, float iTime, float gMaterial ) {
    res.hd = ray.sd;
    res.oid.x = 0.0f;

    for( int i=0;i<=MARCHSTEPS;i++ ) {
        res.hp = ray.ro + ray.rd * res.hd;
        float4 r = DE( res.hp, fts,iTime, gMaterial );
        res.oid = swi3(r,y,z,w);
        if((_fabs(r.x) <= 0.01f) || (res.hd >= ray.rl) || (i > maxIter))
            break;
        res.hd = res.hd + r.x;
    }
    if(res.hd >= ray.rl) {
        res.hd = MAX_DIST;
        res.hp = ray.ro + ray.rd * res.hd;
        res.oid.x = 0.0f;
    }
	
    return res;	
}

// ========================================
__DEVICE__ float getShadow( float3 hp, float3 nor, float3 lrd, float d, float iTime, float gMaterial ) {
#ifdef SHADOWS
    struct sRay ray;
    ray.rd = lrd;
    ray.ro = hp;
    ray.sd = 0.05f / _fabs(dot(lrd, nor));
    ray.rl = d - ray.sd;
    struct sHit si;
    si = march(ray, si, 32, kNt, iTime, gMaterial);
    float s = step(0.0f, si.hd) * step(d, si.hd );
    return s;
#else
    return 1.0f;
#endif
}

// ========================================
__DEVICE__ float getAmbientOcclusion( struct sHit hi, struct sSurf s, float iTime, float gMaterial ) {
#ifdef AMBIENT_OCCLUSION
    float3 hp = hi.hp;
    float3 nor = s.nor;
    float ao = 1.0f;

    float d = 0.0f;
    for( int i=0; i<=5; i++ ) {
        d += 0.1f;
        float4 r = DE(hp + nor * d, kNt, iTime, gMaterial);
        ao *= 1.0f - _fmaxf(0.0f, (d - r.x) * 0.2f / d );
    }
    return ao;
#else
    return 1.0f;
#endif
}

// ========================================
__DEVICE__ float3 getFog( float3 color, struct sRay ray, struct sHit hi, float4 ratio, __TEXTURE2D__ iChannel0 ) {
#ifdef FOG
    float a = _expf(hi.hd * - 0.05f);
    float3 fog = getBackground(ray.rd, ratio, iChannel0);

    #ifdef DIRECTIONAL_LIGHT_FLARE
        struct sLight lig = getDirLight();
        float f = clamp(dot(-lig.rd, ray.rd), 0.0f, 1.0f);
        fog += lig.col * _powf(f, 10.0f);
    #endif 

    color = _mix(fog, color, a);
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
__DEVICE__ float getSchlick(float3 nor, float3 v, float frs, float sf) {
    float f = dot(nor, -v);
    f = clamp((1.0f - f), 0.0f, 1.0f);
    float fDotPow = _powf(f, 5.0f);
    return frs + (1.0f - frs) * fDotPow * sf;
}

// http://en.wikipedia.org/wiki/Fresnel_equations
// ========================================
__DEVICE__ float3 getFresnel( float3 dif, float3 spe, float3 nor, float3 v, struct sMat m ) {
    float f = getSchlick(nor, v, m.frs, m.smt * 0.9f + 0.1f);
    return _mix(dif, spe, f);
}

// ========================================
__DEVICE__ float getPhong( float3 ird, float3 lrd, float3 nor, float smt ) {
    float3  v  = normalize(lrd - ird);
    float f  = _fmaxf(0.0f, dot(v, nor));
    float sp = _exp2f(4.0f + 6.0f * smt);
    float si = (sp + 2.0f) * 0.125f;
    return _powf(f, sp) * si;
}

// ========================================
__DEVICE__ struct sShade setDirLight( struct sLight l, float3 p, float3 d, float3 nor, struct sMat m, float iTime, float gMaterial ) {
    struct sShade s;
    float3 lrd = -l.rd;
    float sf = getShadow( p, nor, lrd, 8.0f, iTime, gMaterial );
    float3 il = l.col * sf * _fmaxf(0.0f, dot(lrd, nor));
    s.dfs = il;
    s.spc = getPhong( d, lrd, nor, m.smt ) * il;
    return s;
}

// ========================================
__DEVICE__ float3 setColor( struct sRay ray, struct sHit hi, struct sSurf sc, struct sMat m, float iTime, float gMaterial, float4 ratio, __TEXTURE2D__ iChannel0 ) {
    float3 color;
    struct sShade s;
    s.dfs = to_float3_s(0.0f);
    s.spc = to_float3_s(0.0f);
    float ao = getAmbientOcclusion(hi, sc, iTime, gMaterial);
    float3 al = getAmbient(sc.nor, ratio, iChannel0) * ao;
    s.dfs += al;
    s.spc += sc.ref;

#ifdef DIRECTIONAL_LIGHT
    struct sLight dl = getDirLight();
    struct sShade sh = setDirLight(dl, hi.hp, ray.rd, sc.nor, m, iTime, gMaterial);
    s.dfs += sh.dfs;
    s.spc += sh.spc;
#endif

    float3 dr = s.dfs * m.ctc;

    dr = _mix(dr, sc.tra, m.trs);

#ifdef SPECULAR
    color = getFresnel(dr , s.spc, sc.nor, ray.rd, m);
#else
    color = dr;
#endif

    return color;
}

// ========================================
__DEVICE__ float3 getColor( struct sRay ray, float iTime, float gMaterial, float4 ratio, __TEXTURE2D__ iChannel0 ) {
    struct sHit hi;
    hi = march(ray, hi, 32, kNt, iTime, gMaterial);
    float3 color;

    if(hi.oid.x < 0.5f) {
        color = getBackground(ray.rd, ratio, iChannel0);
    } else {
        struct sSurf s;
        s.nor  = normal(hi.hp, kNt, iTime, gMaterial);
        struct sMat m = getMaterial( hi );
        s.ref  = getBackground(reflect(ray.rd, s.nor), ratio, iChannel0);
        m.trs  = 0.0f;
        color  = setColor(ray, hi, s, m, iTime, gMaterial, ratio, iChannel0);
    }

    color = getFog(color, ray, hi, ratio, iChannel0);
    return color;
}

// ========================================
__DEVICE__ float3 getReflection( struct sRay ray, struct sHit hitInfo, struct sSurf s, float iTime, float gMaterial, float4 ratio, __TEXTURE2D__ iChannel0 ) {
#ifdef REFLECTIONS
    struct sRay rRay;
    rRay.rd = reflect(ray.rd, s.nor);
    rRay.ro = hitInfo.hp;
    rRay.rl = 16.0f;
    rRay.sd = 0.1f / _fabs(dot(rRay.rd, s.nor));
    return getColor(rRay, iTime, gMaterial, ratio, iChannel0);
#else
    return getBackground(reflect(ray.rd, s.nor), rqatio, iChannel0);
#endif
}

// ========================================
__DEVICE__ float3 getTransparency( struct sRay ray, struct sHit hit, struct sSurf s, struct sMat m, float iTime, float gMaterial, float4 ratio, __TEXTURE2D__ iChannel0 ) {
#ifdef TRANSPARENCY
    struct sRay rRay;
    rRay.rd = refract_f3(ray.rd, s.nor, m.fri);
    rRay.ro = hit.hp;
    rRay.rl = 16.0f;
    rRay.sd = 0.05f / _fabs(dot(rRay.rd, s.nor));

    #ifdef DOUBLE_SIDED_TRANSPARENCY
        struct sHit hit2;
        hit2 = march(rRay, hit2, 32, kIt, iTime, gMaterial);
        float3 nor = normal(hit2.hp, kIt, iTime, gMaterial);
            struct sRay rRay2;
            rRay2.rd = refract_f3(rRay.rd, nor, 1.0f / m.fri);
            rRay2.ro = hit2.hp;
            rRay2.rl = 16.0f;
            rRay2.sd = 0.0f;
        float ed = hit2.hd;
        float3 color = getColor( rRay2, iTime, gMaterial, ratio, iChannel0 );
    #else
        float3 color = getColor( rRay, iTime, gMaterial, ratio, iChannel0 );
        float ed = 0.5f;
    #endif

    return color * clamp(exp_f3(-1.0f*(m.ctc * ed)),0.0f,1.0f);
#else
    return getBackground(reflect(ray.rd, s.nor, ratio, iChannel0));
#endif
}

// ========================================
__DEVICE__ float3 getRayColor( struct sRay ray, float iTime, float gMaterial, float4 ratio, __TEXTURE2D__ iChannel0 ) {
    struct sHit i;
    i = march(ray, i, MARCHSTEPS, kTt, iTime, gMaterial); //256

    float3 color;
    if(i.oid.x < 0.5f) {
        color = getBackground(ray.rd, ratio, iChannel0);
    } else  {
        struct sSurf s;
        s.nor  = normal(i.hp, kTt, iTime, gMaterial);
        struct sMat m = getMaterial( i );
        s.ref  = getReflection(ray, i, s, iTime, gMaterial, ratio, iChannel0);
        if(m.trs > 0.0f) s.tra = getTransparency(ray, i, s, m, iTime, gMaterial, ratio, iChannel0);
        color  = setColor(ray, i, s, m, iTime, gMaterial, ratio, iChannel0);
    }

    getFog(color, ray, i, ratio, iChannel0); // BUG? Is this intentional that color is not updated??
    return color;
}

// ========================================
__DEVICE__ struct sRay setCameraRay( float3 hp, float3 i , float2 fragCoord, float2 iResolution) {
    float fRatio = iResolution.x / iResolution.y; // Aspect Ratio

    float3 f   = normalize(i - hp);
    float3 vUp = to_float3(0.0f, 1.0f, 0.0f);
    float2 vvc = 2.0f*fragCoord/iResolution-1.0f;
    vvc.y /= fRatio;

    struct sRay ray;
    ray.ro = hp;
    float3 r = normalize(cross(f, vUp));
    vUp    = cross(r, f);
    ray.rd = normalize( r * vvc.x + vUp * vvc.y + f);
    ray.sd = 0.0f;
    ray.rl = MAX_DIST;
    return ray;
}

// ========================================
__KERNEL__ void VariableBlobsJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
  
  CONNECT_POINT0(TexPos, 0.0f, 0.0f );
  CONNECT_SLIDER0(TexScale, -10.0f, 10.0f, 1.0f);
  
    float gMaterial  = MATERIAL_1;

    float2 m = to_float2_s(0.0f); // Default OpenGL camera: Look down -z axis

    float4 ratio = to_float4(TexPos.x,TexPos.y,TexScale,iResolution.y/iResolution.x);

#ifdef VIEW_ISOMETRIC
    m = to_float2( 3.5f, 1.0f ) / PI; // fake isoemetric
#else
  #ifdef VIEW_ZERO
    // m.x = 0.0f; // +z // h -> 0      =   0
    // m.x =+1.0f;; //-z // h -> PI     = 180
    // m.x = 0.5f; // +x // h -> PI  /2 =  90
    // m.x =-0.5f; // -x // h -> PI*3/2 = 270
    //m.y = iMouse.y / iResolution.y; // uncomment to allow Y rotation
  #else
    m += 2.0f* swi2(iMouse,x,y) / iResolution;
    m.x += 1.0f;
  #endif // ZERO
#endif // ISOMETRIC

    float nRotate = 0.0f; // no rotation
#ifdef AUTO_ROTATE
    nRotate = iTime *0.05f; // slow rotation
#endif

    //float h  = _mix(0.0f, PI , m.x - nRotate);
    float h  = PI * (m.x - nRotate);
    float e  = _mix(0.0f, 2.5f, m.y ); // eye
    // Hold down mouse button to zoom out & rotate the camera!
    float d  = _mix(2.5f, 2.5f + (iMouse.z > 0.0f ? 0.1f : 0.1f), m.y); // eye distance

    // ro RayOrigin
    float3 ro  = to_float3(_sinf(h) *_cosf(e), _sinf(e), _cosf(h) * _cosf(e)) * d*2.0f;
    float3 ta  = to_float3(0.0f, 0.0f, 0.0f);

    struct sRay ray = setCameraRay( ta + ro, ta, fragCoord, iResolution);
    float3 col = getRayColor( ray, iTime, gMaterial, ratio, iChannel0 );
    fragColor = to_float4_aw( col, 1.0f );

  SetFragmentShaderComputedColor(fragColor);
}