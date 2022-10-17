
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
#define time iTime

__DEVICE__ float4 texelFetchC( __TEXTURE2D__ Channel, int2 pos, float2 R)
{
    if ( (pos.x) >= 0 && (pos.x) < (int)(R.x) && (pos.y) >= 0 && (pos.y) < (int)(R.y) )
    {
        return _tex2DVecN( Channel, (pos.x+0.5f)/R.x,(pos.y+0.5f)/R.y,15 );
    }
    else
        return to_float4_s(0);
}


#define Bf(p) p
#define Bi(p) to_int2_cfloat(p)
//#define texel(a, p) texelFetch(a, Bi(p), 0) //Original
#define pixel(a, p) texture(a, (p)/R)



#define texel(a, p) texture(a, (make_float2(Bi(p))+0.5f)/R)
#define texelint(a, p) texture(a, (make_float2((p))+0.5f)/R)

//#define texelint(a, p) texelFetchC(a, (p), R)

#define texel1(a, p) texelFetchC(a, Bi(p), R)


#define texel2(a, p) texelFetchC(a, (p), R) // Buffer A



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
//float time;

__DEVICE__ mat2 Rot(float ang)
{
    return to_mat2(_cosf(ang), -_sinf(ang), _sinf(ang), _cosf(ang)); 
}


__DEVICE__ float sdBox( in float2 p, in float2 b )
{
    float2 d = abs_f2(p)-b;
    return length(_fmaxf(d,to_float2_s(0.0f))) + _fminf(max(d.x,d.y),0.0f);
}


__DEVICE__ float border(float2 p, float iTime, float2 R)
{
  
    float bound = -sdBox(p - R*0.5f, R*0.5f); 
    float box = sdBox(mul_mat2_f2(Rot(0.5f*time),(p - R*0.5f)), R*to_float2(0.1f, 0.01f));
    float drain = -sdBox(p - R*to_float2(0.9f, 0.05f), to_float2_s(50));
    return _fmaxf(drain,_fminf(bound, box));
}

#define h 0.1f
__DEVICE__ float3 bN(float2 p, float iTime, float2 R)
{
    float3 dx = to_float3(-h,0,h);
    float4 idx = to_float4(-1.0f/h, 0.0f, 1.0f/h, 0.25f);
    float3 r = swi3(idx,z,y,w)*border(p + swi2(dx,z,y),iTime,R)
             + swi3(idx,x,y,w)*border(p + swi2(dx,x,y),iTime,R)
             + swi3(idx,y,z,w)*border(p + swi2(dx,y,z),iTime,R)
             + swi3(idx,y,x,w)*border(p + swi2(dx,y,x),iTime,R);
    return to_float3_aw(normalize(swi2(r,x,y)), r.z);
}

__DEVICE__ uint pack(float2 _x)
{
    _x = 65534.0f*clamp(0.5f*_x+0.5f, 0.0f, 1.0f);
    return uint(_x.x) + 65535u*uint(_x.y);
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

__DEVICE__ float4 decode(float2 _x)
{
    Zahl v,m;
  
    //uint v = floatBitsToUint(_x.x);
    v._Float = _x.x;
    
    //uint m = floatBitsToUint(_x.y);
    m._Float = _x.y;
    
    return to_float4_f2f2(unpack(v._Uint),unpack(m._Uint)*128.0f); 
}

__DEVICE__ float2 encode(float4 _x)
{
    Zahl zv, zm;
    
    uint v = pack(swi2(_x,x,y));
    uint m = pack(swi2(_x,z,w)/128.0f);
    
    zv._Uint = v;
    zm._Uint = m;
    
    //return to_float2(uintBitsToFloat(v),uintBitsToFloat(m)); 
    return to_float2(zv._Float, zm._Float); 
}



__DEVICE__ float3 hash32(float2 p)
{
    float3 p3 = fract_f3((swi3(p,x,y,x)) * to_float3(0.1031f, 0.1030f, 0.0973f));
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

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer D' to iChannel1


#define mass 0.1f
#define div 0.7f


__DEVICE__ float2 Force(float2 dx)
{
    return 20.0f*dx*_expf(-dot(dx,dx));
}

__KERNEL__ void ReintegrationFluidFuse__Buffer_A(float4 U, float2 pos, float iTime, float2 iResolution, float4 iMouse, int iFrame)
{

    pos+=0.5f;
    
    int2 p = to_int2_cfloat(pos);
        
    //particle position
    float2 _x = to_float2_s(0.0f);
    //particle velocity, mass and grid distributed density
    float4 vm = to_float4_s(0.0f); 
    float2 F = to_float2(0.0f, -0.0004f);
    float2 dF = to_float2_s(0.0f);
    
    //reintegration advection
    //basically sum over all updated neighbors 
    //that fall inside of this pixel
    //this makes the tracking conservative
    range(i, -2, 2) range(j, -2, 2)
    {
      
        float4 data = texelint(ch0, p + to_int2(i,j));
        float4 vm0 = decode(swi2(data,z,w));
        float2 x0 = swi2(data,x,y); //update position
        //how much mass falls into this pixel(is exact, 0 or full)
        vm.w += vm0.z*G((pos - x0)/1.5f);
        float D = step(_fmaxf(_fabs(pos.x - x0.x),
                              _fabs(pos.y - x0.y)), 0.5f);
        float D1 = 0.25f*step(_fmaxf(_fabs(pos.x - x0.x),
                              _fabs(pos.y - x0.y)), 1.00f);
        
        float pvm = vm0.z;
        vm0.z *= (pvm<4.0f*mass)?D:D1;
        if(pvm>=4.0f*mass && pvm > 0.0f) 
        {
            x0 = _mix(x0, pos, 0.5f);
        }
           
        //add weighted positions by mass
        _x += x0*vm0.z;
        //add weighted velocities by mass
        swi2S(vm,x,y, swi2(vm,x,y) + swi2(vm0,x,y)*vm0.z);
        //add mass
        vm.z += vm0.z;
        dF += vm0.z*Force(pos - x0)*(1.0f - D);
    }
    vm.z = clamp(vm.z, 0.0f, 2.0f);
    
    //if(vm.z != 0.0f || any(isnan(vm))) //not vacuum
    if(vm.z != 0.0f || isnan(vm.x) || isnan(vm.y) || isnan(vm.z) || isnan(vm.w) ) //not vacuum
    {
        //normalize
        _x /= vm.z;
        //swi2(vm,x,y) /= vm.z;
        vm.x /= vm.z;
        vm.y /= vm.z;
        
        float3 dx = to_float3(-1.0f,0,1.0f);
        float2 px = _x;
        float3 rand = hash32(pos+_x)-0.5f;
        
        //update velocity
        //border 
        float3 N = bN(px,iTime,R);
        float vdotN = step(_fabs(N.z), border_h)*dot(swi2(N,x,y), swi2(vm,x,y));
        swi2S(vm,x,y, swi2(vm,x,y) - 0.5f*(swi2(N,x,y)*vdotN + swi2(N,x,y)*_fabs(vdotN)));
        F += swi2(N,x,y)*step(_fabs(N.z), border_h)/N.z;
        
        //global force field
        
        float4 GF = pixel(ch1, px);


        F += (-1.0f*swi2(GF,x,y) + (swi2(GF,z,w) - swi2(vm,x,y))*0.15f - 0.01f*swi2(vm,x,y)*step(N.z, border_h + 5.0f)); 
        if(iMouse.z > 0.0f)
        {
            float2 dm =(swi2(iMouse,x,y) - swi2(iMouse,z,w))/10.0f; 
            float d = distance_f2(swi2(iMouse,x,y), _x)/20.0f;
            F += 0.01f*dm*_expf(-d*d);
        }
        swi2S(vm,x,y, swi2(vm,x,y) + 0.4f*F*dt/(0.0001f+vm.z));
        //velocity limit
        float v = length(swi2(vm,x,y));
        swi2S(vm,x,y, swi2(vm,x,y) / ((v > 1.0f)?v:1.0f));
        _x = _x + (swi2(vm,x,y) + dF)*dt; 
    }
    else
    {
        _x = pos;
        //swi3(vm,x,y,z) = to_float3_s(0.0f);
        vm.x = 0.0f;
        vm.y = 0.0f;
        vm.z = 0.0f;
        if(distance_f2(pos, R*to_float2(0.55f, 0.75f)) < 1.0f || distance_f2(pos, R*to_float2(0.45f, 0.75f)) < 1.0f)
        {
            //swi3(vm,x,y,z) = to_float3(0.0f, -0.52f, 3.0f*mass);
            vm.x = 0.0f;
            vm.y = -0.52f;
            vm.z = 3.0f*mass;
            
        }
        if(distance_f2(pos, R*to_float2(0.1f, 0.75f)) < 5.0f)
        {
            //swi3(vm,x,y,z) = to_float3(0.5f, -0.5f, 3.0f*mass);
            vm.x = 0.5f;
            vm.y = -0.5f;
            vm.z = 3.0f*mass;

        }
    }

    
    //initial condition
    if(iFrame < 1)
    {
        //random
        float3 rand = hash32(pos);
        if(rand.z < 0.05f) 
        {
            _x = pos;
            float2 tmp = 0.3f*(2.0f*swi2(rand,x,y) - to_float2_s(1.0f));
            vm = to_float4(tmp.x,tmp.y, 4.0f*mass, mass);
        }
        else
        {
            _x = pos;
            vm = to_float4_s(0.0f); //0.0f
        }
    }
    
    U = to_float4_f2f2(_x, encode(vm));

  SetFragmentShaderComputedColor(U);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


#define Radius 3
__KERNEL__ void ReintegrationFluidFuse__Buffer_B(float4 U, float2 pos, float iTime, float2 iResolution)
{

    pos+=0.5f;

    float4 avm = to_float4_s(0.0f);
    float sum = 1.0f;
    range(i, -Radius, Radius)
    {
        int2 p = to_int2_cfloat(pos) + to_int2(i,0);
        if(p.x >= 0 && p.x < int(R.x))
        {
          float k = G(to_float2(i,0)*1.5f/(float)(Radius));
          

          float4 d = decode(swi2(texelint(ch0, p),z,w));
          avm += to_float4_f2f2(swi2(d,x,y)*d.z, swi2(d,z,w))*k;
          sum += k;
        }
    }
    U = avm/sum;
    //swi2(U,x,y) = avm.xy/(avm.z+0.0001f); 
    U.x = avm.x/(avm.z+0.0001f); 
    U.y = avm.y/(avm.z+0.0001f); 

  SetFragmentShaderComputedColor(U);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


#define Radius 3
__KERNEL__ void ReintegrationFluidFuse__Buffer_C(float4 U, float2 pos, float2 iResolution)
{
    pos+=0.5f;

    float4 avm = to_float4_s(0.0f);
    float sum = 1.0f;
    range(i, -Radius, Radius)
    {
        int2 p = to_int2_cfloat(pos) + to_int2(0,i);
        float k = G(to_float2(0,i)*1.5f/(float)(Radius));
        float4 d = texelint(ch0, p);
        avm += to_float4_f2f2(swi2(d,x,y)*d.z, swi2(d,z,w))*k;
        sum += k;
    }
    U = avm/sum;
    U.z = 0.5f*U.z*clamp(_powf(_fabs(U.z/0.07f), 7.0f) - 1.0f, -1.0f, 1.0f); //water
    //U.z = 1.2f*U.z;//gas
    //swi2(U,x,y) = avm.xy/(avm.z+0.0001f); 
    U.x = avm.x/(avm.z+0.0001f); 
    U.y = avm.y/(avm.z+0.0001f); 

  SetFragmentShaderComputedColor(U);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


//force field

__KERNEL__ void ReintegrationFluidFuse__Buffer_D(float4 U, float2 pos, float2 iResolution)
{
   pos+=0.5f;
   
   float3 dx = to_float3(-1.0f, 0.0f, 1.0f);
   swi2S(U,x,y, 0.5f*to_float2(texel(ch0, pos + swi2(dx,z,y)).z - texel(ch0, pos + swi2(dx,x,y)).z,
                               texel(ch0, pos + swi2(dx,y,z)).z - texel(ch0, pos + swi2(dx,y,x)).z));
   //average velocity
   float4 a = texel(ch0, pos); 
//   swi2(U,z,w) = swi2(a,x,y);
   U.z = a.x;
   U.w = a.y;

  SetFragmentShaderComputedColor(U);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer C' to iChannel1
// Connect Image 'Previsualization: Buffer D' to iChannel2


__DEVICE__ float3 hsv2rgb( in float3 c )
{
  float3 rgb = clamp( abs_f3(mod_f3(c.x*6.0f+to_float3(0.0f,4.0f,2.0f),6.0f)-3.0f)-1.0f, 0.0f, 1.0f );

  rgb = rgb*rgb*(3.0f-2.0f*rgb); // cubic smoothing  

  return c.z * _mix( to_float3_s(1.0f), rgb, c.y);
}

__KERNEL__ void ReintegrationFluidFuse(float4 fragColor, float2 pos, float iTime, float2 iResolution)
{
  
    CONNECT_POINT0(Offset, 0.0f, 0.0f);
  
    pos+=0.5f;

    float4 U = decode(swi2(texel(ch0, pos),z,w));
    float4 P = texture(ch1, pos/R);
    float4 D = pixel(ch2, pos);
    float ang = _atan2f(D.w, D.z);
    float mag = length(swi2(D,z,w));
    float3 bord = smoothstep(border_h-1.0f,border_h-3.0f,border(pos,iTime,R))*to_float3_s(1.0f);
    float3 rho = to_float3(1.0f,1.7f,4.0f)*(0.0f*_fabs(U.z)+5.0f*smoothstep(0.1f, 0.25f, P.w));
    // Output to screen
    fragColor = to_float4_aw(sqrt_f3(rho)*(0.15f+hsv2rgb(to_float3(ang, 1.0f, mag))) + bord,0);


//fragColor = texelint(iChannel0,to_int2(pos.x+Offset.x,pos.y+Offset.y));

//fragColor = texture(iChannel0, (pos+Offset)/R);

//fragColor = texel1(iChannel0, pos+Offset);

  SetFragmentShaderComputedColor(fragColor);
}