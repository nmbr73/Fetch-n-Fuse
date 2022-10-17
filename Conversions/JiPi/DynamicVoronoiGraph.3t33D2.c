
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


//simulation variables
#define dt 0.7f
#define radius 17.0f

//definitions
#define size iResolution
//#define texel(a, p)  texelFetch(a, to_int2(p), 0)
#define texel(a, p)  texture(a, (make_float2(to_int2_cfloat(p))+0.5f)/size)
#define ch0 iChannel0
#define ch1 iChannel1
#define ch2 iChannel2
#define ch3 iChannel3
#define PI 3.14159265f

//hash functions
//https://www.shadertoy.com/view/4djSRW
__DEVICE__ float hash11(float p)
{
    p = fract(p * 0.1031f);
    p *= p + 33.33f;
    p *= p + p;
    return fract(p);
}

__DEVICE__ float hash12(float2 p)
{
    float3 p3  = fract_f3(swi3(p,x,y,x) * 0.1031f);
    p3 += dot(p3, swi3(p3,y,z,x) + 33.33f);
    return fract((p3.x + p3.y) * p3.z);
}


__DEVICE__ float2 hash21(float p)
{
    float3 p3 = fract_f3(to_float3_s(p) * to_float3(0.1031f, 0.1030f, 0.0973f));
    p3 += dot(p3, swi3(p3,y,z,x) + 33.33f);
    return fract_f2((swi2(p3,x,x)+swi2(p3,y,z))*swi2(p3,z,y));

}

__DEVICE__ float2 hash22(float2 p)
{
    float3 p3 = fract_f3(swi3(p,x,y,x) * to_float3(0.1031f, 0.1030f, 0.0973f));
    p3 += dot(p3, swi3(p3,y,z,x)+33.33f);
    return fract_f2((swi2(p3,x,x)+swi2(p3,y,z))*swi2(p3,z,y));

}


//functions
__DEVICE__ float gauss(float2 _x, float r)
{
    return _expf(-_powf(length(_x)/r,2.0f));
}
   
__DEVICE__ float sdLine( in float2 p, in float2 a, in float2 b )
{
    float2 pa = p-a, ba = b-a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0f, 1.0f );
    return length( pa - ba*h );
}

//a rainbow colormap from Matlab
__DEVICE__ float interpolate(float val, float y0, float x0, float y1, float x1) 
{
    return (val-x0)*(y1-y0)/(x1-x0) + y0;
}

__DEVICE__ float base(float val) 
{
  
    if ( val <= -0.75f )      return 0.0f;
    else if ( val <= -0.25f ) return interpolate( val, 0.0f, -0.75f, 1.0f, -0.25f );
    else if ( val <= 0.25f )  return 1.0f;
    else if ( val <= 0.75f )  return interpolate( val, 1.0f, 0.25f, 0.0f, 0.75f );
    else                      return 0.0f;
}

__DEVICE__ float3 jet_colormap(float v)
{
    return to_float3(base(v - 0.5f),base(v),base(v + 0.5f));
}

__DEVICE__ float3 jet_range(float v, float a, float b)
{
    return jet_colormap(2.0f*clamp((v-a)/(b-a),0.0f,1.0f) - 1.0f);
}

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0


//voronoi particle tracking 

//loop the vector
__DEVICE__ float2 loop_d(float2 pos, float2 size)
{
  return mod_f(pos + size*0.5f, size) - size*0.5f;
}

//loop the space
__DEVICE__ float2 loop(float2 pos, float2 size)
{
  return mod_f(pos, size);
}


__DEVICE__ void Check(inout float4 *U, float2 pos, float2 dx, float2 size, __TEXTURE2D__ ch0)
{
    float4 Unb = texel(ch0, loop(pos+dx,size));
    //check if the stored neighbouring particle is closer to this position 
    if(length(loop_d(swi2(Unb,x,y) - pos, size)) < length(loop_d(swi2(*U,x,y) - pos, size)))
    {
        *U = Unb; //copy the particle info
    }
}

__DEVICE__ void CheckRadius(inout float4 *U, float2 pos, float r, float2 size, __TEXTURE2D__ ch0)
{
    Check(U, pos, to_float2(-r,0),size,iChannel0);
    Check(U, pos, to_float2(r,0),size,iChannel0);
    Check(U, pos, to_float2(0,-r),size,iChannel0);
    Check(U, pos, to_float2(0,r),size,iChannel0);
}

__KERNEL__ void DynamicVoronoiGraphFuse__Buffer_A(float4 U, float2 pos, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_CHECKBOX1(Textur, 0);
    
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(BlendOff, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(BlendMul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, X,  Y, Par, Clear, Special);
    CONNECT_POINT0(Par, 0.0f, 0.0f);
    
    
    pos+=0.5f;

    //this pixel value
    U = texel(ch0, pos);
    
    //check neighbours 
    CheckRadius(&U, pos, 1.0f,size,iChannel0);
    CheckRadius(&U, pos, 2.0f,size,iChannel0);
    CheckRadius(&U, pos, 3.0f,size,iChannel0);
    CheckRadius(&U, pos, 4.0f,size,iChannel0);
   
    swi2S(U,x,y, loop(swi2(U,x,y),size));
    
    float2 particle_pos = swi2(U,x,y);
    
    if(iMouse.z > 0.0f) 
    {
        float2 dm = (swi2(iMouse,x,y) - swi2(U,x,y));
        swi2S(U,z,w, swi2(U,z,w) + dt*normalize(dm)*_powf(length(dm)+10.0f, -1.0f));
        //swi2(U,z,w) *= 0.998f;
        U.z *= 0.998f;
        U.w *= 0.998f;
    }
    
    //update the particle
    //swi2(U,x,y) += dt*swi2(U,z,w);
    U.x += dt*U.z;
    U.y += dt*U.w;
    
    swi2S(U,x,y, loop(swi2(U,x,y),size));
    
    
    
    //Blending
    if (Blend1 > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(ch1, pos/iResolution);

      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2)
          swi2S(U,z,w, _mix( swi2(U,z,w), (swi2(tex,x,y)+BlendOff)*BlendMul, Blend1));


        if ((int)Modus&4)
        {
          particle_pos = to_float2(10.0f*_floor(pos.x/10.0f),10.0f*_floor(pos.y/10.0f));
          U = to_float4_f2f2(particle_pos, hash22(particle_pos) - 0.5f);
        }          


        if ((int)Modus&8)
        {  
          swi2S(U,z,w, _mix( swi2(U,z,w), Par, Blend1));
        }
#ifdef XXX
        if ((int)Modus&16) //Clear
          C = _mix(C,to_float3_s(0.0f),Blend1);

        if ((int)Modus&32) //Special
        {
          C = _mix(C,swi3(tex,x,y,z),Blend1);
        }
#endif

      }
    }
    
    
    
    
    if(iFrame < 1 || Reset)
    {
        particle_pos = to_float2(10.0f*_floor(pos.x/10.0f),10.0f*_floor(pos.y/10.0f));
        U = to_float4_f2f2(particle_pos, hash22(particle_pos) - 0.5f);
        
        if (Textur) 
        {
          float tex = texture(ch1, pos/iResolution).w;
          U = to_float4_s(0.0f);
          if (tex > 0.0f)
            U = to_float4_f2f2(particle_pos, hash22(particle_pos) - 0.5f);
          
        }
    }

  SetFragmentShaderComputedColor(U);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel1
// Connect Buffer B 'Previsualization: Buffer B' to iChannel0


//voronoi line tracking 

__DEVICE__ float4 loop(float4 line, float2 size)
{
  return to_float4_f2f2(mod_f2f2(swi2(line,x,y), size),mod_f2f2(swi2(line,z,w), size));
}


__DEVICE__ float is_border(float2 pos, float2 size, __TEXTURE2D__ ch1)
{
  
    float4 cp = texel(ch1, pos);
    float4 p0 = texel(ch1, loop(pos+to_float2(-1,0.0f),size));
    float4 p1 = texel(ch1, loop(pos+to_float2(1,0.0f),size));
    float4 p2 = texel(ch1, loop(pos+to_float2(0.0f,-1.0f),size));
    float4 p3 = texel(ch1, loop(pos+to_float2(0.0f,1.0f),size));
    //if(cp!=p0 || cp!=p1 || cp!=p2 || cp!=p3)
      if(   cp.x!=p0.x || cp.y!=p0.y || cp.z!=p0.z || cp.w!=p0.w
         || cp.x!=p1.x || cp.y!=p1.y || cp.z!=p1.z || cp.w!=p1.w
         || cp.x!=p2.x || cp.y!=p2.y || cp.z!=p2.z || cp.w!=p2.w
         || cp.x!=p3.x || cp.y!=p3.y || cp.z!=p3.z || cp.w!=p3.w)
    {
        return 1.0f;
    }
    return 0.0f;
}

__DEVICE__ float4 use_best(float2 pos, float4 U1, float4 U2)
{
    float d1 = sdLine(pos, swi2(U1,x,y), swi2(U1,z,w));
    float d2 = sdLine(pos, swi2(U2,x,y), swi2(U2,z,w));
    //check if the stored neighbouring line is closer to this position 
    if(d2 < d1)
    {
       return U2; //copy the line info
    }
    else
    {
       return U1;
    }
}


__DEVICE__ float d(float2 a, float2 b)
{
    return length(a-b);
}



__DEVICE__ float is_direct_neighbour(float2 p1, float2 p2)
{
   /* float2 cpoint = (p1+p2)/2.0f;
    float2 cvect = 2.0f*normalize(p2 - p1);
    //is the center point a boundary between at least 1 of them
    float4 pp1 = texel(ch1, cpoint + cvect);
    float4 pp2 = texel(ch1, cpoint - cvect);*/
    if( d(p1,p2) < radius)
    {
        return 1.0f;
    }
    
    return 0.0f;
}  

//the boudary line intersection is the source of the line info
__DEVICE__ void on_center(inout float4 *U, float2 pos, float2 size, __TEXTURE2D__ ch1)
{
    float4 p0 = texel(ch1, loop(pos+to_float2(-1,0.0f),size));
    float4 p1 = texel(ch1, loop(pos+to_float2(1,0.0f),size));
    float4 p2 = texel(ch1, loop(pos+to_float2(0.0f,-1.0f),size));
    float4 p3 = texel(ch1, loop(pos+to_float2(0.0f,1.0f),size));

    //if(p0 != p1 && is_direct_neighbour(swi2(p0,x,y),swi2(p1,x,y)) > 0.0f)
    if((p0.x != p1.x || p0.y != p1.y || p0.z != p1.z || p0.w != p1.w ) && is_direct_neighbour(swi2(p0,x,y),swi2(p1,x,y)) > 0.0f)
    {
       *U = use_best(pos, to_float4_f2f2(swi2(p0,x,y), swi2(p1,x,y)), *U);
    }
    //if(p2 != p3 && is_direct_neighbour(swi2(p2,x,y),swi2(p3,x,y)) > 0.0f)
    if((p2.x != p3.x || p2.y != p3.y || p2.z != p3.z || p2.w != p3.w ) && is_direct_neighbour(swi2(p2,x,y),swi2(p3,x,y)) > 0.0f) 
    {
       *U = use_best(pos, to_float4_f2f2(swi2(p2,x,y), swi2(p3,x,y)), *U);
    } 
}


__DEVICE__ void CheckB(inout float4 *U, float2 pos, float2 dx, float2 size, __TEXTURE2D__ ch0)
{
  
    float4 Unb = loop(texel(ch0, loop(pos+dx,size)),size);
    float d1 = sdLine(pos, swi2(*U,x,y), swi2(*U,z,w));
    float d2 = sdLine(pos, swi2(Unb,x,y), swi2(Unb,z,w));
    //check if the stored neighbouring line is closer to this position 
    if(d2 < d1)
    {
        *U = Unb; //copy the line info
    }
}

__DEVICE__ void CheckRadiusB(inout float4 *U, float2 pos, float r, float2 size, __TEXTURE2D__ ch0)
{
    CheckB(U, pos, to_float2(-r,0),size,iChannel0);
    CheckB(U, pos, to_float2(r,0),size,iChannel0);
    CheckB(U, pos, to_float2(0,-r),size,iChannel0);
    CheckB(U, pos, to_float2(0,r),size,iChannel0);
}

__KERNEL__ void DynamicVoronoiGraphFuse__Buffer_B(float4 U, float2 pos, float2 iResolution)
{
    pos+=0.5f;

    //this pixel value
    U = texel(ch0, pos);
    
    //check neighbours 
    CheckRadiusB(&U, pos, 1.0f,size,iChannel0);
    CheckRadiusB(&U, pos, 2.0f,size,iChannel0);
    CheckRadiusB(&U, pos, 3.0f,size,iChannel0);
    CheckRadiusB(&U, pos, 4.0f,size,iChannel0);
    CheckRadiusB(&U, pos, 5.0f,size,iChannel0);
    CheckRadiusB(&U, pos, 6.0f,size,iChannel0);
    CheckRadiusB(&U, pos, 7.0f,size,iChannel0);
    CheckRadiusB(&U, pos, 8.0f,size,iChannel0);
    CheckRadiusB(&U, pos, 9.0f,size,iChannel0);
    CheckRadiusB(&U, pos, 10.0f,size,iChannel0);
    
    //update the line from the particles
    swi2S(U,x,y, loop(swi2(texel(ch1, loop(swi2(U,x,y),size)),x,y),size));
    swi2S(U,z,w, loop(swi2(texel(ch1, loop(swi2(U,z,w),size)),x,y),size));
    
    //sort 
    if(length(swi2(U,x,y)) > length(swi2(U,z,w))) U = swi4(U,z,w,x,y);
    
    if(is_direct_neighbour(swi2(U,x,y), swi2(U,z,w)) < 1.0f)
    {
        U = to_float4_s(0.0f);
    }
    
    on_center(&U, pos, size, ch1);


  SetFragmentShaderComputedColor(U);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1


__KERNEL__ void DynamicVoronoiGraphFuse(float4 fragColor, float2 pos, float2 iResolution)
{
    pos+=0.5f;

    float4 particle = texel(ch0, pos);
    float distr = 4.0f*gauss(pos - swi2(particle,x,y), 0.7f);
    float4 b = texel(ch1, pos);
    float3 c1 = jet_range(length(swi2(texel(ch0, swi2(b,x,y)),z,w)), 0.0f,0.9f);
    float3 c2 = jet_range(length(swi2(texel(ch0, swi2(b,z,w)),z,w)), 0.0f,0.9f);
    float line = _expf(-_powf(sdLine(pos, swi2(b,x,y), swi2(b,z,w))/0.5f,2.0f));
    float linel = length(swi2(b,x,y) - swi2(b,z,w));
    float pl1 = length(pos - swi2(b,z,w));
    float3 color = _mix(c2,c1,pl1/(linel+0.01f)); 
    fragColor = to_float4_aw(1.3f*color*line + distr, 1.0f);


  SetFragmentShaderComputedColor(fragColor);
}