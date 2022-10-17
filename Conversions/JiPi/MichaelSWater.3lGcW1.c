
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

//float2 R;
//float4 Mouse;
//float time;

#define tanh_f4(i) to_float4(_tanhf((i).x), _tanhf((i).y), _tanhf((i).z), _tanhf((i).w))

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution

#define Bf(p) p
#define Bi(p) to_int2_cfloat(p)
//#define texel(a, p) texelFetch(a, Bi(p), 0)
#define texel(a, p) texture(a, (make_float2(Bi(p))+0.5f)/R)
#define pixel(a, p) texture(a, (p)/R)
#define ch0 iChannel0
#define ch1 iChannel1
#define ch2 iChannel2
#define ch3 iChannel3

#define PI 3.14159265f

#define loop(i,x) for(int i = 0; i < x; i++)
#define loop(i,x) for(int i = 0; i < x; i++)
#define range(i,a,b) for(int i = a; i <= b; i++)

#define dt 1.5f

#define border_h 5.0f


#define mass 1.0f

#define fluid_rho 0.5f

__DEVICE__ float Pf(float2 rho)
{
    //return 0.2f*rho.x; //gas
    float GF = 1.0f;//smoothstep(0.49f, 0.5f, 1.0f - rho.y);
    return _mix(0.5f*rho.x,0.04f*rho.x*(rho.x/fluid_rho - 1.0f), GF); //water pressure
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
    float box = sdBox(mul_mat2_f2(Rot(0.0f*time),(p - R*to_float2(0.5f, 0.6f))) , R*to_float2(0.05f, 0.01f));
    float drain = -sdBox(p - R*to_float2(0.5f, 0.7f), R*to_float2(1.5f, 0.5f));
    return _fmaxf(drain,_fminf(bound, box));
}

#define h 1.0f
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
    return to_float4(encode(P.X), encode(P.V), P.M.x,P.M.y);
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
#define dif 1.12f

__DEVICE__ float3 distribution(float2 x, float2 p, float K)
{
    float2 omin = clamp(x - K*0.5f, p - 0.5f, p + 0.5f);
    float2 omax = clamp(x + K*0.5f, p - 0.5f, p + 0.5f); 
    return to_float3_aw(0.5f*(omin + omax), (omax.x - omin.x)*(omax.y - omin.y)/(K*K));
}

/*
__DEVICE__ float3 distribution(float2 x, float2 p, float K)
{
    float4 aabb0 = to_float4_aw(p - 0.5f, p + 0.5f);
    float4 aabb1 = to_float4_aw(x - K*0.5f, x + K*0.5f);
    float4 aabbX = to_float4(_fmaxf(swi2(aabb0,x,y), swi2(aabb1,x,y)), _fminf(swi2(aabb0,z,w), swi2(aabb1,z,w)));
    float2 center = 0.5f*(swi2(aabbX,x,y) + swi2(aabbX,z,w)); //center of mass
    float2 size = _fmaxf(swi2(aabbX,z,w) - swi2(aabbX,x,y), 0.0f); //only positive
    float m = size.x*size.y/(K*K); //relative amount
    //if any of the dimensions are 0 then the mass is 0
    return to_float3_aw(center, m);
}*/

//diffusion and advection basically
__DEVICE__ particle Reintegration(__TEXTURE2D__ ch, particle P, float2 pos, float2 R)
{
    //basically integral over all updated neighbor distributions
    //that fall inside of this pixel
    //this makes the tracking conservative
    range(i, -2, 2) range(j, -2, 2)
    {
        float2 tpos = pos + to_float2(i,j);
        float4 data = texel(ch, tpos);
       
        particle P0 = getParticle(data, tpos);
       
        P0.X += P0.V*dt; //integrate position

        float difR = 0.9f + 0.21f*smoothstep(fluid_rho*0.0f, fluid_rho*0.333f, P0.M.x);
        float3 D = distribution(P0.X, pos, difR);
        //the deposited mass into this cell
        float m = P0.M.x*D.z;
        
        //add weighted by mass
        P.X += swi2(D,x,y)*m;
        P.V += P0.V*m;
        P.M.y += P0.M.y*m;
        
        //add mass
        P.M.x += m;
    }
    
    //normalization
    if(P.M.x != 0.0f)
    {
        P.X /= P.M.x;
        P.V /= P.M.x;
        P.M.y /= P.M.x;
    }
    
    return P;
}

//force calculation and integration
__DEVICE__ particle Simulation(__TEXTURE2D__ ch, particle P, float2 pos, float time, float4 Mouse, float2 R)
{
    //Compute the SPH force
    float2 F = to_float2_s(0.0f);
    float3 avgV = to_float3_s(0.0f);
    
    /*
    float3 center = to_float3_s(0.0f);
    range(i, -2, 2) range(j, -2, 2)
    {
        float2 tpos = pos + to_float2(i,j);
        float4 data = texel(ch, tpos);
        particle P0 = getParticle(data, tpos);
        float2 dx = P0.X - P.X;
        if (length(dx) < 1.0f) {
            swi2(center,x,y) += dx;
            center.z += 1.0f;
        }
    }
    swi2(center,x,y) /= center.z;
    */
    
    range(i, -2, 2) range(j, -2, 2)
    {
        float2 tpos = pos + to_float2(i,j);
        float4 data = texel(ch, tpos);
        particle P0 = getParticle(data, tpos);
        float2 dx = P0.X - P.X;
        float avgP = 0.5f*P0.M.x*(Pf(P.M) + Pf(P0.M)); 
        F -= 0.5f*G(1.0f*dx)*avgP*dx;
        if (length(dx) < 1.0f) {
            float d = length(dx);
            //F -= 0.0001f * dx;
            //F *= 0.0000000001f * dx;
        }
        avgV += P0.M.x*G(1.0f*dx)*to_float3_aw(P0.V,1.0f);
    }
    //F /= to_float2_s(10.0f);
    //swi2(avgV,x,y) /= avgV.z;
    avgV.x /= avgV.z;
    avgV.y /= avgV.z;

    //viscosity
    F += 0.0f*P.M.x*(swi2(avgV,x,y) - P.V);
    
    //gravity
    F -= P.M.x*to_float2(0.0f, 0.0004f);
    float2 PDC = P.X - 0.5f * R;
    if (length(PDC) < length(0.1f * R)) {
        F -= P.M.x*0.0001f*(PDC);
    }
    //F -= P.M.x*0.01f*(swi2(center,x,y));


    if(Mouse.z > 0.0f)
    {
        float2 dm =(swi2(Mouse,x,y) - swi2(Mouse,z,w))/10.0f; 
        float d = distance_f2(swi2(Mouse,x,y), P.X)/20.0f;
        F += 0.001f*dm*_expf(-d*d);
       // P.M.y += 0.1f*_expf(-40.0f*d*d);
    }
    
    //integrate
    P.V += F*dt/P.M.x;

    //border 
    float3 N = bN(P.X,time,R);
    float vdotN = step(N.z, border_h)*dot(-1.0f*swi2(N,x,y), P.V);
    P.V += 0.5f*(swi2(N,x,y)*vdotN + swi2(N,x,y)*_fabs(vdotN));
    P.V += 0.0f*P.M.x*swi2(N,x,y)*step(_fabs(N.z), border_h)*_expf(-N.z);
    
    if(N.z < 0.0f) P.V = to_float2_s(0.0f);
    
    
    //velocity limit
    //P.V *= 0.997f;
    float v = length(P.V);
    P.V /= (v > 1.0f)?v:1.0f;
    
    return P;
}



// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer B' to iChannel0



__KERNEL__ void MichaelSWaterFuse__Buffer_A(float4 U, float2 pos, float iTime, float2 iResolution, float4 iMouse, int iFrame)
{


    pos+=0.5f;
    //int2 p = to_int2(pos);

    float4 data = texel(ch0, pos); 
    
    particle P = { to_float2_s(0.0f), to_float2_s(0.0f), to_float2_s(0.0f)};// = getParticle(data, pos);
       
    P = Reintegration(ch0, P, pos,R);
   
    //initial condition
    if(iFrame < 1)
    {
        //random
        float3 rand = hash32(pos);
        if(rand.z < 0.0f) 
        {
            P.X = pos;
            P.V = 0.5f*(swi2(rand,x,y)-0.5f) + to_float2(0.0f, 0.0f);
            P.M = to_float2(mass, 0.0f);
        }
        else
        {
            P.X = pos;
            P.V = to_float2_s(0.0f);
            P.M = to_float2_s(1e-6);
        }
    }
    
    U = saveParticle(P, pos);

  SetFragmentShaderComputedColor(U);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void MichaelSWaterFuse__Buffer_B(float4 U, float2 pos, float iTime, float2 iResolution, float4 iMouse)
{

    pos+=0.5f;
    //int2 p = to_int2(pos);
        
    float4 data = texel(ch0, pos); 
    
    particle P = getParticle(data, pos);
    
    
    if(P.M.x != 0.0f) //not vacuum
    {
        P = Simulation(ch0, P, pos, iTime, iMouse,R);
    }
    
    if(length(P.X - R*to_float2(0.8f, 0.9f)) < 10.0f) 
    {
        P.X = pos;
        P.V = 0.5f*Dir(-PI*0.25f - PI*0.5f + 0.3f*_sinf(0.4f*iTime));
        P.M = _mix(P.M, to_float2(fluid_rho, 1.0f), 0.4f);
    }

    if(length(P.X - R*to_float2(0.2f, 0.9f)) < 10.0f) 
    {
        P.X = pos;
        P.V = 0.5f*Dir(-PI*0.25f + 0.3f*_sinf(0.3f*iTime));
        P.M = _mix(P.M, to_float2(fluid_rho, 0.0f), 0.4f);
    }
    
    U = saveParticle(P, pos);

  SetFragmentShaderComputedColor(U);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0


//density

__KERNEL__ void MichaelSWaterFuse__Buffer_C(float4 fragColor, float2 pos, float iTime, float2 iResolution)
{

    pos+=0.5f;
    //int2 p = to_int2(pos);

    float4 data = texel(ch0, pos);
    particle P = getParticle(data, pos);
    
    //particle render
    float4 rho = to_float4_s(0.0f);
    range(i, -1, 1) range(j, -1, 1)
    {
        float2 ij = to_float2(i,j);
        float4 data = texel(ch0, pos + ij);
        particle P0 = getParticle(data, pos + ij);

        float2 x0 = P0.X; //update position
        //how much mass falls into this pixel
        rho += 1.0f*to_float4_f2f2(P.V, P.M)*G((pos - x0)/0.75f); 
    }
    
    fragColor = rho;

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Abstract 1' to iChannel3
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

__KERNEL__ void MichaelSWaterFuse(float4 col, float2 pos, float iTime, float2 iResolution, float2 iChannelResolution[], sampler2D iChannel0, sampler2D iChannel3)
{

    pos+=0.5f;
    //pos = R*0.5f + pos*0.1f;
    //int2 p = to_int2(pos);
    
    float4 data = texel(ch0, pos);
    particle P = getParticle(data, pos);
  
    //border render
    float3 Nb = bN(P.X, iTime,R);
    float bord = smoothstep(2.0f*border_h,border_h*0.5f,border(pos, iTime,R));
    
    float4 rho = V(pos,R,ch1);
    float3 dx = to_float3(-2.0f, 0.0f, 2.0f);
    float4 grad = -0.5f*to_float4_f2f2(swi2(V(pos + swi2(dx,z,y),R,ch1),z,w) - swi2(V(pos + swi2(dx,x,y),R,ch1),z,w),
                                       swi2(V(pos + swi2(dx,y,z),R,ch1),z,w) - swi2(V(pos + swi2(dx,y,x),R,ch1),z,w));
    float2 N = _powf(length(swi2(grad,x,z)),0.2f)*normalize(swi2(grad,x,z)+1e-5);
    float specular = _powf(_fmaxf(dot(N, Dir(1.4f)), 0.0f), 3.5f);
    //float specularb = G(0.4f*(swi2(Nb,z,z) - border_h))*_powf(_fmaxf(dot(swi2(Nb,x,y), Dir(1.4f)), 0.0f), 3.0f);
    
    float a = _powf(smoothstep(fluid_rho*0.0f, fluid_rho*2.0f, rho.z),0.1f);
    float b = _expf(-1.7f*smoothstep(fluid_rho*1.0f, fluid_rho*7.5f, rho.z));
    float3 col0 = to_float3(1.000f,0.000f,0.000f);
    float3 col1 = to_float3(0.1f, 0.4f, 1.0f);
    float3 fcol = mixN(col0, col1, _tanhf(3.0f*(rho.w - 0.7f))*0.5f + 0.5f);
    // Output to screen
    float3 colxyz = to_float3_s(3.0f);
  
    colxyz = mixN(colxyz, swi3(fcol,x,y,z)*(1.5f*b + 0.0f * specular*5.0f), a);
    colxyz = mixN(colxyz, 0.0f*to_float3(0.5f,0.5f,1.0f), bord);
    colxyz = tanh_f3(colxyz);
    
    //col.ga = to_float2_s(0.0f);
    //col.rb = swi2(rho,x,z);
    //swi3(col,x,y,z) = to_float3_aw(specular);
    //return;
    
    float2 q = pos/iResolution;

    float3 e = to_float3_aw(to_float2_s(1.0f)/iResolution,0.0f);
    float f = 10.0f;
    //float p10 = texture(iChannel0, q-swi2(e,z,y)).z;
    //float p01 = texture(iChannel0, q-swi2(e,x,z)).z;
    //float p21 = texture(iChannel0, q+swi2(e,x,z)).z;
    //float p12 = texture(iChannel0, q+swi2(e,z,y)).z;
    //float p10 = V(q-swi2(e,z,y)).y;
    //float p01 = V(q-swi2(e,x,z)).y;
    //float p21 = V(q+swi2(e,x,z)).y;
    //float p12 = V(q+swi2(e,z,y)).y;
    
    //vec4 w = _tex2DVecN(iChannel0,q.x,q.y,15);
    //vec4 w = rho.x;
    float4 w = to_float4_s(1.0f);
    
    // Totally fake displacement and shading:
    //vec3 grad3 = normalize(to_float3(p21 - p01, p12 - p10, 0.5f));
    float3 grad3 = to_float3_aw(swi2(grad,x,z), 1.0f);
    //float2 uv = pos*4.0f/iResolution + swi2(grad3,x,y);
                              
    float2 uv = pos*4.0f/swi2(iChannelResolution[3],x,y) + swi2(grad3,x,y);
    uv = uv * 0.5f;
    float4 c = _tex2DVecN(iChannel3,uv.x,uv.y,15);
    c += c * 0.5f;
    c += c * w * (0.5f - distance_f2(q, to_float2_s(0.5f)));
    float3 lightDir = to_float3(0.2f, -0.5f, 0.7f);
    float3 light = normalize(lightDir);
    
    //float diffuse = dot(grad3, light);
    float diffuse = dot(grad3, light);
    float spec = _powf(_fmaxf(0.0f,-reflect(light,grad3).z),32.0f);
    float4 col2 = tanh_f4(_mix(c,to_float4(0.7f,0.8f,1.0f,1.0f),0.25f)*_fmaxf(diffuse,0.0f) + spec);
    //swi3(col,x,y,z) = a > 0.5f ? mixN(swi3(col,x,y,z), swi3(col,x,y,z) * swi3(col2,x,y,z), 1.5f) : swi3(col2,x,y,z);
    //col.xyz = a > 0.5 ? 2.0 * col.xyz * col2.xyz : col2.xyz;
    colxyz = a > 0.5f ? 2.0f * colxyz * swi3(col2,x,y,z) : swi3(col2,x,y,z);
    
    col = to_float4_aw(colxyz, 1.0f);

  SetFragmentShaderComputedColor(col);
}
