
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define Dt  0.5f
#define Dx  0.01f
#define H  1.0f

#define RADIUS    0.06f
#define INTENSITY  0.2f

#define DX  to_float2(1.0f/iResolution.x, 0.0f)
#define DY  to_float2(0.0f, 1.0f/iResolution.y)


__DEVICE__ float2 screen2world(in float2 fragCoord, float2 iResolution)
{
    return (2.0f*fragCoord - iResolution)/iResolution.y;
}

__DEVICE__ float2 world2screen(in float2 pos, float2 iResolution)
{
    return (pos*iResolution.y + iResolution) * 0.5f;
}

__DEVICE__ float2 screen2uv(in float2 fragCoord, float2 iResolution)
{
    return fragCoord / iResolution;
}

__DEVICE__ float2 uv2screen(in float2 uv, float2 iResolution)
{
    return uv * iResolution;
}

__DEVICE__ float2 world2uv(in float2 pos, float2 iResolution)
{
    return world2screen(pos,iResolution) / iResolution;
}

__DEVICE__ float2 uv2world(in float2 uv, float2 iResolution)
{
    return screen2world(uv2screen(uv,iResolution),iResolution);
}

__DEVICE__ float brushIntensity(float r)
{
    if(r/RADIUS <0.707f)
        return INTENSITY;
    return -INTENSITY;
}


__DEVICE__ float PointSegDistance2(in float2 p, in float2 p0, in float2 p1)
{
    float2 px0 = p-p0, p10 = p1-p0;
    float h = clamp(dot(px0, p10) / dot(p10, p10), 0.0f, 1.0f);
    return length(px0 - p10*h);
}

__KERNEL__ void ShallowwaterequationsJipi823Fuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX1(Reset, 0); 
  
    fragCoord+=0.5f;

    float2 uv = screen2uv(fragCoord,iResolution);
    
    float2 pos = screen2world(fragCoord,iResolution);
    
    
    // Retrieving mouse data
    bool mousePressed = iMouse.z > 0.0f;
    bool prevMousePressed = _tex2DVecN(iChannel0,uv.x,uv.y,15).y > 0.0f;
    
    float2 mousePos = screen2world(swi2(iMouse,x,y),iResolution);
    float2 prevMousePos = swi2(_tex2DVecN(iChannel0,uv.x,uv.y,15),z,w);
    

    
    float prev_eta = _tex2DVecN(iChannel0,uv.x,uv.y,15).x;
    
    //float dev_speed_x = 0.5f*(texture(iChannel1, uv + to_float2(1,0)/iResolution).x - texture(iChannel1, uv + to_float2(-1,0)/iResolution).x);
    //float dev_speed_y = 0.5f*(texture(iChannel1, uv + to_float2(0,1)/iResolution).y - texture(iChannel1, uv + to_float2(0,-1)/iResolution).y);
    
    
    float dev_speed_x = texture(iChannel1, uv+DX).x - texture(iChannel1, uv-DX).x;
    float dev_speed_y = texture(iChannel1, uv+DY).y - texture(iChannel1, uv-DY).y;
    
    
    float eta = 0.99f*prev_eta - Dt * (dev_speed_x+dev_speed_y);
    
    fragColor = to_float4_s(eta);
    
    
    if (iMouse.x < 10.0f) 
    {
        //float dist = length(to_float2(_cosf(float(iFrame) * 0.03f), 0.7f*_sinf(2.0f*float(iFrame) * 0.03f))*to_float2(1.2f,0.6f) - pos);
        float2 p1 = to_float2(_cosf((float)(iFrame) * 0.03f), 0.7f*_sinf(2.0f*(float)(iFrame) * 0.03f))*to_float2(1.2f,0.6f);
        float2 p0 = to_float2(_cosf((float)(iFrame-1) * 0.03f), 0.7f*_sinf(2.0f*(float)(iFrame-1) * 0.03f))*to_float2(1.2f,0.6f);
        float dist = PointSegDistance2(pos, p0, p1);
        if (dist < RADIUS) 
        {
            fragColor += to_float4_s(brushIntensity(dist));
            //fragColor -= to_float4(brushIntensity(length(p1-pos)));
        }
    } 
    else 
    {
       
        //fragColor = to_float4_s(0.0f);
        if(mousePressed && prevMousePressed)
        {
            //float dist = length(screen2world(swi2(iMouse,x,y))-pos);
            float dist = PointSegDistance2(pos, mousePos, prevMousePos);
            if(dist < RADIUS)
            {
              fragColor += to_float4_s(brushIntensity(dist));
              //fragColor -= to_float4(brushIntensity(length(mousePos-pos)));
            }
        }
    }
    
    // Back-uping mouse data
    fragColor.y = iMouse.z;
    //swi2(fragColor,z,w) = mousePos;
    fragColor.z = mousePos.x;
    fragColor.w = mousePos.y;
    
    if( iFrame<1 || Reset)
      fragColor = to_float4_s(0.0f);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1


//#define Dt  0.5f
//#define Dx  0.01f
#define B  0.02f
#define G  1.0f

#define iDX  to_int2(1, 0)
#define iDY  to_int2(0, 1)

/*
__DEVICE__ float2 screen2world(in float2 fragCoord)
{
    return (2.0f*fragCoord - iResolution)/iResolution.y;
}

__DEVICE__ float2 world2screen(in float2 pos)
{
    return (pos*iResolution.y + iResolution) * 0.5f;
}

__DEVICE__ float2 screen2uv(in float2 fragCoord)
{
    return fragCoord / iResolution;
}

__DEVICE__ float2 uv2screen(in float2 uv)
{
    return uv * iResolution;
}

__DEVICE__ float2 world2uv(in float2 pos)
{
    return world2screen(pos) / iResolution;
}

__DEVICE__ float2 uv2world(in float2 uv)
{
    return screen2world(uv2screen(uv));
}
*/


__KERNEL__ void ShallowwaterequationsJipi823Fuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX1(Reset, 0); 
  
    fragCoord+=0.5f;

    float2 uv = screen2uv(fragCoord,iResolution);
    

    float2 dev_height;
    //dev_height.x = texelFetch(iChannel0, to_int2(fragCoord)+iDx, 0).x - texelFetch(iChannel0, to_int2(fragCoord)-iDx, 0).x;
    dev_height.x = texture(iChannel0, (make_float2(to_int2_cfloat(fragCoord)+iDX)+0.5f)/iResolution).x 
                 - texture(iChannel0, (make_float2(to_int2_cfloat(fragCoord)-iDX)+0.5f)/iResolution).x;
    
    //dev_height.y = texelFetch(iChannel0, to_int2(fragCoord)+iDy, 0).x - texelFetch(iChannel0, to_int2(fragCoord)-iDy, 0).x;
    dev_height.y = texture(iChannel0, (make_float2(to_int2_cfloat(fragCoord)+iDY)+0.5f)/iResolution).x 
                 - texture(iChannel0, (make_float2(to_int2_cfloat(fragCoord)-iDY)+0.5f)/iResolution).x;
    
    
    //vec2 prev_speed = texelFetch(iChannel1, to_int2(fragCoord), 0).xy;
    float2 prev_speed = swi2(texture(iChannel1, (make_float2(to_int2_cfloat(fragCoord))+0.5f)/iResolution),x,y);
    
    
    float2 new_speed = prev_speed - Dt*(G*dev_height + B * prev_speed);
    
    
    //*
    // Reflexions
    if(fragCoord.x < 0.5f)
    {
        new_speed.x = _fabs(new_speed.x);
    }
    
    if(fragCoord.y < 0.5f)
    {
        new_speed.y = _fabs(new_speed.y);
    }
    if(fragCoord.x > iResolution.x-1.5f)
    {
        new_speed.x = -_fabs(new_speed.x);
    }
    
    if(fragCoord.y > iResolution.y-1.5f)
    {
        new_speed.y = -_fabs(new_speed.y);
    }
  //*/
    
    fragColor = to_float4_f2f2(new_speed, to_float2_s(0.0f));

    if( iFrame<1 || Reset)
      fragColor = to_float4_s(0.0f);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'Textur' to iChannel2



__KERNEL__ void ShallowwaterequationsJipi823Fuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
  
  CONNECT_CHECKBOX0(Textur, 0); 
  
  CONNECT_SLIDER0(Alpha, 0.0f, 1.0f, 1.0f);
  CONNECT_SLIDER1(Strength, 0.0f, 3.0f, 1.0f);
  
  fragCoord+=0.5f;

  float2 uv = screen2uv(fragCoord, iResolution);
  float height = _tex2DVecN(iChannel0,uv.x,uv.y,15).x;
  fragColor = 0.5f+to_float4_s(height)*0.5f*Strength;
  //fragColor = 0.5f + 0.5f*_tex2DVecN(iChannel1,uv.x,uv.y,15)*10.0f; //Bunte Variante
  
  if(Textur)
    fragColor = _tex2DVecN(iChannel2,uv.x,uv.y,15)+to_float4_s(height)*0.5f;
  
  //return;
    
    /*
    float t = dot(normalize(to_float3_aw(dFdx(height), dFdy(height), 1.0f)), normalize(to_float3_s(1.0f)));
    t = _fmaxf(0.0f, t);
    
    fragColor = to_float4(t);
    */
    
    fragColor.w=Alpha;

  SetFragmentShaderComputedColor(fragColor);
}