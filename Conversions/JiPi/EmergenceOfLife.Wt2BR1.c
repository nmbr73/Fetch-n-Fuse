
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define tanh_f4(i) to_float4(_tanhf((i).x), _tanhf((i).y), _tanhf((i).z), _tanhf((i).w))

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
//#define T(p) texelFetch(iChannel0, to_int2(mod_f(p,R)), 0)
#define T(p) texture(iChannel0, (make_float2(to_int2_cfloat(mod_f2f2(p,R)))+0.5f)/R)
#define P(p) texture(iChannel0, mod_f(p,R)/R)
#define C(p) texture(iChannel1, mod_f(p,R)/R)
#define D(p) texture(iChannel2, mod_f(p,R)/R)

#define PI 3.14159265f
//#define dt 1.0f




__DEVICE__ float hash11(float p)
{
    p = fract(p * 0.1031f);
    p *= p + 33.33f;
    p *= p + p;
    return fract(p);
}

#define rand_interval 250
#define random_gen(a, b, seed) ((a) + ((b)-(a))*hash11(seed + float(iFrame/rand_interval)))

//i.e. diffusion 
//#define distribution_size 1.7f

//slime mold sensors
//#define sense_num 12
//#define sense_ang 0.2f
//#define sense_dis 40.0f
//weird oscillation force
//#define sense_oscil 0.0f 
//#define oscil_scale 1.0f
//#define oscil_pow 1.0f
//#define sense_force 0.11
//slime mold sensor distance power law dist = sense_dispow(rho, pw)
//#define distance_scale 0.0f
//#define force_scale 2.0f
//#define acceleration 0.0f

//#define density_normalization_speed 0.25f
//#define density_target 0.24f
//#define vorticity_confinement 0.0f

//useful functions
#define GS(x) _expf(-dot(x,x))
#define GSS(x) _expf(-dot(x,x))
#define GS0(x) _expf(-length(x))
#define Dir(ang) to_float2(_cosf(ang), _sinf(ang))
#define Rot(ang) to_mat2(_cosf(ang), _sinf(ang), -_sinf(ang), _cosf(ang))
#define loop(i,x) for(int i = 0; i < x; i++)
#define range(i,a,b) for(int i = a; i <= b; i++)

//SPH pressure
   
//density*temp = pressure - i.e. ideal gas law
//#define Pressure(rho) 0.9f*rho.z
#define Pressure(rho) Press*rho.z
//#define fluid_rho 0.2f


//data packing
#define PACK(X) ( (uint)(round(65534.0f*clamp(0.5f*X.x+0.5f, 0.0f, 1.0f))) + \
           65535u*(uint)(round(65534.0f*clamp(0.5f*X.y+0.5f, 0.0f, 1.0f))) )   
               
#define UNPACK(X) (clamp(to_float2(X%65535u, X/65535u)/65534.0f, 0.0f,1.0f)*2.0f - 1.0f)              

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

//#define DECODE(X) UNPACK(floatBitsToUint(X))
//#define ENCODE(X) uintBitsToFloat(PACK(X))

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer B' to iChannel0
// Connect Buffer A 'Texture: Blending' to iChannel1

//const int KEY_SPACE = 32;
//__DEVICE__ bool isKeyPressed(int KEY)
//{
//  return texelFetch( iChannel3, to_int2(KEY,0), 0 ).x > 0.5f;
//}

struct particle
{
    float2 X;
    float2 V;
    float M;
};

__DEVICE__ particle Blending( __TEXTURE2D__ channel, float2 uv, particle P, float Blend, float2 Par, float2 MulOff, int Modus, float2 fragCoord, float2 R)
{
 
    if (Blend > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(channel,uv);

      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2) //Startbedingung
        {
          P.X = _mix(P.X, fragCoord, Blend);
          P.V = _mix(P.V, to_float2((tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x), Blend);
          P.M = _mix(P.M, (tex.x+MulOff.y)*MulOff.x, Blend);
        }      
        if ((int)Modus&4) // Geschwindigkeit
        {
          P.V = Par.x*_mix(P.V, to_float2((tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x), Blend);
        }
        
        if ((int)Modus&8) // Masse
          P.M = _mix(P.M, (tex.x+MulOff.y)*MulOff.x, Blend);

      }
      else
      {
        if ((int)Modus&16) 
          P.M = _mix(P.M, (MulOff.y+0.45) - _fabs(fragCoord.x/iResolution.x - 0.5f), Blend);
      
        if ((int)Modus&32) //Special
        {
          P.X = _mix(P.X, fragCoord, Blend);
          P.V = _mix(P.V, to_float2((tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x), Blend);
          P.M = _mix(P.M, (tex.x+MulOff.y)*MulOff.x, Blend);
        }  
      }
    }
  
  return P;
}






__KERNEL__ void EmergenceOfLifeFuse__Buffer_A(float4 U, float2 pos, float2 iResolution, int iFrame, sampler2D iChannel3)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_SLIDER3(density_target, -1.0f, 1.0f, 0.24f);
    CONNECT_SLIDER4(density_normalization_speed, -1.0f, 1.0f, 0.25f);
    
    CONNECT_SLIDER8(dt, -1.0f, 5.0f, 1.0f);
    
    CONNECT_SLIDER15(distribution_size, -1.0f, 3.0f, 1.7f);
    
    
    //Blending
    CONNECT_SLIDER16(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER17(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER18(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT1(Par1, 0.0f, 0.0f);
    
    
    
    pos+=0.5f;
    int2 p = to_int2_cfloat(pos);
    
    float2 X = to_float2_s(0);
    float2 V = to_float2_s(0);
    float M = 0.0f;
    
    //basically integral over all updated neighbor distributions
    //that fall inside of this pixel
    //this makes the tracking conservative
    range(i, -2, 2) range(j, -2, 2)
    {
      float2 tpos = pos + to_float2(i,j);
      float4 data = T(tpos);
       
      float2 X0 = DECODE(data.x) + tpos;
      float2 V0 = DECODE(data.y);
      float2 M0 = swi2(data,z,w);
       
      X0 += V0*dt; //integrate position

      //particle distribution size
      float K = distribution_size;
        
      float4 aabbX = to_float4_f2f2(_fmaxf(pos - 0.5f, X0 - K*0.5f), _fminf(pos + 0.5f, X0 + K*0.5f)); //overlap aabb
      float2 center = 0.5f*(swi2(aabbX,x,y) + swi2(aabbX,z,w)); //center of mass
      float2 size = _fmaxf(swi2(aabbX,z,w) - swi2(aabbX,x,y), to_float2_s(0.0f)); //only positive
        
      //the deposited mass into this cell
      float m = M0.x*size.x*size.y/(K*K); 
        
      //add weighted by mass
      X += center*m;
      V += V0*m;
      
      //add mass
      M += m;
    }
    
    //normalization
    if(M != 0.0f)
    {
      X /= M;
      V /= M;
    }
    
    //mass renormalization
    float prevM = M;
    M = _mix(M, density_target, density_normalization_speed);
    V = V*prevM/M;
    
    //initial condition
    if(iFrame < 1 || Reset)
    {
        X = pos;
        float2 dx0 = (pos - R*0.3f); float2 dx1 = (pos - R*0.7f);
        V = 0.5f*mul_mat2_f2(Rot(PI*0.5f),dx0)*GS(dx0/30.0f) - 0.5f*mul_mat2_f2(Rot(PI*0.5f),dx1)*GS(dx1/30.0f);
        V += 0.2f*Dir(2.0f*PI*hash11(_floor(pos.x/20.0f) + R.x*_floor(pos.y/20.0f)));
        M = 0.1f + pos.x/R.x*0.01f + pos.y/R.x*0.01f;
    }
    
    particle P = {X,V,M};
    if (Blend1>0.0) P = Blending(iChannel1, pos/R, P, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, pos, R);  
    X=P.X;
    V=P.V;
    M=P.M;
    
    X = clamp(X - pos, to_float2_s(-0.5f), to_float2_s(0.5f));
    U = to_float4(ENCODE(X), ENCODE(V), M, 0.0f);

  SetFragmentShaderComputedColor(U);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer C' to iChannel1
// Connect Buffer B 'Previsualization: Buffer D' to iChannel2

__KERNEL__ void EmergenceOfLifeFuse__Buffer_B(float4 U, float2 pos, float2 iResolution, float4 iMouse)
{
    CONNECT_CHECKBOX2(MassPar, 0);
    CONNECT_POINT0(MassParXY, 0.5f, 0.5f);
    CONNECT_SLIDER19(MassParSize, -1.0f, 50.0f, 13.0f);
  
    CONNECT_SLIDER1(Press, 0.0f, 2.0f, 0.9f);
    CONNECT_SLIDER2(vorticity_confinement, -1.0f, 2.0f, 0.0f);
    CONNECT_SLIDER5(acceleration, -1.0f, 2.0f, 0.0f);
    CONNECT_SLIDER6(force_scale, -1.0f, 5.0f, 2.0f);
    CONNECT_SLIDER7(distance_scale, -1.0f, 2.0f, 0.0f);
  
    CONNECT_SLIDER8(dt, -1.0f, 5.0f, 1.0f);
  
    CONNECT_SLIDER9(sense_force, -1.0f, 1.0f, 0.11f);
    CONNECT_SLIDER10(oscil_pow, -1.0f, 3.0f, 1.0f);
    CONNECT_SLIDER11(oscil_scale, -1.0f, 3.0f, 1.0f);
    CONNECT_SLIDER12(sense_oscil, -1.0f, 3.0f, 0.0f);
    
    CONNECT_SLIDER13(sense_dis, -1.0f, 100.0f, 40.0f);
    CONNECT_SLIDER14(sense_ang, -1.0f, 1.0f, 0.2f);
    CONNECT_INTSLIDER0(sense_num, 0, 20, 12);
  
    pos+=0.5f;
    
    const float2 dx = to_float2(0, 1);
    
    float2 uv = pos/R;
    int2 p = to_int2_cfloat(pos);
        
    float4 data = T(pos); 
    float2 X = DECODE(data.x) + pos;
    float2 V = DECODE(data.y);
    float M = data.z;
    
    if(M != 0.0f) //not vacuum
    {
        //Compute the force
        float2 F = to_float2_s(0.0f);
        
        //get neighbor data
        float4 d_u = T(pos + swi2(dx,x,y)), d_d = T(pos - swi2(dx,x,y));
        float4 d_r = T(pos + swi2(dx,y,x)), d_l = T(pos - swi2(dx,y,x));
        
        //position deltas
        float2 p_u = DECODE(d_u.x), p_d = DECODE(d_d.x);
        float2 p_r = DECODE(d_r.x), p_l = DECODE(d_l.x);
        
        //velocities
        float2 v_u = DECODE(d_u.y), v_d = DECODE(d_d.y);
        float2 v_r = DECODE(d_r.y), v_l = DECODE(d_l.y);

        
        //pressure gradient
        float2 p = to_float2(Pressure(d_r) - Pressure(d_l),
                             Pressure(d_u) - Pressure(d_d));
        
        //density gradient
        float2 dgrad = to_float2(d_r.z - d_l.z,
                                 d_u.z - d_d.z);
          
        //velocity operators
        float div = v_r.x - v_l.x + v_u.y - v_d.y;
        float curl = v_r.y - v_l.y - v_u.x + v_d.x;
        //vec2 laplacian = 
     
        F -= 1.0f*M*p;
        
        float ang = _atan2f(V.y, V.x);
        float dang =sense_ang*PI/(float)(sense_num);
        float2 slimeF = to_float2_s(0.0f);
        //slime mold sensors
        range(i, -sense_num, sense_num)
        {
          float cang = ang + (float)(i) * dang;
          float2 dir = (1.0f + sense_dis*_powf(M, distance_scale))*Dir(cang);
          float3 s0 = swi3(C(X + dir),x,y,z);  
          float fs = _powf(s0.z, force_scale);
          float os = oscil_scale*_powf(s0.z - M, oscil_pow);
          slimeF += sense_oscil*mul_mat2_f2(Rot(os),swi2(s0,x,y)) 
                 +  sense_force*Dir(ang + sign_f((float)(i))*PI*0.5f)*fs; 
        }
        
        //remove acceleration component from slime force and leave rotation only
        slimeF -= dot(slimeF, normalize(V))*normalize(V);
        F += slimeF/(float)(2*sense_num);
        
        if(iMouse.z > 0.0f)
        {
            float2 dx= pos - swi2(iMouse,x,y);
            F += 0.1f*mul_mat2_f2(Rot(PI*0.5f),dx)*GS(dx/30.0f);
        }
        
        //integrate velocity
        V += mul_mat2_f2(Rot(-vorticity_confinement*curl),F*dt/M);
        
        //acceleration for fun effects
        V *= 1.0f + acceleration;
        
        //velocity limit
        float v = length(V);
        V /= (v > 1.0f)?v/1.0f:1.0f;
    }
    
    if(MassPar)
    {
      //mass decay
      M *= 0.999f;
      M = _mix(M, 0.5f, GS((pos - (MassParXY*R))/MassParSize));//13.0f));
    }

    //save
    X = clamp(X - pos, to_float2_s(-0.5f), to_float2_s(0.5f));
    U = to_float4(ENCODE(X), ENCODE(V), M, 0.0f);

  SetFragmentShaderComputedColor(U);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


//#define radius 2.0f

__KERNEL__ void EmergenceOfLifeFuse__Buffer_C(float4 fragColor, float2 pos, float2 iResolution)
{
    CONNECT_SLIDER0(radius, 0.0f, 5.0f, 2.0f);
  
    pos+=0.5f;
    float rho = 0.001f;
    float2 vel = to_float2(0.0f, 0.0f);

    //compute the smoothed density and velocity
    range(i, -2, 2) range(j, -2, 2)
    {
        float2 tpos = pos + to_float2(i,j);
        float4 data = T(tpos);

        float2 X0 = DECODE(data.x) + tpos;
        float2 V0 = DECODE(data.y);
        float M0 = data.z;
        float2 dx = X0 - pos;

        float K = GS(dx/radius)/(radius);
        rho += M0*K;
        vel += M0*K*V0;
    }

    vel /= rho;

    fragColor = to_float4(vel.x,vel.y, rho, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Texture: London' to iChannel2
// Connect Buffer D 'Preset: Keyboard' to iChannel3
// Connect Buffer D 'Previsualization: Buffer B' to iChannel0
// Connect Buffer D 'Previsualization: Buffer D' to iChannel1


//normal advection

__KERNEL__ void EmergenceOfLifeFuse__Buffer_D(float4 fragColor, float2 pos, float2 iResolution, int iFrame, sampler2D iChannel3)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_SLIDER8(dt, -1.0f, 5.0f, 1.0f);
    
    pos+=0.5f;
    float2 V0 = to_float2_s(0.0f);
    if(iFrame%1 == 0)
    {
      float4 data = T(pos);
      V0 = 1.0f*DECODE(data.y);
      float M0 = data.z;
    }
    else
    {

    }
    
    fragColor = C(pos - V0*dt);
    //initial condition
    if(iFrame < 1 || Reset)
    {
        //swi2(fragColor,x,y) = pos/R;
        fragColor.x = pos.x/R.x;
        fragColor.y = pos.y/R.y;
    }

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: London' to iChannel3
// Connect Image 'Cubemap: Forest_0' to iChannel2
// Connect Image 'Previsualization: Buffer B' to iChannel0
// Connect Image 'Previsualization: Buffer D' to iChannel1


// Fork of "Fireballs" by michael0884. https://shadertoy.com/view/tlfBDX
// 2020-08-20 00:44:41

// Fork of "Random slime mold generator" by michael0884. https://shadertoy.com/view/ttsfWn
// 2020-08-19 23:28:40

// Fork of "Everflow" by michael0884. https://shadertoy.com/view/ttBcWm
// 2020-07-19 18:18:22

// Fork of "Paint streams" by michael0884. https://shadertoy.com/view/WtfyDj
// 2020-07-11 22:38:47

//3d mode
//#define heightmap

__DEVICE__ float3 hsv2rgb( in float3 c )
{
  float3 rgb = clamp( abs_f3(mod_f3(c.x*6.0f+to_float3(0.0f,4.0f,2.0f),6.0f)-3.0f)-1.0f, 0.0f, 1.0f );

  rgb = rgb*rgb*(3.0f-2.0f*rgb); // cubic smoothing  

  return c.z * _mix( to_float3_s(1.0f), rgb, c.y);
}

#define FOV 1.56f
#define RAD R.x*0.6f

__DEVICE__ float gauss(float x, float r)
{
    x/=r;
    return _expf(-x*x);
}

__DEVICE__ float sdSegment( in float2 p, in float2 a, in float2 b )
{
    float2 pa = p-a, ba = b-a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0f, 1.0f );
    return length( pa - ba*h );
}

__DEVICE__ float sdSphere( float3 p, float s )
{
  return length(p)-s;
}

__DEVICE__ float sdBox( float3 p, float3 b )
{
  float3 q = abs_f3(p) - b;
  return length(_fmaxf(q,to_float3_s(0.0f))) + _fminf(max(q.x,_fmaxf(q.y,q.z)),0.0f);
}

__DEVICE__ float rho(float3 pos, float2 R, __TEXTURE2D__ iChannel0)
{
    //swi2(pos,x,y) += R*0.5f;
    pos.x += R.x*0.5f;
    pos.y += R.y*0.5f;
    
    //swi2(pos,x,y) = mod_f(swi2(pos,x,y), R-1.0f);
    pos.x = mod_f(pos.x, R.x-1.0f);
    pos.y = mod_f(pos.y, R.y-1.0f);
    
    float4 v = P(swi2(pos,x,y));
    return v.z;
}

__DEVICE__ float3 color(float3 pos, float2 R, __TEXTURE2D__ iChannel1)
{
    //swi2(pos,x,y) += R*0.5f;
    pos.x += R.x*0.5f;
    pos.y += R.y*0.5f;
    
    //swi2(pos,x,y) = mod_f(swi2(pos,x,y), R-1.0f);
    pos.x = mod_f(pos.x, R.x-1.0f);
    pos.y = mod_f(pos.y, R.y-1.0f);
    
    float4 v = C(swi2(pos,x,y));
    return swi3(v,x,y,z);
}


__DEVICE__ float DE(float3 pos, float2 R, __TEXTURE2D__ iChannel0)
{
    float y = 16.0f*rho(pos,R,iChannel0);  
    
    //swi2(pos,x,y) += R*0.5f;
    pos.x += R.x*0.5f;
    pos.y += R.y*0.5f;

    //swi2(pos,x,y) = mod_f(swi2(pos,x,y), R-1.0f);
    pos.x = mod_f(pos.x, R.x-1.0f);
    pos.y = mod_f(pos.y, R.y-1.0f);

    float de = 1e10;
    de = _fminf(de, 0.7f*sdBox(pos - to_float3_aw(R, 4.0f*y)*0.5f, to_float3_aw(R*0.51f, 3.0f)));
    return de;
}


__DEVICE__ float4 calcNormal(float3 p, float dx, float2 R, __TEXTURE2D__ iChannel0) {
  const float3 K = to_float3(1,-1,0);
  return  (swi4(K,x,y,y,x)*DE(p + swi3(K,x,y,y)*dx,R,iChannel0) +
           swi4(K,y,y,x,x)*DE(p + swi3(K,y,y,x)*dx,R,iChannel0) +
           swi4(K,y,x,y,x)*DE(p + swi3(K,y,x,y)*dx,R,iChannel0) +
           swi4(K,x,x,x,x)*DE(p + swi3(K,x,x,x)*dx,R,iChannel0))/to_float4(4.0f*dx,4.0f*dx,4.0f*dx,4.0f);
}

#define marches 70.0f
#define min_d 1.0f
__DEVICE__ float4 ray_march(float3 p, float3 r, float2 R, __TEXTURE2D__ iChannel0)
{
    float d;
    for(float i = 0.0f; i < marches; i+=1.0f)
    {
        d = DE(p,R,iChannel0); 
        p += r*d;
        if(d < min_d || d > R.x) break;
    }
    return to_float4_aw(p, d);
}

__KERNEL__ void EmergenceOfLifeFuse(float4 col, float2 pos, float2 iResolution, float iTime, float4 iMouse, sampler2D iChannel2)
{
    CONNECT_CHECKBOX1(Heightmap, 0);    
    CONNECT_SLIDER1(Press, 0.0f, 2.0f, 0.9f);
    
    const float2 dx = to_float2(0, 1);
 
    //#ifdef heightmap
    if(Heightmap)
    {
        // Normalized pixel coordinates 
        pos = (pos - R*0.5f)/_fmaxf(R.x,R.y);
        pos = to_float2(pos.x, pos.y);
        float2 uv = swi2(iMouse,x,y)/R;
        float2 angles = to_float2(-1.5f + 0.03f*iTime, -0.4f)*PI;

        float3 camera_z = to_float3(_cosf(angles.x)*_cosf(angles.y),_sinf(angles.x)*_cosf(angles.y),_sinf(angles.y));
        float3 camera_x = normalize(to_float3(_cosf(angles.x+PI*0.5f), _sinf(angles.x+PI*0.5f),0.0f)); 
        float3 camera_y = -1.0f*normalize(cross(camera_x,camera_z));

        //tracking particle
        float4 fp = to_float4(R.x*0.5f + 0.0f*(150.0f*iTime),R.y*0.5f + 0.0f*(0.0f), 0.0f, 0.0f);

        float3 ray = normalize(camera_z + FOV*(pos.x*camera_x + pos.y*camera_y));
        float3 cam_pos = to_float3_aw(swi2(fp,x,y)-R*0.5f, 0.0f) - RAD*to_float3(_cosf(angles.x)*_cosf(angles.y),_sinf(angles.x)*_cosf(angles.y),_sinf(angles.y));

        float4 X = ray_march(cam_pos, ray,R,iChannel0);

        if(X.w < min_d)
        {

            float D = rho(swi3(X,x,y,z),R,iChannel0);
            float3 c = color(swi3(X,x,y,z),R,iChannel1);
           
            float3 albedo = sin_f3(to_float3(1,2,3)*3.0f*D*D);
            float rough   = 1.0f - 0.1f*distance_f3(albedo, to_float3_s(1.0f));

            float4 N0 = calcNormal(swi3(X,x,y,z), 2.0f*X.w,R,iChannel0)*to_float4(4.0f,4.0f,1.0f,1.0f);
            float3 n = normalize(swi3(N0,x,y,z));
            float3 rd = reflect(ray, n);
            float3 colA = swi3(decube_f3(iChannel2, rd),x,y,z);
            float3 colB = (to_float3_s(0.5f) + 0.5f*dot(rd, normalize(to_float3_s(1.0f))));
            colB += 4.0f*rough*_powf(_fmaxf(dot(rd, normalize(to_float3_s(1.0f))), 0.0f), 8.0f);
            colB += 4.0f*rough*_powf(_fmaxf(dot(rd, normalize(to_float3(-0.5f,-0.9f,0.8f))), 0.0f), 8.0f);
            float b = clamp(0.5f + 0.5f*dot(n, normalize(to_float3(1,1,1))), 0.0f,1.0f);
            float K = 1.0f - _powf(_fmaxf(dot(n,rd),0.0f), 2.0f);
            swi3S(col,x,y,z, 1.0f*albedo*colB + 0.0f*rough*colA*K);
        }
        else
        {    
            //background
            col = 1.0f*decube_f3(iChannel2, ray);
        }
      col = tanh_f4(2.0f*col);
    }
    else
    {
      float r = P(swi2(pos,x,y)).z;
      float4 c = C(swi2(pos,x,y));
      
      //get neighbor data
      float4 d_u = T(pos + swi2(dx,x,y)), d_d = T(pos - swi2(dx,x,y));
      float4 d_r = T(pos + swi2(dx,y,x)), d_l = T(pos - swi2(dx,y,x));
        
      //position deltas
      float2 p_u = DECODE(d_u.x), p_d = DECODE(d_d.x);
      float2 p_r = DECODE(d_r.x), p_l = DECODE(d_l.x);
        
      //velocities
      float2 v_u = DECODE(d_u.y), v_d = DECODE(d_d.y);
      float2 v_r = DECODE(d_r.y), v_l = DECODE(d_l.y);
       
      //pressure gradient
      float2 p = to_float2(Pressure(d_r) - Pressure(d_l),
                           Pressure(d_u) - Pressure(d_d));
        
      //velocity operators
      float div = (v_r.x - v_l.x + v_u.y - v_d.y);
      float curl = (v_r.y - v_l.y - v_u.x + v_d.x);
      
      
      col=sin_f4(to_float4(1,2,3,4)*25.0f*r*r*r);
      //swi3(col,x,y,z) += to_float3(1,0.1f,0.1f)*_fmaxf(curl,0.0f) + to_float3(0.1f,0.1f,1.0f)*_fmaxf(-curl,0.0f);
    }  
    //#endif

  SetFragmentShaderComputedColor(col);
}