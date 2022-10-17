
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


vec2 R;
float4 Mouse;
float time;
int frame;

#define A(p) texelFetch(iChannel0, to_int2(mod_f(p,R)), 0)
#define B(p) texelFetch(iChannel1, to_int2(mod_f(p,R)), 0)

#define CH(c, p) texelFetch(c, to_int2(mod_f(p,R)), 0)

//loop in range
#define range(i,a,b) for(int i = a; i <= b; i++)

#define dt 0.8f
#define max_vel 1.0f
#define rad 2

#define diffusion 0.05f
#define gravity 0.00f

#define PI 3.14159265f

#define rest_rho 1.0f
#define eos_pressure 0.5f

//pressure equation of state
__DEVICE__ float pressure(float rho)
{
    return 1.2f*(rho); //gas
   // return eos_pressure*(_powf(rho/rest_rho,4.0f) - 1.0f); //Tait EOS (water)
}

__DEVICE__ void InitialConditions(inout float m, inout float2 *v, float2 P, float2 R)
{
    float2 dx = P - R*to_float2(0.3f, 0.5f);

    float d = smoothstep(R.y*0.5f, R.y*0.49f, length(dx));  
    m = 0.0f*d;

    *v = d*0.3f*normalize(to_float2(dx.y,-dx.x));
}

//KERNEL FUNCTIONS

float k0(float2 dx) //linear kernel
{
    float2 k = _fmaxf(1.0f - abs_f2(dx), to_float2_s(0.0f));
    return k.x*k.y;
}

float3 K0(float2 dx) //linear kernel with the center of mass
{
    float2 k = _fmaxf(1.0f - abs_f2(dx), to_float2_s(0.0f));
    return to_float3_aw(dx*0.5f, k.x*k.y);
}

float k1(float2 dx) //quadratic kernel
{
    float2 f = _fmaxf(1.5f - abs_f2(dx), to_float2_s(0.0f));
    float2 k = _fminf(max(0.75f - dx*dx, 0.5f), 0.5f*f*f);
    return k.x*k.y;
}

//box size enstimator
__DEVICE__ float2 destimator(float2 dx)
{
    return diffusion*dt+clamp(1.0f - 2.0f*abs_f2(dx), 0.001f, 1.0f);
}

//box overlap with center of mass
__DEVICE__ float3 overlap(float2 dx, float2 box)
{
    float2 min0 = _fmaxf(dx - box*0.5f, to_float2_s(-0.5f)); 
    float2 max0 = _fminf(dx + box*0.5f, to_float2_s(0.5f)); 
    float2 size = _fmaxf(max0 - min0, to_float2_s(0.0f)); 
    return to_float3_aw(0.5f*(max0 + min0), size.x*size.y/(box.x*box.y));
}

//boundary
#define border_h 3.
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

__DEVICE__ float border(float2 p, float2 R)
{
    float bound = -border_h-sdBox(p - R*0.5f, R*to_float2(0.5f, 0.5f)); 

    return bound;
}

#define h 5.
__DEVICE__ float3 bN(float2 p, float2 R)
{
    float3 dx = to_float3(-h,0,h);
    float4 idx = to_float4(-1.0f/h, 0.0f, 1.0f/h, 0.25f);
    float3 r = swi3(idx,z,y,w)*border(p + swi2(dx,z,y),R)
           + swi3(idx,x,y,w)*border(p + swi2(dx,x,y),R)
           + swi3(idx,y,z,w)*border(p + swi2(dx,y,z),R)
           + swi3(idx,y,x,w)*border(p + swi2(dx,y,x),R);
    return to_float3(normalize(swi2(r,x,y)), r.z + 1e-4);
}

//DATA PACKING

__DEVICE__ uint pack(float2 _x)
{
    _x = 65534.0f*clamp(0.5f*_x+0.5f, 0.0f, 1.0f);
    return (uint)(round(_x.x)) + 65535u*(uint)(round(_x.y));
}

__DEVICE__ float2 unpack(uint a)
{
    float2 _x = to_float2(a%65535u, a/65535u);
    return clamp(_x/65534.0f, 0.0f,1.0f)*2.0f - 1.0f;
}

__DEVICE__ float2 decode(float x)
{
    uint X = floatBitsToUint(x);
    return unpack(X)*12.0f; 
}

__DEVICE__ float encode(float2 x)
{
    uint X = pack(x/12.0f);
    return uintBitsToFloat(X); 
}

//particle to grid
__DEVICE__ float4 P2G(__TEXTURE2D__ a, __TEXTURE2D__ b, float2 P, float2 R)
{
    float2 x = to_float2_s(0.0f);
    float2 v = to_float2_s(0.0f);
    float m = 0.0f;
    float rho = 0.0f;
    
    range(i, -1, 1) range(j, -1, 1)
    {
        //load data
        float2 di = to_float2(i,j);
        float4 data0 = CH(a, P + di);
        float4 data1 = CH(b, P + di);
        
        //unpack data
        float m0 = data0.y;
        float r0 = data0.w;
        float2 x0 = decode(data0.x);
        float2 v0 = decode(data1.x);
        mat2 B0 = to_mat2_f2(decode(data1.y),decode(data1.z)); //velocity gradient
        
        //estimate the shape of the distribution
        float2 box = destimator(x0);
        
        //predicted density using the affine matrix, thanks to Grant Kot for this idea
        //float rho = ;//;
        mat2 stress = -to_mat2(clamp(pressure(data1.w/determinant(to_mat2_f(1.0f) + 4.0f*B0*dt)), -12.0f, 12.0f));
        
        //update particle position
        x0 = x0 + di + v0*dt;
    
        //find cell contribution
        float3 o = overlap(x0, box);
         
        //update distribution
        x += m0*swi2(o,x,y)*o.z;
        m += m0*o.z;
        
        //find grid node contribution
        float w = k1(x0);
        
        //distribute momentum onto grid
        v += (v0 + 4.0f*mul_mat2_f2(B0,x0) + dt*mul_mat2_f2(stress,x0))*w*m0;
        rho += m0*w;
    }
    
    
    //normalize
    if(rho > 0.0f) v /= rho;
    if(m > 0.0f) x /= m;
    
   // m = _mix(m, 0.3f, dt*0.002f);
   
    if(length(P - R*to_float2(0.8f + 0.05f*_sinf(2.4f*time), 0.9f)) < 10.0f) 
    {
        x = to_float2_s(0.0f);
        v =8.8f*Dir(-PI*0.25f - PI*0.5f + 0.45f*_sinf(3.4f*time));
        m = _mix(m, rest_rho, 0.4f*dt);
    }

    if(length(P - R*to_float2(0.2f+ 0.05f*_sinf(2.4f*time), 0.9f)) < 10.0f) 
    {
       x = to_float2_s(0.0f);
        v = 8.8f*Dir(-PI*0.25f + 0.45f*_sinf(3.4f*time));
        m = _mix(m, rest_rho, 0.4f*dt);
    }
    
    //initial conditions
    if(frame < 1)
    {
        InitialConditions(m, v, P, R);
    }
    
    v = (length(v)>max_vel)?normalize(v)*max_vel:v;
    
    return to_float4(encode(x), m, encode(v), rho);
}

//grid to particle
__DEVICE__ float4 G2P(sampler2D a, sampler2D b, float2 P)
{
    float2 V = to_float2_s(0.0f);
    mat2 B = to_mat2_f(0.0f);
    
    float4 data = CH(a, P);
    float m = data.y;
    float2 _x = decode(data.x);
    float mass = 0.0f;
    
    range(i, -1, 1) range(j, -1, 1)
    {
        //load data
        float2 di = to_float2(i,j);
        float4 data0 = CH(a, P + di);
        
        //unpack data
        float m0 = data0.y;
        float rho = data0.w;
        float2 x0 = decode(data0.x);
        float2 dx = _x - di;
        float2 v0 = decode(data0.z);
        
        //find grid node contribution
        float w = k1(dx);
        
        //distribute velocities/forces to particles
        V += v0*w;
        float2 nv0 =dot(v0, normalize(dx))*normalize(dx);
        //v0 = v0 - nv0 + clamp(nv0, -0.01f, 0.01f);//remove divergent component, makes it unstable otherwise
        B = add_mat2_mat2(B, to_mat2(v0*dx.x,v0*dx.y)*w);
    }
    
    float divergence = 0.5f*(B[0][0] + B[1][1]);
    
    B -= 1.7f*to_mat2(divergence);
    
    //B = B - B*dt*smoothstep(1.0f, 0.8f, m);
    
    //gravity
    V += -gravity*to_float2(_sinf(0.0f),_cosf(0.0f))*dt;
    
    //push fluid
    V += to_float2(0.0f, 8.0f)*_expf(-0.02f*_powf(distance(P, R*to_float2(0.6f,0.2f)), 2.0f));
    
    if(Mouse.z > 0.0f)
    {
        float2 dx = (swi2(Mouse,x,y) - P); 
        V += 0.001f*_expf(-0.03f*length(dx))*dt*dx; 
    }
    
    //border 
    float3 N = bN(P + _x,R);
    float vdotN = step(N.z, border_h)*dot(-swi2(N,x,y), V);
    //V *= 1.0f - 0.1f*_expf(-N.z);
    V += (0.0f + 1.5f*_fmaxf(vdotN, 0.0f))*swi2(N,x,y)*step(_fabs(N.z), border_h)*_expf(-N.z);
   // V = (length(V)>max_vel)?normalize(V)*max_vel:V;
   
    //estimate density
    _x += V*dt;
   
    range(i, -1, 1) range(j, -1, 1)
    {
        //load data
        float2 di = to_float2(i,j);
        float4 data0 = CH(a, P + di);
        float2 dx = _x - di;
        //unpack data
        float m0 = data0.y;
        
        mass += data0.w*k1(dx);
    }
   
    return to_float4(encode(V), encode(clamp(B[0], -2.5f,2.5f)), encode(clamp(B[1], -2.5f,2.5f)), mass);
}


// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer C' to iChannel0
// Connect Buffer A 'Previsualization: Buffer D' to iChannel1


//Advect mass distributions and distrubute the momentum and density to grid

//Advect + P2G

__KERNEL__ void GasChamberFuse__Buffer_A(float4 C, float2 P, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{

    R = iResolution; time = iTime; Mouse = iMouse; frame = iFrame;
    P = _floor(P);
   
    C = P2G(iChannel0,iChannel1,P);


  SetFragmentShaderComputedColor(C);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1


//Distribute grid velocities to the mass distributions

//G2P

__KERNEL__ void GasChamberFuse__Buffer_B(float4 C, float2 P, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{

    R = iResolution; time = iTime; Mouse = iMouse; frame = iFrame;
    P = _floor(P);
    C = G2P(iChannel0, iChannel1, P);


  SetFragmentShaderComputedColor(C);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1


//Advect + P2G

__KERNEL__ void GasChamberFuse__Buffer_C(float4 C, float2 P, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{

    R = iResolution; time = iTime; Mouse = iMouse; frame = iFrame;
    P = _floor(P);
   
    C = P2G(iChannel0,iChannel1,P);


  SetFragmentShaderComputedColor(C);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer B' to iChannel1
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


//G2P

__KERNEL__ void GasChamberFuse__Buffer_D(float4 C, float2 P, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{

    R = iResolution; time = iTime; Mouse = iMouse; frame = iFrame;
    P = _floor(P);
    C = G2P(iChannel0, iChannel1, P);


  SetFragmentShaderComputedColor(C);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1


//MIT License
//Copyright 2021 Mykhailo Moroz


__DEVICE__ float3 hsv2rgb( in float3 c )
{
    float3 rgb = clamp( _fabs(mod_f(c.x*6.0f+to_float3(0.0f,4.0f,2.0f),6.0f)-3.0f)-1.0f, 0.0f, 1.0f );
  rgb = rgb*rgb*(3.0f-2.0f*rgb);   
  return c.z * _mix( to_float3_s(1.0f), rgb, c.y);
}

__KERNEL__ void GasChamberFuse(float4 C, float2 P, float iTime, float2 iResolution, float4 iMouse)
{

    R = iResolution; time = iTime; Mouse = iMouse;
    P = _floor(P);
    
    if(iMouse.z < 0.0f)
    {
       // P = swi2(iMouse,x,y) + 300.0f*(P/R.x - 0.5f); 
    }
    
    float m = 0.0f;
    float rho = 0.0f;
    float2 v = to_float2_s(0.0f);
    float curl = 0.0f;
    range(i, -2, 2) range(j, -2, 2)
    {
        //load data
        float2 di = to_float2(i,j);
        float4 data0 = A(P + di);
        float4 data1 = B(P + di);
        
        //unpack data
        float m0 = data0.y;
        float2 x0 = decode(data0.x) - fract(P);
        float2 v0 = decode(data1.x);
        mat2 B0 = mat2(decode(data1.y),decode(data1.z)); //velocity gradient
        //update particle position
        x0 = x0 + di + v0*dt;
        
        //find cell contribution
        float3 o = overlap(x0, to_float2_s(1.0f));
        
        m += m0*o.z;
        
        float w = k1(x0);
        
        v += (v0 + 4.0f*B0*x0)*w;
        curl += w*(x0.x*v0.y - x0.y*v0.x); 
        rho += m0*w;
    }
    
    float arg = 0.5f*(_atan2f(v.y, v.x)/PI + 1.0f);
    float d = 8.5f*_sqrtf(_fabs(curl))*sign(curl);
    float3 fluid =step(-d, 0.0f)*_tanhf(d)*to_float3(1.000f,0.180f,0.384f) - step(d, 0.0f)*_tanhf(d)*to_float3(0.122f,0.459f,1.000f);
    
    C = _mix(to_float4_s(1.0f), to_float4_aw(fluid,1.0f), smoothstep(0.0f, 1.0f, rho));


  SetFragmentShaderComputedColor(C);
}