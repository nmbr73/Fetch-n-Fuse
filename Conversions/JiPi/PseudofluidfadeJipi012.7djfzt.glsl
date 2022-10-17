

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
//#define POINTS
void mainImage( out vec4 fragColor, in vec2 fragCoord ){
    vec2 uv = fragCoord.xy/iResolution.xy;
    fragColor = 1.0-(texture(iChannel0, uv));
    vec4 temp = fragColor;
    fragColor = smoothstep(0.0, 1.1, pow(fragColor, vec4(0.4545)));
    #ifdef POINTS
    fragColor.rgb = mix(fragColor.rgb, vec3(1,0,0), texture(iChannel1, uv).a);
    #endif
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
float hash12(vec2 p){
	vec3 p3  = fract(vec3(p.xyx) * 0.1031);
	p3 += dot(p3, p3.yzx + 9.0);
	return fract((p3.x + p3.y) * p3.z);
}

float noise(vec2 x){
    vec2 f = fract(x)*fract(x)*(3.0-2.0*fract(x));
	return mix(mix(hash12(floor(x)),
                   hash12(floor(x)+vec2(1,0)),f.x),
               mix(hash12(floor(x)+vec2(0,1)),
                   hash12(floor(x)+vec2(1)),f.x),f.y);
}

vec4 circle(vec2 uv, vec2 pos, float sz){
    // draw a circle at mouse coordinates
    float s = (3.0+0.0*pow(noise(vec2(iTime*0.5)),2.0))/sz;
    uv += pos+vec2(1.0/s);
    float val = clamp(1.0-length(s*uv-1.0), 0.0, 1.0);
    val = pow(5.0*val, 1.0);
	return vec4(clamp(val, 0.0, 1.0));
}

vec2 hash21(float p){
	vec3 p3 = fract(vec3(p) * vec3(.1031, .1030, .0973));
	p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.xx+p3.yz)*p3.zy);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord ){
	vec2 uv = fragCoord.xy / iResolution.yy;
    float brightness = 1.0;//0.6+0.4*noise(vec2(iTime*2.9));
    fragColor = circle(uv, -vec2(0.888888, 0.35), 1.0)-circle(uv, -vec2(0.888888, 0.35), 0.88);
    //fragColor = texture(iChannel0, uv)*circle(uv, -vec2(0.888888, 0.35))*clamp(1.0, 0.0, 1.0);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// simplex noise obviously not by me, see main() below
vec3 mod289(vec3 x){
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 mod289(vec4 x){
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 permute(vec4 x){
     return mod289(((x*34.0)+1.0)*x);
}

vec4 taylorInvSqrt(vec4 r){
  return 1.79284291400159 - 0.85373472095314 * r;
}

float simplex(vec3 v){
  const vec2  C = vec2(1.0/6.0, 1.0/3.0) ;
  const vec4  D = vec4(0.0, 0.5, 1.0, 2.0);

  vec3 i  = floor(v + dot(v, C.yyy) );
  vec3 x0 =   v - i + dot(i, C.xxx) ;

  vec3 g = step(x0.yzx, x0.xyz);
  vec3 l = 1.0 - g;
  vec3 i1 = min( g.xyz, l.zxy );
  vec3 i2 = max( g.xyz, l.zxy );

  vec3 x1 = x0 - i1 + C.xxx;
  vec3 x2 = x0 - i2 + C.yyy;
  vec3 x3 = x0 - D.yyy;

  i = mod289(i);
  vec4 p = permute( permute( permute(
             i.z + vec4(0.0, i1.z, i2.z, 1.0 ))
           + i.y + vec4(0.0, i1.y, i2.y, 1.0 ))
           + i.x + vec4(0.0, i1.x, i2.x, 1.0 ));

  float n_ = 0.142857142857;
  vec3  ns = n_ * D.wyz - D.xzx;

  vec4 j = p - 49.0 * floor(p * ns.z * ns.z);

  vec4 x_ = floor(j * ns.z);
  vec4 y_ = floor(j - 7.0 * x_ );

  vec4 x = x_ *ns.x + ns.yyyy;
  vec4 y = y_ *ns.x + ns.yyyy;
  vec4 h = 1.0 - abs(x) - abs(y);

  vec4 b0 = vec4( x.xy, y.xy );
  vec4 b1 = vec4( x.zw, y.zw );

  vec4 s0 = floor(b0)*2.0 + 1.0;
  vec4 s1 = floor(b1)*2.0 + 1.0;
  vec4 sh = -step(h, vec4(0.0));

  vec4 a0 = b0.xzyw + s0.xzyw*sh.xxyy ;
  vec4 a1 = b1.xzyw + s1.xzyw*sh.zzww ;

  vec3 p0 = vec3(a0.xy,h.x);
  vec3 p1 = vec3(a0.zw,h.y);
  vec3 p2 = vec3(a1.xy,h.z);
  vec3 p3 = vec3(a1.zw,h.w);

  vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
  p0 *= norm.x;
  p1 *= norm.y;
  p2 *= norm.z;
  p3 *= norm.w;

  vec4 m = max(0.6 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
  m = m * m;
  return 42.0 * dot( m*m, vec4( dot(p0,x0), dot(p1,x1),
                                dot(p2,x2), dot(p3,x3) ) );
  }

float fbm3(vec3 v) {
    float result = simplex(v);
    result += simplex(v * 2.) / 2.;
    result += simplex(v * 4.) / 4.;
    result /= (1. + 1./2. + 1./4.);
    return result;
}

vec3 snoiseVec3( vec3 x ){

  float s  = simplex(vec3( x ));
  float s1 = simplex(vec3( x.y - 19.1 , x.z + 33.4 , x.x + 47.2 ));
  float s2 = simplex(vec3( x.z + 74.2 , x.x - 124.5 , x.y + 99.4 ));
  vec3 c = vec3( s , s1 , s2 );
  return c;

}

vec3 curlNoise(vec3 p)
{
    const float e = .1;
  vec3 dx = vec3( e   , 0.0 , 0.0 );
  vec3 dy = vec3( 0.0 , e   , 0.0 );
  vec3 dz = vec3( 0.0 , 0.0 , e   );

  vec3 p_x0 = snoiseVec3( p - dx );
  vec3 p_x1 = snoiseVec3( p + dx );
  vec3 p_y0 = snoiseVec3( p - dy );
  vec3 p_y1 = snoiseVec3( p + dy );
  vec3 p_z0 = snoiseVec3( p - dz );
  vec3 p_z1 = snoiseVec3( p + dz );

  float x = p_y1.z - p_y0.z - p_z1.y + p_z0.y;
  float y = p_z1.x - p_z0.x - p_x1.z + p_x0.z;
  float z = p_x1.y - p_x0.y - p_y1.x + p_y0.x;

  //const float divisor = 1.0 / ( 2.0 * e );
  //return normalize( vec3( x , y , z ) * divisor );
  // technically incorrect but I like this better...
  return vec3( x , y , z );
}

void mainImage(out vec4 fragColor, in vec2 fragCoord){
    vec2 uv = -1.0+2.0*fragCoord.xy / iResolution.xy;
    uv.x *= iResolution.x/iResolution.y;
    uv *= 1.25;
    
    // noise seed
    vec3 v = vec3(uv, iTime*0.3);
    const float disp_freq = 0.6;
    v.xy *= disp_freq;
    // get first some density variation
    vec4 disp = 0.5*mix(vec4(0), pow(0.5+0.5*vec4(fbm3(-2.0*v+11.2)), vec4(2)), 0.05);
    v.xy /= disp_freq;
    // add to randomization coordinates
    v += disp.x*24.;
    
    // vector field ("fluid" direction)
    vec2 off =  0.25*curlNoise(vec3(v)).xy;// vec2(fbm3(v), fbm3(v+99.0));
    off.y = min(0.0, off.y-0.15);
    // maybe apply density to vector field too?
    //off /= 1.0+disp.x*10.0;
    vec2 uv2 = fragCoord.xy / iResolution.xy;
    // get "emitter" from buffer A
    fragColor += texture(iChannel0, uv2);
    // mutate previous state with field direction
    fragColor += texture(iChannel1, (0.5+0.5*(1.0*(-1.0+2.0*uv2)))+off*0.025);
    
    // disperse and output
    //fragColor = pow( 0.97*clamp(fragColor-disp, 0.0, 111.0), vec4(1.002*(1.0+disp)));
    fragColor = pow( clamp(fragColor-disp, 0.0, 1.0), vec4(1.002*(1.0+disp)));
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord ){
	vec2 uv = fragCoord.xy / iResolution.yy;
    fragColor  = mix(vec4(0), vec4(1,0,0,1), pow( (0.5+0.5*sin(uv.y*120.0)),99.));
    fragColor *= mix(vec4(0), vec4(1,0,0,1), pow( (0.5+0.5*sin(uv.x*120.0)),99.));
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
// simplex noise obviously not by me, see main() below
vec3 mod289(vec3 x){
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 mod289(vec4 x){
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 permute(vec4 x){
     return mod289(((x*34.0)+1.0)*x);
}

vec4 taylorInvSqrt(vec4 r){
  return 1.79284291400159 - 0.85373472095314 * r;
}

float simplex(vec3 v){
  const vec2  C = vec2(1.0/6.0, 1.0/3.0) ;
  const vec4  D = vec4(0.0, 0.5, 1.0, 2.0);

  vec3 i  = floor(v + dot(v, C.yyy) );
  vec3 x0 =   v - i + dot(i, C.xxx) ;

  vec3 g = step(x0.yzx, x0.xyz);
  vec3 l = 1.0 - g;
  vec3 i1 = min( g.xyz, l.zxy );
  vec3 i2 = max( g.xyz, l.zxy );

  vec3 x1 = x0 - i1 + C.xxx;
  vec3 x2 = x0 - i2 + C.yyy;
  vec3 x3 = x0 - D.yyy;

  i = mod289(i);
  vec4 p = permute( permute( permute(
             i.z + vec4(0.0, i1.z, i2.z, 1.0 ))
           + i.y + vec4(0.0, i1.y, i2.y, 1.0 ))
           + i.x + vec4(0.0, i1.x, i2.x, 1.0 ));

  float n_ = 0.142857142857;
  vec3  ns = n_ * D.wyz - D.xzx;

  vec4 j = p - 49.0 * floor(p * ns.z * ns.z);

  vec4 x_ = floor(j * ns.z);
  vec4 y_ = floor(j - 7.0 * x_ );

  vec4 x = x_ *ns.x + ns.yyyy;
  vec4 y = y_ *ns.x + ns.yyyy;
  vec4 h = 1.0 - abs(x) - abs(y);

  vec4 b0 = vec4( x.xy, y.xy );
  vec4 b1 = vec4( x.zw, y.zw );

  vec4 s0 = floor(b0)*2.0 + 1.0;
  vec4 s1 = floor(b1)*2.0 + 1.0;
  vec4 sh = -step(h, vec4(0.0));

  vec4 a0 = b0.xzyw + s0.xzyw*sh.xxyy ;
  vec4 a1 = b1.xzyw + s1.xzyw*sh.zzww ;

  vec3 p0 = vec3(a0.xy,h.x);
  vec3 p1 = vec3(a0.zw,h.y);
  vec3 p2 = vec3(a1.xy,h.z);
  vec3 p3 = vec3(a1.zw,h.w);

  vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
  p0 *= norm.x;
  p1 *= norm.y;
  p2 *= norm.z;
  p3 *= norm.w;

  vec4 m = max(0.6 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
  m = m * m;
  return 42.0 * dot( m*m, vec4( dot(p0,x0), dot(p1,x1),
                                dot(p2,x2), dot(p3,x3) ) );
  }

float fbm3(vec3 v) {
    float result = simplex(v);
    result += simplex(v * 2.) / 2.;
    result += simplex(v * 4.) / 4.;
    result /= (1. + 1./2. + 1./4.);
    return result;
}

vec3 snoiseVec3( vec3 x ){

  float s  = simplex(vec3( x ));
  float s1 = simplex(vec3( x.y - 19.1 , x.z + 33.4 , x.x + 47.2 ));
  float s2 = simplex(vec3( x.z + 74.2 , x.x - 124.5 , x.y + 99.4 ));
  vec3 c = vec3( s , s1 , s2 );
  return c;

}

vec3 curlNoise(vec3 p)
{
    const float e = .1;
  vec3 dx = vec3( e   , 0.0 , 0.0 );
  vec3 dy = vec3( 0.0 , e   , 0.0 );
  vec3 dz = vec3( 0.0 , 0.0 , e   );

  vec3 p_x0 = snoiseVec3( p - dx );
  vec3 p_x1 = snoiseVec3( p + dx );
  vec3 p_y0 = snoiseVec3( p - dy );
  vec3 p_y1 = snoiseVec3( p + dy );
  vec3 p_z0 = snoiseVec3( p - dz );
  vec3 p_z1 = snoiseVec3( p + dz );

  float x = p_y1.z - p_y0.z - p_z1.y + p_z0.y;
  float y = p_z1.x - p_z0.x - p_x1.z + p_x0.z;
  float z = p_x1.y - p_x0.y - p_y1.x + p_y0.x;

  //const float divisor = 1.0 / ( 2.0 * e );
  //return normalize( vec3( x , y , z ) * divisor );
  // technically incorrect but I like this better...
  return vec3( x , y , z );
}

void mainImage(out vec4 fragColor, in vec2 fragCoord){
    vec2 uv = -1.0+2.0*fragCoord.xy / iResolution.xy;
    uv.x *= iResolution.x/iResolution.y;
    uv *= 1.25;
    
    // noise seed
    vec3 v = vec3(uv, iTime*0.3);
    const float disp_freq = 0.6;
    v.xy *= disp_freq;
    // get first some density variation
    vec4 disp = 0.5*mix(vec4(0), pow(0.5+0.5*vec4(fbm3(-2.0*v+11.2)), vec4(2)), 0.05);
    v.xy /= disp_freq;
    // add to randomization coordinates
    v += disp.x*24.;
    
    // vector field ("fluid" direction)
    vec2 off =  0.25*curlNoise(vec3(v)).xy;// vec2(fbm3(v), fbm3(v+99.0));
    off.y = min(0.0, off.y-0.15);
    // maybe apply density to vector field too?
    //off /= 1.0+disp.x*10.0;
    vec2 uv2 = fragCoord.xy / iResolution.xy;
    // mutate previous state with field direction
    fragColor = texture(iChannel0, (0.5+0.5*(1.0*(-1.0+2.0*uv2)))+off*0.025);
    
    
}