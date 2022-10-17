

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
//cloud marching function from shader "Cloudy Shapes" by kaneta: https://www.shadertoy.com/view/WdXGRj
//soft shadows function by iq in shader "Soft Shadow Variation.": https://www.shadertoy.com/view/lsKcDD
//cosine color palette function courtesy of iq

float hash( float n )
{
    return fract(sin(n)*43758.5453);
}
vec3 pal( in float t, in vec3 a, in vec3 b, in vec3 c, in vec3 d )
{
    return a + b*cos( 6.28318*(c*t+d) );
}
vec3 rot_x(in vec3 v, in float theta) {
    mat3 rotx = mat3(1.0, 0.0, 0.0, 0.0, cos(theta), sin(theta), 0.0, -sin(theta), cos(theta));
    return rotx * v;
}
vec3 rot_y(in vec3 v, in float theta) {
    mat3 roty = mat3(cos(theta), 0.0, -sin(theta), 0.0, 1.0, 0.0, sin(theta), 0.0, cos(theta));
    return roty * v;
}
vec3 rot_z(in vec3 v, in float theta) {
    mat3 roty = mat3(cos(theta), sin(theta), 0.0, -sin(theta), cos(theta), 0.0, 0.0, 0.0, 1.0);
    return roty * v;
}
vec3 computeColor( float density, float radius )
{
	// color based on density alone, gives impression of occlusion within
	// the media
	vec3 result = mix( vec3(1.0,0.9,0.8), vec3(0.4,0.15,0.1), density );
	
	// color added to the media
	vec3 colCenter = 7.*vec3(0.8,1.0,1.0);
	vec3 colEdge = 1.5*vec3(0.48,0.53,0.5);
	result *= mix( colCenter, colEdge, min( (radius+.05)/.9, 1.15 ) );
	
	return result;
}
float noise(
	in vec3 x
){
	vec3 p = floor(x);
	vec3 f = fract(x);
	f = f*f*(3.0 - 2.0*f);
    float n = p.x + p.y*157.0 + 113.0*p.z;
    return mix(mix(mix( hash(n+  0.0), hash(n+  1.0),f.x),
                   mix( hash(n+157.0), hash(n+158.0),f.x),f.y),
               mix(mix( hash(n+113.0), hash(n+114.0),f.x),
                   mix( hash(n+270.0), hash(n+271.0),f.x),f.y),f.z);
}
const mat3 m3  = mat3( 0.00,  0.80,  0.60,
                      -0.80,  0.36, -0.48,
                      -0.60, -0.48,  0.64 );
float fbm (in vec3 p, in int o)
{
    float f = 0.0;
    float freq = 1.0;
    for (int i = 0; i < o; i++)
    {
        float n = noise(p * freq) / freq;
        f += n;
        freq *= 2.0;
        p = m3 * p;
    }
    return f;
}
float rmf(vec3 p)
{
    float signal = 0.0;
    float value  = 0.0;
    float weight = 1.0;
    float h = 1.0;
    float f = 1.0;

    for (int i=0; i < 4; i++) 
    {
        signal = noise(p)*2.0-0.4;
        signal = pow(1.0 - abs(signal), 2.0) * weight;
        weight = clamp(0.0, 1.0, signal * 16.0);
        value += (signal * pow(f, -1.0));
        f *= 2.0;
        p *= 2.0;
    }
    
    return (value * 1.25) - 1.0;
}

  
float sdSphere(in vec3 p, in vec3 c, in float r) {
    p = rot_x(p, iTime * 2.0);
    return length(p - c) - (r + fbm(p * 1.35, 4));
}

float sdTorus( vec3 p, vec2 t )
{
  vec2 q = vec2(length(p.xz)-t.x,p.y);
  return length(q)-t.y;
}
vec2 map(in vec3 p){
    float t = iTime * 0.125;
    vec3 p_rz = rot_z(p, iTime * .25);
    vec3 p_rx = rot_x(p, iTime * 0.125);
    vec2 t1 = vec2(1.- sdTorus(p_rz, vec2(6.0, 0.5)) + fbm(t+p_rz+fbm(p, 2)*3.5, 4)*.75, 1.0);
    vec2 t2 = vec2(1.- sdTorus(p_rx, vec2(12.0, 1.0)) +rmf(t+p_rx*1.25*(fbm(t+p_rx, 4)*0.625))*.5, 2.0);
    vec2 s1 = vec2(1.-sdSphere(p, vec3(0), 0.5), 3.0);
    
    if (t1.x > t2.x && t1.x > s1.x) {
        return t1;
    } else if (t2.x > s1.x) {
        return t2;
    } else {
        return s1;
    }
}

float map_2(in vec3 p){
    float t = iTime * 0.125;
    vec3 p_rz = rot_z(p, iTime * .25);
    vec3 p_rx = rot_x(p, iTime * 0.125);
    float t1 = sdTorus(p_rz, vec2(6.0, 0.5)) + fbm(t+p_rz+fbm(p, 1)*3.5, 2)*.75;
    float t2 = sdTorus(p_rx, vec2(12.0, 1.0)) +rmf(t+p_rx*1.25*(fbm(t+p_rx, 2)*0.625))*0.5;
    float s1 = sdSphere(p, vec3(0), 0.5);
    
    return min(t1, min(t2, s1));
}
float calcSoftshadow( in vec3 ro, in vec3 rd, in float mint, in float tmax, int technique )
{
	float res = 1.0;
    float t = mint;
    float ph = 1e10; // big, such that y = 0 on the first iteration
    
    for( int i=0; i<16; i++ )
    {
		float h = map_2( ro + rd*t );

        // traditional technique
        if( technique==0 )
        {
        	res = min( res, 10.0*h/t );
        }
        // improved technique
        else
        {
            // use this if you are getting artifact on the first iteration, or unroll the
            // first iteration out of the loop
            //float y = (i==0) ? 0.0 : h*h/(2.0*ph); 

            float y = h*h/(2.0*ph);
            float d = sqrt(h*h-y*y);
            res = min( res, 10.0*d/max(0.0,t-y) );
            ph = h;
        }
        
        t += h;
        
        if( res<0.0001 || t>tmax ) break;
        
    }
    res = clamp( res, 0.0, 1.0 );
    return res*res*(3.0-2.0*res);
}
    
//fixed value for now
float jitter;

#define MAX_STEPS 35
#define SHADOW_STEPS 6
#define VOLUME_LENGTH 50.
#define SHADOW_LENGTH 2.
vec4 cloudMarch(vec3 p, vec3 ray)
{
    float density = 0.;

    float stepLength = VOLUME_LENGTH / float(MAX_STEPS);
    float shadowStepLength = SHADOW_LENGTH / float(SHADOW_STEPS);
    vec3 light = normalize(vec3(-1.0, 2.0, 1.0));

    vec4 sum = vec4(0., 0., 0., 1.);
    
    vec3 pos = p + ray * jitter * stepLength;
    
    for (int i = 0; i < MAX_STEPS; i++)
    {
        if (abs(sum.a) < 0.1) {
        	break;
        }
        vec2 res_i = map(pos);
        float d = res_i.x;
        if(d> 0.001)
        {
            float mat_i = res_i.y;
            vec3 lpos = pos + light * jitter * shadowStepLength;
            float shadow = 0.;
    
            for (int s = 0; s < SHADOW_STEPS; s++)
            {
                lpos += light * shadowStepLength;
                vec2 res_s = map(lpos);
                float lsample = res_s.x;
                shadow += lsample;
            }
    
            density = clamp((d / float(MAX_STEPS)) * 20.0, 0.0, 1.0);
            float s = exp((-shadow / float(SHADOW_STEPS)) * 3.0);
            sum.rgb += vec3(s * density) * vec3(1.1, 0.9, .5) * sum.a;
            sum.a *= 1.-density;
            vec3 pa_col = vec3(0.0);
            float ss = 1.0;
            if (mat_i == 1.0){
                pa_col = pal( density * 0.5, vec3(0.5,0.5,0.5),vec3(0.5,0.5,0.5),vec3(2.0,1.0,0.0),vec3(0.5,0.20,0.25) );
                ss = clamp(calcSoftshadow(pos, light, 0.01, 50.0, 0)+0.5, 0.0, 1.0);
            } else if (mat_i == 2.0) {
                pa_col = pal( density*2.0, vec3(0.5,0.5,0.5),vec3(0.5,0.5,0.5),vec3(1.0,1.0,1.0),vec3(0.0,0.10,0.20) );
            } else {
                pa_col = pal( density *8.0 , vec3(0.5,0.5,0.5),vec3(0.5,0.5,0.5),vec3(1.0,0.7,0.4),vec3(0.0,0.15,0.20) ) * 3.0;
            }
            vec2 res_m = map(pos + vec3(0,0.25,0.0));
            sum.rgb += exp(-res_m.x * .2) *pa_col * sum.a;
            sum *= ss;
        }
        pos += ray * stepLength;
    }

    return sum;
}

vec3 camera(in vec2 uv, in vec3 ro, vec3 ta, float fd){
    vec3 up = vec3(0,1,0); 
    vec3 ww = normalize(ta-ro); 
    vec3 uu = normalize(cross(ww, up)); 
    vec3 vv = normalize(cross(uu, ww)); 
    
    vec3 rd = normalize(uv.x*uu + uv.y*vv + fd*ww);
    return rd;
}
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;
    uv -= vec2(0.5);
    uv.x *= iResolution.x/iResolution.y;
   // jitter = hash(uv.x + uv.y * 57.0 + iTime) * 0.0125;
    vec3 li = normalize(vec3(0.5, .8, 3.0));
    float a = 10.0 * iMouse.x/iResolution.x;
    vec3 ro = 1.35*vec3( 14.0 * sin(a), 13.0, 14.0* cos(a));
    vec3 ta = vec3(0,0,0);   
    vec3 rd = camera(uv, ro, ta, 1.0);
    fragColor = pow(cloudMarch(ro, rd), vec4(2.15)) * 0.15;
}