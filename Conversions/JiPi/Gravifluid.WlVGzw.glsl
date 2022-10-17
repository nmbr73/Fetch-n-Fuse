

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Fork of "Oil and water" by michael0884. https://shadertoy.com/view/wtVGzW
// 2020-01-09 01:38:11

void mainImage( out vec4 fragColor, in vec2 p )
{
    vec4 Q = pixel(ch0, p);
    vec4 Qv = pixel(ch1, p);
    
    
    vec2 g = 0.1*normalize(Grad(ch0, p));
    vec3 v = vec3(g,  sqrt(1.-sq(g)) );
    vec3 col = texture(ch2, v).xyz;
    //fluid 1 amplitude
    float fd1 = smoothstep(0.0, 0.7, 1.*dot(Q.xy,Q.xy)/sq(a1)) * ( 1. + 0.001*p.y);
    //fluid 2 amplitude
    float fd2 = log(Q.z*Q.z+1.);

    // Output to screen
    fragColor.xyz = col*(sin(vec3(1., 1., 0.)*fd1) + sin(vec3(0., 0.3, 1.)*fd2));
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define pixel(a, p) texture(a, p/vec2(textureSize(a,0)))
#define texel(a, p) texelFetch(a, ivec2(p-0.5),0)
#define s2d iResolution.xy
#define ch0 iChannel0
#define ch1 iChannel1
#define ch2 iChannel2
#define ch3 iChannel3

#define dt 0.5


const float scale = .6;
//fluid 1 density 
const float a1 = 0.8;
//fluid 2 density
const float a2 = 1.;

#define sign vec4(1.,1.,1,1.)

//set to 0 if you want a gas-like behaviour
//fluid 1 fluid-like/gas-like regulator 
const float b1 = 0.99;
//fluid 2 fluid-like/gas-like regulator 
const float b2 = 0.;

//interaction energy cost
const float ie = -0.1;

//initial conditions for amplitudes
const float amp = 0.6;
const vec4 fluid1_Q = amp*vec4(a1,0,0,0);
const vec4 fluid1_Qv = amp*vec4(0,a1,0,0); //minus for antifluid
const vec4 fluid2_Q = 0.*amp*vec4(0,0,a2,0);
const vec4 fluid2_Qv = 0.*amp*vec4(0,0,0,a2);
const float mouser = 25.;
const float initr = 20.;


float sq(float x){ return x*x; }
float cb(float x){ return x*x*x; }
float sq(vec2 x){ return dot(x,x); }

const float pressure = 0.001;

//wave potential
float P(vec4 Q, vec2 p)
{
    //fluid 1 amplitude
    float fd1 = length(Q.xy);
    //fluid 2 amplitude
    float fd2 = length(Q.zw);
    
    //liquifier term 1
    float liq1 = 1. - b1*exp(-3.*sq(fd1-a1));      
    //liquifier term 2
    float liq2 = 1. - b2*exp(-3.*sq(fd2-a2));     
    float grav = 0.002*length(p - vec2(400,225));
        
    float E =(scale*(cb(fd1)*liq1)*(1. - tanh(0.15*Q.z)) + 0.000*sq(Q.z));
    return 20.*tanh(0.05*E);
}

//force
#define d 0.001
vec4 F(vec4 Q, vec2 p)
{
    vec3 dx = 0.5*vec3(-d,0.,d);
    return vec4(P(Q + dx.zyyy, p) - P(Q + dx.xyyy, p),
                P(Q + dx.yzyy, p) - P(Q + dx.yxyy, p),
                P(Q + dx.yyzy, p) - P(Q + dx.yyxy, p),
                P(Q + dx.yyyz, p) - P(Q + dx.yyyx, p))/d;
}

//Laplacian operator
vec4 Laplace(sampler2D ch, vec2 p)
{
    vec3 dx = vec3(-1,0.,1);
    return texel(ch, p+dx.xy)+texel(ch, p+dx.yx)+texel(ch, p+dx.zy)+texel(ch, p+dx.yz)-4.*texel(ch, p);
}

vec2 Grad(sampler2D ch, vec2 p)
{
    vec3 dx = vec3(-1,0.,1);
    return vec2(length(texel(ch, p+dx.zy)),length(texel(ch, p+dx.yz))) - length(texel(ch, p));
}

float dborder(vec2 x, vec2 s)
{
    return min(min(min(x.x, x.y), s.x-x.x),s.y-x.y);
}

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void mouse(inout vec4 Q, inout vec4 Qv, vec2 p)
{
    if(iMouse.z > 0.)
    {
        float f1 = exp(-sq((p-iMouse.xy)/mouser));
        Q += 0.1*fluid1_Q*f1;
        Qv += 0.1*fluid1_Qv*f1;
    }
}

void mainImage( out vec4 Q, in vec2 p )
{
    //get old value
    Q = texel(ch0, p);
    vec4 Qv = texel(ch1, p);
    
    Q += Qv*dt + vec4(0.0001,0.0001,0.001,0.001)*Laplace(ch0, p);
   
    mouse(Q,Qv,p);
    
    Q.z *= 1.-0.1*exp(-0.002*sq(dborder(p, s2d)));
    
    if(iFrame < 1) 
    {
        float f1 = step(sq((p-s2d*0.3)/initr),1.);
        float f2 = step(sq((p-s2d*0.7)/initr),1.);
        Q = fluid1_Q*f1*(1.-f2) + fluid1_Q*f2*(1.-f1);
    }
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
void mouse(inout vec4 Q, inout vec4 Qv, vec2 p)
{
    if(iMouse.z > 0.)
    {
        float f1 = exp(-sq((p-iMouse.xy)/mouser));
        Q += 0.1*fluid1_Q*f1;
        Qv += 0.1*fluid1_Qv*f1;
    }
}


void mainImage( out vec4 Qv, in vec2 p )
{
    //get old value
    vec4 Q = texel(ch0, p);
    Qv = texel(ch1, p);
    
   	Qv += dt*(Laplace(ch0, p) - sign*F(Q, p));
    
    mouse(Q,Qv,p);
    
    if(iFrame < 1) 
    {
        float f1 = step(sq((p-s2d*0.3)/initr),1.);
        float f2 = step(sq((p-s2d*0.7)/initr),1.);
        Qv = fluid1_Qv*f1*(1.-f2) + fluid1_Qv*f2*(1.-f1);
    }
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
void mouse(inout vec4 Q, inout vec4 Qv, vec2 p)
{
    if(iMouse.z > 0.)
    {
        float f1 = exp(-sq((p-iMouse.xy)/mouser));
        Q += 0.1*fluid1_Q*f1;
        Qv += 0.1*fluid1_Qv*f1;
    }
}

void mainImage( out vec4 Q, in vec2 p )
{
    //get old value
    Q = texel(ch0, p);
    vec4 Qv = texel(ch1, p);
    
    Q += Qv*dt + 0.0005*Laplace(ch0, p);
   
    mouse(Q,Qv,p);
    
    if(iFrame < 1) 
    {
        float f1 = step(sq((p-s2d*0.3)/initr),1.);
        float f2 = step(sq((p-s2d*0.7)/initr),1.);
        Q = fluid1_Q*f1*(1.-f2) + fluid1_Q*f2*(1.-f1);
    }
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
void mouse(inout vec4 Q, inout vec4 Qv, vec2 p)
{
    if(iMouse.z > 0.)
    {
        float f1 = exp(-sq((p-iMouse.xy)/mouser));
        Q += 0.1*fluid1_Q*f1;
        Qv += 0.1*fluid1_Qv*f1;
    }
}


void mainImage( out vec4 Qv, in vec2 p )
{
    //get old value
    vec4 Q = texel(ch0, p);
    Qv = texel(ch1, p);
    
   	Qv += dt*(Laplace(ch0, p) - sign*F(Q, p));
    
    mouse(Q,Qv,p);
    
    if(iFrame < 1) 
    {
        float f1 = step(sq((p-s2d*0.3)/initr),1.);
        float f2 = step(sq((p-s2d*0.7)/initr),1.);
        Qv = fluid1_Qv*f1*(1.-f2) + fluid1_Qv*f2*(1.-f1);
    }
}