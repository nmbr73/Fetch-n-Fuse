

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// PHYSICALLY-BASED REAL-TIME SOAP BUBBLE
// @author: Matteo Mannino
// @about:
// This simulates the interference for 81 wavelengths of light on a soap bubble film.
// The idea was to use an RGB-to-Spectrum->FILTER->Spectrum-to-RGB process:
// 1. The RGB-to-Spectrum filter comes from an assumed camera model and pseudo-inverse conversion approach
// 2. The filter is the from Andrew Glassner's notebook. It gives the approximation of the power attenuation-per-wavelength
//	  that the interference creates given the film width and incident angle. 81 wavelengths are used. 
//    Andrew Glassner's notebook on Soap Bubbles (part 2): https://www.glassner.com/wp-content/uploads/2014/04/CG-CGA-PDF-00-11-Soap-Bubbles-2-Nov00.pdf
//    (you can compare figure 17-18 to this simulation)
// 3. For the final color, transform the Spectrum back to RGB
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
// License: Creative Commons Attribution-NonCommercial 4.0 International

// HDR VARS (need to fake an HDR envmap for the image-based reflections)
#define hdrfunc(x) (exp(1.2*(x))-1.0)
#define whitesatval hdrfunc(1.0)

// SOAP REFRACTION VARS
const float R_0 = 0.0278;
const float nu = 1.4;
const float minfilmwidth = 150.0;
const float maxfilmwidth = 700.0;
const float varfilmwidth = 20.0;

// RAY-TRACE EPSILON
const float eps = 0.001;

//SOAP BUBBLE GEOMETRY VARS
const float PI = 3.141592653589793;
const float RADIUS = 0.8;
const float MAX_DEPTH = 9999999.0;
const float THRESH_DEPTH = 0.05;
const float DX = 0.10;
const float VX = 0.04;
const float sDX = 0.01;
const float sVX = 0.04;

// TRANSFORMS FOR RGB-to-SPECTRUM-FILTER-SPECTRUM-to-RGB
mat3 sparsespfiltconst;
mat3 sparsespfilta[13];
mat3 sparsespfiltb[13];

// Fractal Brownian Motion
float fBm(vec3 p) {
	float v = 0.0;
    float amplitude = 4.0;
    float scale = 1.0;
    int octaves = 2;
    for(int i = 0; i < octaves; ++i) {
    	//v += amplitude*texture(iChannel1, scale*p.xyz).r;
        v += amplitude*texture(iChannel1, scale*p.xy).r;
        amplitude *= 0.5;
        scale *= 2.0;
    }
    return v;
}

// 1 level of warp noise for micro waves on bubble surface
float warpnoise3(vec3 p) {
    float f = 0.0;
    const float c1 = 0.06;
    const float tc = 0.05;
    vec3 q = vec3(fBm(p + tc*iTime), 
                  fBm(p + vec3(5.1, 1.3, 2.2) + tc*iTime), 
                  fBm(p + vec3(3.4, 4.8, 5.5) + tc*iTime));
    
    return 1.2*fBm(p + c1*q);
}

// Pre-computed coefficients for spectral response
void initialize_sparse_spectral_transforms()
{
    sparsespfiltconst = mat3(vec3(997.744490776777870, 0.000000000000000, 0.000000000000000), vec3(0.000000000000000, 1000.429230968840700, 0.000000000000000), vec3(0.000000000000000, 0.000000000000000, 1000.314923254210300));
	sparsespfilta[0] = mat3(vec3(-9.173541963568921, 0.000000000000000, 0.000000000000000), vec3(0.000000000000000, 0.000000000000000, 0.000000000000000), vec3(0.000000000000000, 0.000000000000000, 0.000000000000000));
	sparsespfilta[1] = mat3(vec3(-12.118820092848431, 0.000000000000000, 0.000000000000000), vec3(0.000000000000000, 0.362717643641774, 0.000000000000000), vec3(0.000000000000000, 0.000000000000000, 0.000000000000000));
	sparsespfilta[2] = mat3(vec3(-18.453733912103289, 0.000000000000000, 0.000000000000000), vec3(0.000000000000000, 1.063838675818334, 0.000000000000000), vec3(0.000000000000000, 0.000000000000000, 0.000000000000000));
	sparsespfilta[3] = mat3(vec3(-448.414255038845680, -26.846846493079958, 0.000000000000000), vec3(94.833575999184120, 9.525075729872752, 0.000000000000000), vec3(-48.773853498042200, 0.000000000000000, -0.416692876008104));
	sparsespfilta[4] = mat3(vec3(6.312176276235818, -29.044711065580177, 0.000000000000000), vec3(-187.629408328884550, -359.908263134928520, 0.000000000000000), vec3(0.000000000000000, 25.579031651446712, -0.722360089703890));
	sparsespfilta[5] = mat3(vec3(-33.547962219868452, 61.587972582979901, 0.000000000000000), vec3(97.565538879460178, -150.665614921761320, -30.220477643983013), vec3(1.552347379820659, -0.319166631512109, -0.935186347338915));
	sparsespfilta[6] = mat3(vec3(3.894757056395064, 0.000000000000000, 10.573132007634964), vec3(0.000000000000000, -3.434367603334157, -9.216617325755173), vec3(39.438244799684632, 0.000000000000000, -274.009089525723140));
	sparsespfilta[7] = mat3(vec3(3.824490469437192, 0.000000000000000, 0.000000000000000), vec3(0.000000000000000, -1.540065958710146, 35.179624268750139), vec3(0.000000000000000, 0.000000000000000, -239.475015979167920));
	sparsespfilta[8] = mat3(vec3(2.977660826364815, 0.000000000000000, 0.000000000000000), vec3(0.000000000000000, -1.042036915995045, 0.000000000000000), vec3(0.000000000000000, 0.000000000000000, -2.472524681362817));
	sparsespfilta[9] = mat3(vec3(2.307327051977537, 0.000000000000000, 0.000000000000000), vec3(0.000000000000000, -0.875061637866728, 0.000000000000000), vec3(0.000000000000000, 0.000000000000000, -1.409849313639845));
	sparsespfilta[10] = mat3(vec3(1.823790655724537, 0.000000000000000, 0.000000000000000), vec3(0.000000000000000, -0.781918646414733, 0.000000000000000), vec3(0.000000000000000, 0.000000000000000, -1.048825978147449));
	sparsespfilta[11] = mat3(vec3(0.000000000000000, 0.000000000000000, 0.000000000000000), vec3(0.000000000000000, 0.000000000000000, 0.000000000000000), vec3(0.000000000000000, 0.000000000000000, -0.868933490490107));
	sparsespfilta[12] = mat3(vec3(0.000000000000000, 0.000000000000000, 0.000000000000000), vec3(0.000000000000000, 0.000000000000000, 0.000000000000000), vec3(0.000000000000000, 0.000000000000000, -0.766926116519291));
	sparsespfiltb[0] = mat3(vec3(36.508697968439087, 0.000000000000000, 0.000000000000000), vec3(0.000000000000000, 0.000000000000000, 0.000000000000000), vec3(0.000000000000000, 0.000000000000000, 0.000000000000000));
	sparsespfiltb[1] = mat3(vec3(57.242341893668829, 0.000000000000000, 0.000000000000000), vec3(0.000000000000000, 38.326477066948989, 0.000000000000000), vec3(0.000000000000000, 0.000000000000000, 0.000000000000000));
	sparsespfiltb[2] = mat3(vec3(112.305664332688050, 0.000000000000000, 0.000000000000000), vec3(0.000000000000000, 59.761768151790150, 0.000000000000000), vec3(0.000000000000000, 0.000000000000000, 0.000000000000000));
	sparsespfiltb[3] = mat3(vec3(295.791838308625070, 58.489998502973329, 0.000000000000000), vec3(70.091833386311293, 120.512061156381040, 0.000000000000000), vec3(17.204619265336060, 0.000000000000000, 37.784871450121273));
	sparsespfiltb[4] = mat3(vec3(-253.802681237032970, -160.471170139118780, 0.000000000000000), vec3(-194.893137314865900, 220.339388056683760, 0.000000000000000), vec3(0.000000000000000, -22.651202495658183, 57.335351084503102));
	sparsespfiltb[5] = mat3(vec3(-114.597984116320400, 38.688618505605739, 0.000000000000000), vec3(30.320616033665370, -278.354607015268130, 9.944900164751438), vec3(-30.962164636838232, 37.612068254920686, 113.260728861048410));
	sparsespfiltb[6] = mat3(vec3(-78.527368894236332, 0.000000000000000, 30.382451414099631), vec3(0.000000000000000, -116.269817575252430, -55.801473552703627), vec3(0.353768568406928, 0.000000000000000, 243.785483416097240));
	sparsespfiltb[7] = mat3(vec3(-53.536668214025610, 0.000000000000000, 0.000000000000000), vec3(0.000000000000000, -68.933243211639621, 17.821880498324404), vec3(0.000000000000000, 0.000000000000000, -278.470203722289060));
	sparsespfiltb[8] = mat3(vec3(-42.646930307293360, 0.000000000000000, 0.000000000000000), vec3(0.000000000000000, -51.026918452773138, 0.000000000000000), vec3(0.000000000000000, 0.000000000000000, -113.420624636770270));
	sparsespfiltb[9] = mat3(vec3(-35.705990828985080, 0.000000000000000, 0.000000000000000), vec3(0.000000000000000, -40.934269625438475, 0.000000000000000), vec3(0.000000000000000, 0.000000000000000, -67.307342271105213));
	sparsespfiltb[10] = mat3(vec3(-30.901151041566411, 0.000000000000000, 0.000000000000000), vec3(0.000000000000000, -34.440424768095276, 0.000000000000000), vec3(0.000000000000000, 0.000000000000000, -49.156471643386766));
	sparsespfiltb[11] = mat3(vec3(0.000000000000000, 0.000000000000000, 0.000000000000000), vec3(0.000000000000000, 0.000000000000000, 0.000000000000000), vec3(0.000000000000000, 0.000000000000000, -39.178407337105710));
	sparsespfiltb[12] = mat3(vec3(0.000000000000000, 0.000000000000000, 0.000000000000000), vec3(0.000000000000000, 0.000000000000000, 0.000000000000000), vec3(0.000000000000000, 0.000000000000000, -32.812895526130347));
}

// Essentially the BRDF
vec4 sp_spectral_filter(vec4 col, float filmwidth, float cosi)
{
    vec4 retcol = vec4(0.0, 0.0, 0.0, 1.0);
    const float NN = 2001.0;
    float a = 1.0/(nu*nu);
    float cost = sqrt(a*cosi*cosi + (1.0-a));
    float n = 2.0*PI*filmwidth*cost/NN;
    float kn = 0.0;
    mat3 filt = sparsespfiltconst;
    
    for(int i = 0; i < 13; i++)
    {
        kn = (float(i)+6.0f)*n;
        filt += sparsespfilta[i]*cos(kn) + sparsespfiltb[i]*sin(kn);
    }
    
    retcol.xyz = 4.0*(filt*col.xyz)/NN;
    return retcol;
}


// Ray-sphere intersection. Returns both front and backside hit
vec2 sphere(vec3 raydir, vec3 offset, vec4 sparams)
{
    vec3 tcenter = sparams.xyz - offset;
 	float c = dot(tcenter,tcenter)-sparams.w*sparams.w;
    float b = 2.0*dot(-tcenter, raydir);
    //float a = 1.0;//dot(raydir, raydir);
    float det = b*b-4.0*c;
    vec2 hits = vec2(-1.0,-1.0);
    if(det > 0.0) {
    	float t1 = 0.5*(-b+sqrt(det));
    	float t2 = 0.5*(-b-sqrt(det));
        if(t1 < t2) { hits = vec2(t1,t2); }
        else { hits = vec2(t2,t1); }
    }
    return hits;
}

vec3 reflected(vec3 raydir, vec3 normal)
{
    return raydir - 2.0*dot(raydir, normal)*normal;
}

float fresnel_schlick(vec3 raydir, vec3 normal)
{
    float a = 1.0 + dot(raydir, normal);
	return mix(R_0, 1.0, a*a*a*a*a);//R_0 + (1.0-R_0)*a*a*a*a*a;
}

vec4 background(vec3 raydir)
{
    return texture(iChannel0, raydir);
}

vec4 fakehdr(vec4 col)
{
    vec4 hdrcol;
    hdrcol.rgb = pow(col.rgb, vec3(2.2)); // gamma correct
    float lum = dot(hdrcol.rgb, vec3(0.2126, 0.7152, 0.0722));
    hdrcol.rgb = hdrfunc(lum)*hdrcol.rgb; // smooth transition, 0.5-1.0 -> 0.5-100.0
    return hdrcol;
}

vec4 invfakehdr(vec4 hdrcol)
{
    vec4 ihdrcol;
    float lum = dot(hdrcol.rgb, vec3(0.2126, 0.7152, 0.0722));
    float tonescale = ((lum/(whitesatval*whitesatval))+1.0)*lum/(lum+1.0);
    //ihdrcol.rgb = hdrcol.rgb/(vec3(1.0) + hdrcol.rgb);//hdrfunc(1.0);
    ihdrcol.rgb = pow((tonescale/lum)*hdrcol.rgb,vec3(1.0/2.2));
    ihdrcol.w = 1.0;
    return ihdrcol;
}


// Intersects all spheres and interpolates all points close to the hits
// The frontside and the backside hits are handled separately.
// The normals for the backside hits are inverted (pointing inside the sphere) since that's the visible side.
mat4 scene(vec3 raydir, vec3 offset, float time)
{
 	const int NUMSPHERES = 6; // Cannot be greater than 6, sphere[] below hardcodes 6 indices 
    const float rate = 0.1;
	vec4 spheres[NUMSPHERES];
    vec4 fronthits[NUMSPHERES];
    vec4 backhits[NUMSPHERES];
    vec2 hitdp[NUMSPHERES];
    
    spheres[0] = vec4( VX*sin(1.0*rate*time)+DX, sVX*sin(0.1*rate*time)+sDX, 0.0, RADIUS);
    spheres[1] = vec4(-VX*sin(1.2*rate*time)-DX, sVX*sin(0.12*rate*time)+sDX, sVX*sin(0.11*rate*time)+sDX, RADIUS);
    spheres[2] = vec4(-sVX*sin(0.1*rate*time)+sDX, VX*sin(1.1*rate*time)+DX,  0.0, RADIUS);
    spheres[3] = vec4(sVX*sin(0.11*rate*time)+sDX, -VX*sin(1.5*rate*time)-DX, 0.0, RADIUS);
    spheres[4] = vec4(sVX*sin(0.09*rate*time)+sDX, 0.0, VX*sin(1.3*rate*time)+DX, RADIUS);
    spheres[5] = vec4(sVX*sin(0.1*rate*time)+sDX, 0.0, -VX*sin(0.8*rate*time)-DX, RADIUS);
    vec4 minfronthit = vec4(0.0, 0.0, 0.0, MAX_DEPTH);
    vec4 avgfronthit = vec4(0.0, 0.0, 0.0, 0.0);
    vec4 minbackhit = vec4(0.0, 0.0, 0.0, -MAX_DEPTH);
    vec4 avgbackhit = vec4(0.0, 0.0, 0.0, 0.0);
    float count = 0.0;
    float backcount = 0.0;
    for(int i = 0; i < NUMSPHERES; i++) {
  		hitdp[i] = sphere(raydir, offset, spheres[i]);
        
    	vec3 frontpos = hitdp[i].x*raydir + offset;
    	vec3 backpos = hitdp[i].y*raydir + offset;
    	fronthits[i] = vec4(normalize(frontpos - spheres[i].xyz), hitdp[i].x);
    	backhits[i] = vec4(normalize(spheres[i].xyz - backpos), hitdp[i].y);
        
        if(fronthits[i].w > 0.0) {    
            if(count < 1.0) {
                avgfronthit = fronthits[i];
                count = 1.0;
            }
            else {
                if(abs(fronthits[i].w - avgfronthit.w) < THRESH_DEPTH) {
                	count += 1.0;
            		avgfronthit += fronthits[i];
                }
                else if(fronthits[i].w < minfronthit.w) {
                    count = 1.0;
                    avgfronthit = fronthits[i];
                }
            }
            
            if(fronthits[i].w < minfronthit.w) {
            	minfronthit = fronthits[i];
            }
        }
        
        if(backhits[i].w > 0.0) {
            if(backcount < 1.0) {
                avgbackhit = backhits[i];
                backcount = 1.0;
            }
            else {
                if(abs(backhits[i].w - avgbackhit.w) < THRESH_DEPTH) {
                	backcount += 1.0;
            		avgbackhit += backhits[i];
                }
                else if(backhits[i].w > minbackhit.w) {
                    backcount = 1.0;
                    avgbackhit = backhits[i];
                }
            }
            
            if(backhits[i].w > minbackhit.w) {
            	minbackhit = backhits[i];
            }
        }
    }
    
    mat4 rval = mat4(vec4(0.0, 0.0, 0.0, -1.0),
                     vec4(0.0, 0.0, 0.0, -1.0),
                     vec4(0.0, 0.0, 0.0, -1.0),
                     vec4(0.0, 0.0, 0.0, -1.0));
    if(count > 0.01 ) {
        if(count < 1.1) {
            rval[0] = vec4(normalize(minfronthit.xyz),minfronthit.w);
        }
        else {
            // smooth the transition between spheres
        	avgfronthit.xyz = normalize(avgfronthit.xyz);
        	avgfronthit.w = avgfronthit.w/count;
            float tt = min(1.0, (avgfronthit.w - minfronthit.w)/(0.4*THRESH_DEPTH));
            vec4 rfronthit = tt*minfronthit + (1.0-tt)*avgfronthit;
            rval[0] = vec4(normalize(rfronthit.xyz),rfronthit.w);
        }
    }
    
    if(backcount > 0.01 ) {
        if(backcount < 1.1) {
            rval[1] = vec4(normalize(minbackhit.xyz),minbackhit.w);
        }
        else {
            // smooth the transition between spheres
        	avgbackhit.xyz = normalize(avgbackhit.xyz);
        	avgbackhit.w = avgbackhit.w/backcount;
            float tt = min(1.0, (minbackhit.w - avgbackhit.w)/(0.4*THRESH_DEPTH));
            vec4 rbackhit = tt*minbackhit + (1.0-tt)*avgbackhit;
            rval[1] = vec4(normalize(rbackhit.xyz),rbackhit.w);
        }
    }
    
    return rval;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    initialize_sparse_spectral_transforms();
    
    vec2 center = iResolution.xy / 2.0;
    vec2 offset = (fragCoord.xy - center)/center.y;
    float focallength = 1.0;
    
    float ang = 0.04*iTime;
    mat3 rotatez = mat3( vec3(cos(2.0*PI*ang), 0.0, -sin(2.0*PI*ang)), 
                        vec3(0.0, 1.0, 0.0), vec3(sin(2.0*PI*ang), 0.0, cos(2.0*PI*ang)));
    //mat3 rotatex = mat3(vec3(1.0, 0.0, 0.0), vec3(0.0, cos(2.0*PI*ang), -sin(2.0*PI*ang)), vec3(0.0, sin(2.0*PI*ang), cos(2.0*PI*ang)));
    
    mat3 rotatex = mat3(vec3(1.0, 0.0, 0.0), vec3(0.0, 0.8090, 0.5878), vec3(0.0, -0.5878, 0.8090));
    mat3 rotate = rotatez;//*rotatex;
    vec3 raydir = rotate*normalize(vec3(offset/focallength, 1.0)); // pinhole
    vec3 rayorig = -1.5*rotate[2];

    
    mat4 scenenorms = scene(raydir, rayorig, 30.0*iTime);
    vec4 col = fakehdr(background(raydir));
    
    if(scenenorms[0].w > 0.0)
    {
        for(int i = 0; i < 2; ++i) 
        {
	        vec3 rvec = reflected(raydir, scenenorms[i].xyz);
	 	    float R = fresnel_schlick(raydir, scenenorms[i].xyz);
        	float bubbleheight = 0.5 + (i == 0 ? 1.0 : -1.0)*0.5*scenenorms[i].y;
        	float filmwidth = varfilmwidth*warpnoise3(rayorig + scenenorms[i].w*raydir) + minfilmwidth + (1.0-bubbleheight)*(maxfilmwidth-minfilmwidth);

        	col = R*sp_spectral_filter(fakehdr(background(rvec)), filmwidth, dot(scenenorms[i].xyz, raydir)) + (1.0-R)*col;
			
            // DEBUG
            //col = sp_spectral_filter(1.0*vec4(1.0,1.0,1.0,1.0), filmwidth, dot(scenenorms[1].xyz, raydir));
        	//col = vec4(0.8*max(0.0,dot(-raydir,scenenorms[0].xyz)));   
        }
    }
    col.w = 1.0;
    fragColor = invfakehdr(col);
}