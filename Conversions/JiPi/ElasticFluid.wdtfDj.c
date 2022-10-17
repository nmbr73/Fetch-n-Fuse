
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution

//#define T(p) texelFetch(iChannel0, to_int2(mod_f(p,R)), 0)
//#define T1(p) texelFetch(iChannel1, to_int2(mod_f(p,R)), 0)

#define T(p)  texelFetch(iChannel0, (make_float2(to_int2_cfloat(mod_f2f2(p,R)))+0.5f)/R)
#define T1(p) texelFetch(iChannel1, (make_float2(to_int2_cfloat(mod_f2f2(p,R)))+0.5f)/R)

#define P(p) texture(iChannel0, mod_f2f2(p,R)/R)
#define C(p) texture(iChannel1, mod_f2f2(p,R)/R)

#define PI 3.14159265f
#define dt 1.0f


//useful functions
#define GS(x) _expf(-dot(x,x))
#define GS0(x) _expf(-length(x))
#define CI(x) smoothstep(1.0f, 0.9f, length(x))
#define Dir(ang) to_float2(_cosf(ang), _sinf(ang))
#define Rot(ang) to_mat2(_cosf(ang), _sinf(ang), -_sinf(ang), _cosf(ang))
#define loop(i,x) for(int i = 0; i < x; i++)
#define range(i,a,b) for(int i = a; i <= b; i++)

#define velocity_averaging 0.0f

//squishy solid
#define relax 0.03f
#define distribution_size 1.0f
//estimation str
#define difd 2.0f
//target density
#define trho 0.0f
//density target strenght
#define rhoe 0.0f

//estimating the in-cell distribution size
__DEVICE__ float2 destimator(float2 dx, float M)
{
    //size estimate by in-cell location
    float2 ds = clamp(1.0f - 2.0f*abs_f2(dx), 0.001f, 1.0f);
    return ds + 0.0f*_fmaxf(M/(ds.x*ds.y) - 1.1f, 0.0f)*dt;
}

__DEVICE__ float deformation_energy(mat2 D)
{
    D = transpose(D)*D;
    //return 2.0f*(D[0][0]*D[0][0] + D[1][1]*D[1][1] - 2.0f);
    return 2.0f*(D.r0.x*D.r0.x + D.r1.y*D.r1.y - 2.0f);
}


// Lamé parameters for stress-strain relationship
#define elastic_lambda 0.6f
#define elastic_mu 0.0f
#define incompressible_viscosity 1.0f


//viscous fluid
/*
#define relax 0.05f
#define distribution_size 0.98f
// Lamé parameters for stress-strain relationship
#define elastic_lambda 0.2f
#define elastic_mu 0.1f
#define incompressible_viscousity 0.05f
*/ 

//MD force
__DEVICE__ float MF(float2 dx, float2 dv)
{
    return incompressible_viscosity*dot(dx,dv)*GS(0.8f*dx);
}


__DEVICE__ float Ha(float2 x)
{
    return ((x.x >= 0.0f)?1.:0.)*((x.y >= 0.0f)?1.:0.);
}

__DEVICE__ float Hb(float2 x)
{
    return ((x.x > 0.0f)?1.0f:0.0f)*((x.y > 0.0f)?1.0f:0.0f);
}

__DEVICE__ float sdBox( in float2 p, in float2 b )
{
    float2 d = abs_f2(p)-b;
    return length(_fmaxf(d,to_float2_s(0.0f))) + _fminf(_fmaxf(d.x,d.y),0.0f);
}

__DEVICE__ float opSubtraction( float d1, float d2 ) { return _fmaxf(-d1,d2); }

__DEVICE__ float2 opRepLim(in float2 p, in float2 c, in float2 l)
{
    return p-c*clamp(round(p/c),-l,l);
}
//data packing
#define PACK(X) ( (uint)(round(65534.0f*clamp(0.5f*X.x+0.5f, 0.0f, 1.0f))) + \
           65535u*(uint)(round(65534.0f*clamp(0.5f*X.y+0.5f, 0.0f, 1.0f))) )   
               
#define UNPACK(X) (clamp(to_float2(X%65535u, X/65535u)/65534.0f, 0.0f,1.0f)*2.0f - 1.0f)              

//#define DECODE(X) UNPACK(floatBitsToUint(X))
//#define ENCODE(X) uintBitsToFloat(PACK(X))

union Zahl
 {
   float  _Float; //32bit float
   uint   _Uint;  //32bit unsigend integer
 };

__DEVICE__ float2 DECODE(float _x)
{
	Zahl z;
    //uint X = floatBitsToUint(x);
	z._Float = _x;
	
  return UNPACK(z._Uint); 
}

__DEVICE__ float ENCODE(float2 _x)
{
	Zahl z;
  uint X = PACK(_x);
	
	z._Uint = X;
  //return uintBitsToFloat(X); 
	return (z._Float);
}

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: London' to iChannel3
// Connect Buffer A 'Texture: Nyancat' to iChannel2
// Connect Buffer A 'Previsualization: Buffer C' to iChannel1
// Connect Buffer A 'Previsualization: Buffer D' to iChannel0


//Reintegration tracking

__DEVICE__ float particleBox(float2 x, float2 s)
{
    return float(sdBox(x, s) < 0.0f);
}

__DEVICE__ float particleArch(float2 x, float2 s)
{
    return float(opSubtraction(sdBox(x + to_float2(0, s.y*0.4f), s*to_float2(0.5f, 0.9f)), sdBox(x, s)) < 0.0f);
}

__DEVICE__ float Building(float2 x, float2 s)
{
    float2 room_s = s.y*to_float2_s(0.12f);
    float2 rep_s = to_float2_s(0.15f)*s.x;
    float rooms = sdBox(opRepLim(x + to_float2(0.0f, -0.1f*room_s.y), rep_s, to_float2_s(30.0f)), room_s);
    float sd = opSubtraction(rooms, sdBox(x, s));
    return float(sd < 0.0f);
}

__KERNEL__ void ElasticFluidFuse__Buffer_A(float4 U, float2 pos, float2 iResolution, int iFrame, sampler2D iChannel2)
{
    pos+=0.5f;

    int2 p = to_int2_cfloat(pos);
    
    float2 X = to_float2_s(0);
    float2 V = to_float2_s(0);
    float M = 0.0f;
    float2 C = to_float2_s(0.0f);
    //basically integral over all updated neighbor distributions
    //that fall inside of this pixel
    //this makes the tracking conservative
    range(i, -1, 1) range(j, -1, 1)
    {
        float2 tpos = to_float2(i,j);
        float4 data = T(pos + tpos);
       
        float2 X0 = DECODE(data.x) + tpos;
        float2 V0 = DECODE(data.y);
        
        
        //particle distribution size
        float2 K = destimator(X0 - tpos , data.z);
       
        X0 += V0*dt; //integrate position

        //box overlaps
        float2 aabb0 = _fmaxf(X0 - K*0.5f, -0.5f); 
        float2 aabb1 = _fminf(X0 + K*0.5f, 0.5f); 
        float2 size = _fmaxf(aabb1 - aabb0, 0.0f); 
        float2 center = 0.5f*(aabb0 + aabb1);

        //the deposited mass into this cell
        float3 m = data.z*to_float3_aw(center, 1.0f)*size.x*size.y/(K.x*K.y);
        
        //add weighted by mass
        X += swi2(m,x,y);
        V += V0*m.z;
        C += m.z*DECODE(data.w);
        //add mass
        M += m.z;
    }
    
    //normalization
    if(M != 0.0f)
    {
        X /= M;
        V /= M;
        C /= M;
    }
    
    //initial condition
    if(iFrame < 1 || Reset)
    {
        X = pos;
        V = to_float2_s(0.0f);
        float4 nya = texture(iChannel2, clamp(1.8f*X*to_float2(0.166f,1.0f)/R, to_float2_s(0.0f),to_float2(0.1666f, 1.0f)));
        M = _fmaxf(_fmaxf(Building(X - R*to_float2(0.5f,0.32f), R*to_float2(0.4f,0.3f)),
                       particleBox(X - R*to_float2(0.1f,0.9f),  R*to_float2_s(0.0f))), 
                       particleBox(X - R*to_float2(0.5f,0.12f), R*to_float2(0.47f, 0.3f)));
          
        C = mod_f2(3.0f*pos/R, 1.0f);
        X = to_float2_s(0.0f);
    }
    
    U = to_float4_aw(ENCODE(X), ENCODE(V), M, ENCODE(C));

  SetFragmentShaderComputedColor(U);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer C' to iChannel1
// Connect Buffer B 'Previsualization: Buffer D' to iChannel0


//deformation gradient advection and update


//particle distribution
__DEVICE__ float3 PD(float2 x, float2 pos)
{
    return to_float3_aw(x, 1.0f)*Ha(x - (pos - 0.5f))*Hb((pos + 0.5f) - x);
}

__KERNEL__ void ElasticFluidFuse__Buffer_B(float4 U, float2 pos, int iFrame)
{
    pos+=0.5f;

    int2 p = to_int2_cfloat(pos);
    
    //deformation gradient
     mat2 D = to_mat2_f(0);
    float M = 0.0f;
    
    //basically integral over all updated neighbor distributions
    //that fall inside of this pixel
    //this makes the tracking conservative
    range(i, -1, 1) range(j, -1, 1)
    {
        float2 tpos = to_float2(i,j);
        float4 data = T(pos + tpos);
       
        float2 X0 = DECODE(data.x) + tpos;
        float2 V0 = DECODE(data.y);
        mat2 D0 = to_mat2(T1(tpos + pos));
float BBBBBBBBBBBBBBBB;        
        //particle distribution size
        float2 K = destimator(X0 - tpos , data.z);
       
        X0 += V0*dt; //integrate position

        //box overlaps
        float2 aabb0 = _fmaxf(X0 - K*0.5f, -0.5f); 
        float2 aabb1 = _fminf(X0 + K*0.5f, 0.5f); 
        float2 size = _fmaxf(aabb1 - aabb0, 0.0f); 
        float2 center = 0.5f*(aabb0 + aabb1);

        //the deposited mass into this cell
        float3 m = data.z*to_float3_aw(center, 1.0f)*size.x*size.y/(K.x*K.y);
       
        //add deformation grad weighted by mass
        D += D0*m.z;
        
        //add mass
        M += m.z;
    }
    
    //normalization
    if(M != 0.0f)
    {
       D /= M;
    }
  else D = to_mat2_f(1.0f);
    
    //initial condition
    if(iFrame < 1 || Reset)
    {
        D = to_mat2_f(1.0f);
    }

    U = to_float4_f2f2(D.r0,D.r1);

  SetFragmentShaderComputedColor(U);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1


__DEVICE__ float2 normalize2(float2 a)
{
    return all(equal(a, to_float2_s(0.0f)))?to_float2_s(0.0f):normalize(a);
}

__KERNEL__ void ElasticFluidFuse__Buffer_C(float4 U, float2 pos, float2 iResolution)
{
    pos+=0.5f;

    float2 uv = pos/R;
    int2 p = to_int2(pos);
        
    float4 data = T(pos); 
    float2 X = DECODE(data.x) + pos;
    float2 V = DECODE(data.y);
    float M = data.z;
    float C = data.w;
    mat2 D = mat2(T1(pos));
    
    if(M > 0.01f) //not vacuum
    {
        //Compute the velocity gradient matrix
        mat2 B = to_mat2_f(0.0f);
        float a = 0.01f;
        float rho = 0.0f;
        range(i, -1, 1) range(j, -1, 1)
        {
            float2 tpos = pos + to_float2(i,j);
            float4 data = T(tpos);

            float2 X0 = DECODE(data.x) + tpos;
            float2 V0 = DECODE(data.y);
            float M0 = data.z;
            float2 dx = X0 - X;
            float2 dv = V0 - V;
            float2 dsize = clamp(destimator(X0 - tpos, data.z), 0.3f, 1.0f);
            float weight = M0*GS(1.2f*dx);
            rho += M0*weight;
            B += to_mat2_f2(dv*dx.x,dv*dx.y)*weight;
            a += weight;
        }
        B /= a;
        rho /= a;
      
        float drho = rho - 1.0f;
        B -= 0.004f*to_mat2_f(drho)*_fabs(drho);
       
        //integrate deformation gradient
         D += 1.0f*dt*B*D;
       
        //smoothing
        
        float r = relax + 0.05f*smoothstep(-30.0f, 0.0f, -pos.y);
        D = D*(1.0f - r) + to_mat2_f(1.0f)*r;
        
        //clamp the gradient to not go insane
        D = mat2(clamp(to_float4_f2f2(D.r0,D.r1), -5.0f, 5.0f));
    }
    
    //save
    U = to_float4_f2f2(D.r0,D.r1);

  SetFragmentShaderComputedColor(U);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel0
// Connect Buffer D 'Previsualization: Buffer C' to iChannel1


//velocity update

__DEVICE__ float border(float2 p)
{
    float bound = -sdBox(p - R*0.5f, R*to_float2(0.48f, 0.48f)); 
    float box = sdBox((p - R*to_float2(0.5f, 0.6f)) , R*to_float2(0.05f, 0.01f));
    float drain = -sdBox(p - R*to_float2(0.5f, 0.7f), R*to_float2(0.0f, 0.0f));
    return bound;
}

#define h 1.0f
__DEVICE__ float3 bN(float2 p)
{
    float3 dx = to_float3(-h,0,h);
    float4 idx = to_float4(-1.0f/h, 0.0f, 1.0f/h, 0.25f);
    float3 r = swi3(idx,z,y,w)*border(p + swi2(dx,z,y))
           + swi3(idx,x,y,w)*border(p + swi2(dx,x,y))
           + swi3(idx,y,z,w)*border(p + swi2(dx,y,z))
           + swi3(idx,y,x,w)*border(p + swi2(dx,y,x));
    return to_float3(normalize(swi2(r,x,y)), r.z + 1e-4);
}

__DEVICE__ float2 normalize2(float2 a)
{
    return all(equal(a, to_float2_s(0.0f)))?to_float2_s(0.0f):normalize(a);
}


__DEVICE__ mat2 strain(mat2 D)
{
    float J = _fabs(determinant(D)) + 0.001f;

    // MPM course, page 46
    float volume = J;

    // useful matrices for Neo-Hookean model
    mat2 F_T = transpose(D);
    mat2 F_inv_T = inverse(F_T);
    mat2 F_minus_F_inv_T = D - F_inv_T;

    // MPM course equation 48
    mat2 P_term_0 = elastic_mu * (F_minus_F_inv_T);
    mat2 P_term_1 = elastic_lambda * _logf(J) * F_inv_T;
    mat2 P = P_term_0 + P_term_1;

    // equation 38, MPM course
    mat2 stress = (1.0f/J)* P * F_T;

    return volume * stress;
}


__KERNEL__ void ElasticFluidFuse__Buffer_D(float4 U, float2 pos, float4 iMouse)
{
    pos+=0.5f;  

    float2 uv = pos/R;
    int2 p = to_int2_cfloat(pos);
        
    float4 data = T(pos); 
    mat2 D = to_mat2(T1(pos));
    float2 X = DECODE(data.x) + pos;
    float2 V = DECODE(data.y);
    float M = clamp(data.z, 0.0f, 2.0f);
    float2 C = DECODE(data.w);
    if(M>0.0f) //not vacuum
    {
        //Compute the force
      
        float2 F = to_float2_s(0.0f);
        float b = 0.0f;
   
        mat2 local_strain = strain(D);
        if(M > 0.0f)
        {
            range(i, -1,1) range(j, -1, 1)
            {
                if(!(i == 0 && j == 0))
                {
                    float2 tpos = pos + to_float2(i,j);
                    float4 data = T(tpos);

                    float2 X0 = DECODE(data.x) + tpos;
                    float2 V0 = DECODE(data.y);
                    float M0 = data.z;
                    float2 dx = X0 - X;
                    float2 dv = V0 - V;
                    mat2 D0 = mat2(T1(tpos));
                    float weight = GS(1.2f*dx);
                   
                    //F += M0*strain((D0*M + D*M0)/(M+M0))*dx*weight;
                    mat2 strain0 = 0.5f*(strain(D0) + local_strain) + mat2(0.6f*dot(dx,dv));
                    F += M0*strain0*dx*weight;
                   
                    b += weight;
                }
            }
       
            F /= b;
       F = clamp(F, -0.1f,0.1f);
        }
        if(iMouse.z > 0.0f)
        {
            float2 dx= pos - swi2(iMouse,x,y);
            F += 0.02f*normalize2(dx)*GS(dx/80.0f);
        }
        
         //gravity
        F += 0.001f*to_float2(0,-1);
        
        //integrate velocity
        V += F*dt;
        //X +=  0.0f*F*dt;
        
        float3 BORD = bN(X);
        V += 0.1f*smoothstep(0.0f, 5.0f, -BORD.z)*swi2(BORD,x,y);
        V *= 1.0f - 0.5f*smoothstep(-30.0f, 0.0f, -pos.y);
        
        //velocity limit
        float v = length(V);
        V /= (v > 1.0f)?1.*v:1.;
    }
    
    //save
    X = X - pos;
    U = to_float4_aw(ENCODE(X), ENCODE(V), data.z, ENCODE(C));


  SetFragmentShaderComputedColor(U);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Organic 1' to iChannel2
// Connect Image 'Preset: Keyboard' to iChannel3
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1


// Fork of "Building smasher" by michael0884. https://shadertoy.com/view/wdGcRK
// 2020-11-22 17:42:31

// Fork of "Neo-Hookean Field" by michael0884. https://shadertoy.com/view/3dVyDD
// 2020-10-20 18:45:49

// Fork of "Neo-Hookean 2: Electric Boogaloo" by michael0884. https://shadertoy.com/view/tsVyWR
// 2020-10-16 17:16:16

// Fork of "CA Neo-Hookean" by michael0884. https://shadertoy.com/view/WdGyWR
// 2020-10-13 18:17:06

//used sources 
//https://github.com/nialltl/incremental_mpm/blob/master/Assets/2.%20MLS_MPM_NeoHookean_Multithreaded/MLS_MPM_NeoHookean_Multithreaded.cs
//https://www.seas.upenn.edu/~cffjiang/research/mpmcourse/mpmcourse.pdf

// Fork of "CA Paste" by michael0884. https://shadertoy.com/view/tsGczh
// 2020-10-12 21:02:54

// Fork of "CA Molecular dynamics" by michael0884. https://shadertoy.com/view/3s3cWr
// 2020-10-08 22:00:15

// Fork of "Landau Ginzburg fluid" by michael0884. https://shadertoy.com/view/WlXBDf
// 2020-09-21 21:03:05

// Fork of "Everflow" by michael0884. https://shadertoy.com/view/ttBcWm
// 2020-07-19 18:18:22

// Fork of "Paint streams" by michael0884. https://shadertoy.com/view/WtfyDj
// 2020-07-11 22:38:47

__DEVICE__ float3 hsv2rgb( in float3 c )
{
    float3 rgb = clamp( abs_f3(mod_f3(c.x*6.0f+to_float3(0.0f,4.0f,2.0f),6.0f)-3.0f)-1.0f, 0.0f, 1.0f );

  rgb = rgb*rgb*(3.0f-2.0f*rgb); // cubic smoothing  

  return c.z * _mix( to_float3_s(1.0f), rgb, c.y);
}

#ifdef XXX
const int KEY_SPACE = 32;
__DEVICE__ bool isKeyPressed(int KEY)
{
  return texelFetch( iChannel3, to_int2(KEY,2), 0 ).x > 0.5f;
}
#endif

#define radius 1.0f
#define zoom 0.35f

__KERNEL__ void ElasticFluidFuse(float4 col, float2 pos, float4 iMouse, sampler2D iChannel2, sampler2D iChannel3)
{
    CONNECT_CHECKBOX1(KEY_SPACE, 0);
  
    pos+=0.5f;    
    
    //zoom in
    if(KEY_SPACE)
    {
      pos = swi2(iMouse,x,y) + pos*zoom - R*zoom*0.5f;
    }
    float rho = 0.001f;
    float2 c = to_float2_s(0.0f);
    float De = 0.0f;
    float2 vel = to_float2(0.0f, 0.0f);
    float2 grad = to_float2_s(0.0f);

    float rho2 = 0.0f;
    //compute the smoothed density and velocity
    range(i, -1, 1) range(j, -1, 1)
    {
        float2 tpos = _floor(pos) + to_float2(i,j);
        float4 data = T(tpos);

        float2 X0 = DECODE(data.x) + tpos;
        float2 V0 = DECODE(data.y);
        float M0 = data.z;
        float2 dx = X0 - pos;
        float2 dx0 = X0 - tpos;
        mat2 D0 = to_mat2(T1(tpos));
        
        float K = GS(dx/radius)/(radius*radius);
        rho += M0*K;
        grad += normalize(dx)*K;
        c += M0*K*DECODE(data.w);
        De += M0*K*_fabs(deformation_energy(D0));
float IIIIIIIIIIIIIIII;        
        vel += M0*K*V0;
        float2 dsize = destimator(dx0,  data.z);
        float bsdf = sdBox(pos - X0,0.5f*dsize);
        //float bsdf = length(pos - X0) - 0.5f*length(destimator(dx0));
        rho2 += M0*smoothstep(0.3f, -0.3f, bsdf)/(dsize.x*dsize.y);
    }

   grad /= rho; 
   c /= rho;
   vel /= rho;
   De /= rho;
    
   //vec3 vc = hsv2rgb(to_float3_aw(6.0f*_atan2f(vel.x, vel.y)/(2.0f*PI), 1.0f, rho*length(swi2(vel,x,y))));
   float d = 0.5f*rho2;
   swi3S(col,x,y,z, _mix(to_float3_s(0.0f),1.0f*swi3(_tex2DVecN(iChannel2,c.x,c.y,15),x,y,z), d));
   swi3S(col,x,y,z, 1.0f - swi3(col,x,y,z));
    //swi3(col,x,y,z) = to_float3(rho2)*0.2f;

  SetFragmentShaderComputedColor(col);
}