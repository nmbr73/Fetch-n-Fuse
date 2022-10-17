

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
float sdSphere(vec3 p, float r )
{
  return length(p) - r;
}

float hash1( vec2 p )
{
    p  = 50.0*fract( p*0.3183099 );
    return fract( p.x*p.y*(p.x+p.y) );
}

float hash1( float n )
{
    return fract( n*17.0*fract( n*0.3183099 ) );
}

float noise( in vec3 x )
{
    vec3 p = floor(x);
    vec3 w = fract(x);
    
    #if 1
    vec3 u = w*w*w*(w*(w*6.0-15.0)+10.0);
    #else
    vec3 u = w*w*(3.0-2.0*w);
    #endif
    


    float n = 111.0*p.x + 317.0*p.y + 157.0*p.z;
    
    float a = hash1(n+(  0.0+  0.0+  0.0));
    float b = hash1(n+(111.0+  0.0+  0.0));
    float c = hash1(n+(  0.0+317.0+  0.0));
    float d = hash1(n+(111.0+317.0+  0.0));
    float e = hash1(n+(  0.0+  0.0+157.0));
	float f = hash1(n+(111.0+  0.0+157.0));
    float g = hash1(n+(  0.0+317.0+157.0));
    float h = hash1(n+(111.0+317.0+157.0));

    float k0 =   a;
    float k1 =   b - a;
    float k2 =   c - a;
    float k3 =   e - a;
    float k4 =   a - b - c + d;
    float k5 =   a - c - e + g;
    float k6 =   a - b - e + f;
    float k7 = - a + b + c - d + e - f - g + h;

    return -1.0+2.0*(k0 + k1*u.x + k2*u.y + k3*u.z + k4*u.x*u.y + k5*u.y*u.z + k6*u.z*u.x + k7*u.x*u.y*u.z);
}
const mat3 m3  = mat3( 0.00,  0.80,  0.60,
                      -0.80,  0.36, -0.48,
                      -0.60, -0.48,  0.64 );
float fbm_4( in vec3 x )
{
    float f = 2.0;
    float s = 0.5;
    float a = 0.0;
    float b = 0.5;
    for( int i=0; i<4; i++ )
    {
        float n = noise(x);
        a += b*n;
        b *= s;
        x = f*m3*x;
    }
	return a;
}
float fbm_2( in vec3 x )
{
    float f = 2.0;
    float s = 0.5;
    float a = 0.0;
    float b = 0.5;
    for( int i=0; i<2; i++ )
    {
        float n = noise(x);
        a += b*n;
        b *= s;
        x = f*m3*x;
    }
	return a;
}

float opSmoothSubtraction( float d1, float d2, float k ) {
    float h = clamp( 0.5 - 0.5*(d2+d1)/k, 0.0, 1.0 );
    return mix( d2, -d1, h ) + k*h*(1.0-h); }
float opSmoothUnion( float d1, float d2, float k )
{
    float h = clamp( 0.5 + 0.5*(d2-d1)/k, 0.0, 1.0 );
    return mix( d2, d1, h ) - k*h*(1.0-h); 
}
    
float map(vec3 p){


  float s;

  float s1 =sdSphere(p+vec3(1.0,0.1,0.0), 1.7*(sin(iTime+p.x)*0.005+1.0));
  float s2 =sdSphere(p+vec3(-1.0,0.0,0.1), 1.8*(sin(iTime+p.x+1.0)*0.005+1.0));
  s = opSmoothUnion(s1,s2,0.5);
  
  vec3 noiseP =p*0.7;

  float n =smoothstep(-0.2,1.0,fbm_4(noiseP*2.0+fbm_4(noiseP*2.0)*1.5))*0.1;
  s-=n;
  
  float skin = smoothstep(0.5,1.0,1.0-n*5.0);
  s+=skin*smoothstep(-1.0,1.0,fbm_2(noiseP*50.0)*fbm_2(noiseP*4.0))*0.02;

  return s;

}
vec4 getColor(vec3 p){

  vec3 noiseP =p*0.7;
  float n1=abs(fbm_2(noiseP*1.0));
 
  float n =smoothstep(-0.2,1.0,fbm_4(noiseP*2.0+fbm_4(noiseP*2.0)*1.5));
  vec3 base1 = mix(vec3(0.2,0,0.1),vec3(0.9,0.2,0.3),vec3(n));
  vec3 lum = vec3(0.299, 0.587, 0.114);
  vec3 gray = vec3(dot(lum, base1));
   vec4 color =vec4(0,0,0,0);
  color.xyz = mix(base1, gray, vec3(pow(n1,2.0)));
  color.w =40.0;
  float s = smoothstep(0.2,0.4,n);
  color.w -=s*20.0;
  color.xyz+=vec3(s)*vec3(0.7,0.7,0.4)*0.5;
  return color;
}

vec3 calcNormal(vec3 p) {
    vec2 e = vec2(1.0, -1.0) * 0.0005; // epsilon
    float r = 1.; // radius of sphere
    return normalize(
      e.xyy * map(p + e.xyy) +
      e.yyx * map(p + e.yyx) +
      e.yxy * map(p + e.yxy) +
      e.xxx * map(p + e.xxx));
}



float rayMarch(vec3 ro, vec3 rd, float start, float end) {
  float depth = start;

  for (int i = 0; i < 256; i++) {
    vec3 p = ro + depth * rd;
    
    float d =map(p);
    
    depth += d;
    if (d < 0.001 || depth > end) break;
  }

  return depth;
}
float calcSoftshadow( in vec3 ro, in vec3 rd, in float mint, in float tmax )
{
	float res = 1.0;
    float t = mint;
    float ph = 1e10; // big, such that y = 0 on the first iteration
    
    for( int i=0; i<16; i++ )
    {
		float h = map( ro + rd*t );

      
            // use this if you are getting artifact on the first iteration, or unroll the
            // first iteration out of the loop
            //float y = (i==0) ? 0.0 : h*h/(2.0*ph); 

            float y = h*h/(2.0*ph);
            float d = sqrt(h*h-y*y);
            res = min( res, 10.0*d/max(0.0,t-y) );
            ph = h;
       
        
        t += h;
        
        if( res<0.0001 || t>tmax ) break;
        
    }
    res = clamp( res, 0.0, 1.0 );
    return res*res*(3.0-2.0*res);
}

mat3 setCamera( in vec3 ro, in vec3 ta, float cr )
{
	vec3 cw = normalize(ta-ro);
	vec3 cp = vec3(sin(cr), cos(cr),0.0);
	vec3 cu = normalize( cross(cw,cp) );
	vec3 cv =          ( cross(cu,cw) );
    return mat3( cu, cv, cw );
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{

  float time =sin( iTime*0.2)*0.2+1.3;
  // camera	
  vec3 ta = vec3( 0.0, 0.0, 0.0 );
  vec3 ro = ta + vec3( 10.0*cos(time ), 0, 10.0*sin(time ) );
  // camera-to-world transformation
  mat3 ca = setCamera( ro, ta, 0.0 );
  vec2 p = (2.0*fragCoord-iResolution.xy)/iResolution.y;


  // focal length
  const float fl = 3.5;
        
  // ray direction
  vec3 rd = ca * normalize( vec3(p,fl) );

  vec3 col = vec3(0);


  float d = rayMarch(ro, rd, 0., 100.); 

  if (d > 100.0) 
  {
    col = vec3(0.2,0.2,0.4)*0.5*(1.0-pow(length(p)*0.5,2.0)); // ray didn't hit anything
  } 
  else 
  {
    vec3 p = ro + rd * d; // point on sphere we discovered from ray marching
    vec3 N = calcNormal(p);
    vec4 colin = getColor(p);
    vec3 albedo = colin.xyz;
    vec3 lightpos =vec3(2.0,3.0,3.0);
    vec3 L = normalize(lightpos - p);
    
    float shadow = calcSoftshadow(p,L, 0.01, 3.0);
    vec3 irr =vec3(max(0.0,dot(N,L))*2.0)*shadow+vec3(0.1,0.1,0.2);
    col =irr*albedo;
    
    vec3  ref = reflect(rd,N);            
    float fre = clamp(1.0+dot(N,rd),0.0,1.0);
    float spe = (colin.w/15.0)*pow( clamp(dot(ref,L),0.0, 1.0), colin.w )*2.0*(0.5+0.5*pow(fre,42.0));
    col += spe*shadow;
   
    col +=vec3(pow(1.0+dot(rd,N),2.0))*vec3(0.2,0.1,0.1);

  }

  // Output to screen
  fragColor = vec4(col, 1.0);
}