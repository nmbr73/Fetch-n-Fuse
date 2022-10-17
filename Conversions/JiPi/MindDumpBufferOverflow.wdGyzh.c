
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define R iResolution

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define Bf(p) mod_f(p,R)
#define Bi(p) to_int2_cfloat(mod_f2f2(p,R))
//#define texel(a, p) texelFetch(a, Bi(p), 0)
#define texel(a, p) texture(a, (make_float2(Bi(p))+0.5f)/R)


#define pixel(a, p) texture(a, (p)/R)
#define ch0 iChannel0
#define ch1 iChannel1
#define ch2 iChannel2
#define ch3 iChannel3

#define PI 3.14159265

#define loop(i,x) for(int i = 0; i < x; i++)
#define range(i,a,b) for(int i = a; i <= b; i++)

#define dt 1.5f
#define border_h 5.0f
#define mass 11.5f
#define fluid_rho 0.20f


//mold stuff 
#define sense_ang 0.14325975283464746f
#define sense_dis 01.2750f
#define sense_force 0.251f
#define trailing 0200.10f/_cosf(time*01.0f)
#define acceleration 0.051f

//SPH stuff
__DEVICE__ float Pf(float2 rho)
{
    return 0.0075042347503666f*rho.x + 0.0f*rho.y; //gas
    return 0.002f*rho.x*(rho.x/fluid_rho - 1.0f);  //water pressure
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
    return length(_fmaxf(d,to_float2_s(0.0f))) + _fminf(max(d.x,d.y),0.0f);
}

__DEVICE__ float border(float2 p, float2 R, float time)
{
    float bound = -sdBox(p - R*0.5f, R*to_float2(0.5f, 0.5f)); 
    float box = sdBox(mul_mat2_f2(Rot(0.0f*time),(p - R*to_float2(1.5f, 0.6f))) , R*to_float2(0.05f, 0.01f));
    float drain = -sdBox(p - R*to_float2(0.5f, 0.7f), R*to_float2(0.0f, 0.0f));
    return _fmaxf(drain,_fminf(bound, box));
}

#define h 1.0f
__DEVICE__ float3 bN(float2 p, float2 R, float time)
{
    float3 dx = to_float3(-h,0,h);
    float4 idx = to_float4(-1.0f/h, 0.0f, 1.0f/h, 0.25f);
    float3 r = swi3(idx,z,y,w)*border(p + swi2(dx,z,y), R,time)
             + swi3(idx,x,y,w)*border(p + swi2(dx,x,y), R,time)
             + swi3(idx,y,z,w)*border(p + swi2(dx,y,z), R,time)
             + swi3(idx,y,x,w)*border(p + swi2(dx,y,x), R,time);
    return to_float3_aw(normalize(swi2(r,x,y)), r.z + 1e-4);
}

__DEVICE__ uint pack(float2 _x)
{
    _x = 65534.0f*clamp(0.5f*_x+0.5f, 0.0f, 1.0f);
    return uint(round(_x.x)) + 65535u*uint(round(_x.y));
}

__DEVICE__ float2 unpack(uint a)
{
    float2 _x = to_float2(a%65535u, a/65535u);
    return clamp(_x/65534.0f, 0.0f,1.0f)*2.0f - 1.0f;
}

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
	
    return unpack(z._Uint); 
}

__DEVICE__ float encode(float2 _x)
{
	Zahl z;
    uint X = pack(_x);
	
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
    P.X = clamp(P.X - pos, to_float2_s(-0.5f), to_float2_s(0.5f));
    return to_float4(encode(P.X), encode(P.V), P.M.x, P.M.y);
}

__DEVICE__ float3 hash32(float2 p)
{
    float3 p3 = fract_f3(swi3(p,x,y,x) * to_float3(0.1031f, 0.1030f, 0.0973f));
    p3 += dot(p3, swi3(p3,y,x,z)+33.33f);
    return fract_f3((swi3(p3,x,x,y)+swi3(p3,y,z,z))*swi3(p3,z,y,x));
}

__DEVICE__ float G(float2 _x)
{
    return _expf(-dot(_x,_x));
}

__DEVICE__ float G0(float2 _x)
{
    return _expf(-length(_x));
}

//diffusion amount
#define dif 01.30f
__DEVICE__ float3 distribution(float2 x, float2 p, float K)
{
    float4 aabb0 = to_float4_f2f2(p - 0.5f, p + 0.5f);
    float4 aabb1 = to_float4_f2f2(x - K*0.5f, x + K*0.5f);
    float4 aabbX = to_float4_f2f2(_fmaxf(swi2(aabb0,x,y), swi2(aabb1,x,y)), _fminf(swi2(aabb0,z,w), swi2(aabb1,z,w)));
    float2 center = 0.5f*(swi2(aabbX,x,y) + swi2(aabbX,z,w)); //center of mass
    float2 size = _fmaxf(swi2(aabbX,z,w) - swi2(aabbX,x,y), to_float2_s(0.0f)); //only positive
    float m = size.x*size.y/(K*K); //relative amount
    //if any of the dimensions are 0 then the mass is 0
    return to_float3_aw(center, m);
}

//diffusion and advection basically
__DEVICE__ particle Reintegration(__TEXTURE2D__ ch, particle P, float2 pos, float2 R, float time)
{
    P.X *= 0.0f;
    P.V *= 0.0f;
    P.M.x *= 0.0f;
    
    P.M.y *= trailing;
    
    //basically integral over all updated neighbor distributions
    //that fall inside of this pixel
    //this makes the tracking conservative
    range(i, -2, 2) range(j, -2, 2)
    {
        float2 tpos = pos + to_float2(i,j);
        float4 data = texel(ch, tpos);

        particle P0 = getParticle(data, tpos);
       
        P0.X += P0.V*dt; //integrate position

        float3 D = distribution(P0.X, pos, dif);
        //the deposited mass into this cell
        float m = P0.M.x*D.z;
        
        //add weighted by mass
        P.X += swi2(D,x,y)*m;
        P.V += P0.V*m;
      
        //add mass
        P.M.x += m;
    }
    
    //normalization
    if(P.M.x != 0.0f)
    {
        P.X /= P.M.x;
        P.V /= P.M.x;
    }
    //pheromone trail
    P.M.y = P.M.x;
    
    return P;
}


//force calculation and integration
__DEVICE__ particle Simulation(__TEXTURE2D__ ch, particle P, float2 pos, float2 R, float time, float4 Mouse)
{
    //Compute the SPH force
    float2 F = to_float2_s(0.0f);
    float3 avgV = to_float3_s(0.0f);
    range(i, -2, 2) range(j, -2, 2)
    {
        float2 tpos = pos + to_float2(i,j);
        float4 data = texel(ch, tpos);
        particle P0 = getParticle(data, tpos);
        float2 dx = P0.X - P.X;
        float avgP = 0.5f*P0.M.x*(Pf(P.M) + Pf(P0.M)); 
        F -= 0.5f*G(1.0f*dx)*avgP*dx;
        avgV += P0.M.x*G(1.0f*dx)*to_float3_aw(P0.V,1.0f);
    }
    //swi2(avgV,x,y) /= avgV.z;
    avgV.x /= avgV.z;
    avgV.y /= avgV.z;

     //sensors
    float ang = _atan2f(P.V.y, P.V.x);
    float4 dir = sense_dis*to_float4_f2f2(Dir(ang+sense_ang), Dir(ang - sense_ang));
    float2 sd = to_float2(pixel(ch, P.X + swi2(dir,x,y)).w, pixel(ch, P.X + swi2(dir,z,w)).w);
    F += sense_force*(Dir(ang+PI*0.5f)*sd.x + Dir(ang-PI*0.5f)*sd.y); 
    
    //gravity
    F -= sin_f2(-0.000f*P.M.x*to_float2(0,1));

    if(Mouse.z > 0.0f)
    {
        float2 dm =(swi2(Mouse,x,y) - swi2(Mouse,z,w))/10.0f; 
        float d = distance_f2(swi2(Mouse,x,y), P.X)/20.0f;
        F += 0.01f*dm*_expf(-d*d);
       // P.M.y += 0.1f*_expf(-40.0f*d*d);
    }
    
    //integrate
    P.V += F*dt/P.M.x;

    //border 
    float3 N = bN(P.X, R, time);
    float vdotN = step(N.z, border_h)*dot(-1.0f*swi2(N,x,y), P.V);
    P.V += 0.5f*(swi2(N,x,y)*vdotN + swi2(N,x,y)*_fabs(vdotN));
    P.V += 0.0f*P.M.x*swi2(N,x,y)*step(_fabs(N.z), border_h)*_expf(-N.z);
    
    if(N.z < 0.0f) P.V = to_float2_s(0.0f);
    
    P.V *= 1.0f + acceleration;
    //velocity limit
    float v = length(P.V);
    P.V /= (v > 1.0f)?1.0f*v:1.0f;
    
    return P;
}



// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer B' to iChannel0



__KERNEL__ void MindDumpBufferOverflowFuse__Buffer_A(float4 U, float2 pos, float iTime, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0);
    pos+=0.5f;
float AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA;    
    float4 data = texel(ch0, pos); 
    
    particle P = getParticle(data, pos);
       
    P = Reintegration(ch0, P, pos, R, iTime);
   
    //initial condition
    if(iFrame < 1 || Reset)
    {
        //random
        float3 rand = hash32(pos);
        rand.z = distance_f2(pos, R*0.5f)/R.x;
        if(rand.z < 0.1f) 
        {
            P.X = pos;
            P.V = 0.5f*(swi2(rand,x,y)-0.5f) + 0.0f*to_float2(_sinf(2.0f*pos.x/R.x), _cosf(2.0f*pos.x/R.x));
            P.M = to_float2(mass, 0.0f);
        }
        else
        {
            P.X = pos;
            P.V = to_float2_s(0.0f);
            P.M = to_float2_s(1e-6);
        }
    }
    
    U = saveParticle(P, pos);


  SetFragmentShaderComputedColor(U);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void MindDumpBufferOverflowFuse__Buffer_B(float4 U, float2 pos, float iTime, float2 iResolution, float4 iMouse)
{

    pos+=0.5f;
    
    float4 data = texel(ch0, pos); 
    
    particle P = getParticle(data, pos);
float BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB;    
    
    if(P.M.x != 0.0f) //not vacuum
    {
        P = Simulation(ch0, P, pos, R, iTime, iMouse);
    }
    
    U = saveParticle(P, pos);


  SetFragmentShaderComputedColor(U);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0


//density

__KERNEL__ void MindDumpBufferOverflowFuse__Buffer_C(float4 fragColor, float2 pos, float iTime, float2 iResolution)
{

    pos+=0.5f;

    float4 data = texel(ch0, pos);
    particle P = getParticle(data, pos);

    //particle render
    float4 rho = to_float4_s(0.0f);
    range(i, -1, 1) range(j, -1, 1)
    {
        float2 ij = to_float2(i,j);
        float4 data = texel(ch0, pos + ij);
        particle P0 = getParticle(data, pos + ij);

        float2 x0 = P0.X; //update position
        //how much mass falls into this pixel
        rho += 1.0f*to_float4_f2f2(P.V, P.M)*G((pos - x0)/0.75f); 
    }
    
    rho.w  =P.M.y;
    fragColor = rho;


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel2
// Connect Image 'Previsualization: Buffer C' to iChannel1


// Fork of "My virtual slime molds" by michael0884. https://shadertoy.com/view/WtBcDG
// 2020-07-24 21:29:48


__DEVICE__ float3 hsv2rgb( in float3 c )
{
  float3 rgb = clamp( abs_f3(mod_f3(c.x*6.0f+to_float3(0.0f,4.0f,2.0f),6.0f)-3.0f)-1.0f, 0.0f, 1.0f );

  rgb = rgb*rgb*(3.0f-2.0f*rgb); // cubic smoothing  

  return c.z * _mix( to_float3_s(1.0f), rgb, c.y);
}

__DEVICE__ float3 mixN(float3 a, float3 b, float k)
{
    return sqrt_f3(_mix(a*a, b*b, clamp(k,0.0f,1.0f)));
}

__DEVICE__ float4 V(__TEXTURE2D__ ch1, float2 p, float2 R)
{
    return pixel(ch1, p);
}

__KERNEL__ void MindDumpBufferOverflowFuse(float4 col, float2 pos, float iTime, float2 iResolution)
{

    pos+=0.5f;
    //pos = R*0.5f + pos*0.1f;
    

    float4 data = texel(ch0, pos);
    particle P = getParticle(data, pos);
    
    //border render
    float3 Nb = bN(P.X, R, iTime);
    float bord = smoothstep(2.0f*border_h,border_h*0.5f,border(pos, R, iTime));
    
    float4 rho = V(ch1, pos, R);
    float3 dx = to_float3(-3.0f, 0.0f, 3.0f);
    float4 grad = -0.5f*to_float4_f2f2(swi2(V(ch1,pos + swi2(dx,z,y), R),z,w) - swi2(V(ch1,pos + swi2(dx,x,y), R),z,w),
                                       swi2(V(ch1,pos + swi2(dx,y,z), R),z,w) - swi2(V(ch1,pos + swi2(dx,y,x), R),z,w));
    float2 N = _powf(length(swi2(grad,x,z)),0.2f)*normalize(swi2(grad,x,z)+1e-5);
    float specular = _powf(_fmaxf(dot(N, Dir(1.4f)), 0.0f), 3.5f);
    float specularb = G(0.4f*(swi2(Nb,z,z) - border_h))*_powf(_fmaxf(dot(swi2(Nb,x,y), Dir(1.4f)), 0.0f), 3.0f);
    
    float a = _powf(smoothstep(fluid_rho*0.0f, fluid_rho*2.0f, rho.z),0.1f);
    float b = _expf(-0.7f*smoothstep(fluid_rho*1.0f, fluid_rho*7.5f, rho.z));
    float3 col0 = to_float3(1.0f, 0.17f, 0.17f);
    float3 col1 = to_float3(0.5f, 0.2f, 0.2f);

    // Output to screen
    float3 colxyz = swi3(col,x,y,z) + to_float3_s(0.1f*a);
           colxyz += 0.5f - 0.5f*cos_f3(01.0f*to_float3(0.21f*_sinf(iTime*0.521f),0.31f*_cosf(iTime*0.15f),0.1f*_cosf(iTime*(0.219f)))*rho.w);
    //swi3(col,x,y,z) += to_float3(1,1,1)*bord;
    
          colxyz = tanh_f3(4.0f*pow_f3(colxyz,to_float3_s(2.0f)));
    col = to_float4_aw(colxyz, 1.5f);


  SetFragmentShaderComputedColor(col);
}