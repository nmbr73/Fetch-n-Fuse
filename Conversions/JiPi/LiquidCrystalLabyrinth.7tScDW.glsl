

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// WebGL2 cleaned-up version of "Viscous Fingering" by cornusammonis. https://shadertoy.com/view/Xst3Dj

void mainImage(out vec4 O, vec2 u) {
vec2 U = u;

    
    O.r += 0.5f * normalize(T()).x; 
    O.g += 0.25f * normalize(T()).y; 
    O.b += 0.25f * normalize(T()).z; 
    O += .5 * normalize(T()).zzzz; 
    
    
    }
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<


void mainImage( out vec4 O, vec2 U )
{
float _K0 = -20./6., // center weight
      _K1 =   4./6., // edge-neighbors
      _K2 =   1./6., // vertex-neighbors
       cs =  .12052 + (U.x*0.0000051),    // curl scale
       ls =  .12052 + (U.x*0.0000051),    // laplacian scale
       ps = -.06,    // laplacian of divergence scale
       ds = -.08,    // divergence scale
      pwr =  .2,     // power when deriving rotation angle from curl
      amp = 0.999+(U.y*0.000051),      // self-amplification
      sq2 =  .7;     // diagonal weight

// 3x3 neighborhood coordinates
    vec4 uv = T( ),
          n = T(vec2( 0,  1 )),
          e = T(vec2( 1,  0 )),
          s = T(vec2( 0, -1 )),
          w = T(vec2(-1,  0 )),
         nw = T(vec2(-1,  1 )),
         sw = T(vec2(-1     )),
         ne = T(vec2( 1     )),
         se = T(vec2( 1, -1 ));
    
    // uv.x and uv.y are our x and y components, uv.z is divergence 

    // laplacian of all components
    vec4 lapl  = _K0*uv + _K1*(n + e + w + s) 
                        + _K2*(nw + sw + ne + se);
    float sp = ps * lapl.z;
    
    // calculate curl
    // vectors point clockwise about the center point
    float curl = n.x - s.x - e.y + w.y 
        + sq2 * (nw.x + nw.y + ne.x - ne.y + sw.y - sw.x - se.y - se.x);
    
    // compute angle of rotation from curl
    float a = cs * sign(curl) * pow(abs(curl), pwr);
    
    // calculate divergence
    // vectors point inwards towards the center point
    float div  = s.y - n.y - e.x + w.x 
        + sq2 * (nw.x - nw.y - ne.x - ne.y + sw.x + sw.y + se.y - se.x);
    float sd = ds * div;

    vec2 norm = normalize(uv.xy);
    
    // temp values for the update rule
     vec2 t = (amp * uv + ls * lapl + uv * sd).xy + norm * sp;
     float red = sd;
     float green = div;
     float blue = t.x;
     //O = vec4(red,green, blue, 1.0);
     //O = clamp(vec4(t,div,0), -1., 1.);
     
     t *= mat2(cos(a), -sin(a), sin(a), cos(a) );
    if(iFrame<10)
        O = -.5 + texture(iChannel1, U/R), O.a=0.;
     else 
        O = clamp(vec4(t,div,0), -1., 1.);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R    iResolution.xy//
#define T(d) texelFetch(iChannel0, ivec2(d+U)%ivec2(R),0)