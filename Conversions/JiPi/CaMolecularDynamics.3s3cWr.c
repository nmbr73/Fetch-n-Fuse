
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

//#define T(p) texelFetch(iChannel0, to_int2(mod_f(p,R)), 0)
#define T(p) texture(iChannel0, (make_float2(to_int2_cfloat(mod_f2f2(p,R)))+0.5f)/R)
#define P(p) texture(iChannel0, mod_f(p,R)/R)
#define C(p) texture(iChannel1, mod_f(p,R)/R)

#define PI 3.14159265f
#define dt 0.5f
#define R iResolution

//useful functions
#define GS(x) _expf(-dot(x,x))
#define GS0(x) _expf(-length(x))
#define CI(x) smoothstep(1.0f, 0.9f, length(x))
#define Dir(ang) to_float2(_cosf(ang), _sinf(ang))
#define Rot(ang) to_mat2(_cosf(ang), _sinf(ang), -_sinf(ang), _cosf(ang))
#define loop(i,x) for(int i = 0; i < x; i++)
#define range(i,a,b) for(int i = a; i <= b; i++)

//Wyatt thermostat
#define cooling 1.5f

//MD force
__DEVICE__ float MF(float2 dx)
{
    return -GS(0.75f*dx) + 0.13f*GS(0.4f*dx);
}


//the step functions need to be exactly like this!! step(x,0) does not work!
__DEVICE__ float Ha(float2 _x)
{
    return ((_x.x >= 0.0f)?1.0f:0.0f)*((_x.y >= 0.0f)?1.0f:0.0f);
}

__DEVICE__ float Hb(float2 _x)
{
    return ((_x.x > 0.0f)?1.0f:0.0f)*((_x.y > 0.0f)?1.0f:0.0f);
}

//particle grid

#ifdef Original
//data packing
#define PACK(X) ( uint(round(65534.0f*clamp(0.5f*X.x+0.5f, 0.0f, 1.0f))) + \
           65535u*uint(round(65534.0f*clamp(0.5f*X.y+0.5f, 0.0f, 1.0f))) )   
               
#define UNPACK(X) (clamp(to_float2(X%65535u, X/65535u)/65534.0f, 0.0f,1.0f)*2.0f - 1.0f)              

#define DECODE(X) UNPACK(floatBitsToUint(X))
#define ENCODE(X) uintBitsToFloat(PACK(X))
#endif

#define DECODE(X) decode(X)
#define ENCODE(X) encode(X)

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
// Connect Buffer A 'Image: Blendtextur' to iChannel1

//particle distribution
__DEVICE__ float3 PD(float2 x, float2 pos)
{
    return to_float3_aw(x, 1.0f)*Ha(x - (pos - 0.5f))*Hb((pos + 0.5f) - x);
}

__KERNEL__ void CaMolecularDynamicsFuse__Buffer_A(float4 U, float2 pos, float2 iResolution, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_SLIDER1(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER2(BlendV, -10.0f, 10.0f, 2.0f);
    CONNECT_SLIDER3(BlendM, 0.0f, 10.0f, 1.0f);
    CONNECT_SLIDER4(Masse, -10.0f, 10.0f, 0.0f);
    CONNECT_BUTTON0(Modus, 1, XY,  V, M, Clear, NN);
  
    CONNECT_SLIDER9(Debug, -10.0f, 10.0f, 0.0f);
  
    pos+=0.5f;

    int2 p = to_int2_cfloat(pos);
    
    float2 X = to_float2_s(0);
    float2 V = to_float2_s(0);
    float M = 0.0f;
    
    //basically integral over all updated neighbor distributions
    //that fall inside of this pixel
    //this makes the tracking conservative
    range(i, -1, 1) range(j, -1, 1)
    {
        float2 tpos = pos + to_float2(i,j);
        float4 data = T(tpos);
       
        float2 X0 = DECODE(data.x) + tpos;
        float2 V0 = DECODE(data.y);
        int M0 = (int)(data.z);
        int M0H = M0/2;
        
        X0 += V0*dt; //integrate position
        
        //the deposited mass into this cell
        float3 m = (M0 >= 2)?
            ((float)(M0H)*PD(X0+to_float2(0.5f, 0.0f), pos) + (float)(M0 - M0H)*PD(X0-to_float2(0.5f, 0.0f), pos))
            :((float)(M0)*PD(X0, pos)); 
        
        //add weighted by mass
        X += swi2(m,x,y);
        V += V0*m.z;
      
        //add mass
        M += m.z;
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
        M = Ha(pos - (R*0.5f - R.x*0.15f))*Hb((R*0.5f + R.x*0.15f) - pos);
    }
    
    
    //Textureblending
    if (Blend1 > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(iChannel1,pos/iResolution);

      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2)
          X = pos, V = to_float2_s(0.0f), M = Masse, Debug = 1.0f;

        if ((int)Modus&4)
          V = _mix(V, swi2(tex,x,y)*BlendV-1.0f, Blend1);

        if ((int)Modus&8)
        {  
          M = _mix(M, tex.x*BlendM-1.0f, Blend1);
        }
     
      }
    }
    
    if ((int)Modus&16) //Clear
      V = to_float2_s(0.0f), M = 0.50f;
    
    
    
    
    X = X - pos;
    U = to_float4(ENCODE(X), ENCODE(V), M, Debug);


  SetFragmentShaderComputedColor(U);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer C' to iChannel1


__DEVICE__ float sdBox( in float2 p, in float2 b )
{
    float2 d = abs_f2(p)-b;
    return length(_fmaxf(d,to_float2_s(0.0f))) + _fminf(max(d.x,d.y),0.0f);
}

__DEVICE__ float border(float2 p, float2 R)
{
    float bound = -sdBox(p - R*0.5f, R*to_float2(0.49f, 0.49f)); 
    float box = sdBox((p - R*to_float2(0.5f, 0.6f)) , R*to_float2(0.05f, 0.01f));
    float drain = -sdBox(p - R*to_float2(0.5f, 0.7f), R*to_float2(0.0f, 0.0f));
    return bound;
}

#define h 1.0f
__DEVICE__ float3 bN(float2 p, float2 R)
{
    float3 dx = to_float3(-h,0,h);
    float4 idx = to_float4(-1.0f/h, 0.0f, 1.0f/h, 0.25f);
    float3 r = swi3(idx,z,y,w)*border(p + swi2(dx,z,y),R)
             + swi3(idx,x,y,w)*border(p + swi2(dx,x,y),R)
             + swi3(idx,y,z,w)*border(p + swi2(dx,y,z),R)
             + swi3(idx,y,x,w)*border(p + swi2(dx,y,x),R);
    return to_float3_aw(normalize(swi2(r,x,y)), r.z + 1e-4);
}

__KERNEL__ void CaMolecularDynamicsFuse__Buffer_B(float4 U, float2 pos, float2 iResolution, float4 iMouse)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_SLIDER5(Blend2, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER6(Blend2V, -10.0f, 10.0f, 2.0f);
    CONNECT_SLIDER7(Blend2M, 0.0f, 10.0f, 1.0f);
    CONNECT_SLIDER8(Masse2, -10.0f, 10.0f, 0.0f);
    CONNECT_BUTTON1(Modus2, 1, XY,  V, M, Clear, NN);
  
    pos+=0.5f;
  
    float2 uv = pos/R;
    int2 p = to_int2_cfloat(pos);
        
    float4 data = T(pos); 
    float2 X = DECODE(data.x) + pos;
    float2 V = DECODE(data.y);
    float M = data.z;
    
    if(M != 0.0f) //not vacuum
    {
        //Compute the force
        float2 Fa = to_float2_s(0.0f);
        range(i, -2, 2) range(j, -2, 2)
        {
          float2 tpos = pos + to_float2(i,j);
          float4 data = T(tpos);

          float2 X0 = DECODE(data.x) + tpos;
          float2 V0 = DECODE(data.y);
          float M0 = data.z;
          float2 dx = X0 - X;
           
          Fa += M0*MF(dx)*dx;
        }
        
        float2 F = to_float2_s(0.0f);
        if(iMouse.z > 0.0f)
        {
          float2 dx= pos - swi2(iMouse,x,y);
          F -= 0.003f*dx*GS(dx/30.0f);
        }
        
         //gravity
        F += 0.001f*to_float2(0,-1);
        
        //integrate velocity
        V += (F + Fa)*dt/M;
        
        //Wyatt thermostat
        X += cooling*Fa*dt/M;
        
        float3 BORD = bN(X,R);
        V += 0.5f*smoothstep(0.0f, 5.0f, -BORD.z)*swi2(BORD,x,y);
        
        //velocity limit
        float v = length(V);
        V /= (v > 1.0f)?1.*v:1.;
    }
    
    
        //Textureblending
    if (Blend2 > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(iChannel2,pos/iResolution);

      if (tex.w > 0.0f)
      {      
        if ((int)Modus2&2)
          X = pos, V = to_float2_s(0.0f), M = Masse2;

        if ((int)Modus2&4)
          V = _mix(V, swi2(tex,x,y)*Blend2V-1.0f, Blend2);

        if ((int)Modus2&8)
        {  
          M = _mix(M, tex.x*Blend2M-1.0f, Blend2);
        }
     
      }
    }
    
    if ((int)Modus2&16) //Clear
      V = to_float2_s(0.0f), M = 0.0f;
    
    
    
    
    
    //save
    X = X - pos;
    U = to_float4(ENCODE(X), ENCODE(V), M, 0.0f);


  SetFragmentShaderComputedColor(U);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Preset: Keyboard' to iChannel3
// Connect Image 'Previsualization: Buffer B' to iChannel0


// Fork of "Landau Ginzburg fluid" by michael0884. https://shadertoy.com/view/WlXBDf
// 2020-09-21 21:03:05

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

//const int KEY_SPACE = 32;
//__DEVICE__ bool isKeyPressed(int KEY)
//{
//  return texelFetch( iChannel3, to_int2(KEY,2), 0 ).x > 0.5f;
//}


#define radius 1.0f
#define zoom 0.3f

__KERNEL__ void CaMolecularDynamicsFuse(float4 col, float2 pos, float2 iResolution, float4 iMouse, sampler2D iChannel3)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_CHECKBOX1(Invers, 0);
    CONNECT_SLIDER0(Alpha, 0.0f, 1.0f, 1.0f);
    
    pos+=0.5f;
    
    //zoom in
    if(Reset)
    {
      pos = swi2(iMouse,x,y) + pos*zoom - R*zoom*0.5f;
    }
    float rho = 0.001f;
    float2 vel = to_float2(0.0f, 0.0f);

    //compute the smoothed density and velocity
    range(i, -2, 2) range(j, -2, 2)
    {
        float2 tpos = _floor(pos) + to_float2(i,j);
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
   float3 vc = hsv2rgb(to_float3(6.0f*_atan2f(vel.x, vel.y)/(2.0f*PI), 1.0f, rho*length(swi2(vel,x,y))));
   col = to_float4_aw(cos_f3(0.9f*to_float3(3,2,1)*rho) + 0.0f*vc, Alpha);

   if (Invers) col = to_float4_aw(to_float3_s(1.0f)-(cos_f3(0.9f*to_float3(3,2,1)*rho) + 0.0f*vc), Alpha);

   if (col.x>=0.99f&&col.y>=0.99f&&col.z>=0.99f) col = to_float4_s(0.0f);



  SetFragmentShaderComputedColor(col);
}