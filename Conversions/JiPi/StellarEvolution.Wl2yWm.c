
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)
#define R iResolution

#define Bf(p) mod_f2f2(p,R)
#define Bi(p) to_int2_cfloat(mod_f2f2(p,R))
//#define texel(a, p) texelFetch(a, Bi(p), 0)
#define texel(a, p) texture(a, (make_float2(Bi(p))+0.5f)/R)

#define pixel(a, p) texture(a, (p)/R)
#define ch0 iChannel0
#define ch1 iChannel1
#define ch2 iChannel2
#define ch3 iChannel3

#define PI 3.14159265f

#define loop(i,x) for(int i = 0; i < x; i++)
#define range(i,a,b) for(int i = a; i <= b; i++)

#define dt 1.0f

#define border_h 5.0f
//float2 R;
//float4 Mouse;
//float time;

#define temporal_blurring 0.98f

#define mass 1.0f

#define fluid_rho 0.5f

__DEVICE__ float Pf(float rho)
{
    return 1.0f*rho*rho + 1.0f*_powf(rho, 10.0f); //gas + supernova
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
    float box = sdBox(mul_mat2_f2(Rot(0.0f*time),(p - R*to_float2(0.5f, 0.2f))) , R*to_float2(0.05f, 0.01f));
    float drain = -sdBox(p - R*to_float2(0.5f, 0.6f), R*to_float2(1.5f, 2.0f));
    return _fmaxf(drain,_fminf(bound, box));
}

#define h 1.0f
__DEVICE__ float3 bN(float2 p, float2 R, float time)
{
    float3 dx = to_float3(-h,0,h);
    float4 idx = to_float4(-1.0f/h, 0.0f, 1.0f/h, 0.25f);
    float3 r = swi3(idx,z,y,w)*border(p + swi2(dx,z,y),R,time)
             + swi3(idx,x,y,w)*border(p + swi2(dx,x,y),R,time)
             + swi3(idx,y,z,w)*border(p + swi2(dx,y,z),R,time)
             + swi3(idx,y,x,w)*border(p + swi2(dx,y,x),R,time);
    return to_float3_aw(normalize(swi2(r,x,y)), r.z + 1e-4);
}

__DEVICE__ uint pack(float2 x)
{
    x = 65534.0f*clamp(0.5f*x+0.5f, 0.0f, 1.0f);
    return uint(round(x.x)) + 65535u*uint(round(x.y));
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
    float2 X; //
    float2 V; //velocity
    float M; //mass
    float I; //angular velocity
};
    
__DEVICE__ particle getParticle(float4 data, float2 pos)
{
    particle P; 
    P.X = decode(data.x) + pos;
    P.V = decode(data.y);
    P.M = data.z;
    P.I = data.w;
    return P;
}

__DEVICE__ float4 saveParticle(particle P, float2 pos)
{
    P.X = clamp(P.X - pos, to_float2_s(-0.5f), to_float2_s(0.5f));
    return to_float4(encode(P.X), encode(P.V), P.M, P.I);
}

__DEVICE__ float3 hash32(float2 p)
{
    float3 p3 = fract_f3((swi3(p,x,y,x)) * to_float3(0.1031f, 0.1030f, 0.0973f));
    p3 += dot(p3, swi3(p3,y,x,z)+33.33f);
    return fract_f3((swi3(p3,x,x,y)+swi3(p3,y,z,z))*swi3(p3,z,y,x));
}

__DEVICE__ float G(float2 x)
{
    return _expf(-dot(x,x));
}

__DEVICE__ float G0(float2 x)
{
    return _expf(-length(x));
}

//diffusion amount
#define dif 0.92f
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
__DEVICE__ particle Reintegration(__TEXTURE2D__ ch, particle P, float2 pos, float2 R)
{
    //basically integral over all updated neighbor distributions
    //that fall inside of this pixel
    //this makes the tracking conservative
   
    //pass 1 - get center of mass
    range(i, -2, 2) range(j, -2, 2)
    {
        float2 tpos = pos + to_float2(i,j);
        float4 data = texel(ch, tpos);
       
        particle P0 = getParticle(data, tpos);
       
        P0.X += P0.V*dt; //integrate position

        float3 D = distribution(P0.X, pos, dif);
       
        //the deposited mass into this cell
        float m = P0.M*D.z;
        
        //add weighted by mass
        P.X += swi2(D,x,y)*m;
        
        //add mass
        P.M += m;
    }
    
    //normalization
    if(P.M != 0.0f)
    {
        P.X /= P.M;
    }
    
    //moment of inertia
    float I = 0.0f;
    //pass 2 - get velocity and angular momentum
    range(i, -1, 1) range(j, -1, 1)
    {
        float2 tpos = pos + to_float2(i,j);
        float4 data = texel(ch, tpos);
       
        particle P0 = getParticle(data, tpos);
       
        P0.X += P0.V*dt; //integrate position

        float3 D = distribution(P0.X, pos, dif);
       
        //the deposited mass into this cell
        float m = P0.M*D.z;
        
        float2 dx = P0.X - P.X;
      
        float W = P0.I; 
        //relative velocity of this part of the square
        float2 rel_V = P0.V + W*to_float2(dx.y, -dx.x);
        float v = length(P.V);
        rel_V /= (v > 2.0f)?v:1.;
        
        //add momentum
        P.V += rel_V*m;
        //add angular momentum
        P.I += (dx.x*P0.V.y - dx.y*P0.V.x)*m;
        //add moment of inertia
        I += dot(dx, dx)*m;
    }
   // I = _fmaxf(I, 0.1f);
    //normalization
    if(P.M != 0.0f)
    {
        P.V /= P.M; //get velocity
        P.I /= I;
    }
    
    return P;
}

//force calculation and integration
__DEVICE__ particle Simulation(__TEXTURE2D__ ch, __TEXTURE2D__ chG, particle P, float2 pos, float2 R, float4 Mouse, float time)
{
    //Compute the forces
    float2 F = to_float2_s(0.0f);
    float w = 0.0f;
    float3 avgV = to_float3_s(0.0f);
    //local gravity potential
    float lU = pixel(chG, P.X).w;
    range(i, -2, 2) range(j, -2, 2)
    {
        float2 tpos = pos + to_float2(i,j);
        float4 data = texel(ch, tpos);
        particle P0 = getParticle(data, tpos);
        float2 dx = P0.X - P.X;
        float avgP = 0.5f*P0.M*(Pf(P.M) + Pf(P0.M)); 
        //gas pressure
        F -= 0.5f*G(1.0f*dx)*avgP*dx;
        
        //neighbor gravity potential
        float rU = pixel(chG, P0.X).w;
        
        //gas gravity
        F -= 0.0015f*P.M*dx*clamp(lU - rU, -15.0f, 15.0f)*G(1.0f*dx);
        avgV += G(1.0f*dx)*to_float3_aw(P0.V,1.0f);
    }
    
    avgV /= avgV.z;


    if(Mouse.z > 0.0f)
    {
        float2 dm =(swi2(Mouse,x,y) - swi2(Mouse,z,w))/10.0f; 
        float d = distance_f2(swi2(Mouse,x,y), P.X)/9.0f;
        F += 0.003f*dm*_expf(-d*d);
        // P.M.y += 0.1f*_expf(-40.0f*d*d);
    }
    
    //integrate
    P.V += F*dt/P.M;

    //border 
    float3 N = bN(P.X,R,time);
    float vdotN = step(N.z, border_h)*dot(-1.0f*swi2(N,x,y), P.V);
    P.V += 0.5f*(swi2(N,x,y)*vdotN + swi2(N,x,y)*_fabs(vdotN));
    P.V += 2.0f*P.M*swi2(N,x,y)*_expf(-_fabs(N.z));
    if(N.z < 5.0f) 
    {
        //P.X = pos;
        P.V *= 0.0f;
        // P.M = 2.0f*fluid_rho;
    }
    //velocity limit
    float v = length(P.V);
    P.V /= (v > 1.0f)?v:1.;
    //angular momentum limit
    P.I = P.M*clamp(P.I/P.M, -0.5f, 0.5f);
    float PPPPPPPPPPPPPPPPPP;
    return P;
}



// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer B' to iChannel0



__KERNEL__ void StellarEvolutionFuse__Buffer_A(float4 U, float2 pos, float iTime, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0);
  
    pos+=0.5f;

    //R = iResolution; time = iTime; Mouse = iMouse;
    //int2 p = to_int2(pos);
        
    //particle velocity, mass and grid distributed density
    float2 F = to_float2_s(0.0f);
    
    float4 data = texel(ch0, pos); 
    
    particle P = {to_float2_s(0.0f),to_float2_s(0.0f),0.0f,0.0f};// = getParticle(data, pos);
       
    P = Reintegration(ch0, P, pos, R);
   
    //initial condition
    if(iFrame < 1 || Reset)
    {
        //random
        float3 rand = hash32(pos);
        if(rand.z < 1.0f) 
        {
            float2 dC = pos - R*0.5f;
            P.X = pos;
            P.V = 0.0f*(swi2(rand,x,y)-0.5f) + 0.5f*to_float2(dC.y/R.x, -dC.x/R.x);
            P.M = 0.005f*mass;
            P.I = 0.0f;
        }
        else
        {
            P.X = pos;
            P.V = to_float2_s(0.0f);
            P.M = 1e-6;
            P.I = 0.0f;
        }
    }
    
    U = saveParticle(P, pos);

  SetFragmentShaderComputedColor(U);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer C' to iChannel1


__KERNEL__ void StellarEvolutionFuse__Buffer_B(float4 U, float2 pos, float iTime, float2 iResolution, float4 iMouse)
{
    pos+=0.5f;
    //R = iResolution; time = iTime; Mouse = iMouse;
    //int2 p = to_int2(pos);
    
    float4 data = texel(ch0, pos); 
    
    particle P = getParticle(data, pos); 
    
   
    if(P.M != 0.0f) //not vacuum
    {
        P = Simulation(ch0, ch1, P, pos, R, iMouse, iTime);
    }
    
   
  /*
    if(length(P.X - R*to_float2(0.2f, 0.9f)) < 10.0f) 
    {
        P.X = pos;
        P.V = 0.5f*Dir(-PI*0.25f + 0.3f*_sinf(0.3f*time));
        P.M = _mix(P.M, to_float2(fluid_rho, 0.0f), 0.4f);
    }*/
    
    if(length(pos - R*to_float2(0.5f, 0.1f)) < 10.0f) 
    {
      // P.I = 0.2f;
    }
    
    U = saveParticle(P, pos);

  SetFragmentShaderComputedColor(U);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer C' to iChannel1


//density
//gravity solved by using a Jacobi-like solver with future estimation
#define Rad 4

__KERNEL__ void StellarEvolutionFuse__Buffer_C(float4 fragColor, float2 pos, float iTime, float2 iResolution)
{
    pos+=0.5f;
    //R = iResolution; time = iTime;
    //int2 p = to_int2(pos);

    float4 data = texel(ch0, pos);
    particle P = getParticle(data, pos);
    
    //particle render
    float4 rho = to_float4_s(0.0f);
    float U = 0.0f;
    float3 avgV = to_float3(0.00f, 0, 0.0000001f);
    float4 avgU = to_float4(0.00f, 0, 0, 0.00000001f);
    float4 dd = pixel(ch1, pos);
    range(i, -Rad, Rad) range(j, -Rad, Rad)
    {
        float2 ij = to_float2(i,j);
        float4 data = texel(ch0, pos + ij);
        particle P0 = getParticle(data, pos + ij);

        float2 x0 = P0.X; //update position
        //how much mass falls into this pixel
        float2 dx = pos - x0;
        rho += 1.0f*to_float4(P.V.x,P.V.y, P.M, P.I)*G(dx/1.5f); 
       
        //local potential
        U += P0.M/(length(dx)+0.1f);
        //local average velocity
        avgV += to_float3_aw(P0.V, 1.0f)*G(ij/1.0f)*P0.M; 
        
        //advected blurring, 
        //i.e. estimating where the past potential could have moved 
        float3 pU = swi3(pixel(ch1, pos + ij - swi2(dd,x,y)*dt),x,y,w);
        avgU += to_float4_aw(pU, 1.0f)*G(ij/3.0f); //blurring field
    }
    
    //spacio-temporally blurred velocity
    swi2S(rho,x,y, _mix(swi2(avgV,x,y)/avgV.z, swi2(avgU,x,y)/avgU.w, 0.95f));
                  
    //spacio-temporally blurred gravitational potential
    rho.w = U + temporal_blurring*avgU.z/avgU.w;
                  
    fragColor = rho;

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Preset: Keyboard' to iChannel3
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel2
// Connect Image 'Previsualization: Buffer C' to iChannel1


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

__DEVICE__ float4 V(float2 p, float2 R, __TEXTURE2D__ ch1)
{
    return pixel(ch1, p);
}

__KERNEL__ void StellarEvolutionFuse(float4 col, float2 pos, float iTime, float2 iResolution)
{
    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);
    CONNECT_COLOR1(RhoW, 0.2f, 0.5f, 1.0f, 1.0f);
    CONNECT_COLOR2(RhoZ, 1.0f, 0.7f, 0.5f, 1.0f);
    
    CONNECT_SLIDER0(RhoZMul, -20.0f, 20.0f, 5.0f);
  
    pos+=0.5f;

    //R = iResolution; time = iTime;
    //pos = R*0.5f + pos*0.1f;
    //int2 p = to_int2(pos);
    
    float4 data = texel(ch0, pos);
    particle P = getParticle(data, pos);
    
    //border render
    float3 Nb = bN(P.X, R, iTime);
    float bord = smoothstep(2.0f*border_h,border_h*0.5f,border(pos, R, iTime));
    
    float4 rho = V(pos,R,ch1);
    rho.z *= RhoZMul;//5.0f;
    //rho.w = _tanhf(rho.w);
    float3 dx = to_float3(-3.0f, 0.0f, 3.0f);
    float4 grad = -0.5f*to_float4_f2f2(swi2(V(pos + swi2(dx,z,y),R,ch1),z,w) - swi2(V(pos + swi2(dx,x,y),R,ch1),z,w),
                                       swi2(V(pos + swi2(dx,y,z),R,ch1),z,w) - swi2(V(pos + swi2(dx,y,x),R,ch1),z,w));
    float2 N = _powf(length(swi2(grad,x,z)),0.2f)*normalize(swi2(grad,x,z)+1e-5);
    float specular = _powf(_fmaxf(dot(N, Dir(1.4f)), 0.0f), 3.5f);
    float specularb = G(0.4f*(swi2(Nb,z,z) - border_h))*_powf(_fmaxf(dot(swi2(Nb,x,y), Dir(1.4f)), 0.0f), 3.0f);
    
    float a = _powf(smoothstep(fluid_rho*0.5f, fluid_rho*2.0f, rho.z),0.1f);
   
    //float3 colxyz = swi3(col,x,y,z) + 0.02f*to_float3(0.2f, 0.5f, 1.0f)*rho.w;
    float3 colxyz = swi3(col,x,y,z) + 0.02f*swi3(RhoW,x,y,z)*rho.w;
    //colxyz += to_float3(1.0f, 0.7f, 0.5f)*rho.z;
    colxyz += swi3(RhoZ,x,y,z)*rho.z;
    
    colxyz = tanh_f3(colxyz);
    
    col = to_float4_aw(colxyz*swi3(Color,x,y,z),Color.w); 


  SetFragmentShaderComputedColor(col);
}
