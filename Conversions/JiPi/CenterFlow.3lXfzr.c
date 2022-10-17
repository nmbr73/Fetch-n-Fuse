
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution

//#define T(p) texelFetch(iChannel0, to_int2(mod_f(p,R)), 0)
#define T(p) texture(iChannel0, (make_float2(to_int2_cfloat(mod_f2f2(p,R)))+0.5f)/R)
#define P(p) texture(iChannel0, mod_f2f2(p,R)/R)

#define PI 3.14159265f
#define dt 1.5f


//mold stuff 
#define sense_ang 0.4f
#define sense_dis 2.5f
#define sense_force 0.1f
#define trailing 0.0f
#define acceleration 0.01f

//SPH pressure
#define Pressure(rho) 1.0f*rho
#define fluid_rho 0.2f

//useful functions
#define GS(x) _expf(-dot(x,x))
#define GS0(x) _expf(-length(x))
#define Dir(ang) to_float2(_cosf(ang), _sinf(ang))
#define loop(i,x) for(int i = 0; i < x; i++)
#define range(i,a,b) for(int i = a; i <= b; i++)

//data packing
#define PACK(X) ( uint(round(65534.0f*clamp(0.5f*X.x+0.5f, 0.0f, 1.0f))) + \
           65535u*uint(round(65534.0f*clamp(0.5f*X.y+0.5f, 0.0f, 1.0f))) )   
               
#define UNPACK(X) (clamp(to_float2(X%65535u, X/65535u)/65534.0f, 0.0f,1.0f)*2.0f - 1.0f)              

//#define DECODE(X) UNPACK(floatBitsToUint(X))
//#define ENCODE(X) uintBitsToFloat(PACK(X))




__DEVICE__ uint pack(float2 a)
{
  return ( uint(round(65534.0f*clamp(0.5f*a.x+0.5f, 0.0f, 1.0f))) + 
           65535u*uint(round(65534.0f*clamp(0.5f*a.y+0.5f, 0.0f, 1.0f))) );
}

__DEVICE__ float2 unpack(uint a)
{
  return (clamp(to_float2(a%65535u, a/65535u)/65534.0f, 0.0f,1.0f)*2.0f - 1.0f);
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

  //return unpack(X);
	return unpack(z._Uint);
}

__DEVICE__ float encode(float2 _x)
{
	Zahl z;
  uint X = pack(_x);

	z._Uint = X;
	return (z._Float);
	
  //return uintBitsToFloat(X);
} 


// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer B' to iChannel0


__KERNEL__ void CenterFlowFuse__Buffer_A(float4 U, float2 pos, float2 iResolution, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_CHECKBOX1(Start, 1);
    
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
       
      float2 X0 = decode(data.x) + tpos;
      float2 V0 = decode(data.y);
      float2 M0 = swi2(data,z,w);
       
      X0 += V0*dt; //integrate position

      //particle distribution size
      float K = 1.3f;
      
      float4 aabbX = to_float4_f2f2(_fmaxf(pos - 0.5f, X0 - K*0.5f), _fminf(pos + 0.5f, X0 + K*0.5f)); //overlap aabb
      float2 center = 0.5f*(swi2(aabbX,x,y) + swi2(aabbX,z,w)); //center of mass
      float2 size = _fmaxf(swi2(aabbX,z,w) - swi2(aabbX,x,y), to_float2_s(0.0f)); //only positive
      
            
      //the deposited mass into this cell
      float m = M0.x*size.x*size.y/(K*K); 
      
      //add weighted by mass
      //center.y +=01.25;
      
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
    
    //initial condition
    if(iFrame < 1 || Reset)
    {
        X = pos;
        V = to_float2_s(0.0f);
        M = 1e-6;
    }
    
    X = clamp(X - pos, to_float2_s(-0.5f), to_float2_s(0.5f));
    U = to_float4(encode(X), encode(V), M, 0.0f);

  SetFragmentShaderComputedColor(U);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void CenterFlowFuse__Buffer_B(float4 U, float2 pos, float2 iResolution, float4 iMouse)
{

    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1_Thr, 0.0f, 10.0f, 4.0f);
    CONNECT_SLIDER2(Blend2_Thr, 0.0f, 10.0f, 1.0f);


    pos+=0.5f;
    int2 p = to_int2_cfloat(pos);

    float4 data = T(pos); 
    float2 X = decode(data.x) + pos;
    float2 V = decode(data.y);
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

            float2 X0 = decode(data.x) + tpos;
            float2 V0 = decode(data.y);
            float M0 = data.z;
            float2 dx = X0 - X;
            
            float avgP = 0.5f*M0*(Pressure(M) + Pressure(M0)); 
            F -= 0.5f*GS(1.0f*dx)*avgP*dx;
            avgV += M0*GS(1.0f*dx)*to_float3_aw(V0,1.0f);
        }
        //swi2(avgV,x,y) /= avgV.z;
        avgV.x /= avgV.z;
        avgV.y /= avgV.z;

        //slime mold sensors
        float ang = _atan2f(V.y, V.x);
        float4 dir = sense_dis*to_float4_f2f2(Dir(ang+sense_ang), Dir(ang - sense_ang));
        float2 sd = to_float2(P(X + swi2(dir,x,y)).z, P(X + swi2(dir,z,w)).z);
        F += sense_force*(Dir(ang+PI*0.5f)*sd.x + Dir(ang-PI*0.5f)*sd.y); 

        //integrate velocity
        V += F*dt/M;
        
        //acceleration for fun effects
        V *= 1.0f + acceleration;
        
        //velocity limit
        float v = length(V);
        V /= (v > 1.0f)?1.0f*v:1.0f;
    }
    
    //mass decay
    M *= 0.99f;

    //input
    if(iMouse.z > 0.0f)
      M = _mix(M, 0.5f, GS((pos - swi2(iMouse,x,y))/13.0f));
    else
      M = _mix(M, 0.5f, GS((pos - R*0.5f)/13.0f));
    
    
    //Textureblending
    if (Blend1 > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(iChannel1,pos/R);

      if (tex.w > 0.0f)
      {
        M = _mix(M,tex.x*Blend1_Thr,Blend1);       
        //Q.w = 1.0;
        V = _mix(V,swi2(tex,x,y)*Blend2_Thr,Blend1);
      }
    }   
    
    
    
    //save
    X = clamp(X - pos, to_float2_s(-0.5f), to_float2_s(0.5f));
    U = to_float4(encode(X), encode(V), M, 0.0f);


  SetFragmentShaderComputedColor(U);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer B' to iChannel0


// Fork of "Everflow" by michael0884. https://shadertoy.com/view/ttBcWm
// 2020-07-19 18:18:22

// Fork of "Paint streams" by michael0884. https://shadertoy.com/view/WtfyDj
// 2020-07-11 22:38:47

__DEVICE__ float3 hsv2rgb( in float3 c )
{
    float3 rgb = clamp( abs_f3(mod_f3(c.x*6.0f+to_float3(0.0f,4.0f,2.0f),6.0f)-3.0f)-1.0f, 0.0f, 1.0f );

  rgb = rgb*rgb*(3.0f-2.0f*rgb); // cubic smoothing  

  return c.z * _mix( to_float3_s(1.0f), rgb, c.y);
}

__KERNEL__ void CenterFlowFuse(float4 col, float2 pos, float2 iResolution)
{
    CONNECT_COLOR0(Color1, 0.3f, 0.5f, 1.0f, 1.0f);
    CONNECT_COLOR1(Color2, 0.5f, 0.5f, 0.5f, 1.0f);
    
    pos+=0.5f;
    int2 p = to_int2_cfloat(pos);
   
    float4 data = T(pos); 
    float2 X = decode(data.x) + pos;
    float2 V = decode(data.y);
    float M = data.z;

    //how much mass falls into this pixel
    float4 rho = to_float4(V.x,V.y, M, 1.0f)*GS((pos - X)/0.5f); 
    float3 dx = to_float3(-3.0f, 0.0f, 3.0f);

    float ang = _atan2f(V.x, V.y);
    float mag = 0.0f + 3.0f*length(swi2(V,x,y))*rho.z;
    
    float a = _powf(smoothstep(fluid_rho*0.0f, fluid_rho*2.0f, rho.z),0.1f);
    // Output to screen
    
    //float3 _col = to_float3_s(0.2f*a); //Org
    float3 _col = 0.2f*swi3(Color2,x,y,z)*a; //
    
    
    //_col += 0.5f - 0.5f*cos_f3(2.0f*to_float3(0.3f,0.5f,1.0f)*_mix(rho.w,rho.z,0.0f));
    _col += 0.5f - 0.5f*cos_f3(2.0f*swi3(Color1,x,y,z)*_mix(rho.w,rho.z,0.0f));
    //swi3(col,x,y,z) += to_float3(1,1,1)*bord;
    
    float3 col2 = swi3(Color2,x,y,z)-0.5f;
    
    col = to_float4_aw( tanh_f3(4.0f*pow_f3(_col,to_float3_s(1.5f))) + hsv2rgb(to_float3(5.0f*ang/PI, 1.2f, mag)), 1.0f); //Org
    //col = to_float4_aw( tanh_f3(4.0f*pow_f3(_col,to_float3_s(1.5f))) + hsv2rgb(to_float3(5.0f*ang/PI+col2.x, 1.2f+col2.y, mag+col2.z)), 1.0f);
    //col.w=1.0f;

  SetFragmentShaderComputedColor(col);
}