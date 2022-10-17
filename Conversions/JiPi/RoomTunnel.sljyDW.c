
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define R    iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

//-------common
#define PI 3.14159265
///Gracias a Shane...16-jun-2020
__DEVICE__ float3 tex3D( __TEXTURE2D__ tex, in float3 p, in float3 n ){    
  n = _fmaxf(n*n - 0.2f, to_float3_s(0.001f)); // _fmaxf(_fabs(n), 0.001f), etc.
  n /= dot(n, to_float3_s(1)); 
  float3 tx = swi3(texture(tex, swi2(p,y,z)),x,y,z);
  float3 ty = swi3(texture(tex, swi2(p,z,x)),x,y,z);
  float3 tz = swi3(texture(tex, swi2(p,x,y)),x,y,z);
  return mul_mat3_f3(to_mat3_f3(tx*tx, ty*ty, tz*tz),n); 
}


// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Rusty Metal' to iChannel0


//----------image
//por jorge2017a2-
#define MAX_STEPS 100
#define MAX_DIST 110.0f
#define MIN_DIST 0.001f
#define EPSILON 0.001f
#define Ka 0.5f
#define Kd 0.4f
//room tunnel---11-abril-2022
//gracias a la comunidad de Shadertoy por su Informacion 
//y a IQ por compartir su conocimieto :)
__DEVICE__ float3 GetColorYMaterial(float3 p,  float3 n, float3 ro,  float3 rd, int id_color, float id_material);
__DEVICE__ float3 getMaterial( float3 pp, float id_material, __TEXTURE2D__ iChannel0);

//__DEVICE__ float3 light_pos1;  float3 light_color1 ;
//__DEVICE__ float3 light_pos2;  float3 light_color2 ;

//operacion de Union  por FabriceNeyret2
#define opU2(d1, d2) ( d1.x < d2.x ? d1 : d2 )

__DEVICE__ float sdBox( float3 p, float3 b )
  { float3 d = abs_f3(p) - b;   return length(_fmaxf(d,to_float3_s(0.0f)))+ _fminf(max(d.x,_fmaxf(d.y,d.z)),0.0f); }
__DEVICE__ float sdCylinderXY( float3 p, float2 h )
  { float2 d = abs_f2(to_float2(length(swi2(p,x,y)),p.z)) - h; return _fminf(max(d.x,d.y),0.0f) + length(_fmaxf(d,to_float2_s(0.0f))); }

__DEVICE__ float Intersect(float distA, float distB) { return _fmaxf(distA, distB);}
__DEVICE__ float Union(float distA, float distB)  { return _fminf(distA, distB);}
__DEVICE__ float Difference(float distA, float distB) { return _fmaxf(distA, -distB);}

// object transformation
__DEVICE__ float3 rotate_x(float3 p, float phi)
{   float uuuuuuuuuuuuuuu;
    float c = _cosf(phi);  float s = _sinf(phi);
    return to_float3(p.x, c*p.y - s*p.z, s*p.y + c*p.z);
}
__DEVICE__ float3 rotate_y(float3 p, float phi)
{  float c = _cosf(phi);  float s = _sinf(phi);
  return to_float3(c*p.x + s*p.z, p.y, c*p.z - s*p.x);
}
__DEVICE__ float3 rotate_z(float3 p, float phi)
{  float c = _cosf(phi);  float s = _sinf(phi);
  return to_float3(c*p.x - s*p.y, s*p.x + c*p.y, p.z);
}


__DEVICE__ float opRep1D( float p, float c )
  { float q = mod_f(p+0.5f*c,c)-0.5f*c; return  q ;}

__DEVICE__ float sdTriPrism( float3 p, float2 h )
{ float3 q = abs_f3(p);
  return _fmaxf(q.z-h.y,_fmaxf(q.x*0.866025f+p.y*0.5f,-p.y)-h.x*0.5f);
}

__DEVICE__ float2 GetDist(float3 p  ) 
{   float2 res= to_float2(9999.0f, -1.0f); 
    float3 p0=p;

    p.y=p.y-11.0f;
    float3 prep1=p;
    float3 prep2=p;
    float3 prep3=p;
    prep1.z= opRep1D(prep1.z,24.0f );
    prep2.z= opRep1D(prep2.z,55.0f );
    prep3.z= opRep1D(prep3.z,30.0f );
        
    float3 pos;
    float d0= sdBox( prep2, to_float3(35.0f,25.0f,9.0f) ); //bloque 1
    //---plataforma
    float d0a= sdBox( prep1-to_float3(0.0f,-12.5f,-8.0f), to_float3(30.0f,2.5f,20.0f) ); //bloque 1
    float d0b= sdBox( prep1-to_float3(0.0f,-11.5f,-8.0f), to_float3(10.0f,2.0f,21.0f) ); //bloque 2
    d0a=Difference(d0a, d0b);
    
    float d1a= sdCylinderXY(prep2, to_float2(10.0f,10.0f) );  //tubo ext
    float d1b= sdCylinderXY(prep2, to_float2(9.0f,11.0f) );  //tubo int
    float dif1=Difference(d1a, d1b);
    d0=Difference(d0, d1a);
    pos=to_float3(0.0f,0.0f,10.0f);
    float d2a= sdCylinderXY(prep2-pos, to_float2(10.0f,1.0f) ); //tubo ch,ext
    float d2b= sdCylinderXY(prep2-pos, to_float2(8.0f,1.5f) ); // tubo ch,int
    float dif2=Difference(d2a, d2b);
    float d3=sdTriPrism(prep3-to_float3(0.0f,-12.0f,5.0f), to_float2(5.0f,20.0f) );
    
    res =opU2(res, to_float2(dif1,100.0f));
    res =opU2(res, to_float2(dif2,100.0f));
    res =opU2(res, to_float2(d0,100.0f));
    res =opU2(res, to_float2(d0a,100.0f));
    res =opU2(res, to_float2(d3,101.0f));
    return res;
}

__DEVICE__ float3 GetNormal(float3 p)
{   float d = GetDist(p).x;
    float2 e = to_float2(0.001f, 0);
    float3 n = d - to_float3(GetDist(p-swi3(e,x,y,y)).x,GetDist(p-swi3(e,y,x,y)).x,GetDist(p-swi3(e,y,y,x)).x);
    return normalize(n);
}

__DEVICE__ float2 RayMarch(float3 ro, float3 rd, int PMaxSteps)
{   float3 p;
    float2 hit, object=to_float2(0.1f,0.0f);
    for(int i=0; i <= PMaxSteps; i++) 
    { p = ro + rd*object.x;
      hit = GetDist(p);
      object.x += hit.x;
      object.y = hit.y;
      if (_fabs(hit.x) < EPSILON || object.x > MAX_DIST) break;
    }    
    return object;
}


__DEVICE__ float getSoftShadow(float3 p, float3 lightPos) {
    float res = 9999.0f;
    float dist = 0.01f;
    float lightSize = 0.03f;
    for (int i = 0; i < MAX_STEPS; i++) {
      float hit = GetDist(p + lightPos * dist).x;
      res = _fminf(res, hit / (dist * lightSize));
      dist += hit;
      if (hit < 0.0001f || dist > 60.0f) break;
    }
    return clamp(res, 0.0f, 1.0f);
}

__DEVICE__ float occlusion(float3 pos, float3 nor)
{   float sca = 2.0f, occ = 0.0f;
    for(int i = 0; i < 10; i++) {    
      float hr = 0.01f + (float)(i) * 0.5f / 4.0f;
      float dd = GetDist(nor * hr + pos).x;
      occ += (hr - dd)*sca;
      sca *= 0.6f;
    }
    return clamp( 1.0f - occ, 0.0f, 1.0f );    
}

__DEVICE__ float3 lightingv3(float3 normal,float3 p, float3 lp, float3 rd, float3 ro,float3 lightColor, float t) 
{   float3 lightPos=lp;
    float3 worldPos = p;
    float3 V = -rd;
    float3 N = normal;
    float3 L = normalize (lightPos - worldPos);
    float3 R = reflect (-L, N);

    float lightDist = _fmaxf(length(L), 0.001f);
    float atten=1.0f / (1.0f + lightDist * 0.125f + lightDist * lightDist * 0.05f);
    L /= (lightDist*atten);

    float shadow = getSoftShadow(worldPos, L);// shadows
        
    float occ = occlusion(worldPos, N);// occ
    float3 ambient = Ka + Ka * dot(normal, to_float3(0.0f, 1.0f, 0.0f))*lightColor;
    ambient*=0.5f;

    float3 fresnel =  lightColor *  _powf(clamp(1.0f + dot(rd, N), 0.0f, 1.0f), 2.0f);;
    float diff= clamp(dot(N, L), 0.0f, 1.0f);
    float3 diffuse =  lightColor * diff;
    float shininess=10.0f;
    float specular    = _powf(_fmaxf(dot(R, V), 0.0f), shininess);

    float3 back = 0.5f * lightColor * clamp(dot(N, -L), 0.0f, 1.0f); // back
    float3 colOut = occ*lightColor*(ambient+diffuse*shadow+0.25f +back) + to_float3(0.7f,0.9f,1)*specular*specular;
    return colOut;
}

__DEVICE__ float3 getColorTextura( float3 p, float3 nor,  int i, __TEXTURE2D__ iChannel0)
{  
  if (i==100 )
    { float3 col=tex3D(iChannel0, p/32.0f, nor); return col*2.0f; }
  if (i==101 ) 
    { return tex3D(iChannel0, p/32.0f, nor)+0.2f; }
  return to_float3_s(0.0f); //Error?
}

__DEVICE__ float3 render_sky_color(float3 rd)
{   float t = (rd.x + 1.0f) / 2.0f;
    float3 col= to_float3((1.0f - t) + t * 0.3f, (1.0f - t) + t * 0.5f, (1.0f - t) + t);
    float3  sky = _mix(to_float3(0.0f, 0.1f, 0.4f)*col, to_float3(0.3f, 0.6f, 0.8f), 1.0f - rd.y);
    return sky;
}


__DEVICE__ float3 GetMaterial(float3 p,  float3 nor, float3 ro,  float3 rd, int id_color, __TEXTURE2D__ iChannel0)
{   
    float3 colobj; 
    if (id_color>=100 ){ return  getColorTextura( p, nor,id_color,iChannel0); }
    return to_float3_s(0.0f); //Error?
}

__DEVICE__ float3 linear2srgb(float3 c) 
{ 
  float zzzzzzzzzzzzzzz;
  return mix_f3(12.92f * c,1.055f * pow_f3(c, to_float3_s(1.0f/1.8f)) - 0.055f, step(to_float3_s(0.0031308f), c)); 
}

__DEVICE__ float3 exposureToneMapping(float exposure, float3 hdrColor) 
{ return to_float3_s(1.0f) - exp_f3(-hdrColor * exposure); }

__DEVICE__ float3 ACESFilm(float3 x)
{   float a,b,c,d,e;
    a = 2.51f; b = 0.03f; c = 2.43f;d = 0.59f; e = 0.14f;
    return (x*(a*x+b))/(x*(c*x+d)+e);
}

__DEVICE__ float3 Render(float3 ro, float3 rd, __TEXTURE2D__ iChannel0,float3 light_pos1,float3 light_pos2,float3 light_color1,float3 light_color2)
{  float3 col = to_float3_s(0);
   float3 p;
   float2 hit=RayMarch(ro,rd, MAX_STEPS);
   if(hit.x<MAX_DIST)
   {   p = (ro + rd * hit.x );
       float3 nor=GetNormal(p);
       float3 colobj;
       colobj=GetMaterial( p, nor, ro, rd,  (int)(hit.y),iChannel0);
       float3 result;
       result= lightingv3(nor, p,light_pos1, rd,ro,colobj,hit.x)*light_color1;
       result+= lightingv3(nor, p,light_pos2,rd, ro,colobj,hit.x)*light_color2;
       col= result/2.0f;
       col= (ACESFilm(col)+linear2srgb(col)+col+ exposureToneMapping(3.0f, col))/4.0f ;
       //if(5.0f*_sinf(1.0f-iTime)>0.0f)
       //{ float sum=(col.x+col.y+col.z)/3.0f; col=to_float3_aw(sum);} //grises
       //if(hit.y!=101.0f)
//       { float sum=col.x*0.299f+col.y*0.587f+col.z*0.11f; col=to_float3_s(sum);} //grises
       //Y'=0.299R'+0.587G'+0.114B'
   }
   //else if(hit.x>MAX_DIST)
   //col= render_sky_color(rd);
   return col;
}

__KERNEL__ void RoomTunnelFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{
  
   float2 uv = (fragCoord-0.5f*iResolution)/iResolution.y;
   float t,t0;t0=iTime;
   t=mod_f(t0*10.0f,1000.0f);
   float3 light_pos1 = to_float3(-40.0f, 80.0f, -20.0f); float3 light_color1=to_float3( 1.0f,1.0f,1.0f );
   float3 light_pos2 = to_float3( 40.0f, 80.0f, -25.0f ); float3 light_color2 =to_float3( 1.0f,1.0f,1.0f );
   float3 ro=to_float3(0.0f+4.0f*_sinf(t0),10.0f,t);
   float3 rd=normalize( to_float3(uv.x,uv.y,1.0f));
   float ang=0.58f*_cosf(180.0f-t0*0.5f);
   rd= rotate_y(rd, ang);
   light_pos1+=ro;
   light_pos2+=ro;
   float3 col= Render( ro,  rd, iChannel0,light_pos1,light_pos2,light_color1,light_color2);
   fragColor = to_float4_aw(col,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}