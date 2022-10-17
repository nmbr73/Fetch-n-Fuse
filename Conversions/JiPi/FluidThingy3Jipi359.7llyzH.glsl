

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
** This is rendering
** I took rendering from flockaroo at : https://www.shadertoy.com/view/WdVXWy
*/

#define FANCY_REFLECTIVE   // undef to see the reac-diff in 2 colors only, I like it too
#define MOTION_SICKNESS 1. // change to 0 to stop mooving the cubemap

void mainImage( out vec4 o, in vec2 f )
{
    iR = iResolution.xy;

	vec2 uv = f / R.xy;
    vec4 ret = texture(iChannel0, uv);
	#ifdef FANCY_REFLECTIVE
    vec2 d  = vec2(1./R.y, .0);
    vec2 gd = vec2( (A(uv+d.xy)-A(uv-d.xy) ).x , (A(uv+d.yx)-A(uv-d.yx)).x )/R.y;
    vec3 n  = normalize( vec3(gd*500., 1.) );
    vec3 rd = normalize( vec3((f-R*.5-MOTION_SICKNESS*vec2( (sin(iTime*.25)+.5)*30.,.0))/R, -.25) );
    rd = reflect(rd, n);
    vec3 rf = C(rd.xyz).xyz;
    //rf = sin(length(rf)*1.+.0*vec3(.0, 1.04, 2.08));
    ret.xyz = rf*ret.x;
    #endif 
	o = vec4(ret.xyz, 1.0);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<

/* Linear Filtering Shim by Theron Tarigo, 2019.
   https://www.shadertoy.com/view/tssXWf

   The following codes may be used and copied freely,
   with or without attribution.  However, please do not
   remove the URL, so that others may find the explanation
   provided here, which may be expanded in future.
   This is not a legal requirement.
*/

#define LINEAR_FILTER_CONFIG  true,true,true,true
//#define LINEAR_FILTER_CONFIG  false,false,false,false

const struct LINEARFILTER_T {
  bool iChannel0, iChannel1, iChannel2, iChannel3;
} LINEARFILTER = LINEARFILTER_T(LINEAR_FILTER_CONFIG);

const struct SAMPLERINDEX_T {
  int iChannel0, iChannel1, iChannel2, iChannel3;
} SAMPLERINDEX = SAMPLERINDEX_T(0,1,2,3);


#ifdef ORG
vec4 textureLinearPix (sampler2D sampler, vec2 U) {
  return mix(mix(
      texelFetch(sampler,ivec2(U)+ivec2(0,0),0),
      texelFetch(sampler,ivec2(U)+ivec2(1,0),0),
    fract((U).x)),mix(
      texelFetch(sampler,ivec2(U)+ivec2(0,1),0),
      texelFetch(sampler,ivec2(U)+ivec2(1,1),0),
    fract((U).x)),fract((U).y));
}
#endif


vec4 textureLinearPix (sampler2D sampler, vec2 U) {
  return mix(mix(
      texture(sampler,(vec2(ivec2(U)+ivec2(0,0))+0.5)/iR),
      texture(sampler,(vec2(ivec2(U)+ivec2(1,0))+0.5)/iR),
    fract((U).x)),mix(
      texture(sampler,(vec2(ivec2(U)+ivec2(0,1))+0.5)/iR),
      texture(sampler,(vec2(ivec2(U)+ivec2(1,1))+0.5)/iR),
    fract((U).x)),fract((U).y));
}



#define textureLinear(sampler,P) textureLinearPix(sampler, \
    (P)*iChannelResolution[SAMPLERINDEX.sampler].xy-vec2(.5))

#define texture(sampler,P) ( LINEARFILTER.iChannel0 ? \
    textureLinear(sampler,P) \
  : texture(sampler,P) )
// End of Linear Filtering Shim

/*
** This is reaction diffusion
** Taken from me at : https://www.shadertoy.com/view/XlKXDm
*/

#define FEED_DEFAULT (.030550 + .0105*bvar*.0  )
#define KILL_DEFAULT (.0620   + .060205*bvar*.0)

vec2 texsize;
vec2	laplacian_convolution(vec2 uv)
{
	vec2	ret = vec2(0.);
    
    if (uv.x == 0. || uv.y == 0. || uv.x== 1. || uv.y ==1.)
        return (ret);
    ret += texture(iChannel0, vec2(uv.x , uv.y) ).xy * -1.;
    
    ret += texture(iChannel0, vec2(uv.x -texsize.x, uv.y) ).xy * (.2);
    ret += texture(iChannel0, vec2(uv.x +texsize.x, uv.y) ).xy * (.2);
    ret += texture(iChannel0, vec2(uv.x , uv.y -texsize.y) ).xy * (.2);
    ret += texture(iChannel0, vec2(uv.x , uv.y +texsize.y) ).xy * (.2);
    
    ret += texture(iChannel0, vec2(uv.x -texsize.x, uv.y -texsize.y) ).xy * (.05);
    ret += texture(iChannel0, vec2(uv.x +texsize.x, uv.y -texsize.y) ).xy * (.05);
    ret += texture(iChannel0, vec2(uv.x +texsize.x, uv.y +texsize.y) ).xy * (.05);
    ret += texture(iChannel0, vec2(uv.x -texsize.x, uv.y +texsize.y) ).xy * (.05);
    return (ret);
}

void mainImage( out vec4 o, in vec2 f )
{
    iR = iResolution.xy;

    vec2 uv = f / R;
    texsize = 1./R;
    vec4	ret = texture(iChannel0, uv);
    float	bvar = B(uv).w;
    vec2	ab = ret.xy+vec2(.0, 1.)*bvar*iTimeDelta*1.;
    vec2	mouse = iMouse.xy / iResolution.xy;
    float	kb = C(vec2(32.5/256., 0.25)).x;
    if (iMouse.z > .5 && length(f-iMouse.xy) < 50.) 
    {
	    o.y = 1.;
        return;
	}
    if (kb >.5 || abs(iTime) <= .5)
    {
        o.x = 1.;
        o.y = 0.;
        if (sin(uv.x) <0.5 && sin(uv.y-.4) < .5 && sin(uv.x) > .45 && sin(uv.y-.4) > .45)
            o = vec4(1., 1., 0., 1.);
    }
    else
    {
     o.x = clamp(ab.x + (1. * (laplacian_convolution(uv).x) - ab.x * ab.y * ab.y + FEED_DEFAULT * (1. - ab.x) ) ,-1.,1.);
     o.y = clamp(ab.y + (.5 * (laplacian_convolution(uv).y) + ab.x * ab.y * ab.y - (FEED_DEFAULT + KILL_DEFAULT) * ab.y ),-1.,1.);
    }
}

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
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

#ifdef ORG
vec4 textureLinearPix (sampler2D sampler, vec2 U) {
  return mix(mix(
      texelFetch(sampler,ivec2(U)+ivec2(0,0),0),
      texelFetch(sampler,ivec2(U)+ivec2(1,0),0),
    fract((U).x)),mix(
      texelFetch(sampler,ivec2(U)+ivec2(0,1),0),
      texelFetch(sampler,ivec2(U)+ivec2(1,1),0),
    fract((U).x)),fract((U).y));
}
#endif


vec4 textureLinearPix (sampler2D sampler, vec2 U) {
  return mix(mix(
      texture(sampler,(vec2(ivec2(U)+ivec2(0,0))+0.5)/iR),
      texture(sampler,(vec2(ivec2(U)+ivec2(1,0))+0.5)/iR),
    fract((U).x)),mix(
      texture(sampler,(vec2(ivec2(U)+ivec2(0,1))+0.5)/iR),
      texture(sampler,(vec2(ivec2(U)+ivec2(1,1))+0.5)/iR),
    fract((U).x)),fract((U).y));
}
#define textureLinear(sampler,P) textureLinearPix(sampler, \
    (P)*iChannelResolution[SAMPLERINDEX.sampler].xy-vec2(.5))

#define texture(sampler,P) ( LINEARFILTER.iChannel0 ? \
    textureLinear(sampler,P) \
  : texture(sampler,P) )
// End of Linear Filtering Shim

/*
** This is fluid
** Following this tutorial : http://wyattflanders.com/MeAndMyNeighborhood.pdf
*/

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
    iR = iResolution.xy;

    if (f.x < 10. || f.y < 10. || -f.x+R.x < 10. || -f.y+R.y < 10.) {o = vec4(0.); return;}

    float kb = C(vec2(32.5/256., 0.25)).x;
    if (kb > .5 || iFrame < 10) {o = vec4(0); return;}

    o = F(f);
    vec4 En = F(f+n);
    vec4 Es = F(f+s);
    vec4 Ew = F(f+w);
    vec4 Ee = F(f+e);

    o.z = (En + Es + Ew + Ee).z * .25;//06125;

    o.xy += vec2(Ee.z - Ew.z, Es.z - En.z) * .25;

    o.z += (Es.y - En.y + Ee.x - Ew.x) *.25;

    //o.xy += (B(f/R).xy -.5)/400.;
    //o.y += -o.w*sin(1.57+1.*g(D(f/R)))/400.;//*iTimeDelta*1.;

    o.y += -o.w*iTimeDelta*.75;
//    o.y += -o.w/400.;
    //o.xy += -o.w*(cs(length(o.w)*6.28)/200.);
	o.w += B(f/R).y*iTimeDelta*1.;
    o.w += (Ee.x*Ee.w-Ew.x*Ew.w+Es.y*Es.w-En.y*En.w) * .25;

    //o.xy += o.w*cs(o.w*50.*1.0+g(B(f/R))*5000. )*.505;
    //o.xy += o.w*cs( (o.w*10./(1.0001+g(B(f/R)))) * 100.)*.10501;
    //o.xy += -NA(f/R).xy*1./8.;
    //o.w += .001001 * (g(B(f/R))*2.-1.);

    //if (iMouse.z > .5 && length(f-iMouse.xy) < 100.) o.w = .5;

    //if (f.x < 9. || f.y < 9. || -f.x+R.x < 9. || -f.y+R.y < 9.) o *= .0;

    o = clamp(o, -1.0, 1.);
}


// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<

#define R iResolution.xy

vec2 iR;

//code shortcuts
#define A(u) texture(iChannel0, u)
#define B(u) texture(iChannel1, u)
#define C(u) texture(iChannel2, u)
#define D(u) texture(iChannel3, u)
