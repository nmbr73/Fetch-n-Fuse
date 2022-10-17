
!!!! unpackHalf2x16 und packHalf2x16(d) nicht konvertierbar!!!!!!!!!!!!!

// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


#define PI 3.14159265f
#define TWO_PI 6.28318530718f

#define TURBULENCE_SCALE 0.1f
#define VORTICITY_SCALE 0.005f
#define VISCOSITY_SCALE 0.01f
#define MAX_CONSERVATIVE_DISTANCE 4.0f

#define MULTISCALE_KERNEL_POWER 3.0f
#define MULTISCALE_KERNEL_STEPS 1

#define ENABLE_BOUNDS
#define USE_TANH
#define ENABLE_AUTO_MOUSE

//#define VIRTUAL_PARTICLE_SIZE _mix(0.4f, 0.01f, smoothstep(0.0f, 3.0f, mass * length(v)))
#define VIRTUAL_PARTICLE_SIZE _mix(1.0f, 0.15f, smoothstep(0.0f, 3.0f, length(v)))
//#define VIRTUAL_PARTICLE_SIZE 0.15

#define INIT_MASS 0.01f
#define FRAME_DIVIDER 1
#define FRAME_MOD(x) ((iFrame % FRAME_DIVIDER)==0)


#define R iResolution

//float2 R;
int F;
float2 uv;
float2 texel;

float4 bounds;

//internal RNG state 
uint4 s0; 

__DEVICE__ void initialize(inout float2 *p, int frame, float3 res)
{
    uv = p / swi2(res,x,y);
    p = _floor(p);
    R = swi2(res,x,y);
    texel = 1.0f/R;
    F = frame;
    
    bounds = to_float4_aw(2.0f*texel,1.0f-2.0f*texel);

    //white noise seed
    s0 = uto_float4_aw(p, (uint)(frame), (uint)(p.x) + (uint)(p.y));
}

// https://www.pcg-random.org/
__DEVICE__ uint4 pcg4d(inout uvec4 v)
{
    v = v * 1664525u + 1013904223u;
    v.x += v.y*v.w; v.y += v.z*v.x; v.z += v.x*v.y; v.w += v.y*v.z;
    v = v ^ (v>>16u);
    v.x += v.y*v.w; v.y += v.z*v.x; v.z += v.x*v.y; v.w += v.y*v.z;
    return v;
}

__DEVICE__ float rand(){ return float(pcg4d(s0).x)/float(0xffffffffu); }
__DEVICE__ float2 rand2(){ return to_float2(swi2(pcg4d(s0),x,y))/(float)(0xffffffffu); }
__DEVICE__ float3 rand3(){ return to_float3(swi3(pcg4d(s0),x,y,z))/float(0xffffffffu); }
__DEVICE__ float4 rand4(){ return to_float4(pcg4d(s0))/float(0xffffffffu); }


#define _PH_COMP xy

__DEVICE__ float2 normz(float2 x) {
  return length(x) < 1e-6 ? to_float2_s(0) : normalize(x);
}

__DEVICE__ float4 normz(float4 x) {
  return length(x) < 1e-6 ? to_float4_s(0) : normalize(x);
}

#define pack2x16(d) uintBitsToFloat(packHalf2x16(d))
#define unpack2x16(d) unpackHalf2x16(floatBitsToUint(d))

__DEVICE__ bool reset(sampler2D ch) {
    return texture(ch, to_float2(32.5f/256.0f, 0.5f) ).x > 0.5f;
}


__DEVICE__ float G1V(float dnv, float k){
    return 1.0f/(dnv*(1.0f-k)+k);
}

__DEVICE__ float ggx(float3 n, float3 v, float3 l, float rough, float f0){
    float alpha = rough*rough;
    float3 h = normalize(v+l);
    float dnl = clamp(dot(n,l), 0.0f, 1.0f);
    float dnv = clamp(dot(n,v), 0.0f, 1.0f);
    float dnh = clamp(dot(n,h), 0.0f, 1.0f);
    float dlh = clamp(dot(l,h), 0.0f, 1.0f);
    float f, d, vis;
    float asqr = alpha*alpha;
    const float pi = 3.14159f;
    float den = dnh*dnh*(asqr-1.0f)+1.0f;
    d = asqr/(pi * den * den);
    dlh = _powf(1.0f-dlh, 5.0f);
    f = f0 + (1.0f-f0)*dlh;
    float k = alpha/1.0f;
    vis = G1V(dnl, k)*G1V(dnv, k);
    float spec = dnl * d * f * vis;
    return spec;
}



struct Vec4Neighborhood {
    float4 c; float4 n; float4 e; float4 w; float4 s; float4 ne; float4 nw; float4 sw; float4 se;
};

__DEVICE__ float4 GetCenter(Vec4Neighborhood n) {
    return n.c;
}

struct Vec4Kernel {
    float4 c; float4 n; float4 e; float4 w; float4 s; float4 ne; float4 nw; float4 sw; float4 se;
};

__DEVICE__ float4 ApplyVec4KernelVector(Vec4Neighborhood n, Vec4Kernel k) {
    return n.c*k.c + n.n*k.n + n.e*k.e + n.w*k.w + n.s*k.s + n.ne*k.ne + n.nw*k.nw + n.sw*k.sw + n.se*k.se;
}

__DEVICE__ float ApplyVec4KernelScalar(Vec4Neighborhood n, Vec4Kernel k) {
    return dot(n.c,k.c) + dot(n.n,k.n) + dot(n.e,k.e) + dot(n.w,k.w) + dot(n.s,k.s) 
         + dot(n.ne,k.ne) + dot(n.nw,k.nw) + dot(n.sw,k.sw) + dot(n.se,k.se);
}

__DEVICE__ float4 ApplyVec4KernelPermutationVector(Vec4Neighborhood n, Vec4Kernel k, int v) {
    return n.c[v]*k.c + n.n[v]*k.n + n.e[v]*k.e + n.w[v]*k.w + n.s[v]*k.s 
         + n.ne[v]*k.ne + n.nw[v]*k.nw + n.sw[v]*k.sw + n.se[v]*k.se;
}

__DEVICE__ float4 ApplyVec4KernelPermutationVectorAbs(Vec4Neighborhood n, Vec4Kernel k, int v) {
    return _fabs(n.c[v])*k.c + _fabs(n.n[v])*k.n + _fabs(n.e[v])*k.e + _fabs(n.w[v])*k.w + _fabs(n.s[v])*k.s 
         + _fabs(n.ne[v])*k.ne + _fabs(n.nw[v])*k.nw + _fabs(n.sw[v])*k.sw + _fabs(n.se[v])*k.se;
}

__DEVICE__ float ApplyVec4KernelPermutationScalar(Vec4Neighborhood n, Vec4Kernel k, int v) {
    return dot(to_float4_aw(n.c[v]),k.c) + dot(to_float4_aw(n.n[v]),k.n) + dot(to_float4_aw(n.e[v]),k.e) 
         + dot(to_float4_aw(n.w[v]),k.w) + dot(to_float4_aw(n.s[v]),k.s) + dot(to_float4_aw(n.ne[v]),k.ne) 
         + dot(to_float4_aw(n.nw[v]),k.nw) + dot(to_float4_aw(n.sw[v]),k.sw) + dot(to_float4_aw(n.se[v]),k.se);
}


__DEVICE__ bool BoundsCheck(float2 ouv) {
    #ifdef ENABLE_BOUNDS
        return (ouv.x < bounds.x || ouv.y < bounds.y || ouv.x > bounds.z || ouv.y > bounds.w);
    #else
        return false;
    #endif
}

__DEVICE__ float2 BoundsClamp(float2 ouv) {
    return clamp(ouv, swi2(bounds,x,y), swi2(bounds,z,w));
}

__DEVICE__ float4 BoundedTex(sampler2D ch, float2 p) {
    if (BoundsCheck(p)) {
        return to_float4(0,0,0,0);
    } else {
        return textureLod(ch, p, 0.0f);
    }
}

__DEVICE__ float4 BoundedTex(sampler2D ch, float2 off, int x, int y) {
    float2 ouv = uv + texel * (off + to_float2(x,y));
    return BoundedTex(ch, ouv);
}

__DEVICE__ float4 BoundedTex(sampler2D ch, float2 off, float x, float y) {
    float2 ouv = uv + texel * (off + to_float2(x,y));
    return BoundedTex(ch, ouv);
}

#define U(name,x,y) float3 name = BoundedTex(ch, to_float2(0), x, y)
#define S(name,x,y) name = BoundedTex(ch, to_float2(0), x, y)
#define COM(name,x,y) name = BoundedTex(ch_com, to_float2(0), x, y).zw
#define SO(name,x,y) name = BoundedTex(ch, off, float(x), float(y))
#define SR(name,x,y) name = BoundedTex(ch, off - RK4(ch, uv + texel*(off+to_float2(x,y)), 1.0f).xy, float(x), float(y))
#define K(name,x,y,z,w) name = to_float4(x,y,z,w)
#define KV(name,x) name = to_float4(x)

__DEVICE__ float2 RK4(sampler2D ch, float2 p, float h){
    float2 k1 = BoundedTex(ch,p).xy;
    float2 k2 = BoundedTex(ch,p - texel*0.5f*h*k1).xy;
    float2 k3 = BoundedTex(ch,p - texel*0.5f*h*k2).xy;
    float2 k4 = BoundedTex(ch,p - texel*h*k3).xy;
    return h/3.0f*(0.5f*k1+k2+k3+0.5f*k4);
}

__DEVICE__ float2 RK4(sampler2D ch, float h){
    return RK4(ch, uv, h);
}

Vec4Neighborhood GetVec4Neighborhood(sampler2D ch) {
    Vec4Neighborhood n;
    S(n.c,0,0); S(n.n,0,1); S(n.e,1,0); S(n.s,0,-1); S(n.w,-1,0);
    S(n.nw,-1,1); S(n.sw,-1,-1); S(n.ne,1,1); S(n.se,1,-1);
    return n;
}

Vec4Neighborhood GetVec4Neighborhood(sampler2D ch, float2 off) {
    Vec4Neighborhood n;
    SO(n.c,0,0); SO(n.n,0,1); SO(n.e,1,0); SO(n.s,0,-1); SO(n.w,-1,0);
    SO(n.nw,-1,1); SO(n.sw,-1,-1); SO(n.ne,1,1); SO(n.se,1,-1);
    return n;
}

Vec4Neighborhood GetVec4NeighborhoodRK4(sampler2D ch) {
    Vec4Neighborhood n;
    float2 off = to_float2(0);
    SR(n.c,0,0); SR(n.n,0,1); SR(n.e,1,0); SR(n.s,0,-1); SR(n.w,-1,0);
    SR(n.nw,-1,1); SR(n.sw,-1,-1); SR(n.ne,1,1); SR(n.se,1,-1);
    return n;
}

Vec4Neighborhood GetStridedVec4Neighborhood(sampler2D ch, float stride) {
    Vec4Neighborhood n;
    float2 off = to_float2(0);
    float s = stride;
    SO(n.c,0,0); SO(n.n,0,s); SO(n.e,s,0); SO(n.s,0,-s); SO(n.w,-s,0);
    SO(n.nw,-s,1); SO(n.sw,-s,-s); SO(n.ne,s,s); SO(n.se,s,-s);
    return n;
}

Vec4Neighborhood GetStridedVec4NeighborhoodRK4(sampler2D ch, float stride) {
    Vec4Neighborhood n;
    float2 off = to_float2(0);
    float s = stride;
    SR(n.c,0,0); SR(n.n,0,s); SR(n.e,s,0); SR(n.s,0,-s); SR(n.w,-s,0);
    SR(n.nw,-s,1); SR(n.sw,-s,-s); SR(n.ne,s,s); SR(n.se,s,-s);
    return n;
}


Vec4Neighborhood GetVec4NeighborhoodRK4(sampler2D ch, float2 off) {
    Vec4Neighborhood n;
    SR(n.c,0,0); SR(n.n,0,1); SR(n.e,1,0); SR(n.s,0,-1); SR(n.w,-1,0);
    SR(n.nw,-1,1); SR(n.sw,-1,-1); SR(n.ne,1,1); SR(n.se,1,-1);
    return n;
}

Vec4Kernel Vec4NeighborhoodToVec4KernelTransform(Vec4Neighborhood n, Vec4Kernel k) {
    Vec4Kernel k2;
    KV(k2.nw,n.nw*k.nw); KV(k2.n,n.n*k.n); KV(k2.ne,n.ne*k.ne);
    KV(k2.w,n.w*k.w); KV(k2.c,n.c*k.c); KV(k2.e,n.e*k.e);
    KV(k2.sw,n.sw*k.sw); KV(k2.s,n.s*k.s); KV(k2.se,n.se*k.se);
    return k2;
}

Vec4Kernel GetCurlKernel() {
    const float D = 0.5f;
    Vec4Kernel k;
    K(k.c, 0, 0, 0, 0);
    K(k.n, 1, 0, 0, 0);
    K(k.s,-1, 0, 0, 0);
    K(k.e, 0,-1, 0, 0);
    K(k.w, 0, 1, 0, 0);
    K(k.nw, D, D, 0, 0);
    K(k.ne, D,-D, 0, 0);
    K(k.sw,-D, D, 0, 0);
    K(k.se,-D,-D, 0, 0);
    return k;
}


Vec4Kernel GetDivKernel() {
    const float D = 0.5f;
    Vec4Kernel k;
    K(k.c, 0, 0, 0, 0);
    K(k.n, 0,-1, 0, 0);
    K(k.s, 0, 1, 0, 0);
    K(k.e,-1, 0, 0, 0);
    K(k.w, 1, 0, 0, 0);
    K(k.nw, D,-D, 0, 0);
    K(k.ne,-D,-D, 0, 0);
    K(k.sw, D, D, 0, 0);
    K(k.se,-D, D, 0, 0);
    return k;
}

__DEVICE__ float2 Turbulence(Vec4Neighborhood n) {
    return  - 4.0f * n.swi2(c,x,y) 
            + 2.0f * to_float2(n.n.x + n.s.x, n.e.y + n.w.y)
            + (n.se - n.ne - n.sw + n.nw).yx;
}


Vec4Kernel GetScalarKernel(float center, float edge, float vertex) {
    Vec4Kernel k;
    KV(k.c, center);
    KV(k.n, edge);
    KV(k.s, edge);
    KV(k.e, edge);
    KV(k.w, edge);
    KV(k.nw, vertex);
    KV(k.ne, vertex);
    KV(k.sw, vertex);
    KV(k.se, vertex);
    return k;
}

Vec4Kernel GetGaussianKernel() {
    const float G0 = 0.25f;
    const float G1 = 0.125f;
    const float G2 = 0.0625f;
    return GetScalarKernel(G0, G1, G2);
}

Vec4Kernel GetNeighborAvgKernel() {
    const float G0 = 0.0f;
    const float G1 = 1.0f/6.0f;
    const float G2 = 1.0f/12.0f;
    return GetScalarKernel(G0, G1, G2);
}

Vec4Kernel GetNeighborAvgVonNeumannKernel() {
    const float G0 = 0.0f;
    const float G1 = 0.25f;
    const float G2 = 0.0f;
    return GetScalarKernel(G0, G1, G2);
}

Vec4Kernel GetLaplacianKernel() {
    const float L0 = -20.0f/6.0f;
    const float L1 = 4.0f/6.0f;
    const float L2 = 1.0f/6.0f;
    return GetScalarKernel(L0, L1, L2);
}

__DEVICE__ float4 Advect(sampler2D ch, float timestep) {
    return textureLod(ch,fract(uv - texel*RK4(ch,timestep)), 0.0f);
}

__DEVICE__ float2 Rotate(float2 v, float r) {
    float s = _sinf(r);
    float c = _cosf(r);
    return mat2(c, -s, s, c) * v;
}

__DEVICE__ float2 SoftBound(float2 x, float p) {
    float2 soft = normz(x) * _powf(dot(x,x),1.5f);
    return x - p * soft;
}

__DEVICE__ float SoftBound(float x, float p) {
    float soft = sign(x) * _powf(_fabs(x),3.0f);
    return x - p * soft;
}

__DEVICE__ float2 SoftBound(float2 x, float s, float p) {
    float2 soft = normz(x) * _powf(dot(s*x,s*x),1.5f);
    return x - p * soft;
}

__DEVICE__ float SoftBound(float x, float s, float p) {
    float soft = sign(x) * _powf(_fabs(s*x),3.0f);
    return x - p * soft;
}

__DEVICE__ float2 HardBound(float2 x, float p) {
    return _fmaxf(min((length(x) / p) > 1.0f ? (p * normz(x)) : x, p), -p);
}

__DEVICE__ float HardBound(float x, float p) {
    return _fmaxf(min(x, p), -p);
}

__DEVICE__ float2 Vorticity(Vec4Neighborhood n, float curl) {
    return  -curl * normz(ApplyVec4KernelPermutationVectorAbs(n, GetCurlKernel(), 3).xy);
}

__DEVICE__ float2 Delta(Vec4Neighborhood n, int channel) {
    return ApplyVec4KernelPermutationVector(n, GetDivKernel(), channel).xy;
}

__DEVICE__ float4 getAutoMouse() {
    int stage = (F/120)%4;
    float4 auto = to_float4(0);
    switch(stage) {
        case 0:
            auto = to_float4(0.2f, 0.5f, 1.0f, 0.0f); break;
        case 1:
            auto = to_float4(0.5f, 0.2f, 0.0f, 1.0f); break;
        case 2:
            auto = to_float4(0.8f, 0.5f, -1.0f, 0.0f); break;
        case 3:
            auto = to_float4(0.5f, 0.8f, 0.0f, -1.0f); break;
    }
    return auto * to_float4(R,1,1);
}

__DEVICE__ float4 MouseSpace(float4 mouse, float4 phase, float2 p, float width, float strength) {
    if (mouse.z > 0.0f) {
        swi2(phase,x,y) += strength * _expf(-length(p-swi2(mouse,x,y)) / width) * normz(swi2(mouse,x,y)-_fabs(swi2(mouse,z,w)));
    } else {
        #ifdef ENABLE_AUTO_MOUSE
            float4 auto = getAutoMouse();
            swi2(phase,x,y) += strength * _expf(-length(p-swi2(auto,x,y)) / width) * swi2(auto,z,w);
        #endif
    }
    return phase;
}

__DEVICE__ float4 MouseMass(float4 mouse, float4 phase, float2 p, float width, float strength) {
    if (mouse.z > 0.0f) {
        phase.z += strength * _expf(-length(p-swi2(mouse,x,y)) / width);
    } else {
        #ifdef ENABLE_AUTO_MOUSE
            float4 auto = getAutoMouse();
            phase.z += strength * _expf(-length(p-swi2(auto,x,y)) / width);
        #endif
    }
    return phase;
}

#undef T
#undef V


__DEVICE__ float erf(float x) {
    #ifdef USE_TANH
        return _tanhf(1.22848f*x);
    #elif USE_SMOOTHSTEP
        return -1.0f+2.0f*smoothstep(-1.657f,1.657f,sign(x)*_powf(_fabs(x),0.85715f));
    #else
    if (x > 9.0f) {
        return 1.0f;
    } else if (x < -9.0f) {
        return -1.0f;
    } else if (_fabs(x) < 1e-9) {
        return x;
    }
    const float p = 0.3275911f;
    const float a1 = 0.254829592f;
    const float a2 = -0.284496736f;
    const float a3 = 1.421413741f;
    const float a4 = -1.453152027f;
    const float a5 = 1.061405429f;
    float sx = sign(x);
    x *= sx;
    float t = 1.0f / (1.0f + p * x);
    return clamp(sx * (1.0f - (a1*t + a2*t*t + a3*t*t*t + a4*t*t*t*t + a5*t*t*t*t*t) * _expf(-x*x)),-1.0f,1.0f);
    #endif
}


__DEVICE__ float safeexp(float x) {
    return _expf(clamp(x, -87.0f, 87.0f));
}

//https://www.wolframalpha.com/input/?i=%28%28sqrt%28k%29+e%5E%28-%28a%5E2+%2B+b%5E2%29%2Fk%29+%28e%5E%28a%5E2%2Fk%29+-+e%5E%28b%5E2%2Fk%29%29+%28erf%28c%2Fsqrt%28k%29%29+-+erf%28d%2Fsqrt%28k%29%29%29%29%2F%284+sqrt%28%CF%80%29%29%29++%2F+%281%2F4+%28erf%28a%2Fsqrt%28k%29%29+-+erf%28b%2Fsqrt%28k%29%29%29+%28erf%28c%2Fsqrt%28k%29%29+-+erf%28d%2Fsqrt%28k%29%29%29%29
__DEVICE__ float center_of_mass(float2 b, float K) {
    float sqK = _sqrtf(K);
    float sqP = _sqrtf(PI);
    float erax = erf(b.x/sqK);
    float erbx = erf(b.y/sqK);
    float exabx = safeexp((b.x*b.x + b.y*b.y)/K);
    float exax = safeexp(-(b.x*b.x)/K);
    float exbx = safeexp(-(b.y*b.y)/K);
    
    //return clamp((sqK * (exax - exbx)) / (sqP * (erbx - erax)),-4.0f,4.0f);
    return HardBound((sqK * (exax - exbx)) / (sqP * (erbx - erax)),16.0f);
}

__DEVICE__ float3 distribution(float2 x, float2 p, float K)
{
    float2 omin = p - 0.5f;
    float2 omax = p + 0.5f; 
    
    float sqK = _sqrtf(K);
    float sqP = _sqrtf(PI);
    
    //https://www.wolframalpha.com/input/?i=integral+of+%28integral+of+exp%28-%28x%5E2%2By%5E2%29%2Fk%29+with+respect+to+x+from+a+to+b%29+with+respect+to+y+from+c+to+d
    float masst = 0.25f *
                    ((erf((omin.x - x.x)/sqK) - erf((omax.x - x.x)/sqK)) * 
                    (erf((omin.y - x.y)/sqK) - erf((omax.y - x.y)/sqK)));
    
    float2 com2 = x-p+to_float2(center_of_mass(to_float2(omin.x - x.x,omax.x - x.x), K), center_of_mass(to_float2(omin.y - x.y,omax.y - x.y), K));
    return to_float3_aw(com2, masst);
}

#define range(i, r) for(int i = -r; i < r; i++)




__DEVICE__ float2 com(float2 p, sampler2D ch, sampler2D ch_com) {
    float mass_t = 0.0f;
    float2 com_t = to_float2(0);
    range(i, 5) {
        range(j, 5) {
            float2 off = to_float2(0);
            S(float4 u,i,j);
            COM(float2 com_p,i,j);
            float mass = u.z;
            float curl = u.w;
            float2 v = swi2(u,x,y);
            float2 p0 = com_p + to_float2(i,j) + v;
            float3 d = distribution(p0, to_float2(0), VIRTUAL_PARTICLE_SIZE);
            float mass_p = mass * d.z;
            mass_t += mass_p;
            com_t += mass_p * swi2(d,x,y);
        }
    }
    if (mass_t != 0.0f) {
        com_t /= mass_t;
    }
    return to_float2(com_t);
}

__DEVICE__ float4 ForwardAdvection(float2 p, sampler2D ch, sampler2D ch_com) {
    float mass_t = 0.0f;
    float2 vel_t = to_float2(0);
    float curl_t = 0.0f;
    range(i, 5) {
        range(j, 5) {
            float2 off = to_float2(0);
            S(float4 u,i,j);
            COM(float2 com_p,i,j);
            float mass = u.z;
            float curl = u.w;
            float2 v = swi2(u,x,y);
            float2 p0 = com_p + to_float2(i,j) + v;
            float3 d = distribution(p0, to_float2(0), VIRTUAL_PARTICLE_SIZE);
            float mass_p = mass * d.z;
            mass_t += mass_p;
            vel_t += v * mass_p;
            curl_t += curl * mass_p;
        }
    }
    if (mass_t != 0.0f) {
        vel_t /= mass_t;
        curl_t /= mass_t;
    }
    return to_float4(vel_t, mass_t, curl_t);
}


__DEVICE__ float2 MultiscaleTurbulence(sampler2D ch) {
    float2 turbulence = to_float2(0);
    for (int i = 1; i <= MULTISCALE_KERNEL_STEPS; i++) {
        float stride = float(i);
        Vec4Neighborhood n = GetStridedVec4NeighborhoodRK4(ch, stride);
        float4 U = GetCenter(n);
        float M = length(swi2(U,x,y));
        turbulence += M*(1.0f/_powf(stride,MULTISCALE_KERNEL_POWER))*Turbulence(n);  
    }
    return turbulence;
}

__DEVICE__ float2 MultiscaleVorticity(sampler2D ch) {
    float2 vorticity = to_float2(0);
    for (int i = 1; i <= MULTISCALE_KERNEL_STEPS; i++) {
        float stride = float(i);
        Vec4Neighborhood n = GetStridedVec4NeighborhoodRK4(ch, stride);
        float4 U = GetCenter(n);
        float M = length(swi2(U,x,y));
        float curl = ApplyVec4KernelScalar(n, GetCurlKernel());
        vorticity += M*(1.0f/_powf(stride,MULTISCALE_KERNEL_POWER))*Vorticity(n, curl);
    }
    return vorticity;
}

__DEVICE__ float4 MultiscaleViscosity(sampler2D ch) {
    float4 viscosity = to_float4_aw(0);
    for (int i = 1; i <= MULTISCALE_KERNEL_STEPS; i++) {
        float stride = float(i);
        Vec4Neighborhood n = GetStridedVec4NeighborhoodRK4(ch, stride);
        float4 U = GetCenter(n);
        float4 laplacian = ApplyVec4KernelVector(n, GetLaplacianKernel());
        viscosity += (1.0f/_powf(stride,MULTISCALE_KERNEL_POWER))*laplacian;
    }
    return viscosity;
}

__DEVICE__ void MultiscaleKernels(sampler2D ch, out float2 turbulence, out float2 vorticity, out float2 viscosity) {
    turbulence = to_float2(0);
    vorticity = to_float2(0);
    viscosity = to_float2(0);
    for (int i = 1; i <= MULTISCALE_KERNEL_STEPS; i++) {
        float stride = float(i);
        Vec4Neighborhood n = GetStridedVec4NeighborhoodRK4(ch, stride);
        float4 U = GetCenter(n);
        float M = length(swi2(U,x,y));
        float curl = ApplyVec4KernelScalar(n, GetCurlKernel());
        float4 laplacian = ApplyVec4KernelVector(n, GetLaplacianKernel());
        float W = (1.0f/_powf(stride,MULTISCALE_KERNEL_POWER));
        viscosity += W*swi2(laplacian,x,y);
        turbulence += M*W*Turbulence(n);  
        vorticity += M*W*Vorticity(n, curl);
    }
}


__DEVICE__ void Fluid( out float4 U, in float2 p, sampler2D ch, sampler2D ch_com, float4 mouse )
{
    float2 turbulence, viscosity, vorticity;
    MultiscaleKernels(ch, turbulence, vorticity, viscosity);

    Vec4Neighborhood neighborhood = GetVec4NeighborhoodRK4(ch, TURBULENCE_SCALE * turbulence);

    float4 dist = ForwardAdvection(p, ch, ch_com);
    U = GetCenter(neighborhood);
    //U = _mix(dist,U,smoothstep(0.0f,MAX_CONSERVATIVE_DISTANCE,length(swi2(U,x,y))));
    U = _mix(dist,U,smoothstep(MAX_CONSERVATIVE_DISTANCE - 1.0f,MAX_CONSERVATIVE_DISTANCE,length(swi2(U,x,y))));
    
    //float lU = length(swi2(U,x,y));

    // Laplacian/Viscosity
    swi2(U,x,y) += VISCOSITY_SCALE*viscosity;

    // Curl/Vorticity
    U.w = ApplyVec4KernelScalar(neighborhood, GetCurlKernel());
    swi2(U,x,y) += VORTICITY_SCALE*vorticity; 
    
    //swi2(U,x,y) = lU*normz(swi2(U,x,y));

    // Add mass with the mouse
    U = MouseMass(mouse, U, p, 10.0f, 0.2f);
    
    // Mouse interaction in phase domain/space domain
    U = MouseSpace(mouse, U, p, 20.0f, 0.3f);

    swi2(U,x,y) = SoftBound(swi2(U,x,y), 1.0f, 0.00001f);
    swi2(U,x,y) = HardBound(swi2(U,x,y), 16.0f);
    U.z = _fmaxf(0.0f, SoftBound(U.z, 0.00001f));
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Preset: Keyboard' to iChannel3
// Connect Buffer A 'Previsualization: Buffer C' to iChannel1
// Connect Buffer A 'Previsualization: Buffer D' to iChannel0


__KERNEL__ void HybridVirtualParticleFluidFuse__Buffer_A(float4 c, float2 p, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{

    initialize(p, iFrame, iResolution);
    if (iFrame <= 1) {
        c = to_float4(0,0,INIT_MASS,0);
    } else {
        if (FRAME_MOD(0)) {
            Fluid(c, uv*R, iChannel0, iChannel1, iMouse);
        } else {
            c = texelFetch(iChannel0, to_int2(p), 0);
        }
    }


  SetFragmentShaderComputedColor(c);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer C' to iChannel1


bool reset() {
    return iFrame <= 1 || texture(iChannel3, to_float2(32.5f/256.0f, 0.5f) ).x > 0.5f;
}

__DEVICE__ float4 Po(int m, int n) {
    float2 ouv = uv + texel * to_float2(m,n);
    if (BoundsCheck(ouv)) {
        return to_float4_aw(pack2x16(to_float2(0)),pack2x16(to_float2(0)),pack2x16(to_float2(0)),0);
    } else {
        return textureLod(iChannel0, ouv, 0.0f);
    }
}

__DEVICE__ float Go(int m, int n) {
    float2 ouv = uv + texel * to_float2(m,n);
    if (BoundsCheck(ouv)) {
        return 0.0f;
    } else {
        return textureLod(iChannel1, ouv, 0.0f).x;
    }
}

__KERNEL__ void HybridVirtualParticleFluidFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel3)
{

    initialize(fragCoord, iFrame, iResolution);
    
    float3 p_y[151] = vec3[](to_float3(-0.00039936512386562484f,-0.0013037830496811509f,-0.003024369266497462f),to_float3(-0.00043479272952107184f,-0.001418247837094988f,-0.0032841431420175815f),to_float3(-0.00047311175208775147f,-0.0015418984185112672f,-0.0035640171624620187f),to_float3(-0.0005145378703601011f,-0.0016753999013442086f,-0.003865351345832546f),to_float3(-0.0005593015430286112f,-0.00181946044562425f,-0.004189581775882141f),to_float3(-0.0006076489826017296f,-0.001974833797626846f,-0.004538223620654535f),to_float3(-0.0006598431999115183f,-0.002142321974855929f,-0.004912874168389093f),to_float3(-0.0007161651258619417f,-0.0023227781140663494f,-0.005315215867981123f),to_float3(-0.0007769148178819725f,-0.0025171094952469407f,-0.00574701935895483f),to_float3(-0.0008424127594587307f,-0.002726280755872174f,-0.0062101464732849465f),to_float3(-0.0009130012621639432f,-0.002951317311279131f,-0.006706553188329675f),to_float3(-0.0009890459807688295f,-0.003193308998756704f,-0.007238292506523368f),to_float3(-0.0010709375533892544f,-0.0034534139648667857f,-0.007807517233220254f),to_float3(-0.001159093380140449f,-0.003732862817675909f,-0.008416482619054973f),to_float3(-0.0012539595555377625f,-0.004032963067987648f,-0.009067548827244675f),to_float3(-0.0013560129718921017f,-0.004355103886359663f,-0.009763183179221676f),to_float3(-0.0014657636132559054f,-0.004700761205697934f,-0.010505962123640127f),to_float3(-0.0015837570621268875f,-0.0050715032025820585f,-0.011298572863887848f),to_float3(-0.001710577244168271f,-0.005468996194229675f,-0.01214381456744096f),to_float3(-0.0018468494397243054f,-0.005895010992201898f,-0.01304459906634993f),to_float3(-0.0019932435949786592f,-0.006351429758636426f,-0.014003950941382284f),to_float3(-0.0021504779703164487f,-0.006840253416027218f,-0.015025006862317022f),to_float3(-0.0023193231689242525f,-0.007363609667412167f,-0.016111014032915174f),to_float3(-0.002500606595033479f,-0.007923761690353888f,-0.017265327560358507f),to_float3(-0.002695217398648073f,-0.008523117575376556f,-0.018491406534454295f),to_float3(-0.002904111972299025f,-0.009164240587639544f,-0.019792808560418648f),to_float3(-0.0031283200755794476f,-0.00984986033967067f,-0.021173182439071275f),to_float3(-0.003368951675232755f,-0.01058288497304438f,-0.022636258627945997f),to_float3(-0.0036272046027564153f,-0.01136641445806573f,-0.02418583704385647f),to_float3(-0.0039043731482907218f,-0.012203755132905171f,-0.025825771679025524f),to_float3(-0.004201857729535902f,-0.013098435617310009f,-0.027559951395473018f),to_float3(-0.00452117579826187f,-0.01405422425106731f,-0.029392276131583135f),to_float3(-0.00486397417548751f,-0.015075148223856253f,-0.0313266275951637f),to_float3(-0.005232043040666073f,-0.016165514581003473f,-0.03336683332200887f),to_float3(-0.00562733184154224f,-0.017329933308865297f,-0.03551662273933928f),to_float3(-0.0060519674414129645f,-0.018573342723915864f,-0.037779573578614306f),to_float3(-0.006508274881439065f,-0.01990103741075051f,-0.040159046618245986f),to_float3(-0.006998801210110017f,-0.021318698975493443f,-0.0426581062860751f),to_float3(-0.007526342923405148f,-0.022832429901508428f,-0.04527942409155754f),to_float3(-0.008093977672062791f,-0.024448790812264518f,-0.048025161159515134f),to_float3(-0.008705101032420682f,-0.026174841459272888f,-0.05089682526368315f),to_float3(-0.009363469312043276f,-0.028018185757524506f,-0.05389509666073895f),to_float3(-0.010073249580677117f,-0.029987021181332632f,-0.05701961564096375f),to_float3(-0.01083907839404583f,-0.03209019280176492f,-0.06026872295763385f),to_float3(-0.011666131030046848f,-0.03433725218081176f,-0.0636391420650306f),to_float3(-0.012560203507485332f,-0.03673852121908325f,-0.06712558924087116f),to_float3(-0.013527810238371974f,-0.03930516085637037f,-0.07072029400179898f),to_float3(-0.014576300919535715f,-0.04204924420785956f,-0.07441240748450588f),to_float3(-0.015714001257992345f,-0.04498383322316428f,-0.07818727031588318f),to_float3(-0.016950383431117873f,-0.04812305719009568f,-0.08202550346745388f),to_float3(-0.018296273925712297f,-0.051482190232598785f,-0.08590187504816119f),to_float3(-0.01976410874874513f,-0.05507772316147769f,-0.0897838820674849f),to_float3(-0.02136824920173702f,-0.05892742230067349f,-0.09362996770370406f),to_float3(-0.02312537581916243f,-0.06305036371943105f,-0.09738726988920325f),to_float3(-0.025054984222680283f,-0.06746692484105196f,-0.10098876378585503f),to_float3(-0.02718001534382911f,-0.07219870536957633f,-0.10434961578501721f),to_float3(-0.02952766495752478f,-0.07726833375162516f,-0.10736250560624028f),to_float3(-0.03213043568796663f,-0.08269909044537457f,-0.10989158977713533f),to_float3(-0.03502752169832732f,-0.08851423913888098f,-0.11176466592987913f),to_float3(-0.03826665722222464f,-0.09473589144745072f,-0.11276294193733785f),to_float3(-0.041906623421512364f,-0.10138312131867837f,-0.11260760327593745f),to_float3(-0.04602070835716154f,-0.10846885934496983f,-0.11094209169468648f),to_float3(-0.050701578024441495f,-0.1159947726771164f,-0.10730865033138975f),to_float3(-0.05606828993205902f,-0.12394275393722222f,-0.10111727591157052f),to_float3(-0.06227665518595623f,-0.13226056303078285f,-0.09160486258770702f),to_float3(-0.06953501099043398f,-0.14083708737623918f,-0.077782417479591f),to_float3(-0.07812908130749861f,-0.149458503599576f,-0.058369977232929894f),to_float3(-0.08846282054472075f,-0.15772775580732623f,-0.031725854990398726f),to_float3(-0.10112895949827036f,-0.1649097516191365f,0.004199908847829801f),to_float3(-0.11703860203562352f,-0.16961595360355963f,0.05178318985988212f),to_float3(-0.13767855574664672f,-0.16911182932471974f,0.11316073490042394f),to_float3(-0.16567645398825667f,-0.15763971441508162f,0.18793345990415217f),to_float3(-0.2062192509611813f,-0.12179330842214287f,0.2642830614724927f),to_float3(-0.2713074094027817f,-0.02537420724641723f,0.28182569573805677f),to_float3(-0.395760620255607f,0.2520387049261719f,-0.04827629938430805f),to_float3(-0.4794057541719356f,0.45941736433304614f,-0.35998402950121244f),to_float3(-0.39576062025560704f,0.25203870492617186f,-0.04827629938430784f),to_float3(-0.27130740940278164f,-0.02537420724641726f,0.2818256957380568f),to_float3(-0.20621925096118135f,-0.12179330842214289f,0.2642830614724926f),to_float3(-0.16567645398825667f,-0.15763971441508165f,0.18793345990415228f),to_float3(-0.13767855574664675f,-0.1691118293247198f,0.1131607349004239f),to_float3(-0.11703860203562354f,-0.1696159536035597f,0.05178318985988209f),to_float3(-0.10112895949827036f,-0.1649097516191365f,0.004199908847829665f),to_float3(-0.0884628205447208f,-0.1577277558073263f,-0.03172585499039877f),to_float3(-0.07812908130749864f,-0.14945850359957605f,-0.05836997723292993f),to_float3(-0.06953501099043398f,-0.14083708737623918f,-0.07778241747959105f),to_float3(-0.06227665518595623f,-0.13226056303078287f,-0.09160486258770703f),to_float3(-0.05606828993205902f,-0.12394275393722226f,-0.10111727591157058f),to_float3(-0.050701578024441495f,-0.11599477267711644f,-0.10730865033138978f),to_float3(-0.04602070835716156f,-0.10846885934496989f,-0.1109420916946865f),to_float3(-0.04190662342151239f,-0.10138312131867841f,-0.11260760327593748f),to_float3(-0.03826665722222467f,-0.09473589144745076f,-0.11276294193733784f),to_float3(-0.03502752169832737f,-0.08851423913888105f,-0.1117646659298792f),to_float3(-0.032130435687966606f,-0.0826990904453746f,-0.10989158977713538f),to_float3(-0.029527664957524794f,-0.0772683337516252f,-0.10736250560624033f),to_float3(-0.027180015343829116f,-0.07219870536957636f,-0.1043496157850173f),to_float3(-0.025054984222680304f,-0.06746692484105203f,-0.10098876378585511f),to_float3(-0.023125375819162436f,-0.06305036371943108f,-0.09738726988920338f),to_float3(-0.02136824920173702f,-0.058927422300673514f,-0.09362996770370416f),to_float3(-0.019764108748745155f,-0.05507772316147772f,-0.08978388206748496f),to_float3(-0.01829627392571232f,-0.05148219023259884f,-0.08590187504816127f),to_float3(-0.01695038343111789f,-0.04812305719009571f,-0.08202550346745398f),to_float3(-0.015714001257992355f,-0.0449838332231643f,-0.07818727031588321f),to_float3(-0.014576300919535724f,-0.04204924420785958f,-0.07441240748450599f),to_float3(-0.013527810238371971f,-0.039305160856370404f,-0.07072029400179902f),to_float3(-0.012560203507485332f,-0.036738521219083255f,-0.06712558924087117f),to_float3(-0.011666131030046859f,-0.03433725218081179f,-0.0636391420650307f),to_float3(-0.01083907839404584f,-0.03209019280176495f,-0.06026872295763393f),to_float3(-0.010073249580677119f,-0.029987021181332653f,-0.05701961564096378f),to_float3(-0.009363469312043281f,-0.02801818575752456f,-0.05389509666073898f),to_float3(-0.008705101032420694f,-0.026174841459272933f,-0.0508968252636832f),to_float3(-0.008093977672062803f,-0.024448790812264518f,-0.04802516115951517f),to_float3(-0.007526342923405146f,-0.02283242990150845f,-0.04527942409155761f),to_float3(-0.006998801210110013f,-0.021318698975493443f,-0.04265810628607512f),to_float3(-0.006508274881439066f,-0.01990103741075051f,-0.040159046618246f),to_float3(-0.00605196744141298f,-0.018573342723915892f,-0.03777957357861436f),to_float3(-0.005627331841542247f,-0.017329933308865328f,-0.03551662273933936f),to_float3(-0.005232043040666078f,-0.016165514581003487f,-0.033366833322008904f),to_float3(-0.004863974175487524f,-0.015075148223856267f,-0.031326627595163734f),to_float3(-0.004521175798261876f,-0.01405422425106733f,-0.029392276131583166f),to_float3(-0.004201857729535902f,-0.013098435617310021f,-0.027559951395473042f),to_float3(-0.0039043731482907213f,-0.012203755132905178f,-0.025825771679025535f),to_float3(-0.003627204602756424f,-0.01136641445806575f,-0.024185837043856497f),to_float3(-0.003368951675232753f,-0.010582884973044387f,-0.022636258627946024f),to_float3(-0.0031283200755794494f,-0.009849860339670675f,-0.021173182439071295f),to_float3(-0.002904111972299031f,-0.009164240587639563f,-0.019792808560418675f),to_float3(-0.002695217398648074f,-0.00852311757537658f,-0.01849140653445433f),to_float3(-0.002500606595033485f,-0.007923761690353899f,-0.017265327560358527f),to_float3(-0.0023193231689242495f,-0.007363609667412189f,-0.016111014032915188f),to_float3(-0.002150477970316447f,-0.006840253416027219f,-0.01502500686231705f),to_float3(-0.001993243594978657f,-0.006351429758636433f,-0.014003950941382294f),to_float3(-0.0018468494397243095f,-0.005895010992201904f,-0.013044599066349954f),to_float3(-0.0017105772441682716f,-0.005468996194229684f,-0.012143814567440982f),to_float3(-0.0015837570621268916f,-0.0050715032025820655f,-0.011298572863887869f),to_float3(-0.0014657636132559086f,-0.004700761205697936f,-0.010505962123640147f),to_float3(-0.0013560129718921034f,-0.004355103886359678f,-0.009763183179221696f),to_float3(-0.0012539595555377642f,-0.004032963067987664f,-0.009067548827244698f),to_float3(-0.0011590933801404499f,-0.003732862817675913f,-0.00841648261905499f),to_float3(-0.0010709375533892564f,-0.003453413964866797f,-0.007807517233220275f),to_float3(-0.0009890459807688297f,-0.0031933089987567142f,-0.007238292506523384f),to_float3(-0.0009130012621639455f,-0.002951317311279133f,-0.006706553188329679f),to_float3(-0.0008424127594587336f,-0.002726280755872182f,-0.006210146473284956f),to_float3(-0.0007769148178819765f,-0.002517109495246948f,-0.005747019358954837f),to_float3(-0.0007161651258619422f,-0.002322778114066351f,-0.005315215867981134f),to_float3(-0.0006598431999115193f,-0.0021423219748559342f,-0.004912874168389099f),to_float3(-0.0006076489826017314f,-0.0019748337976268505f,-0.004538223620654547f),to_float3(-0.0005593015430286124f,-0.001819460445624252f,-0.004189581775882171f),to_float3(-0.0005145378703601023f,-0.001675399901344218f,-0.00386535134583253f),to_float3(-0.00047311175208774605f,-0.001541898418511293f,-0.003564017162461541f),to_float3(-0.0004347927295209686f,-0.0014182478370946185f,-0.003284143142016918f),to_float3(-0.0003993651238658139f,-0.0013037830496811053f,-0.003024369266498018f));
    float3 p_x[151] = vec3[](to_float3(-0.002137124133264062f,-0.0045652949750816674f,-0.007944999489624014f),to_float3(-0.002300156955533428f,-0.004908806940460253f,-0.008525633362203022f),to_float3(-0.0024740180160609526f,-0.0052745508615946735f,-0.009141755140060714f),to_float3(-0.0026593024187144994f,-0.005663677685852353f,-0.009794948578414675f),to_float3(-0.0028566298846672803f,-0.006077377342767512f,-0.010486819494099436f),to_float3(-0.0030666454073479945f,-0.006516878738501725f,-0.011218991293007716f),to_float3(-0.0032900199322388997f,-0.0069834496606139915f,-0.011993099926857181f),to_float3(-0.0035274510673012965f,-0.007478396589859675f,-0.012810788233390612f),to_float3(-0.0037796638307912127f,-0.008003064415882877f,-0.013673699609992897f),to_float3(-0.004047411444335345f,-0.008558836053786144f,-0.014583470965923965f),to_float3(-0.004331476180377802f,-0.009147131958638085f,-0.015541724892891462f),to_float3(-0.00463267027449884f,-0.009769409534995865f,-0.01655006098737826f),to_float3(-0.004951836914666999f,-0.010427162438453347f,-0.017610046250856503f),to_float3(-0.005289851321237063f,-0.011121919766049194f,-0.018723204485601174f),to_float3(-0.005647621933474761f,-0.011855245132051603f,-0.019891004594067244f),to_float3(-0.006026091720604429f,-0.012628735625135244f,-0.02111484767848965f),to_float3(-0.0064262396378738365f,-0.013444020642236373f,-0.02239605282424208f),to_float3(-0.006849082250952507f,-0.014302760593353525f,-0.02373584143523893f),to_float3(-0.007295675555175712f,-0.015206645470184573f,-0.025135319971921f),to_float3(-0.0077671170197740995f,-0.01615739326966947f,-0.026595460921701564f),to_float3(-0.008264547891358274f,-0.01715674826113821f,-0.02811708180766433f),to_float3(-0.008789155795641856f,-0.0182064790827157f,-0.02970082201319753f),to_float3(-0.009342177681784285f,-0.019308376648754934f,-0.031347117167421494f),to_float3(-0.00992490315993552f,-0.02046425184516241f,-0.03305617079787943f),to_float3(-0.010538678289711445f,-0.021675932983309377f,-0.03482792291202483f),to_float3(-0.011184909885595262f,-0.022945262975493885f,-0.036662015116369455f),to_float3(-0.011865070414853763f,-0.024274096185269295f,-0.03855775182034264f),to_float3(-0.01258070357473496f,-0.025664294893927936f,-0.040514056999271006f),to_float3(-0.013333430648782561f,-0.02711772530945963f,-0.042529425905391674f),to_float3(-0.01412495775744369f,-0.028636253025678345f,-0.04460187101503015f),to_float3(-0.014957084136223639f,-0.030221737816030457f,-0.04672886138108205f),to_float3(-0.01583171159602881f,-0.031876027617728446f,-0.04890725441919534f),to_float3(-0.016750855345744924f,-0.03360095152585251f,-0.051133218989284704f),to_float3(-0.01771665638740403f,-0.03539831157208638f,-0.0534021484360339f),to_float3(-0.018731395730599044f,-0.03726987300644707f,-0.055708562016558806f),to_float3(-0.019797510716488533f,-0.0392173527296855f,-0.05804599286274144f),to_float3(-0.020917613794531456f,-0.04124240543505191f,-0.06040686029055546f),to_float3(-0.02209451415920214f,-0.043346606905710634f,-0.06278232386756345f),to_float3(-0.023331242732158364f,-0.045531433771570005f,-0.06516211616871623f),to_float3(-0.024631081071251157f,-0.04779823884785927f,-0.06753435057251908f),to_float3(-0.02599759490599191f,-0.05014822094579368f,-0.06988529975365311f),to_float3(-0.02743467314560627f,-0.05258238774762672f,-0.07219913968862342f),to_float3(-0.028946573388404322f,-0.05510150995351754f,-0.07445765297670581f),to_float3(-0.03053797519013025f,-0.05770606440794075f,-0.07663988405127538f),to_float3(-0.03221404263776634f,-0.0603961632607468f,-0.07872173737035694f),to_float3(-0.033980498142029227f,-0.06317146536031418f,-0.08067550787447306f),to_float3(-0.035843709830794705f,-0.06603106494154067f,-0.08246933081865009f),to_float3(-0.03781079552984435f,-0.0689733511594637f,-0.08406653544718634f),to_float3(-0.03988974710170204f,-0.07199582998922682f,-0.08542488379813157f),to_float3(-0.042089579940160424f,-0.07509489726503166f,-0.08649567210767374f),to_float3(-0.044420513774308154f,-0.0782655478780229f,-0.0872226677458533f),to_float3(-0.046894192744119025f,-0.0815010009801992f,-0.08754084929384119f),to_float3(-0.04952395514522684f,-0.08479221383871625f,-0.08737491127852688f),to_float3(-0.052325166556904924f,-0.08812724684632407f,-0.0866374883735367f),to_float3(-0.05531563463635159f,-0.09149042775415551f,-0.08522704702612285f),to_float3(-0.05851613023760088f,-0.09486124236558141f,-0.0830253865465934f),to_float3(-0.06195104853165206f,-0.09821284847174405f,-0.0798946889222969f),to_float3(-0.06564925675340012f,-0.10151006459451122f,-0.07567406140614437f),to_float3(-0.06964519409927278f,-0.1047066168725813f,-0.07017553687965904f),to_float3(-0.07398031737724835f,-0.10774132259935602f,-0.06317955075217202f),to_float3(-0.0787050285543731f,-0.11053272464789683f,-0.054430032064937746f),to_float3(-0.08388128622421473f,-0.11297142788834234f,-0.0436294939520135f),to_float3(-0.08958620751898465f,-0.1149089568361548f,-0.03043501110604133f),to_float3(-0.09591713730219445f,-0.11614122538666281f,-0.014456986894158296f),to_float3(-0.10299894760598283f,-0.11638344275691273f,0.004735321117216877f),to_float3(-0.11099482783905858f,-0.11523099884933204f,0.027593267367261202f),to_float3(-0.12012272629777299f,-0.1120965987947297f,0.054535916053032214f),to_float3(-0.13068130718798532f,-0.10610553463683693f,0.08582732348023998f),to_float3(-0.1430926878027598f,-0.09591364237278834f,0.12130023611322262f),to_float3(-0.15797644463814361f,-0.07937430036055836f,0.15972950012046713f),to_float3(-0.17628590900450966f,-0.05289019694443883f,0.1973636686263485f),to_float3(-0.19957914170178254f,-0.010050447660950014f,0.22428672225770246f),to_float3(-0.23061230582092926f,0.06152568473621712f,0.21465485784075855f),to_float3(-0.2748091985304043f,0.18844826204726473f,0.09754352781288295f),to_float3(-0.3445128239871801f,0.4367329316543684f,-0.34280145660982947f),to_float3(3.01878863857207e-17,2.1657391732312188e-17,2.1073950360688117e-17),to_float3(0.3445128239871802f,-0.4367329316543684f,0.3428014566098292f),to_float3(0.27480919853040436f,-0.18844826204726473f,-0.09754352781288317f),to_float3(0.23061230582092923f,-0.061525684736217036f,-0.21465485784075863f),to_float3(0.19957914170178256f,0.01005044766095007f,-0.2242867222577026f),to_float3(0.17628590900450966f,0.0528901969444389f,-0.19736366862634852f),to_float3(0.15797644463814361f,0.07937430036055836f,-0.15972950012046708f),to_float3(0.14309268780275983f,0.09591364237278836f,-0.1213002361132226f),to_float3(0.13068130718798535f,0.10610553463683695f,-0.08582732348023968f),to_float3(0.12012272629777299f,0.11209659879472973f,-0.054535916053032235f),to_float3(0.11099482783905856f,0.11523099884933206f,-0.027593267367261167f),to_float3(0.10299894760598281f,0.11638344275691272f,-0.004735321117216822f),to_float3(0.09591713730219444f,0.11614122538666284f,0.014456986894158358f),to_float3(0.08958620751898468f,0.11490895683615483f,0.03043501110604141f),to_float3(0.08388128622421473f,0.11297142788834233f,0.04362949395201367f),to_float3(0.0787050285543731f,0.11053272464789683f,0.054430032064937794f),to_float3(0.07398031737724835f,0.10774132259935604f,0.06317955075217199f),to_float3(0.06964519409927278f,0.10470661687258127f,0.07017553687965912f),to_float3(0.06564925675340012f,0.10151006459451126f,0.07567406140614436f),to_float3(0.06195104853165206f,0.09821284847174405f,0.0798946889222969f),to_float3(0.05851613023760088f,0.09486124236558141f,0.08302538654659344f),to_float3(0.05531563463635159f,0.09149042775415551f,0.08522704702612297f),to_float3(0.05232516655690491f,0.08812724684632407f,0.08663748837353664f),to_float3(0.04952395514522684f,0.08479221383871625f,0.08737491127852687f),to_float3(0.046894192744119025f,0.08150100098019919f,0.0875408492938412f),to_float3(0.044420513774308154f,0.0782655478780229f,0.0872226677458533f),to_float3(0.04208957994016041f,0.07509489726503163f,0.08649567210767378f),to_float3(0.03988974710170204f,0.07199582998922681f,0.08542488379813157f),to_float3(0.037810795529844336f,0.0689733511594637f,0.08406653544718634f),to_float3(0.03584370983079468f,0.06603106494154064f,0.0824693308186501f),to_float3(0.03398049814202922f,0.06317146536031418f,0.08067550787447306f),to_float3(0.03221404263776634f,0.0603961632607468f,0.07872173737035698f),to_float3(0.03053797519013025f,0.05770606440794075f,0.07663988405127538f),to_float3(0.02894657338840431f,0.055101509953517515f,0.07445765297670578f),to_float3(0.02743467314560627f,0.05258238774762672f,0.07219913968862343f),to_float3(0.025997594905991905f,0.05014822094579368f,0.06988529975365308f),to_float3(0.024631081071251146f,0.04779823884785925f,0.06753435057251908f),to_float3(0.02333124273215836f,0.04553143377157f,0.06516211616871614f),to_float3(0.022094514159202137f,0.04334660690571063f,0.06278232386756344f),to_float3(0.020917613794531453f,0.041242405435051886f,0.060406860290555434f),to_float3(0.019797510716488522f,0.039217352729685476f,0.05804599286274142f),to_float3(0.01873139573059904f,0.03726987300644707f,0.05570856201655882f),to_float3(0.017716656387404026f,0.035398311572086366f,0.05340214843603387f),to_float3(0.016750855345744917f,0.0336009515258525f,0.051133218989284704f),to_float3(0.015831711596028804f,0.03187602761772843f,0.04890725441919533f),to_float3(0.01495708413622363f,0.030221737816030454f,0.04672886138108203f),to_float3(0.014124957757443688f,0.028636253025678342f,0.04460187101503013f),to_float3(0.013333430648782552f,0.02711772530945961f,0.04252942590539163f),to_float3(0.012580703574734953f,0.025664294893927922f,0.040514056999270985f),to_float3(0.011865070414853759f,0.024274096185269285f,0.03855775182034264f),to_float3(0.011184909885595262f,0.022945262975493878f,0.036662015116369455f),to_float3(0.010538678289711445f,0.02167593298330937f,0.0348279229120248f),to_float3(0.009924903159935513f,0.020464251845162408f,0.03305617079787941f),to_float3(0.00934217768178428f,0.019308376648754923f,0.03134711716742148f),to_float3(0.00878915579564185f,0.01820647908271569f,0.029700822013197525f),to_float3(0.008264547891358267f,0.017156748261138197f,0.028117081807664302f),to_float3(0.007767117019774098f,0.016157393269669456f,0.026595460921701543f),to_float3(0.0072956755551757f,0.015206645470184556f,0.02513531997192098f),to_float3(0.0068490822509524995f,0.014302760593353517f,0.023735841435238908f),to_float3(0.006426239637873832f,0.013444020642236359f,0.022396052824242063f),to_float3(0.006026091720604427f,0.012628735625135236f,0.02111484767848964f),to_float3(0.0056476219334747595f,0.011855245132051592f,0.019891004594067244f),to_float3(0.005289851321237059f,0.01112191976604919f,0.018723204485601167f),to_float3(0.0049518369146669934f,0.010427162438453336f,0.017610046250856482f),to_float3(0.004632670274498837f,0.009769409534995856f,0.016550060987378257f),to_float3(0.004331476180377796f,0.009147131958638074f,0.015541724892891445f),to_float3(0.004047411444335342f,0.008558836053786137f,0.014583470965923953f),to_float3(0.0037796638307912088f,0.008003064415882869f,0.013673699609992889f),to_float3(0.0035274510673012917f,0.007478396589859665f,0.012810788233390602f),to_float3(0.003290019932238895f,0.006983449660613981f,0.011993099926857178f),to_float3(0.0030666454073479924f,0.006516878738501718f,0.011218991293007703f),to_float3(0.002856629884667277f,0.006077377342767509f,0.01048681949409938f),to_float3(0.0026593024187144963f,0.005663677685852356f,0.00979494857841503f),to_float3(0.0024740180160610133f,0.0052745508615947585f,0.009141755140060302f),to_float3(0.0023001569555331914f,0.004908806940460417f,0.00852563336220343f),to_float3(0.0021371241332640567f,0.0045652949750816605f,0.007944999489623995f));
    float s_i[3] = float[](0.2424503566193514f,0.10170785486914974f,0.03723335168874466f);
    float g_x[151] = float[](-0.012893115592183313f,-0.013698670060406285f,-0.01454270853033401f,-0.015426186340698034f,-0.01635001813564313f,-0.01731507233271321f,-0.018322165451038992f,-0.01937205631983224f,-0.020465440189183726f,-0.021602942767020888f,-0.022785114207886127f,-0.024012423080935617f,-0.025285250346210764f,-0.02660388336978562f,-0.02796851000982456f,-0.029379212806879423f,-0.030835963312895207f,-0.03233861659436575f,-0.03388690594586321f,-0.03548043785074925f,-0.03711868722624115f,-0.03880099299014412f,-0.04052655398645646f,-0.0422944253066969f,-0.04410351504318662f,-0.04595258150963188f,-0.047840230963193016f,-0.0497649158607877f,-0.0517249336806611f,-0.05371842633826146f,-0.05574338022319264f,-0.057797626881478534f,-0.059878844364578875f,-0.06198455926355096f,-0.06411214944346842f,-0.06625884748970856f,-0.068421744874013f,-0.07059779684434063f,-0.07278382803848439f,-0.07497653881724314f,-0.07717251230864798f,-0.0793682221503752f,-0.08156004091305374f,-0.08374424918274104f,-0.0859170452764106f,-0.08807455555991833f,-0.09021284533361659f,-0.09232793024659676f,-0.0944157881965094f,-0.09647237166805361f,-0.09849362045958987f,-0.10047547474393566f,-0.10241388840629213f,-0.10430484259943712f,-0.10614435945385038f,-0.10792851587831802f,-0.109653457384833f,-0.11131541187027436f,-0.11291070328643685f,-0.11443576512950307f,-0.11588715368001017f,-0.1172615609247814f,-0.11855582709315698f,-0.11976695274118251f,-0.12089211031918369f,-0.12192865516037081f,-0.12287413583076501f,-0.12372630378379504f,-0.12448312226638165f,-0.12514277442715682f,-0.1257036705816586f,-0.12616445459385237f,-0.126524009338131f,-0.12678146121101333f,-0.12693618366704282f,-0.1269877997588608f,-0.12693618366704285f,-0.12678146121101333f,-0.12652400933813102f,-0.12616445459385237f,-0.12570367058165863f,-0.12514277442715685f,-0.12448312226638174f,-0.12372630378379511f,-0.12287413583076504f,-0.12192865516037085f,-0.1208921103191837f,-0.11976695274118256f,-0.11855582709315705f,-0.11726156092478153f,-0.11588715368001025f,-0.11443576512950313f,-0.1129107032864369f,-0.11131541187027441f,-0.10965345738483308f,-0.10792851587831806f,-0.10614435945385046f,-0.10430484259943716f,-0.10241388840629222f,-0.1004754747439357f,-0.09849362045958993f,-0.09647237166805367f,-0.09441578819650946f,-0.09232793024659687f,-0.09021284533361669f,-0.08807455555991844f,-0.08591704527641067f,-0.08374424918274113f,-0.08156004091305383f,-0.07936822215037528f,-0.07717251230864812f,-0.07497653881724321f,-0.07278382803848449f,-0.07059779684434074f,-0.06842174487401309f,-0.06625884748970862f,-0.06411214944346849f,-0.06198455926355104f,-0.05987884436457899f,-0.057797626881478596f,-0.05574338022319272f,-0.05371842633826156f,-0.05172493368066117f,-0.04976491586078776f,-0.047840230963193085f,-0.04595258150963194f,-0.04410351504318669f,-0.04229442530669699f,-0.040526553986456555f,-0.03880099299014417f,-0.0371186872262412f,-0.03548043785074932f,-0.033886905945863265f,-0.03233861659436582f,-0.03083596331289527f,-0.029379212806879485f,-0.02796851000982462f,-0.026603883369785666f,-0.02528525034621083f,-0.02401242308093568f,-0.022785114207886165f,-0.02160294276702093f,-0.020465440189183767f,-0.019372056319832277f,-0.018322165451039044f,-0.017315072332713243f,-0.016350018135643175f,-0.015426186340698074f,-0.014542708530334052f,-0.013698670060406317f,-0.012893115592183577f);

    
    #define RANGE 75
    
    float2 P1 = to_float2(0);
    float2 P2 = to_float2(0);
    float2 P3 = to_float2(0);
    float G = 0.0f;
    float Gw = 0.0f;
    for (int i = -RANGE; i <= RANGE; i++) {
        int index = RANGE + i;

        float2 t = Po(i,0).xy;
        float g = Go(i,0);
        
        float3 py = p_y[index];
        float3 px = p_x[index];
        
        P1 += to_float2(px.x, py.x) * t;
        P2 += to_float2(px.y, py.y) * t;
        P3 += to_float2(px.z, py.z) * t;
        
        Gw += _fabs(g_x[index]);
        G  += _fabs(g_x[index]) * g;
    }
    
    G /= Gw;
    
    if(reset()) {
        fragColor = to_float4_aw(0);
    } else {
        fragColor = to_float4(pack2x16(P1),pack2x16(P2),pack2x16(P3), G);
    }



  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0
// Connect Buffer C 'Previsualization: Buffer C' to iChannel1
// Connect Buffer C 'Previsualization: Buffer D' to iChannel2


bool reset() {
    return iFrame <= 1 || texture(iChannel3, to_float2(32.5f/256.0f, 0.5f) ).x > 0.5f;
}

__DEVICE__ float4 Po(int m, int n) {
    float2 ouv = uv + texel * to_float2(m,n);
    if (BoundsCheck(ouv)) {
        return to_float4_aw(pack2x16(to_float2(0)),pack2x16(to_float2(0)),pack2x16(to_float2(0)),0);
    } else {
        return textureLod(iChannel0, ouv, 0.0f);
    }
}

__DEVICE__ float Go(int m, int n) {
    float2 ouv = uv + texel * to_float2(m,n);
    if (BoundsCheck(ouv)) {
        return 0.0f;
    } else {
        return textureLod(iChannel1, ouv, 0.0f).x;
    }
}

__KERNEL__ void HybridVirtualParticleFluidFuse__Buffer_C(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{

    initialize(fragCoord, iFrame, iResolution);
    
    
    float3 p_y[151] = vec3[](to_float3(-0.00039936512386562484f,-0.0013037830496811509f,-0.003024369266497462f),to_float3(-0.00043479272952107184f,-0.001418247837094988f,-0.0032841431420175815f),to_float3(-0.00047311175208775147f,-0.0015418984185112672f,-0.0035640171624620187f),to_float3(-0.0005145378703601011f,-0.0016753999013442086f,-0.003865351345832546f),to_float3(-0.0005593015430286112f,-0.00181946044562425f,-0.004189581775882141f),to_float3(-0.0006076489826017296f,-0.001974833797626846f,-0.004538223620654535f),to_float3(-0.0006598431999115183f,-0.002142321974855929f,-0.004912874168389093f),to_float3(-0.0007161651258619417f,-0.0023227781140663494f,-0.005315215867981123f),to_float3(-0.0007769148178819725f,-0.0025171094952469407f,-0.00574701935895483f),to_float3(-0.0008424127594587307f,-0.002726280755872174f,-0.0062101464732849465f),to_float3(-0.0009130012621639432f,-0.002951317311279131f,-0.006706553188329675f),to_float3(-0.0009890459807688295f,-0.003193308998756704f,-0.007238292506523368f),to_float3(-0.0010709375533892544f,-0.0034534139648667857f,-0.007807517233220254f),to_float3(-0.001159093380140449f,-0.003732862817675909f,-0.008416482619054973f),to_float3(-0.0012539595555377625f,-0.004032963067987648f,-0.009067548827244675f),to_float3(-0.0013560129718921017f,-0.004355103886359663f,-0.009763183179221676f),to_float3(-0.0014657636132559054f,-0.004700761205697934f,-0.010505962123640127f),to_float3(-0.0015837570621268875f,-0.0050715032025820585f,-0.011298572863887848f),to_float3(-0.001710577244168271f,-0.005468996194229675f,-0.01214381456744096f),to_float3(-0.0018468494397243054f,-0.005895010992201898f,-0.01304459906634993f),to_float3(-0.0019932435949786592f,-0.006351429758636426f,-0.014003950941382284f),to_float3(-0.0021504779703164487f,-0.006840253416027218f,-0.015025006862317022f),to_float3(-0.0023193231689242525f,-0.007363609667412167f,-0.016111014032915174f),to_float3(-0.002500606595033479f,-0.007923761690353888f,-0.017265327560358507f),to_float3(-0.002695217398648073f,-0.008523117575376556f,-0.018491406534454295f),to_float3(-0.002904111972299025f,-0.009164240587639544f,-0.019792808560418648f),to_float3(-0.0031283200755794476f,-0.00984986033967067f,-0.021173182439071275f),to_float3(-0.003368951675232755f,-0.01058288497304438f,-0.022636258627945997f),to_float3(-0.0036272046027564153f,-0.01136641445806573f,-0.02418583704385647f),to_float3(-0.0039043731482907218f,-0.012203755132905171f,-0.025825771679025524f),to_float3(-0.004201857729535902f,-0.013098435617310009f,-0.027559951395473018f),to_float3(-0.00452117579826187f,-0.01405422425106731f,-0.029392276131583135f),to_float3(-0.00486397417548751f,-0.015075148223856253f,-0.0313266275951637f),to_float3(-0.005232043040666073f,-0.016165514581003473f,-0.03336683332200887f),to_float3(-0.00562733184154224f,-0.017329933308865297f,-0.03551662273933928f),to_float3(-0.0060519674414129645f,-0.018573342723915864f,-0.037779573578614306f),to_float3(-0.006508274881439065f,-0.01990103741075051f,-0.040159046618245986f),to_float3(-0.006998801210110017f,-0.021318698975493443f,-0.0426581062860751f),to_float3(-0.007526342923405148f,-0.022832429901508428f,-0.04527942409155754f),to_float3(-0.008093977672062791f,-0.024448790812264518f,-0.048025161159515134f),to_float3(-0.008705101032420682f,-0.026174841459272888f,-0.05089682526368315f),to_float3(-0.009363469312043276f,-0.028018185757524506f,-0.05389509666073895f),to_float3(-0.010073249580677117f,-0.029987021181332632f,-0.05701961564096375f),to_float3(-0.01083907839404583f,-0.03209019280176492f,-0.06026872295763385f),to_float3(-0.011666131030046848f,-0.03433725218081176f,-0.0636391420650306f),to_float3(-0.012560203507485332f,-0.03673852121908325f,-0.06712558924087116f),to_float3(-0.013527810238371974f,-0.03930516085637037f,-0.07072029400179898f),to_float3(-0.014576300919535715f,-0.04204924420785956f,-0.07441240748450588f),to_float3(-0.015714001257992345f,-0.04498383322316428f,-0.07818727031588318f),to_float3(-0.016950383431117873f,-0.04812305719009568f,-0.08202550346745388f),to_float3(-0.018296273925712297f,-0.051482190232598785f,-0.08590187504816119f),to_float3(-0.01976410874874513f,-0.05507772316147769f,-0.0897838820674849f),to_float3(-0.02136824920173702f,-0.05892742230067349f,-0.09362996770370406f),to_float3(-0.02312537581916243f,-0.06305036371943105f,-0.09738726988920325f),to_float3(-0.025054984222680283f,-0.06746692484105196f,-0.10098876378585503f),to_float3(-0.02718001534382911f,-0.07219870536957633f,-0.10434961578501721f),to_float3(-0.02952766495752478f,-0.07726833375162516f,-0.10736250560624028f),to_float3(-0.03213043568796663f,-0.08269909044537457f,-0.10989158977713533f),to_float3(-0.03502752169832732f,-0.08851423913888098f,-0.11176466592987913f),to_float3(-0.03826665722222464f,-0.09473589144745072f,-0.11276294193733785f),to_float3(-0.041906623421512364f,-0.10138312131867837f,-0.11260760327593745f),to_float3(-0.04602070835716154f,-0.10846885934496983f,-0.11094209169468648f),to_float3(-0.050701578024441495f,-0.1159947726771164f,-0.10730865033138975f),to_float3(-0.05606828993205902f,-0.12394275393722222f,-0.10111727591157052f),to_float3(-0.06227665518595623f,-0.13226056303078285f,-0.09160486258770702f),to_float3(-0.06953501099043398f,-0.14083708737623918f,-0.077782417479591f),to_float3(-0.07812908130749861f,-0.149458503599576f,-0.058369977232929894f),to_float3(-0.08846282054472075f,-0.15772775580732623f,-0.031725854990398726f),to_float3(-0.10112895949827036f,-0.1649097516191365f,0.004199908847829801f),to_float3(-0.11703860203562352f,-0.16961595360355963f,0.05178318985988212f),to_float3(-0.13767855574664672f,-0.16911182932471974f,0.11316073490042394f),to_float3(-0.16567645398825667f,-0.15763971441508162f,0.18793345990415217f),to_float3(-0.2062192509611813f,-0.12179330842214287f,0.2642830614724927f),to_float3(-0.2713074094027817f,-0.02537420724641723f,0.28182569573805677f),to_float3(-0.395760620255607f,0.2520387049261719f,-0.04827629938430805f),to_float3(-0.4794057541719356f,0.45941736433304614f,-0.35998402950121244f),to_float3(-0.39576062025560704f,0.25203870492617186f,-0.04827629938430784f),to_float3(-0.27130740940278164f,-0.02537420724641726f,0.2818256957380568f),to_float3(-0.20621925096118135f,-0.12179330842214289f,0.2642830614724926f),to_float3(-0.16567645398825667f,-0.15763971441508165f,0.18793345990415228f),to_float3(-0.13767855574664675f,-0.1691118293247198f,0.1131607349004239f),to_float3(-0.11703860203562354f,-0.1696159536035597f,0.05178318985988209f),to_float3(-0.10112895949827036f,-0.1649097516191365f,0.004199908847829665f),to_float3(-0.0884628205447208f,-0.1577277558073263f,-0.03172585499039877f),to_float3(-0.07812908130749864f,-0.14945850359957605f,-0.05836997723292993f),to_float3(-0.06953501099043398f,-0.14083708737623918f,-0.07778241747959105f),to_float3(-0.06227665518595623f,-0.13226056303078287f,-0.09160486258770703f),to_float3(-0.05606828993205902f,-0.12394275393722226f,-0.10111727591157058f),to_float3(-0.050701578024441495f,-0.11599477267711644f,-0.10730865033138978f),to_float3(-0.04602070835716156f,-0.10846885934496989f,-0.1109420916946865f),to_float3(-0.04190662342151239f,-0.10138312131867841f,-0.11260760327593748f),to_float3(-0.03826665722222467f,-0.09473589144745076f,-0.11276294193733784f),to_float3(-0.03502752169832737f,-0.08851423913888105f,-0.1117646659298792f),to_float3(-0.032130435687966606f,-0.0826990904453746f,-0.10989158977713538f),to_float3(-0.029527664957524794f,-0.0772683337516252f,-0.10736250560624033f),to_float3(-0.027180015343829116f,-0.07219870536957636f,-0.1043496157850173f),to_float3(-0.025054984222680304f,-0.06746692484105203f,-0.10098876378585511f),to_float3(-0.023125375819162436f,-0.06305036371943108f,-0.09738726988920338f),to_float3(-0.02136824920173702f,-0.058927422300673514f,-0.09362996770370416f),to_float3(-0.019764108748745155f,-0.05507772316147772f,-0.08978388206748496f),to_float3(-0.01829627392571232f,-0.05148219023259884f,-0.08590187504816127f),to_float3(-0.01695038343111789f,-0.04812305719009571f,-0.08202550346745398f),to_float3(-0.015714001257992355f,-0.0449838332231643f,-0.07818727031588321f),to_float3(-0.014576300919535724f,-0.04204924420785958f,-0.07441240748450599f),to_float3(-0.013527810238371971f,-0.039305160856370404f,-0.07072029400179902f),to_float3(-0.012560203507485332f,-0.036738521219083255f,-0.06712558924087117f),to_float3(-0.011666131030046859f,-0.03433725218081179f,-0.0636391420650307f),to_float3(-0.01083907839404584f,-0.03209019280176495f,-0.06026872295763393f),to_float3(-0.010073249580677119f,-0.029987021181332653f,-0.05701961564096378f),to_float3(-0.009363469312043281f,-0.02801818575752456f,-0.05389509666073898f),to_float3(-0.008705101032420694f,-0.026174841459272933f,-0.0508968252636832f),to_float3(-0.008093977672062803f,-0.024448790812264518f,-0.04802516115951517f),to_float3(-0.007526342923405146f,-0.02283242990150845f,-0.04527942409155761f),to_float3(-0.006998801210110013f,-0.021318698975493443f,-0.04265810628607512f),to_float3(-0.006508274881439066f,-0.01990103741075051f,-0.040159046618246f),to_float3(-0.00605196744141298f,-0.018573342723915892f,-0.03777957357861436f),to_float3(-0.005627331841542247f,-0.017329933308865328f,-0.03551662273933936f),to_float3(-0.005232043040666078f,-0.016165514581003487f,-0.033366833322008904f),to_float3(-0.004863974175487524f,-0.015075148223856267f,-0.031326627595163734f),to_float3(-0.004521175798261876f,-0.01405422425106733f,-0.029392276131583166f),to_float3(-0.004201857729535902f,-0.013098435617310021f,-0.027559951395473042f),to_float3(-0.0039043731482907213f,-0.012203755132905178f,-0.025825771679025535f),to_float3(-0.003627204602756424f,-0.01136641445806575f,-0.024185837043856497f),to_float3(-0.003368951675232753f,-0.010582884973044387f,-0.022636258627946024f),to_float3(-0.0031283200755794494f,-0.009849860339670675f,-0.021173182439071295f),to_float3(-0.002904111972299031f,-0.009164240587639563f,-0.019792808560418675f),to_float3(-0.002695217398648074f,-0.00852311757537658f,-0.01849140653445433f),to_float3(-0.002500606595033485f,-0.007923761690353899f,-0.017265327560358527f),to_float3(-0.0023193231689242495f,-0.007363609667412189f,-0.016111014032915188f),to_float3(-0.002150477970316447f,-0.006840253416027219f,-0.01502500686231705f),to_float3(-0.001993243594978657f,-0.006351429758636433f,-0.014003950941382294f),to_float3(-0.0018468494397243095f,-0.005895010992201904f,-0.013044599066349954f),to_float3(-0.0017105772441682716f,-0.005468996194229684f,-0.012143814567440982f),to_float3(-0.0015837570621268916f,-0.0050715032025820655f,-0.011298572863887869f),to_float3(-0.0014657636132559086f,-0.004700761205697936f,-0.010505962123640147f),to_float3(-0.0013560129718921034f,-0.004355103886359678f,-0.009763183179221696f),to_float3(-0.0012539595555377642f,-0.004032963067987664f,-0.009067548827244698f),to_float3(-0.0011590933801404499f,-0.003732862817675913f,-0.00841648261905499f),to_float3(-0.0010709375533892564f,-0.003453413964866797f,-0.007807517233220275f),to_float3(-0.0009890459807688297f,-0.0031933089987567142f,-0.007238292506523384f),to_float3(-0.0009130012621639455f,-0.002951317311279133f,-0.006706553188329679f),to_float3(-0.0008424127594587336f,-0.002726280755872182f,-0.006210146473284956f),to_float3(-0.0007769148178819765f,-0.002517109495246948f,-0.005747019358954837f),to_float3(-0.0007161651258619422f,-0.002322778114066351f,-0.005315215867981134f),to_float3(-0.0006598431999115193f,-0.0021423219748559342f,-0.004912874168389099f),to_float3(-0.0006076489826017314f,-0.0019748337976268505f,-0.004538223620654547f),to_float3(-0.0005593015430286124f,-0.001819460445624252f,-0.004189581775882171f),to_float3(-0.0005145378703601023f,-0.001675399901344218f,-0.00386535134583253f),to_float3(-0.00047311175208774605f,-0.001541898418511293f,-0.003564017162461541f),to_float3(-0.0004347927295209686f,-0.0014182478370946185f,-0.003284143142016918f),to_float3(-0.0003993651238658139f,-0.0013037830496811053f,-0.003024369266498018f));
    float3 p_x[151] = vec3[](to_float3(-0.002137124133264062f,-0.0045652949750816674f,-0.007944999489624014f),to_float3(-0.002300156955533428f,-0.004908806940460253f,-0.008525633362203022f),to_float3(-0.0024740180160609526f,-0.0052745508615946735f,-0.009141755140060714f),to_float3(-0.0026593024187144994f,-0.005663677685852353f,-0.009794948578414675f),to_float3(-0.0028566298846672803f,-0.006077377342767512f,-0.010486819494099436f),to_float3(-0.0030666454073479945f,-0.006516878738501725f,-0.011218991293007716f),to_float3(-0.0032900199322388997f,-0.0069834496606139915f,-0.011993099926857181f),to_float3(-0.0035274510673012965f,-0.007478396589859675f,-0.012810788233390612f),to_float3(-0.0037796638307912127f,-0.008003064415882877f,-0.013673699609992897f),to_float3(-0.004047411444335345f,-0.008558836053786144f,-0.014583470965923965f),to_float3(-0.004331476180377802f,-0.009147131958638085f,-0.015541724892891462f),to_float3(-0.00463267027449884f,-0.009769409534995865f,-0.01655006098737826f),to_float3(-0.004951836914666999f,-0.010427162438453347f,-0.017610046250856503f),to_float3(-0.005289851321237063f,-0.011121919766049194f,-0.018723204485601174f),to_float3(-0.005647621933474761f,-0.011855245132051603f,-0.019891004594067244f),to_float3(-0.006026091720604429f,-0.012628735625135244f,-0.02111484767848965f),to_float3(-0.0064262396378738365f,-0.013444020642236373f,-0.02239605282424208f),to_float3(-0.006849082250952507f,-0.014302760593353525f,-0.02373584143523893f),to_float3(-0.007295675555175712f,-0.015206645470184573f,-0.025135319971921f),to_float3(-0.0077671170197740995f,-0.01615739326966947f,-0.026595460921701564f),to_float3(-0.008264547891358274f,-0.01715674826113821f,-0.02811708180766433f),to_float3(-0.008789155795641856f,-0.0182064790827157f,-0.02970082201319753f),to_float3(-0.009342177681784285f,-0.019308376648754934f,-0.031347117167421494f),to_float3(-0.00992490315993552f,-0.02046425184516241f,-0.03305617079787943f),to_float3(-0.010538678289711445f,-0.021675932983309377f,-0.03482792291202483f),to_float3(-0.011184909885595262f,-0.022945262975493885f,-0.036662015116369455f),to_float3(-0.011865070414853763f,-0.024274096185269295f,-0.03855775182034264f),to_float3(-0.01258070357473496f,-0.025664294893927936f,-0.040514056999271006f),to_float3(-0.013333430648782561f,-0.02711772530945963f,-0.042529425905391674f),to_float3(-0.01412495775744369f,-0.028636253025678345f,-0.04460187101503015f),to_float3(-0.014957084136223639f,-0.030221737816030457f,-0.04672886138108205f),to_float3(-0.01583171159602881f,-0.031876027617728446f,-0.04890725441919534f),to_float3(-0.016750855345744924f,-0.03360095152585251f,-0.051133218989284704f),to_float3(-0.01771665638740403f,-0.03539831157208638f,-0.0534021484360339f),to_float3(-0.018731395730599044f,-0.03726987300644707f,-0.055708562016558806f),to_float3(-0.019797510716488533f,-0.0392173527296855f,-0.05804599286274144f),to_float3(-0.020917613794531456f,-0.04124240543505191f,-0.06040686029055546f),to_float3(-0.02209451415920214f,-0.043346606905710634f,-0.06278232386756345f),to_float3(-0.023331242732158364f,-0.045531433771570005f,-0.06516211616871623f),to_float3(-0.024631081071251157f,-0.04779823884785927f,-0.06753435057251908f),to_float3(-0.02599759490599191f,-0.05014822094579368f,-0.06988529975365311f),to_float3(-0.02743467314560627f,-0.05258238774762672f,-0.07219913968862342f),to_float3(-0.028946573388404322f,-0.05510150995351754f,-0.07445765297670581f),to_float3(-0.03053797519013025f,-0.05770606440794075f,-0.07663988405127538f),to_float3(-0.03221404263776634f,-0.0603961632607468f,-0.07872173737035694f),to_float3(-0.033980498142029227f,-0.06317146536031418f,-0.08067550787447306f),to_float3(-0.035843709830794705f,-0.06603106494154067f,-0.08246933081865009f),to_float3(-0.03781079552984435f,-0.0689733511594637f,-0.08406653544718634f),to_float3(-0.03988974710170204f,-0.07199582998922682f,-0.08542488379813157f),to_float3(-0.042089579940160424f,-0.07509489726503166f,-0.08649567210767374f),to_float3(-0.044420513774308154f,-0.0782655478780229f,-0.0872226677458533f),to_float3(-0.046894192744119025f,-0.0815010009801992f,-0.08754084929384119f),to_float3(-0.04952395514522684f,-0.08479221383871625f,-0.08737491127852688f),to_float3(-0.052325166556904924f,-0.08812724684632407f,-0.0866374883735367f),to_float3(-0.05531563463635159f,-0.09149042775415551f,-0.08522704702612285f),to_float3(-0.05851613023760088f,-0.09486124236558141f,-0.0830253865465934f),to_float3(-0.06195104853165206f,-0.09821284847174405f,-0.0798946889222969f),to_float3(-0.06564925675340012f,-0.10151006459451122f,-0.07567406140614437f),to_float3(-0.06964519409927278f,-0.1047066168725813f,-0.07017553687965904f),to_float3(-0.07398031737724835f,-0.10774132259935602f,-0.06317955075217202f),to_float3(-0.0787050285543731f,-0.11053272464789683f,-0.054430032064937746f),to_float3(-0.08388128622421473f,-0.11297142788834234f,-0.0436294939520135f),to_float3(-0.08958620751898465f,-0.1149089568361548f,-0.03043501110604133f),to_float3(-0.09591713730219445f,-0.11614122538666281f,-0.014456986894158296f),to_float3(-0.10299894760598283f,-0.11638344275691273f,0.004735321117216877f),to_float3(-0.11099482783905858f,-0.11523099884933204f,0.027593267367261202f),to_float3(-0.12012272629777299f,-0.1120965987947297f,0.054535916053032214f),to_float3(-0.13068130718798532f,-0.10610553463683693f,0.08582732348023998f),to_float3(-0.1430926878027598f,-0.09591364237278834f,0.12130023611322262f),to_float3(-0.15797644463814361f,-0.07937430036055836f,0.15972950012046713f),to_float3(-0.17628590900450966f,-0.05289019694443883f,0.1973636686263485f),to_float3(-0.19957914170178254f,-0.010050447660950014f,0.22428672225770246f),to_float3(-0.23061230582092926f,0.06152568473621712f,0.21465485784075855f),to_float3(-0.2748091985304043f,0.18844826204726473f,0.09754352781288295f),to_float3(-0.3445128239871801f,0.4367329316543684f,-0.34280145660982947f),to_float3(3.01878863857207e-17,2.1657391732312188e-17,2.1073950360688117e-17),to_float3(0.3445128239871802f,-0.4367329316543684f,0.3428014566098292f),to_float3(0.27480919853040436f,-0.18844826204726473f,-0.09754352781288317f),to_float3(0.23061230582092923f,-0.061525684736217036f,-0.21465485784075863f),to_float3(0.19957914170178256f,0.01005044766095007f,-0.2242867222577026f),to_float3(0.17628590900450966f,0.0528901969444389f,-0.19736366862634852f),to_float3(0.15797644463814361f,0.07937430036055836f,-0.15972950012046708f),to_float3(0.14309268780275983f,0.09591364237278836f,-0.1213002361132226f),to_float3(0.13068130718798535f,0.10610553463683695f,-0.08582732348023968f),to_float3(0.12012272629777299f,0.11209659879472973f,-0.054535916053032235f),to_float3(0.11099482783905856f,0.11523099884933206f,-0.027593267367261167f),to_float3(0.10299894760598281f,0.11638344275691272f,-0.004735321117216822f),to_float3(0.09591713730219444f,0.11614122538666284f,0.014456986894158358f),to_float3(0.08958620751898468f,0.11490895683615483f,0.03043501110604141f),to_float3(0.08388128622421473f,0.11297142788834233f,0.04362949395201367f),to_float3(0.0787050285543731f,0.11053272464789683f,0.054430032064937794f),to_float3(0.07398031737724835f,0.10774132259935604f,0.06317955075217199f),to_float3(0.06964519409927278f,0.10470661687258127f,0.07017553687965912f),to_float3(0.06564925675340012f,0.10151006459451126f,0.07567406140614436f),to_float3(0.06195104853165206f,0.09821284847174405f,0.0798946889222969f),to_float3(0.05851613023760088f,0.09486124236558141f,0.08302538654659344f),to_float3(0.05531563463635159f,0.09149042775415551f,0.08522704702612297f),to_float3(0.05232516655690491f,0.08812724684632407f,0.08663748837353664f),to_float3(0.04952395514522684f,0.08479221383871625f,0.08737491127852687f),to_float3(0.046894192744119025f,0.08150100098019919f,0.0875408492938412f),to_float3(0.044420513774308154f,0.0782655478780229f,0.0872226677458533f),to_float3(0.04208957994016041f,0.07509489726503163f,0.08649567210767378f),to_float3(0.03988974710170204f,0.07199582998922681f,0.08542488379813157f),to_float3(0.037810795529844336f,0.0689733511594637f,0.08406653544718634f),to_float3(0.03584370983079468f,0.06603106494154064f,0.0824693308186501f),to_float3(0.03398049814202922f,0.06317146536031418f,0.08067550787447306f),to_float3(0.03221404263776634f,0.0603961632607468f,0.07872173737035698f),to_float3(0.03053797519013025f,0.05770606440794075f,0.07663988405127538f),to_float3(0.02894657338840431f,0.055101509953517515f,0.07445765297670578f),to_float3(0.02743467314560627f,0.05258238774762672f,0.07219913968862343f),to_float3(0.025997594905991905f,0.05014822094579368f,0.06988529975365308f),to_float3(0.024631081071251146f,0.04779823884785925f,0.06753435057251908f),to_float3(0.02333124273215836f,0.04553143377157f,0.06516211616871614f),to_float3(0.022094514159202137f,0.04334660690571063f,0.06278232386756344f),to_float3(0.020917613794531453f,0.041242405435051886f,0.060406860290555434f),to_float3(0.019797510716488522f,0.039217352729685476f,0.05804599286274142f),to_float3(0.01873139573059904f,0.03726987300644707f,0.05570856201655882f),to_float3(0.017716656387404026f,0.035398311572086366f,0.05340214843603387f),to_float3(0.016750855345744917f,0.0336009515258525f,0.051133218989284704f),to_float3(0.015831711596028804f,0.03187602761772843f,0.04890725441919533f),to_float3(0.01495708413622363f,0.030221737816030454f,0.04672886138108203f),to_float3(0.014124957757443688f,0.028636253025678342f,0.04460187101503013f),to_float3(0.013333430648782552f,0.02711772530945961f,0.04252942590539163f),to_float3(0.012580703574734953f,0.025664294893927922f,0.040514056999270985f),to_float3(0.011865070414853759f,0.024274096185269285f,0.03855775182034264f),to_float3(0.011184909885595262f,0.022945262975493878f,0.036662015116369455f),to_float3(0.010538678289711445f,0.02167593298330937f,0.0348279229120248f),to_float3(0.009924903159935513f,0.020464251845162408f,0.03305617079787941f),to_float3(0.00934217768178428f,0.019308376648754923f,0.03134711716742148f),to_float3(0.00878915579564185f,0.01820647908271569f,0.029700822013197525f),to_float3(0.008264547891358267f,0.017156748261138197f,0.028117081807664302f),to_float3(0.007767117019774098f,0.016157393269669456f,0.026595460921701543f),to_float3(0.0072956755551757f,0.015206645470184556f,0.02513531997192098f),to_float3(0.0068490822509524995f,0.014302760593353517f,0.023735841435238908f),to_float3(0.006426239637873832f,0.013444020642236359f,0.022396052824242063f),to_float3(0.006026091720604427f,0.012628735625135236f,0.02111484767848964f),to_float3(0.0056476219334747595f,0.011855245132051592f,0.019891004594067244f),to_float3(0.005289851321237059f,0.01112191976604919f,0.018723204485601167f),to_float3(0.0049518369146669934f,0.010427162438453336f,0.017610046250856482f),to_float3(0.004632670274498837f,0.009769409534995856f,0.016550060987378257f),to_float3(0.004331476180377796f,0.009147131958638074f,0.015541724892891445f),to_float3(0.004047411444335342f,0.008558836053786137f,0.014583470965923953f),to_float3(0.0037796638307912088f,0.008003064415882869f,0.013673699609992889f),to_float3(0.0035274510673012917f,0.007478396589859665f,0.012810788233390602f),to_float3(0.003290019932238895f,0.006983449660613981f,0.011993099926857178f),to_float3(0.0030666454073479924f,0.006516878738501718f,0.011218991293007703f),to_float3(0.002856629884667277f,0.006077377342767509f,0.01048681949409938f),to_float3(0.0026593024187144963f,0.005663677685852356f,0.00979494857841503f),to_float3(0.0024740180160610133f,0.0052745508615947585f,0.009141755140060302f),to_float3(0.0023001569555331914f,0.004908806940460417f,0.00852563336220343f),to_float3(0.0021371241332640567f,0.0045652949750816605f,0.007944999489623995f));
    float s_i[3] = float[](0.2424503566193514f,0.10170785486914974f,0.03723335168874466f);
    float g_x[151] = float[](-0.012893115592183313f,-0.013698670060406285f,-0.01454270853033401f,-0.015426186340698034f,-0.01635001813564313f,-0.01731507233271321f,-0.018322165451038992f,-0.01937205631983224f,-0.020465440189183726f,-0.021602942767020888f,-0.022785114207886127f,-0.024012423080935617f,-0.025285250346210764f,-0.02660388336978562f,-0.02796851000982456f,-0.029379212806879423f,-0.030835963312895207f,-0.03233861659436575f,-0.03388690594586321f,-0.03548043785074925f,-0.03711868722624115f,-0.03880099299014412f,-0.04052655398645646f,-0.0422944253066969f,-0.04410351504318662f,-0.04595258150963188f,-0.047840230963193016f,-0.0497649158607877f,-0.0517249336806611f,-0.05371842633826146f,-0.05574338022319264f,-0.057797626881478534f,-0.059878844364578875f,-0.06198455926355096f,-0.06411214944346842f,-0.06625884748970856f,-0.068421744874013f,-0.07059779684434063f,-0.07278382803848439f,-0.07497653881724314f,-0.07717251230864798f,-0.0793682221503752f,-0.08156004091305374f,-0.08374424918274104f,-0.0859170452764106f,-0.08807455555991833f,-0.09021284533361659f,-0.09232793024659676f,-0.0944157881965094f,-0.09647237166805361f,-0.09849362045958987f,-0.10047547474393566f,-0.10241388840629213f,-0.10430484259943712f,-0.10614435945385038f,-0.10792851587831802f,-0.109653457384833f,-0.11131541187027436f,-0.11291070328643685f,-0.11443576512950307f,-0.11588715368001017f,-0.1172615609247814f,-0.11855582709315698f,-0.11976695274118251f,-0.12089211031918369f,-0.12192865516037081f,-0.12287413583076501f,-0.12372630378379504f,-0.12448312226638165f,-0.12514277442715682f,-0.1257036705816586f,-0.12616445459385237f,-0.126524009338131f,-0.12678146121101333f,-0.12693618366704282f,-0.1269877997588608f,-0.12693618366704285f,-0.12678146121101333f,-0.12652400933813102f,-0.12616445459385237f,-0.12570367058165863f,-0.12514277442715685f,-0.12448312226638174f,-0.12372630378379511f,-0.12287413583076504f,-0.12192865516037085f,-0.1208921103191837f,-0.11976695274118256f,-0.11855582709315705f,-0.11726156092478153f,-0.11588715368001025f,-0.11443576512950313f,-0.1129107032864369f,-0.11131541187027441f,-0.10965345738483308f,-0.10792851587831806f,-0.10614435945385046f,-0.10430484259943716f,-0.10241388840629222f,-0.1004754747439357f,-0.09849362045958993f,-0.09647237166805367f,-0.09441578819650946f,-0.09232793024659687f,-0.09021284533361669f,-0.08807455555991844f,-0.08591704527641067f,-0.08374424918274113f,-0.08156004091305383f,-0.07936822215037528f,-0.07717251230864812f,-0.07497653881724321f,-0.07278382803848449f,-0.07059779684434074f,-0.06842174487401309f,-0.06625884748970862f,-0.06411214944346849f,-0.06198455926355104f,-0.05987884436457899f,-0.057797626881478596f,-0.05574338022319272f,-0.05371842633826156f,-0.05172493368066117f,-0.04976491586078776f,-0.047840230963193085f,-0.04595258150963194f,-0.04410351504318669f,-0.04229442530669699f,-0.040526553986456555f,-0.03880099299014417f,-0.0371186872262412f,-0.03548043785074932f,-0.033886905945863265f,-0.03233861659436582f,-0.03083596331289527f,-0.029379212806879485f,-0.02796851000982462f,-0.026603883369785666f,-0.02528525034621083f,-0.02401242308093568f,-0.022785114207886165f,-0.02160294276702093f,-0.020465440189183767f,-0.019372056319832277f,-0.018322165451039044f,-0.017315072332713243f,-0.016350018135643175f,-0.015426186340698074f,-0.014542708530334052f,-0.013698670060406317f,-0.012893115592183577f);


    #define RANGE 75
    
    float2 P = to_float2(0);
    float G = 0.0f;
    float Gw = 0.0f;
    for (int i = -RANGE; i <= RANGE; i++) {
        int index = RANGE + i;
        
        float4 tx = Po(0,i);
        float2 t1 = unpack2x16(tx.x);
        float2 t2 = unpack2x16(tx.y);
        float2 t3 = unpack2x16(tx.z);

        float g = tx.w;
        
        float3 py = p_y[index];
        float3 px = p_x[index];
        
        P += s_i[0] * to_float2(px.x, py.x).yx * t1;
        P += s_i[1] * to_float2(px.y, py.y).yx * t2;
        P += s_i[2] * to_float2(px.z, py.z).yx * t3;
        Gw += _fabs(g_x[index]);
        G  += _fabs(g_x[index]) * g;
    }
    
    G /= Gw;

    if(reset()) {
        fragColor = to_float4(0);
    } else {
        float2 com_n;
        if (FRAME_MOD(0)) {
            com_n = com(fragCoord, iChannel2, iChannel1);
        } else {
            com_n = textureLod(iChannel1, uv, 0.0f).zw;
        }
        fragColor = to_float4_aw(to_float2((P.x + P.y) + G),com_n);
    }



  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel1
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


bool reset() {
    return iFrame <= 1 || texture(iChannel3, to_float2(32.5f/256.0f, 0.5f) ).x > 0.5f;
}

__KERNEL__ void HybridVirtualParticleFluidFuse__Buffer_D(float4 c, float2 p, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel3)
{

    initialize(p, iFrame, iResolution);
    Vec4Neighborhood pn = GetVec4Neighborhood(iChannel0);
    float4 U = texelFetch(iChannel1, to_int2(p), 0);
    float2 dp = Delta(pn, 0);
    c = U + to_float4(dp,0,0)/2.0f;
    
    if (reset()) {
        c = to_float4(0,0,INIT_MASS,0);
    }


  SetFragmentShaderComputedColor(c);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: Forest Blurred_0' to iChannel0
// Connect Image 'Previsualization: Buffer A' to iChannel1
// Connect Image 'Previsualization: Buffer B' to iChannel3
// Connect Image 'Previsualization: Buffer C' to iChannel2


/*
License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.

***Click to paint.***

Automatic mouse movement can be turned off using the ENABLE_AUTO_MOUSE #define
The bounding box can be turned off using the ENABLE_BOUNDS #define.

This is a hybrid fluid simulation that combines both forward and reverse
advection techniques to achieve a high-quality pressure solution,
accurate advection, and decent performance while also remaining 
conservative unless under extreme velocities. The virtual particle
method used here is based on Michael Moroz' Reintegration Tracking 
method, extended to support Gaussian particle kernels:
https://www.shadertoy.com/view/WtfyDj

In the low-velocity regime, forward advection with virtual particles is
used. Virtual particle size is controlled according to the magnitude of
velocity; low velocity particles increase in size while high velocity
particle decrease in size. The virtual particle size scaling measure
can be changed using the VIRTUAL_PARTICLE_SIZE #define.

Virtual particles use a gaussian kernel. In order to conserve velocity
and mass in forward advection, masses and velocities from neighboring
particles are accumulated according to box integrals of the error function
(approximated here as tanh, but a more accurate approximation is provided
by toggling the USE_TANH #define). Both the integral and center of mass
of box intersections with a gaussian kernel are computed here.

When particle velocities exceed the forward advection integration range, 
reverse advection is used, using the RK4 method. Using this method,
it is possible to achieve forward advection without also setting a hard
upper bound on velocity.

The Poisson pressure solver kernel used here is precomputed using a custom solver.
First, a 2D kernel is computed, then a separable kernel is derived using
Singular Value Decomposition. The separable kernel method used here can
achieve a nearly-perfect pressure solve in 4 steps, but a single step is
used here for interactivity. The number of pressure solver steps per
fluid solver steps can be changed with the FRAME_DIVIDER #define (set to 1
by default, but can be changed to 4 for a high quality pressure solve).

Additional methods are implemented here in order to increase fluid detail.
This simulation implements multiscale Vorticity Confinement, a kernel-based
turbulence method based on my earlier Multiscale MIP Fluid simulation:
https://www.shadertoy.com/view/tdVSDh
and Florian Berger's work:
https://www.shadertoy.com/view/MsGSRd
This work also implements multiscale viscosity. The size and shape of
the kernels for these methods can be changed using the
MULTISCALE_KERNEL_POWER and MULTISCALE_KERNEL_STEPS #defines.


*/

#define THIN_FILM
#ifdef NORMAL
__KERNEL__ void HybridVirtualParticleFluidFuse(float4 c, float2 p, float iTime, float2 iResolution, int iFrame, float4 iDate, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
    
    initialize(p, iFrame, iResolution);

    float4 fluid = _tex2DVecN(iChannel1,uv.x,uv.y,15);
    #ifdef USE_VORTICITY
        float v = 0.5f*fluid.w + 0.5f;
    #else
        float v = fluid.w;
    #endif
    c = v*(0.5f + 1.0f*fluid);
    
    float4 curlcol =  _mix(to_float4(1,0,0,0),to_float4(0,0,1,0),smoothstep(0.0f,1.0f,fluid.w + 0.5f));
    curlcol = _mix(to_float4(1), curlcol, smoothstep(0.0f,1.0f,_powf(_fabs(4.0f*fluid.w),0.5f)));
    
    float p0 = textureLod(iChannel1, uv, 0.0f).x;
    float p1 = textureLod(iChannel1, uv, 12.0f).x;
    float h = smoothstep(-1.0f,1.0f, 0.2f*(p0-p1));
    c = to_float4(smoothstep(-0.4f,1.2f,2.0f*h * length(swi2(fluid,x,y)) * curlcol));
    //c = to_float4(length(swi2(fluid,x,y)));
    c = swi4(fluid,z,z,z,z);
    
    float2 comt = textureLod(iChannel2, uv, 0.0f).zw;
    c = to_float4_aw(4.0f*comt + 0.5f,8.0f*length(comt),0);
}
#endif

#ifdef HEIGHT
__KERNEL__ void HybridVirtualParticleFluidFuse(float4 c, float2 p, float iTime, float2 iResolution, int iFrame, float4 iDate, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
    
    initialize(p, iFrame, iResolution);
    
    float4 tx = texelFetch(iChannel3, to_int2(p), 0);
    float2 t1 = unpack2x16(tx.x);
    float2 t2 = unpack2x16(tx.y);
    float2 t3 = unpack2x16(tx.z);

    float height = _tex2DVecN(iChannel2,uv.x,uv.y,15).x;
    float4 fluid = _tex2DVecN(iChannel1,uv.x,uv.y,15);
    
    //c = 0.5f + 0.5f*to_float4(height);
    //c = length(swi2(fluid,x,y)) * (0.5f+fluid);
    //c = 0.3f*to_float4(swi3(fluid,z,z,z),1);
    c = 0.15f*to_float4(swi3(fluid,z,z,z),1) * to_float4(length(swi2(fluid,x,y)));
}
#endif


#ifdef THIN_FILM
/*
  Fast Thin-Film Interference

  This is a performance-optimized version of my previous 
  thin-film interference shader here: https://www.shadertoy.com/view/XddXRj
  This version also fixes a platform-specific bug and has
  a few other tweaks as well.

  Thin-film interference and chromatic dispersion are simulated at
  six different wavelengths and then downsampled to RGB.
*/

// To see just the reflection (no refraction/transmission) uncomment this next line:
//#define REFLECTANCE_ONLY

// performance and raymarching options
#define INTERSECTION_PRECISION 0.01f  // raymarcher intersection precision
#define ITERATIONS 20         // max number of iterations
#define AA_SAMPLES 1         // anti aliasing samples
#define BOUND 6.0f           // cube bounds check
#define DIST_SCALE 0.9f          // scaling factor for raymarching position update

// optical properties
#define DISPERSION 0.05f           // dispersion amount
#define IOR 0.9f              // base IOR value specified as a ratio
#define THICKNESS_SCALE 32.0f     // film thickness scaling factor
#define THICKNESS_CUBEMAP_SCALE 0.1f  // film thickness cubemap scaling factor
#define REFLECTANCE_SCALE 3.0f        // reflectance scaling factor
#define REFLECTANCE_GAMMA_SCALE 1.0f  // reflectance gamma scaling factor
#define FRESNEL_RATIO 0.1f       // fresnel weight for reflectance
#define SIGMOID_CONTRAST 10.0f         // contrast enhancement

#define GAMMA_CURVE 1.0
#define GAMMA_SCALE 1.0

#define TWO_PI 6.28318530718
#define WAVELENGTHS 6         // number of wavelengths, not a free parameter

// iq's cubemap function
__DEVICE__ float3 fancyCube( sampler2D sam, in float3 d, in float s, in float b )
{
    float3 colx = textureLod( sam, 0.5f + s*d.yz/d.x, b ).xyz;
    float3 coly = textureLod( sam, 0.5f + s*d.zx/d.y, b ).xyz;
    float3 colz = textureLod( sam, 0.5f + s*d.xy/d.z, b ).xyz;
    
    float3 n = d*d;
    
    return (colx*n.x + coly*n.y + colz*n.z)/(n.x+n.y+n.z);
}

// iq's 3D noise function
__DEVICE__ float hash( float n ){
    return fract(_sinf(n)*43758.5453f);
}

__DEVICE__ float noise( in float3 x ) {
    float3 p = _floor(x);
    float3 f = fract(x);

    f = f*f*(3.0f-2.0f*f);
    float n = p.x + p.y*57.0f + 113.0f*p.z;
    return _mix(mix(_mix( hash(n+  0.0f), hash(n+  1.0f),f.x),
                   _mix( hash(n+ 57.0f), hash(n+ 58.0f),f.x),f.y),
               _mix(mix( hash(n+113.0f), hash(n+114.0f),f.x),
                   _mix( hash(n+170.0f), hash(n+171.0f),f.x),f.y),f.z);
}

__DEVICE__ float3 noise3(float3 x) {
  return to_float3( noise(x+to_float3(123.456f,0.567f,0.37f)),
         noise(x+to_float3(0.11f,47.43f,19.17f)),
         noise(x) );
}

// a sphere with a little bit of warp
__DEVICE__ float sdf( float3 p ) {
  float3 n = to_float3_aw(_sinf(iDate.w * 0.5f), _sinf(iDate.w * 0.3f), _cosf(iDate.w * 0.2f));
  float3 q = 0.1f * (noise3(p + n) - 0.5f);
  
  return length(q + p) - 3.5f;
}

__DEVICE__ float3 fresnel( float3 rd, float3 norm, float3 n2 ) {
   float3 r0 = _powf((1.0f-n2)/(1.0f+n2), to_float3_aw(2));
   return r0 + (1.0f - r0)*_powf(clamp(1.0f + dot(rd, norm), 0.0f, 1.0f), 5.0f);
}

__DEVICE__ float3 calcNormal( in float3 pos ) {
    const float eps = INTERSECTION_PRECISION;

    const float3 v1 = to_float3( 1.0f,-1.0f,-1.0f);
    const float3 v2 = to_float3(-1.0f,-1.0f, 1.0f);
    const float3 v3 = to_float3(-1.0f, 1.0f,-1.0f);
    const float3 v4 = to_float3( 1.0f, 1.0f, 1.0f);

  return normalize( v1*sdf( pos + v1*eps ) + 
            v2*sdf( pos + v2*eps ) + 
            v3*sdf( pos + v3*eps ) + 
            v4*sdf( pos + v4*eps ) );
}

__DEVICE__ float3 filmic_gamma(float3 x) {
  return _logf(GAMMA_CURVE * x + 1.0f) / GAMMA_SCALE;    
}

__DEVICE__ float3 filmic_gamma_inverse(float3 y) {
  return (1.0f / GAMMA_CURVE) * (_expf(GAMMA_SCALE * y) - 1.0f); 
}

// sample weights for the cubemap given a wavelength i
// room for improvement in this function
#define GREEN_WEIGHT 2.8
__DEVICE__ float3 texCubeSampleWeights(float i) {
  float3 w = to_float3_aw((1.0f - i) * (1.0f - i), GREEN_WEIGHT * i * (1.0f - i), i * i);
    return w / dot(w, to_float3_s(1.0f));
}

__DEVICE__ float3 sampleCubeMap(float3 i, float3 rd) {
  float3 col = textureLod(iChannel0, rd * to_float3(1.0f,-1.0f,1.0f), 0.0f).xyz; 
    return to_float3_aw(
        dot(texCubeSampleWeights(i.x), col),
        dot(texCubeSampleWeights(i.y), col),
        dot(texCubeSampleWeights(i.z), col)
    );
}

__DEVICE__ float3 sampleCubeMap(float3 i, float3 rd0, float3 rd1, float3 rd2) {
  float3 col0 = textureLod(iChannel0, rd0 * to_float3(1.0f,-1.0f,1.0f), 0.0f).xyz;
    float3 col1 = textureLod(iChannel0, rd1 * to_float3(1.0f,-1.0f,1.0f), 0.0f).xyz; 
    float3 col2 = textureLod(iChannel0, rd2 * to_float3(1.0f,-1.0f,1.0f), 0.0f).xyz; 
    return to_float3_aw(
        dot(texCubeSampleWeights(i.x), col0),
        dot(texCubeSampleWeights(i.y), col1),
        dot(texCubeSampleWeights(i.z), col2)
    );
}



__DEVICE__ float3 sampleWeights(float i) {
  return to_float3_aw((1.0f - i) * (1.0f - i), GREEN_WEIGHT * i * (1.0f - i), i * i);
}

__DEVICE__ float3 resample(float3 wl0, float3 wl1, float3 i0, float3 i1) {
  float3 w0 = sampleWeights(wl0.x);
    float3 w1 = sampleWeights(wl0.y);
    float3 w2 = sampleWeights(wl0.z);
    float3 w3 = sampleWeights(wl1.x);
    float3 w4 = sampleWeights(wl1.y);
    float3 w5 = sampleWeights(wl1.z);
    
    return i0.x * w0 + i0.y * w1 + i0.z * w2
         + i1.x * w3 + i1.y * w4 + i1.z * w5;
}

// downsample to RGB
__DEVICE__ float3 resampleColor(vec3[WAVELENGTHS] rds, float3 refl0, float3 refl1, float3 wl0, float3 wl1) {

    
    #ifdef REFLECTANCE_ONLY
      float3 intensity0 = refl0;
      float3 intensity1 = refl1;
    #else
        float3 cube0 = sampleCubeMap(wl0, rds[0], rds[1], rds[2]);
      float3 cube1 = sampleCubeMap(wl1, rds[3], rds[4], rds[5]);
    
        float3 intensity0 = filmic_gamma_inverse(cube0) + refl0;
      float3 intensity1 = filmic_gamma_inverse(cube1) + refl1;
    #endif
    float3 col = resample(wl0, wl1, intensity0, intensity1);

    return col / float(WAVELENGTHS);
}

// compute the wavelength/IOR curve values.
__DEVICE__ float3 iorCurve(float3 x) {
  return x;
}

__DEVICE__ float3 attenuation(float filmThickness, float3 wavelengths, float3 normal, float3 rd) {
  return 0.5f + 0.5f * _cosf(((THICKNESS_SCALE * filmThickness)/(wavelengths + 1.0f)) * dot(normal, rd));    
}

__DEVICE__ float3 contrast(float3 x) {
  return 1.0f / (1.0f + _expf(-SIGMOID_CONTRAST * (x - 0.5f)));    
}

__DEVICE__ void doCamera( out float3 camPos, out float3 camTar, in float time, in float4 m ) {
    camTar = to_float3(0.0f,0.0f,0.0f); 
    if (_fmaxf(m.z, m.w) <= 0.0f) {
      float an = 1.5f + _sinf(time * 0.05f) * 4.0f;
    camPos = to_float3(6.5f*_sinf(an), 0.0f ,6.5f*_cosf(an));   
    } else {
      float an = 10.0f * m.x - 5.0f;
    camPos = to_float3(6.5f*_sinf(an),10.0f * m.y - 5.0f,6.5f*_cosf(an)); 
    }
}

__DEVICE__ mat3 calcLookAtMatrix( in float3 ro, in float3 ta, in float roll )
{
    float3 ww = normalize( ta - ro );
    float3 uu = normalize( cross(ww,to_float3_aw(_sinf(roll),_cosf(roll),0.0f) ) );
    float3 vv = normalize( cross(uu,ww));
    return mat3( uu, vv, ww );
}

__KERNEL__ void HybridVirtualParticleFluidFuse(float4 c, float2 p, float iTime, float2 iResolution, int iFrame, float4 iDate, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
   
    initialize(p, iFrame, iResolution);
    float3 col = to_float3_s(0.0f);
    
    Vec4Neighborhood pn = GetVec4Neighborhood(iChannel2);
    float2 dp = Delta(pn, 0);
    
    float3 wavelengths0 = to_float3(1.0f, 0.8f, 0.6f);
    float3 wavelengths1 = to_float3(0.4f, 0.2f, 0.0f);
    float3 iors0 = IOR + iorCurve(wavelengths0) * DISPERSION;
    float3 iors1 = IOR + iorCurve(wavelengths1) * DISPERSION;
    
    float3 rds[WAVELENGTHS];
    

    float3 normal = normalize(to_float3_aw(dp,10.0f));
    float3 nggx = normalize(to_float3_aw(dp,0.1f));
    
    /*
    mat3 camMat = calcLookAtMatrix( to_float3_aw(1.0f*(uv-0.5f),1), to_float3(1,0,-1), 0.0f );
    float3 rd = camMat*to_float3(0,0,1);*/
    
    #define TIME (0.05f*(15.0f*_sinf(iTime/30.0f)+60.0f))
    //#define TIME 16.2f+_sinf(0.05f*51.2f)
    float2 lookat = to_float2(_sinf(TIME*1.1f), _cosf(TIME));
    mat3 camMat = calcLookAtMatrix( to_float3(0,0,0), to_float3_aw(lookat,-1), PI );
    float3 rd = camMat*to_float3_aw(uv-0.5f,1.0f);
    
    float spec = 1.0f*ggx(nggx, normalize(rd), to_float3(0,1,8), 0.02f, 1.0f);

    float filmThickness = 0.1f+0.2f*textureLod(iChannel1, uv, 0.0f).z;

    float3 att0 = attenuation(filmThickness, wavelengths0, normal, rd);
    float3 att1 = attenuation(filmThickness, wavelengths1, normal, rd);

    float3 rrd = reflect(rd, normal);
    float3 f0 = (1.0f - FRESNEL_RATIO) + FRESNEL_RATIO * fresnel(rd, normal, 1.0f / iors0);
    float3 f1 = (1.0f - FRESNEL_RATIO) + FRESNEL_RATIO * fresnel(rd, normal, 1.0f / iors1);

    //vec3 rrd = reflect(rd, normal);

    float3 cube0 = REFLECTANCE_GAMMA_SCALE * att0 * filmic_gamma_inverse(sampleCubeMap(wavelengths0, rrd));
    float3 cube1 = REFLECTANCE_GAMMA_SCALE * att1 * filmic_gamma_inverse(sampleCubeMap(wavelengths1, rrd));

    float3 refl0 = REFLECTANCE_SCALE * _mix(to_float3(0), cube0, f0);
    float3 refl1 = REFLECTANCE_SCALE * _mix(to_float3(0), cube1, f1);

    rds[0] = refract(rd, normal, iors0.x);
    rds[1] = refract(rd, normal, iors0.y);
    rds[2] = refract(rd, normal, iors0.z);
    rds[3] = refract(rd, normal, iors1.x);
    rds[4] = refract(rd, normal, iors1.y);
    rds[5] = refract(rd, normal, iors1.z);

    col += resampleColor(rds, refl0, refl1, wavelengths0, wavelengths1);
        
    //c = to_float4_aw( contrast(col)+spec*col, 1.0f );
    //c = to_float4_aw(contrast(0.6f*filmic_gamma(spec*col)),1);
    //c = to_float4_aw(contrast(filmic_gamma(col/1.0f)),4);
    c = to_float4_aw(contrast(filmic_gamma(col/2.0f)),1);
    //c += 0.25f*spec;


  SetFragmentShaderComputedColor(c);
}