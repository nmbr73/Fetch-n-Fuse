
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

//const float rho=1.0f;
//const float dx=1.0f;

__DEVICE__ float hash11(float p)
{
    float2 p2 = fract_f2(to_float2(p * 5.3983f, p * 5.4427f));
    p2 += dot(swi2(p2,y,x), swi2(p2,x,y) + to_float2(21.5351f, 14.3137f));
    return fract(p2.x * p2.y * 95.4337f);
}

__DEVICE__ float noise(float x)
{
    float p=_floor(x);
    float f=fract(x);
    f=f*f*(3.0f-2.0f*f);
    return _mix(hash11(p),hash11(p+1.0f),f);
}

__DEVICE__ mat3 m_rot(float angle)
{
    float c = _cosf(angle);
    float s = _sinf(angle);
    return to_mat3( c, s, 0, -s, c, 0, 0, 0, 1);
}
__DEVICE__ mat3 m_trans(float x, float y)
{
    return to_mat3(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0, -x, -y, 1.0f);
}
__DEVICE__ mat3 m_scale(float s)
{
    return to_mat3(s, 0, 0, 0, s, 0, 0, 0, 1);
}

__DEVICE__ float distToOcc(float2 pos, float2 resolution, float time)
{
    float ratio=resolution.x/resolution.y;
    pos.x*=ratio;
    pos=pos*2.0f-1.0f;
    pos*=0.6f;
    pos.x-=0.4f;
   
    pos*=6.0f;
    pos.y+=3.0f;
    float3 p = to_float3_aw(pos, 1.0f);
    float d = 1.0f;
    for(int i = 0; i < 7; ++i)
    {
        d=_fminf(d,(length(_fmaxf(abs_f2(swi2(p,x,y))-to_float2(0.01f,1.0f), to_float2_s(0.0f))))/p.z);
        p.x=_fabs(p.x);
        float pi=3.1415936f;
        p = mul_mat3_f3(mul_mat3_mat3(mul_mat3_mat3(m_scale(1.22f) , m_rot(0.25f*pi)) , m_trans(0.0f,3.0f*noise((float)(i)))) , p);
    }
    
    d=smoothstep(0.1f, 0.15f,d);
    return d;
}

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1
// Connect Buffer A 'Previsualization: Buffer D' to iChannel3


// Created by evilryu
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.

// Advection and body force

__KERNEL__ void AbasicfluidJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, float iTimeDelta, sampler2D iChannel1, sampler2D iChannel3)
{

    fragCoord+=0.5f;

    float2 pixelSize=1.0f/iResolution;
    float2 pos=fragCoord*pixelSize;
    
    // semi-lagrangian
    float2 U=swi2(_tex2DVecN(iChannel3,pos.x,pos.y,15),x,y);
    float2 prevPos=pos-iTimeDelta*U*pixelSize;
    
    // Interpolation is done by sampling
    float2 Unext = swi2(_tex2DVecN(iChannel3,prevPos.x,prevPos.y,15),x,y);
    
    // Body forces
   
    if(pos.x>=0.43f && pos.x<=0.5f && pos.y<=1.0f && pos.y>=0.8f)
    {
        Unext+=iTimeDelta*to_float2(0.0f,-1000.0f);
    }
    
    if(pos.x>=0.0f && pos.x<=0.1f && pos.y<=0.5f && pos.y>=0.45f)
    {
        Unext+=iTimeDelta*to_float2(1000.0f,0.0f);
    }
    
    if(pos.x>=0.0f && pos.x<=0.1f && pos.y<=0.5f && pos.y>=0.45f)
    {
        Unext+=iTimeDelta*to_float2(1000.0f,0.0f);
    }
    
     if(pos.x>=0.9f && pos.x<=1.0f && pos.y<=0.5f && pos.y>=0.45f)
    {
        Unext+=iTimeDelta*to_float2(-1000.0f,0.0f);
    }
    
    if(iMouse.z>0.0f||iMouse.w>0.0f)
    {
        float intensity=smoothstep(0.0f,0.05f,length((fragCoord-swi2(iMouse,x,y))/iResolution));
        Unext*=intensity;
    }
        
    // Vorticity confinement force
    float W=_tex2DVecN(iChannel1,pos.x,pos.y,15).y;
    float Wleft=texture(iChannel1, pos-to_float2(pixelSize.x,0)).y;
    float Wright=texture(iChannel1, pos+to_float2(pixelSize.x,0)).y;
    float Wdown=texture(iChannel1, pos-to_float2(0,pixelSize.y)).y;
    float Wup=texture(iChannel1, pos+to_float2(0,pixelSize.y)).y;
    
    float3 GradW=to_float3(Wright-Wleft, Wup-Wdown, 0.0f)*0.5f;
    float2 Fcon=swi2(cross(GradW/(length(GradW)+1e-10), to_float3(0,0,W)),x,y);
    
    Unext+=50.0f*iTimeDelta*Fcon;
    
    // Boundary conditions
    if(pos.x > 1.0f - pixelSize.x ||
       pos.y > 1.0f - pixelSize.y ||
       pos.x < pixelSize.x ||
       pos.y < pixelSize.y)
    {
       Unext = to_float2(0.0f, 0.0f);
    }
    
    // occluder
    if(distToOcc(pos, iResolution, iTime)<=0.0f)
    {
       Unext=to_float2_s(0);
    }
   
    // output Fcon is just for visualization
    fragColor=to_float4_f2f2(Unext,Fcon);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


// Created by evilryu
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.

// Divergence/Vortocity

__KERNEL__ void AbasicfluidJipiFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
  
    fragCoord+=0.5f;
    

    float2 pixelSize=1.0f/iResolution;
    float2 pos=fragCoord*pixelSize;
    
    float2 left=swi2(texture(iChannel0, pos-to_float2(pixelSize.x,0)),x,y);
    float2 right=swi2(texture(iChannel0, pos+to_float2(pixelSize.x,0)),x,y);
    float2 down=swi2(texture(iChannel0, pos-to_float2(0,pixelSize.y)),x,y);
    float2 up=swi2(texture(iChannel0, pos+to_float2(0,pixelSize.y)),x,y);
    
    // Central difference
    float D=((right.x-left.x) + (up.y-down.y))*0.5f;
    
    // Vortisity
    float W=((right.y-left.y) - (up.x-down.x))*0.5f;
    
    fragColor = to_float4(D,W,0,0);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1
// Connect Buffer C 'Previsualization: Buffer C' to iChannel2


// Created by evilryu
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.

// Jacobian

//float2 pixelSize;
//float P;

__DEVICE__ float getPressure(float2 pos, bool hori, float2 iResolution, float iTime, float2 pixelSize, float P, float rho, float dx, float iTimeDelta, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel2)
{
    if(distToOcc(pos, iResolution, iTime)<=0.0f)
    {
        if(hori)
        {
            return P+rho*dx/iTimeDelta*_tex2DVecN(iChannel0,pos.x,pos.y,15).x;
        }
        else
        {
            return P+rho*dx/iTimeDelta*_tex2DVecN(iChannel0,pos.x,pos.y,15).y;
        }
        
    }
    if(pos.x<pixelSize.x || pos.x>1.0f-pixelSize.x)
    {
        return P+rho*dx/iTimeDelta*_tex2DVecN(iChannel0,pos.x,pos.y,15).x;
    }
    else if(pos.y<pixelSize.y || pos.y>1.0f-pixelSize.y)
    {
        return P+rho*dx/iTimeDelta*_tex2DVecN(iChannel0,pos.x,pos.y,15).y;
    }
    else
    {
        return _tex2DVecN(iChannel2,pos.x,pos.y,15).x;
    }
}

__KERNEL__ void AbasicfluidJipiFuse__Buffer_C(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float iTimeDelta, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{

    fragCoord+=0.5f;
    
    const float rho=1.0f;
    const float dx=1.0f;
float CCCCCCCCCCCCCCCCC;    
    float2 pixelSize;
    float P;

    pixelSize=1.0f/iResolution;
    float2 pos=fragCoord*pixelSize;
    
    float D=_tex2DVecN(iChannel1,pos.x,pos.y,15).x;
    
    // Using previous pressure as guess
    float Pleft, Pright, Pup, Pdown;
    
    P=_tex2DVecN(iChannel2,pos.x,pos.y,15).x;
    
    float Usolid=0.0f;
              
    Pleft = getPressure(pos-to_float2(pixelSize.x,0),true,iResolution,iTime,pixelSize,P,rho,dx,iTimeDelta,iChannel0,iChannel2);
    Pright= getPressure(pos+to_float2(pixelSize.x,0),true,iResolution,iTime,pixelSize,P,rho,dx,iTimeDelta,iChannel0,iChannel2);
    Pup   = getPressure(pos+to_float2(0,pixelSize.y),false,iResolution,iTime,pixelSize,P,rho,dx,iTimeDelta,iChannel0,iChannel2);
    Pdown = getPressure(pos-to_float2(0,pixelSize.y),false,iResolution,iTime,pixelSize,P,rho,dx,iTimeDelta,iChannel0,iChannel2);
    
    P=(Pleft+Pright+Pup+Pdown-D*rho*dx/iTimeDelta)/4.0f;

    fragColor = to_float4(P,0,0,0);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel0
// Connect Buffer D 'Previsualization: Buffer C' to iChannel2


// Created by evilryu
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.

// Velocity field substract pressure gradient force


__KERNEL__ void AbasicfluidJipiFuse__Buffer_D(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float iTimeDelta, sampler2D iChannel0, sampler2D iChannel2)
{

    fragCoord+=0.5f;
    
    const float rho=1.0f;
    const float dx=1.0f;

    float2 pixelSize=1.0f/iResolution;
    float2 pos=fragCoord*pixelSize;
  
    float2 U=swi2(_tex2DVecN(iChannel0,pos.x,pos.y,15),x,y);
    
    float Pleft=texture(iChannel2, pos-to_float2(pixelSize.x,0)).x;
    float Pright=texture(iChannel2, pos+to_float2(pixelSize.x,0)).x;
    float Pup=texture(iChannel2, pos+to_float2(0,pixelSize.y)).x;
    float Pdown=texture(iChannel2, pos-to_float2(0,pixelSize.y)).x;
    

    float2 dP=to_float2((Pright-Pleft)/dx, (Pup-Pdown)/dx)*0.5f;
    U-=dP*iTimeDelta/rho;
    fragColor = to_float4(U.x,U.y,0,0);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'Previsualization: Buffer C' to iChannel2
// Connect Image 'Previsualization: Buffer D' to iChannel3


// Created by evilryu
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.

// from aiekick： https://www.shadertoy.com/view/lttXDn
__DEVICE__ float3 blackbody(float t)
{
    float Temp = t*7500.0f;
    float3 col = to_float3_s(255.0f);
    col.x = 56100000.0f * _powf(Temp,(-3.0f / 2.0f)) + 148.0f;
    col.y = 100.04f * _logf(Temp) - 623.6f;
    if (Temp > 6500.0f) col.y = 35200000.0f * _powf(Temp,(-3.0f / 2.0f)) + 184.0f;
    col.z = 194.18f * _logf(Temp) - 1448.6f;
    col = clamp(col, 0.0f, 255.0f)/255.0f;
    if (Temp < 1000.0f) col *= Temp/1000.0f;
    return col;
}

__KERNEL__ void AbasicfluidJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel3)
{

    fragCoord+=0.5f;

    float2 pos = fragCoord/iResolution;
    float3 col=swi3(_tex2DVecN(iChannel3,pos.x,pos.y,15),x,y,z);
    col=blackbody(smoothstep(-3.0f,250.0f,length(col)));
    float d=distToOcc(pos,iResolution, iTime);
    swi3S(col,x,y,z, swi3(col,x,y,z) + to_float3_s(0.6f)*(1.0f-smoothstep(0.0f,0.1f, d)));
    fragColor = to_float4_aw(col,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}