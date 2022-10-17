
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Rusty Metal' to iChannel0
// Connect Image 'Cubemap: Forest_0' to iChannel2
// Connect Image 'Texture: RGBA Noise Medium' to iChannel1

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

//http://dept-info.labri.fr/~schlick/DOC/gem2.ps.gz
__DEVICE__ float bias(float x, float b) {
  return  x/((1.0f/b-2.0f)*(1.0f-x)+1.0f);
}

__DEVICE__ float gain(float x, float g) {
  float t = (1.0f/g-2.0f)*(1.0f-(2.0f*x));  
  return x<0.5f ? (x/(t+1.0f)) : (t-x)/(t-1.0f);
}

__DEVICE__ float3 degamma(float3 c)
{
  return pow_f3(c,to_float3_s(2.2f));
}
__DEVICE__ float3 gamma(float3 c)
{
	//zzzzzzzzzzz=0;
  return pow_f3(c,to_float3_s(1.0f/1.6f));
}

#define pi 3.1415927f


__DEVICE__ float dfdx(float value, float2 fragCoord, float2 iResolution)
{
   return ( value*fragCoord.x / iResolution.x );
}

__DEVICE__ float dfdy(float value, float2 fragCoord, float2 iResolution)
{
   return ( value*fragCoord.y / iResolution.y );
}

__KERNEL__ void WipplesJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{

  float2 uv = fragCoord / iResolution;
  uv.y=1.0f-uv.y;
  uv.x *= iResolution.x / iResolution.y;

  float h = 0.0f;

#if 1
  float time = iTime;
#else
  //go forwards and backwards!  
  float time = mod_f(iTime,30.0f);
  if (time > 15.0f) time = 30.0f-time;
#endif
  

#define DIVS  8
  
  for (int iy=0; iy<DIVS; iy++)
  {
    for (int ix=0; ix<DIVS*2; ix++)
    {
      //random variations for droplet
      float4 t = texture(iChannel1,(4.0f/256.0f)*to_float2((float)(ix),(float)(iy)));
      
      
      //stratify droplet positions
      float2 p = to_float2(ix,iy)*(1.0f/(float)(DIVS-1));
      p += (0.75f/(float)(DIVS-1))*(swi2(t,x,y)*2.0f-1.0f);
        
      //radius
      float2 v = uv-p;
      float d = dot(v,v);
      d = _powf(d,0.7f);
      float life = 10.0f;
      
      float n = time*5.0f*(t.w+0.2f) - t.z*6.0f;
      n *= 0.1f+ t.w;
      n = mod_f(n,life+t.z*3.0f+10.0f);        //repeat, plus a pause
      float x = d*99.0f;
      float T = x<(2.0f*pi*n) ? 1.0f : 0.0f;  //clip to 0 after end
      float e = _fmaxf(1.0f - (n/life),0.0f);    //entirely fade out by now
      float F = e*x/(2.0f*pi*n);        //leading edge stronger and decay
      
      float s = _sinf(x-(2.0f*pi*n)-pi*0.5f);
               
      s = s*0.5f+0.5f;    //bias needs [0,1]
      s = bias(s,0.6f);  //shape the ripple profile
      
      
      s = (F*s)/(x+1.1f)*T;      

      h+=s*100.0f*(0.5f+t.w);      

    }
  }

  
  //float3 n = to_float3(dFdx(h),17.0f,dFdy(h));    
  float3 n = to_float3(dfdx(h,fragCoord,iResolution),17.0f,dfdy(h,fragCoord,iResolution));    
  n = normalize(n);
  
  float3 E = normalize(to_float3(-uv.y*2.0f-1.0f,1.0f,uv.x*2.0f-1.0f));  //fake up an eye vector
  float3 rv = reflect(-E,n);
  //float3 reflect_color = degamma(_tex2DVecN(iChannel2,rv.x,rv.y,15).xyz);
  float3 reflect_color = degamma(swi3(decube_f3(iChannel2,rv),x,y,z));

  float3 fn = refract_f3(to_float3(0,1,0),n,2.5f);
  uv += swi2(fn,x,z)*0.1f;
  
  //float lod = length(swi2(fn,x,z))*10.0f;
  
  float3 c = to_float3_s(0.0f);
  c += degamma(swi3(texture(iChannel0,uv+to_float2(0.66f,0.0f)),x,y,z));
  c *= 1.0f-h*0.0125f;
  c += reflect_color*0.3f;
  
//  fragColor = to_float4_aw(h*0.5f+0.5f);
//  fragColor = to_float4(n*0.5f+0.5f,1.0f);
  float3 L = normalize(to_float3(1,1,1));
  float dl = _fmaxf(dot(n,L),0.0f)*0.7f+0.3f;
  c *= dl;
//  fragColor = to_float4_aw(to_float3(dl),1.0f);
  
  c = gamma(c);
  fragColor = to_float4_aw((c),1.0f);

  SetFragmentShaderComputedColor(fragColor);
}