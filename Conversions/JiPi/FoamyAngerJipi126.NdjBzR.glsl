

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<

float sminMe(float a, float b, float k, float p, out float t)
{
    float h = max(k - abs(a-b), 0.0)/k;
    float m = 0.5 * pow(h, p);
    t = (a < b) ? m : 1.0-m;
    return min(a, b) - (m*k/p);
}

float smin( in float a, in float b, in float k )
{
    float h = max(k-abs(a-b),0.0);
    float m = 0.25*h*h/k;
    return min(a,  b) - m;
}

void mainImage( out vec4 O, in vec2 pos )
{
	R = iResolution.xy; time = iTime;

    O.xyz = vec3(1.0);
    
    float d = 100.0;
    
    vec3 c = vec3(1.0);
    float m = 1.0;
    float v = 0.0;
    
    //rendering
    int I = int(ceil(particle_size*0.5))+2; 
    range(i, -I, I) range(j, -I, I)
    {
        vec2 tpos = pos + vec2(i,j);
        vec4 data = texel(ch0, tpos);
        particle P0 = getParticle(data, tpos);
        
        if (P0.M == 0.0) continue;
            
        float nd = distance(pos, P0.NX) - P0.R;

        float k = 4.0 / (1.0 + abs(m - P0.M)*1.5);
        float t;
        d = sminMe(d, nd, k, 3.0, t);
        m = mix(m, P0.M, t);
        v = mix(v, texel(ch1, tpos).w, t);
    }  
    
    //shadow
    float s = 100.0;
    vec2 off = vec2(10.0, 20.0);
    if (d > 0.0)
    {
        range(i, -I, I) range(j, -I, I)
        {
            vec2 tpos = pos-off + vec2(i,j);
            vec4 data = texel(ch0, tpos);
            particle P0 = getParticle(data, tpos);
            
            if (tpos.x < 0.0
            ||  tpos.x > R.x
            ||  tpos.y < 0.0
            ||  tpos.y > R.x) { s = 0.0; break; }
            if (P0.M == 0.0)  { continue; }

            float nd = distance(pos - off, P0.NX) - P0.R;
            if (texel(ch1, tpos).x > 1.0)
                s = smin(s, nd, 3.0);
        } 
    }
    
    //coloring and stuff
    if (d < 0.0)
        //d = 1.0-cos(d);
        d = sin(d);
    O.xyz *= vec3(abs(d));
    if (d < 0.0)
    {
        O.xyz *= c;
        O.xyz /= m*2.0;
        //col.xyz /= 0.5 + m*0.25;
        O.xyz -= vec3(v) / m * 0.06;
    }
    
    //checkerboard
    if (d > 1.0)
    {
        float size = 100.0;
        float cy = step(mod(pos.y, size), size*0.5);
        float ct = step(mod(pos.x + cy*size*0.5, size), size*0.5);
        
        ct = saturate(ct + 0.0);        
        //col.xyz = mix(vec3(ct), col.xyz, 1.0-saturate(d));
    }
    
    O.xyz = saturate(O.xyz);    
    if (d > 0.0)
        O.xyz *= mix(vec3(0.7), vec3(1.0), saturate(s));
        
    O.xyz = pow(O.xyz, vec3(0.7));
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
vec2 R;
vec4 Mouse;
float time;

#define Bf(p) p
#define Bi(p) ivec2(p)
//#define texel(a, p) texelFetch(a, Bi(p), 0)
#define texel(a, p) texture(a, (vec2(Bi(p))+0.5)/R)
#define pixel(a, p) texture(a, (p)/R)
#define ch0 iChannel0
#define ch1 iChannel1
#define ch2 iChannel2
#define ch3 iChannel3

#define PI 3.14159265

#define loop(i,x) for(int i = 0; i < x; i++)
#define range(i,a,b) for(int i = a; i <= b; i++)

#define snormalize(x) (length(x) > 0.0) ? normalize(x) : x
#define saturate(x) clamp(x, 0.0, 1.0)
#define G(x) exp(-dot(x*2.0,x*2.0))
#define GS(x) exp(-length(x*2.0))
#define dot2(x) dot(x,x)


#define UV (pos/R)





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
    float box = sdBox(Rot(0.*time-0.0)*(p - R*vec2(0.5, 0.6)) , R*vec2(0.05, 0.0125));
    float drain = -sdBox(p - R*vec2(0.5, 0.7), R*vec2(1.5, 0.5));
    //return bound - 10.0;
    return min(bound-10.0, box);
    return max(drain,min(bound-10.0, box));
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


//-------------
// Particle
//-------------
vec2 decode(float x)
{
    uint X = floatBitsToUint(x); 
    
    //return unpackSnorm2x16(X);
    return unpackHalf2x16(X);
}

float encode(vec2 x)
{
    //uint X = packSnorm2x16(x);
    uint X = packHalf2x16(x);

    return uintBitsToFloat(X); 
}

struct particle
{
    vec2 X;
    vec2 NX;
    float R;
    float M;
};
    
particle getParticle(vec4 data, vec2 pos)
{
    particle P; 
    P.X = decode(data.x) + pos;
    P.NX = decode(data.y) + pos;
    P.R = data.z;
    P.M = data.w;
    return P;
}

vec4 saveParticle(particle P, vec2 pos)
{
    P.X = P.X - pos;
    P.NX = P.NX - pos;
    return vec4(encode(P.X), encode(P.NX), (P.R), (P.M));
}

vec2 vel(particle P)
{
    return P.NX - P.X;
}

particle getParticleAt(sampler2D ch, vec2 pos)
{
    vec4 data = texel(ch, pos);
    return getParticle(data, pos);
}


//-------------
// RNG
//-------------
uvec4 s0; 

void rng_initialize(vec2 p, int frame)
{
    s0 = uvec4(p, uint(frame), uint(p.x) + uint(p.y));
}

// https://www.pcg-random.org/
void pcg4d(inout uvec4 v)
{
	v = v * 1664525u + 1013904223u;
    v.x += v.y*v.w; v.y += v.z*v.x; v.z += v.x*v.y; v.w += v.y*v.z;
    v = v ^ (v>>16u);
    v.x += v.y*v.w; v.y += v.z*v.x; v.z += v.x*v.y; v.w += v.y*v.z;
}

float rand() { pcg4d(s0); return float(s0.x)/float(0xffffffffu);  }
vec2 rand2() { pcg4d(s0); return vec2(s0.xy)/float(0xffffffffu);  }
vec3 rand3() { pcg4d(s0); return vec3(s0.xyz)/float(0xffffffffu); }
vec4 rand4() { pcg4d(s0); return vec4(s0)/float(0xffffffffu);     }



//-------------
// Kernels
//-------------

#define pixel_scale 4.0
float W(vec2 r, float h)
{
	return (length(r/pixel_scale) >= 0.0 && h >= length(r/pixel_scale)) ? //>=
        ( 4.0 / (PI * pow(h, 8.0))) * pow(h*h - dot2(r/pixel_scale), 3.0 ) :
		0.0;
}
vec2 WS(vec2 r, float h)
{
	return (length(r/pixel_scale) > 0.0 && h >= length(r/pixel_scale)) ?
        -(30.0 / (PI * pow(h, 5.0)))  * pow(h - length(r/pixel_scale), 2.0) * normalize(r) :
		vec2(0.0);
}
float WV(vec2 r, float h)
{
	return (length(r/pixel_scale) > 0.0 && h >= length(r/pixel_scale)) ?
        (20.0 / (PI * pow(h, 5.0)))  * (h - length(r/pixel_scale)) :
		0.0;
}

float WTest(vec2 r, float h)
{
	return (length(r) >= 0.0 && h >= length(r)) ? //>=
        ( 4.0 / (PI * pow(h, 5.7))) * pow(h*h - dot2(r), 3.0 ) :
		0.0;
}

float WC(vec2 r, float h)
{
    r /= pixel_scale;
    float a = 32.0 / (PI * pow(h, 9.0));
    
	if (length(r)*2.0 > h && length(r) <= h)
        return a * ( pow(h - length(r), 3.0) * pow(length(r), 3.0) );
    if (length(r) > 0.0 && length(r)*2.0 <= h)
        return a * ( pow(h - length(r), 3.0) * 2.0 - (pow(h, 6.0)/64.0) );
	return 0.0;
}

float WA(vec2 r, float h)
{
    r /= pixel_scale;
    float x = length(r);
    if (x*2.0 > h && x <= h)
        return pow(-(4.0*x*x/h) + 6.0*x - 2.0*h, 1.0/4.0) * 0.007/pow(h, 3.25);
    return 0.0;
}


//-------------
// Border
//-------------
bool imBorder(vec2 pos)
{
    return border(pos) < 0.0
        && mod(pos.x, 1.8) < 1.0
        && mod(pos.y, 1.8) < 1.0;
}

particle getVP(vec2 pos)
{
    particle P;
    
    P.X = pos;
    P.NX = pos;
    P.M = 1.25;
    P.R = 1.8 * 0.5; 
    return P;
}


//-------------
// Sim
//-------------
#define dt 1.0

#define particle_size 1.6
#define relax_value 1.0 / 2.0

#define fluid_rho 2.5
#define particle_rad 1.0

void Simulation(sampler2D ch, sampler2D chd, inout particle P, vec2 pos)
{
    vec2 F = vec2(0.0);
    vec3 n = vec3(0.0);
    
    vec4 pr = texel(chd, pos);
    
    //int I = int(ceil(particle_size))+2; 
    int I = int(ceil(particle_rad*pixel_scale));
    range(i, -I, I) range(j, -I, I)
    {
        if (i == 0 && j == 0) continue;
        
        vec2 tpos = pos + vec2(i,j);
        vec4 data = texel(ch, tpos);       
        particle P0 = getParticle(data, tpos);
        
        if (P0.M == 0.0 || tpos.x < 0.0 || tpos.y < 0.0) continue;
        if (length(P0.NX - P.NX) > particle_rad*pixel_scale) continue;
        
        vec2 dx = P.NX - P0.NX;
        float d = length(dx);
        float r = P.R + P0.R;
        float m = (((P.M-P0.M)/(P.M+P0.M)) + ((2.0*P0.M)/(P.M+P0.M)));
        //m = P0.M / (P.M + P0.M);
        
        float rho = (P.M < 1.0) ? fluid_rho*0.5 : fluid_rho*P.M;
    
        vec4 pr0 = texel(chd, tpos);       
        float pf = (pr.z+pr0.z);
        
        //collision
        F += normalize(dx) * max(r - d, 0.0) * m;
        //fluid
        F -= WS(dx, particle_rad) * pf / rho * P0.M;
           
        //cohesion
        //vec2 co = 0.2 * WC(dx, particle_rad*2.0) * normalize(dx);
        //F -= ((fluid_rho*2.0)/(pr.x+pr0.x))*co;
        
        //adhesion
        //if (imBorder(tpos))
            //F -= 1.0 * WA(dx, particle_rad) * normalize(dx) * P0.M;
        
        //curl
        n -= vec3(WS(dx, particle_rad) * abs(pr0.w) * P0.M, 0.0);
        
        //viscosity
        //F -= 0.01 * WTest(dx, 4.0) * (vel(P) - vel(P0)) * P0.M * (imBorder(tpos) ? 100.0*0.0 : 0.5);
    }
 
    if (length(n) > 0.0 && pr.z > 0.0)
       F += cross(normalize(n), vec3(0.0, 0.0, pr.w)).xy * 0.1 * pr.z;
    
    //border
    vec2 dp = P.NX;
    float d = border(dp) - P.R;
    if (d < 0.0)
        F -= bN(dp).xy * d;
      
    P.NX += F * relax_value;
}





// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<

void Integrate(sampler2D ch, inout particle P, vec2 pos)
{
    int I = 2; 
    range(i, -I, I) range(j, -I, I)
    {
        vec2 tpos = pos + vec2(i,j);
        vec4 data = texel(ch, tpos);
        
        if (tpos.x < 0.0 || tpos.y < 0.0) continue;
       
        particle P0 = getParticle(data, tpos);

        //falls in this pixel
        if (P0.NX.x >= pos.x-0.5
        &&  P0.NX.x < pos.x+0.5
        &&  P0.NX.y >= pos.y-0.5
        &&  P0.NX.y < pos.y+0.5
        &&  P0.M > 0.0)
        {
            vec2 P0V = vel(P0)/dt;
        
            //external forces
            if(iMouse.z > 0.0)
            {
                vec2 dm =(iMouse.xy - iMouse.zw)/10.; 
                float d = distance(iMouse.xy, P0.NX)/20.;
                P0V += 0.005*dm*exp(-d*d) * 1.0;
            }

            P0V += vec2(0., -0.005) * ((P0.M < 0.95) ? 0.05 : 1.0);//*P0.M;
            //P0V -= normalize(P0.NX - iResolution.xy*0.5) * 0.005 * ((P0.M < 0.95) ? 0.05 : 1.0);

            float v = length(P0V);
            P0V /= (v > 1.0) ? v : 1.0;

            //
            P0.X = P0.NX;     
            P0.NX = P0.NX + P0V*dt;
            P = P0;
            break;
        }
    }
}

int emitTime(float area, float pc)
{
    float ppf = area/particle_size;
    return int(((R.x*R.y) / ppf) * pc);
}

void mainImage( out vec4 O, in vec2 pos )
{
    R = iResolution.xy;
    rng_initialize(pos, iFrame);

    particle P;    

    Integrate(ch0, P, pos);
    
    if (imBorder(pos)) P = getVP(pos);

    //liquid emitter
    if (P.M == 0.0 && pos.x > 10.0 && pos.x < 11.0 && UV.y > 0.6 && UV.y < 0.75
    && mod(pos.y, particle_size*2.0) < 1.0 && rand() > 0.5 && iFrame < emitTime(R.x*0.15*0.5, 0.18) && true)
    {
        P.X = pos;
        P.NX = pos + vec2(1.0, 0.0);
        P.M = 1.0;
        P.R = particle_size * 0.5;
    }
    //gas emitter
    if (P.M == 0.0 && pos.x > R.x - 11.0 && pos.x < R.x - 10.0 && UV.y > 0.6 && UV.y < 0.75
    && mod(pos.y, particle_size*2.0) < 1.0 && rand() > 0.25 && iFrame < emitTime(R.x*0.15*0.75, 0.4) && true)
    {
        P.X = pos;
        P.NX = pos - vec2(0.5, 0.0);
        P.M = 0.5;// + sin(iTime)*0.05;
        P.R = particle_size * 0.5;
    }
    //dense liquid emitter
    if (P.M == 0.0 && pos.x > R.x - 11.0 && pos.x < R.x - 10.0 && UV.y > 0.2 && UV.y < 0.3
    && mod(pos.y, particle_size*2.0) < 1.0 && rand() > 0.25 && iFrame < emitTime(R.x*0.15*0.75, 0.05) && true)
    {
        P.X = pos;
        P.NX = pos - vec2(0.5, 0.0);
        P.M = 2.5;
        P.R = particle_size * 0.5;
    }
   
    O = saveParticle(P, pos);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
vec3 distribution(vec2 x, vec2 p, vec2 K)
{
    vec2 omin = clamp(x - K*0.5, p - 0.5*K, p + 0.5*K);
    vec2 omax = clamp(x + K*0.5, p - 0.5*K, p + 0.5*K); 
    return vec3(0.5*(omin + omax), (omax.x - omin.x)*(omax.y - omin.y)/(K.x*K.y));
}

vec4 FluidData(particle P, vec2 pos)
{
    float den = 0.0;
    vec3 curl = vec3(0.0);
    
    vec2 gradki = vec2(0.0);
    float gradl = 0.0;
    
    float rho = (P.M < 1.0) ? fluid_rho*0.5 : fluid_rho*P.M;
    
    int I = int(ceil(particle_rad*pixel_scale)); 
    range(i, -I, I) range(j, -I, I)
    {
        vec2 tpos = pos + vec2(i,j);
        vec4 data = texel(ch0, tpos);
        particle P0 = getParticle(data, tpos);
        
        if (P0.M == 0.0) continue;
 
        //density
        den += W(P.NX - P0.NX, particle_rad) * P0.M;
        //den += distribution(P.NX, P0.NX, vec2(particle_rad*pixel_scale)).z * P0.M;
        
        //gradient
        vec2 g = WS(P.NX - P0.NX, particle_rad) * P0.M / rho;// * particle_rad*pixel_scale;
        gradki += g;
        gradl += dot2(g);

        //curl
        if (i == 0 && j == 0) continue;
        vec2 u = (vel(P) - vel(P0)) * P0.M;
		vec2 v = WS(P.NX - P0.NX, particle_rad);  
        //vec2 v = W(P.NX - P0.NX, particle_rad) * normalize(P.NX - P0.NX); 
		curl += cross(vec3(u, 0.0), vec3(v, 0.0));    
    }
    gradl += dot2(gradki);

    //pressure
    float Y = 3.0;
    float C = 0.08;
    float pr = ((fluid_rho*C)/Y) * (pow(den/rho, Y) - 1.0);
    
    //pr = den/fluid_rho - 1.0;
        
    //some hardcoded stuff
    //gas
    if (P.M < 1.0)
        pr = den*0.02;
    //pr = 0.02*(den-rho);
    
    //pr = max(pr, 0.0);
    if (pr < 0.0)
        pr *= 0.1;
        

    float l = pr / (gradl + 0.01);
  
    return vec4(den, pr, l, curl.z);
}

void mainImage( out vec4 O, in vec2 pos )
{
    R = iResolution.xy; time = iTime;
    ivec2 p = ivec2(pos);

    vec4 data = texel(ch0, pos);
    particle P = getParticle(data, pos);
   
    if (P.M > 0.0)   
        O = FluidData(P, pos);
}

// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<


void mainImage( out vec4 O, in vec2 pos )
{
    R = iResolution.xy; time = iTime; Mouse = iMouse;     
    vec4 data = texel(ch0, pos); 
    particle P = getParticle(data, pos);
      
    if (P.M > 0.0 && !imBorder(P.NX))
        Simulation(ch0, ch1, P, pos);
 
    O = saveParticle(P, pos);
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<


void mainImage( out vec4 O, in vec2 pos )
{
    R = iResolution.xy; time = iTime; Mouse = iMouse;     
    vec4 data = texel(ch0, pos); 
    particle P = getParticle(data, pos);
      
    if (P.M > 0.0 && !imBorder(P.NX))
        Simulation(ch0, ch1, P, pos);
 
    O = saveParticle(P, pos);
}