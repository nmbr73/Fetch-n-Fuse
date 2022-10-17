
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


#define R iResolution

#define Bf(p) p
#define Bi(p) to_int2_cfloat(p)
//#define texel(a, p) texelFetch(a, Bi(p), 0)
#define texel(a, p) texture(a, (make_float2(Bi(p))+0.5f)/R)

#define texelLoop(a, p) texelFetch(a, Bi(mod_f(p,R)), 0)
#define pixel(a, p) texture(a, (p)/R)
#define textureLoop(a, p) texture(a, mod_f(p,R)/R)
#define ch0 iChannel0
#define ch1 iChannel1
#define ch2 iChannel2
#define ch3 iChannel3

#define PI 3.14159265f

#define loop(i,x) for(int i = 0; i < x; i++)
#define range(i,a,b) for(int i = a; i <= b; i++)

#define _saturatef(x) clamp(x, 0.0f, 1.0f)
#define Gn(x) _expf(-dot(x,x))
#define G0n(x) _expf(-length(x))

#define dt 1.0f

//------
//Params
//------

#define SCALE 4.25f
#define DT_MULT  ((P0.M.x) + length(P0.V))

//0-1
#define DIFFUSION_CENTER 0.0f
#define DC_MULT (1.0f - _powf(length(P0.V), 0.25f))

#define MASS_REST 0.6f
#define MR_SPEED 0.05f
#define MR_INCREASE _fabs(MASS_REST - P.M.x)*0.05

#define CENTERING_SPEED 0.1f

#define GRAVITY_MULT 0.0f

//effects
#define COLL_AMNT 1.0f
#define SMOKE_AMNT 0.25f

#define FLUID_AMNT 0.25f
#define fluid_rho 0.2f
#define particle_rad 1.5f





//float2 R;
//float4 Mouse;
//float time;


__DEVICE__ float Pf(float den, float rest)
{
    //return 0.2f*den;
    //return 0.04f*(den-rest);
    return 0.05f*den*(den/rest- 1.0f);
}

__DEVICE__ float W(float2 r, float h)
{
  return (length(r) > 0.0f && h >= length(r)) ?
    (315.0f / (64.0f * PI * _powf(h, 9.0f))) * _powf((_powf(h, 2.0f) - _powf(length(r), 2.0f)), 3.0f) :
    0.0f;
}

__DEVICE__ float2 WS(float2 r, float h)
{
  return (length(r) > 0.0f && h >= length(r)) ?
    -(45.0f / (PI * _powf(h, 6.0f))) * _powf((h - length(r)), 2.0f) * (r / length(r)) :
    to_float2_s(0.0f);
}

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
    return length(_fmaxf(d,to_float2_s(0.0f))) + _fminf(_fmaxf(d.x,d.y),0.0f);
}

__DEVICE__ float border(float2 p, float2 R, float time)
{
    float bound = -sdBox(p - R*0.5f, R*to_float2(0.5f, 0.5f)); 
    float box = sdBox(mul_mat2_f2(Rot(1.0f*time-0.0f) , (p - R*to_float2(0.5f, 0.5f))) , R*to_float2(0.15f, 0.01f));
    float drain = -sdBox(p - R*to_float2(0.5f, 0.7f), R*to_float2(1.5f, 0.5f));
    return bound;
    return _fminf(bound, box);
    return _fmaxf(drain,_fminf(bound, box));
}

#define hh 1.
__DEVICE__ float3 bN(float2 p, float2 R, float time)
{
    float3 dx = to_float3(-hh,0,hh);
    float4 idx = to_float4(-1.0f/hh, 0.0f, 1.0f/hh, 0.25f);
    float3 r = swi3(idx,z,y,w)*border(p + swi2(dx,z,y),R,time)
             + swi3(idx,x,y,w)*border(p + swi2(dx,x,y),R,time)
             + swi3(idx,y,z,w)*border(p + swi2(dx,y,z),R,time)
             + swi3(idx,y,x,w)*border(p + swi2(dx,y,x),R,time);
    return to_float3_aw(normalize(swi2(r,x,y)), r.z + 1e-4);
}

__DEVICE__ uint pack(float2 x)
{
    x = 65534.0f*clamp(0.5f*x+0.5f, 0.0f, 1.0f);
    return (uint)(round(x.x)) + 65535u*(uint)(round(x.y));
}

__DEVICE__ float2 unpack(uint a)
{
    float2 x = make_float2(a%65535u, a/65535u);
    return clamp(x/65534.0f, 0.0f,1.0f)*2.0f - 1.0f;
}




// Auxiliary functions
union Zahl
 {
   float  _Float; //32bit float
   uint   _Uint;  //32bit unsigend integer
 };

__DEVICE__ uint _float2half(uint f) {
	return	((f >> (uint)(16)) & (uint)(0x8000)) |
		((((f & (uint)(0x7f800000)) - (uint)(0x38000000)) >> (uint)(13)) & (uint)(0x7c00)) |
		((f >> (uint)(13)) & (uint)(0x03ff));
}

__DEVICE__ uint _half2float(uint h) {
	return ((h & (uint)(0x8000)) << (uint)(16)) | ((( h & (uint)(0x7c00)) + (uint)(0x1c000)) << (uint)(13)) | ((h & (uint)(0x03ff)) << (uint)(13));
}

__DEVICE__ uint _packHalf2x16(float2 v) {
	
  Zahl zx,zy;
  
  zx._Float = v.x;
  zy._Float = v.y;
  
  //return _float2half(floatBitsToUint(v.x)) | _float2half(floatBitsToUint(v.y)) << uint(16);
  return _float2half(zx._Uint) | _float2half(zy._Uint) << (uint)(16);
}

__DEVICE__ float2 _unpackHalf2x16(uint v) {	
	
  Zahl zx, zy;
  
  zx._Uint = _half2float(v & (uint)(0xffff));
  zy._Uint = _half2float(v >> (uint)(16));
  
  //return vec2(uintBitsToFloat(_half2float(v & uint(0xffff))),
	//	          uintBitsToFloat(_half2float(v >> uint(16))));
  return to_float2(zx._Float,zy._Float);
}



__DEVICE__ float2 decode(float _x)
{
	Zahl z;
    //uint X = floatBitsToUint(x);
	z._Float = _x;
	
  return _unpackHalf2x16(z._Uint); 
}

__DEVICE__ float encode(float2 _x)
{
	Zahl z;
  uint X = _packHalf2x16(_x);
	
	z._Uint = X;
  //return uintBitsToFloat(X); 
	return (z._Float);
}


struct particle
{
    float2 X;
    float2 V;
    float2 M;
};
    
__DEVICE__ particle getParticle(float4 data, float2 pos)
{
    particle P; 
    P.X = decode(data.x) + pos;
    P.V = decode(data.y);
    P.M = swi2(data,z,w);
    return P;
}

__DEVICE__ float4 saveParticle(particle P, float2 pos)
{
    P.X = P.X - pos;
    return to_float4(encode(P.X), encode(P.V), (P.M.x),(P.M.y));
}

__DEVICE__ float3 hash32(float2 p)
{
  float3 p3 = fract_f3((swi3(p,x,y,x)) * to_float3(0.1031f, 0.1030f, 0.0973f));
    p3 += dot(p3, swi3(p3,y,x,z)+33.33f);
    return fract_f3((swi3(p3,x,x,y)+swi3(p3,y,z,z))*swi3(p3,z,y,x));
}

__DEVICE__ float G(float2 x)
{
    //return _powf(50.0f, -dot(x,x));
    //return _expf(-length(x)*length(x));
    return _expf(-dot(x,x));
}

__DEVICE__ float G0(float2 x)
{
    //return _powf(2.0f, -length(x));
    return _expf(-length(x));
}


__DEVICE__ float GT(float2 d, float h)
{
if (length(d)>h || length(d)<0.0f)
    return 0.0f;
    return _powf(h - _powf(length(d), 2.0f), 3.0f);
}

__DEVICE__ float GT0(float2 d, float h)
{
if (length(d)>h || length(d)<0.0f)
    return 0.0f;
    return _powf(h - length(d), 2.0f);
}


// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer C' to iChannel0



__DEVICE__ float3 distribution(float2 x, float2 p, float K)
{
    float2 omin = clamp(x - K*0.5f, p - 0.5f, p + 0.5f);
    float2 omax = clamp(x + K*0.5f, p - 0.5f, p + 0.5f); 
    return to_float3_aw(0.5f*(omin + omax), (omax.x - omin.x)*(omax.y - omin.y)/(K*K));
}

//diffusion and advection basically
__DEVICE__ particle Reintegration(__TEXTURE2D__ ch, inout particle P, float2 pos, float iTime, float2 R)
{
    int I = 10; 
    range(i, -I, I) range(j, -I, I)
    {
        float2 tpos = pos + to_float2(i,j);
        float4 data = texel(ch, tpos);
       
        particle P0 = getParticle(data, tpos);
        
        if (P0.M.y < -9999.0f || P.M.y < -9999.0f)
            continue;
       
        P0.X += P0.V*dt * DT_MULT;

        //float difR = 2.75f + _powf(length(P0.V), 4.5f) * 0.5f;
        float difR = SCALE;

        float3 D = distribution(P0.X, pos, difR);
        float m = P0.M.x*D.z; //the deposited mass into this cell

        //add weighted by mass
        P.X += _mix(P0.X, swi2(D,x,y), DIFFUSION_CENTER * DC_MULT)*m;
        //P.X += swi2(D,x,y)*m;
        //P.X += P0.X*m;
        P.V += P0.V*m;
        P.M.y += P0.M.y*m;
        
        //add mass
        P.M.x += m;
    }
    
    //normalization
    if (P.M.x != 0.0f)
    {
        P.X /= P.M.x;
        P.V /= P.M.x;
        P.M.y /= P.M.x;
    }
    
    //----- 
    float prevM = P.M.x;
    P.M.x = _mix(P.M.x, MASS_REST, MR_SPEED + MR_INCREASE);
    P.V = P.V * prevM/P.M.x;
    
    P.M.y = _mix(P.M.y, length(P.V),  0.1f);
    
    float2 CP = clamp(P.X - pos, -0.5f, 0.5f) + pos;
    //CP = pos;
    P.X = _mix(P.X, CP, _saturatef(distance_f2(P.X, pos) - 0.0f) * CENTERING_SPEED);
    
    //border/solids
    if (border(pos,R, iTime) < 3.0f)
    {
        P.X = pos;
        P.V = to_float2_s(0.0f);
        P.M.x = 90.05f;
        P.M.y = -10000.5f;
    }
    
    return P;
}


__KERNEL__ void ParticleFieldJipiFuse__Buffer_A(float4 U, float2 pos, float iTime, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0);

    pos+=0.5f;

    //R = iResolution; time = iTime; Mouse = iMouse;
    int2 p = to_int2_cfloat(pos);

    float4 data = texel(ch0, pos); 
    
    particle P = {to_float2_s(0.0f),to_float2_s(0.0f),to_float2_s(0.0f)};// = getParticle(data, pos);
       
    
   
    if (iFrame < 1 || Reset)
    {
        float3 rand = hash32(pos);
        
        P.X = pos;
        P.V = to_float2_s(0.0f) + (0.5f*(swi2(rand,x,y)-0.5f))*0.5f;
        P.M = to_float2(1.0f, 0.0f);
    }
    else
      P = Reintegration(ch0, P, pos, iTime, R);
    
    U = saveParticle(P, pos);

  SetFragmentShaderComputedColor(U);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1


//force calculation and integration
__DEVICE__ particle Simulation(__TEXTURE2D__ ch, in particle P, float2 pos, float2 R, float iTime, float4 iMouse)
{
    float2 F = to_float2_s(0.0f);
    int I = 3;
    range(i, -I, I) range(j, -I, I)
    {
        float2 tpos = pos + to_float2(i,j);
        float4 data = texel(ch, tpos);
        particle P0 = getParticle(data, tpos);
              
        float2 dx = P0.X - P.X;
        float d = length(dx);         
        
        //border
        if (P0.M.y < -9999.9f)
        {
            if (d < 3.0f)
                F -= dx * (3.0f - d) * _saturatef(P.M.x-1.5f) * 0.25f;
           continue;
        }
        
        //collision-ish
        float L = 2.5f;
        if (d < L)
            F -= dx* (L - d)
            * _fabs(length(P0.V)-length(P.V))
            * _fabs(_saturatef(P0.M.x-1.0f)-_saturatef(P.M.x-1.0f))
            * -dot(P.V, P0.V)
            * G0(dx*4.0f/L)
            * COLL_AMNT;
            
        //smoke 
        if (d < L)
            F -= dx* (L - d)
            * _saturatef(P0.M.x-P.M.x)
            * (P0.M.x/P.M.x)
            * _saturatef(length(P0.V)-length(P.V))
            * G0(dx*4.0f/L)
            * SMOKE_AMNT;
            
        //fluid
        L = particle_rad;
        if (d < L)
            F -= dx * (L - d)
            //* (Pf(P0.M.x, fluid_rho)+Pf(P.M.x, fluid_rho))
            * P0.M.x
            * G0(dx*4.0f/L)
            * (Pf(_saturatef(P0.M.x-1.0f), fluid_rho)+Pf(_saturatef(P.M.x-1.0f), fluid_rho))
            //* _fabs(_saturatef(P0.M.x-1.0f)-_saturatef(P.M.x-1.0f))
            * FLUID_AMNT;    
    }
    
    F += to_float2(0.0f, -0.01f)* P.M.x * length(P.V) * GRAVITY_MULT;
    
    if (iMouse.z > 0.0f)
    {
        float2 dx = pos - swi2(iMouse,x,y);
        if (length(dx) < 30.0f)
            F += 0.005f* mul_mat2_f2(Rot(PI*0.0f*iTime) , dx);// * (0.0f + length(dm)*0.25f);
    }       
          
    //integrate
    P.V += F;
    
    return P;
}


__KERNEL__ void ParticleFieldJipiFuse__Buffer_C(float4 U, float2 pos, float iTime, float2 iResolution, float4 iMouse, int iFrame)
{
  
    pos+=0.5f;

    //R = iResolution; time = iTime; Mouse = iMouse;
    int2 p = to_int2_cfloat(pos);
        
    float4 data = texel(ch0, pos); 
    
    particle P = getParticle(data, pos);

    if(iFrame <= 0) // Versuch out of Memory zu verhindern
      P.X = to_float2_s(0.0f), P.V = to_float2_s(0.0f), P.M = to_float2_s(0.0f);
    else
    
    if(P.M.x != 0.0f) //not vacuum
    {
        P = Simulation(ch0, P, pos, R, iTime, iMouse);
    }
    
    //if (iTime < 5.0f)
    {   
        //if(length(P.X - R*to_float2(0.9f, 0.85f)) < 15.0f) 
        //    P.V += 0.125f*Dir(-PI*0.25f - PI*0.5f + 0.3f*_sinf(0.4f*time));
        //if(length(P.X - R*to_float2(0.1f, 0.85f)) < 15.0f) 
        //    P.V += 0.125f*Dir(-PI*0.25f + 0.3f*_sinf(0.3f*time));
        
        float2 dx = P.X - to_float2((R.x - ((float)(iFrame)*2.2f)), 0.5f*R.y);
        if (length(dx - to_float2(0.4f, 0.0f)) < 15.0f)
            P.V += 0.5f*Dir(-PI*0.0f);         
        if (length(dx) < 30.0f)
            P.V += 0.005f* mul_mat2_f2(Rot(PI*0.0f*iTime),dx);
    }
    
    U = saveParticle(P, pos);

  SetFragmentShaderComputedColor(U);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'Previsualization: Buffer C' to iChannel2


// Fork of "Paint streams" by michael0884. https://shadertoy.com/view/WtfyDj
// 2020-12-16 10:30:33

__KERNEL__ void ParticleFieldJipiFuse(float4 col, float2 pos, float iTime, float2 iResolution)
{
    pos+=0.5f;

    //R = iResolution; time = iTime;
    int2 p = to_int2_cfloat(pos);
    
    float4 data = texel(ch0, pos);
    particle P = getParticle(data, pos);
    
    float2 V = P.V*1.2f;

    //vec3 cv = to_float3_aw(V*2.0f, length(V)-0.5f*length(V) ) * 0.5f + 0.5f;
    float3 cv = to_float3_aw(V*0.5f+0.5f, length(_fminf(mul_f2_mat2(V , Rot(length(V))), to_float2_s(0.0f)))*0.5f );
    cv = _mix(to_float3_s(1.0f), cv, length(V));
    //cv.z = 1.0f;
    cv -= swi3(_saturatef(cv-1.0f),y,z,x)*1.0f;
   
    //swi3(col,x,y,z) = cv;
    col.x = cv.x;
    col.y = cv.y;
    col.z = cv.z;
    //swi3(col,x,y,z) *= P.M.y*1.25f*0.8f;
    
    float d = distance_f2(P.X, pos)/SCALE*2.0f;
    d = _fmaxf(_fabs(P.X.x - pos.x),
               _fabs(P.X.y - pos.y)) / SCALE * 2.0f;
    d = _powf(d, 0.25f);
    //swi3(col,x,y,z) *= 0.2f+to_float3(d);
  
    swi3S(col,x,y,z, swi3(col,x,y,z) * _powf(P.M.x*0.6f, 0.75f));
    swi3S(col,x,y,z, pow_f3(swi3(col,x,y,z), to_float3_s(1.0f/2.2f))); 
    
    //swi3(col,x,y,z) = 1.0f - swi3(col,x,y,z);

  SetFragmentShaderComputedColor(col);
}