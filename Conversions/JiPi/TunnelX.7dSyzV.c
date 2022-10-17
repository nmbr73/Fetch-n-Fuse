
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


//----------common
struct TObj
{
    float id_color;
    float id_objeto;
    float id_material;
    float dist;
    float3 normal;
    float3 ro;
    float3 rd;
    float2 uv;
    float3 color;
    float3 p;
    float3 phit; //22-mar-2021
    float3 rf;
    float marchCount;
    bool blnShadow;
    bool hitbln;
};

    
//TObj mObj;
//float3 glpRoRd;
//float2 gres2;
//float itime;

#define PI 3.14159265358979323846264
#define MATERIAL_NO -1.0f
#define COLOR_NO -1.0f
#define COLORSKY to_float3(0.1f, 0.1f, 0.6f)

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

///Gracias a SHane...16-jun-2020
__DEVICE__ float3 tex3D( __TEXTURE2D__ tex, in float3 p, in float3 n ){    
    n = _fmaxf(n*n - 0.2f, to_float3_s(0.001f)); // _fmaxf(_fabs(n), 0.001f), etc.
    n /= dot(n, to_float3_s(1)); 
    float3 tx = swi3(texture(tex, swi2(p,y,z)/to_float2(1.77f,1.0f)),x,y,z);
    float3 ty = swi3(texture(tex, swi2(p,z,x)/to_float2(1.77f,1.0f)+to_float2(0.0f,0.5f)),x,y,z);
    float3 tz = swi3(texture(tex, swi2(p,x,y)),x,y,z);
    return mul_mat3_f3(to_mat3_f3(tx*tx, ty*ty, tz*tz),n); 
}


// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Rock Tiles' to iChannel0


//----------image
//por jorge2017a2-
///tunnel x
//https://www.shadertoy.com/view/WlsSWS
//   01/feb/2022
#define MAX_STEPS 100
#define MAX_DIST 100.0f
#define MIN_DIST 0.001f
#define EPSILON 0.001f
#define REFLECT 2

__DEVICE__ float3 GetColorYMaterial(float3 p,  float3 n, float3 ro,  float3 rd, int id_color, float id_material);
__DEVICE__ float3 getMaterial( float3 pp, float id_material);
//float3 light_pos1;  float3 light_color1 ;
//float3 light_pos2;  float3 light_color2 ;

//operacion de Union  por FabriceNeyret2
#define opU3(d1, d2) ( d1.x < d2.x ? d1 : d2 )
#define opU(d1, d2) ( d1.x < d2.x ? d1 : d2 )


__DEVICE__ float sdBox( float3 p, float3 b )
  { float3 d = abs_f3(p) - b;   return length(_fmaxf(d,to_float3_s(0.0f)))+ _fminf(max(d.x,_fmaxf(d.y,d.z)),0.0f); }

///----------Operacion de Distancia--------
__DEVICE__ float intersectSDF(float distA, float distB)
  { return _fmaxf(distA, distB);}
__DEVICE__ float unionSDF(float distA, float distB)
  { return _fminf(distA, distB);}
__DEVICE__ float differenceSDF(float distA, float distB)
  { return _fmaxf(distA, -distB);}
__DEVICE__ float opRep1D( float p, float c )
  { float q = mod_f(p+0.5f*c,c)-0.5f*c; return  q ;}

// object transformation
__DEVICE__ float3 rotate_x(float3 p, float phi)
{   float c = _cosf(phi);  float s = _sinf(phi);
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
__DEVICE__ float2 rotatev2(float2 p, float ang)
{   float c = _cosf(ang);
    float s = _sinf(ang);
    return to_float2(p.x*c - p.y*s, p.x*s + p.y*c);
}


//https://www.shadertoy.com/view/WlsSWS
__DEVICE__ float pathterrain(float x,float z)
{   // Common height function for path and terrain
    return  _sinf(x*0.5f )*1.0f+_cosf(z*0.3f )*0.3f +_cosf(x*3.0f+z )*0.1f+_sinf(x-z*0.2f )*0.2f;
}

__DEVICE__ float3 GetDist(float3 p  ) 
{   float3 res= to_float3(9999.0f, -1.0f,-1.0f);  float3 p0=p;
    float planeDist1 = p.y+0.0f;  //piso inf
   
    res =opU3(res, to_float3(planeDist1,-1.0f,7.0f));
   
    p.y=p.y-5.0f;
    p=rotate_y(p, radians(90.0f));
    
    float h1= pathterrain(p.x*0.5f,p.z);
    float h2= pathterrain(p.y*0.5f,p.z);
    float h3= pathterrain(p.x*0.5f,p.y);
    
    p.x=  opRep1D( p.x, 6.0f );
    float d2= sdBox( p-to_float3(0.0f,h1+h2,0.0f+h3), to_float3(6.0f,3.0f,6.0f) );
    res =opU3(res, to_float3(d2,100.0f,-1.0f));
   
    return res;
}

__DEVICE__ float3 GetNormal(float3 p)
{   float d = GetDist(p).x;
    float2 e = to_float2(0.001f, 0);
    float3 n = d - to_float3(GetDist(p-swi3(e,x,y,y)).x,GetDist(p-swi3(e,y,x,y)).x,GetDist(p-swi3(e,y,y,x)).x);
    return normalize(n);
}

__DEVICE__ float RayMarch(float3 ro, float3 rd, int PMaxSteps, TObj *mObj)
{   float t = 0.0f; 
    float3 dS=to_float3(9999.0f,-1.0f,-1.0f);
    float marchCount = 0.0f;
    float3 p;
    float minDist = 9999.0f; 
    
    for(int i=0; i <= PMaxSteps; i++) 
    {   p = ro + rd*t;
        dS = GetDist(p);
        t += dS.x;
        if ( _fabs(dS.x)<MIN_DIST  || i == PMaxSteps)
            { (*mObj).hitbln = true; minDist = _fabs(t); break;}
        if(t>MAX_DIST)
            { (*mObj).hitbln = false;    minDist = t;    break; } 
        marchCount++;
    }
    (*mObj).dist = minDist;
    (*mObj).id_color = dS.y;
    (*mObj).marchCount=marchCount;
    (*mObj).id_material=dS.z;
    (*mObj).normal=GetNormal(p);
    (*mObj).phit=p;
    return t;
}


__DEVICE__ float GetShadow(float3 p, float3 plig, TObj *mObj)
{   float3 lightPos = plig;
    float3 l = normalize(lightPos-p);
    float3 n = GetNormal(p);
    float dif = clamp(dot(n, l), 0.0f, 1.0f);
    float d = RayMarch(p+n*MIN_DIST*2.0f, l , MAX_STEPS/2, mObj);
    if(d<length(lightPos-p)) dif *= 0.1f;
    return dif;
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

__DEVICE__ float3 lightingv3(float3 normal,float3 p, float3 lp, float3 rd, float3 ro,float3 col, float t) 
{   float3 lightPos=lp;
    float3 hit = ro + rd * t;
    float3 norm = GetNormal(hit);
    float3 light = lightPos - hit;
    float lightDist = _fmaxf(length(light), 0.001f);
    float atten = 1.0f / (1.0f + lightDist * 0.125f + lightDist * lightDist * 0.05f);
    light /= lightDist;
    
    float occ = occlusion(hit, norm);
    float dif = clamp(dot(norm, light), 0.0f, 1.0f);
    dif = _powf(dif, 4.0f) * 2.0f;
    float spe = _powf(_fmaxf(dot(reflect(-light, norm), -rd), 0.0f), 8.0f);
    float3 color = col*(dif+0.35f +to_float3(0.35f,0.45f,0.5f)*spe) + to_float3(0.7f,0.9f,1)*spe*spe;
    color*=occ;
    return color;
}

__DEVICE__ float3 getColorTextura( float3 p, float3 nor,  int i, __TEXTURE2D__ iChannel0)
{  if (i==100 )
    { float3 col=tex3D(iChannel0, p/16.0f, nor); return col; }
}

__DEVICE__ float3 Getluz(float3 p, float3 ro, float3 rd, float3 nor , float3 colobj ,float3 plight_pos, float tdist)
{  float intensity=1.0f;
     float3 result;
    result = lightingv3( nor, p, plight_pos,  rd,ro, colobj, tdist);
    return result;
}


__DEVICE__ float3 GetColorYMaterial(float3 p,  float3 n, float3 ro,  float3 rd, int id_color, float id_material, __TEXTURE2D__ iChannel0)
{    float3 colobj; 
      
    if ( float( id_color)>=100.0f  && float( id_color)<=199.0f ) 
   {  float3 coltex=getColorTextura(p, n, int( id_color),iChannel0);
        colobj=coltex;
  }

    return colobj;
}


__DEVICE__ float3 linear2srgb(float3 c) 
{ return mix_f3(12.92f * c,1.055f * pow_f3(c, to_float3_s(1.0f/1.8f)) - 0.055f, step(to_float3_s(0.0031308f), c)); }

__DEVICE__ float3 exposureToneMapping(float exposure, float3 hdrColor) 
{ return to_float3_s(1.0f) - exp_f3(-hdrColor * exposure); }


__DEVICE__ float3 ACESFilm(float3 x)
{   float a,b,c,d,e;
    a = 2.51f; b = 0.03f; c = 2.43f; 
    d = 0.59f; e = 0.14f;

    return (x*(a*x+b))/(x*(c*x+d)+e);
}

__DEVICE__ float3 Render(float3 ro, float3 rd, float3 light_pos1, float3 light_pos2, float3 light_color1, float3 light_color2, __TEXTURE2D__ iChannel0, TObj *mObj)
{  float3 col = to_float3_s(0);
   TObj Obj;
   (*mObj).rd=rd;
   (*mObj).ro=ro;
   float3 p;

   float d=RayMarch(ro,rd, MAX_STEPS, mObj);
   
   Obj=*mObj;
   if((*mObj).hitbln) 
    {   p = (ro + rd * d );  
        float3 nor=(*mObj).normal;
        float3 colobj;
        colobj=GetColorYMaterial( p, nor, ro, rd,  int( Obj.id_color), Obj.id_material,iChannel0);

        float dif1=1.0f;
        float3 result;
        result=  Getluz( p,ro,rd, nor, colobj ,light_pos1,d)*light_color1;
        result+= Getluz( p,ro,rd, nor, colobj ,light_pos2,d)*light_color2;
   
        col= result;
        col= (ACESFilm(col)+linear2srgb(col)+col+ exposureToneMapping(3.0f, col))/4.0f ;
    }
   
   return col;
}


__KERNEL__ void TunnelXFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{
   float2 uv = (fragCoord-0.5f*iResolution)/iResolution.y;
   
   TObj mObj;
   
   mObj.uv=uv;
   float t;
   t=mod_f(iTime*5.0f,500.0f);
   //itime=t;
   //mObj.blnShadow=false;
   mObj.blnShadow=true;
        
   float3 light_pos1= to_float3(0.0f, 10.0f, -20.0f ); float3 light_color1=to_float3( 1.0f,1.0f,1.0f );
   float3 light_pos2= to_float3(50.0f, 50.0f, 20.0f ); float3 light_color2 =to_float3( 1.0f,1.0f,1.0f ); 
 
   
   float3 ro=to_float3(0.0f, 7.0f, 0.0f-t);
   float h1= pathterrain(ro.x*0.5f,ro.z);
   //float h2= pathterrain(ro.y*0.5f,ro.z);
   float h3= pathterrain(ro.x*0.5f,ro.y);
   
   ro.y-=h1;
   ro.x+=h3;
   float3 rd=normalize( to_float3(uv.x,uv.y,1.0f));
      
    light_pos1+=ro;
    light_pos2+=ro;
    float3 col= Render( ro,  rd,light_pos1,light_pos2,light_color1,light_color2, iChannel0, &mObj);
    fragColor = to_float4_aw(col,1.0f);


  SetFragmentShaderComputedColor(fragColor);
}