

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<

// "SoundSpector"
// by Julien Vergnaud @duvengar-2018
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
//////////////////////////////////////////////////////////////////////////////////////

#define CEL 64.                  // How many buckets to divide spectrum into

// the original accurate spectrum was taken from 
// https://www.shadertoy.com/view/lt2fRz by 834144373

#define barSize 1.0 / CEL        // Constant to avoid division in main loop
#define sampleSize 0.2           // How accurately to sample spectrum

// iq's cosine palette function
vec3 palette( in float t, in vec3 a, in vec3 b, in vec3 c, in vec3 d )
{
    return a + b*cos( 6.28318*(c*t+d) );
}



// logo distance function written specially for the occasion :)
float rect(vec2 uv,vec2 p, float w, float h, float blur){
    
    uv += p;
    float rv = S(w, w + blur, length(uv.x));
    float rh = S(h, h + blur, length(uv.y));
    return SAT( rv + rh);
}

float logo(vec2 uv, float blur){    
    uv += vec2(.001,.008);
    uv.y *= 1.15;

    float as = blur;
    // first make 4 circles
    float s1 = S(.011 - as, .011, length(uv - vec2(-.035,.0)));
    float s2 = S(.019 - as, .019, length(uv - vec2(-.015,.01)));
    float s3 = S(.023 - as, .023, length(uv - vec2(.01,.02)));
    float s4 = S(.0142 - as, .0142, length(uv - vec2(.035,.0022)));
    // then a rectangle
    float r1 = rect(uv, vec2(.0,-.0012), .032, .0101, as);
    // make vertical lines stripes for part off the screen
          uv = uv.x > -.0072 ? uv : fract(uv * 220.);
    float lines = S(.7, .7 + as,length(uv.x + .25));
    // merge circles together
    float shape =  SAT(s1 * s2 * s3 * s4 * r1); 
    // and finally substract the line from the shape
    shape = 1. - max(shape, lines);
    
    return SAT(shape);
}

void mainImage( out vec4 fragColor, in vec2 U ) {
  	
 
   	// DOMAIN
    vec2 uv = (U -.5 * R)/R.x;
    vec2 uv2  = 1. - 2. * U / R;                       // additionnal domain for post processing effect       
    vec2 st = uv;   					               // store original domain before distortion
    uv *= M(TWO_PI / 8. * cos(length(st)) * 20.);      // rotate and twist domain 
    vec2 pol = vec2(atan(uv.x, uv.y), length(uv));     // polar coordinates
    float alfa = pol.x / TWO_PI + .5;                  // full polar domain from -PI to PI
    
    uv = vec2(alfa , pol.y);                           // convert domain to polar coordinates
   	
    
    // SPECTRUM
    float barStart = floor(uv.x * CEL) / CEL;          // spectrum buckets id's
    float amp = .0;  
    amp -= texture(iChannel0,vec2(400.,0.),0.).x;      // store global sound amplitude
    
    float intensity = .0;                              // loop get all buckets intensity
    
    for(float i = 0.0; i < barSize; i += barSize * sampleSize)
    {      
        intensity += .9 * texture(iChannel0, vec2(barStart + i, 0.)).r;  
    }
    
	intensity *= sampleSize;                           // accurate intensity
	intensity  = clamp(intensity + .1, 0.29, 1.);       // apply floor on lower intensities   
    float height  = S(.0,.005, intensity - uv.y * 3.);   // height of the bucket
  	
     
    // SHADING                                         // I'm using iq's cosine color palette for coloring
    vec3 pal = palette(intensity -.2,
                            vec3(.5, .5, .0),
                            vec3(.5,.5,.35),
                            vec3(.9,.5,.3),
                            vec3(vec2(intensity * length(uv) + uv.y), 1.)); 		
    
    vec4 spectrum =  vec4(SAT(pal) * height, 1.);               // final colored spectrum
    float lines  = S(.2, .4, .85*length(fract(uv.x * CEL)-.5)); // lines gap to be substrated from the spectrum
    float center = 1.- S(.09, .091, length(st));	            // circle to be substrated from the spectrum
    float c0     = 1.- S(.082, .083, length(st));               // highlight circle of the logo bakground
    vec4 bg = vec4(.1,.15,.3, 1.);					            // base color bakground
    vec3 pat  = texture(iChannel1, U / R).xyz;                  // get image texture from buffer A
    
    bg      += -amp/3.+.1 * vec4(max(pat,center), 1.);                 // connect bg luminosity to global amplitude
    bg      += .23 * S(.03,.0,length(st));                             // hightlight middle of screen
    float lns = S(.5,.51,length(fract(pol.y*250.)));                   // radial lines pattern applied on bg
	bg -= length(pol.y*.1) * lns;                       
    bg += .05 * length( (st.y + .08)* 30.) * c0;                       // apply highlight on logo circle bg
    vec4 col = min(vec4(spectrum),vec4(spectrum) - (lines + center)) ; //substract middle of the spectrum
    col = mix( SAT(bg), SAT(3. * col), .5);
    
    
    // POSTPROD
    col += .1 * hash2(T + st * vec2(1462.439, 297.185));               // animated grain (hash2 function in common tab)
    col *= 1.1 * vec4(.9 - S(.0, 2.2, length(uv2*uv2)));               // vigneting
    col = pow(col, vec4(1.3));                                         // gamma
    
    
    // LOGO
    col = 1.1 * col + logo(st * (.9+amp*.4), .003 ) * 4. ;	                // add soundcloud logo
 	col += (.075*amp)* logo((st- vec2(.0,-.009)) * (.75 + amp *.1), .004) * 2. ;   // add soundcloud logo shadows
    
     
    fragColor = col ;
    
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define T iTime
#define R iResolution.xy
#define LOWRES 320.
#define SAT(a) clamp(a, .0,1.)
#define S(a, b, c) smoothstep(a, b, c)
#define M(a) mat2(cos(a), -sin(a), sin(a), cos(a))
#define PI acos(-1.)
#define TWO_PI (PI * 2.)


/////////////////////////////////////////////
// hash2 taken from Dave Hoskins 
// https://www.shadertoy.com/view/4djSRW
/////////////////////////////////////////////

float hash2(vec2 p)
{  
	vec3 p3  = fract(vec3(p.xyx) * .2831);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}


/////////////////////////////////////////////
// hash and noise function by Inigo Quilez
/////////////////////////////////////////////

float hash( float n ) { 
	return fract(sin(n)*75728.5453123); 
}


float noise( in vec2 x ) {
    vec2 p = floor(x);
    vec2 f = fract(x);
    f = f*f*(3.0-2.0*f);
    float n = p.x + p.y*57.0;
    return mix(mix( hash(n + 0.0), hash(n + 1.0), f.x), mix(hash(n + 57.0), hash(n + 58.0), f.x), f.y);
}



// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// copacabana pattern + noise distortion + rot
// https://www.shadertoy.com/view/MsXBzB

void mainImage( out vec4 C, vec2 U )
{  
    U = (U -.5 * R)/R.x;
    U *= M(T * .1);    
    U += .4 * length(U-.5) * noise(U * 4.);
    U *= 70.;   
    U.y -= cos(U.x) + iDate.w ; 
    C = vec4(smoothstep(0.5,
                        1.,
                        mix(cos(U.x) * sin(U.y),
                        0.3,
                        cos(U.y) * .5) * 8.));
        
}