

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// I expect someone has done this better, faster, cheaper somewhere
// and I wouldn't mind seeing how, but I didn't see any other
// examples of such cubemap face debugging tool here on the site,
// so I took the first thing I got working and made a toy out of it.
// Hopefully someone will find it useful.  I'd appreciate any tips.
// heck I probably got the face id's wrong or uv's backward or upside down.

// Hey!  I did find something related, finally:  https://shadertoy.com/view/3l2SDR
// Fabrice has some coordinate conversion code:  https://shadertoy.com/view/WdlGRr
// Wunkolo has some stuff here I hope to grok:   https://shadertoy.com/view/wltXDl

#define CUBEMAP iChannel1 // pick a channel


void mainImage(out vec4 c, vec2 p)
{
    vec2 R = iResolution.xy
        , q = (p + p - R)/R.y;
    c.rgb = Unwrap(CUBEMAP, q * .755).rgb;
//    c.rgb = pow(c.rgb, vec3(1./2.2)); // gamma (disable for texture cube sources, or fix those on load)
    c.a = 1.;
}


// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
// these I just threw together over in https://shadertoy.com/view/wtVSDw
// but they seem to work, or at least seem to be inverses of each other.

int CubeFaceOfDir(vec3 d) // just the face id
{
    vec3 a = abs(d);
    int f = a.x >= a.y ? (a.x >= a.z ? 0 : 2) : (a.y >= a.z ? 1 : 2);
    int i = f + f;
    if (d[f] < 0.) ++i;
    return i;
}
// takes normalized direction vector, returns uv in c.xy and face id in c.z
vec3 DirToCubeFace(vec3 d)
{
    int i = CubeFaceOfDir(d)
    , f = i >> 1;
    vec2 uv;
    switch (f) {
        case 0: uv = d.yz; break;
        case 1: uv = d.xz; break;
        case 2: uv = d.xy; break;
    }
    uv /= abs(d[f]); // project
    if ((i&1) != 0) // negative faces are odd indices
        uv.x = -uv.x; // flip u
    return vec3(uv, float(i));
}
// takes uv in c.xy and face id in c.z, returns unnormalized direction vector
vec3 CubeFaceToDir(vec3 c)
{
    int i = int(c.z); 
    vec3 d = vec3(c.xy, 1. - 2. * float(i & 1));
    d.x *= d.z; // only unflip u 
    switch (i >> 1) { // f
        case 0: d = d.zxy; break;
        case 1: d = d.xzy; break;
        case 2: d = d.xyz; break;
    }
    return d; // needs normalized probably but texture() doesn't mind.
}

// just for debugging so probably broken and imprecise.
// in fact it's a big ol' kludge atm.  what a mess!  I'll try to improve it as I get time.
vec4 Unwrap(samplerCube ch, vec2 q)
{
    vec2 uv = q * .5 + .5;
    uv *= 4.;
    uv -= vec2(.0,.5);
    int i = -1;
    if (uv.y >= 1. && uv.y < 2.) {
        int f = int(floor(uv.x));
        if (f >= 0 && f < 2) i = 3*f + 1;
     	else if (f >= 2 && f < 4) i = 5*f - 10;
        if (f == 2) uv = vec2(uv.y, -uv.x); // maybe rotate, different directions
        else if (f == 0) uv = vec2(-uv.y, uv.x);
    } else {
		if (int(uv.x) == 1) {
        	if (uv.y >= 0. && uv.y < 1.) { i = 3; uv.x = 0.-uv.x; }
        	else if (uv.y >= 2. && uv.y < 3.) { i = 2; uv.y = 0.-uv.y; }
    	}
    }
	if (!(i >= 0)) return vec4(vec3(.7),1);
    uv = fract(uv);
    vec3 d = CubeFaceToDir(vec3(uv * 2. - 1., float(i)));
//    d = CubeFaceToDir(DirToCubeFace(d)); // ensure can convert back&forth flawlessly
//    d = CubeFaceToDir(DirToCubeFace(d));
    vec4 c = textureLod(ch, d, 0.);
    //c.rgb = pow(c.rgb, vec3(2.2)); // gamma correction - skipping as it cancels out here
    return c;
} // result in srgb gamma atm

