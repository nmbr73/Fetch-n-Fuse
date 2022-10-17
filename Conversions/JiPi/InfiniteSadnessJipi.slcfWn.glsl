

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Fork of "Paint streams" by michael0884. https://shadertoy.com/view/WtfyDj
// 2020-12-16 10:30:33

vec3 normals(vec2 pos, sampler2D sampler)
{
    float c = texel(sampler, pos).z;
    float l = texel(sampler, pos + vec2(-1.0, 0.0)).z;
    float r = texel(sampler, pos + vec2(1.0, 0.0)).z;
    float d = texel(sampler, pos + vec2(0.0, -1.0)).z;
    float u = texel(sampler, pos + vec2(0.0, 1.0)).z;
    
    
    vec3 va = normalize(vec3(vec2(2.0, 0.0), r-l));
    vec3 vb = normalize(vec3(vec2(0.0, 2.0), u-d));
    //return cross(va,vb);
    
    float ld = texel(sampler, pos + vec2(-1.0, -1.0)).z;
    float ru = texel(sampler, pos + vec2(1.0, 1.0)).z;
    float rd = texel(sampler, pos + vec2(1.0, -1.0)).z;
    float lu = texel(sampler, pos + vec2(-1.0, 1.0)).z;
    
    float me = (l+r+u+d+ld+ru+rd+ld)/8.0;
    me = l * 0.25
        +r * 0.25
        +d * 0.25
        +u * 0.25;

    //return normalize(vec3(l-r, d-u, c/c) * vec3(c, c, 1.0));
    return normalize(vec3(l-r, d-u, saturate(me-c)+c*0.3) * vec3(c, c, 1.0));
}

void mainImage( out vec4 col, in vec2 pos )
{
	R = iResolution.xy; time = iTime;
    ivec2 p = ivec2(pos);
    
    vec4 data = texel(ch0, pos);
    particle P = getParticle(data, pos);
    
    
    // Velocity Color
    vec2 V = P.V*0.2*Rot(2.7+iTime*0.1);

    vec3 vc = vec3(V, length(V)-0.5*length(V) ) * 0.5 + 0.5;
    vc = mix(vec3(1.0), vc, length(V));
    vc.z = 1.0;
    vc -= P.M.y*0.25;
    
    vc = vec3(1.0); //Here yumminess <-----
   
    
    // Mass Color
    vec3 mc = vc;
    mc *= pow(P.M.x*0.5, 0.75);
    mc = pow(mc, vec3(0.75)); 
    
    
    // Water Color
    vec3 NN = normals(pos, ch0);
    
    float d = dot(NN, normalize(vec3(-1.5, 1.5, 1.5)))*0.5+0.5;
    float s = pow(d, 50.0);
    float c = pow(s * 0.75, 1.0);
    
    float sd = dot(NN, normalize(vec3(1.5, -1.5, 10.5)));
    
    vec3 wc = mix(vec3(1.0), vc, saturate(P.M.x/1.5)); 
    wc *= vec3(pow(sd*0.5, 0.25));   
    wc += vec3(c*0.8 * P.M.x);  
    
    float test = dot(NN, normalize(vec3(1.5, -1.5, 2.5)))*0.5+0.5;
    test = pow(test, 70.0);
    wc += vec3(test*P.M.x);

    //===
    col.xyz = wc;
    //col.xyz = mc; //And here milkyness <-----
    
    col.xyz = pow(col.xyz, vec3(0.9));
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<


vec2 R;

#define Bf(p) p
#define Bi(p) ivec2(p)
//#define texel(a, p) texelFetch(a, Bi(p), 0)
#define texel(a, p) texture(a, (vec2(Bi(p))+0.5f)/R)

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

#define UP vec3(1.0, 0.0, 0.0)
#define RED UP

#define fluid_rho 1.5
#define particle_rad 1.5






vec4 Mouse;
float time;


float Pf(float den, float rest)
{
    return 0.4*(den-rest);
    //return 0.05*den*(den/rest- 1.);
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
    float box = sdBox(Rot(0.0*time-0.0)*(p - R*vec2(0.5, 0.5)) , R*vec2(0.15, 0.01));
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
    int I = 6; 
    range(i, -I, I) range(j, -I, I)
    {
        vec2 tpos = pos + vec2(i,j);
        vec4 data = texel(ch, tpos);
       
        particle P0 = getParticle(data, tpos);
        
        P0.X += P0.V*dt;// * (1.0 + (P0.M.x) * length(P0.V));

        float difR = 1.8 + (P0.M.x/fluid_rho);

        vec3 D = distribution(P0.X, pos, difR);
        float m = P0.M.x*D.z; //the deposited mass into this cell

        P.X += mix(P0.X, D.xy, saturate( pow(texel(ch, pos).z, 1.25)*1.5) )*m;  
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
    P.M.x = max(0.002, P.M.x);
    P.V = P.V * prevM/P.M.x;
      
    P.M.y = mix(0.0, P.M.x, 0.5);

    P.X = clamp(P.X - pos, -0.5, 0.5) + pos;
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
        P.M = vec2(rand.y*0.02, 0.0);
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
                      
        //fluid
        float L = particle_rad;
        if (d < L && d > 0.001)
            F -= normalize(dx) * (L - d)
            * P0.M.x
            * G0(dx*4.0/L)
            //* (Pf(P0.M.y, fluid_rho)+Pf(P.M.y, fluid_rho))
            * (Pf(P0.M.x, fluid_rho)+Pf(P.M.x, fluid_rho))
            * 0.5;   
         
        //viscosity
        F += G(dx*2.0/1.5) * (P0.V - P.V) * 0.8;
    }
    
    float l = length(P.V);
    float v = ( l< 1.0)
                ? pow(l, 1.0)
                : pow(l, 0.25);
                
    F += vec2(0.0, -0.04) * P.M.x * P.M.y * v;
    
    if (iMouse.z > 0.)
    {
        vec2 dx = pos - iMouse.xy;
        float d = length(dx);
        
        if (d < 30.0)
            F += dx * (30.0-d)
            * Rot(PI*0.0*iTime)
            * 0.002;
    }    
    
                 
    // Border
    vec2 pd = P.X + P.V*dt;
    if (pd.y - 5.0 < 0.0)
    {
        P.V.y *= 1.0 + ((pd.y - 5.0)/5.0);
        //P.V.x /= 1.0 + ((pd.y - 5.0)/5.0)*0.5 *(1.0+P.M.x);

        P.X += vec2(0.0, 1.0) * abs(pd.y - 5.0);
        P.V += vec2(0.0, 1.0) * abs(pd.y - 5.0)*0.1*P.M.x;
        P.M.x = max(P.M.x - 0.001, 0.0);
    }
    
    // Scale
    /*if (P.M.x > fluid_rho && distance(pos, P.X) != 0.0)
        F += normalize(pos - P.X) * max(P.M.x - fluid_rho, 0.0) * 0.05;*/
        
    //integrate
    P.V += F;
    
    P.V /= 1.0 + pow(length(P.V)*0.1, 3.0);
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
    
    if (iTime < 30.0)
    {   
        if(length(P.X - R*vec2(0.9, 0.85)) < 15.0 * (R.x/600.0))
        {
            P.X = pos;
            P.V = 0.5*Dir(-PI*0.25 - PI*0.5 + 0.3*sin(0.4*time));
            P.M.x = 1.75;
        }
        if(length(P.X - R*vec2(0.1, 0.85)) < 15.0 * (R.x/600.0)) 
        {
            P.X = pos;
            P.V = 0.5*Dir(-PI*0.25 + 0.3*sin(0.3*time));
            P.M.x = 1.75;
        }
    }

    U = saveParticle(P, pos);
}