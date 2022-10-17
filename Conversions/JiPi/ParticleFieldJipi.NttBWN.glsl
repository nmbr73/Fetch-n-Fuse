

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Fork of "Paint streams" by michael0884. https://shadertoy.com/view/WtfyDj
// 2020-12-16 10:30:33

void mainImage( out vec4 col, in vec2 pos )
{
	R = iResolution.xy; time = iTime;
    ivec2 p = ivec2(pos);
    
    vec4 data = texel(ch0, pos);
    particle P = getParticle(data, pos);
    
    vec2 V = P.V*1.2;

    //vec3 cv = vec3(V*2.0, length(V)-0.5*length(V) ) * 0.5 + 0.5;
    vec3 cv = vec3(V*0.5+0.5, length(min(V*Rot(length(V)), 0.0))*0.5 );
    cv = mix(vec3(1.0), cv, length(V));
    //cv.z = 1.0;
    cv -= saturate(cv-1.0).yzx*1.0;
   
    col.xyz = cv;
    //col.xyz *= P.M.y*1.25*0.8;
    
    float d = distance(P.X, pos)/SCALE*2.0;
    d = max(abs(P.X.x - pos.x),
            abs(P.X.y - pos.y)) / SCALE * 2.0;
    d = pow(d, 0.25);
    //col.xyz *= 0.2+vec3(d);
  
    col.xyz *= pow(P.M.x*0.6, 0.75);
    col.xyz = pow(col.xyz, vec3(1.0/2.2)); 
    
    //col.xyz = 1.0 - col.xyz;
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define Bf(p) p
#define Bi(p) ivec2(p)
#define texel(a, p) texelFetch(a, Bi(p), 0)
#define texelLoop(a, p) texelFetch(a, Bi(mod(p,R)), 0)
#define pixel(a, p) texture(a, (p)/R)
#define textureLoop(a, p) texture(a, mod(p,R)/R)
#define ch0 iChannel0
#define ch1 iChannel1
#define ch2 iChannel2
#define ch3 iChannel3

#define PI 3.14159265

#define loop(i,x) for(int i = 0; i < x; i++)
#define range(i,a,b) for(int i = a; i <= b; i++)

#define saturate(x) clamp(x, 0.0, 1.0)
#define Gn(x) exp(-dot(x,x))
#define G0n(x) exp(-length(x))

#define dt 1.0

//------
//Params
//------

#define SCALE 4.25
#define DT_MULT  ((P0.M.x) + length(P0.V))

//0-1
#define DIFFUSION_CENTER 0.0
#define DC_MULT (1.0 - pow(length(P0.V), 0.25))

#define MASS_REST 0.6
#define MR_SPEED 0.05
#define MR_INCREASE abs(MASS_REST - P.M.x)*0.05

#define CENTERING_SPEED 0.1

#define GRAVITY_MULT 0.0;

//effects
#define COLL_AMNT 1.0
#define SMOKE_AMNT 0.25

#define FLUID_AMNT 0.25
#define fluid_rho 0.2
#define particle_rad 1.5





vec2 R;
vec4 Mouse;
float time;


float Pf(float den, float rest)
{
    //return 0.2*den;
    //return 0.04*(den-rest);
    return 0.05*den*(den/rest- 1.);
}

float W(vec2 r, float h)
{
	return (length(r) > 0.0 && h >= length(r)) ?
		(315.0 / (64.0 * PI * pow(h, 9.0))) * pow((pow(h, 2.0) - pow(length(r), 2.0)), 3.0) :
		0.0;
}

vec2 WS(vec2 r, float h)
{
	return (length(r) > 0.0 && h >= length(r)) ?
		-(45.0 / (PI * pow(h, 6.0))) * pow((h - length(r)), 2.0) * (r / length(r)) :
		vec2(0.0);
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
    float box = sdBox(Rot(1.0*time-0.0)*(p - R*vec2(0.5, 0.5)) , R*vec2(0.15, 0.01));
    float drain = -sdBox(p - R*vec2(0.5, 0.7), R*vec2(1.5, 0.5));
    return bound;
    return min(bound, box);
    return max(drain,min(bound, box));
}

#define hh 1.
vec3 bN(vec2 p)
{
    vec3 dx = vec3(-hh,0,hh);
    vec4 idx = vec4(-1./hh, 0., 1./hh, 0.25);
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
    
    //return unpackSnorm2x16(X);
    return unpackHalf2x16(X);
    //return unpack(X); 
}

float encode(vec2 x)
{
    //uint X = packSnorm2x16(x);
    uint X = packHalf2x16(x);
    //uint X = pack(x); 
   
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
    P.X = P.X - pos;
    return vec4(encode(P.X), encode(P.V), (P.M));
}

vec3 hash32(vec2 p)
{
	vec3 p3 = fract(vec3(p.xyx) * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yxz+33.33);
    return fract((p3.xxy+p3.yzz)*p3.zyx);
}

float G(vec2 x)
{
    //return pow(50.0, -dot(x,x));
    //return exp(-length(x)*length(x));
    return exp(-dot(x,x));
}

float G0(vec2 x)
{
    //return pow(2.0, -length(x));
    return exp(-length(x));
}


float GT(vec2 d, float h)
{
if (length(d)>h || length(d)<0.0)
    return 0.0;
    return pow(h - pow(length(d), 2.0), 3.0);
}

float GT0(vec2 d, float h)
{
if (length(d)>h || length(d)<0.0)
    return 0.0;
    return pow(h - length(d), 2.0);
}






// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<

vec3 distribution(vec2 x, vec2 p, float K)
{
    vec2 omin = clamp(x - K*0.5, p - 0.5, p + 0.5);
    vec2 omax = clamp(x + K*0.5, p - 0.5, p + 0.5); 
    return vec3(0.5*(omin + omax), (omax.x - omin.x)*(omax.y - omin.y)/(K*K));
}

//diffusion and advection basically
void Reintegration(sampler2D ch, inout particle P, vec2 pos)
{
    int I = 10; 
    range(i, -I, I) range(j, -I, I)
    {
        vec2 tpos = pos + vec2(i,j);
        vec4 data = texel(ch, tpos);
       
        particle P0 = getParticle(data, tpos);
        
        if (P0.M.y < -9999.0 || P.M.y < -9999.0)
            continue;
       
        P0.X += P0.V*dt * DT_MULT;

        //float difR = 2.75 + pow(length(P0.V), 4.5) * 0.5;
        float difR = SCALE;

        vec3 D = distribution(P0.X, pos, difR);
        float m = P0.M.x*D.z; //the deposited mass into this cell

        //add weighted by mass
        P.X += mix(P0.X, D.xy, DIFFUSION_CENTER * DC_MULT)*m;
        //P.X += D.xy*m;
        //P.X += P0.X*m;
        P.V += P0.V*m;
        P.M.y += P0.M.y*m;
        
        //add mass
        P.M.x += m;
    }
    
    //normalization
    if (P.M.x != 0.0)
    {
        P.X /= P.M.x;
        P.V /= P.M.x;
        P.M.y /= P.M.x;
    }
    
    //----- 
    float prevM = P.M.x;
    P.M.x = mix(P.M.x, MASS_REST, MR_SPEED + MR_INCREASE);
    P.V = P.V * prevM/P.M.x;
    
    P.M.y = mix(P.M.y, length(P.V),  0.1);
    
    vec2 CP = clamp(P.X - pos, -0.5, 0.5) + pos;
    //CP = pos;
    P.X = mix(P.X, CP, saturate(distance(P.X, pos) - 0.0) * CENTERING_SPEED);
    
    //border/solids
    if (border(pos) < 3.0)
    {
        P.X = pos;
        P.V = vec2(0.0);
        P.M.x = 90.05;
        P.M.y = -10000.5;
    }
}
void mainImage( out vec4 U, in vec2 pos )
{
    R = iResolution.xy; time = iTime; Mouse = iMouse;
    ivec2 p = ivec2(pos);

    vec4 data = texel(ch0, pos); 
    
    particle P;// = getParticle(data, pos);
       
    Reintegration(ch0, P, pos);
   
    if (iFrame < 1)
    {
        vec3 rand = hash32(pos);
        
        P.X = pos;
        P.V = vec2(0.0) + (0.5*(rand.xy-0.5))*0.5;
        P.M = vec2(1.0, 0.0);
    }
    
    U = saveParticle(P, pos);
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
//force calculation and integration
void Simulation(sampler2D ch, inout particle P, vec2 pos)
{
    vec2 F = vec2(0.);
    int I = 3;
    range(i, -I, I) range(j, -I, I)
    {
        vec2 tpos = pos + vec2(i,j);
        vec4 data = texel(ch, tpos);
        particle P0 = getParticle(data, tpos);
              
        vec2 dx = P0.X - P.X;
        float d = length(dx);         
        
        //border
        if (P0.M.y < -9999.9)
        {
            if (d < 3.0)
                F -= dx * (3.0 - d) * saturate(P.M.x-1.5) * 0.25;
           continue;
        }
        
        //collision-ish
        float L = 2.5;
        if (d < L)
            F -= dx* (L - d)
            * abs(length(P0.V)-length(P.V))
            * abs(saturate(P0.M.x-1.0)-saturate(P.M.x-1.0))
            * -dot(P.V, P0.V)
            * G0(dx*4.0/L)
            * COLL_AMNT;
            
        //smoke 
        if (d < L)
            F -= dx* (L - d)
            * saturate(P0.M.x-P.M.x)
            * (P0.M.x/P.M.x)
            * saturate(length(P0.V)-length(P.V))
            * G0(dx*4.0/L)
            * SMOKE_AMNT;
            
        //fluid
        L = particle_rad;
        if (d < L)
            F -= dx * (L - d)
            //* (Pf(P0.M.x, fluid_rho)+Pf(P.M.x, fluid_rho))
            * P0.M.x
            * G0(dx*4.0/L)
            * (Pf(saturate(P0.M.x-1.0), fluid_rho)+Pf(saturate(P.M.x-1.0), fluid_rho))
            //* abs(saturate(P0.M.x-1.0)-saturate(P.M.x-1.0))
            * FLUID_AMNT;    
    }
    
    F += vec2(0.0, -0.01)* P.M.x * length(P.V) * GRAVITY_MULT;
    
    if (iMouse.z > 0.)
    {
        vec2 dx = pos - iMouse.xy;
        if (length(dx) < 30.)
            F += 0.005*Rot(PI*0.0*iTime)*dx;// * (0.0 + length(dm)*0.25);
    }       
          
    //integrate
    P.V += F;
}


void mainImage( out vec4 U, in vec2 pos )
{
    R = iResolution.xy; time = iTime; Mouse = iMouse;
    ivec2 p = ivec2(pos);
        
    vec4 data = texel(ch0, pos); 
    
    particle P = getParticle(data, pos);

    
    if(P.M.x != 0.) //not vacuum
    {
        Simulation(ch0, P, pos);
    }
    
    //if (iTime < 5.0)
    {   
        //if(length(P.X - R*vec2(0.9, 0.85)) < 15.) 
        //    P.V += 0.125*Dir(-PI*0.25 - PI*0.5 + 0.3*sin(0.4*time));
        //if(length(P.X - R*vec2(0.1, 0.85)) < 15.) 
        //    P.V += 0.125*Dir(-PI*0.25 + 0.3*sin(0.3*time));
        
        vec2 dx = P.X - vec2((R.x - (float(iFrame)*2.2)), 0.5*R.y);
        if (length(dx - vec2(0.4, 0.0)) < 15.0)
            P.V += 0.5*Dir(-PI*0.0);         
        if (length(dx) < 30.0)
            P.V += 0.005*Rot(PI*0.0*iTime)*dx;
    }
    
    U = saveParticle(P, pos);
}