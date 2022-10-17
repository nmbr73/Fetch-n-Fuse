
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


#define R iResolution

#define Bf(p) p
#define Bi(p) to_int2_cfloat(p)
//#define texel(a, p) texelFetch(a, Bi(p), 0)
#define texel(a, p) texture(a, (to_float2_cint(Bi(p))+0.5f)/R)

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

#define dt 1.0

#define UP to_float3(1.0f, 0.0f, 0.0f)
#define RED UP

#define fluid_rho 1.5f
#define particle_rad 1.5f


__DEVICE__ float Pf(float den, float rest)
{
    return 0.4f*(den-rest);
    //return 0.05f*den*(den/rest- 1.0f);
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
    float box = sdBox(mul_mat2_f2(Rot(0.0f*time-0.0f) , (p - R*to_float2(0.5f, 0.5f))) , R*to_float2(0.15f, 0.01f));
    float drain = -sdBox(p - R*to_float2(0.5f, 0.7f), R*to_float2(1.5f, 0.5f));
    return bound;
    return _fminf(bound, box);
    return _fmaxf(drain,_fminf(bound, box));
}

#define hh 1.0f
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
    float2 x = to_float2(a%65535u, a/65535u);
    return clamp(x/65534.0f, 0.0f,1.0f)*2.0f - 1.0f;
}



union Zahl
 {
   float  _Float; //32bit float
   uint   _Uint;  //32bit unsigend integer
 };



// Auxiliary functions
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
    return to_float4(encode(P.X), encode(P.V), (P.M.x), (P.M.y));
}

__DEVICE__ float3 hash32(float2 p)
{
  float3 p3 = fract_f3((swi3(p,x,y,x)) * to_float3(0.1031f, 0.1030f, 0.0973f));
  p3 += dot(p3, swi3(p3,y,x,z)+33.33f);
  return fract_f3((swi3(p3,x,x,y)+swi3(p3,y,z,z))*swi3(p3,z,y,x));
}

__DEVICE__ float G(float2 x)
{
  float ttttttttttttttttttttt;
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
// Connect Buffer A 'Previsualization: Buffer B' to iChannel0



__DEVICE__ float3 distribution(float2 x, float2 p, float K)
{
    float2 omin = clamp(x - K*0.5f, p - 0.5f, p + 0.5f);
    float2 omax = clamp(x + K*0.5f, p - 0.5f, p + 0.5f); 
    return to_float3_aw(0.5f*(omin + omax), (omax.x - omin.x)*(omax.y - omin.y)/(K*K));
}


//diffusion and advection basically
__DEVICE__ particle Reintegration(__TEXTURE2D__ ch, in particle P, float2 pos, float2 R)
{
    int I = 6; 
    range(i, -I, I) range(j, -I, I)
    {
        float2 tpos = pos + to_float2(i,j);
        float4 data = texel(ch, tpos);
       
        particle P0 = getParticle(data, tpos);
        
        P0.X += P0.V*dt;// * (1.0f + (P0.M.x) * length(P0.V));

        float difR = 1.8f + (P0.M.x/fluid_rho);

        float3 D = distribution(P0.X, pos, difR);
        float m = P0.M.x*D.z; //the deposited mass into this cell

        P.X += _mix(P0.X, swi2(D,x,y), _saturatef( _powf(texel(ch, pos).z, 1.25f)*1.5f) )*m;  
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
    P.M.x = _fmaxf(0.002f, P.M.x);
    P.V = P.V * prevM/P.M.x;
      
    P.M.y = _mix(0.0f, P.M.x, 0.5f);

    P.X = clamp(P.X - pos, -0.5f, 0.5f) + pos;
    
    return P;
}
__KERNEL__ void InfiniteSadnessJipiFuse__Buffer_A(float4 U, float2 pos, float iTime, float2 iResolution, float4 iMouse, int iFrame)
{

    pos+=0.5f;
float AAAAAAAAAAAAAAAAAAAA;
    //R = iResolution; time = iTime; Mouse = iMouse;
    int2 p = to_int2_cfloat(pos);

    float4 data = texel(ch0, pos); 
    
    particle P = { to_float2_s(0.0f),to_float2_s(0.0f),to_float2_s(0.0f)};  // = getParticle(data, pos);
       
    P = Reintegration(ch0, P, pos, R);
   
    if (iFrame < 1)
    {
        float3 rand = hash32(pos);
        
        P.X = pos;
        P.V = to_float2_s(0.0f) + (0.5f*(swi2(rand,x,y)-0.5f))*0.5f;
        P.M = to_float2(rand.y*0.02f, 0.0f);
    }
    
    U = saveParticle(P, pos);

  SetFragmentShaderComputedColor(U);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1


//force calculation and integration
__DEVICE__ particle Simulation(__TEXTURE2D__ ch, in particle P, float2 pos, float2 R, float4 iMouse, float iTime)
{
    float2 F = to_float2_s(0.0f);
    int I = 3;
    range(i, -I, I) range(j, -I, I)
    {
        float2 tpos = pos + to_float2(i,j);
float zzzzzzzzzzzzzzzzz;        
        float4 data = texel(ch, tpos);
        particle P0 = getParticle(data, tpos);
              
        float2 dx = P0.X - P.X;
        float d = length(dx);         
                      
        //fluid
        float L = particle_rad;
        if (d < L && d > 0.001f)
            F -= normalize(dx) * (L - d)
                  * P0.M.x
                  * G0(dx*4.0f/L)
                  //* (Pf(P0.M.y, fluid_rho)+Pf(P.M.y, fluid_rho))
                  * (Pf(P0.M.x, fluid_rho)+Pf(P.M.x, fluid_rho))
                  * 0.5f;   
         
        //viscosity
        F += G(dx*2.0f/1.5f) * (P0.V - P.V) * 0.8f;
    }
    
    float l = length(P.V);
    float v = ( l< 1.0f)
                ? _powf(l, 1.0f)
                : _powf(l, 0.25f);
                
    F += to_float2(0.0f, -0.04f) * P.M.x * P.M.y * v;
    
    if (iMouse.z > 0.0f)
    {
        float2 dx = pos - swi2(iMouse,x,y);
        float d = length(dx);
        
        if (d < 30.0f)
            F += mul_f2_mat2(dx * (30.0f-d) , Rot(PI*0.0f*iTime)) * 0.002f;
    }    
                     
    // Border
    float2 pd = P.X + P.V*dt;
    if (pd.y - 5.0f < 0.0f)
    {
        P.V.y *= 1.0f + ((pd.y - 5.0f)/5.0f);
        //P.V.x /= 1.0f + ((pd.y - 5.0f)/5.0f)*0.5f *(1.0f+P.M.x);

        P.X += to_float2(0.0f, 1.0f) * _fabs(pd.y - 5.0f);
        P.V += to_float2(0.0f, 1.0f) * _fabs(pd.y - 5.0f)*0.1f*P.M.x;
        P.M.x = _fmaxf(P.M.x - 0.001f, 0.0f);
    }
    
    // Scale
    /*if (P.M.x > fluid_rho && distance(pos, P.X) != 0.0f)
        F += normalize(pos - P.X) * _fmaxf(P.M.x - fluid_rho, 0.0f) * 0.05f;*/
        
    //integrate
    P.V += F;
    
    P.V /= 1.0f + _powf(length(P.V)*0.1f, 3.0f);
    
    return P;
}


__KERNEL__ void InfiniteSadnessJipiFuse__Buffer_B(float4 U, float2 pos, float iTime, float2 iResolution, float4 iMouse)
{

    pos+=0.5f;
    
    //R = iResolution; time = iTime; Mouse = iMouse;
    int2 p = to_int2_cfloat(pos);
        
    float4 data = texel(ch0, pos); 
float BBBBBBBBBBBBBBBBBBBBBBBB;    
    particle P = getParticle(data, pos);

    
    if(P.M.x != 0.0f) //not vacuum
    {
        P = Simulation(ch0, P, pos, R, iMouse, iTime);
    }
    
    if (iTime < 30.0f)
    {   
        if(length(P.X - R*to_float2(0.9f, 0.85f)) < 15.0f * (R.x/600.0f))
        {
            P.X = pos;
            P.V = 0.5f*Dir(-PI*0.25f - PI*0.5f + 0.3f*_sinf(0.4f* iTime));
            P.M.x = 1.75f;
        }
        if(length(P.X - R*to_float2(0.1f, 0.85f)) < 15.0f * (R.x/600.0f)) 
        {
            P.X = pos;
            P.V = 0.5f*Dir(-PI*0.25f + 0.3f*_sinf(0.3f*iTime));
            P.M.x = 1.75f;
        }
    }

    U = saveParticle(P, pos);

  SetFragmentShaderComputedColor(U);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0

// Connect Image 'Previsualization: Buffer B' to iChannel2


// Fork of "Paint streams" by michael0884. https://shadertoy.com/view/WtfyDj
// 2020-12-16 10:30:33

__DEVICE__ float3 normals(float2 pos, __TEXTURE2D__ sampler, float2 R)
{
    float c = texel(sampler, pos).z;
    float l = texel(sampler, pos + to_float2(-1.0f, 0.0f)).z;
    float r = texel(sampler, pos + to_float2(1.0f, 0.0f)).z;
    float d = texel(sampler, pos + to_float2(0.0f, -1.0f)).z;
    float u = texel(sampler, pos + to_float2(0.0f, 1.0f)).z;
    
    
    float3 va = normalize(to_float3_aw(to_float2(2.0f, 0.0f), r-l));
    float3 vb = normalize(to_float3_aw(to_float2(0.0f, 2.0f), u-d));
    //return cross(va,vb);
    
    float ld = texel(sampler, pos + to_float2(-1.0f, -1.0f)).z;
    float ru = texel(sampler, pos + to_float2(1.0f, 1.0f)).z;
    float rd = texel(sampler, pos + to_float2(1.0f, -1.0f)).z;
    float lu = texel(sampler, pos + to_float2(-1.0f, 1.0f)).z;
    
    float me = (l+r+u+d+ld+ru+rd+ld)/8.0f;
    me = l * 0.25
        +r * 0.25
        +d * 0.25
        +u * 0.25f;
float kkkkkkkkkkkkkk;
    //return normalize(to_float3(l-r, d-u, c/c) * to_float3(c, c, 1.0f));
    return normalize(to_float3(l-r, d-u, _saturatef(me-c)+c*0.3f) * to_float3(c, c, 1.0f));
}

__KERNEL__ void InfiniteSadnessJipiFuse(float4 col, float2 pos, float iTime, float2 iResolution)
{

    pos+=0.5f;
float IIIIIIIIIIIIIIIIIIIIIII;    
    //R = iResolution; time = iTime;
    int2 p = to_int2_cfloat(pos);
    
    float4 data = texel(ch0, pos);
    particle P = getParticle(data, pos);
    
    
    // Velocity Color
    float2 V = mul_f2_mat2(P.V*0.2f , Rot(2.7f+iTime*0.1f));

    float3 vc = to_float3_aw(V, length(V)-0.5f*length(V) ) * 0.5f + 0.5f;
    vc = _mix(to_float3_s(1.0f), vc, length(V));
    vc.z = 1.0f;
    vc -= P.M.y*0.25f;
    
    vc = to_float3_s(1.0f); //Here yumminess <-----
   
    
    // Mass Color
    float3 mc = vc;
    mc *= _powf(P.M.x*0.5f, 0.75f);
    mc = pow_f3(mc, to_float3_s(0.75f)); 
    
    
    // Water Color
    float3 NN = normals(pos, ch0, R);
    
    float d = dot(NN, normalize(to_float3(-1.5f, 1.5f, 1.5f)))*0.5f+0.5f;
    float s = _powf(d, 50.0f);
    float c = _powf(s * 0.75f, 1.0f);
    
    float sd = dot(NN, normalize(to_float3(1.5f, -1.5f, 10.5f)));
    
    float3 wc = _mix(to_float3_s(1.0f), vc, _saturatef(P.M.x/1.5f)); 
    wc *= to_float3_s(_powf(sd*0.5f, 0.25f));   
    wc += to_float3_s(c*0.8f * P.M.x);  
    
    float test = dot(NN, normalize(to_float3(1.5f, -1.5f, 2.5f)))*0.5f+0.5f;
    test = _powf(test, 70.0f);
    wc += to_float3_s(test*P.M.x);

    //===
    swi3S(col,x,y,z, wc);
    //swi3(col,x,y,z) = mc; //And here milkyness <-----
    
    swi3S(col,x,y,z, pow_f3(swi3(col,x,y,z), to_float3_s(0.9f)));

  SetFragmentShaderComputedColor(col);
}