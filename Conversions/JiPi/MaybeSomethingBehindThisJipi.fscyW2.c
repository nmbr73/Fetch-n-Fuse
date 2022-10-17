
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: London' to iChannel0


#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define time iTime*0.05f



__DEVICE__ float formula(float2 z, inout float *thick, float _width) {
  float ot=1000.0f;
  for (int i=0; i<11; i++) {
    float dz=dot(z,z);
    z=abs_f2(z*2.0f)/dz-1.0f;
    ot=_fminf(ot,dz);
  }
  
  float h=0.014f/(_fmaxf(0.0f,_width-ot)/_width*0.9f+0.1f);
  *thick+=_fmaxf(0.0f,1.0f-h);
  return h;
}

__DEVICE__ float3 normal(float2 z, inout float *thick, float pix, float _width) {
  float2 d=to_float2(pix,0.0f);
  float3 n=normalize(cross( //get normal
                          to_float3(d.x*2.0f,0.0f,formula(z-swi2(d,x,y), thick,_width)-formula(z+swi2(d,x,y), thick,_width)),
                          to_float3(0.0f,d.x*2.0f,formula(z-swi2(d,y,x), thick,_width)-formula(z+swi2(d,y,x), thick,_width))));

  return n;
}


__KERNEL__ void MaybeSomethingBehindThisJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{
  CONNECT_CHECKBOX0(TexturMove, 1);
  
  CONNECT_COLOR0(Color1, 0.55f,0.55f,0.7f, 1.0f);
  CONNECT_COLOR1(Color2, 0.79f,0.79f,0.95f, 1.0f);
  //CONNECT_COLOR2(Color3, 0.0f, 0.0f, 0.0f, 1.0f);
  
  CONNECT_POINT0(LightPoint1XY, 0.0f, 0.0f );
  CONNECT_SLIDER0(LightPoint1Z, -10.0f, 10.0f, 0.0f);

  CONNECT_SLIDER1(zoom, -1.0f, 1.0f, 0.5f);
  CONNECT_SLIDER2(nDiv, -4.0f, 10.0f, 4.0f);
  CONNECT_SLIDER3(thickDiv, -20.0f, 30.0f, 16.0f);
  CONNECT_SLIDER4(refrMul, -1.0f, 1.0f, 0.08f);
  CONNECT_SLIDER5(_width, -1.0f, 3.0f, 0.2f);
//#define _width 0.2f

  float thick=0.0f;
  float pix;
  //float zoom = 0.5f;

  float2 pos = fragCoord / iResolution;
  float2 uv=pos-0.5f;
  
  float2 tuv = uv;
  uv.x*=iResolution.x/iResolution.y;
  
    
  zoom*=1.0f+_sinf(time*2.0f)*0.5f;
  float2 luv=uv;
  uv+=to_float2(_sinf(time),_cosf(time))*2.0f;
  uv+=sin_f2(uv*30.0f+time*200.0f)*0.0015f;
  uv*=zoom;
  luv*=zoom;
  pix=1.0f/iResolution.x*zoom;
  float2 d=to_float2(pix,0.0f);
  float3 n= normal(uv-swi2(d,x,y),&thick,pix,_width)+normal(uv+swi2(d,x,y),&thick,pix,_width);
         n+=normal(uv-swi2(d,y,x),&thick,pix,_width)+normal(uv+swi2(d,y,x),&thick,pix,_width);
         n/=nDiv;//4.0f;
  thick/=thickDiv;//16.0f;
  float2 refr=-1.0f*swi2(n,x,y)*refrMul;//0.08f;
  
  float3 tex;
  if (TexturMove)
    tex=swi3(texture(iChannel0,((tuv+refr)*3.0f+ to_float2(time,0.0f) )),x,y,z)+0.75f;
  else
    tex=swi3(texture(iChannel0, pos+refr ),x,y,z)+0.75f;
  
  //float3 colo=tex*_mix(to_float3(0.55f,0.55f,0.7f)*2.5f,to_float3(0.79f,0.79f,0.95f)*0.25f,_sqrtf(thick));  
  float3 colo=tex*_mix(swi3(Color1,x,y,z)*2.5f,swi3(Color2,x,y,z)*0.25f,_sqrtf(thick));  
  
  //float3 lightdir=normalize(to_float3(1.0f,0.5f,2.0f));
  float3 lightdir=normalize(to_float3(1.0f,0.5f,2.0f)+to_float3_aw(LightPoint1XY,LightPoint1Z));
  colo+=_fmaxf(0.0f,dot(-n,lightdir))*0.4f;
  colo+=_powf(_fmaxf(0.0f,dot(reflect(-n,to_float3(0.0f,0.0f,-1.0f)),lightdir)),50.0f)*0.6f;
  colo+=_powf(_fmaxf(0.0f,dot(reflect(to_float3(0.0f,0.0f,-1.0f),-n),lightdir)),50.0f)*0.2f;
  colo=pow_f3(colo,to_float3_s(1.5f))*2.0f;
  fragColor = to_float4_aw(colo,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}