

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
vec4 light(sampler2D src, vec2 fragCoord)
{
    vec4 l = vec4(0.0);
    l += texture(src, (fragCoord+vec2( 0,  0))/iResolution.xy);
    l -= texture(src, (fragCoord+vec2(-1,  0))/iResolution.xy)*0.5;
    l -= texture(src, (fragCoord+vec2( 0, -1))/iResolution.xy)*0.5;
    return l;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec4 col = texture(iChannel0, fragCoord/iResolution.xy);
    vec4 l = light(iChannel0, fragCoord);
	fragColor.xyz = sqrt(col.xyz*7.5*col.w)-l.www*15.0+l.xyz*4.0;
}
// >>> ___ GLSL:[Buf A] ____________________________________________________________________ <<<
#define rnd(U) hash12(U)

#define T(i,j) texture(iChannel0, fract( ( U +vec2(i,j) ) / iResolution.xy) )

#define PREVIEW (iResolution.y < 250.0)

/*
#define K 0.3
#define L 0.7
#define M 0.6
*/


#define K 0.44
#define L 0.7
#define M 1.0


/*
#define K 0.7
#define L 1.4
#define M 0.3
*/

/*
#define K 0.35
#define L 0.84
#define M 0.4
*/

/*
#define K 0.2
#define L 0.62
#define M 1.0
*/

/*
#define K 0.01
#define L 0.49
#define M 10.0
*/

/*
#define K 0.1
#define L 0.54
#define M 1.6
*/

/*
#define K 0.06
#define L 0.52
#define M 2.65
*/

#define TIMESTEP (PREVIEW ? 1.0 : 0.3)

// https://www.shadertoy.com/view/4djSRW
#define HASHSCALE1 .1031
float hash12(vec2 p)
{
	vec3 p3  = fract(vec3(p.xyx) * HASHSCALE1);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}

vec3 hsv2rgb(vec3 hsv)
{
    vec3 rgb;
    float h = mod(hsv.x * 6.0, 6.0);
    float q = h-float(int(h));
    if      (h < 1.0) rgb = vec3( 1.0,    q,  0.0);
    else if (h < 2.0) rgb = vec3(1.-q,  1.0,  0.0);
    else if (h < 3.0) rgb = vec3( 0.0,  1.0,    q);
    else if (h < 4.0) rgb = vec3( 0.0, 1.-q,  1.0);
    else if (h < 5.0) rgb = vec3(   q,  0.0,  1.0);
    else if (h < 6.0) rgb = vec3( 1.0,  0.0, 1.-q);
    rgb = hsv.z*(1.0-hsv.y*(1.0-rgb));
    return rgb;
}

void mainImage( out vec4 O,  vec2 U )
{
    vec2 uv = U/iResolution.xy;
    if (iTime < 1.5 && !PREVIEW)
    {
        O = vec4(0.0);
        return;
    }
    vec4 old = T(0, 0);
	O = ( T(-1, 1) + T(0, 1) + T(1, 1) 
        + T(-1, 0) + T(0, 0) + T(1, 0) 
        + T(-1,-1) + T(0,-1) + T(1,-1) ) / 9.;
    O *= 2.0-O;

    if (iFrame==0) O = vec4(.4);
    float time = mod(iTime, 1000.01246341);
    for (int i=0; i<3; ++i)
    {
        if (rnd(U-9.*time+.0+0.03*float(i))>.9999) O[i] = 1.0;
        if (rnd(U-9.*time+.1+0.03*float(i))>.9999) O[i] = 0.0;
    }
    float m = iMouse.z > 0.0 ? max(0.0, 1.0-length(iMouse.xy-U)/10.0) : 0.0;
    if (rnd(U-9.*time+.23)<m)
    {
        for (int i=0; i<4; ++i)
        	O[i] += 4.0*m*(rnd(U-9.*time+.3+0.03*float(i))-0.5);
    }
    O.w = mix(O.w, dot(O.xyz, O.yzx)/3.0, K)*L;
    O.xyz -= O.w*(O.xyz+M);
    for (int i=0; i<3; ++i)
    {
    	if (O[i] < 0.0) O[i] = 0.0;
    }
    O.xyz *= O.w*0.5+hsv2rgb(vec3(iTime*0.04+O.w*4.0, 0.1, 1.0));
    O = mix(old.xyzw, O.xyzw, TIMESTEP);
}