
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


//#define T(p) texelFetch(iChannel0, to_int2(mod_f(p,R)), 0)
#define T(p) texture(iChannel0, (make_float2(to_int2_cfloat(mod_f2f2(p,R)))+0.5f)/R)
#define P(p) texture(iChannel0, mod_f2f2(p,R)/R)
#define C(p) texture(iChannel1, mod_f2f2(p,R)/R)
#define D(p) texture(iChannel2, mod_f2f2(p,R)/R)

#define PI 3.14159265f
#define dt 1.0f




__DEVICE__ float hash11(float p)
{
    p = fract(p * 0.1031f);
    p *= p + 33.33f;
    p *= p + p;
    return fract(p);
}

#define rand_interval 25
#define random_gen(a, b, seed) ((a) + ((b)-(a))*hash11(seed + (float)(iFrame/rand_interval)))


#define distribution_size 01.07f



#define sense_num 22
#define sense_ang 0.27511f
//#define sense_dis 12.0f*(010.51f*_cosf(iTime*0.51f))
#define sense_dis 4.40f*(020.0f*_cosf(iTime*0.1f))
#define sense_oscil 00.1
#define oscil_scale 0.51f*(01.1f*_cosf(iTime*0.1f))
#define oscil_pow 01.0
#define sense_force 0.2f*(010.1f*_cosf(iTime*0.51f))
#define distance_scale 00.51f*(0.51f*_cosf(iTime*0.951f))
#define force_scale 01.51f
#define trailing 0.0f
#define acceleration 0.09f

//useful functions
#define GS(x) _expf(-dot(x,x))
#define GSS(x) _expf(-dot(x,x))
#define GS0(x) _expf(-length(x))
#define Dir(ang) to_float2(_cosf(ang), _sinf(ang))
#define Rot(ang) to_mat2(_cosf(ang), _sinf(ang), -_sinf(ang), _cosf(ang))
#define loop(i,x) for(int i = 0; i < x; i++)
#define range(i,a,b) for(int i = a; i <= b; i++)

//SPH pressure
#define Pressure(rho) 0.3f*rho.z
#define fluid_rho 0.2f


//data packing
#define PACK(X) ( uint(round(65534.0f*clamp(0.5f*X.x+0.5f, 0.0f, 1.0f))) + \
           65535u*uint(round(65534.0f*clamp(0.5f*X.y+0.5f, 0.0f, 1.0f))) )   
               
#define UNPACK(X) (clamp(to_float2(X%65535u, X/65535u)/65534.0f, 0.0f,1.0f)*2.0f - 1.0f)              

#define DECODE(X) decode(X) //UNPACK(floatBitsToUint(X))
#define ENCODE(X) encode(X) //uintBitsToFloat(PACK(X))



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
	
    return UNPACK(z._Uint); 
}

__DEVICE__ float encode(float2 _x)
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
// Connect Buffer A 'Preset: Keyboard' to iChannel3
// Connect Buffer A 'Previsualization: Buffer B' to iChannel0




__KERNEL__ void SurfaceTensionPhaseShiftOsciFuse__Buffer_A(float4 U, float2 pos, float2 iResolution, int iFrame, sampler2D iChannel3)
{
    CONNECT_CHECKBOX0(Reset, 0); 

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
    M = _mix(M, 0.2f, 0.05f);
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


__KERNEL__ void SurfaceTensionPhaseShiftOsciFuse__Buffer_B(float4 U, float2 pos, float iTime, float2 iResolution, float4 iMouse)
{

    pos+=0.5f;

    float2 dx = to_float2(0, 1);

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
            
        F -= M*p;
        

        float ang = _atan2f(V.y, V.x);
        float dang = sense_ang*PI/(float)(sense_num);
        float2 slimeF = to_float2_s(0.0f);
        //slime mold sensors
        range(i, -sense_num, sense_num)
        {
          float cang = ang + (float)(i) * dang;
          float2 dir = (1.0f + sense_dis*_powf(M, distance_scale))*Dir(cang);
          float3 s0 = swi3(C(X + dir),x,y,z);  
          float fs = _powf(s0.z, force_scale);
          float os = oscil_scale*_powf(s0.z - M, oscil_pow);
          slimeF +=  sense_oscil*mul_mat2_f2(Rot(os),swi2(s0,x,y)) 
                 + sense_force*Dir(ang + sign_f((float)(i))*PI*0.5f)*fs; 
        }
        
        //remove acceleration component and leave rotation
        // slimeF -= dot(slimeF, normalize(V))*normalize(V);
        F += slimeF/(float)(2*sense_num);
        
        if(iMouse.z > 0.0f)
        {
            float2 _dx = pos - swi2(iMouse,x,y);
            F += 0.1f*mul_mat2_f2(Rot(PI*0.5f), _dx*GS(_dx/30.0f));
        }
        
        //integrate velocity
        //V += mul_mat2_f2(Rot(-0.0f*curl),(F*dt/M));
        V += mul_mat2_f2(Rot(-0.0f*curl),F)*dt/M;
        
        //acceleration for fun effects
        V *= 1.0f + acceleration;
        
        //velocity limit
        float v = length(V);
        V /= (v > 1.0f)?v/1.0f:1.0f;
    }
    
    //mass decay
    // M *= 0.999f;
    
    //input
    //if(iMouse.z > 0.0f)
    //\\  M = _mix(M, 0.5f, GS((pos - swi2(iMouse,x,y))/13.0f));
    //else
     //   M = _mix(M, 0.5f, GS((pos - R*0.5f)/13.0f));
    
    //save
    X = clamp(X - pos, to_float2_s(-0.5f), to_float2_s(0.5f));
    U = to_float4(ENCODE(X), ENCODE(V), M, 0.0f);

  SetFragmentShaderComputedColor(U);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


#define radius 2.0f

__KERNEL__ void SurfaceTensionPhaseShiftOsciFuse__Buffer_C(float4 fragColor, float2 pos, float2 iResolution,)
{
    pos+=0.5f;

    float rho = 0.01f;
    float2 vel = to_float2(0.0f, 0.0f);

    //compute the smoothed density and velocity
    range(i, -2, 2) range(j, -2, 2)
    {
        float2 tpos = pos + to_float2(i,j);
        float4 data = T(tpos);

        float2 X0 = DECODE(data.x) + tpos;
        float2 V0 = DECODE(data.y);
        float M0 = data.z;
        float2 _dx = X0 - pos;

        float K = GS(_dx/radius)/(radius);
        rho += M0*K;
        vel += M0*K*V0;
    }

    vel /= rho;

    fragColor = to_float4(vel.x, vel.y, rho, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Texture: Abstract 1' to iChannel2
// Connect Buffer D 'Preset: Keyboard' to iChannel3
// Connect Buffer D 'Previsualization: Buffer B' to iChannel0
// Connect Buffer D 'Previsualization: Buffer D' to iChannel1


__KERNEL__ void SurfaceTensionPhaseShiftOsciFuse__Buffer_D(float4 fragColor, float2 pos, float2 iResolution, int iFrame, sampler2D iChannel3)
{
    CONNECT_CHECKBOX0(Reset, 0); 
  
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
    
    swi2S(fragColor,x,y, swi2(C(pos - V0*dt),x,y) - V0*dt/R);
    
    //initial condition
    if(iFrame < 1 || Reset)
    {
        //swi2S(fragColor,x,y, to_float2_s(0.0f));
        fragColor.x =  0.0f;
        fragColor.y =  0.0f;
    }


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Organic 2' to iChannel3
// Connect Image 'Cubemap: Forest_0' to iChannel2
// Connect Image 'Previsualization: Buffer B' to iChannel0
// Connect Image 'Previsualization: Buffer D' to iChannel1


// Fork of "Slime mold advection" by michael0884. https://shadertoy.com/view/3tjfzh
// 2020-10-01 10:52:06

// Fork of "Fireballs" by michael0884. https://shadertoy.com/view/tlfBDX
// 2020-08-20 00:44:41

// Fork of "Random slime mold generator" by michael0884. https://shadertoy.com/view/ttsfWn
// 2020-08-19 23:28:40

// Fork of "Everflow" by michael0884. https://shadertoy.com/view/ttBcWm
// 2020-07-19 18:18:22

// Fork of "Paint streams" by michael0884. https://shadertoy.com/view/WtfyDj
// 2020-07-11 22:38:47

//3d mode
//#define heightmap iChannel3

__DEVICE__ float3 hsv2rgb( in float3 c )
{
  float3 rgb = clamp( abs_f3(mod_f3(c.x*6.0f+to_float3(0.0f,4.0f,2.0f),6.0f)-3.0f)-1.0f, 0.0f, 1.0f );

  rgb = rgb*rgb*(3.0f-2.0f*rgb); // cubic smoothing  

  return c.z * _mix( to_float3_s(1.0f), rgb, c.y);
}

#define FOV 2.56f
#define RAD R.x*01.7f

__DEVICE__ float gauss(float _x, float r)
{
    _x/=r;
    return _expf(-_x*_x);
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
    swi2S(pos,x,y, mod_f2f2(swi2(pos,x,y), R-1.0f));

    float4 v = P(swi2(pos,x,y));
    return v.z;
}

__DEVICE__ float3 color(float3 pos, float2 R, __TEXTURE2D__ iChannel1)
{
    //swi2(pos,x,y) += R*0.5f;
    pos.x += R.x*0.5f;
    pos.y += R.y*0.5f;
    
    swi2S(pos,x,y, mod_f2f2(swi2(pos,x,y), R-1.0f));

    float4 v = C(swi2(pos,x,y));
    return swi3(v,x,y,z);
}


__DEVICE__ float DE(float3 pos, float2 R, __TEXTURE2D__ iChannel0)
{
    float _y = 1.0f*rho(pos,R,iChannel0);  
    
    //swi2(pos,x,y) += R*0.5f;
    pos.x += R.x*0.5f;
    pos.y += R.y*0.5f;
    
    swi2S(pos,x,y, mod_f2f2(swi2(pos,x,y), R-1.0f));
    float de = 1e10;
    de = _fminf(de, 0.7f*sdBox(pos - to_float3_aw(R, 4.0f*_y)*0.5f, to_float3_aw(R*0.51f, 3.0f)));
    return de;
}


__DEVICE__ float4 calcNormal(float3 p, float _dx, float2 R, __TEXTURE2D__ iChannel0) {
  const float3 K = to_float3(1,-1,0);
  return  (swi4(K,x,y,y,x)*DE(p + swi3(K,x,y,y)*_dx,R,iChannel0) +
           swi4(K,y,y,x,x)*DE(p + swi3(K,y,y,x)*_dx,R,iChannel0) +
           swi4(K,y,x,y,x)*DE(p + swi3(K,y,x,y)*_dx,R,iChannel0) +
           swi4(K,x,x,x,x)*DE(p + swi3(K,x,x,x)*_dx,R,iChannel0))/to_float4(4.0f*_dx,4.0f*_dx,4.0f*_dx,4.0f);
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

__KERNEL__ void SurfaceTensionPhaseShiftOsciFuse(float4 col, float2 pos, float2 iResolution, float iTime, float4 iMouse, sampler2D iChannel2, sampler2D iChannel3)
{
        pos+=0.5f;
        
        float2 dx = to_float2(0, 1);

    #ifdef heightmap
        // Normalized pixel coordinates 
        pos = (pos - R*0.5f)/_fmaxf(R.x,R.y);
        pos = to_float2(pos.x, pos.y);
        float2 uv = swi2(iMouse,x,y)/R;
        float2 angles = to_float2(-1.5f + 0.0f*iTime, -0.5f)*PI;

        float3 camera_z = to_float3(_cosf(angles.x)*_cosf(angles.y),_sinf(angles.x)*_cosf(angles.y),_sinf(angles.y));
        float3 camera_x = normalize(to_float3(_cosf(angles.x+PI*0.5f), _sinf(angles.x+PI*0.5f),0.0f)); 
        float3 camera_y = -1.0f*normalize(cross(camera_x,camera_z));

        //tracking particle
        float4 fp = to_float4(R.x*0.5f + 0.0f*150.0f*iTime,R.y*0.5f + 0.0f*0.0f, 0.0f, 0.0f);

        float3 ray = normalize(camera_z + FOV*(pos.x*camera_x + pos.y*camera_y));
        float3 cam_pos = to_float3(swi2(fp,x,y)-R*0.5f, 0.0f) - RAD*to_float3(_cosf(angles.x)*_cosf(angles.y),_sinf(angles.x)*_cosf(angles.y),_sinf(angles.y));

        float4 X = ray_march(cam_pos, ray,R,iChannel0);

        if(X.w < min_d)
        {

            float D = rho(swi3(X,x,y,z),R,iChannel0);
            float3 c = color(swi3(X,x,y,z),R,iChannel1);
           
            float3 albedo = 0.5f*(D+0.07f)*swi3(texture(iChannel3, swi2(c,x,y)),x,y,z);
            float rough = 1.0f - 0.1f*distance_f3(albedo, to_float3_s(1.0f));

            float4 N0 = calcNormal(swi3(X,x,y,z), 2.0f*X.w)*to_float4(4.0f,4.0f,1.0f,1.0f);
            float3 n = normalize(swi3(N0,x,y,z));
            float3 rd = reflect(ray, n);
            float3 colA =texture(iChannel2,  swi3(rd,y,z,x)).xyz;
            float3 colB = (to_float3_s(0.5f) + 0.5f*dot(rd, normalize(to_float3_s(1.0f))));
            colB += 3.0f*rough*_powf(_fmaxf(dot(rd, normalize(to_float3_s(1.0f))), 0.0f), 10.0f);
            colB += 3.0f*rough*_powf(_fmaxf(dot(rd, normalize(to_float3(-0.5f,-0.9f,0.8f))), 0.0f), 10.0f);
            float b = clamp(0.5f + 0.5f*dot(n, normalize(to_float3(1,1,1))), 0.0f,1.0f);
            float K = 1.0f - _powf(_fmaxf(dot(n,rd),0.0f), 2.0f);
            swi3S(col,x,y,z, 1.0f*albedo*colB + 0.0f*rough*colA*K);
        }
        else
        {    
            //background
            col = 1.0f*decube_f3(iChannel2,  swi3(ray,y,z,x));
        }
    col = _tanhf(8.0f*col);
    #else
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
      
      
        col= 3.7f*r*texture(iChannel3, swi2(c,x,y) + pos/R);
      //swi3(col,x,y,z) += to_float3(1,0.1f,0.1f)*_fmaxf(curl,0.0f) + to_float3(0.1f,0.1f,1.0f)*_fmaxf(-curl,0.0f);
      
    #endif

  SetFragmentShaderComputedColor(col);
}