

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define R iResolution
// ---------------------- utils from https://www.shadertoy.com/view/llySRh
#define H(p) ( 2.* fract(sin((p)*mat2(127.1,311.7, 269.5,183.3)) *43758.5453123) -1. )

vec4 char(vec2 p, int c) {
   // vec2 dFdx = dFdx(p/16.), dFdy = dFdy(p/16.);
   vec2 dFdx = vec2(3./R.y/16.,0), dFdy = dFdx.yx ;
    if (p.x<.0|| p.x>1. || p.y<0.|| p.y>1.) return vec4(0,0,0,1e5);
	return textureGrad( iChannel0, p/16. + fract( vec2(c, 15-c/16) / 16. ), 
                        dFdx, dFdy );
}
#define spc  U.x-=.5;
#define C(c) spc O += char(U,64+32+c).x;
// ----------------------

void mainImage( out vec4 O, vec2 u ) {
    vec2 U = u / R.y, 
         p = floor(u/2.);
         
    float e = max(0.,.12*(U.x-.2));       // jitter
    if ( H(p+.5 ).x < e ) U += e * H(p);
    
    O -= O;
    U *= 2.5; U -= vec2(-.3,.8);          // draw text
    C(5)C(14)C(20)C(18)C(15)C(16)C(25)    // "entropy"
    O = 1.-O;
}
