
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Gray Noise Small' to iChannel1
// Connect Image 'Cubemap: St Peters Basilica_0' to iChannel0


// PHYSICALLY-BASED REAL-TIME SOAP BUBBLE
// @author: Matteo Mannino
// @about:
// This simulates the interference for 81 wavelengths of light on a soap bubble film.
// The idea was to use an RGB-to-Spectrum->FILTER->Spectrum-to-RGB process:
// 1.0f The RGB-to-Spectrum filter comes from an assumed camera model and pseudo-inverse conversion approach
// 2.0f The filter is the from Andrew Glassner's notebook. It gives the approximation of the power attenuation-per-wavelength
//    that the interference creates given the film width and incident angle. 81 wavelengths are used. 
//    Andrew Glassner's notebook on Soap Bubbles (part 2): https://www.glassner.com/wp-content/uploads/2014/04/CG-CGA-PDF-00-11-Soap-Bubbles-2-Nov00.pdf
//    (you can compare figure 17-18 to this simulation)
// 3.0f For the final color, transform the Spectrum back to RGB
//
// The above process is condensed into a single filter function. Instead of summing over 81 wavelength coefficents, the fourier
// coefficients of the filter are used (13 for cos and sin components each), and the fourier representation of the function
// is used to evaluate the entire function. This is a vastly more efficient evaluation.
//
// As in Glassner's notes, the film width is thin on top and thick on the bottom (set to vary between 150nm on top and 700 on bottom)
//
// The surface micro-sloshing is simulated using time-varying 3d warp noise, 
// as described here: https://iquilezles.org/articles/warp
// Only one level of warp is used.
//
// The bubble geometry is just 6 spheres aligned on each axis,randomly jittering, interpolated together
// The ray-trace function returns both the front and backside of the sphere, so reflections can be computed for both.
//
// License: Creative Commons Attribution-NonCommercial 4.0f International

// HDR VARS (need to fake an HDR envmap for the image-based reflections)
#define hdrfunc(x) (_expf(1.2f*(x))-1.0f)
#define whitesatval hdrfunc(1.0f)

// SOAP REFRACTION VARS
const float R_0 = 0.0278f;
const float nu = 1.4f;
const float minfilmwidth = 150.0f;
const float maxfilmwidth = 700.0f;
const float varfilmwidth = 20.0f;

// RAY-TRACE EPSILON
#define eps 0.001f

//SOAP BUBBLE GEOMETRY VARS
#define PI 3.141592653589793f
const float RADIUS = 0.8f;
const float MAX_DEPTH = 9999999.0f;
const float THRESH_DEPTH = 0.05f;
const float DX = 0.10f;
const float VX = 0.04f;
const float sDX = 0.01f;
const float sVX = 0.04f;

// TRANSFORMS FOR RGB-to-SPECTRUM-FILTER-SPECTRUM-to-RGB
mat3 sparsespfiltconst;
mat3 sparsespfilta[13];
mat3 sparsespfiltb[13];

// Fractal Brownian Motion
__DEVICE__ float fBm(float3 p, __TEXTURE2D__ iChannel1) {
  float v = 0.0f;
    float amplitude = 4.0f;
    float scale = 1.0f;
    int octaves = 2;
    for(int i = 0; i < octaves; ++i) {
      //v += amplitude*texture(iChannel1, scale*swi3(p,x,y,z)).r;
        v += amplitude*_tex2DVecN(iChannel1, scale*p.x, scale*p.y).x;
        amplitude *= 0.5f;
        scale *= 2.0f;
    }
    return v;
}

// 1 level of warp noise for micro waves on bubble surface
__DEVICE__ float warpnoise3(float3 p, float iTime, __TEXTURE2D__ iChannel1) {
    float f = 0.0f;
    const float c1 = 0.06f;
    const float tc = 0.05f;
    float3 q = to_float3(fBm(p + tc*iTime, iChannel1), 
                         fBm(p + to_float3(5.1f, 1.3f, 2.2f) + tc*iTime, iChannel1), 
                         fBm(p + to_float3(3.4f, 4.8f, 5.5f) + tc*iTime), iChannel1);
    
    return 1.2f*fBm(p + c1*q, iChannel1);
}

// Pre-computed coefficients for spectral response
__DEVICE__ void initialize_sparse_spectral_transforms()
{
  sparsespfiltconst = mat3(to_float3(997.744490776777870f, 0.000000000000000f, 0.000000000000000f), to_float3(0.000000000000000f, 1000.429230968840700f, 0.000000000000000f), to_float3(0.000000000000000f, 0.000000000000000f, 1000.314923254210300f));
  sparsespfilta[0] = mat3(to_float3(-9.173541963568921f, 0.000000000000000f, 0.000000000000000f), to_float3(0.000000000000000f, 0.000000000000000f, 0.000000000000000f), to_float3(0.000000000000000f, 0.000000000000000f, 0.000000000000000f));
  sparsespfilta[1] = mat3(to_float3(-12.118820092848431f, 0.000000000000000f, 0.000000000000000f), to_float3(0.000000000000000f, 0.362717643641774f, 0.000000000000000f), to_float3(0.000000000000000f, 0.000000000000000f, 0.000000000000000f));
  sparsespfilta[2] = mat3(to_float3(-18.453733912103289f, 0.000000000000000f, 0.000000000000000f), to_float3(0.000000000000000f, 1.063838675818334f, 0.000000000000000f), to_float3(0.000000000000000f, 0.000000000000000f, 0.000000000000000f));
  sparsespfilta[3] = mat3(to_float3(-448.414255038845680f, -26.846846493079958f, 0.000000000000000f), to_float3(94.833575999184120f, 9.525075729872752f, 0.000000000000000f), to_float3(-48.773853498042200f, 0.000000000000000f, -0.416692876008104f));
  sparsespfilta[4] = mat3(to_float3(6.312176276235818f, -29.044711065580177f, 0.000000000000000f), to_float3(-187.629408328884550f, -359.908263134928520f, 0.000000000000000f), to_float3(0.000000000000000f, 25.579031651446712f, -0.722360089703890f));
  sparsespfilta[5] = mat3(to_float3(-33.547962219868452f, 61.587972582979901f, 0.000000000000000f), to_float3(97.565538879460178f, -150.665614921761320f, -30.220477643983013f), to_float3(1.552347379820659f, -0.319166631512109f, -0.935186347338915f));
  sparsespfilta[6] = mat3(to_float3(3.894757056395064f, 0.000000000000000f, 10.573132007634964f), to_float3(0.000000000000000f, -3.434367603334157f, -9.216617325755173f), to_float3(39.438244799684632f, 0.000000000000000f, -274.009089525723140f));
  sparsespfilta[7] = mat3(to_float3(3.824490469437192f, 0.000000000000000f, 0.000000000000000f), to_float3(0.000000000000000f, -1.540065958710146f, 35.179624268750139f), to_float3(0.000000000000000f, 0.000000000000000f, -239.475015979167920f));
  sparsespfilta[8] = mat3(to_float3(2.977660826364815f, 0.000000000000000f, 0.000000000000000f), to_float3(0.000000000000000f, -1.042036915995045f, 0.000000000000000f), to_float3(0.000000000000000f, 0.000000000000000f, -2.472524681362817f));
  sparsespfilta[9] = mat3(to_float3(2.307327051977537f, 0.000000000000000f, 0.000000000000000f), to_float3(0.000000000000000f, -0.875061637866728f, 0.000000000000000f), to_float3(0.000000000000000f, 0.000000000000000f, -1.409849313639845f));
  sparsespfilta[10] = mat3(to_float3(1.823790655724537f, 0.000000000000000f, 0.000000000000000f), to_float3(0.000000000000000f, -0.781918646414733f, 0.000000000000000f), to_float3(0.000000000000000f, 0.000000000000000f, -1.048825978147449f));
  sparsespfilta[11] = mat3(to_float3(0.000000000000000f, 0.000000000000000f, 0.000000000000000f), to_float3(0.000000000000000f, 0.000000000000000f, 0.000000000000000f), to_float3(0.000000000000000f, 0.000000000000000f, -0.868933490490107f));
  sparsespfilta[12] = mat3(to_float3(0.000000000000000f, 0.000000000000000f, 0.000000000000000f), to_float3(0.000000000000000f, 0.000000000000000f, 0.000000000000000f), to_float3(0.000000000000000f, 0.000000000000000f, -0.766926116519291f));
  sparsespfiltb[0] = mat3(to_float3(36.508697968439087f, 0.000000000000000f, 0.000000000000000f), to_float3(0.000000000000000f, 0.000000000000000f, 0.000000000000000f), to_float3(0.000000000000000f, 0.000000000000000f, 0.000000000000000f));
  sparsespfiltb[1] = mat3(to_float3(57.242341893668829f, 0.000000000000000f, 0.000000000000000f), to_float3(0.000000000000000f, 38.326477066948989f, 0.000000000000000f), to_float3(0.000000000000000f, 0.000000000000000f, 0.000000000000000f));
  sparsespfiltb[2] = mat3(to_float3(112.305664332688050f, 0.000000000000000f, 0.000000000000000f), to_float3(0.000000000000000f, 59.761768151790150f, 0.000000000000000f), to_float3(0.000000000000000f, 0.000000000000000f, 0.000000000000000f));
  sparsespfiltb[3] = mat3(to_float3(295.791838308625070f, 58.489998502973329f, 0.000000000000000f), to_float3(70.091833386311293f, 120.512061156381040f, 0.000000000000000f), to_float3(17.204619265336060f, 0.000000000000000f, 37.784871450121273f));
  sparsespfiltb[4] = mat3(to_float3(-253.802681237032970f, -160.471170139118780f, 0.000000000000000f), to_float3(-194.893137314865900f, 220.339388056683760f, 0.000000000000000f), to_float3(0.000000000000000f, -22.651202495658183f, 57.335351084503102f));
  sparsespfiltb[5] = mat3(to_float3(-114.597984116320400f, 38.688618505605739f, 0.000000000000000f), to_float3(30.320616033665370f, -278.354607015268130f, 9.944900164751438f), to_float3(-30.962164636838232f, 37.612068254920686f, 113.260728861048410f));
  sparsespfiltb[6] = mat3(to_float3(-78.527368894236332f, 0.000000000000000f, 30.382451414099631f), to_float3(0.000000000000000f, -116.269817575252430f, -55.801473552703627f), to_float3(0.353768568406928f, 0.000000000000000f, 243.785483416097240f));
  sparsespfiltb[7] = mat3(to_float3(-53.536668214025610f, 0.000000000000000f, 0.000000000000000f), to_float3(0.000000000000000f, -68.933243211639621f, 17.821880498324404f), to_float3(0.000000000000000f, 0.000000000000000f, -278.470203722289060f));
  sparsespfiltb[8] = mat3(to_float3(-42.646930307293360f, 0.000000000000000f, 0.000000000000000f), to_float3(0.000000000000000f, -51.026918452773138f, 0.000000000000000f), to_float3(0.000000000000000f, 0.000000000000000f, -113.420624636770270f));
  sparsespfiltb[9] = mat3(to_float3(-35.705990828985080f, 0.000000000000000f, 0.000000000000000f), to_float3(0.000000000000000f, -40.934269625438475f, 0.000000000000000f), to_float3(0.000000000000000f, 0.000000000000000f, -67.307342271105213f));
  sparsespfiltb[10] = mat3(to_float3(-30.901151041566411f, 0.000000000000000f, 0.000000000000000f), to_float3(0.000000000000000f, -34.440424768095276f, 0.000000000000000f), to_float3(0.000000000000000f, 0.000000000000000f, -49.156471643386766f));
  sparsespfiltb[11] = mat3(to_float3(0.000000000000000f, 0.000000000000000f, 0.000000000000000f), to_float3(0.000000000000000f, 0.000000000000000f, 0.000000000000000f), to_float3(0.000000000000000f, 0.000000000000000f, -39.178407337105710f));
  sparsespfiltb[12] = mat3(to_float3(0.000000000000000f, 0.000000000000000f, 0.000000000000000f), to_float3(0.000000000000000f, 0.000000000000000f, 0.000000000000000f), to_float3(0.000000000000000f, 0.000000000000000f, -32.812895526130347f));
}

// Essentially the BRDF
__DEVICE__ float4 sp_spectral_filter(float4 col, float filmwidth, float cosi)
{
    float4 retcol = to_float4(0.0f, 0.0f, 0.0f, 1.0f);
    const float NN = 2001.0f;
    float a = 1.0f/(nu*nu);
    float cost = _sqrtf(a*cosi*cosi + (1.0f-a));
    float n = 2.0f*PI*filmwidth*cost/NN;
    float kn = 0.0f;
    mat3 filt = sparsespfiltconst;
    
    for(int i = 0; i < 13; i++)
    {
        kn = (float(i)+6.0f)*n;
        filt += sparsespfilta[i]*_cosf(kn) + sparsespfiltb[i]*_sinf(kn);
    }
    
    swi3(retcol,x,y,z) = 4.0f*(filt*swi3(col,x,y,z))/NN;
    return retcol;
}


// Ray-sphere intersection. Returns both front and backside hit
__DEVICE__ float2 sphere(float3 raydir, float3 offset, float4 sparams)
{
    float3 tcenter = swi3(sparams,x,y,z) - offset;
    float c = dot(tcenter,tcenter)-sparams.w*sparams.w;
    float b = 2.0f*dot(-tcenter, raydir);
    //float a = 1.0f;//dot(raydir, raydir);
    float det = b*b-4.0f*c;
    float2 hits = to_float2(-1.0f,-1.0f);
    if(det > 0.0f) {
      float t1 = 0.5f*(-b+_sqrtf(det));
      float t2 = 0.5f*(-b-_sqrtf(det));
        if(t1 < t2) { hits = to_float2(t1,t2); }
        else { hits = to_float2(t2,t1); }
    }
    return hits;
}

__DEVICE__ float3 reflected(float3 raydir, float3 normal)
{
    return raydir - 2.0f*dot(raydir, normal)*normal;
}

__DEVICE__ float fresnel_schlick(float3 raydir, float3 normal)
{
    float a = 1.0f + dot(raydir, normal);
  return _mix(R_0, 1.0f, a*a*a*a*a);//R_0 + (1.0f-R_0)*a*a*a*a*a;
}

__DEVICE__ float4 background(float3 raydir, __TEXTURE2D__ iChannel0)
{
    return decube_f3(iChannel0,raydir);
}

__DEVICE__ float4 fakehdr(float4 col)
{
    float4 hdrcol;
    swi3S(hdrcol,x,y,z, pow_f3(swi3(col,x,y,z), to_float3_s(2.2f))); // gamma correct
    float lum = dot(swi3(hdrcol,x,y,z), to_float3(0.2126f, 0.7152f, 0.0722f));
    swi3S(hdrcol,x,y,z, hdrfunc(lum)*swi3(hdrcol,x,y,z)); // smooth transition, 0.5f-1.0f -> 0.5f-100.0
    return hdrcol;
}

__DEVICE__ float4 invfakehdr(float4 hdrcol)
{
    float4 ihdrcol;
    float lum = dot(swi3(hdrcol,x,y,z), to_float3(0.2126f, 0.7152f, 0.0722f));
    float tonescale = ((lum/(whitesatval*whitesatval))+1.0f)*lum/(lum+1.0f);
    //swi3(ihdrcol,x,y,z) = hdrcol.xyz/(to_float3_s(1.0f) + swi3(hdrcol,x,y,z));//hdrfunc(1.0f);
    swi3S(ihdrcol,x,y,z, _powf((tonescale/lum)*swi3(hdrcol,x,y,z),to_float3(1.0f/2.2f)));
    ihdrcol.w = 1.0f;
    return ihdrcol;
}


// Intersects all spheres and interpolates all points close to the hits
// The frontside and the backside hits are handled separately.
// The normals for the backside hits are inverted (pointing inside the sphere) since that's the visible side.
__DEVICE__ mat4 scene(float3 raydir, float3 offset, float time)
{
    const int NUMSPHERES = 6; // Cannot be greater than 6, sphere[] below hardcodes 6 indices 
    const float rate = 0.1f;
    float4 spheres[NUMSPHERES];
    float4 fronthits[NUMSPHERES];
    float4 backhits[NUMSPHERES];
    float2 hitdp[NUMSPHERES];
    
    spheres[0] = to_float4_aw( VX*_sinf(1.0f*rate*time)+DX, sVX*_sinf(0.1f*rate*time)+sDX, 0.0f, RADIUS);
    spheres[1] = to_float4_aw(-VX*_sinf(1.2f*rate*time)-DX, sVX*_sinf(0.12f*rate*time)+sDX, sVX*_sinf(0.11f*rate*time)+sDX, RADIUS);
    spheres[2] = to_float4_aw(-sVX*_sinf(0.1f*rate*time)+sDX, VX*_sinf(1.1f*rate*time)+DX,  0.0f, RADIUS);
    spheres[3] = to_float4_aw(sVX*_sinf(0.11f*rate*time)+sDX, -VX*_sinf(1.5f*rate*time)-DX, 0.0f, RADIUS);
    spheres[4] = to_float4(sVX*_sinf(0.09f*rate*time)+sDX, 0.0f, VX*_sinf(1.3f*rate*time)+DX, RADIUS);
    spheres[5] = to_float4(sVX*_sinf(0.1f*rate*time)+sDX, 0.0f, -VX*_sinf(0.8f*rate*time)-DX, RADIUS);
    float4 minfronthit = to_float4(0.0f, 0.0f, 0.0f, MAX_DEPTH);
    float4 avgfronthit = to_float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 minbackhit = to_float4(0.0f, 0.0f, 0.0f, -MAX_DEPTH);
    float4 avgbackhit = to_float4(0.0f, 0.0f, 0.0f, 0.0f);
    float count = 0.0f;
    float backcount = 0.0f;
    for(int i = 0; i < NUMSPHERES; i++) {
      hitdp[i] = sphere(raydir, offset, spheres[i]);
        
      float3 frontpos = hitdp[i].x*raydir + offset;
      float3 backpos = hitdp[i].y*raydir + offset;
      fronthits[i] = to_float4_aw(normalize(frontpos - swi3(spheres[i],x,y,z)), hitdp[i].x);
      backhits[i] = to_float4_aw(normalize(swi3(spheres[i],x,y,z) - backpos), hitdp[i].y);
        
        if(fronthits[i].w > 0.0f) {    
            if(count < 1.0f) {
                avgfronthit = fronthits[i];
                count = 1.0f;
            }
            else {
                if(_fabs(fronthits[i].w - avgfronthit.w) < THRESH_DEPTH) {
                  count += 1.0f;
                avgfronthit += fronthits[i];
                }
                else if(fronthits[i].w < minfronthit.w) {
                    count = 1.0f;
                    avgfronthit = fronthits[i];
                }
            }
            
            if(fronthits[i].w < minfronthit.w) {
              minfronthit = fronthits[i];
            }
        }
        
        if(backhits[i].w > 0.0f) {
            if(backcount < 1.0f) {
                avgbackhit = backhits[i];
                backcount = 1.0f;
            }
            else {
                if(_fabs(backhits[i].w - avgbackhit.w) < THRESH_DEPTH) {
                  backcount += 1.0f;
                avgbackhit += backhits[i];
                }
                else if(backhits[i].w > minbackhit.w) {
                    backcount = 1.0f;
                    avgbackhit = backhits[i];
                }
            }
            
            if(backhits[i].w > minbackhit.w) {
              minbackhit = backhits[i];
            }
        }
    }
    
    mat4 rval = mat4(to_float4(0.0f, 0.0f, 0.0f, -1.0f),
                     to_float4(0.0f, 0.0f, 0.0f, -1.0f),
                     to_float4(0.0f, 0.0f, 0.0f, -1.0f),
                     to_float4(0.0f, 0.0f, 0.0f, -1.0f));
    if(count > 0.01f ) {
        if(count < 1.1f) {
            rval.r0 = to_float4(normalize(swi3(minfronthit,x,y,z)),minfronthit.w);
        }
        else {
            // smooth the transition between spheres
          swi3(avgfronthit,x,y,z) = normalize(swi3(avgfronthit,x,y,z));
          avgfronthit.w = avgfronthit.w/count;
            float tt = _fminf(1.0f, (avgfronthit.w - minfronthit.w)/(0.4f*THRESH_DEPTH));
            float4 rfronthit = tt*minfronthit + (1.0f-tt)*avgfronthit;
            rval.r0 = to_float4_aw(normalize(swi3(rfronthit,x,y,z)),rfronthit.w);
        }
    }
    
    if(backcount > 0.01f ) {
        if(backcount < 1.1f) {
            rval.r1] = to_float4_aw(normalize(swi3(minbackhit,x,y,z)),minbackhit.w);
        }
        else {
            // smooth the transition between spheres
          swi3(avgbackhit,x,y,z) = normalize(swi3(avgbackhit,x,y,z));
          avgbackhit.w = avgbackhit.w/backcount;
            float tt = _fminf(1.0f, (minbackhit.w - avgbackhit.w)/(0.4f*THRESH_DEPTH));
            float4 rbackhit = tt*minbackhit + (1.0f-tt)*avgbackhit;
            rval.r2 = to_float4_aw(normalize(swi3(rbackhit,x,y,z)),rbackhit.w);
        }
    }
    
    return rval;
}



__KERNEL__ void PhysicallyJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

    initialize_sparse_spectral_transforms();
    
    float2 center = iResolution / 2.0f;
    float2 offset = (fragCoord - center)/center.y;
    float focallength = 1.0f;
    
    float ang = 0.04f*iTime;
    mat3 rotatez = mat3( to_float3(_cosf(2.0f*PI*ang), 0.0f, -_sinf(2.0f*PI*ang)), 
                        to_float3(0.0f, 1.0f, 0.0f), to_float3(_sinf(2.0f*PI*ang), 0.0f, _cosf(2.0f*PI*ang)));
    //mat3 rotatex = mat3(to_float3(1.0f, 0.0f, 0.0f), to_float3_aw(0.0f, _cosf(2.0f*PI*ang), -_sinf(2.0f*PI*ang)), to_float3_aw(0.0f, _sinf(2.0f*PI*ang), _cosf(2.0f*PI*ang)));
    
    mat3 rotatex = mat3(to_float3(1.0f, 0.0f, 0.0f), to_float3(0.0f, 0.8090f, 0.5878f), to_float3(0.0f, -0.5878f, 0.8090f));
    mat3 rotate = rotatez;//*rotatex;
    float3 raydir = mul_mat3_f3(rotate , normalize(to_float3_aw(offset/focallength, 1.0f))); // pinhole
    float3 rayorig = -1.5f*rotate.r2;

    
    mat4 scenenorms = scene(raydir, rayorig, 30.0f*iTime);
    float4 col = fakehdr(background(raydir, iChannel0));
    
    if(scenenorms.r0.w > 0.0f)
    {
        for(int i = 0; i < 2; ++i) 
        {
          float3 rvec = swi3(reflected(raydir, scenenorms[i],x,y,z));
          float R = swi3(fresnel_schlick(raydir, scenenorms[i],x,y,z));
          float bubbleheight = 0.5f + (i == 0 ? 1.0f : -1.0f)*0.5f*scenenorms[i].y;
          float filmwidth = varfilmwidth*warpnoise3(rayorig + scenenorms[i].w*raydir) + minfilmwidth + (1.0f-bubbleheight)*(maxfilmwidth-minfilmwidth);

          col = R*sp_spectral_filter(fakehdr(background(rvec)), filmwidth, dot(scenenorms[i].xyz, raydir)) + (1.0f-R)*col;
      
            // DEBUG
            //col = sp_spectral_filter(1.0f*to_float4(1.0f,1.0f,1.0f,1.0f), filmwidth, dot(scenenorms[1].xyz, raydir));
          //col = to_float4(0.8f*_fmaxf(0.0f,dot(-raydir,scenenorms[0].xyz)));   
        }
    }
    col.w = 1.0f;
    fragColor = invfakehdr(col);


  SetFragmentShaderComputedColor(fragColor);
}