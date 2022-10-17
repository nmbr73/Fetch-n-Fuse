

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

vec3 mixN(vec3 a, vec3 b, float k)
{
    return sqrt(mix(a*a, b*b, clamp(k,0.,1.)));
}

vec4 V(vec2 p)
{
    return pixel(ch1, p);
}

void mainImage( out vec4 col, in vec2 pos )
{
	R = iResolution.xy; time = iTime;
    //pos = R*0.5 + pos*0.1;
    ivec2 p = ivec2(pos);
    
    vec4 data = texel(ch0, pos);
    particle P = getParticle(data, pos);
    
    //border render
    vec3 Nb = bN(P.X);
    float bord = smoothstep(2.*border_h,border_h*0.5,border(pos));
    
    vec4 rho = V(pos);
    rho.z *= 5.;
    //rho.w = tanh(rho.w);
    vec3 dx = vec3(-3., 0., 3.);
    vec4 grad = -0.5*vec4(V(pos + dx.zy).zw - V(pos + dx.xy).zw,
                         V(pos + dx.yz).zw - V(pos + dx.yx).zw);
    vec2 N = pow(length(grad.xz),0.2)*normalize(grad.xz+1e-5);
    float specular = pow(max(dot(N, Dir(1.4)), 0.), 3.5);
    float specularb = G(0.4*(Nb.zz - border_h))*pow(max(dot(Nb.xy, Dir(1.4)), 0.), 3.);
    
    float a = pow(smoothstep(fluid_rho*0.5, fluid_rho*2., rho.z),0.1);
   
    col.xyz += 0.02*vec3(0.2, 0.5, 1.)*rho.w;
    col.xyz += vec3(1., 0.7, 0.5)*rho.z;
    
    col.xyz = tanh(col.xyz);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define Bf(p) mod(p,R)
#define Bi(p) ivec2(mod(p,R))
#define texel(a, p) texelFetch(a, Bi(p), 0)
#define pixel(a, p) texture(a, (p)/R)
#define ch0 iChannel0
#define ch1 iChannel1
#define ch2 iChannel2
#define ch3 iChannel3

#define PI 3.14159265

#define loop(i,x) for(int i = 0; i < x; i++)
#define range(i,a,b) for(int i = a; i <= b; i++)

#define dt 1.

#define border_h 5.
vec2 R;
vec4 Mouse;
float time;

#define temporal_blurring 0.98

#define mass 1.

#define fluid_rho 0.5

float Pf(float rho)
{
    return 1.*rho*rho + 1.*pow(rho, 10.); //gas + supernova
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
    float box = sdBox(Rot(0.*time)*(p - R*vec2(0.5, 0.2)) , R*vec2(0.05, 0.01));
    float drain = -sdBox(p - R*vec2(0.5, 0.6), R*vec2(1.5, 2.));
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
    vec2 X; //
    vec2 V; //velocity
    float M; //mass
    float I; //angular velocity
};
    
particle getParticle(vec4 data, vec2 pos)
{
    particle P; 
    P.X = decode(data.x) + pos;
    P.V = decode(data.y);
    P.M = data.z;
    P.I = data.w;
    return P;
}

vec4 saveParticle(particle P, vec2 pos)
{
    P.X = clamp(P.X - pos, vec2(-0.5), vec2(0.5));
    return vec4(encode(P.X), encode(P.V), P.M, P.I);
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
#define dif 0.92
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
   
    //pass 1 - get center of mass
    range(i, -2, 2) range(j, -2, 2)
    {
        vec2 tpos = pos + vec2(i,j);
        vec4 data = texel(ch, tpos);
       
        particle P0 = getParticle(data, tpos);
       
        P0.X += P0.V*dt; //integrate position

        vec3 D = distribution(P0.X, pos, dif);
       
        //the deposited mass into this cell
        float m = P0.M*D.z;
        
        //add weighted by mass
        P.X += D.xy*m;
        
        //add mass
        P.M += m;
    }
    
    //normalization
    if(P.M != 0.)
    {
        P.X /= P.M;
    }
    
    //moment of inertia
    float I = 0.;
    //pass 2 - get velocity and angular momentum
    range(i, -1, 1) range(j, -1, 1)
    {
        vec2 tpos = pos + vec2(i,j);
        vec4 data = texel(ch, tpos);
       
        particle P0 = getParticle(data, tpos);
       
        P0.X += P0.V*dt; //integrate position

        vec3 D = distribution(P0.X, pos, dif);
       
        //the deposited mass into this cell
        float m = P0.M*D.z;
        
        vec2 dx = P0.X - P.X;
      
		float W = P0.I; 
        //relative velocity of this part of the square
        vec2 rel_V = P0.V + W*vec2(dx.y, -dx.x);
        float v = length(P.V);
    	rel_V /= (v > 2.)?v:1.;
        
        //add momentum
        P.V += rel_V*m;
        //add angular momentum
        P.I += (dx.x*P0.V.y - dx.y*P0.V.x)*m;
        //add moment of inertia
        I += dot(dx, dx)*m;
    }
   // I = max(I, 0.1);
    //normalization
    if(P.M != 0.)
    {
        P.V /= P.M; //get velocity
        P.I /= I;
    }
}

//force calculation and integration
void Simulation(sampler2D ch, sampler2D chG, inout particle P, vec2 pos)
{
    //Compute the forces
    vec2 F = vec2(0.);
    float w = 0.;
    vec3 avgV = vec3(0.);
    //local gravity potential
    float lU = pixel(chG, P.X).w;
    range(i, -2, 2) range(j, -2, 2)
    {
        vec2 tpos = pos + vec2(i,j);
        vec4 data = texel(ch, tpos);
        particle P0 = getParticle(data, tpos);
        vec2 dx = P0.X - P.X;
        float avgP = 0.5*P0.M*(Pf(P.M) + Pf(P0.M)); 
        //gas pressure
        F -= 0.5*G(1.*dx)*avgP*dx;
        
        //neighbor gravity potential
        float rU = pixel(chG, P0.X).w;
        
        //gas gravity
        F -= 0.0015*P.M*dx*clamp(lU - rU, -15., 15.)*G(1.*dx);
        avgV += G(1.*dx)*vec3(P0.V,1.);
    }
    
    avgV /= avgV.z;


    if(Mouse.z > 0.)
    {
        vec2 dm =(Mouse.xy - Mouse.zw)/10.; 
        float d = distance(Mouse.xy, P.X)/9.;
        F += 0.003*dm*exp(-d*d);
       // P.M.y += 0.1*exp(-40.*d*d);
    }
    
    //integrate
    P.V += F*dt/P.M;

    //border 
    vec3 N = bN(P.X);
    float vdotN = step(N.z, border_h)*dot(-N.xy, P.V);
    P.V += 0.5*(N.xy*vdotN + N.xy*abs(vdotN));
    P.V += 2.*P.M*N.xy*exp(-abs(N.z));
    if(N.z < 5.) 
    {
        //P.X = pos;
        P.V *= 0.;
       // P.M = 2.*fluid_rho;
    }
    //velocity limit
    float v = length(P.V);
    P.V /= (v > 1.)?v:1.;
    //angular momentum limit
    P.I = P.M*clamp(P.I/P.M, -0.5, 0.5);
}



// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<

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
        if(rand.z < 1.) 
        {
            vec2 dC = pos - R*0.5;
            P.X = pos;
            P.V = 0.*(rand.xy-0.5) + 0.5*vec2(dC.y/R.x, -dC.x/R.x);
            P.M = 0.005*mass;
            P.I = 0.;
        }
        else
        {
            P.X = pos;
            P.V = vec2(0.);
            P.M = 1e-6;
            P.I = 0.;
        }
    }
    
   
    
    U = saveParticle(P, pos);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
void mainImage( out vec4 U, in vec2 pos )
{
    R = iResolution.xy; time = iTime; Mouse = iMouse;
    ivec2 p = ivec2(pos);
    
    vec4 data = texel(ch0, pos); 
    
    particle P = getParticle(data, pos); 
    
   
    if(P.M != 0.) //not vacuum
    {
        Simulation(ch0, ch1, P, pos);
    }
    
   
	/*
    if(length(P.X - R*vec2(0.2, 0.9)) < 10.) 
    {
        P.X = pos;
        P.V = 0.5*Dir(-PI*0.25 + 0.3*sin(0.3*time));
        P.M = mix(P.M, vec2(fluid_rho, 0.), 0.4);
    }*/
    
    if(length(pos - R*vec2(0.5, 0.1)) < 10.) 
    {
      // P.I = 0.2;
    }
    
    U = saveParticle(P, pos);
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
//density
//gravity solved by using a Jacobi-like solver with future estimation
#define Rad 4

void mainImage( out vec4 fragColor, in vec2 pos )
{
    R = iResolution.xy; time = iTime;
    ivec2 p = ivec2(pos);

    vec4 data = texel(ch0, pos);
    particle P = getParticle(data, pos);
    
    //particle render
    vec4 rho = vec4(0.);
    float U = 0.;
    vec3 avgV = vec3(0.00, 0, 0.0000001);
    vec4 avgU = vec4(0.00, 0, 0, 0.00000001);
    vec4 dd = pixel(ch1, pos);
    range(i, -Rad, Rad) range(j, -Rad, Rad)
    {
        vec2 ij = vec2(i,j);
        vec4 data = texel(ch0, pos + ij);
        particle P0 = getParticle(data, pos + ij);

        vec2 x0 = P0.X; //update position
        //how much mass falls into this pixel
        vec2 dx = pos - x0;
        rho += 1.*vec4(P.V, P.M, P.I)*G(dx/1.5); 
       
        //local potential
        U += P0.M/(length(dx)+0.1);
        //local average velocity
        avgV += vec3(P0.V, 1.)*G(ij/1.)*P0.M; 
        
        //advected blurring, 
        //i.e. estimating where the past potential could have moved 
        vec3 pU = pixel(ch1, pos + ij - dd.xy*dt).xyw;
        avgU += vec4(pU, 1.)*G(ij/3.); //blurring field
    }
    
    //spacio-temporally blurred velocity
    rho.xy = mix(avgV.xy/avgV.z, avgU.xy/avgU.w, 0.95);
                  
    //spacio-temporally blurred gravitational potential
    rho.w = U + temporal_blurring*avgU.z/avgU.w;
                  
    fragColor = rho;
}