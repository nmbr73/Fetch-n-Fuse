
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define PI 3.14159265f
#define dt 1.0f
#define R iResolution


//#define T(p) texelFetch(iChannel0, to_int2(mod_f(p,R)), 0)
#define T(p) texture(iChannel0, (make_float2(to_int2_cfloat(mod_f2f2(p,R)))+0.5f)/R)

#define P(p) texture(iChannel0, mod_f2f2(p,R)/R)
#define C(p) texture(iChannel1, mod_f2f2(p,R)/R)



__DEVICE__ float hash11(float p)
{
    p = fract(p * 0.1031f);
    p *= p + 33.33f;
    p *= p + p;
    return fract(p);
}

#define rand_interval 250
#define random_gen(a, b, seed) ((a) + ((b)-(a))*hash11(seed + float(iFrame/rand_interval)))


#define distribution_size 1.1f

/* FIRE
//mold stuff 
#define sense_num 6
#define sense_ang 1.
#define sense_dis 420.
#define sense_oscil 0.1
#define oscil_scale 1.
#define oscil_pow 2.
#define sense_force -0.35
#define distance_scale 0.2
#define force_scale 1.
#define trailing 0.
#define acceleration 0.
*/

#define sense_num 6
#define sense_ang 1.0f
#define sense_dis 150.0f
#define sense_oscil 0.1f
#define oscil_scale 1.0f
#define oscil_pow 1.0f
#define sense_force -0.35f
#define distance_scale 0.2f
#define force_scale 1.0f
#define trailing 0.0f
#define acceleration 0.0f


//SPH pressure
#define Pressure(rho) 0.5f*rho
#define fluid_rho 0.2f

//useful functions
#define GS(x) _expf(-dot(x,x))
#define GS0(x) _expf(-length(x))
#define Dir(ang) to_float2(_cosf(ang), _sinf(ang))
#define Rot(ang) to_mat2(_cosf(ang), _sinf(ang), -_sinf(ang), _cosf(ang))
#define loop(i,x) for(int i = 0; i < x; i++)
#define range(i,a,b) for(int i = a; i <= b; i++)

//data packing
#define PACK(X) ( uint(round(65534.0f*clamp(0.5f*X.x+0.5f, 0.0f, 1.0f))) + \
           65535u*uint(round(65534.0f*clamp(0.5f*X.y+0.5f, 0.0f, 1.0f))) )   
               
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
    uint _X = PACK(_x);
	
	z._Uint = _X;
    //return uintBitsToFloat(X); 
	return (z._Float);
}

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Preset: Keyboard' to iChannel3
// Connect Buffer A 'Previsualization: Buffer B' to iChannel0




__KERNEL__ void FireballsFuse__Buffer_A(float4 U, float2 pos, float2 iResolution, int iFrame, sampler2D iChannel3)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    pos+=0.5f;
float AAAAAAAAAAAAAAAAAAA;
    int2 p = to_int2_cfloat(pos);
    
    float2 _X = to_float2_s(0);
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
        _X += center*m;
        V += V0*m;
      
        //add mass
        M += m;
    }
    
    //normalization
    if(M != 0.0f)
    {
        _X /= M;
        V /= M;
    }
    
    //initial condition
    if(iFrame < 1 || Reset)
    {
        _X = pos;
        V = to_float2_s(0.0f);
        M = 0.3f*GS(-pos/R);
    }
    
    _X = clamp(_X - pos, to_float2_s(-0.5f), to_float2_s(0.5f));
    U = to_float4(ENCODE(_X), ENCODE(V), M, 0.0f);

  SetFragmentShaderComputedColor(U);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer C' to iChannel1


__KERNEL__ void FireballsFuse__Buffer_B(float4 U, float2 pos, float2 iResolution, float4 iMouse)
{
    pos+=0.5f;
float BBBBBBBBBBBBBBBBBB;
    float2 uv = pos/R;
    int2 p = to_int2_cfloat(pos);
        
    float4 data = T(pos); 
    float2 _X = DECODE(data.x) + pos;
    float2 V = DECODE(data.y);
    float M = data.z;
    
    if(M != 0.0f) //not vacuum
    {
        //Compute the SPH force
        float2 F = to_float2_s(0.0f);
        float3 avgV = to_float3_s(0.0f);
        range(i, -2, 2) range(j, -2, 2)
        {
            float2 tpos = pos + to_float2(i,j);
            float4 data = T(tpos);

            float2 X0 = DECODE(data.x) + tpos;
            float2 V0 = DECODE(data.y);
            float M0 = data.z;
            float2 dx = X0 - _X;
            
            float avgP = 0.5f*M0*(Pressure(M) + Pressure(M0)); 
            F -= 0.5f*GS(1.0f*dx)*avgP*dx;
            avgV += M0*GS(1.0f*dx)*to_float3_aw(V0,1.0f);
        }
        //swi2(avgV,x,y) /= avgV.z;
        avgV.x /= avgV.z;
        avgV.y /= avgV.z;

        float ang = _atan2f(V.y, V.x);
        float dang = sense_ang*PI/(float)(sense_num);
        float2 slimeF = to_float2_s(0.0f);
        //slime mold sensors
        range(i, -sense_num, sense_num)
        {
          float cang = ang + (float)(i) * dang;
          float2 dir = (1.0f + sense_dis*_powf(M, distance_scale))*Dir(cang);
          float3 s0 = swi3(C(_X + dir),x,y,z);  
          float fs = _powf(s0.z, force_scale);
          float os = oscil_scale*_powf(s0.z - M, oscil_pow);
          slimeF +=  sense_oscil*mul_mat2_f2(Rot(os),swi2(s0,x,y)) 
                 + sense_force*Dir(ang + sign_f((float)(i))*PI*0.5f)*fs; 
        }
        
        //remove acceleration component and leave rotation
        slimeF -= dot(slimeF, normalize(V))*normalize(V);
        F += slimeF/(float)(2*sense_num);
        
        if(iMouse.z > 0.0f)
        {
            float2 dx= pos - swi2(iMouse,x,y);
             F += 0.6f*dx*GS(dx/20.0f);
        }
        
        //integrate velocity
        V += F*dt/M;
        
        //acceleration for fun effects
        V *= 1.0f + acceleration;
        
        //velocity limit
        float v = length(V);
        V /= (v > 1.0f)?1.0f*v:1.0f;
    }
    
    //mass decay
   // M *= 0.999f;
    
    //input
    //if(iMouse.z > 0.0f)
    //\\  M = _mix(M, 0.5f, GS((pos - swi2(iMouse,x,y))/13.0f));
    //else
     //   M = _mix(M, 0.5f, GS((pos - R*0.5f)/13.0f));
    
    //save
    _X = clamp(_X - pos, to_float2_s(-0.5f), to_float2_s(0.5f));
    U = to_float4(ENCODE(_X), ENCODE(V), M, 0.0f);

  SetFragmentShaderComputedColor(U);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


#define radius 1.0f

__KERNEL__ void FireballsFuse__Buffer_C(float4 fragColor, float2 pos, float2 iResolution)
{
    pos+=0.5f;
float CCCCCCCCCCCCCCCCCCCC;    
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

        float K = GS(dx/radius)/(radius*radius);
        rho += M0*K;
        vel += M0*K*V0;
    }

    vel /= rho;

    fragColor = to_float4(vel.x, vel.y, rho, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: Forest_0' to iChannel2
// Connect Image 'Cubemap: Forest Blurred_0' to iChannel3
// Connect Image 'Previsualization: Buffer C' to iChannel0


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

#define FOV 1.5f
#define RAD R.x*0.7f

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
    //swi2(pos,x,y) = mod_f2(swi2(pos,x,y), R-1.0f);
    pos.x = mod_f(pos.x, R.x-1.0f);
    pos.y = mod_f(pos.y, R.y-1.0f);

    
    float de = 1e10f;
    float4 v = P(swi2(pos,x,y));
    return v.z;
}

__DEVICE__ float DE(float3 pos, float2 R, __TEXTURE2D__ iChannel0)
{
    float _y = 4.0f*_tanhf(0.5f*rho(pos,R,iChannel0));  
    
    //swi2(pos,x,y) += R*0.5f;
    pos.x += R.x*0.5f;
    pos.y += R.y*0.5f;
    //swi2(pos,x,y) = mod_f2(swi2(pos,x,y), R-1.0f);
    pos.x = mod_f(pos.x, R.x-1.0f);
    pos.y = mod_f(pos.y, R.y-1.0f);
    float zzzzzzzzzzzzzzzzz;
    float de = 1e10f;
    de = _fminf(de, 0.7f*sdBox(pos - to_float3_aw(R, 4.0f*_y)*0.5f, to_float3_aw(R*0.51f, 3.0f)));
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
    for(float i = 0.0f; i < marches; i++)
    {
        d = DE(p,R,iChannel0); 
        p += r*d;
        if(d < min_d || d > R.x) break;
    }
    return to_float4_aw(p, d);
}

__KERNEL__ void FireballsFuse(float4 col, float2 pos, float2 iResolution, float iTime, float4 iMouse, sampler2D iChannel2)
{
    pos+=0.5f;
float IIIIIIIIIIIIIIIII;
    #ifdef heightmap
        // Normalized pixel coordinates 
        pos = (pos - R*0.5f)/_fmaxf(R.x,R.y);

        float2 uv = swi2(iMouse,x,y)/R;
        float2 angles = to_float2(0.5f, -0.5f)*PI;

        float3 camera_z = to_float3(_cosf(angles.x)*_cosf(angles.y),_sinf(angles.x)*_cosf(angles.y),_sinf(angles.y));
        float3 camera_x = normalize(to_float3(_cosf(angles.x+PI*0.5f), _sinf(angles.x+PI*0.5f),0.0f)); 
        float3 camera_y = -normalize(cross(camera_x,camera_z));

        //tracking particle
        float4 fp = to_float4_f2f2(R*0.5f + 0.0f*to_float2(150.0f*iTime, 0.0f), to_float2_s(0.0f));

        float3 ray = normalize(camera_z + FOV*(pos.x*camera_x + pos.y*camera_y));
        float3 cam_pos = to_float3_aw(swi2(fp,x,y)-R*0.5f, 0.0f) - RAD*to_float3(_cosf(angles.x)*_cosf(angles.y),_sinf(angles.x)*_cosf(angles.y),_sinf(angles.y));

        float4 _X = ray_march(cam_pos, ray,R);

        if(_X.w < min_d)
        {

            float D = rho(swi3(_X,x,y,z),R);
            float3 albedo = to_float3(1,0.3f,0.3f) + sin_f3(1.0f*to_float3(1.0f,0.2f,0.1f)*D);

            float4 N0 = calcNormal(swi3(_X,x,y,z), 2.0f*_X.w,R)*to_float4(1.0f,1.0f,1.0f,1.0f);
            float3 n = normalize(swi3(N0,x,y,z));
            float3 rd = reflect(ray, n);
            float3 colA = swi3(decube_f3(iChannel2,  swi3(rd,y,z,x)),x,y,z);
            float3 colB = 0.6f*(to_float3_s(0.5f) + 0.5f*dot(rd, normalize(to_float3_s(1.0f))));
            colB += 3.0f*_powf(_fmaxf(dot(rd, normalize(to_float3_s(1.0f))), 0.0f), 10.0f);
            colB += 3.0f*_powf(_fmaxf(dot(rd, normalize(to_float3(-1,-0.5f,0.8f))), 0.0f), 10.0f);
            float b = clamp(0.5f + 0.5f*dot(n, normalize(to_float3(1,1,1))), 0.0f,1.0f);
            float K = 1.0f - _powf(_fmaxf(dot(n,rd),0.0f), 4.0f);
            swi3S(col,x,y,z, 1.0f*albedo*colB + 0.3f*colA*K);
        }
        else
        {    
            //background
            col = 1.0f*decube_f3(iChannel2,  swi3(ray,y,z,x));
        }
    col = tanh_f3(1.3f*col*col);
    #else
      float r = P(swi2(pos,x,y)).z;
    
      swi3S(col,x,y,z, 3.0f*sin_f3(0.1f*to_float3(3,0.9f,0.4f)*r));
    #endif

  SetFragmentShaderComputedColor(col);
}
