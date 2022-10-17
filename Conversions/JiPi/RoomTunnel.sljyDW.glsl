

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
//----------image
//por jorge2017a2-
#define MAX_STEPS 100
#define MAX_DIST 110.
#define MIN_DIST 0.001
#define EPSILON 0.001
#define Ka 0.5
#define Kd 0.4
//room tunnel---11-abril-2022
//gracias a la comunidad de Shadertoy por su Informacion 
//y a IQ por compartir su conocimieto :)
vec3 GetColorYMaterial(vec3 p,  vec3 n, vec3 ro,  vec3 rd, int id_color, float id_material);
vec3 getMaterial( vec3 pp, float id_material);
vec3 light_pos1;  vec3 light_color1 ;
vec3 light_pos2;  vec3 light_color2 ;

//operacion de Union  por FabriceNeyret2
#define opU2(d1, d2) ( d1.x < d2.x ? d1 : d2 )

float sdBox( vec3 p, vec3 b )
	{ vec3 d = abs(p) - b;   return length(max(d,0.0))+ min(max(d.x,max(d.y,d.z)),0.0); }
float sdCylinderXY( vec3 p, vec2 h )
	{ vec2 d = abs(vec2(length(p.xy),p.z)) - h; return min(max(d.x,d.y),0.0) + length(max(d,0.0)); }

float Intersect(float distA, float distB) { return max(distA, distB);}
float Union(float distA, float distB)  { return min(distA, distB);}
float Difference(float distA, float distB) { return max(distA, -distB);}

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


float opRep1D( float p, float c )
	{ float q = mod(p+0.5*c,c)-0.5*c; return  q ;}

float sdTriPrism( vec3 p, vec2 h )
{ vec3 q = abs(p);
  return max(q.z-h.y,max(q.x*0.866025+p.y*0.5,-p.y)-h.x*0.5);
}

vec2 GetDist(vec3 p  ) 
{	vec2 res= vec2(9999.0, -1.0); 
    vec3 p0=p;

	 p.y=p.y-11.0;
    vec3 prep1=p;
    vec3 prep2=p;
    vec3 prep3=p;
    prep1.z= opRep1D(prep1.z,24.0 );
    prep2.z= opRep1D(prep2.z,55.0 );
    prep3.z= opRep1D(prep3.z,30.0 );
    
    
    vec3 pos;
    float d0= sdBox( prep2, vec3(35.0,25.0,9.0) ); //bloque 1
    //---plataforma
    float d0a= sdBox( prep1-vec3(0.0,-12.5,-8.0), vec3(30.,2.5,20.0) ); //bloque 1
    float d0b= sdBox( prep1-vec3(0.0,-11.5,-8.0), vec3(10.,2.0,21.0) ); //bloque 2
    d0a=Difference(d0a, d0b);
    
    float d1a= sdCylinderXY(prep2, vec2(10.0,10.0) );  //tubo ext
    float d1b= sdCylinderXY(prep2, vec2(9.0,11.0) );  //tubo int
    float dif1=Difference(d1a, d1b);
    d0=Difference(d0, d1a);
    pos=vec3(0.0,0.0,10.0);
    float d2a= sdCylinderXY(prep2-pos, vec2(10.0,1.0) ); //tubo ch,ext
    float d2b= sdCylinderXY(prep2-pos, vec2(8.0,1.5) ); // tubo ch,int
    float dif2=Difference(d2a, d2b);
    float d3=sdTriPrism(prep3-vec3(0.0,-12.0,5.0), vec2(5.0,20.0) );
    
    res =opU2(res, vec2(dif1,100.0));
    res =opU2(res, vec2(dif2,100.0));
    res =opU2(res, vec2(d0,100.0));
    res =opU2(res, vec2(d0a,100.0));
    res =opU2(res, vec2(d3,101.0));
    return res;
}

vec3 GetNormal(vec3 p)
{   float d = GetDist(p).x;
    vec2 e = vec2(.001, 0);
    vec3 n = d - vec3(GetDist(p-e.xyy).x,GetDist(p-e.yxy).x,GetDist(p-e.yyx).x);
    return normalize(n);
}

vec2 RayMarch(vec3 ro, vec3 rd, int PMaxSteps)
{   vec3 p;
    vec2 hit, object=vec2(0.1,0.0);
    for(int i=0; i <= PMaxSteps; i++) 
    { p = ro + rd*object.x;
      hit = GetDist(p);
      object.x += hit.x;
      object.y = hit.y;
      if (abs(hit.x) < EPSILON || object.x > MAX_DIST) break;
    }    
    return object;
}


float getSoftShadow(vec3 p, vec3 lightPos) {
    float res = 9999.0;
    float dist = 0.01;
    float lightSize = 0.03;
    for (int i = 0; i < MAX_STEPS; i++) {
      float hit = GetDist(p + lightPos * dist).x;
      res = min(res, hit / (dist * lightSize));
      dist += hit;
      if (hit < 0.0001 || dist > 60.0) break;
    }
    return clamp(res, 0.0, 1.0);
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

vec3 lightingv3(vec3 normal,vec3 p, vec3 lp, vec3 rd, vec3 ro,vec3 lightColor, float t) 
{   vec3 lightPos=lp;
    vec3 worldPos = p;
    vec3 V = -rd;
    vec3 N = normal;
    vec3 L = normalize (lightPos - worldPos);
    vec3 R = reflect (-L, N);

    float lightDist = max(length(L), .001);
    float atten=1.0 / (1.0 + lightDist * 0.125 + lightDist * lightDist * .05);
    L /= (lightDist*atten);

    float shadow = getSoftShadow(worldPos, L);// shadows
        
    float occ = occlusion(worldPos, N);// occ
    vec3 ambient = Ka + Ka * dot(normal, vec3(0., 1., 0.))*lightColor;
    ambient*=0.5;

    vec3 fresnel =  lightColor *  pow(clamp(1.0 + dot(rd, N), 0.0, 1.0), 2.0);;
    float diff= clamp(dot(N, L), 0.0, 1.0);
    vec3 diffuse =  lightColor * diff;
    float shininess=10.0;
    float specular    = pow(max(dot(R, V), 0.0), shininess);

    vec3 back = 0.5 * lightColor * clamp(dot(N, -L), 0.0, 1.0); // back
    vec3 colOut = occ*lightColor*(ambient+diffuse*shadow+.25 +back) + vec3(.7,.9,1)*specular*specular;
    return colOut;
}

vec3 getColorTextura( vec3 p, vec3 nor,  int i)
{	if (i==100 )
    { vec3 col=tex3D(iChannel0, p/32., nor); return col*2.0; }
	if (i==101 ) { return tex3D(iChannel0, p/32., nor)+0.2; }
}

vec3 render_sky_color(vec3 rd)
{   float t = (rd.x + 1.0) / 2.0;
    vec3 col= vec3((1.0 - t) + t * 0.3, (1.0 - t) + t * 0.5, (1.0 - t) + t);
    vec3  sky = mix(vec3(.0, .1, .4)*col, vec3(.3, .6, .8), 1.0 - rd.y);
    return sky;
}


vec3 GetMaterial(vec3 p,  vec3 nor, vec3 ro,  vec3 rd, int id_color)
{  	vec3 colobj; 
    if (id_color>=100 ){ return  getColorTextura( p, nor,id_color); }
}

vec3 linear2srgb(vec3 c) 
{ return mix(12.92 * c,1.055 * pow(c, vec3(1.0/1.8)) - 0.055, step(vec3(0.0031308), c)); }

vec3 exposureToneMapping(float exposure, vec3 hdrColor) 
{ return vec3(1.0) - exp(-hdrColor * exposure); }

vec3 ACESFilm(vec3 x)
{   float a,b,c,d,e;
    a = 2.51; b = 0.03; c = 2.43;d = 0.59; e = 0.14;
    return (x*(a*x+b))/(x*(c*x+d)+e);
}

vec3 Render(vec3 ro, vec3 rd)
{  vec3 col = vec3(0);
  vec3 p;
     vec2 hit=RayMarch(ro,rd, MAX_STEPS);
      if(hit.x<MAX_DIST)
       {   p = (ro + rd * hit.x );
        vec3 nor=GetNormal(p);
        vec3 colobj;
        colobj=GetMaterial( p, nor, ro, rd,  int(hit.y));
        vec3 result;
         result= lightingv3(nor, p,light_pos1, rd,ro,colobj,hit.x)*light_color1;
        result+= lightingv3(nor, p,light_pos2,rd, ro,colobj,hit.x)*light_color2;
        col= result/2.0;
        col= (ACESFilm(col)+linear2srgb(col)+col+ exposureToneMapping(3.0, col))/4.0 ;
        //if(5.0*sin(1.0-iTime)>0.0)
        //{ float sum=(col.x+col.y+col.z)/3.0; col=vec3(sum);} //grises
        //if(hit.y!=101.0)
        { float sum=col.x*0.299+col.y*0.587+col.z*0.11; col=vec3(sum);} //grises
        //Y'=0.299R'+0.587G'+0.114B'
    }
    //else if(hit.x>MAX_DIST)
    //col= render_sky_color(rd);
   return col;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{  vec2 uv = (fragCoord-.5*iResolution.xy)/iResolution.y;
    float t,t0;t0=iTime;
    t=mod(t0*10.0,1000.0);
 	light_pos1= vec3(-40.0, 80.0, -20.); light_color1=vec3( 1.0,1.0,1.0 );
 	light_pos2= vec3( 40.0, 80.0, -25.0 ); light_color2 =vec3( 1.0,1.0,1.0 );
   vec3 ro=vec3(0.0+4.0*sin(t0),10.0,t);
   vec3 rd=normalize( vec3(uv.x,uv.y,1.0));
   float ang=0.58*cos(180.-t0*0.5);
   rd= rotate_y(rd, ang);
    light_pos1+=ro;
    light_pos2+=ro;
    vec3 col= Render( ro,  rd);
    fragColor = vec4(col,1.0);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
//-------common
#define PI 3.14159265
///Gracias a Shane...16-jun-2020
vec3 tex3D( sampler2D tex, in vec3 p, in vec3 n ){    
  n = max(n*n - .2, .001); // max(abs(n), 0.001), etc.
  n /= dot(n, vec3(1)); 
  vec3 tx = texture(tex, p.yz).xyz;
  vec3 ty = texture(tex, p.zx).xyz;
  vec3 tz = texture(tex, p.xy).xyz;
  return mat3(tx*tx, ty*ty, tz*tz)*n; 
}

