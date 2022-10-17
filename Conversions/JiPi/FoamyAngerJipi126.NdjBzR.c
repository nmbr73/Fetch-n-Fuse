
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

//vec2 R;
//float4 Mouse;
//float time;

#define Bf(p) p
#define Bi(p) to_int2(p)
//#define texel(a, p) texelFetch(a, Bi(p), 0)
#define texel(a, p) texture(a, (to_float2(Bi(p))+0.5f)/R)
#define pixel(a, p) texture(a, (p)/R)
#define ch0 iChannel0
#define ch1 iChannel1
#define ch2 iChannel2
#define ch3 iChannel3

#define PI 3.14159265f

#define loop(i,x) for(int i = 0; i < x; i++)
#define range(i,a,b) for(int i = a; i <= b; i++)

#define snormalize(x) (length(x) > 0.0f) ? normalize(x) : x
#define _saturatef(x) clamp(x, 0.0f, 1.0f)
#define G(x) _expf(-dot(x*2.0f,x*2.0f))
#define GS(x) _expf(-length(x*2.0f))
#define dot2(x) dot(x,x)


#define UV (pos/R)





__DEVICE__ mat2 Rot(float ang)
{
    return to_mat2(_cosf(ang), -_sinf(ang), _sinf(ang), _cosf(ang)); 
}

__DEVICE__ float2 Dir(float ang)
{
    return to_float2(_cosf(ang), _sinf(ang));
}


__DEVICE__ float sdBox( in float2 p, in float2 b )
{
    float2 d = abs_f2(p)-b;
    return length(_fmaxf(d,to_float2_s(0.0f))) + _fminf(max(d.x,d.y),0.0f);
}

__DEVICE__ float border(float2 p,float time)
{
    float bound = -sdBox(p - R*0.5f, R*to_float2(0.5f, 0.5f)); 
    float box = sdBox(Rot(0.0f*time-0.0f)*(p - R*to_float2(0.5f, 0.6f)) , R*to_float2(0.05f, 0.0125f));
    float drain = -sdBox(p - R*to_float2(0.5f, 0.7f), R*to_float2(1.5f, 0.5f));
    //return bound - 10.0f;
    return _fminf(bound-10.0f, box);
    return _fmaxf(drain,_fminf(bound-10.0f, box));
}

#define hh 1.0f
__DEVICE__ float3 bN(float2 p,float time)
{
    float3 dx = to_float3(-hh,0,hh);
    float4 idx = to_float4(-1.0f/hh, 0.0f, 1.0f/hh, 0.25f);
    float3 r = swi3(idx,z,y,w)*border(p + swi2(dx,z,y),time)
             + swi3(idx,x,y,w)*border(p + swi2(dx,x,y),time)
             + swi3(idx,y,z,w)*border(p + swi2(dx,y,z),time)
             + swi3(idx,y,x,w)*border(p + swi2(dx,y,x),time);
    return to_float3_aw(normalize(swi2(r,x,y)), r.z + 1e-4);
}


//-------------
// Particle
//-------------

union Zahl
 {
   float  _Float; //32bit float
   uint   _Uint;  //32bit unsigend integer
 };

__DEVICE__ float2 decode(float _x)
{
	Zahl z;
    //uint X = floatBitsToUint(x);
	z._Float = _x;
	
    return unpack(z._Uint)*3.0f; 
}

__DEVICE__ float encode(float2 _x)
{
	Zahl z;
    uint X = pack(_x/3.0f);
	
	z._Uint = X;
    //return uintBitsToFloat(X); 
	return (z._Float);
}


__DEVICE__ float2 decode(float x)
{
    uint X = floatBitsToUint(x); 
    
    //return unpackSnorm2x16(X);
    return unpackHalf2x16(X);
}

__DEVICE__ float encode(float2 x)
{
    //uint X = packSnorm2x16(x);
    uint X = packHalf2x16(x);

    return uintBitsToFloat(X); 
}

struct particle
{
    float2 X;
    float2 NX;
    float R;
    float M;
};
    
__DEVICE__ particle getParticle(float4 data, float2 pos)
{
    particle P; 
    P.X = decode(data.x) + pos;
    P.NX = decode(data.y) + pos;
    P.R = data.z;
    P.M = data.w;
    return P;
}

__DEVICE__ float4 saveParticle(particle P, float2 pos)
{
    P.X = P.X - pos;
    P.NX = P.NX - pos;
    return to_float4(encode(P.X), encode(P.NX), (P.R), (P.M));
}

__DEVICE__ float2 vel(particle P)
{
    return P.NX - P.X;
}

__DEVICE__ particle getParticleAt(sampler2D ch, float2 pos)
{
    float4 data = texel(ch, pos);
    return getParticle(data, pos);
}


//-------------
// RNG
//-------------
//uvec4 s0; 

__DEVICE__ uint4 rng_initialize(float2 p, int frame)
{
    //s0 = to_uint4(p.x,p.y, uint(frame), uint(p.x) + uint(p.y));
    return to_uint4(p.x,p.y, uint(frame), uint(p.x) + uint(p.y));
}

// https://www.pcg-random.org/
__DEVICE__ uint4 pcg4d(uint4 v)
{
  v = v * 1664525u + 1013904223u;
  v.x += v.y*v.w; v.y += v.z*v.x; v.z += v.x*v.y; v.w += v.y*v.z;
  v = v ^ (v>>16u);
  v.x += v.y*v.w; v.y += v.z*v.x; v.z += v.x*v.y; v.w += v.y*v.z;
  return v;
}

__DEVICE__ float  rand(uint4 *s0)  { *s0=pcg4d(s0); return (float)((*s0).x)/(float)(0xffffffffu);  }



//-------------
// Kernels
//-------------

#define pixel_scale 4.0
__DEVICE__ float W(float2 r, float h)
{
  return (length(r/pixel_scale) >= 0.0f && h >= length(r/pixel_scale)) ? //>=
        ( 4.0f / (PI * _powf(h, 8.0f))) * _powf(h*h - dot2(r/pixel_scale), 3.0f ) : 0.0f;
}
__DEVICE__ float2 WS(float2 r, float h)
{
  return (length(r/pixel_scale) > 0.0f && h >= length(r/pixel_scale)) ?
        -(30.0f / (PI * _powf(h, 5.0f)))  * _powf(h - length(r/pixel_scale), 2.0f) * normalize(r) : to_float2_s(0.0f);
}
__DEVICE__ float WV(float2 r, float h)
{
  return (length(r/pixel_scale) > 0.0f && h >= length(r/pixel_scale)) ?
        (20.0f / (PI * _powf(h, 5.0f)))  * (h - length(r/pixel_scale)) : 0.0f;
}

__DEVICE__ float WTest(float2 r, float h)
{
  return (length(r) >= 0.0f && h >= length(r)) ? //>=
        ( 4.0f / (PI * _powf(h, 5.7f))) * _powf(h*h - dot2(r), 3.0f ) : 0.0f;
}

__DEVICE__ float WC(float2 r, float h)
{
  r /= pixel_scale;
  float a = 32.0f / (PI * _powf(h, 9.0f));
    
  if (length(r)*2.0f > h && length(r) <= h)
        return a * ( _powf(h - length(r), 3.0f) * _powf(length(r), 3.0f) );
    if (length(r) > 0.0f && length(r)*2.0f <= h)
        return a * ( _powf(h - length(r), 3.0f) * 2.0f - (_powf(h, 6.0f)/64.0f) );
  return 0.0f;
}

__DEVICE__ float WA(float2 r, float h)
{
    r /= pixel_scale;
    float x = length(r);
    if (x*2.0f > h && x <= h)
        return _powf(-(4.0f*x*x/h) + 6.0f*x - 2.0f*h, 1.0f/4.0f) * 0.007f/_powf(h, 3.25f);
    return 0.0f;
}


//-------------
// Border
//-------------
__DEVICE__ bool imBorder(float2 pos,float time)
{
    return border(pos,time) < 0.0
        && mod_f(pos.x, 1.8f) < 1.0
        && mod_f(pos.y, 1.8f) < 1.0f;
}

__DEVICE__ particle getVP(float2 pos)
{
    particle P;
    
    P.X = pos;
    P.NX = pos;
    P.M = 1.25f;
    P.R = 1.8f * 0.5f; 
    return P;
}


//-------------
// Sim
//-------------
#define dt 1.0f

#define particle_size 1.6f
#define relax_value 1.0f / 2.0f

#define fluid_rho 2.5f
#define particle_rad 1.0f

__DEVICE__ particle Simulation(sampler2D ch, sampler2D chd, particle P, float2 pos, float time)
{
    float2 F = to_float2_s(0.0f);
    float3 n = to_float3_s(0.0f);
    
    float4 pr = texel(chd, pos);
    
    //int I = int(_ceil(particle_size))+2; 
    int I = (int)(_ceil(particle_rad*pixel_scale));
    range(i, -I, I) range(j, -I, I)
    {
        if (i == 0 && j == 0) continue;
        
        float2 tpos = pos + to_float2(i,j);
        float4 data = texel(ch, tpos);       
        particle P0 = getParticle(data, tpos);
        
        if (P0.M == 0.0f || tpos.x < 0.0f || tpos.y < 0.0f) continue;
        if (length(P0.NX - P.NX) > particle_rad*pixel_scale) continue;
        
        float2 dx = P.NX - P0.NX;
        float d = length(dx);
        float r = P.R + P0.R;
        float m = (((P.M-P0.M)/(P.M+P0.M)) + ((2.0f*P0.M)/(P.M+P0.M)));
        //m = P0.M / (P.M + P0.M);
        
        float rho = (P.M < 1.0f) ? fluid_rho*0.5f : fluid_rho*P.M;
    
        float4 pr0 = texel(chd, tpos);       
        float pf = (pr.z+pr0.z);
        
        //collision
        F += normalize(dx) * _fmaxf(r - d, 0.0f) * m;
        //fluid
        F -= WS(dx, particle_rad) * pf / rho * P0.M;
           
        //cohesion
        //vec2 co = 0.2f * WC(dx, particle_rad*2.0f) * normalize(dx);
        //F -= ((fluid_rho*2.0f)/(pr.x+pr0.x))*co;
        
        //adhesion
        //if (imBorder(tpos,time))
        //F -= 1.0f * WA(dx, particle_rad) * normalize(dx) * P0.M;
        
        //curl
        n -= to_float3_aw(WS(dx, particle_rad) * _fabs(pr0.w) * P0.M, 0.0f);
        
        //viscosity
        //F -= 0.01f * WTest(dx, 4.0f) * (vel(P) - vel(P0)) * P0.M * (imBorder(tpos,time) ? 100.0f*0.0f : 0.5f);
    }
 
    if (length(n) > 0.0f && pr.z > 0.0f)
       F += cross(normalize(n), to_float3(0.0f, 0.0f, pr.w)).xy * 0.1f * pr.z;
    
    //border
    float2 dp = P.NX;
    float d = border(dp,time) - P.R;
    if (d < 0.0f)
        F -= swi2(bN(dp,time),x,y) * d;
      
    P.NX += F * relax_value;
    
    return P;
}





// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer D' to iChannel0



__DEVICE__ particle Integrate(sampler2D ch, particle P, float2 pos, float4 iMouse)
{
    int I = 2; 
    range(i, -I, I) range(j, -I, I)
    {
        float2 tpos = pos + to_float2(i,j);
        float4 data = texel(ch, tpos);
        
        if (tpos.x < 0.0f || tpos.y < 0.0f) continue;
       
        particle P0 = getParticle(data, tpos);

        //falls in this pixel
        if (P0.NX.x >= pos.x-0.5
        &&  P0.NX.x < pos.x+0.5
        &&  P0.NX.y >= pos.y-0.5
        &&  P0.NX.y < pos.y+0.5
        &&  P0.M > 0.0f)
        {
            float2 P0V = vel(P0)/dt;
        
            //external forces
            if(iMouse.z > 0.0f)
            {
                float2 dm =(swi2(iMouse,x,y) - swi2(iMouse,z,w))/10.0f; 
                float d = distance_f2(swi2(iMouse,x,y), P0.NX)/20.0f;
                P0V += 0.005f*dm*_expf(-d*d) * 1.0f;
            }

            P0V += to_float2(0.0f, -0.005f) * ((P0.M < 0.95f) ? 0.05f : 1.0f);//*P0.M;
            //P0V -= normalize(P0.NX - iResolution*0.5f) * 0.005f * ((P0.M < 0.95f) ? 0.05f : 1.0f);

            float v = length(P0V);
            P0V /= (v > 1.0f) ? v : 1.0f;

            //
            P0.X = P0.NX;     
            P0.NX = P0.NX + P0V*dt;
            P = P0;
            break;
        }
    }
    
    return P;
}

__DEVICE__ int emitTime(float area, float pc, float2 R)
{
    float ppf = area/particle_size;
    return int(((R.x*R.y) / ppf) * pc);
}

__KERNEL__ void FoamyAngerJipi126Fuse__Buffer_A(float4 O, float2 pos, float iTime, float2 iResolution, float4 iMouse, int iFrame)
{

    pos+=0.5f;
    
        
    uint4 s0 = rng_initialize(pos, iFrame);

    particle P = { to_float2(0.0f,0.0),
                   to_float2(0.0f,0.0),
                   float 0.0f,
                   float 0.0f};    

    P = Integrate(ch0, P, pos);
    
    if (imBorder(pos,time)) P = getVP(pos);

    //liquid emitter
    if (P.M == 0.0f && pos.x > 10.0f && pos.x < 11.0f && UV.y > 0.6f && UV.y < 0.75
    && mod_f(pos.y, particle_size*2.0f) < 1.0f && rand(&s0) > 0.5f && iFrame < emitTime(R.x*0.15f*0.5f, 0.18f,R) && true)
    {
        P.X = pos;
        P.NX = pos + to_float2(1.0f, 0.0f);
        P.M = 1.0f;
        P.R = particle_size * 0.5f;
    }
    //gas emitter
    if (P.M == 0.0f && pos.x > R.x - 11.0f && pos.x < R.x - 10.0f && UV.y > 0.6f && UV.y < 0.75
    && mod_f(pos.y, particle_size*2.0f) < 1.0f && rand(&s0) > 0.25f && iFrame < emitTime(R.x*0.15f*0.75f, 0.4f,R) && true)
    {
        P.X = pos;
        P.NX = pos - to_float2(0.5f, 0.0f);
        P.M = 0.5f;// + _sinf(iTime)*0.05f;
        P.R = particle_size * 0.5f;
    }
    //dense liquid emitter
    if (P.M == 0.0f && pos.x > R.x - 11.0f && pos.x < R.x - 10.0f && UV.y > 0.2f && UV.y < 0.3
    && mod_f(pos.y, particle_size*2.0f) < 1.0f && rand(&s0) > 0.25f && iFrame < emitTime(R.x*0.15f*0.75f, 0.05f,R) && true)
    {
        P.X = pos;
        P.NX = pos - to_float2(0.5f, 0.0f);
        P.M = 2.5f;
        P.R = particle_size * 0.5f;
    }
   
    O = saveParticle(P, pos);


  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


vec3 distribution(float2 x, float2 p, float2 K)
{
    float2 omin = clamp(x - K*0.5f, p - 0.5f*K, p + 0.5f*K);
    float2 omax = clamp(x + K*0.5f, p - 0.5f*K, p + 0.5f*K); 
    return to_float3_aw(0.5f*(omin + omax), (omax.x - omin.x)*(omax.y - omin.y)/(K.x*K.y));
}

__DEVICE__ float4 FluidData(particle P, float2 pos)
{
    float den = 0.0f;
    float3 curl = to_float3_s(0.0f);
    
    float2 gradki = to_float2_s(0.0f);
    float gradl = 0.0f;
    
    float rho = (P.M < 1.0f) ? fluid_rho*0.5f : fluid_rho*P.M;
    
    int I = int(_ceil(particle_rad*pixel_scale)); 
    range(i, -I, I) range(j, -I, I)
    {
        float2 tpos = pos + to_float2(i,j);
        float4 data = texel(ch0, tpos);
        particle P0 = getParticle(data, tpos);
        
        if (P0.M == 0.0f) continue;
 
        //density
        den += W(P.NX - P0.NX, particle_rad) * P0.M;
        //den += distribution(P.NX, P0.NX, to_float2(particle_rad*pixel_scale)).z * P0.M;
        
        //gradient
        float2 g = WS(P.NX - P0.NX, particle_rad) * P0.M / rho;// * particle_rad*pixel_scale;
        gradki += g;
        gradl += dot2(g);

        //curl
        if (i == 0 && j == 0) continue;
        float2 u = (vel(P) - vel(P0)) * P0.M;
    float2 v = WS(P.NX - P0.NX, particle_rad);  
        //vec2 v = W(P.NX - P0.NX, particle_rad) * normalize(P.NX - P0.NX); 
    curl += cross(to_float3_aw(u, 0.0f), to_float3_aw(v, 0.0f));    
    }
    gradl += dot2(gradki);

    //pressure
    float Y = 3.0f;
    float C = 0.08f;
    float pr = ((fluid_rho*C)/Y) * (_powf(den/rho, Y) - 1.0f);
    
    //pr = den/fluid_rho - 1.0f;
        
    //some hardcoded stuff
    //gas
    if (P.M < 1.0f)
        pr = den*0.02f;
    //pr = 0.02f*(den-rho);
    
    //pr = _fmaxf(pr, 0.0f);
    if (pr < 0.0f)
        pr *= 0.1f;
        

    float l = pr / (gradl + 0.01f);
  
    return to_float4(den, pr, l, curl.z);
}

__KERNEL__ void FoamyAngerJipi126Fuse__Buffer_B(float4 O, float2 pos, float iTime, float2 iResolution)
{
    pos+=0.5f;

    R = iResolution; time = iTime;
    int2 p = to_int2(pos);

    float4 data = texel(ch0, pos);
    particle P = getParticle(data, pos);
   
    if (P.M > 0.0f)   
        O = FluidData(P, pos);


  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1




__KERNEL__ void FoamyAngerJipi126Fuse__Buffer_C(float4 O, float2 pos, float iTime, float2 iResolution, float4 iMouse)
{
    pos+=0.5f;
    
    R = iResolution; time = iTime; Mouse = iMouse;     
    float4 data = texel(ch0, pos); 
    particle P = getParticle(data, pos);
      
    if (P.M > 0.0f && !imBorder(P.NX))
        Simulation(ch0, ch1, P, pos);

    O = saveParticle(P, pos);


  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer B' to iChannel1
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0




__KERNEL__ void FoamyAngerJipi126Fuse__Buffer_D(float4 O, float2 pos, float iTime, float2 iResolution, float4 iMouse)
{

    pos+=0.5f;
     
    R = iResolution; time = iTime; Mouse = iMouse;     
    float4 data = texel(ch0, pos); 
    particle P = getParticle(data, pos);
      
    if (P.M > 0.0f && !imBorder(P.NX))
        Simulation(ch0, ch1, P, pos);
 
    O = saveParticle(P, pos);


  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'Previsualization: Buffer D' to iChannel0



__DEVICE__ float sminMe(float a, float b, float k, float p, out float t)
{
    float h = _fmaxf(k - _fabs(a-b), 0.0f)/k;
    float m = 0.5f * _powf(h, p);
    t = (a < b) ? m : 1.0f-m;
    return _fminf(a, b) - (m*k/p);
}

__DEVICE__ float smin( in float a, in float b, in float k )
{
    float h = _fmaxf(k-_fabs(a-b),0.0f);
    float m = 0.25f*h*h/k;
    return _fminf(a,  b) - m;
}

__KERNEL__ void FoamyAngerJipi126Fuse(float4 O, float2 pos, float iTime, float2 iResolution)
{

  pos+=0.5f;
  R = iResolution; time = iTime;

    swi3(O,x,y,z) = to_float3_s(1.0f);
    
    float d = 100.0f;
    
    float3 c = to_float3_s(1.0f);
    float m = 1.0f;
    float v = 0.0f;
    
    //rendering
    int I = int(_ceil(particle_size*0.5f))+2; 
    range(i, -I, I) range(j, -I, I)
    {
        float2 tpos = pos + to_float2(i,j);
        float4 data = texel(ch0, tpos);
        particle P0 = getParticle(data, tpos);
        
        if (P0.M == 0.0f) continue;
            
        float nd = distance(pos, P0.NX) - P0.R;

        float k = 4.0f / (1.0f + _fabs(m - P0.M)*1.5f);
        float t;
        d = sminMe(d, nd, k, 3.0f, t);
        m = _mix(m, P0.M, t);
        v = _mix(v, texel(ch1, tpos).w, t);
    }  
    
    //shadow
    float s = 100.0f;
    float2 off = to_float2(10.0f, 20.0f);
    if (d > 0.0f)
    {
        range(i, -I, I) range(j, -I, I)
        {
            float2 tpos = pos-off + to_float2(i,j);
            float4 data = texel(ch0, tpos);
            particle P0 = getParticle(data, tpos);
            
            if (tpos.x < 0.0
            ||  tpos.x > R.x
            ||  tpos.y < 0.0
            ||  tpos.y > R.x) { s = 0.0f; break; }
            if (P0.M == 0.0f)  { continue; }

            float nd = distance(pos - off, P0.NX) - P0.R;
            if (texel(ch1, tpos).x > 1.0f)
                s = smin(s, nd, 3.0f);
        } 
    }
    
    //coloring and stuff
    if (d < 0.0f)
        //d = 1.0f-_cosf(d);
        d = _sinf(d);
    swi3(O,x,y,z) *= to_float3(_fabs(d));
    if (d < 0.0f)
    {
        swi3(O,x,y,z) *= c;
        swi3(O,x,y,z) /= m*2.0f;
        //swi3(col,x,y,z) /= 0.5f + m*0.25f;
        swi3(O,x,y,z) -= to_float3_aw(v) / m * 0.06f;
    }
    
    //checkerboard
    if (d > 1.0f)
    {
        float size = 100.0f;
        float cy = step(mod_f(pos.y, size), size*0.5f);
        float ct = step(mod_f(pos.x + cy*size*0.5f, size), size*0.5f);
        
        ct = _saturatef(ct + 0.0f);        
        //swi3(col,x,y,z) = _mix(to_float3(ct), swi3(col,x,y,z), 1.0f-_saturatef(d));
    }
    
    swi3(O,x,y,z) = _saturatef(swi3(O,x,y,z));    
    if (d > 0.0f)
        swi3(O,x,y,z) *= _mix(to_float3_s(0.7f), to_float3_s(1.0f), _saturatef(s));
        
    swi3(O,x,y,z) = _powf(swi3(O,x,y,z), to_float3_s(0.7f));


  SetFragmentShaderComputedColor(O);
}