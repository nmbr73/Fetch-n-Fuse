
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

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

    
//__DEVICE__ TObj mObj;
__DEVICE__ float3 glpRoRd;
__DEVICE__ float2 gres2;
//__DEVICE__ float itime;

#define PI 3.14159265358979323846264f
#define MATERIAL_NO -1.0f
#define COLOR_NO -1.0f
#define COLORSKY to_float3(0.1f, 0.1f, 0.6f)


struct Ray
{   
  float3 ro; // origin
  float3 rd; // direction
};


///Gracias a SHane...16-jun-2020
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
// Connect 'Texture: Wood' to iChannel0


//----------image
//por jorge2017a1-
#define MAX_STEPS 100
#define MAX_DIST 100.0f
#define MIN_DIST 0.001f
#define EPSILON 0.001f
#define REFLECT 2

__DEVICE__ float3 GetColorYMaterial(float3 p,  float3 n, float3 ro,  float3 rd, int id_color, float id_material, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1, __TEXTURE2D__ iChannel2, __TEXTURE2D__ iChannel3);
__DEVICE__ float3 getMaterial( float3 pp, float id_material);
//__DEVICE__ float3 light_pos1,  light_color1 ;
//__DEVICE__ float3 light_pos2,  light_color2 ;

//__DEVICE__ float t1, t2;

//operacion de Union  por FabriceNeyret2
#define opU3(d1, d2) ( d1.x < d2.x ? d1 : d2 )
#define opU(d1, d2) ( d1.x < d2.x ? d1 : d2 )


__DEVICE__ float sdSphere( float3 p, float s )
  { return length(p)-s;}
__DEVICE__ float sdBox( float3 p, float3 b )
  { float3 d = abs_f3(p) - b;   return length(_fmaxf(d,to_float3_s(0.0f)))+ _fminf(max(d.x,_fmaxf(d.y,d.z)),0.0f); }

///----------Operacion de Distancia--------
__DEVICE__ float intersectSDF(float distA, float distB)
  { return _fmaxf(distA, distB);}
__DEVICE__ float unionSDF(float distA, float distB)
  { return _fminf(distA, distB);}
__DEVICE__ float differenceSDF(float distA, float distB)
  { return _fmaxf(distA, -distB);}
//----------oPeraciones de Repeticion
__DEVICE__ float opRep1D( float p, float c )
  { float q = mod_f(p+0.5f*c,c)-0.5f*c; return  q ;}
///------------------------------------
// object transformation
__DEVICE__ float3 rotate_x(float3 p, float phi)
{
  float c = _cosf(phi);  float s = _sinf(phi);
  return to_float3(p.x, c*p.y - s*p.z, s*p.y + c*p.z);
}
__DEVICE__ float3 rotate_y(float3 p, float phi)
{
  float c = _cosf(phi);  float s = _sinf(phi);
  return to_float3(c*p.x + s*p.z, p.y, c*p.z - s*p.x);
}
__DEVICE__ float3 rotate_z(float3 p, float phi)
{ 
  float c = _cosf(phi);  float s = _sinf(phi);
  return to_float3(c*p.x - s*p.y, s*p.x + c*p.y, p.z);
}
__DEVICE__ float2 rotatev2(float2 p, float ang)
{ 
  float c = _cosf(ang);
  float s = _sinf(ang);
  return to_float2(p.x*c - p.y*s, p.x*s + p.y*c);
}

__DEVICE__ float sdBox2D(float2 p, float2 b) {
  float2  di = abs_f2(p) - b;
  float mc = _fmaxf(di.x,di.y);
  return _fminf(mc,length(_fmaxf(di,to_float2_s(0.0f))));
}

__DEVICE__ float sdCross( in float3 p )
{
  float da = sdBox2D(swi2(p,x,y),to_float2_s(1.0f));
  float db = sdBox2D(swi2(p,y,z),to_float2_s(1.0f));
  float dc = sdBox2D(swi2(p,z,x),to_float2_s(1.0f));
  return _fminf(da,_fminf(db,dc));
}

#define iterations 4.0f  //5.0

__DEVICE__ float MengerSponge(float3 p,float t1,float t2) 
{  if (t1<t2)
    p.y= opRep1D(p.y, 1.0f );
   else
    p.z= opRep1D(p.z, 1.0f );
    
    float d = sdBox(p,to_float3_s(1.0f));
    
    for (float i = 0.0f; i < iterations; i+=1.0f) 
    {   float scale = _powf(2.0f,i);
        float3 q = mod_f3(scale*p,2.0f)-1.0f;
        q = 1.0f-abs_f3(q);
        float c = sdCross(q*3.0f)/(scale*3.0f);
        d = _fmaxf(d,-c),1.0f;
        p += scale/3.0f;
   }
    return d;
}

__DEVICE__ float3 GetDist(float3 p,float t1,float t2  ) 
{ 
  float3 res= to_float3(9999.0f, -1.0f,-1.0f);  float3 p0=p;
  float planeDist1 = p.y+0.0f;  //piso inf
  res =opU3(res, to_float3(planeDist1,-1.0f,7.0f));
  p.y=p.y-5.0f;
   
  float d1=MengerSponge(p,t1,t2);
  res =opU3(res, to_float3(d1,100.0f,-1.0f));
   
  return res;
}

__DEVICE__ float3 GetNormal(float3 p,float t1,float t2)
{   float d = GetDist(p,t1,t2).x;
    float2 e = to_float2(0.001f, 0);
    float3 n = d - to_float3(GetDist(p-swi3(e,x,y,y),t1,t2).x,GetDist(p-swi3(e,y,x,y),t1,t2).x,GetDist(p-swi3(e,y,y,x),t1,t2).x);
    return normalize(n);
}

__DEVICE__ float RayMarch(float3 ro, float3 rd, int PMaxSteps,float t1,float t2, TObj *mObj)
{   float t = 0.0f; 
    float3 dS=to_float3(9999.0f,-1.0f,-1.0f);
    float marchCount = 0.0f;
    float3 p;
    
    #define DISTANCE_BIAS 0.75
    float minDist = 9999.0f; 
    
    for(int i=0; i <= PMaxSteps; i++) 
    {    p = ro + rd*t;
        dS = GetDist(p,t1,t2);
        t += dS.x;
        if ( _fabs(dS.x)<MIN_DIST  || i == PMaxSteps)
            {(*mObj).hitbln = true; minDist = _fabs(t); break;}
        if(t>MAX_DIST)
            {(*mObj).hitbln = false;    minDist = t;    break; } 
        marchCount++;
    }
    (*mObj).dist = minDist;
    (*mObj).id_color = dS.y;
    (*mObj).marchCount=marchCount;
    (*mObj).id_material=dS.z;
    (*mObj).normal=GetNormal(p,t1,t2);
    (*mObj).phit=p;
    return t;
}


__DEVICE__ float GetShadow(float3 p, float3 plig,float t1,float t2, TObj *mObj)
{   float3 lightPos = plig;
    float3 l = normalize(lightPos-p);
    float3 n = GetNormal(p,t1,t2);
    float dif = clamp(dot(n, l), 0.0f, 1.0f);
    float d = RayMarch(p+n*MIN_DIST*2.0f, l , MAX_STEPS/2,t1,t2,mObj);
    if(d<length(lightPos-p)) dif *= 0.1f;
    return dif;
}


__DEVICE__ float occlusion(float3 pos, float3 nor,float t1,float t2)
{
    float sca = 2.0f, occ = 0.0f;
    for(int i = 0; i < 10; i++) {
    
        float hr = 0.01f + float(i) * 0.5f / 4.0f;        
        float dd = GetDist(nor * hr + pos,t1,t2).x;
        occ += (hr - dd)*sca;
        sca *= 0.6f;
    }
    return clamp( 1.0f - occ, 0.0f, 1.0f );    
}

__DEVICE__ float3 lightingv3(float3 normal,float3 p, float3 lp, float3 rd, float3 ro,float3 col, float t,float t1,float t2) 
{   float3 lightPos=lp;
    float3 hit = ro + rd * t;
    float3 norm = GetNormal(hit,t1,t2);
    
    float3 light = lightPos - hit;
    float lightDist = _fmaxf(length(light), 0.001f);
    float atten = 1.0f / (1.0f + lightDist * 0.125f + lightDist * lightDist * 0.05f);
    light /= lightDist;
    
    float occ = occlusion(hit, norm,t1,t2);
    
    float dif = clamp(dot(norm, light), 0.0f, 1.0f);
    dif = _powf(dif, 4.0f) * 2.0f;
    float spe = _powf(_fmaxf(dot(reflect(-light, norm), -rd), 0.0f), 8.0f);
    float3 color = col*(dif+0.35f +to_float3(0.35f,0.45f,0.5f)*spe) + to_float3(0.7f,0.9f,1)*spe*spe;
    color*=occ;
    return color;
}

__DEVICE__ float3 getColorTextura( float3 p, float3 nor,  int i, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1, __TEXTURE2D__ iChannel2, __TEXTURE2D__ iChannel3)
{  if (i==100 )
    //{ float3 col=tex3D(iChannel0, p/32.0f, nor); return col*2.0f; }
    //{ float3 col=tex3D(iChannel0, p/0.25f, nor); return col*2.0f; }
    //por Shane
    // Using the "Wood" texture.
    if (i==100){
    
       // Normal based UV coordinates.
       float2 uv = _fabs(nor.x)>0.5f? swi2(p,y,z) : _fabs(nor.y)>0.5f? swi2(p,x,z) : swi2(p,x,y);
       // Color sample, followed by rough sRGB to linear conversion.
       float3 col = swi3(texture(iChannel0, uv*2.0f),x,y,z); col *= col;
       // Second layer at a higher frequency, if you wanted.
       float3 col2 = swi3(texture(iChannel0, uv*4.0f + 0.35f),x,y,z); col2 *= col2;
       col = _mix(col, col2, 0.75f); // Mixing the two layers.
       // Smoothstep is a handy way to tweak the texture tone.
       col = smoothstep(to_float3_s(-0.2f), to_float3_s(0.6f), col); 
   
       return col;
    }

    
  if (i==101 ) { return tex3D(iChannel1, p/32.0f, nor); }
  if (i==102 ) { return tex3D(iChannel2, p/32.0f, nor); }
  if (i==103 ) { return tex3D(iChannel3, p/32.0f, nor); }
  
  return to_float3_s(0.0f);
}

__DEVICE__ float3 Getluz(float3 p, float3 ro, float3 rd, float3 nor , float3 colobj ,float3 plight_pos, float tdist,float t1,float t2)
{  
    float intensity=1.0f;
    float3 result;
    result = lightingv3( nor, p, plight_pos,  rd,ro, colobj, tdist,t1,t2);
    return result;
}

__DEVICE__ float3 render_sky_color(float3 rd)
{   
    float t = (rd.x + 1.0f) / 2.0f;
    float3 col= to_float3((1.0f - t) + t * 0.3f, (1.0f - t) + t * 0.5f, (1.0f - t) + t);
    float3  sky = _mix(to_float3(0.0f, 0.1f, 0.4f)*col, to_float3(0.3f, 0.6f, 0.8f), 1.0f - rd.y);
  return sky;
}

//https://www.shadertoy.com/view/4lcSRn   ///IQ
__DEVICE__ float3 pattern( in float2 uv )
{   float3 col = to_float3_s(0.4f);
    col += 0.4f*smoothstep(-0.01f,0.02f,_cosf(uv.x*0.5f)*_cosf(uv.y*0.5f)); 
    col *= smoothstep(-1.0f,-0.98f,_cosf(uv.x))*smoothstep(-1.0f,-0.98f,_cosf(uv.y));
    return col;
}

__DEVICE__ float3 getMaterial( float3 pp, float id_material)
{ float3 col=to_float3_s(1.0f);
  float3 p=pp;
  float3 l1;

    if (id_material==7.0f)
        {return pattern( swi2(p,x,z) );}
    if (id_material==8.0f)
        {return pattern( swi2(p,x,y) );}
    if (id_material==9.0f)
        {return pattern( swi2(p,z,y) );}
      
    return pattern( swi2(p,z,y) );
}

__DEVICE__ float3 GetColorYMaterial(float3 p,  float3 n, float3 ro,  float3 rd, int id_color, float id_material, TObj *mObj, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1, __TEXTURE2D__ iChannel2, __TEXTURE2D__ iChannel3)
{    float3 colobj; 
    
    if( (*mObj).hitbln==false) return  render_sky_color(rd);
    
    
    
    if ( float( id_color)>=100.0f  && (float)( id_color)<=199.0f ) 
    { 
      float3 coltex=getColorTextura(p, n, (int)( id_color),iChannel0,iChannel1,iChannel2,iChannel3);
      colobj=coltex;
    }

    if (id_material>-1.0f && id_color==-1)
      { 
        colobj=to_float3_s(0.5f);
        colobj*=getMaterial(p, id_material); 
        return colobj;
      }
    return colobj;
}

__DEVICE__ float3 linear2srgb(float3 c) 
{   
  return mix_f3(12.92f * c,1.055f * pow_f3(c, to_float3_s(1.0f/1.8f)) - 0.055f,
                step(to_float3_s(0.0031308f), 
                c));
}

__DEVICE__ float3 exposureToneMapping(float exposure, float3 hdrColor) 
{    
  return to_float3_s(1.0f) - exp_f3(-hdrColor * exposure);  
}


__DEVICE__ float3 ACESFilm(float3 x)
{
  
  float a = 2.51f;
  float b = 0.03f;
  float c = 2.43f;
  float d = 0.59f;
  float e = 0.14f;
  return (x*(a*x+b))/(x*(c*x+d)+e);
}


__DEVICE__ float3 Render(float3 ro, float3 rd,float3 light_pos1,float3 light_pos2,float3 light_color1,float3 light_color2,float t1,float t2, TObj *mObj,  __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1, __TEXTURE2D__ iChannel2, __TEXTURE2D__ iChannel3)
{  float3 col = to_float3_s(0);
   TObj Obj;
   (*mObj).rd=rd;
   (*mObj).ro=ro;
   float3 p;

     float d=RayMarch(ro,rd, MAX_STEPS,t1,t2,mObj);
   
    Obj=*mObj;
    if((*mObj).hitbln) 
    {   p = (ro + rd * d );  
        float3 nor=(*mObj).normal;
        float3 colobj;
        colobj=GetColorYMaterial( p, nor, ro, rd,  int( Obj.id_color), Obj.id_material,mObj,iChannel0,iChannel1,iChannel2,iChannel3);

        float dif1=1.0f;
        float3 result;
        result=  Getluz( p,ro,rd, nor, colobj ,light_pos1,d,t1,t2)*light_color1;
        result+= Getluz( p,ro,rd, nor, colobj ,light_pos2,d,t1,t2)*light_color2;
   
        col= result;
        col= (ACESFilm(col)+linear2srgb(col)+col+ exposureToneMapping(3.0f, col))/4.0f ;
    }
    else if(d>MAX_DIST)
    col= render_sky_color(rd);
   return col;
}

__DEVICE__ Ray RotarEnCirculo(float3 ro,float3 rd,float iTime)
{
   float r=1.7f+0.3f*_sinf(iTime);
   float veltime=10.0f;  
    float tt = radians( -iTime*veltime);
    float3  pos=to_float3(0.0f,0.0f,0.0f);
    float3 rotation1 = pos+to_float3(r*_sinf(tt), 0.0f, r*_cosf(tt));
    ro +=rotation1;   
    rd=rotate_y( to_float3(-rd.x,rd.y,-rd.z), tt);
    Ray ret = {ro,rd};
    
    return  ret;  //Ray(ro,rd);
}


__KERNEL__ void MengerBuildingFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
  float2 uv = (fragCoord-0.5f*iResolution)/iResolution.y;
   TObj mObj;
   mObj.uv=uv;
    float t;
    t=mod_f(iTime*0.5f,360.0f);
    //itime=t;
  //mObj.blnShadow=false;
    mObj.blnShadow=true;
        
   float3 light_pos1= to_float3(-10.0f, 20.0f, -10.0f ); float3 light_color1=to_float3( 1.0f,1.0f,1.0f );
   float3 light_pos2= to_float3(10.0f, 10.0f, 10.0f ); float3 light_color2 =to_float3( 1.0f,1.0f,1.0f ); 
   float3 ro,rd;
   
   float t1=mod_f(iTime,3.0f);
   float t2=mod_f(iTime,5.0f);
   
   if (t1<t2)
   {
    ro=to_float3(0.0f,5.0f+t,0.0f);
    rd=normalize( to_float3(uv.x,uv.y,1.0f)); 
    Ray ray= RotarEnCirculo(ro,rd,iTime);
    ro=ray.ro;
    rd=ray.rd;
   }
   else
   {
    ro=to_float3(0.0f+0.5f*_sinf(iTime),5.0f,-1.0f+t);
    rd=normalize( to_float3(uv.x,uv.y,1.0f)); 
   }

    light_pos1+=ro;
    light_pos2+=ro;
    float3 col= Render( ro,  rd,light_pos1,light_pos2,light_color1,light_color2,t1,t2,&mObj,iChannel0,iChannel1,iChannel2,iChannel3);
    fragColor = to_float4_aw(col,1.0f);


  SetFragmentShaderComputedColor(fragColor);
}