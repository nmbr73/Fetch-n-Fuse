
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define Bf(p) p
#define Bi(p) to_int2_cfloat(p)

#define texel(a, p) texelFetchC(a, Bi(p), R)
#define pixel(a, p) texture(a, (p)/R)

#define ch0 iChannel0
#define ch1 iChannel1
#define ch2 iChannel2
#define ch3 iChannel3

#define PI 3.14159265f

#define loop(i,x) for(int i = 0; i < x; i++)
#define range(i,a,b) for(int i = a; i <= b; i++)

#define dt 1.5f

#define border_h 5.0f
#define R iResolution
//float4 Mouse;
//float time;


__DEVICE__ float4 texelFetchC( __TEXTURE2D__ Channel, int2 pos, float2 R)
{
    if ( (pos.x) >= 0 && (pos.x) < (int)(R.x) && (pos.y) > 0 && (pos.y) < (int)(R.y) )
    {
        return _tex2DVecN( Channel, (pos.x+0.5f)/R.x,(pos.y+0.5f)/R.y,15 );
    }
    else
        return to_float4_s(0);
}


__DEVICE__ float Pf(float2 rho)
{
    //return 0.2f*rho; //gas
    float GF = smoothstep(0.49f, 0.5f, 1.0f - rho.y);
    return _mix(0.5f*rho.x,0.04f*rho.x*(rho.x/0.2f - 1.0f + 0.1f*rho.y), GF); //water pressure
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

__DEVICE__ float border(float2 p, float time, float2 R)
{
    float bound = -sdBox(p - R*0.5f, R*to_float2(0.5f, 0.5f)); 
    float box = 1e10 + sdBox( mul_mat2_f2( Rot(0.4f*time),(p - R*0.5f)) , R*to_float2(0.005f, 0.2f));
    float drain = -sdBox(p - R*to_float2(0.9f, 0.05f), to_float2_s(0));
    return _fmaxf(drain,_fminf(bound, box));
}

#define h 1.
__DEVICE__ float3 bN(float2 p, float time, float2 R)
{
    float3 dx = to_float3(-h,0,h);
    float4 idx = to_float4(-1.0f/h, 0.0f, 1.0f/h, 0.25f);
    float3 r = swi3(idx,z,y,w)*border(p + swi2(dx,z,y),time,R)
             + swi3(idx,x,y,w)*border(p + swi2(dx,x,y),time,R)
             + swi3(idx,y,z,w)*border(p + swi2(dx,y,z),time,R)
             + swi3(idx,y,x,w)*border(p + swi2(dx,y,x),time,R);
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
#define dif 1.1f
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
    float Mi = 0.0f;
    range(i, -2, 2) range(j, -2, 2)
    {
        float2 tpos = pos + to_float2(i,j);
        float4 data = texel(ch, tpos);
    
        particle P0 = getParticle(data, tpos);
       
        P0.X += P0.V*dt; //integrate position

        float3 D = distribution(P0.X, pos, dif);
        float3 D1 = distribution(P0.X, pos, 1.0f);
        //the deposited mass into this cell
        float m = P0.M.x*D.z;
        
        //add weighted by mass
        P.X += swi2(D,x,y)*m;
        P.V += P0.V*m;
        
        P.M.y += P0.M.y*P0.M.x*D1.z;
        
        //add mass
        P.M.x += m;
        Mi += P0.M.x*D1.z;
    }
    
    //normalization
    if(P.M.x != 0.0f)
    {
        P.X /= P.M.x;
        P.V /= P.M.x;
    }
    if(Mi != 0.0f)
    {
        P.M.y /= Mi;
    }
    return P;
}

//force calculation and integration
__DEVICE__ particle Simulation(__TEXTURE2D__ ch, particle P, float2 pos, float2 R, float4 Mouse, float time)
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
        F -= 1.0f*G(1.0f*dx)*avgP*dx;
        avgV += P0.M.x*G(1.0f*dx)*to_float3_aw(P0.V,1.0f);
    }
    //swi2(avgV,x,y) /= avgV.z;
    avgV.x /= avgV.z;
    avgV.y /= avgV.z;


    //viscosity
    F += 0.0f*P.M.x*(swi2(avgV,x,y) - P.V);
    
    //gravity
    F += P.M.x*to_float2(0.0f, -0.0005f) + P.M.x*step(0.5f, P.M.y)*to_float2(0.0f, 0.005f);

    if(Mouse.z > 0.0f)
    {
        float2 dm =(swi2(Mouse,x,y) - swi2(Mouse,z,w))/10.0f; 
        float d = distance_f2(swi2(Mouse,x,y), P.X)/20.0f;
        F += 0.001f*dm*_expf(-d*d);
        P.M.y += 0.1f*_expf(-40.0f*d*d);
    }
    
    //integrate
    P.V += F*dt/P.M.x;

    //border 
    float3 N = bN(P.X, time, R);
    float vdotN = step(N.z, border_h)*dot(-1.0f*swi2(N,x,y), P.V);
    P.V += 1.0f*(swi2(N,x,y)*vdotN + swi2(N,x,y)*_fabs(vdotN));
    P.V += P.M.x*swi2(N,x,y)*step(_fabs(N.z), border_h)*_expf(-N.x);
    
        
    if(length(to_float2(1.0f, 1.0f)*(P.X - R*to_float2(0.5f, 0.1f))) < 10.0f) P.M.y = _mix(P.M.y, 1.0f, 0.06f);
    if(N.z < 2.0f*border_h) P.M.y *= 0.9f;

    
    //velocity limit
    float v = length(P.V);
    P.V /= (v > 1.0f)?v:1.0f;
    
    return P;
}



// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer B' to iChannel0


#define mass 1.0f

__KERNEL__ void VirtualParticleSphBoilingFuse__Buffer_A(float4 U, float2 pos, float iTime, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0);
    pos+=0.5f;

    //R = iResolution; time = iTime; Mouse = iMouse;
    int2 p = to_int2_cfloat(pos);
        
    //particle velocity, mass and grid distributed density
    float2 F = to_float2_s(0.0f);
    
    float4 data = texel(ch0, pos); 
    
    particle P = {to_float2_s(0.0),to_float2_s(0.0),to_float2_s(0.0)};// = getParticle(data, pos);
       
    P = Reintegration(ch0, P, pos, R);
   
    //initial condition
    if(iFrame < 1 || Reset)
    {
        //random
        float3 rand = hash32(pos);
        if(rand.z < 0.3f) 
        {
            P.X = pos;
            P.V = 0.5f*(swi2(rand,x,y)-0.5f) + to_float2(0.0f, 0.0f);
            P.M = to_float2(mass, 0.0f);
        }
        else
        {
            P.X = pos;
            P.V = to_float2_s(0.0f);
            P.M = to_float2_s(0.0f);
        }
    }
    
    U = saveParticle(P, pos);


  SetFragmentShaderComputedColor(U);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Texture: Blending' to iChannel1

__DEVICE__ float hash12(float2 p)
{
    float3 p3  = fract_f3(swi3(p,x,y,x) * 0.1031f);
    p3 += dot(p3, swi3(p3,y,z,x) + 33.33f);
    return fract((p3.x + p3.y) * p3.z);
}


__DEVICE__ particle Blending( __TEXTURE2D__ channel, float2 uv, particle P, float Blend, float2 Par, float2 MulOff, int Modus, float2 fragCoord, float2 R, float MAX_SPEED)
{
 
    if (Blend > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(channel,uv);

      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2) //Startbedingung
        {
          float q = 2.0f*PI * hash12(1.0f + fragCoord);
          P.X = _mix(P.X, fragCoord, Blend);
          P.V = _mix(P.V, MAX_SPEED * to_float2(_cosf(q), _sinf(q)), Blend);
          P.M = _mix(P.M, 0.45f - abs_f2(fragCoord/iResolution - 0.5f), Blend);
        }      
        if ((int)Modus&4) // Geschwindigkeit
        {
          P.V = Par.x*_mix(P.V, to_float2((tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x), Blend);
        }
        
        if ((int)Modus&8) // Masse
          P.M = _mix(P.M, (swi2(tex,x,y)+MulOff.y)*MulOff.x, Blend);

      }
      else
      {
        if ((int)Modus&16) 
          P.M = _mix(P.M, (MulOff.y+0.45) - abs_f2(fragCoord/iResolution - 0.5f), Blend);
      
        if ((int)Modus&32) //Special
        {
          float q = 2.0f*PI * hash12(1.0f + fragCoord);
          P.X = fragCoord;
          P.V = MAX_SPEED * to_float2(_cosf(q), _sinf(q));
          P.M = 0.45f - abs_f2(fragCoord/iResolution - 0.5f);
        }  
      }
    }
  
  return P;
}


__KERNEL__ void VirtualParticleSphBoilingFuse__Buffer_B(float4 U, float2 pos, float iTime, float2 iResolution, float4 iMouse)
{
    CONNECT_CHECKBOX0(Reset, 0);
    
    //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);
    
    CONNECT_SLIDER5(MAX_SPEED, -10.0f, 10.0f, 0.50f);

    pos+=0.5f;

    //R = iResolution; time = iTime; Mouse = iMouse;
    int2 p = to_int2_cfloat(pos);
        
    //particle velocity, mass and grid distributed density
    float2 F = to_float2_s(0.0f);
    
    float4 data = texel(ch0, pos); 
    
    particle P = getParticle(data, pos);
    
    
    if(P.M.x != 0.0f) //not vacuum
    {
        P = Simulation(ch0, P, pos, R, iMouse, iTime);
    }
    
    if (Blend1>0.0) P = Blending(iChannel1, pos/R, P, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, pos, R, MAX_SPEED);  
    
    U = saveParticle(P, pos);


  SetFragmentShaderComputedColor(U);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel2
// Connect Image 'Previsualization: Buffer D' to iChannel1


__DEVICE__ float3 hsv2rgb( in float3 c )
{
  float3 rgb = clamp( abs_f3(mod_f3(c.x*6.0f+to_float3(0.0f,4.0f,2.0f),6.0f)-3.0f)-1.0f, 0.0f, 1.0f );

  rgb = rgb*rgb*(3.0f-2.0f*rgb); // cubic smoothing  

  return c.z * _mix( to_float3_s(1.0f), rgb, c.y);
}

__KERNEL__ void VirtualParticleSphBoilingFuse(float4 fragColor, float2 pos, float iTime, float2 iResolution)
{
    CONNECT_COLOR0(Color1, 0.2f, 0.4f, 1.0f, 1.0f); 
    CONNECT_COLOR1(Color2, 1.5f, 0.3f, 0.3f, 1.0f); 
    CONNECT_COLOR2(Color3, 1.0f, 1.1f, 1.3f, 1.0f); 


    //R = iResolution; time = iTime;
    //pos = R*0.5f + pos*0.1f;
    int2 p = to_int2_cfloat(pos);
    
    //pressure
    float4 P = texture(ch1, pos/R);
    
    //border render
    float3 bord = smoothstep(border_h+1.0f,border_h-1.0f,border(pos, iTime,R))*to_float3_s(1.0f);
  
    //particle render
    float2 rho = to_float2_s(0.0f);

    range(i, -1, 1) range(j, -1, 1)
    {
       float2 dx = to_float2(i,j);
       float4 data = texel(ch0, pos + dx);
       particle P = getParticle(data, pos + dx);
       
        float2 x0 = P.X; //update position
        //how much mass falls into this pixel
        rho += 1.0f*P.M*G((pos - x0)/0.75f); 
    }
    rho = 1.2f*rho;
    
     float4 D = pixel(ch2, pos);
    float ang = _atan2f(D.x, D.y);
    float mag = 0.0f + 10.0f*length(swi2(D,x,y))*rho.x;
    
    // Output to screen
    //fragColor = to_float4_aw(1.6f*to_float3(0.2f,0.4f,1.0f)*rho.x + 1.0f*to_float3(1.5f,0.3f,0.3f)*rho.y*rho.x + bord + 0.0f*_fabs(P.x),0);
    //swi3S(fragColor,x,y,z, tanh_f3(to_float3(1.0f,1.1f,1.3f)*swi3(fragColor,x,y,z)));


    fragColor = to_float4_aw(1.6f*swi3(Color1,x,y,z)*rho.x + 1.0f*swi3(Color2,x,y,z)*rho.y*rho.x + bord + 0.0f*_fabs(P.x),Color1.w);
    swi3S(fragColor,x,y,z, tanh_f3(swi3(Color3,x,y,z)*swi3(fragColor,x,y,z)));

  SetFragmentShaderComputedColor(fragColor);
}
