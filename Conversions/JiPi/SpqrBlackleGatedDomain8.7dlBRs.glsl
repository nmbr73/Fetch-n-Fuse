

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
//CC0 1.0 Universal https://creativecommons.org/publicdomain/zero/1.0/
//To the extent possible under law, Blackle Mori has waived all copyright and related or neighboring rights to this work.


//returns a vector pointing in the direction of the closest neighbouring cell

#define saturate(a) (clamp((a),0.,1.))
mat2 rot(float a){
    float s = sin(a);
    float c = cos(a);
    return mat2(c,s,-s,c);
}

mat3 m = mat3( 0.00,  0.80,  0.60,
              -0.80,  0.36, -0.48,
              -0.60, -0.48,  0.64 );

float hash( float n )
{
    return fract(sin(n)*43758.5453123);
}

// 3d noise function
float noise( in vec3 x )
{
    vec3 p = floor(x);
    vec3 f = fract(x);
    f = f*f*(3.0-2.0*f);
    float n = p.x + p.y*57.0 + 113.0*p.z;
    float res = mix(mix(mix( hash(n+  0.0), hash(n+  1.0),f.x),
                        mix( hash(n+ 57.0), hash(n+ 58.0),f.x),f.y),
                    mix(mix( hash(n+113.0), hash(n+114.0),f.x),
                        mix( hash(n+170.0), hash(n+171.0),f.x),f.y),f.z);
    return res;
}

// fbm noise for 2-4 octaves including rotation per octave
float fbm( vec3 p )
{
    float f = 0.0;
    f += 0.5000*noise( p );
	p = m*p*2.02;
    f += 0.2500*noise( p ); 
	p = m*p*2.03;
    f += 0.1250*noise( p );
	p = m*p*2.01;
    f += 0.0625*noise( p );
    return f/0.9375;
}
float box(vec3 p,vec3 s)
{
  vec3 d=abs(p)-s;
  return length(max(d,0.));
}



float hash(float a, float b) {
    return fract(sin(a*1.2664745 + b*.9560333 + 3.) * 14958.5453);
}



float tick (float t){
    float i = floor(t);
    float r = fract(t);
    r = smoothstep(0.,1.,r);
    r = smoothstep(0.,1.,r);
    r = smoothstep(0.,1.,r);
    r = smoothstep(0.,1.,r);
    
    return i + r;
}

float tock (float t){
    float i = floor(t);
    float r = fract(t);
    r = smoothstep(0.,1.,r);
  
    
    return i + r;
}

float ball;
#define MOD3 vec3(.1031,.11369,.13787)

//value noise hash
float hash31(vec3 p3)
{
	p3  = fract(p3 * MOD3);
    p3 += dot(p3, p3.yzx + 19.19);
    return -1.0 + 2.0 * fract((p3.x + p3.y) * p3.z);
}
vec3 randomdir (float n) {
    return fract(vec3(5.1283,9.3242,13.8381) * hash(n) * 8421.4242);
}
float glow = 0.;
float sph(vec3 p,float r) {
    return length(p) -r ;
}
float cyl (vec2 p, float r){
    return length(p) - r;
}
float map(vec3 p) {

    //geo
    

    float tt = iTime * .3;
    vec3 q = p;
    
    // wierd skylights
    q.yz *= rot(.42);
    q.xz *= rot(.4);
  
    q.xy *= rot(q.z/50.);
    q.x += tt * 10.;
    q.xz = mod( q.xz, 40.) - 20.;
  
    float uu = cyl(q.xz,.001);
    glow += .04/(.1+pow(uu,1.8));
    
    float domain = 1.4;
  ;
   
    
    vec3 id = floor((p*.1)/domain);
     vec3 id2 = floor((p)/domain);
    p = mod(p,domain) - domain/2.;
    
    float thresh = fbm(id);
    
    float rando = hash31(id2);


   
    vec3 flit = vec3(.04);
    flit.xz *= rot(rando*5.1+tt*2.3);
    flit.yz *= rot(rando*4.2+tt*1.4);
    flit.xy *= rot(rando*3.3+tt*1.1);
   
    //vec3 flit = randomdir(hash31(id)) * .2;
    
    
    vec3 jitter = flit * sin((tt*9.1+rando*12.1));
  
    
   
    
    //(.5)hash(float(id)) * vec3(.5) * sin(iTime*6.+3.*hash(float(id)));
    
    if (  rando *.6< thresh) {

        p = abs(p);
        if (p.x > p.y) p.xy = p.yx;
        if (p.y > p.z) p.yz = p.zy;
        if (p.x > p.y) p.xy = p.yx;
        p.z -= domain;
        //return length(p)-1.;
        
        float u = box(p + jitter, vec3(.4));
      
        return min(uu,u*.5);
        
    } else {
        //return length(p)-1.;
        float u = box(p + jitter, vec3(.4));
       
        return min(uu,u*.5);
    }
 
}


vec3 norm(vec3 p,vec2 d)
{
  return normalize(vec3(
    map(p+d.yxx)-map(p-d.yxx),
    map(p+d.xyx)-map(p-d.xyx),
    map(p+d.xxy)-map(p-d.xxy)
  ));
}

vec3 norm(vec3 p) {
    mat3 k = mat3(p,p,p)-mat3(0.01);
    return normalize(map(p) - vec3( map(k[0]),map(k[1]),map(k[2]) ));
}


const float PI = acos(-1.);

vec3 pixel_color(vec2 uv) {
   
  // nav

    float tt = iTime ;
    vec3 jump = vec3(0) * tick(tt*.05)*77.2;
    jump.xz *= rot(tt*.00001);
  
    
    vec3 s = vec3(10.,3.2,7.1)*tt*.18 + jump;
    vec3 arm = vec3(1,0,0);
    arm.xz *= rot(sin(tt* .19));
    arm.yz *= rot(sin(tt*.23));
    //arm.yx *= rot(sin(tt*.28));
    
    vec3 t = s + arm;
    
 
 
  
  vec3 cz=normalize(t-s);
  vec3 cx=normalize(cross(cz,vec3(0,1,0)));
  vec3 cy=-normalize(cross(cz,cx));
  cz -= dot(uv,uv)/15.;
  vec3 r=normalize(cx*uv.x+cy*uv.y+cz);
 

    
    vec3 p = s;
    bool hit = false;
  
    float d;
    float i;
    float dd = 0.;
    //ray marching
    for ( i = 0.; i < 1500.; i++) {
        
        d = map(p);
        d = abs(d);
        if ( d < .001) {
           hit = true;
           break;
        }
        if (dd>10000.) { break;}
        
        dd += d;
        p+=d*r;
    }

 
 
  
  vec3 col = vec3(.8, .5, .2);
  //col = vec3(.1,.1,.2)*1.;

  
  float ao = pow(1. - i/500.,6.);
  col *= ao;
  col += glow*.6;
  
  vec3 light = normalize(vec3(1));
  vec3 n = norm(p);
 // if ( dot(light,n) < 0.) { light = -light;}
  float spec =pow(max(dot(reflect(-light,n),-r),0.),40.) * 10.;
  col += spec * .1;
   float diff = max(0., dot(n,light)*.5 +.5);
   col *= diff;
  vec3 n2 = norm(p, vec2(0.0, 1E-2 ));// + 3E-2*.01) );
  vec3 n1 = norm(p, vec2(0.0, 2.3E-2) );


  float edge = saturate(length(n1-n2)/0.1);
  
  if ( edge > 0.01) {
      col = vec3(0);
  }
  
  
    
  if (! hit){
      col = vec3(0);
   
  }



  return col; 
}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (fragCoord-.5*iResolution.xy)/iResolution.y;
    fragColor = vec4(0);
    fragColor += vec4(pixel_color(uv), 1.);
  	fragColor.xyz = sqrt(fragColor.xyz/fragColor.w);
}

/*
  
  //float uniformity = (sin(iTime*.01)*.5 + .5) * 10. + 5.;
  float uniformity = 15.;
  vec3 hue = 1.-sin(p/uniformity);

  vec3 light =normalize(vec3(60,10,10));
  if ( dot(light,n) < 0.) { light = -light;}

  float diff = max(0., dot(n,light)*.5 +.5);
 
  float spec =pow(max(dot(reflect(-light,n),-r),0.),40.) * 10.;
  vec3 fog = vec3(0);
 
 // vec3 col = mix(spec * 0.2 + hue * ( diff ),fog, min(fren,.8));
  vec3 col = mix(spec * 0.2 + hue * ( diff ),fog,.5);
  
  col = mix(col,fog,1. - exp(-.000003*dd*dd*dd));
*/