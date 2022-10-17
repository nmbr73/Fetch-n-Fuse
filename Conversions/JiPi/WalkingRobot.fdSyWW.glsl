

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;
    float vign = pow( 16.0*uv.x*uv.y*(1.0-uv.x)*(1.0-uv.y), 0.8 );
    
    vec3 col = texture(iChannel0, uv).rgb;
    
    col = col*col*(3.0 - 2.0*col);
    
    col += 0.01*step(0.3, texture(iChannel1, uv*2.0).r * (1.0 - vign));
    
    col = 1.0 - exp(-0.5*col);
    
    col = pow(col, vec3(1.0/2.2));
    
    col *= 0.3 + 0.7*vign;
    fragColor = vec4(col, 1.0);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
#define PI 3.14159265359
#define AA 1
const float leg_segment = 0.25; // length of the leg / 3



//globals
struct leg {vec3 p1, p2, p3, p4;};
leg legs[4];
mat3x3 rotX;
mat3x3 rotZ;
vec3 center;
float inside = 1.0;

//random////////////////////////////////////////////////////////////////////////////////
uint seed = 0u;
void hash(){
    seed ^= 2747636419u;
    seed *= 2654435769u;
    seed ^= seed >> 16;
    seed *= 2654435769u;
    seed ^= seed >> 16;
    seed *= 2654435769u;
}
void initRandomGenerator(vec2 fragCoord){
    seed = uint(fragCoord.y*iResolution.x + fragCoord.x)+uint(iFrame)*uint(iResolution.x)*uint(iResolution.y);
}

float random(){
    hash();
    return float(seed)/4294967295.0;
}

float smoothNoise(float time){
    seed = (uint(time)+45456u) * 23456u;
    float a = random();
    seed = (uint(time)+45457u) * 23456u;
    float b = random();
    float t = fract(time);
    return mix(a, b, t*t*(3.0 - 2.0*t));
    
}

float timeFbm(float time, int depth){
    float a = 0.0;
    float b = 1.0;
    float t = 0.0;
    for(int i = 0; i < depth; i++){
        float n = smoothNoise(time);
        a += b*n;
        t += b;
        b *= 0.6;
        time *= 2.0; 
    }
    return a/t;
}
///////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////

float sdBox( vec3 p, float size){
  return max(max(abs(p.x), abs(p.y)), abs(p.z)) - size;
}

// http://iquilezles.org/www/articles/smin/smin.htm
vec2 smin( vec2 a, vec2 b, float k )
{
    float h = clamp( 0.5+0.5*(b.x-a.x)/k, 0.0, 1.0 );
    return mix( b, a, h ) - k*h*(1.0-h);
}


//https://www.shadertoy.com/view/Xds3zN
float dot2(vec3 v) {return dot(v, v);}
float sdRoundCone(vec3 p, vec3 a, vec3 b, float r1, float r2)
{
  //return length(p - b)  - 0.05;
  // sampling independent computations (only depend on shape)
  vec3  ba = b - a;
  float l2 = dot(ba,ba);
  float rr = r1 - r2;
  float a2 = l2 - rr*rr;
  float il2 = 1.0/l2;
    
  // sampling dependant computations
  vec3 pa = p - a;
  float y = dot(pa,ba);
  float z = y - l2;
  float x2 = dot2( pa*l2 - ba*y );
  float y2 = y*y*l2;
  float z2 = z*z*l2;

  // single square root!
  float k = sign(rr)*rr*rr*x2;
  if( sign(z)*a2*z2>k ) return  sqrt(x2 + z2)        *il2 - r2;
  if( sign(y)*a2*y2<k ) return  sqrt(x2 + y2)        *il2 - r1;
                        return (sqrt(x2*a2*il2)+y*rr)*il2 - r1;
}



//animation///////////////////////////////////////////////////////////////////////////
vec2 walking_cycle(float t){
    return (t < 0.5) ? vec2(-1.0 + 4.0*t, sin(2.0*PI*t)) : vec2(3.0 - 4.0*t, 0.0);
} 


void inverse_kinematic(in vec3 p1, out vec3 p2, out vec3 p3, inout vec3 p4){
    float d = distance(p1, p4);
    vec3 u = (p4 - p1)/d;
    
    if (d > leg_segment * 3.0){
        p2 = p1 + u * leg_segment;
        p3 = p1 + 2.0 * u * leg_segment;
        p4 = p1 + 3.0 * u * leg_segment;
        return;
    }
    
    float a = (d - leg_segment) * 0.5;
    float h = sqrt(leg_segment*leg_segment - a*a);
    
    vec3 b1 = cross(u, vec3(0.0, 1.0, 0.0));
    vec3 up = normalize(cross(u, b1));
    
    p2 = p1 + u*a + h* up * sign(up.y);
    p3 = p2 + u*leg_segment;
    
    return; 
}

void compute_leg_pos(float t){
    center =  vec3(timeFbm(t, 3)*0.2 - 0.1, 
                   timeFbm(t + 10.0, 4)*0.2 - 0.1 + 0.05*sin(4.0*PI*t), 
                   timeFbm(t + 20.0, 3)*0.2 - 0.1);
    
    float angleX = 0.3*(timeFbm(t*4.0 + 30.0, 2)*2.0 - 1.0);
    float angleZ = 0.3*(timeFbm(t*4.0 + 30.0, 2)*2.0 - 1.0);
    
    rotX = mat3x3(1.0, 0.0, 0.0,
                  0.0, cos(angleX), -sin(angleX),
                  0.0, sin(angleX), cos(angleX));
    
    rotZ = mat3x3( cos(angleZ), -sin(angleZ),   0.0,
                   sin(angleZ),  cos(angleZ),   0.0,
                       0.0    ,      0.0    ,   1.0   );
    
    for(int i=0; i < 4; i++){
        float i_f = float(i);
        float angle = 0.5*PI*i_f;
        vec3 direction = vec3(cos(angle), 0.0, sin(angle));
        
        legs[i].p1 = (center + rotZ * rotX * direction * 0.2);
        legs[i].p4 = vec3(0.2, 0.1, 0.1)*walking_cycle(fract(t + 0.5*i_f)).xyx 
                    + vec3(0.0, -0.4, 0.0) + 0.53 * direction;
        
        inverse_kinematic(legs[i].p1, legs[i].p2, legs[i].p3, legs[i].p4);
    }
    
    rotX = inverse(rotX);
    rotZ = inverse(rotZ);
    
    return;
}

//render//////////////////////////////////////////////////////////////////////////////

float map(vec3 p, out int matID){
    float d = 1e5;
    float d2;
    matID = 0;
     for(int i=0; i < 4; i++){
         d = min(d, sdRoundCone(p, legs[i].p1, legs[i].p2, 0.1, 0.06));
        d = min(d, sdRoundCone(p, legs[i].p2, legs[i].p3, 0.06, 0.03));
        d = min(d, sdRoundCone(p, legs[i].p3, legs[i].p4, 0.03, 0.001));
     
    }
     
    vec3 p2 = rotZ * rotX * p;
    d = min(d,  sdBox(p2 - center, 0.17));
    
    if (d > p.y + 0.4){ //plane
        d = p.y + 0.4;
        matID = 2;
    }
    
    d *= inside;
    
    d2 = 1e5;
    for(int i=0; i < 4; i++){
        d2 = min(d2, length(p - legs[i].p1) - 0.13);
    }
    
    if (d > d2){ 
        d = d2;
        matID = 1;
    }
    
    return d;
}

float map(vec3 p){
    int i_;
    return map(p, i_);
}



#define OFFSET 0.0005 
vec3 normal(vec3 p){
    return normalize(vec3(map(p+vec3(OFFSET,0,0))-map(p-vec3(OFFSET,0,0)),
                          map(p+vec3(0,OFFSET,0))-map(p-vec3(0,OFFSET,0)),
                          map(p+vec3(0,0,OFFSET))-map(p-vec3(0,0,OFFSET)))); 
}

float softshadow( in vec3 ro, in vec3 rd){
    float res = 1.0;
    float tmax = 12.0;  
    float t = OFFSET*4.0;
    
    for( int i=0; i<100; i++ )
    {
		float h = map( ro + rd*t);
        res = min(res, 16.0*h/t);
        t += h;
        if( h<OFFSET ) return 0.0;
        if( t>tmax ) break;
    }
    return clamp(res, 0.0, 1.0);
}

float distance_scene(vec3 ro, vec3 rd, out int matID){
    float t = 0.02;
    float dt;
    
    for(int i=0; i < 100; i++){
        dt = map(ro + t*rd, matID);
        t += dt;
        
        if(abs(dt) < OFFSET)
            return t;
        
        if(t > 10.0)
            return -1.0;
        
    }
    return -1.0;
}

float fresnel(vec3 rd, vec3 n){
    float n2 = 1.0, n1 = 1.330;
    float R0 = pow((n2 - n1)/(n2 + n1), 2.0);
    float teta = -dot(n, rd);
    float R = R0 + (1.0 - R0)*pow(1.0 - teta, 5.0);
    return R;
}

vec3 march(vec3 ro, vec3 rd){
    float t;
    vec3 n, col, mask, p;
    int matID;
    
    const vec3 sun_dir = normalize(vec3(-0.8, 1.0, 0.5));
    
    inside = 1.0;
    mask = vec3(1.0);
    col = vec3(0.0);
    
    vec3 bgCol = vec3(0.0);
    
    for(int i = 0; i < 6; i++){
        t = distance_scene(ro, rd, matID);
        if(t < 0.0) return col + mask * bgCol;
        p = ro + t*rd;
        n = normal(p); 
        ro = p;
        
        if (matID == 0){ //glass
            float f = fresnel(rd, n);
                vec3 rd2 = reflect(rd, n);
                float s = softshadow(p, sun_dir);
                
                
                if(dot(n, sun_dir) > 0.0){
                    vec3 sky = mix(vec3(0.5, 0.4, 0.4), vec3(vec3(2.0)), 0.5*n.y + 0.5);
                    col += s*f*mask*(sky + 70.0*pow(max(dot(rd2, sun_dir), 0.0), 30.0));
                }
                mask *= 1.0 - f;
                rd = refract(rd, n, 1.0/1.33);
                inside *= -1.0;
                mask *= vec3(1.0, 0.85, 0.85);
            
            
        }
        
        if(matID == 1){ //cube
            mask *= vec3(1.0, 0.1, 0.1);
            col += mask *( vec3(0.1, 0.1, 0.2) * (map(p + n*0.02) / 0.02) * (min(1.0, 1.0 + dot(sun_dir, n)))
                      + 1.0*vec3(1.30,1.00,0.70) * dot(sun_dir, n) * softshadow(p, sun_dir)
                        );
            
            return col;
        }
        
        if(matID == 2){ //plane
            vec3 tex = texture(iChannel1, p.xz*0.2 + 0.8*vec2(0.2, 0.1)*iTime).rgb;
            mask *= tex*tex;
            mask *= exp(-dot(p.xz + vec2(-0.5, 0.5), p.xz + vec2(-0.5, 0.5)));
            col += mask *( vec3(0.1, 0.1, 0.2) * (min(1.0, 1.0 + dot(sun_dir, n)))
                      + 1.0*vec3(1.30,1.00,0.70) * dot(sun_dir, n) * softshadow(p, sun_dir)
                        );
            return col;
        }

    }
    return col;
}



void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    compute_leg_pos(iTime);
    initRandomGenerator(fragCoord);
    
    vec2 uv = fragCoord/iResolution.xy*2.0-1.0;
    uv.x *= iResolution.x/iResolution.y;
    
    vec3 ro = vec3(sin(0.2), 0.5, cos(0.2));
    
    vec3 dir0 = normalize(-ro);
    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, dir0));
    up = cross(dir0, right);
    vec3 rd;
    
    vec3 col = vec3(0.0);
    for(int x = 0; x < AA; x++){
    for(int y = 0; y < AA; y++){
        rd = normalize(dir0 + right*(uv.x + float(x)/float(AA)/iResolution.x) 
                            + up*(uv.y + float(y)/float(AA)/iResolution.x));
        col += march(ro, rd);
    }
    }
    col /= float(AA*AA);
    
    fragColor = vec4(col, 1.0);
}