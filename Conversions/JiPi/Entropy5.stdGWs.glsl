

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// variant of https://shadertoy.com/view/sld3Dl

// ---------------------- utils from https://www.shadertoy.com/view/llySRh
#define H(p) fract(sin((p)*mat2(127.1,311.7, 269.5,183.3)) *43758.5453123)

vec4 char(vec2 p, int c) {
   // vec2 dFdx = dFdx(p/16.), dFdy = dFdy(p/16.);
   vec2 dFdx = vec2(3./R.y/16.,0), dFdy = dFdx.yx ;
    if (p.x<.0|| p.x>1. || p.y<0.|| p.y>1.) return vec4(0,0,0,1e5);
	return textureGrad( iChannel1, p/16. + fract( vec2(c, 15-c/16) / 16. ), 
                        dFdx, dFdy );
}
#define spc  U.x-=.5;
#define C(c) spc d = min(d, char(U,64+32+c).w );
// ----------------------

void mainImage( out vec4 O, vec2 u ) {
    vec2 U = u / R.y, 
         p = (u/2.);
         
    float d = 9.;
    U = T(u).xy;
    U *= 2.5; U -= vec2(-.3,.8);            // draw text
    C(5)C(14)C(20)C(18)C(15)C(16)C(25)      // "entropy"
    O = vec4( smoothstep(0., 4./R.y,d-.5) );
}

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void mainImage( out vec4 O, vec2 u )
{
    vec2 U = u/R.y;
    if (iFrame<1) O = vec4(U,0,0);
    else {
        vec2 P =  U - H(vec2(iTime))*R /R.y;       
        float d = length(P);
        O.xy = T(u).xy + .3* max(0.,U.x-.2)*  P* smoothstep(.1,0.,d);   // bomp uv space
     }
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R    iResolution.xy
#define H(p) fract(sin((p)*mat2(127.1,311.7, 269.5,183.3)) *43758.5453123)
#define T(U) texelFetch( iChannel0, ivec2(U), 0 )