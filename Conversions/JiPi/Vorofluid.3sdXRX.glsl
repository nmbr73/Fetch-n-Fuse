

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Fork of "Voronoi vortex particle fluid" by michael0884. https://shadertoy.com/view/WdcXzS
// 2019-10-30 21:27:02
const int KEY_UP = 38;
const int KEY_DOWN  = 40;

vec4 B(vec2 pos)
{
   return SAMPLE(iChannel1, pos, size);
}

//density and velocity
vec3 pdensity(vec2 pos)
{
   vec4 particle_param = SAMPLE(iChannel0, pos, size);
   return vec3(particle_param.zw,gauss(pos - particle_param.xy, 0.7*radius));
}

void mainImage( out vec4 fragColor, in vec2 pos )
{
   vec3 density = pdensity(pos);
   vec4 blur = SAMPLE(iChannel1, pos, size);
    float vorticity = B(pos+vec2(1,0)).y-B(pos-vec2(1,0)).y-B(pos+vec2(0,1)).x+B(pos-vec2(0,1)).x;
   //fragColor = vec4(SAMPLE(iChannel2, pos, size).xyz  + 0.8*vec3(0.4,0.6,0.9)*vorticity,1.0);
    if(texelFetch( iChannel2, ivec2(KEY_UP,2), 0 ).x > 0.5)
    {
        fragColor = vec4(2.*density.z*(7.*abs(density.xyy)+vec3(0.2, 0.1, 0.1)),1.0);
      
    }
    else
    {
     
         fragColor = vec4(200.*vec3(vorticity, abs(vorticity)*0.3, - vorticity),1.0);
            fragColor = vec4(10.*abs(density.xyy) + 30.*vec3(0,0,abs(blur.z)),1.0);
    }
    
    
}


// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
//voronoi particle tracking 

void Check(inout vec4 U, vec2 pos, vec2 dx)
{
    vec4 Unb = SAMPLE(iChannel0, pos+dx, size);
    vec2 sizep = size - vec2(1,1);
    vec2 rpos1 = mod(pos-Unb.xy+size*0.5,size) - size*0.5;
    vec2 rpos2 = mod(pos-U.xy+size*0.5,size) - size*0.5;
    //check if the stored neighbouring particle is closer to this position 
    if(length(rpos1) < length(rpos2))
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
    Check(U, pos, vec2(-1,-1));
    Check(U, pos, vec2(1,1));
    Check(U, pos, vec2(1,-1));
    Check(U, pos, vec2(1,-1));
    Check(U, pos, vec2(-2,0));
    Check(U, pos, vec2(2,0));
    Check(U, pos, vec2(0,-2));
    Check(U, pos, vec2(0,2));
    
    vec2 ppos = U.xy*(1. - divergence) + divergence*pos;

    //dont make the particles be too close
    vec2 repulsion = vec2(B(ppos+vec2(1,0)).w - B(ppos+vec2(-1,0)).w, B(ppos+vec2(0,1)).w - B(ppos+vec2(0,-1)).w);
    vec2 pressure = vec2(B(ppos+vec2(1,0)).z - B(ppos+vec2(-1,0)).z, B(ppos+vec2(0,1)).z - B(ppos+vec2(0,-1)).z);
    //mouse interaction
    if(iMouse.z>0.)
    {
        float k = gauss(ppos-iMouse.xy, 5.);
        U.zw = U.zw*(1.-k) + k*0.2*vec2(cos(0.03*iTime*dt), sin(0.03*iTime*dt));
    }

     U.zw += 0.002*vec2(cos(0.03*iTime*dt), sin(0.03*iTime*dt))*gauss(ppos-size*vec2(0.5,0.5),8.)*dt;
    //update the particle
    
    U.zw = U.zw;
    U.zw += P*pressure*dt;
    //smooth velocity
    vec2 velocity = 0.*B(ppos).xy + U.zw - 0.000*repulsion;
    U.xy += dt*velocity;
    U.xy = mod(U.xy,size); //limit the position to the texture
    
    
    if(iFrame < 1)
    {
      	if(mod(pos, vec2(1./particle_density)).x < 1. && mod(pos, vec2(1./particle_density)).y < 1.)
           U = vec4(pos,0.,0.);
      
    }
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define size iResolution.xy
#define SAMPLE(a, p, s) texture((a), (p)/s)

float gauss(vec2 x, float r)
{
    return exp(-pow(length(x)/r,2.));
}
#define SPEED
   
#define PI 3.14159265

#ifdef SPEED
//high speed
    #define dt 6.5
    #define P 0.03
	#define divergence 0.3
#else
//high precision
 	#define dt 2.
    #define P 0.08
	#define divergence 0.0002
#endif

//how many particles per pixel, 1 is max
#define particle_density 1.

const float radius = 2.0;

// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
vec4 B(vec2 pos)
{
   return SAMPLE(iChannel1, pos, size);
}

//density and velocity
vec3 pdensity(vec2 pos)
{
   vec4 particle_param = SAMPLE(iChannel0, pos, size);
   return vec3(particle_param.zw,gauss(pos - particle_param.xy, 0.7*radius));
}

const vec2 damp = vec2(0.000,0.01);
const vec2 ampl = vec2(0.1,1.);

void mainImage( out vec4 u, in vec2 pos )
{
    vec4 prev_u = SAMPLE(iChannel1, pos, size);
   
  
    vec3 density = pdensity(pos);
     //exponential rolling average
      u.xyz =  0.5*density;
    float div = B(pos+vec2(1,0)).x-B(pos-vec2(1,0)).x+B(pos+vec2(0,1)).y-B(pos-vec2(0,1)).y;
    u.zw = (1.-0.001)*0.25*(B(pos+vec2(0,1))+B(pos+vec2(1,0))+B(pos-vec2(0,1))+B(pos-vec2(1,0))).zw;
    u.zw += ampl*vec2(div,density.z);
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
vec4 B(vec2 pos)
{
   return SAMPLE(iChannel1, pos, size);
}

//density and velocity
vec3 pdensity(vec2 pos)
{
   vec4 particle_param = SAMPLE(iChannel0, pos, size);
   return vec3(particle_param.zw,gauss(pos - particle_param.xy, 0.7*radius));
}

const vec2 damp = vec2(0.000,0.01);
const vec2 ampl = vec2(0.1,1.);

void mainImage( out vec4 u, in vec2 pos )
{
    vec4 prev_u = SAMPLE(iChannel1, pos, size);
   
  
    vec3 density = pdensity(pos);
     //exponential rolling average
      u.xyz = 0.5*density;
    float div = B(pos+vec2(1,0)).x-B(pos-vec2(1,0)).x+B(pos+vec2(0,1)).y-B(pos-vec2(0,1)).y;
    u.zw = (1.-0.001)*0.25*(B(pos+vec2(0,1))+B(pos+vec2(1,0))+B(pos-vec2(0,1))+B(pos-vec2(1,0))).zw;
    u.zw += ampl*vec2(div,density.z);
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
vec4 B(vec2 pos)
{
   return SAMPLE(iChannel1, pos, size);
}

//density and velocity
vec3 pdensity(vec2 pos)
{
   vec4 particle_param = SAMPLE(iChannel0, pos, size);
   return vec3(particle_param.zw,gauss(pos - particle_param.xy, 0.7*radius));
}

const vec2 damp = vec2(0.000,0.01);
const vec2 ampl = vec2(0.1,1.);

void mainImage( out vec4 u, in vec2 pos )
{
    vec4 prev_u = SAMPLE(iChannel1, pos, size);
   
  
    vec3 density = pdensity(pos);
     //exponential rolling average
      u.xyz = 0.5*density;
    float div = B(pos+vec2(1,0)).x-B(pos-vec2(1,0)).x+B(pos+vec2(0,1)).y-B(pos-vec2(0,1)).y;
    u.zw = (1.-0.001)*0.25*(B(pos+vec2(0,1))+B(pos+vec2(1,0))+B(pos-vec2(0,1))+B(pos-vec2(1,0))).zw;
    u.zw += ampl*vec2(div,density.z);
}