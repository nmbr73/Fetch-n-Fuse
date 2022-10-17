

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
/*
** Following this tutorial : http://wyattflanders.com/MeAndMyNeighborhood.pdf
*/

void mainImage( out vec4 o, in vec2 f )
{
    o = A(f/R);
	o = sin(o.z*1.0+vec4(.0,1.04,2.08,.0)+3.14*o.wwww*2.);
    /*
    o.xyz -= mix(
    vec3(.9,.8,.59)
        ,
        vec3(.7, .3, .4)
        ,
        o.w
    );
	*/
    
    //o = (1.*A(f/R).wwww);
	//o = (1.*A(f/R).xyxy);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
/*
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
    return A(r/R);//-NA(r/R)*.25;
}

void mainImage( out vec4 o, in vec2 f )
{
    if (f.x < 10. || f.y < 10. || -f.x+R.x < 10. || -f.y+R.y < 10.) {o = vec4(0.); return;}

    float kb = C(vec2(32.5/256., 0.25)).x;
    if (kb > .5 || iFrame < 10) {o = vec4(0.,0.,0., (g(B(f/R))*2.-1.)*.5 ); return;}

    o = F(f);//-NA(f/R)*.25;
    vec4 En = F(f+n);
    vec4 Es = F(f+s);
    vec4 Ew = F(f+w);
    vec4 Ee = F(f+e);

    o.z = (En + Es + Ew + Ee).z * .25;//06125;

    o.xy += vec2(Ee.z - Ew.z, Es.z - En.z) * .25;

    o.z += (Es.y - En.y + Ee.x - Ew.x) *.25;

    //o.xy += (B(f/R).xy -.5)/400.;
    //o.w += g(B(f/R));

    o.w += (Ee.x*Ee.w-Ew.x*Ew.w+Es.y*Es.w-En.y*En.w) * .25;

    o.xy += o.w*cs(o.w*50.*1.0+g(B(f/R))*5000. )*.505;
    //o.xy += o.w*cs( (o.w*10./(1.0001+g(B(f/R)))) * 100.)*.10501;
    //o.xy += -NA(f/R).xy*1./8.;
    //o.w += .001001 * (g(B(f/R))*2.-1.);

    if (iMouse.z > .5 && length(f-iMouse.xy) < 100.) o.w = .5;

    if (f.x < 9. || f.y < 9. || -f.x+R.x < 9. || -f.y+R.y < 9.) o *= .0;

    o = clamp(o, -10.0, 10.);
}


// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<

#define R iResolution.xy

// neighboor collection (not used)
vec4 N(sampler2D sam, vec2 u, vec2 iR)
{
    vec4 r = vec4(0);

    r += texture(sam, (u+vec2(+.0, +1.) )/iR );
    r += texture(sam, (u+vec2(+.0, -1.) )/iR );
    r += texture(sam, (u+vec2(+1., -0.) )/iR );
    r += texture(sam, (u+vec2(-1., +0.) )/iR );
    return r;
}

// grayscale
#define g(c) (.3*c.x + .59*c.y + .11*c.z) 

vec2 cs(float a) { return vec2(cos(a), sin(a)); }

//code shortcuts
#define NA(u) N(iChannel0, u, R)
#define NB(u) N(iChannel1, u, R)
#define NC(u) N(iChannel2, u, R)
#define ND(u) N(iChannel3, u, R)

#define A(u) texture(iChannel0, u)
#define B(u) texture(iChannel1, u)
#define C(u) texture(iChannel2, u)
#define D(u) texture(iChannel3, u)