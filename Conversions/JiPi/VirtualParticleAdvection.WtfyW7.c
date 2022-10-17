
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution

__DEVICE__ float4 texelFetchC( __TEXTURE2D__ Channel, int2 pos, float2 R)
{
    if ( (pos.x) > 0 && (pos.x) < (int)(R.x) && (pos.y) > 0 && (pos.y) < (int)(R.y) )
    {
        return _tex2DVecN( Channel, (pos.x+0.5f)/R.x,(pos.y+0.5f)/R.y,15 );
    }
    else
        return to_float4_s(0);
}

#define Bf(p) p
#define Bi(p) to_int2_cfloat(p)
#define texel(a, p) texelFetchC(a, Bi(p), R)
#define texel2(a, p) texelFetchC(a, (p), R) // Buffer A

#define pixel(a, p) texture(a, (p)/R)
#define ch0 iChannel0
#define ch1 iChannel1
#define ch2 iChannel2
#define ch3 iChannel3

#define PI 3.14159265

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

__DEVICE__ float2 Dir(float ang)
{
    return to_float2(_cosf(ang), _sinf(ang));
}


__DEVICE__ float sdBox( in float2 p, in float2 b )
{
    float2 d = abs_f2(p)-b;
    return length(_fmaxf(d,to_float2_s(0.0f))) + _fminf(max(d.x,d.y),0.0f);
}

__DEVICE__ float sdSph( in float2 p, in float r )
{
    return length(p) - r; 
}


__DEVICE__ float border(float2 p, float2 R)
{
    float bound = -sdBox(p - R*0.5f, R*to_float2(0.52f, 0.5f)); 
    
    float box = sdSph(p - R*to_float2(0.2f, 0.3f), R.x*0.05f);
    box =_fminf(box, sdSph(p - R*to_float2(0.2f, 0.7f), R.x*0.05f));
    float drain = -sdBox(p - R*to_float2(0.9f, 0.05f), to_float2_s(0));
    return _fmaxf(drain,_fminf(bound, box));
}

#define h 0.1f
__DEVICE__ float3 bN(float2 p, float2 R)
{
    
    float3 dx = to_float3(-h,0,h);
    float4 idx = to_float4(-1.0f/h, 0.0f, 1.0f/h, 0.25f);
    float3 r = swi3(idx,z,y,w)*border(p + swi2(dx,z,y),R)
             + swi3(idx,x,y,w)*border(p + swi2(dx,x,y),R)
             + swi3(idx,y,z,w)*border(p + swi2(dx,y,z),R)
             + swi3(idx,y,x,w)*border(p + swi2(dx,y,x),R);
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
    Zahl z;
    //uint v = floatBitsToUint(x.x);
    //uint m = floatBitsToUint(x.y);
    z._Float = _x.x;
    
    return to_float4_f2f2(unpack(z._Uint),swi2(_x,y,y)); 
}

__DEVICE__ float2 encode(float4 _x)
{
    Zahl z;
  
    uint v = pack(swi2(_x,x,y));

	  z._Uint = v;
    
    //uint m = pack(x.zw/1.0f);
    //return to_float2(uintBitsToFloat(v),x.z); 
    return to_float2(z._Float, _x.z);
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
    return 0.0f*dx*_expf(-dot(dx,dx));
}

__DEVICE__ float2 Pzw(float2 p, float2 R, __TEXTURE2D__ ch1)
{
    return swi2(pixel(ch1, p),z,w);
}

//diffusion amount
#define dif 0.93f
__DEVICE__ float3 distribution(float2 _x, float2 p)
{

    float4 aabb0 = to_float4_f2f2(p - 0.5f, p + 0.5f);
    float4 aabb1 = to_float4_f2f2(_x - dif*0.5f, _x + dif*0.5f);
    float4 aabbX = to_float4_f2f2(_fmaxf(swi2(aabb0,x,y), swi2(aabb1,x,y)), _fminf(swi2(aabb0,z,w), swi2(aabb1,z,w)));
    float2 center = 0.5f*(swi2(aabbX,x,y) + swi2(aabbX,z,w)); //center of mass
    float2 size = _fmaxf(swi2(aabbX,z,w) - swi2(aabbX,x,y), to_float2_s(0.0f)); //only positive
    float m = size.x*size.y/(dif*dif); //relative amount
    //if any of the dimensions are 0 then the mass is 0
    return to_float3_aw(center, m);
}

__KERNEL__ void VirtualParticleAdvectionFuse__Buffer_A(float4 U, float2 pos, float iTime, float2 iResolution, float4 iMouse, int iFrame)
{
    //R = iResolution; time = iTime;
    int2 p = to_int2_cfloat(pos);
        
    //particle velocity, mass and grid distributed density
    float4 vm = to_float4_s(0.0f);
    float2 F = to_float2(0.0f, -0.00f);
    float2 dF = to_float2_s(0.0f);
    
    //particle position
    float2 _x = pos*vm.z;

    //reintegration advection
    //basically sum over all updated neighbors 
    //that fall inside of this pixel
    //this makes the tracking conservative
    range(i, -2, 2) range(j, -2, 2)
    {
        float4 data = texel2(ch0, p + to_int2(i,j)); // Schon int2
        float4 vm0 = decode(swi2(data,z,w));
       
        float2 vv = swi2(vm0,x,y);
        float2 xx = swi2(data,x,y) + vv*dt; //integrate position

        float3 D = distribution(xx, pos);

        //the deposited mass into this cell
        float m = vm0.z*D.z;
        //local center of mass in this cell
        xx = swi2(D,x,y); 

        //add weighted positions by mass
        _x += xx*m;
        //add weighted velocities by mass
        //swi2(vm,x,y) += vv*m;
        vm.x += vv.x*m;
        vm.y += vv.y*m;
        
        //add mass
        vm.z += m;
    }
    
    if(vm.z != 0.0f)
    {
        //normalize
        _x /= vm.z;
        swi2S(vm,x,y, swi2(vm,x,y) / vm.z);

        //update velocity
        //border 
        float3 N = bN(_x,R);
        N.z += 0.0001f;
        
        if(N.z < 0.0f) vm.z*=0.99f;

        float vdotN = step(_fabs(N.z), border_h)*dot(swi2(N,x,y), swi2(vm,x,y));
        swi2(vm,x,y) = swi2(vm,x,y) - 1.0f*(swi2(N,x,y)*vdotN + swi2(N,x,y)*_fabs(vdotN));
        F += swi2(N,x,y)*step(_fabs(N.z), border_h)/N.z;

         float3 dx = to_float3(-1.0f, 0.0f, 1.0f) + 1.0f;
        //global force field
        float2 pressure = Pzw(_x, R, ch1);

        F += 0.4f*pressure - 0.01f*swi2(vm,x,y)*step(N.z, border_h + 5.0f);
        if(iMouse.z > 0.0f)
        {
            float2 dm =(swi2(iMouse,x,y) - swi2(iMouse,z,w))/10.0f; 
            float d = distance_f2(swi2(iMouse,x,y), _x)/20.0f;
            F += 0.01f*dm*_expf(-d*d);
        }
        swi2S(vm,x,y, swi2(vm,x,y) + 0.4f*F*dt);
        
        //velocity limit
        float v = length(swi2(vm,x,y));
        swi2S(vm,x,y, swi2(vm,x,y) / ((v > 1.0f)?v:1.0f));
    }

    if(pos.x < 1.0f)
    {
        _x = pos;
        swi3S(vm,x,y,z, to_float3(0.6f, 0.0f, 0.5f*mass));
    }
    
    //initial condition
    if(iFrame < 1)
    {
        //random
        float3 rand = hash32(pos);
        if(rand.z < 0.1f) 
        {
            _x = pos;
            float2 tmp = 0.5f*(swi2(rand,x,y)-0.5f) + to_float2(0.5f, 0.0f);
            vm = to_float4(tmp.x,tmp.y, 4.0f*mass, mass);
        }
        else
        {
          _x = pos;
          vm = to_float4_s(0.0f);
        }
    }
    
    U = to_float4_f2f2(_x, encode(vm));


  SetFragmentShaderComputedColor(U);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1


//velocity blur


__KERNEL__ void VirtualParticleAdvectionFuse__Buffer_B(float4 U, float2 pos, float iTime, float2 iResolution)
{


    U = texel(ch1, pos);
    float4 av = to_float4_s(0.0f); float s = 0.0001f;
    range(i, -3, 3) range(j, -3, 3)
    {
        float2 dx = to_float2(i,j);
        float4 dc = decode(swi2(texel(ch0, pos + dx),z,w));
        float k = dc.z*G(dx/1.0f);
        s += k;
        av += k*swi4(dc,x,y,z,z);
    }
    U = av/s; 


  SetFragmentShaderComputedColor(U);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0
// Connect Buffer C 'Previsualization: Buffer D' to iChannel1


//pressure solve

__DEVICE__ float2 V(float2 p, float2 R, __TEXTURE2D__ ch0)
{
    float4 d =texel(ch0, p); 
    return swi2(d,x,y);
}

__DEVICE__ float Px(float2 p, float2 R, __TEXTURE2D__ ch1)
{
    return texel(ch1, p).x;
}

__KERNEL__ void VirtualParticleAdvectionFuse__Buffer_C(float4 U, float2 pos, float iTime, float2 iResolution)
{

    float b = border(pos, R);
      
    
    float3 dx = to_float3(-1.0f, 0.0f, 1.0f);

    float _div = 0.5*(V(pos + swi2(dx,z,y),R,ch0).x - V(pos + swi2(dx,x,y),R,ch0).x +
                  V(pos + swi2(dx,y,z),R,ch0).y - V(pos + swi2(dx,y,x),R,ch0).y);

    //neighbor average
    float L = 0.25f*(Px(pos + swi2(dx,z,y),R,ch1) + Px(pos + swi2(dx,x,y),R,ch1) +
                     Px(pos + swi2(dx,y,z),R,ch1) + Px(pos + swi2(dx,y,x),R,ch1));
    U.x = 0.995f*L + _div;
  

  SetFragmentShaderComputedColor(U);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer B' to iChannel0
// Connect Buffer D 'Previsualization: Buffer C' to iChannel1


//pressure solve + gradient

//__DEVICE__ float2 V(float2 p)
//{
//    float4 d =texel(ch0, p); 
//    return swi2(d,x,y);
//}

__DEVICE__ float sqr(float _x)
{
  return _x*_x;
}

__DEVICE__ float Px2(float2 p, float2 R, __TEXTURE2D__ ch0, __TEXTURE2D__ ch1)
{
    return -0.08f*texel(ch0, p).z+ texel(ch1, p).x;
}

__KERNEL__ void VirtualParticleAdvectionFuse__Buffer_D(float4 U, float2 pos, float iTime, float2 iResolution)
{
   
    float b = border(pos, R);
      
    if(b > 0.0f || true) 
    {
        float3 dx = to_float3(-1.0f, 0.0f, 1.0f);
        //velocity divergence
        float _div = 0.5f*(V(pos + swi2(dx,z,y),R,ch0).x - V(pos + swi2(dx,x,y),R,ch0).x +
                          V(pos + swi2(dx,y,z),R,ch0).y - V(pos + swi2(dx,y,x),R,ch0).y);
        //neighbor average
        float L = 0.25f*(Px2(pos + swi2(dx,z,y),R,ch0,ch1) + Px2(pos + swi2(dx,x,y),R,ch0,ch1) +
                         Px2(pos + swi2(dx,y,z),R,ch0,ch1) + Px2(pos + swi2(dx,y,x),R,ch0,ch1));
        U.x = 0.995f*L + _div;
    }  
    
    
    float3 dx = to_float3(-1.0f, 0.0f, 1.0f);
    //global force field
    float2 pressure = 0.5f*to_float2(Px2(pos + swi2(dx,z,y),R,ch0,ch1) - Px2(pos + swi2(dx,x,y),R,ch0,ch1),
                                     Px2(pos + swi2(dx,y,z),R,ch0,ch1) - Px2(pos + swi2(dx,y,x),R,ch0,ch1));
    //swi2(U,z,w) = pressure;
    U.z = pressure.x;
    U.w = pressure.y;

  SetFragmentShaderComputedColor(U);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer D' to iChannel1


__DEVICE__ float3 hsv2rgb( in float3 c )
{
  float3 rgb = clamp( abs_f3(mod_f3(c.x*6.0f+to_float3(0.0f,4.0f,2.0f),6.0f)-3.0f)-1.0f, 0.0f, 1.0f );

  rgb = rgb*rgb*(3.0f-2.0f*rgb); // cubic smoothing  

  return c.z * _mix( to_float3_s(1.0f), rgb, c.y);
}

__KERNEL__ void VirtualParticleAdvectionFuse(float4 fragColor, float2 pos, float iTime, float2 iResolution)
{


    CONNECT_COLOR0(Color, 1.0f, 2.0f, 3.0f, 1.0f);

  //R = iResolution; time = iTime;
    //pos = R*0.5f + pos*0.1f;
    int2 p = to_int2_cfloat(pos);
    
    //cur particle
    float4 U = decode(swi2(texel(ch0, pos),z,w));
    
    //pressure
    float4 P = texture(ch1, pos/R);
    
    //border render
    float3 bord = smoothstep(border_h-1.0f,border_h-3.0f,border(pos,R))*to_float3_s(1.0f);
    
    //particle render
    float rho = 0.0f;
    range(i, -1, 1) range(j, -1, 1)
    {
        float4 data = texel2(ch0, p + to_int2(i,j));
        float4 vm0 = decode(swi2(data,z,w));
        float2 x0 = swi2(data,x,y); //update position
        //how much mass falls into this pixel
        rho += 1.0f*vm0.z*G((pos - x0)/0.5f);
    }


    // Output to screen
    fragColor = to_float4_aw(tanh_f3(4.0f*to_float3(1.0f,2.0f,3.0f)*rho) + bord + 0.0f*_fabs(P.x), 1.0f);// Color.w);

  SetFragmentShaderComputedColor(fragColor);
}