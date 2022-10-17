

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// variant of https://shadertoy.com/view/cdfGz7
// field = texture isogrey

void mainImage( out vec4 O, vec2 u )  { 
 // O = sqrt(T(u));
    O = sqrt(1.-exp(-T(u)*vec4(4,2,1,1))); // color
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// ---- compute BBoxs of next curves

void mainImage( out vec4 O, vec2 u )
{
    R = iResolution.xy;
    if (u.y>1.) { O = texture(iChannel1,u/R); 
                  O = vec4(-dFdy(O.x),dFdx(O.x),O.x,0); // contour vectors
               // O = vec4(.1*normalize(vec2(-dFdy(O.x),dFdx(O.x))),O.x,0);
                  return; 
                }
    float i = u.x-.5, l = 9., p = 1./R.y;
    vec2  P = H(i+.5-iTime), _P=P, m=P,M=P;  // random start point
    
    for( int t=0; t <len; t++, _P = P )      // follow trajectory
        P += field(P,iChannel1),
        m = min(m,P), M = max(M,P);        

    O = vec4(m-p,M+p);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
int len = 10; // curve length ( i.e. number of samples )
vec2 R;
#define T(U)   texelFetch( iChannel0, ivec2(U), 0 )
#define H(p) ( fract(sin(float(p)*vec2(269.5,183.3)) *43758.5453123)*R/R.y )

float L(vec2 p, vec2 a,vec2 b) { // --- draw line  ( squared distance )
    p -= a, b -= a;
    float h = dot(p, b) / dot(b, b), 
          c = clamp(h, 0., 1.);     
    return h==c ? dot(p -= b*h ,p) : 1e5; 
}

vec2 field(vec2 U, sampler2D CH) {  // --- velocity field induced by texture
    return  textureLod(CH, max(vec2(0,2./R.y), U*R.y/R),0.).rg * .2;
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// --- draw only curves which BBox covers the pixel

void mainImage( out vec4 O, vec2 u )
{
     R = iResolution.xy;
     vec2 U = u / R.y, P, _P, b;
    O =  .98*T(u);                         // cumulates previous draw
   
    for( float i=0.,l; i < 400.; i++ ) {   // draw N streamlines per frame
        vec4 B = texelFetch(iChannel1,ivec2(i,0),0); // curve bbox
        b = step(B.xy,U)*step(U,B.zw);
     // if (b.x*b.y == 0.) continue;       // this form crash Firefox !
        if (b.x*b.y == 1.) {               // pixel not in BBox: skip draw
        _P = P = H(i+.5-iTime);            // random start point
        l = 9.;
        for( int t=0; t <len; t++, _P = P ) // follow trajectory
            P += field(P,iChannel1),
            l = min( l, L(U,_P,P) );
        O += .2*max(0., 1.-R.y*sqrt(l));   // draw the curve
       }
    }
}