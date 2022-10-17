

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Fork of "Random slime mold generator" by michael0884. https://shadertoy.com/view/ttsfWn
// 2020-08-19 23:28:40

// Fork of "Everflow" by michael0884. https://shadertoy.com/view/ttBcWm
// 2020-07-19 18:18:22

// Fork of "Paint streams" by michael0884. https://shadertoy.com/view/WtfyDj
// 2020-07-11 22:38:47

//3d mode
//#define heightmap

vec3 hsv2rgb( in vec3 c )
{
    vec3 rgb = clamp( abs(mod(c.x*6.0+vec3(0.0,4.0,2.0),6.0)-3.0)-1.0, 0.0, 1.0 );

	rgb = rgb*rgb*(3.0-2.0*rgb); // cubic smoothing	

	return c.z * mix( vec3(1.0), rgb, c.y);
}

#define FOV 1.5
#define RAD R.x*0.7

float gauss(float x, float r)
{
    x/=r;
    return exp(-x*x);
}

float sdSegment( in vec2 p, in vec2 a, in vec2 b )
{
    vec2 pa = p-a, ba = b-a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
    return length( pa - ba*h );
}

float sdSphere( vec3 p, float s )
{
  return length(p)-s;
}

float sdBox( vec3 p, vec3 b )
{
  vec3 q = abs(p) - b;
  return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
}

float rho(vec3 pos)
{
    pos.xy += R*0.5;
 	pos.xy = mod(pos.xy, R-1.);
    float de = 1e10;
    vec4 v = P(pos.xy);
    return v.z;
}

float DE(vec3 pos)
{
    float y = 4.*tanh(0.5*rho(pos));  
    
    pos.xy += R*0.5;
 	pos.xy = mod(pos.xy, R-1.);
    float de = 1e10;
    de = min(de, 0.7*sdBox(pos - vec3(R, 4.*y)*0.5, vec3(R*0.51, 3.)));
    return de;
}


vec4 calcNormal(vec3 p, float dx) {
	const vec3 K = vec3(1,-1,0);
	return  (K.xyyx*DE(p + K.xyy*dx) +
			 K.yyxx*DE(p + K.yyx*dx) +
			 K.yxyx*DE(p + K.yxy*dx) +
			 K.xxxx*DE(p + K.xxx*dx))/vec4(4.*dx,4.*dx,4.*dx,4.);
}

#define marches 70.
#define min_d 1.
vec4 ray_march(vec3 p, vec3 r)
{
    float d;
    for(float i = 0.; i < marches; i++)
    {
        d = DE(p); 
        p += r*d;
        if(d < min_d || d > R.x) break;
    }
    return vec4(p, d);
}

void mainImage( out vec4 col, in vec2 pos )
{    
    #ifdef heightmap
        // Normalized pixel coordinates 
        pos = (pos - R*0.5)/max(R.x,R.y);

        vec2 uv = iMouse.xy/R;
        vec2 angles = vec2(0.5, -0.5)*PI;

        vec3 camera_z = vec3(cos(angles.x)*cos(angles.y),sin(angles.x)*cos(angles.y),sin(angles.y));
        vec3 camera_x = normalize(vec3(cos(angles.x+PI*0.5), sin(angles.x+PI*0.5),0.)); 
        vec3 camera_y = -normalize(cross(camera_x,camera_z));

        //tracking particle
        vec4 fp = vec4(R*0.5 + 0.*vec2(150.*iTime, 0.), 0., 0.);

        vec3 ray = normalize(camera_z + FOV*(pos.x*camera_x + pos.y*camera_y));
        vec3 cam_pos = vec3(fp.xy-R*0.5, 0.) - RAD*vec3(cos(angles.x)*cos(angles.y),sin(angles.x)*cos(angles.y),sin(angles.y));

        vec4 X = ray_march(cam_pos, ray);

        if(X.w < min_d)
        {

            float D = rho(X.xyz);
            vec3 albedo = vec3(1,0.3,0.3) + sin(1.*vec3(1.,0.2,0.1)*D);

            vec4 N0 = calcNormal(X.xyz, 2.*X.w)*vec4(1.,1.,1.,1.);
            vec3 n = normalize(N0.xyz);
            vec3 rd = reflect(ray, n);
            vec3 colA =texture(iChannel2,  rd.yzx).xyz;
            vec3 colB = 0.6*(vec3(0.5) + 0.5*dot(rd, normalize(vec3(1.))));
            colB += 3.*pow(max(dot(rd, normalize(vec3(1.))), 0.), 10.);
            colB += 3.*pow(max(dot(rd, normalize(vec3(-1,-0.5,0.8))), 0.), 10.);
            float b = clamp(0.5 + 0.5*dot(n, normalize(vec3(1,1,1))), 0.,1.);
            float K = 1. - pow(max(dot(n,rd),0.), 4.);
            col.xyz = 1.*albedo*colB + 0.3*colA*K;
        }
        else
        {    
            //background
            col = 1.*texture(iChannel2,  ray.yzx);
        }
    col = tanh(1.3*col*col);
    #else
    	float r = P(pos.xy).z;
    
    	col.xyz = 3.*sin(0.1*vec3(3,0.9,0.4)*r);
    #endif
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define T(p) texelFetch(iChannel0, ivec2(mod(p,R)), 0)
#define P(p) texture(iChannel0, mod(p,R)/R)
#define C(p) texture(iChannel1, mod(p,R)/R)

#define PI 3.14159265
#define dt 1.
#define R iResolution.xy


float hash11(float p)
{
    p = fract(p * .1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

#define rand_interval 250
#define random_gen(a, b, seed) ((a) + ((b)-(a))*hash11(seed + float(iFrame/rand_interval)))


#define distribution_size 1.1

/* FIRE
//mold stuff 
#define sense_num 6
#define sense_ang 1.
#define sense_dis 420.
#define sense_oscil 0.1
#define oscil_scale 1.
#define oscil_pow 2.
#define sense_force -0.35
#define distance_scale 0.2
#define force_scale 1.
#define trailing 0.
#define acceleration 0.
*/

#define sense_num 6
#define sense_ang 1.
#define sense_dis 150.
#define sense_oscil 0.1
#define oscil_scale 1.
#define oscil_pow 1.
#define sense_force -0.35
#define distance_scale 0.2
#define force_scale 1.
#define trailing 0.
#define acceleration 0.


//SPH pressure
#define Pressure(rho) 0.5*rho
#define fluid_rho 0.2

//useful functions
#define GS(x) exp(-dot(x,x))
#define GS0(x) exp(-length(x))
#define Dir(ang) vec2(cos(ang), sin(ang))
#define Rot(ang) mat2(cos(ang), sin(ang), -sin(ang), cos(ang))
#define loop(i,x) for(int i = 0; i < x; i++)
#define range(i,a,b) for(int i = a; i <= b; i++)

//data packing
#define PACK(X) ( uint(round(65534.0*clamp(0.5*X.x+0.5, 0., 1.))) + \
           65535u*uint(round(65534.0*clamp(0.5*X.y+0.5, 0., 1.))) )   
               
#define UNPACK(X) (clamp(vec2(X%65535u, X/65535u)/65534.0, 0.,1.)*2.0 - 1.0)              

#define DECODE(X) UNPACK(floatBitsToUint(X))
#define ENCODE(X) uintBitsToFloat(PACK(X))
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
const int KEY_SPACE = 32;
bool isKeyPressed(int KEY)
{
	return texelFetch( iChannel3, ivec2(KEY,0), 0 ).x > 0.5;
}

void mainImage( out vec4 U, in vec2 pos )
{
    ivec2 p = ivec2(pos);
    
    vec2 X = vec2(0);
    vec2 V = vec2(0);
    float M = 0.;
    
    //basically integral over all updated neighbor distributions
    //that fall inside of this pixel
    //this makes the tracking conservative
    range(i, -2, 2) range(j, -2, 2)
    {
        vec2 tpos = pos + vec2(i,j);
        vec4 data = T(tpos);
       
        vec2 X0 = DECODE(data.x) + tpos;
    	vec2 V0 = DECODE(data.y);
    	vec2 M0 = data.zw;
       
        X0 += V0*dt; //integrate position

        //particle distribution size
        float K = distribution_size;
        
        vec4 aabbX = vec4(max(pos - 0.5, X0 - K*0.5), min(pos + 0.5, X0 + K*0.5)); //overlap aabb
        vec2 center = 0.5*(aabbX.xy + aabbX.zw); //center of mass
        vec2 size = max(aabbX.zw - aabbX.xy, 0.); //only positive
        
        //the deposited mass into this cell
        float m = M0.x*size.x*size.y/(K*K); 
        
        //add weighted by mass
        X += center*m;
        V += V0*m;
      
        //add mass
        M += m;
    }
    
    //normalization
    if(M != 0.)
    {
        X /= M;
        V /= M;
    }
    
    //initial condition
    if(iFrame < 1 || isKeyPressed(KEY_SPACE))
    {
        X = pos;
        V = vec2(0.);
        M = 0.3*GS(-pos/R);
    }
    
    X = clamp(X - pos, vec2(-0.5), vec2(0.5));
    U = vec4(ENCODE(X), ENCODE(V), M, 0.);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
void mainImage( out vec4 U, in vec2 pos )
{
    vec2 uv = pos/R;
    ivec2 p = ivec2(pos);
        
    vec4 data = T(pos); 
    vec2 X = DECODE(data.x) + pos;
    vec2 V = DECODE(data.y);
    float M = data.z;
    
    if(M != 0.) //not vacuum
    {
        //Compute the SPH force
        vec2 F = vec2(0.);
        vec3 avgV = vec3(0.);
        range(i, -2, 2) range(j, -2, 2)
        {
            vec2 tpos = pos + vec2(i,j);
            vec4 data = T(tpos);

            vec2 X0 = DECODE(data.x) + tpos;
            vec2 V0 = DECODE(data.y);
            float M0 = data.z;
            vec2 dx = X0 - X;
            
            float avgP = 0.5*M0*(Pressure(M) + Pressure(M0)); 
            F -= 0.5*GS(1.*dx)*avgP*dx;
            avgV += M0*GS(1.*dx)*vec3(V0,1.);
        }
        avgV.xy /= avgV.z;

        float ang = atan(V.y, V.x);
        float dang = sense_ang*PI/float(sense_num);
        vec2 slimeF = vec2(0.);
        //slime mold sensors
        range(i, -sense_num, sense_num)
        {
            float cang = ang + float(i) * dang;
        	vec2 dir = (1. + sense_dis*pow(M, distance_scale))*Dir(cang);
        	vec3 s0 = C(X + dir).xyz;  
   			float fs = pow(s0.z, force_scale);
            float os = oscil_scale*pow(s0.z - M, oscil_pow);
        	slimeF +=  sense_oscil*Rot(os)*s0.xy 
                     + sense_force*Dir(ang + sign(float(i))*PI*0.5)*fs; 
        }
        
        //remove acceleration component and leave rotation
        slimeF -= dot(slimeF, normalize(V))*normalize(V);
		F += slimeF/float(2*sense_num);
        
        if(iMouse.z > 0.)
        {
            vec2 dx= pos - iMouse.xy;
             F += 0.6*dx*GS(dx/20.);
        }
        
        //integrate velocity
        V += F*dt/M;
        
        //acceleration for fun effects
        V *= 1. + acceleration;
        
        //velocity limit
        float v = length(V);
        V /= (v > 1.)?1.*v:1.;
    }
    
    //mass decay
   // M *= 0.999;
    
    //input
    //if(iMouse.z > 0.)
    //\\	M = mix(M, 0.5, GS((pos - iMouse.xy)/13.));
    //else
     //   M = mix(M, 0.5, GS((pos - R*0.5)/13.));
    
    //save
    X = clamp(X - pos, vec2(-0.5), vec2(0.5));
    U = vec4(ENCODE(X), ENCODE(V), M, 0.);
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
#define radius 1.

void mainImage( out vec4 fragColor, in vec2 pos )
{
    float rho = 0.001;
    vec2 vel = vec2(0., 0.);

    //compute the smoothed density and velocity
    range(i, -2, 2) range(j, -2, 2)
    {
        vec2 tpos = pos + vec2(i,j);
        vec4 data = T(tpos);

        vec2 X0 = DECODE(data.x) + tpos;
        vec2 V0 = DECODE(data.y);
        float M0 = data.z;
        vec2 dx = X0 - pos;

        float K = GS(dx/radius)/(radius*radius);
        rho += M0*K;
        vel += M0*K*V0;
    }

    vel /= rho;

    fragColor = vec4(vel, rho, 1.0);
}