
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


// Xyptonjtroz by nimitz (twitter: @stormoid)
// https://www.shadertoy.com/view/4ts3z2
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License
// Contact the author for other licensing options

//Audio by Dave_Hoskins

#define ITR 100
//#define FAR 30.0f
#define time iTime

/*
  Believable animated volumetric dust storm in 7 samples,
  blending each layer in based on geometry distance allows to
  render it without visible seams. 3d Triangle noise is 
  used for the dust volume.

  Also included is procedural bump mapping and glow based on
  curvature*fresnel. (see: https://www.shadertoy.com/view/Xts3WM)


  Further explanation of the dust generation (per Dave's request):
    
  The basic idea is to have layers of gradient shaded volumetric
  animated noise. The problem is when geometry is intersected
  before the ray reaches the far plane. A way to smoothly blend
  the low sampled noise is needed.  So I am blending (smoothstep)
  each dust layer based on current ray distance and the solid 
  interesction distance. I am also scaling the noise taps  as a 
  function of the current distance so that the distant dust doesn't
  appear too noisy and as a function of current height to get some
  "ground hugging" effect.
  
*/

//__DEVICE__ mat2 mm2(in float a){float c = _cosf(a), s = _sinf(a);return to_mat2(c,s,-s,c);}

__DEVICE__ float height(in float2 p)
{
    p *= 0.2f;
    return _sinf(p.y)*0.4f + _sinf(p.x)*0.4f;
}

//smooth _fminf (http://iquilezles.org/www/articles/smin/smin.htm)
__DEVICE__ float smin( float a, float b)
{
  float h = clamp(0.5f + 0.5f*(b-a)/0.7f, 0.0f, 1.0f);
  return _mix(b, a, h) - 0.7f*h*(1.0f-h);
}


__DEVICE__ float2 nmzHash22(float2 q)
{
    uint2 p = make_uint2(to_int2_cfloat(q));
    //p = p*make_uint2(3266489917U, 668265263U) + swi2(p,y,x);
    p = p*make_uint2(3266489917U, 668265263U) + make_uint2(p.y,p.x);
    //p = p*(swi2(p,y,x)^(p >> 15U));
    p = make_uint2(p.x*(p.y^(p.x >> 15U)), p.y*(p.x^(p.y >> 15U)) );
   
    return make_float2(p.x^(p.x >> 16U),p.y^(p.y >> 16U) )*(1.0f/make_float2(0xffffffffU));
}

__DEVICE__ float vine(float3 p, in float c, in float h, float time)
{
    p.y += _sinf(p.z*0.2625f)*2.5f;
    p.x += _cosf(p.z*0.1575f)*3.0f;
    float2 q = to_float2(mod_f(p.x, c)-c/2.0f, p.y);
    return length(q) - h -_sinf(p.z*2.0f+_sinf(p.x*7.0f)*0.5f+time*0.5f)*0.13f;
}

__DEVICE__ float map(float3 p, float time)
{
    p.y += height(swi2(p,z,x));
    
    float3 bp = p;
    float2 hs = nmzHash22(_floor(swi2(p,z,x)/4.0f));
    swi2S(p,z,x, mod_f2(swi2(p,z,x),4.0f)-2.0f);
    
    float d = p.y+0.5f;
    p.y -= hs.x*0.4f-0.15f;
    //swi2(p,z,x) += hs*1.3f;
    p.z += hs.x*1.3f;
    p.x += hs.y*1.3f;
    
    d = smin(d, length(p)-hs.x*0.4f);
    
    d = smin(d, vine(bp+to_float3(1.8f,0.0f,0),15.0f,0.8f, time) );
    d = smin(d, vine(swi3(bp,z,y,x)+to_float3(0.0f,0,17.0f),20.0f,0.75f, time) );
    
    return d*1.1f;
}

__DEVICE__ float march(in float3 ro, in float3 rd, float time, float FAR)
{
    float precis = 0.002f;
    float h=precis*2.0f;
    float d = 0.0f;
    for( int i=0; i<ITR; i++ )
    {
      if( _fabs(h)<precis || d>FAR ) break;
      d += h;
      float res = map(ro+rd*d, time);
      h = res;
    }
  return d;
}

__DEVICE__ float tri(in float x){return _fabs(fract(x)-0.5f);}
__DEVICE__ float3 tri3(in float3 p){return to_float3( tri(p.z+tri(p.y*1.0f)), tri(p.z+tri(p.x*1.0f)), tri(p.y+tri(p.x*1.0f)));}
                                 


__DEVICE__ float triNoise3d(in float3 p, in float spd, float time)
{
  //mat2 m2 = to_mat2(0.970f,  0.242f, -0.242f,  0.970f);
  
  float z=1.4f;
  float rz = 0.0f;
  float3 bp = p;
  for (float i=0.0f; i<=3.0f; i+=1.0f )
  {
        float3 dg = tri3(bp*2.0f);
        p += (dg+time*spd);

        bp *= 1.8f;
        z *= 1.5f;
        p *= 1.2f;
        //swi2(p,x,z)*= m2;
        
        rz+= (tri(p.z+tri(p.x+tri(p.y))))/z;
        bp += 0.14f;
  }
  return rz;
}

__DEVICE__ float fogmap(in float3 p, in float d, float time)
{
    p.x += time*1.5f;
    p.z += _sinf(p.x*0.5f);
    return triNoise3d(p*2.2f/(d+20.0f),0.2f, time)*(1.0f-smoothstep(0.0f,0.7f,p.y));
}

__DEVICE__ float3 fog(in float3 col, in float3 ro, in float3 rd, in float mt, float time)
{
    float d = 0.5f;
    for(int i=0; i<7; i++)
    {
        float3  pos = ro + rd*d;
        float rz = fogmap(pos, d, time);
        float grd =  clamp((rz - fogmap(pos+0.8f-float(i)*0.1f,d, time))*3.0f, 0.1f, 1.0f );
        float3 col2 = (to_float3(0.1f,0.8f,0.5f)*0.5f + 0.5f*to_float3(0.5f, 0.8f, 1.0f)*(1.7f-grd))*0.55f;
        col = _mix(col,col2,clamp(rz*smoothstep(d-0.4f,d+2.0f+d*0.75f,mt),0.0f,1.0f) );
        d *= 1.5f+0.3f;
        if (d>mt)break;
    }
    return col;
}

__DEVICE__ float3 normal(in float3 p, float time)
{  

  float2 e = to_float2(-1.0f, 1.0f)*0.005f;   
  return normalize(swi3(e,y,x,x)*map(p + swi3(e,y,x,x), time) + swi3(e,x,x,y)*map(p + swi3(e,x,x,y), time) + 
                   swi3(e,x,y,x)*map(p + swi3(e,x,y,x), time) + swi3(e,y,y,y)*map(p + swi3(e,y,y,y), time) );   
}

__DEVICE__ float bnoise(in float3 p, float time)
{
    float n = _sinf(triNoise3d(p*0.3f,0.0f,time)*11.0f)*0.6f+0.4f;
    n += _sinf(triNoise3d(p*1.0f,0.05f, time)*40.0f)*0.1f+0.9f;
    return (n*n)*0.003f;
}

__DEVICE__ float3 bump(in float3 p, in float3 n, in float ds, float time)
{
    float2 e = to_float2(0.005f,0);
    float n0 = bnoise(p, time);
    float3 d = to_float3(bnoise(p+swi3(e,x,y,y), time)-n0, bnoise(p+swi3(e,y,x,y), time)-n0, bnoise(p+swi3(e,y,y,x), time)-n0)/e.x;
    n = normalize(n-d*2.5f/_sqrtf(ds));
    return n;
}

__DEVICE__ float shadow(in float3 ro, in float3 rd, in float mint, in float tmax, float time)
{
  float res = 1.0f;
    float t = mint;
    for( int i=0; i<10; i++ )
    {
    float h = map(ro + rd*t, time);
        res = _fminf( res, 4.0f*h/t );
        t += clamp( h, 0.05f, 0.5f );
        if(h<0.001f || t>tmax) break;
    }
    return clamp( res, 0.0f, 1.0f );

}

__DEVICE__ float curv(in float3 p, in float w, float time)
{
    float2 e = to_float2(-1.0f, 1.0f)*w;   
    
    float t1 = map(p + swi3(e,y,x,x), time), t2 = map(p + swi3(e,x,x,y),time);
    float t3 = map(p + swi3(e,x,y,x), time), t4 = map(p + swi3(e,y,y,y), time);
    
    return 0.125f/(e.x*e.x) *(t1 + t2 + t3 + t4 - 4.0f * map(p, time));
}

__KERNEL__ void XyptonjtrozJipi051Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse)
{

    CONNECT_CHECKBOX1(Invers, 0);
    //CONNECT_CHECKBOX2(ApplyColor, 0);

    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f); 
    CONNECT_SLIDER0(AlphaThres, 0.0f, 1.0f, 1.0f);
    CONNECT_SLIDER1(FAR, 0.0f, 100.0f, 30.0f);
    CONNECT_SLIDER2(Fog, 0.0f, 1.0f, 1.0f);

  
  float2 p = fragCoord/iResolution-0.5f;
  float2 q = fragCoord/iResolution;
  p.x*=iResolution.x/iResolution.y;
  float2 mo = swi2(iMouse,x,y) / iResolution-0.5f;
  mo = (mo.x==-0.5f&&mo.y==-0.5f)?mo=to_float2(-0.1f,0.07f):mo;
  mo.x *= iResolution.x/iResolution.y;

  float3 ro = to_float3(smoothstep(0.0f,1.0f,tri(time*0.45f)*2.0f)*0.1f, smoothstep(0.0f,1.0f,tri(time*0.9f)*2.0f)*0.07f, -time*0.6f);
  ro.y -= height(swi2(ro,z,x))+0.05f;
  mo.x += smoothstep(0.6f,1.0f,_sinf(time*0.6f)*0.5f+0.5f)-1.5f;
  float3 eyedir = normalize(to_float3(_cosf(mo.x),mo.y*2.0f-0.2f+_sinf(time*0.45f*1.57f)*0.1f,_sinf(mo.x)));
  float3 rightdir = normalize(to_float3(_cosf(mo.x+1.5708f),0.0f,_sinf(mo.x+1.5708f)));
  float3 updir = normalize(cross(rightdir,eyedir));
  float3 rd=normalize((p.x*rightdir+p.y*updir)*1.0f+eyedir);
  
  float3 ligt = normalize( to_float3(0.5f, 0.05f, -0.2f) );
  float3 ligt2 = normalize( to_float3(0.5f, -0.1f, -0.2f) );
    
  float rz = march(ro,rd, time, FAR);
  
  float3 fogb = _mix(to_float3(0.7f,0.8f,0.8f  )*0.3f, to_float3(1.0f,1.0f,0.77f)*0.95f, _powf(dot(rd,ligt2)+1.2f, 2.5f)*0.25f);
  fogb *= clamp(rd.y*0.5f+0.6f, 0.0f, 1.0f);
  float3 col = fogb;
    
  if ( rz < FAR )
  {
        float3 pos = ro+rz*rd;
        float3 nor= normal( pos, time );
        float d = distance_f3(pos,ro);
        nor = bump(pos,nor,d,time);
        float crv = clamp(curv(pos, 0.4f,time),0.0f,10.0f);
        float shd = shadow(pos,ligt,0.1f,3.0f,time);
        float dif = clamp( dot( nor, ligt ), 0.0f, 1.0f )*shd;
        float spe = _powf(clamp( dot( reflect(rd,nor), ligt ), 0.0f, 1.0f ),50.0f)*shd;
        float fre = _powf( clamp(1.0f+dot(nor,rd),0.0f,1.0f), 1.5f );
        float3 brdf = to_float3(0.10f,0.11f,0.13f);
        brdf += 1.5f*dif*to_float3(1.00f,0.90f,0.7f);
        col = _mix(to_float3(0.1f,0.2f,1),to_float3(0.3f,0.5f,1),pos.y*0.5f)*0.2f+0.1f;
        col *= (_sinf(bnoise(pos,time)*900.0f)*0.2f+0.8f);
        col = col*brdf + col*spe*0.5f + fre*to_float3(0.7f,1.0f,0.2f)*0.3f*crv;
    }
    
    //ordinary distance fog first
    col = _mix(col, fogb, smoothstep(FAR-7.0f,FAR,rz));
    
    //then volumetric fog
    col = _mix(col, fog(col, ro, rd, rz, time), Fog);
    
    //post
    col = pow_f3(col,to_float3_s(0.8f));
    col *= 1.0f-smoothstep(0.1f,2.0f,length(p));
    
    fragColor = to_float4_aw( col, 1.0f );
  

    fragColor = (fragColor + (Color-0.5f));
    if (fragColor.x <= AlphaThres)    fragColor.w = Color.w;  
    if (Invers) fragColor = to_float4_s(1.0f) - fragColor, fragColor.w=Color.w;

  SetFragmentShaderComputedColor(fragColor);
}
