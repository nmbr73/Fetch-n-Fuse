

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
//MIT License
//Copyright 2021 Mykhailo Moroz


vec3 hsv2rgb( in vec3 c )
{
    vec3 rgb = clamp( abs(mod(c.x*6.0+vec3(0.0,4.0,2.0),6.0)-3.0)-1.0, 0.0, 1.0 );
	rgb = rgb*rgb*(3.0-2.0*rgb); 	
	return c.z * mix( vec3(1.0), rgb, c.y);
}

void mainImage( out vec4 C, in vec2 P )
{
    R = iResolution.xy; time = iTime; Mouse = iMouse;
    P = floor(P);
    
    if(iMouse.z < 0.)
    {
       // P = iMouse.xy + 100.*(P/R.x - 0.5); 
    }
    
    float m = 0.0;
    float rho = 0.0;
    vec2 v = vec2(0.0);
    range(i, -1, 1) range(j, -1, 1)
    {
        //load data
        vec2 di = vec2(i,j);
        vec4 data0 = A(P + di);
        vec4 data1 = B(P + di);
        
        //unpack data
        float m0 = data0.y;
        vec2 x0 = decode(data0.x) - fract(P);
        vec2 v0 = decode(data1.x);
        mat2 B0 = mat2(decode(data1.y),decode(data1.z)); //velocity gradient
        //update particle position
        x0 = x0 + di + v0*dt;
        
        //find cell contribution
        vec3 o = overlap(x0, vec2(1.0));
        
        m += m0*o.z;
        
        float w = k1(x0);
        
        v += (v0 + 4.0*B0*x0)*w;
        rho += m0*w;
    }
    
    float arg = 0.5*(atan(v.y, v.x)/PI + 1.0);
    float d = 2.5*length(v);
    vec3 fluid = hsv2rgb(vec3(arg, 0.64, tanh(d)));
    
    C = vec4(mix( vec3(0.000,0.000,0.000),fluid, smoothstep(0.,1.,rho)), 1.0);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
vec2 R;
vec4 Mouse;
float time;
int frame;

#define A(p) texelFetch(iChannel0, ivec2(mod(p,R)), 0)
#define B(p) texelFetch(iChannel1, ivec2(mod(p,R)), 0)

#define CH(c, p) texelFetch(c, ivec2(mod(p,R)), 0)

//loop in range
#define range(i,a,b) for(int i = a; i <= b; i++)

#define dt 0.4
#define max_vel 1.0
#define rad 2

#define diffusion 0.005
#define gravity 0.005

#define PI 3.14159265

#define rest_rho 1.0
#define eos_pressure 0.5

//reduce rotation/shearing of low density particles
//without this the vacuum and border state very chaotic, but it still works fine
//in usual MPM that isn't necessary since the particles don't change mass
float affine_str(float m)
{
    return 1.0; //full APIC
    //return smoothstep(0.5*rest_rho,0.55*rest_rho,m);
}

//pressure equation of state
float pressure(float rho)
{
    return 1.0*(rho - 0.0); //gas
   // return eos_pressure*(pow(rho/rest_rho,4.) - 1.0); //Tait EOS (water)
}

void InitialConditions(inout float m, inout vec2 v, vec2 P)
{
    vec2 dx = P - R*vec2(0.3, 0.5);

    float d = smoothstep(R.y*0.5, R.y*0.49, length(dx));  
    m = 0.0*d;

    v = d*0.3*normalize(vec2(dx.y,-dx.x));
}

//KERNEL FUNCTIONS

float k0(vec2 dx) //linear kernel
{
    vec2 k = max(1.0 - abs(dx), 0.);
    return k.x*k.y;
}

vec3 K0(vec2 dx) //linear kernel with the center of mass
{
    vec2 k = max(1.0 - abs(dx), 0.);
    return vec3(dx*0.5, k.x*k.y);
}

float k1(vec2 dx) //quadratic kernel
{
    vec2 f = max(1.5 - abs(dx), 0.0);
    vec2 k = min(max(0.75 - dx*dx, 0.5), 0.5*f*f);
    return k.x*k.y;
}

//box size enstimator
vec2 destimator(vec2 dx)
{
    return diffusion*dt+clamp(1.0 - 2.0*abs(dx), 0.001, 1.0);
}

//box overlap with center of mass
vec3 overlap(vec2 dx, vec2 box)
{
    vec2 min0 = max(dx - box*0.5, -0.5); 
    vec2 max0 = min(dx + box*0.5, 0.5); 
    vec2 size = max(max0 - min0, 0.); 
    return vec3(0.5*(max0 + min0), size.x*size.y/(box.x*box.y));
}

//boundary
#define border_h 3.
mat2 Rot(float ang)
{
    return mat2(cos(ang), -sin(ang), sin(ang), cos(ang)); 
}

vec2 Dir(float ang)
{
    return vec2(cos(ang), sin(ang));
}


float sdBox( in vec2 p, in vec2 b )
{
    vec2 d = abs(p)-b;
    return length(max(d,0.0)) + min(max(d.x,d.y),0.0);
}

float border(vec2 p)
{
    float bound = -border_h-sdBox(p - R*0.5, R*vec2(0.5, 0.5)); 

    return bound;
}

#define h 5.
vec3 bN(vec2 p)
{
    vec3 dx = vec3(-h,0,h);
    vec4 idx = vec4(-1./h, 0., 1./h, 0.25);
    vec3 r = idx.zyw*border(p + dx.zy)
           + idx.xyw*border(p + dx.xy)
           + idx.yzw*border(p + dx.yz)
           + idx.yxw*border(p + dx.yx);
    return vec3(normalize(r.xy), r.z + 1e-4);
}

//DATA PACKING

uint pack(vec2 x)
{
    x = 65534.0*clamp(0.5*x+0.5, 0., 1.);
    return uint(round(x.x)) + 65535u*uint(round(x.y));
}

vec2 unpack(uint a)
{
    vec2 x = vec2(a%65535u, a/65535u);
    return clamp(x/65534.0, 0.,1.)*2.0 - 1.0;
}

vec2 decode(float x)
{
    uint X = floatBitsToUint(x);
    return unpack(X)*3.0; 
}

float encode(vec2 x)
{
    uint X = pack(x/3.0);
    return uintBitsToFloat(X); 
}

//particle to grid
vec4 P2G(sampler2D a, sampler2D b, vec2 P)
{
    vec2 x = vec2(0.0);
    vec2 v = vec2(0.0);
    float m = 0.0;
    float rho = 0.0;
    
    range(i, -1, 1) range(j, -1, 1)
    {
        //load data
        vec2 di = vec2(i,j);
        vec4 data0 = CH(a, P + di);
        vec4 data1 = CH(b, P + di);
        
        //unpack data
        float m0 = data0.y;
        float r0 = data0.w;
        vec2 x0 = decode(data0.x);
        vec2 v0 = decode(data1.x);
        mat2 B0 = mat2(decode(data1.y),decode(data1.z)); //velocity gradient
        
        //estimate the shape of the distribution
        vec2 box = destimator(x0);
        
        //predicted density using the affine matrix, thanks to Grant Kot for this idea
        //float rho = ;//;
        mat2 stress = -mat2(clamp(pressure(data1.w/determinant(mat2(1.0) + 4.0*B0*dt)), -1., 1.));
        
        //update particle position
        x0 = x0 + di + v0*dt;
    
        //find cell contribution
        vec3 o = overlap(x0, box);
         
        //update distribution
        x += m0*o.xy*o.z;
        m += m0*o.z;
        
        //find grid node contribution
        float w = k1(x0);
        
        //distribute momentum onto grid
        v += (v0 + 4.0*B0*x0 + dt*stress*x0)*w*m0;
        rho += m0*w;
    }
    
    
    //normalize
    if(rho > 0.0) v /= rho;
    if(m > 0.0) x /= m;
    
    m = mix(m, 0.3, dt*0.0085);
   
    if(length(P - R*vec2(0.8, 0.9)) < 10.) 
    {
       // x = vec2(0.0);
       // v = 1.8*Dir(-PI*0.25 - PI*0.5 + 0.3*sin(0.4*time));
       // m = mix(m, rest_rho, 0.4*dt);
    }

    if(length(P - R*vec2(0.2, 0.9)) < 10.) 
    {
       //x = vec2(0.0);
       // v =1.8*Dir(-PI*0.25 + 0.3*sin(0.4*time));
       // m = mix(m, rest_rho, 0.4*dt);
    }
    
    //initial conditions
    if(frame < 1)
    {
        InitialConditions(m, v, P);
    }
    
    v = (length(v)>max_vel)?normalize(v)*max_vel:v;
    
    return vec4(encode(x), m, encode(v), rho);
}

//grid to particle
vec4 G2P(sampler2D a, sampler2D b, vec2 P)
{
    vec2 V = vec2(0.0);
    mat2 B = mat2(0.0);
    
    vec4 data = CH(a, P);
    float m = data.y;
    vec2 x = decode(data.x);
    float mass = 0.0;
    
    range(i, -1, 1) range(j, -1, 1)
    {
        //load data
        vec2 di = vec2(i,j);
        vec4 data0 = CH(a, P + di);
        
        //unpack data
        float m0 = data0.y;
        float rho = data0.w;
        vec2 x0 = decode(data0.x);
        vec2 dx = x - di;
        vec2 v0 = decode(data0.z);
        
        //find grid node contribution
        float w = k1(dx);
        
        //distribute velocities/forces to particles
        V += v0*w;
        vec2 nv0 =dot(v0, normalize(dx))*normalize(dx);
        //v0 = v0 - nv0 + clamp(nv0, -0.01, 0.01);//remove divergent component, makes it unstable otherwise
        B += mat2(v0*dx.x,v0*dx.y)*w;
    }
    
    //gravity
  //  V += vec2(0.0, -gravity)*dt;
    
    //push fluid
    V += vec2(0.0, 0.1)*exp(-0.02*pow(distance(P, R*vec2(0.6,0.2)), 2.));
    
    if(Mouse.z > 0.)
    {
        vec2 dx = (Mouse.xy - P); 
        V += 0.005*exp(-0.05*length(dx))*dt*dx; 
    }
    
    //border 
    vec3 N = bN(P + x);
    float vdotN = step(N.z, border_h)*dot(-N.xy, V);
    //V *= 1. - 0.1*exp(-N.z);
    V += (0. + 1.5*max(vdotN, 0.0))*N.xy*step(abs(N.z), border_h)*exp(-N.z);
   // V = (length(V)>max_vel)?normalize(V)*max_vel:V;
   
    //estimate density
    x += V*dt;
   
    range(i, -1, 1) range(j, -1, 1)
    {
        //load data
        vec2 di = vec2(i,j);
        vec4 data0 = CH(a, P + di);
        vec2 dx = x - di;
        //unpack data
        float m0 = data0.y;
        
        mass += m0*k1(dx);
    }
    const float lim = 0.01; //affine matrix clamp
    return vec4(encode(V), encode(clamp(B[0], -lim,lim)), encode(clamp(B[1], -lim,lim)), mass);
}


// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
//Advect mass distributions and distrubute the momentum and density to grid

//Advect + P2G

void mainImage( out vec4 C, in vec2 P )
{
    R = iResolution.xy; time = iTime; Mouse = iMouse; frame = iFrame;
    P = floor(P);
   
    C = P2G(iChannel0,iChannel1,P);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
//Distribute grid velocities to the mass distributions

//G2P

//technically its more correct to do the force in the P2G step 2
//but I didn't want to waste a buffer here

void mainImage( out vec4 C, in vec2 P )
{
    R = iResolution.xy; time = iTime; Mouse = iMouse; frame = iFrame;
    P = floor(P);
    C = G2P(iChannel0, iChannel1, P);
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
//Advect + P2G

void mainImage( out vec4 C, in vec2 P )
{
    R = iResolution.xy; time = iTime; Mouse = iMouse; frame = iFrame;
    P = floor(P);
   
    C = P2G(iChannel0,iChannel1,P);
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
//G2P

void mainImage( out vec4 C, in vec2 P )
{
    R = iResolution.xy; time = iTime; Mouse = iMouse; frame = iFrame;
    P = floor(P);
    C = G2P(iChannel0, iChannel1, P);
}