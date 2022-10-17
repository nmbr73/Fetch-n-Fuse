

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Fork of "Hellflame" by michael0884. https://shadertoy.com/view/3d3SRS
// 2019-10-27 20:38:34

void mainImage( out vec4 fragColor, in vec2 pos )
{
    
   vec4 density = 1.3*SAMPLE(iChannel1, pos, size);
    fragColor = vec4(sin(2.*density.x*vec3(0.4,0.2,1.1)) +sin(5.*vec3(0.7,0.7,1.)*density.y),1.0);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
//voronoi particle tracking 

void Check(inout vec4 U, vec2 pos, vec2 dx)
{
    vec4 Unb = SAMPLE(iChannel0, pos+dx, size);
    //check if the stored neighbouring particle is closer to this position 
    if(length(Unb.xy - pos) < length(U.xy - pos))
    {
        U = Unb; //copy the particle info
    }
}

vec4 B(vec2 pos)
{
   return 5.*SAMPLE(iChannel1, pos, size);
}

void mainImage( out vec4 U, in vec2 pos )
{
    U = SAMPLE(iChannel0, pos, size);
    
    //check neighbours 
    Check(U, pos, vec2(-1,0));
    Check(U, pos, vec2(1,0));
    Check(U, pos, vec2(0,-1));
    Check(U, pos, vec2(0,1));
    
    //small divergence
    vec2 ppos = 0.999*U.xy+0.001*pos.xy;
    
    vec4 Bdx = B(ppos+vec2(1,0)) - B(ppos-vec2(1,0)); 
    vec4 Bdy = B(ppos+vec2(0,1)) - B(ppos-vec2(0,1));
    
    //update the particle
    U.xy -= dt*(vec2(Bdx.x - Bdx.y, Bdy.x - Bdy.y) +vec2(0.,0.45));
    
    
    if(iFrame < 1)
    {
        U = vec4(30.*floor(pos.x/30.)+60.*sin(pos.x),30.*floor(pos.y/30.)-10.*cos(pos.y+pos.y),1.,1.);
    }
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define size iResolution.xy
#define SAMPLE(a, p, s) texture((a), (p)/s)

float gauss(vec2 x, float r)
{
    return exp(-pow(length(x)/r,2.));
}
   
#define PI 3.14159265
#define dt 0.6

// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
//solve the dampened poisson equation
const float radius = 1.5;

vec4 pdensity(vec2 pos)
{
   vec4 particle_param = SAMPLE(iChannel0, pos, size);
   return vec4(particle_param.zw,1.,1.)*gauss(pos - particle_param.xy, radius);
}

vec4 B(vec2 pos)
{
   return SAMPLE(iChannel1, pos, size);
}

const vec4 damp = vec4(0.005,0.05,0.001,0.001);
const vec4 ampl = vec4(0.05,0.05,0.001,0.001);

void mainImage( out vec4 U, in vec2 pos )
{
    vec4 density = pdensity(pos);
    U = (1.-damp)*0.25*(B(pos+vec2(0,1))+B(pos+vec2(1,0))+B(pos-vec2(0,1))+B(pos-vec2(1,0)));
    U += density*ampl;
}