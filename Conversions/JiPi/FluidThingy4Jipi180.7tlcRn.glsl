

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
/*
** License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
** Created by bal-khan
**
** This is rendering code
**
** BufferA contain Fluid Code with a define to switch on/off
**
*/

void mainImage( out vec4 o, in vec2 f )
{
    vec4 ret = texture(iChannel0, f/R);
	o = vec4(ret.zzz*1e2, 1.0); // mult value by bignum 
    o = clamp(o, .0, 1.); // clamp bigvalue to be in [0-1] range
    o -= .25; // offset value
    o = sin(B(f/R)*-2.+length(o.xyz)*3.14+1.57*1.0+.250*vec4(.0,1.04,2.08,.0) ); // sinus coloring
}

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
/*
** Based on this tutorial : http://wyattflanders.com/MeAndMyNeighborhood.pdf
*/

//
#define ORDER 

const vec2 n = vec2(+.0, +1.);
const vec2 s = -n;
const vec2 w = vec2(+1., +0.);
const vec2 e = -w;

vec4 F(vec2 p)
{
	vec2 r = A(p/R).xy;
    r = p - r;
    return A(r/R);
}

void mainImage( out vec4 o, in vec2 f )
{
    float kb = C(vec2(32.5/256., 0.25)).x;
    if (kb > .5 || iFrame < 1) {o = vec4(0.,0.,0., .0 ); return;}

    o = F(f);
    vec4 En = F(f+n);
    vec4 Es = F(f+s);
    vec4 Ew = F(f+w);
    vec4 Ee = F(f+e);

    o.z = (En + Es + Ew + Ee).z * .12405;// *exp(1.-4.*o.z);

    // grayscale of camera texture
    float vgb = g( B(f/R).xyz );
    // Adding waves of different magnitudes
    o.xy += 15. * iTimeDelta*cs(50.*vgb + 1.*(length(o.xy) ) ) ;
    o.xy += -30.* iTimeDelta*cs(20.*vgb + 7.*(length(o.xy) )  - (length(vec2(Ee.z - Ew.z, Es.z - En.z))));
    o.xy += 20. * iTimeDelta*cs(70.*vgb + 15.*(length(o.xy) ) + 5.*(length(vec2(Ee.z - Ew.z, Es.z - En.z))));
    
    // experimental other sets of values, these replace the last wave
    /*
    o.xy += 150.*iTimeDelta*cs(1.*g( B(f/R).xyz )+1.0*1.*(length(o.xy) ) ) ;
    o.xy += 150.*iTimeDelta*cs(10.*g( B(f/R).xyz )+3.0*1.*(length(o.xy) ) ) ;
	o.xy += 20.*iTimeDelta*cs(61.*g( B(f/R).xyz )+1.0*10.*(length(o.xy) )+1.0*6.*(length(vec2(Ee.z - Ew.z, Es.z - En.z)))) ;
    */
    /*
    o.xy *= 1.709333; // This artifact is too cool to be killed just like that, I want to make something out of it
	*/
    #ifdef ORDER
    o.xy += vec2(Ee.z - Ew.z, Es.z - En.z) *.25; // ORDER
	#else
    o.xy += vec2(Ee.z - Ew.z, Es.z - En.z) *.0125; // 50% more CHAOS
    #endif

    o.z += (Es.y - En.y + Ee.x - Ew.x) *-.99;

    o = clamp(o, -1.0, 1.); // This should not be needed but idk for sure that it's not needed
    // I should probably also clamp from -PI to PI but I like it as is
}


// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<

/* Linear Filtering Shim by Theron Tarigo, 2019.
   https://www.shadertoy.com/view/tssXWf

   The following codes may be used and copied freely,
   with or without attribution.  However, please do not
   remove the URL, so that others may find the explanation
   provided here, which may be expanded in future.
   This is not a legal requirement.
*/

#define LINEAR_FILTER_CONFIG  true,true,true,true

const struct LINEARFILTER_T {
  bool iChannel0, iChannel1, iChannel2, iChannel3;
} LINEARFILTER = LINEARFILTER_T(LINEAR_FILTER_CONFIG);

const struct SAMPLERINDEX_T {
  int iChannel0, iChannel1, iChannel2, iChannel3;
} SAMPLERINDEX = SAMPLERINDEX_T(0,1,2,3);

vec4 textureLinearPix (sampler2D sampler, vec2 U) {
  return mix(mix(
      texelFetch(sampler,ivec2(U)+ivec2(0,0),0),
      texelFetch(sampler,ivec2(U)+ivec2(1,0),0),
    fract((U).x)),mix(
      texelFetch(sampler,ivec2(U)+ivec2(0,1),0),
      texelFetch(sampler,ivec2(U)+ivec2(1,1),0),
    fract((U).x)),fract((U).y));
}

#define textureLinear(sampler,P) textureLinearPix(sampler, \
    (P)*iChannelResolution[SAMPLERINDEX.sampler].xy-vec2(.5))

#define texture(sampler,P) ( LINEARFILTER.iChannel0 ? \
    textureLinear(sampler,P) \
  : texture(sampler,P) )
// End of Linear Filtering Shim

#define R iResolution.xy

// grayscale
#define g(c) (.3*c.x + .59*c.y + .11*c.z) 

// trigonometrics
vec2 cs(float a) { return vec2(cos(a), sin(a)); }

//code shortcuts
#define A(u) texture(iChannel0, u)
#define B(u) texture(iChannel1, u)
#define C(u) texture(iChannel2, u)
#define D(u) texture(iChannel3, u)
