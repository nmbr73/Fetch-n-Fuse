

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
//----------image
//por jorge2017a2-
///tunnel x
//https://www.shadertoy.com/view/WlsSWS
//   01/feb/2022
#define MAX_STEPS 100
#define MAX_DIST 100.
#define MIN_DIST 0.001
#define EPSILON 0.001
#define REFLECT 2

vec3 GetColorYMaterial(vec3 p,  vec3 n, vec3 ro,  vec3 rd, int id_color, float id_material);
vec3 getMaterial( vec3 pp, float id_material);
vec3 light_pos1;  vec3 light_color1 ;
vec3 light_pos2;  vec3 light_color2 ;

//operacion de Union  por FabriceNeyret2
#define opU3(d1, d2) ( d1.x < d2.x ? d1 : d2 )
#define opU(d1, d2) ( d1.x < d2.x ? d1 : d2 )


float sdBox( vec3 p, vec3 b )
	{ vec3 d = abs(p) - b;   return length(max(d,0.0))+ min(max(d.x,max(d.y,d.z)),0.0); }

///----------Operacion de Distancia--------
float intersectSDF(float distA, float distB)
	{ return max(distA, distB);}
float unionSDF(float distA, float distB)
	{ return min(distA, distB);}
float differenceSDF(float distA, float distB)
	{ return max(distA, -distB);}
float opRep1D( float p, float c )
	{ float q = mod(p+0.5*c,c)-0.5*c; return  q ;}

// object transformation
vec3 rotate_x(vec3 p, float phi)
{   float c = cos(phi);	float s = sin(phi);
    return vec3(p.x, c*p.y - s*p.z, s*p.y + c*p.z);
}
vec3 rotate_y(vec3 p, float phi)
{	float c = cos(phi);	float s = sin(phi);
	return vec3(c*p.x + s*p.z, p.y, c*p.z - s*p.x);
}
vec3 rotate_z(vec3 p, float phi)
{	float c = cos(phi);	float s = sin(phi);
	return vec3(c*p.x - s*p.y, s*p.x + c*p.y, p.z);
}
vec2 rotatev2(vec2 p, float ang)
{   float c = cos(ang);
    float s = sin(ang);
    return vec2(p.x*c - p.y*s, p.x*s + p.y*c);
}


//https://www.shadertoy.com/view/WlsSWS
float pathterrain(float x,float z)
{   // Common height function for path and terrain
    return  sin(x*.5 )*1.+cos(z*.3 )*0.3 +cos(x*3.+z )*0.1+sin(x-z*.2 )*0.2;
}

vec3 GetDist(vec3 p  ) 
{	vec3 res= vec3(9999.0, -1.0,-1.0);  vec3 p0=p;
	float planeDist1 = p.y+0.0;  //piso inf
   
    res =opU3(res, vec3(planeDist1,-1.0,7.0));
   
    p.y=p.y-5.0;
    p=rotate_y(p, radians(90.0));
    
    float h1= pathterrain(p.x*0.5,p.z);
    float h2= pathterrain(p.y*0.5,p.z);
    float h3= pathterrain(p.x*0.5,p.y);
    
    p.x=  opRep1D( p.x, 6.0 );
    float d2= sdBox( p-vec3(0.0,h1+h2,0.0+h3), vec3(6.0,3.0,6.0) );
    res =opU3(res, vec3(d2,100.0,-1.0));
   
    return res;
}

vec3 GetNormal(vec3 p)
{   float d = GetDist(p).x;
    vec2 e = vec2(.001, 0);
    vec3 n = d - vec3(GetDist(p-e.xyy).x,GetDist(p-e.yxy).x,GetDist(p-e.yyx).x);
    return normalize(n);
}

float RayMarch(vec3 ro, vec3 rd, int PMaxSteps)
{   float t = 0.; 
    vec3 dS=vec3(9999.0,-1.0,-1.0);
    float marchCount = 0.0;
    vec3 p;
    float minDist = 9999.0; 
    
    for(int i=0; i <= PMaxSteps; i++) 
    {  	p = ro + rd*t;
        dS = GetDist(p);
        t += dS.x;
        if ( abs(dS.x)<MIN_DIST  || i == PMaxSteps)
            {mObj.hitbln = true; minDist = abs(t); break;}
        if(t>MAX_DIST)
            {mObj.hitbln = false;    minDist = t;    break; } 
        marchCount++;
    }
    mObj.dist = minDist;
    mObj.id_color = dS.y;
    mObj.marchCount=marchCount;
    mObj.id_material=dS.z;
    mObj.normal=GetNormal(p);
    mObj.phit=p;
    return t;
}


float GetShadow(vec3 p, vec3 plig)
{   vec3 lightPos = plig;
    vec3 l = normalize(lightPos-p);
    vec3 n = GetNormal(p);
    float dif = clamp(dot(n, l), 0., 1.);
    float d = RayMarch(p+n*MIN_DIST*2., l , MAX_STEPS/2);
    if(d<length(lightPos-p)) dif *= .1;
    return dif;
}


float occlusion(vec3 pos, vec3 nor)
{   float sca = 2.0, occ = 0.0;
    for(int i = 0; i < 10; i++) {
        float hr = 0.01 + float(i) * 0.5 / 4.0;
        float dd = GetDist(nor * hr + pos).x;
        occ += (hr - dd)*sca;
        sca *= 0.6;
    }
    return clamp( 1.0 - occ, 0.0, 1.0 );    
}

vec3 lightingv3(vec3 normal,vec3 p, vec3 lp, vec3 rd, vec3 ro,vec3 col, float t) 
{   vec3 lightPos=lp;
    vec3 hit = ro + rd * t;
    vec3 norm = GetNormal(hit);
    vec3 light = lightPos - hit;
    float lightDist = max(length(light), .001);
    float atten = 1. / (1.0 + lightDist * 0.125 + lightDist * lightDist * .05);
    light /= lightDist;
    
    float occ = occlusion(hit, norm);
    float dif = clamp(dot(norm, light), 0.0, 1.0);
    dif = pow(dif, 4.) * 2.;
    float spe = pow(max(dot(reflect(-light, norm), -rd), 0.), 8.);
    vec3 color = col*(dif+.35 +vec3(.35,.45,.5)*spe) + vec3(.7,.9,1)*spe*spe;
    color*=occ;
    return color;
}

vec3 getColorTextura( vec3 p, vec3 nor,  int i)
{	if (i==100 )
    { vec3 col=tex3D(iChannel0, p/16., nor); return col; }
}

vec3 Getluz(vec3 p, vec3 ro, vec3 rd, vec3 nor , vec3 colobj ,vec3 plight_pos, float tdist)
{  float intensity=1.0;
     vec3 result;
    result = lightingv3( nor, p, plight_pos,  rd,ro, colobj, tdist);
    return result;
}


vec3 GetColorYMaterial(vec3 p,  vec3 n, vec3 ro,  vec3 rd, int id_color, float id_material)
{  	vec3 colobj; 
      
    if ( float( id_color)>=100.0  && float( id_color)<=199.0 ) 
 	{  vec3 coltex=getColorTextura(p, n, int( id_color));
        colobj=coltex;
	}

    return colobj;
}


vec3 linear2srgb(vec3 c) 
{ return mix(12.92 * c,1.055 * pow(c, vec3(1.0/1.8)) - 0.055, step(vec3(0.0031308), c)); }

vec3 exposureToneMapping(float exposure, vec3 hdrColor) 
{ return vec3(1.0) - exp(-hdrColor * exposure); }


vec3 ACESFilm(vec3 x)
{   float a,b,c,d,e;
    a = 2.51; b = 0.03; c = 2.43; 
    d = 0.59; e = 0.14;
    return (x*(a*x+b))/(x*(c*x+d)+e);
}

vec3 Render(vec3 ro, vec3 rd)
{  vec3 col = vec3(0);
   TObj Obj;
   mObj.rd=rd;
   mObj.ro=ro;
   vec3 p;

     float d=RayMarch(ro,rd, MAX_STEPS);
   
    Obj=mObj;
    if(mObj.hitbln) 
    {   p = (ro + rd * d );  
        vec3 nor=mObj.normal;
        vec3 colobj;
        colobj=GetColorYMaterial( p, nor, ro, rd,  int( Obj.id_color), Obj.id_material);

        float dif1=1.0;
        vec3 result;
        result=  Getluz( p,ro,rd, nor, colobj ,light_pos1,d)*light_color1;
        result+= Getluz( p,ro,rd, nor, colobj ,light_pos2,d)*light_color2;
   
        col= result;
        col= (ACESFilm(col)+linear2srgb(col)+col+ exposureToneMapping(3.0, col))/4.0 ;
    }
   
   return col;
}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{  vec2 uv = (fragCoord-.5*iResolution.xy)/iResolution.y;
   mObj.uv=uv;
    float t;
    t=mod(iTime*5.0,500.0);
    itime=t;
	//mObj.blnShadow=false;
    mObj.blnShadow=true;
        
 	light_pos1= vec3(0.0, 10.0, -20.0 ); light_color1=vec3( 1.0,1.0,1.0 );
 	light_pos2= vec3(50.0, 50.0, 20.0 ); light_color2 =vec3( 1.0,1.0,1.0 ); 
 
   
   vec3 ro=vec3(0.0, 7.0, 0.0-t);
   float h1= pathterrain(ro.x*0.5,ro.z);
   //float h2= pathterrain(ro.y*0.5,ro.z);
   float h3= pathterrain(ro.x*0.5,ro.y);
   
   ro.y-=h1;
   ro.x+=h3;
   vec3 rd=normalize( vec3(uv.x,uv.y,1.0));
      
    light_pos1+=ro;
    light_pos2+=ro;
    vec3 col= Render( ro,  rd);
    fragColor = vec4(col,1.0);
}


// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
//----------common
struct TObj
{
    float id_color;
    float id_objeto;
    float id_material;
    float dist;
    vec3 normal;
    vec3 ro;
    vec3 rd;
    vec2 uv;
    vec3 color;
    vec3 p;
    vec3 phit; //22-mar-2021
    vec3 rf;
    float marchCount;
    bool blnShadow;
    bool hitbln;
};

    
TObj mObj;
vec3 glpRoRd;
vec2 gres2;
float itime;

#define PI 3.14159265358979323846264
#define MATERIAL_NO -1.0
#define COLOR_NO -1.0
#define COLORSKY vec3(0.1, 0.1, 0.6)


///Gracias a SHane...16-jun-2020
vec3 tex3D( sampler2D tex, in vec3 p, in vec3 n ){    
    n = max(n*n - .2, .001); // max(abs(n), 0.001), etc.
    n /= dot(n, vec3(1)); 
    vec3 tx = texture(tex, p.yz).xyz;
    vec3 ty = texture(tex, p.zx).xyz;
    vec3 tz = texture(tex, p.xy).xyz;
    return mat3(tx*tx, ty*ty, tz*tz)*n; 
}

