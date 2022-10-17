

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
vec3 hsv2rgb( in vec3 c )
{
    vec3 rgb = clamp( abs(mod(c.x*6.0+vec3(0.0,4.0,2.0),6.0)-3.0)-1.0, 0.0, 1.0 );

	rgb = rgb*rgb*(3.0-2.0*rgb); // cubic smoothing	

	return c.z * mix( vec3(1.0), rgb, c.y);
}

void mainImage( out vec4 fragColor, in vec2 pos )
{
	R = iResolution.xy; time = iTime;
    //pos = R*0.5 + pos*0.1;
    ivec2 p = ivec2(pos);
    
    //pressure
    vec4 P = textureLod(ch1, pos/R, 0.);
    
    //border render
    vec3 bord = smoothstep(border_h+1.,border_h-1.,border(pos))*vec3(1.);
    
    //particle render
    vec2 rho = vec2(0.);

    range(i, -1, 1) range(j, -1, 1)
    {
       vec2 dx = vec2(i,j);
       vec4 data = texel(ch0, pos + dx);
       particle P = getParticle(data, pos + dx);
       
        vec2 x0 = P.X; //update position
        //how much mass falls into this pixel
        rho += 1.*P.M*G((pos - x0)/0.75); 
    }
  	rho = 1.2*rho;
    
     vec4 D = pixel(ch2, pos);
    float ang = atan(D.x, D.y);
    float mag = 0. + 10.*length(D.xy)*rho.x;
    
    // Output to screen
    fragColor = vec4(1.6*vec3(0.2,0.4,1.)*rho.x + 1.*vec3(1.5,0.3,0.3)*rho.y*rho.x + bord + 0.*abs(P.x),0);
	fragColor.xyz = tanh(vec3(1.,1.1,1.3)*fragColor.xyz);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define Bf(p) p
#define Bi(p) ivec2(p)
#define texel(a, p) texelFetch(a, Bi(p), 0)
#define pixel(a, p) texture(a, (p)/R)
#define ch0 iChannel0
#define ch1 iChannel1
#define ch2 iChannel2
#define ch3 iChannel3

#define PI 3.14159265

#define loop(i,x) for(int i = 0; i < x; i++)
#define range(i,a,b) for(int i = a; i <= b; i++)

#define dt 1.5

#define border_h 5.
vec2 R;
vec4 Mouse;
float time;

float Pf(vec2 rho)
{
    //return 0.2*rho; //gas
    float GF = smoothstep(0.49, 0.5, 1. - rho.y);
    return mix(0.5*rho.x,0.04*rho.x*(rho.x/0.2 - 1. + 0.1*rho.y), GF); //water pressure
}

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
    float bound = -sdBox(p - R*0.5, R*vec2(0.5, 0.5)); 
    float box = 1e10 + sdBox(Rot(0.4*time)*(p - R*0.5) , R*vec2(0.005, 0.2));
    float drain = -sdBox(p - R*vec2(0.9, 0.05), vec2(0));
    return max(drain,min(bound, box));
}

#define h 1.
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
    return unpack(X); 
}

float encode(vec2 x)
{
    uint X = pack(x);
    return uintBitsToFloat(X); 
}

struct particle
{
    vec2 X;
    vec2 V;
    vec2 M;
};
    
particle getParticle(vec4 data, vec2 pos)
{
    particle P; 
    P.X = decode(data.x) + pos;
    P.V = decode(data.y);
    P.M = data.zw;
    return P;
}

vec4 saveParticle(particle P, vec2 pos)
{
    P.X = clamp(P.X - pos, vec2(-0.5), vec2(0.5));
    return vec4(encode(P.X), encode(P.V), P.M);
}

vec3 hash32(vec2 p)
{
	vec3 p3 = fract(vec3(p.xyx) * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yxz+33.33);
    return fract((p3.xxy+p3.yzz)*p3.zyx);
}

float G(vec2 x)
{
    return exp(-dot(x,x));
}

float G0(vec2 x)
{
    return exp(-length(x));
}

//diffusion amount
#define dif 1.1
vec3 distribution(vec2 x, vec2 p, float K)
{
    vec4 aabb0 = vec4(p - 0.5, p + 0.5);
    vec4 aabb1 = vec4(x - K*0.5, x + K*0.5);
    vec4 aabbX = vec4(max(aabb0.xy, aabb1.xy), min(aabb0.zw, aabb1.zw));
    vec2 center = 0.5*(aabbX.xy + aabbX.zw); //center of mass
    vec2 size = max(aabbX.zw - aabbX.xy, 0.); //only positive
    float m = size.x*size.y/(K*K); //relative amount
    //if any of the dimensions are 0 then the mass is 0
    return vec3(center, m);
}

//diffusion and advection basically
void Reintegration(sampler2D ch, inout particle P, vec2 pos)
{
    //basically integral over all updated neighbor distributions
    //that fall inside of this pixel
    //this makes the tracking conservative
    float Mi = 0.;
    range(i, -2, 2) range(j, -2, 2)
    {
        vec2 tpos = pos + vec2(i,j);
        vec4 data = texel(ch, tpos);
       
        particle P0 = getParticle(data, tpos);
       
        P0.X += P0.V*dt; //integrate position

        vec3 D = distribution(P0.X, pos, dif);
		vec3 D1 = distribution(P0.X, pos, 1.);
        //the deposited mass into this cell
        float m = P0.M.x*D.z;
        
        //add weighted by mass
        P.X += D.xy*m;
        P.V += P0.V*m;
        
        P.M.y += P0.M.y*P0.M.x*D1.z;
        
        //add mass
        P.M.x += m;
        Mi += P0.M.x*D1.z;
    }
    
    //normalization
    if(P.M.x != 0.)
    {
        P.X /= P.M.x;
        P.V /= P.M.x;
    }
    if(Mi != 0.)
    {
        P.M.y /= Mi;
    }
}

//force calculation and integration
void Simulation(sampler2D ch, inout particle P, vec2 pos)
{
    //Compute the SPH force
    vec2 F = vec2(0.);
    vec3 avgV = vec3(0.);
    range(i, -2, 2) range(j, -2, 2)
    {
        vec2 tpos = pos + vec2(i,j);
        vec4 data = texel(ch, tpos);
        particle P0 = getParticle(data, tpos);
        vec2 dx = P0.X - P.X;
        float avgP = 0.5*P0.M.x*(Pf(P.M) + Pf(P0.M)); 
        F -= 1.*G(1.*dx)*avgP*dx;
        avgV += P0.M.x*G(1.*dx)*vec3(P0.V,1.);
    }
    avgV.xy /= avgV.z;

    //viscosity
    F += 0.*P.M.x*(avgV.xy - P.V);
    
    //gravity
    F += P.M.x*vec2(0., -0.0005) + P.M.x*step(0.5, P.M.y)*vec2(0., 0.005);

    if(Mouse.z > 0.)
    {
        vec2 dm =(Mouse.xy - Mouse.zw)/10.; 
        float d = distance(Mouse.xy, P.X)/20.;
        F += 0.001*dm*exp(-d*d);
        P.M.y += 0.1*exp(-40.*d*d);
    }
    
    //integrate
    P.V += F*dt/P.M.x;

    //border 
    vec3 N = bN(P.X);
    float vdotN = step(N.z, border_h)*dot(-N.xy, P.V);
    P.V += 1.*(N.xy*vdotN + N.xy*abs(vdotN));
    P.V += P.M.x*N.xy*step(abs(N.z), border_h)*exp(-N.x);
    
        
    if(length(vec2(1., 1.)*(P.X - R*vec2(0.5, 0.1))) < 10.) P.M.y = mix(P.M.y, 1., 0.06);
    if(N.z < 2.*border_h) P.M.y *= 0.9;

    
    //velocity limit
    float v = length(P.V);
    P.V /= (v > 1.)?v:1.;
}



// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
#define mass 1.

void mainImage( out vec4 U, in vec2 pos )
{
    R = iResolution.xy; time = iTime; Mouse = iMouse;
    ivec2 p = ivec2(pos);
        
    //particle velocity, mass and grid distributed density
    vec2 F = vec2(0.);
    
    vec4 data = texel(ch0, pos); 
    
    particle P;// = getParticle(data, pos);
       
    Reintegration(ch0, P, pos);
   
    //initial condition
    if(iFrame < 1)
    {
        //random
        vec3 rand = hash32(pos);
        if(rand.z < 0.3) 
        {
            P.X = pos;
            P.V = 0.5*(rand.xy-0.5) + vec2(0., 0.);
            P.M = vec2(mass, 0.);
        }
        else
        {
            P.X = pos;
            P.V = vec2(0.);
            P.M = vec2(0.);
        }
    }
    
    U = saveParticle(P, pos);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
void mainImage( out vec4 U, in vec2 pos )
{
    R = iResolution.xy; time = iTime; Mouse = iMouse;
    ivec2 p = ivec2(pos);
        
    //particle velocity, mass and grid distributed density
    vec2 F = vec2(0.);
    
    vec4 data = texel(ch0, pos); 
    
    particle P = getParticle(data, pos);
    
    
    if(P.M.x != 0.) //not vacuum
    {
        Simulation(ch0, P, pos);
    }
    
    U = saveParticle(P, pos);
}