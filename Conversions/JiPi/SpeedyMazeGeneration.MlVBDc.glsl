

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
//Click and drag to damage the maze and watch it heal

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 a=texture(iChannel0,fragCoord/iResolution.xy).xy;
    fragColor=a.x*vec4(hsv2rgb(vec3(a.y/1000.,1.,1.)),1.);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
//by copying this code into multiple buffers it is possible to generate mazes 4x as fast
#define g(x,y) texture(iChannel0,(vec2(x,y)+U)/R)

void mainImage( out vec4 O, vec2 U )
{
    if( iFrame == 0 )
         O = vec4( U.x<=1. && U.y<=2. );
    else {
        vec2 R  = iResolution.xy;
        vec4 a  = g(0,0),
             b  = g(1,0),
             c  = g(0,1),
             d  = g(-1,0),
             e  = g(0,-1),
             h0 = g(-1,-1),
             h1 = g(1,-1),
             h2 = g(-1,1),
             h3 = g(1,1);
        int r = int( 4.* hash13(vec3(U,iTime+iDate.a)) ), f;
        vec2 p;
        if (r==0) { p = b.xy; f = int(h0+h2); }
        if (r==1) { p = c.xy; f = int(h0+h1); }
        if (r==2) { p = d.xy; f = int(h1+h3); }
        if (r==3) { p = e.xy; f = int(h2+h3); }
        
        O = (2.*a+b+c+d+e).x==1. && f==0 && p.x==1.
        	? vec4(1,1.+p.y,0,0)
            : a;

        U = abs( U-iMouse.xy ); 
        U = min( U, abs(U-R) );
     	if( length(U)<50. && iMouse.z>0.) O = vec4(0);
    }
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
//https://www.shadertoy.com/view/4djSRW
//  1 out, 2 in...
float hash12(vec2 p)
{
	vec3 p3  = fract(vec3(p.xyx) * .1031);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}
//  1 out, 3 in...
float hash13(vec3 p3)
{
	p3  = fract(p3 * .1031);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}

vec3 hsv2rgb(vec3 c){
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}