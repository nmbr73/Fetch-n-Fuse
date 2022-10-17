

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Fork of "Nebula 11 preintegrated" by Leria. https://shadertoy.com/view/wlBBDw
// 2020-09-08 16:02:06

//Free for any use, just let my name appears or a link to this shader

//animation 0 or 1
#define ANIM				1

#define RADIUS				4.
#define GRAV_CONST			6.674
#define THICK				2.
#define DISTORSION			6.67
#define	MASS				.1
    
#define ALPHA 				20.
#define COLOR				1   

#define DISPERSION_VELOCITY	.15

//Set High definition to 1 for more details (sort of LOD) else 0 :
#define HIGH_DEF			1
//----> in HIGH_DEF mode, you can
//choose the nathure of noise ADDITIVE 1 = additive noise || 0 = multiplicative
	#define ADDITIVE 		0


//Stretch or not the colored volume
#define STRETCH		 		1

//////////////////////////////////////////////////////////////////

#define GaussRand( rand1, rand2 ) \
    ( sqrt( - 2.*log(max(1e-5,rand1)) ) * vec2 ( cos( 6.28*(rand2) ),sin( 6.28*(rand2) ) ) )

#define GaussNoise(p)  GaussRand( .5+.5*snoise(p), .5+.5*snoise(p+17.12) ).x

#define f(x) (.5+.5*cos(x))
#define Pnoise(p) (2.* (clamp( noise(p)/.122 +.5 ,0.,1.)) )
//#define Psnoise(p) ( 2.*( exp( snoise(p)) ) )
//#define Psnoise(p)   2.*(  snoise(p) + 1. )
//#define Psnoise(p)   max(0., 1. + 1.*GaussNoise(p) )
//#define Psnoise(p)   max(0., .8 + 1.8*snoise(p) )
  #define Psnoise(p)   max(0., 1. + .4*GaussNoise(p) )

#define  rnd(v)  fract(sin( v * vec2(12.9898, 78.233) ) * 43758.5453)
#define srnd(v) ( 2.* rnd(v) - 1. )

struct Camera
{
 	vec3 pos; //position
    vec3 target; //focal point = target point
    vec3 forward;
    vec3 right;
    vec3 up;
    
    mat4 view;
};

struct Matter
{
    vec3 pos; //position
    float radius; //accretion disk
    float mass;
};

///////////////////////////////////////////////
Matter m;
Camera cam;
///////////////////////////////////////////////

vec3 I = vec3(1., 0., 0.); 	//x axis
vec3 J = vec3(0., 1., 0.);	//y axis
vec3 K = vec3(0., 0., 1.);	// z axis
vec3 lightpos = vec3(0.);

#define Bnoise(x) abs(noise(x))

vec3 stretching  = vec3(  1., 1., 1. ); 

float fbm_add( vec3 p ) { // in [-1,1]
    
    float f;
    vec3 s = vec3(2.);
    #if STRETCH
   	p *= stretching*  vec3(   1./8. );
    s = 2./pow(stretching,vec3(1./4.));
    #endif
    
    f = noise(p); p = p*s;

    #if HIGH_DEF
    f += 0.5000*noise( p ); p = p*s;
    f += 0.2500*noise( p ); p = p*s;
    f += 0.1250*noise( p ); p = p*s;
    f += 0.0625*noise( p );   
    #endif
    return f;
}

float fbm_mul( vec3 p ) { // in [-1,1]
    
    float f;
    vec3 s = vec3(2.);
    #if STRETCH
   	p *= stretching* 1./8.;
   	s = 2./pow(stretching,vec3(1./4.));
    #endif

   
    f = Psnoise(p); p = p*s;

    #if HIGH_DEF
    f *=  Psnoise( p ); p = p*s;
    f *=  Psnoise( p ); p = p*s;
    f *=  Psnoise( p ); p = p*s;
    f *=  Psnoise( p ); 
    #endif
    return f;
}

float fbm(vec3 p)
{
 
    #if ADDITIVE
    return fbm_add(p);
    #else
   	return fbm_mul(p);
    #endif
    
}

/* Transparency */
float current_transparency(float dist, float material_coef, float density)
{
   return exp(-dist*material_coef*density); 
}

float current_opacity(float t)
{
 	return 1.-t;   
}

vec3 current_opacity(vec3 rgb_o)
{
 	return 1.-rgb_o; 
}

#define transp current_transparency

#define ROT rotation_matrix

//end of rotation

vec3 ray_interpolation(Ray r, float t) 
{
 	return (r.origin + r.dir*t);   
}

void set_matter(vec3 pos, 
                float mass,
                float radius)
{
 	m = Matter(pos, radius, mass);
}

float sdf_sphere(vec3 pXp0, float radius)
{
    return (length(pXp0) - (radius));
}

void init_matter(void)
{
 	set_matter(vec3(0., 0., 0.), MASS, RADIUS);
}

void set_camera(vec3 pos, vec3 target)
{
    cam.pos = pos;
    cam.target = target;
    cam.forward = normalize(pos-target);
    cam.right = cross(normalize(vec3(0., 1., 0.)), cam.forward);
    cam.up = cross(cam.forward, cam.right);
        
    cam.view = mat4(vec4(cam.right, 0.), vec4(cam.up, 0.), vec4(cam.forward, 0.), vec4(1.) );
    
}

void init_camera(void)
{
    init_matter();
    set_camera(vec3(0., 0., 4.5), m.pos); 
}

float energy_t_r(float velocity, float typical_scale)
{
 	return .5*(pow(velocity,2.))/typical_scale;   
}

float local_velocity( vec3 p, float disp_turb)
{
	float disp_rate =  32.*disp_turb*Psnoise(p*1.25); //generation of a local dispersion = turbulence * rate
    disp_rate +=  64.*disp_turb*fbm(p*5.); //generation of a local dispersion = turbulence * rate

	return 1.60*DISPERSION_VELOCITY *disp_rate; 
}

void ray_march_scene(Ray r, float k, inout vec3 c, inout vec3 transp_tot)
{
    float uniform_step = k;
    float jit = 1.;
    //jit = 50.*fract(1e4*sin(1e4*dot(r.dir, vec3(1., 7.1, 13.3))));
   
    float t_gen = 1.;

    float param_t = intersect_sphere(r, m.pos, RADIUS);
    if(param_t <= -1.)
        return;
    vec3 p = ray_interpolation(r, k*jit);        
     
    float vt = -1.;
    //rgb transparency               
    
    vec3 t_acc = vec3(1.);	// accumulated parameters for transparency
    
    for(int s=0; s < 120; s++)
    {               
        vec3 dU = vec3(0.);

        float dist_dist = dot(p-cam.pos, p-cam.pos);
        float dist_center = length(m.pos-cam.pos);
        vec3 center = p-m.pos;

        
        //if too far, then big step        
        float d = length(center)-RADIUS-.5-jit*k;

        if(length(center)-RADIUS < 0.)
        {
            
            float anim = 1.;
            #if ANIM      
            //anim = iTime;
            
            #endif
                                                 // --- textured bubble model
            
            float rad_bubl = 2.35; //radius of bubble, also control the opening of the bubble
            vec3 pB = vec3(0); // vec3(-1.,1.,-1.);//RADIUS/2.);
            float r_p = length(p-pB)/2.*RADIUS;
            float d = 1. +.1*( length(p-pB) / (rad_bubl) -1.);
            
            //if ( length(p-pB) < rad_bubl ) c.r += .03; // hack to display the bubble

            
			vec3 dp = vec3(0.);
            dp += d*(p-pB);      
            
            float size = length((p-pB+dp)-m.pos)/RADIUS;

#define SQR(x) ( (x)*(x) ) 
            //push bubble
            float l = max(0., 1.-d*d);
			//float l = exp(-.5*SQR(d/2.));
#if 0
          //float n = Psnoise( (p-pB+dp)*l)*(max(0., d)*l) ;
          //float n = .5+.5*snoise( (p-pB+dp)*l)*(max(0., d)*l) ;

            float mask = smoothstep(n,
                                    .5*RADIUS,
                                  	RADIUS-length(center)
                                    );
            //mask = n;
#else
            rad_bubl+=1.;
             float dr = noise( p - 124.17),
                  de = noise( p - 98.12 ),
                 
              mask = smoothstep ( .2+.1*de, .0, abs( ( length(p)-(rad_bubl))/rad_bubl  -.8*dr )   ) ;
#endif
            //mask = 1.;
            
                                                 // --- local transparency 
            float dispersion_turbulence =  clamp( mask, 0.,1. );            
                                    
            //DISPERSION_VELOCITY is the average dispersion velocity
            float velocity = local_velocity(p, dispersion_turbulence); //local velocity (sometimes called sigma_v)
            #define VT velocity
            
            //energy 
            float scale = 10.;
            float energy_transfer_rate = energy_t_r(VT, scale); //energy transfer rate by unit of mass as a dust grain density (transfer function)

            //primitive of the transfer function
#define INT_E(v)	(.5*(v*v*v)/(4.*scale))       
            
            //preintegration formula
#define PREINT_E(d0, d1)	((INT_E(d1)-INT_E(d0))/(d1-d0))          
            
            vec3 absorb_coef = vec3(.5, .01, 2.)/4.;   // <<< $PHYS $PARAM sigma_t (well, part of)
            absorb_coef *= exp( 10.*(KeyParam(64+1)-.5) ); // $TUNE
            vec3 prof = vec3(0);
            if(vt <= 0.)
            {
                prof = absorb_coef*k*energy_transfer_rate;
                vt = velocity;                
            }
            
            #define PREINTEGRATION	1
            
            #if PREINTEGRATION
            else
                
            {
                prof = absorb_coef*k*PREINT_E(vt, VT);
                vt = velocity;
            }
            
            #else
           vt = 0.;
            prof = absorb_coef*k*energy_transfer_rate;

            #endif
            
            vec3 rgb_t = exp(-prof);    // <<< local transparency
            
            vec3 col_loc = vec3(.8, .5, .1);  // <<< $PHYS $PARAM albedo, = sigma_s/sigma_t
            if KeyToggle(64+3) col_loc /= absorb_coef; //  $TUNE  control sigma_s rather than albedo
            
            
            // attention: sigma_t = sigma_a + sigma_s , albedo = sigma_s/sigma_t
            //        -> sigma_a = sigma_t ( 1 - albedo ) must be physical. Or always ok ?
            
            // e.g. here: sigma_a = vec3(.1, .6, 10) * Etr(VT,10) * ( 1 - vec3(.8, .5, .1) )
            //            E_tr = .5 VT³/10,  VT = 1.6 * .15 *256.*mask*fbm(p*4.)
            //                 = 12000 *(mask*fbm)³
            // note that Dl = k has no unit: part of big coef should go there
            

            float epsilon = k/10.;           // --- local lighting
            vec3 L = normalize(p-lightpos);
            #define val(x,y,z) energy_t_r( local_velocity(p+epsilon*vec3(x,y,z), dispersion_turbulence) , scale)
#if 1
            vec3 N =   vec3( val(1,0,0), val(0,1,0), val(0,0,1) )
                      - energy_transfer_rate;
            N = normalize(N+1e-5);
            float dif = abs(dot(N, L));
#else                  
         	
            float dif = abs(clamp(( val(0,0,0) // energy_transfer_rate 
                                   - val(L.x,L.y,L.z) // energy_t_r( local_velocity(p+epsilon*L, dispersion_turbulence) , scale)
                                  )/epsilon
                              , -1.0
                              , 1.0  ));
#endif
                                           // above : diff = abs(Lambert)
            //dif = .75* ( 1. + dif*dif ); // Rayleigh phase function

#define Gauss(x,s) 1./(std_dev*2.51)*exp(-(X*X)/(2.*(std_dev*std_dev)))
            float meansunlight = .7;
            float std_dev = .7;
            float X = (size-meansunlight);
            float L0 = 7.;
            L = p-lightpos;
            float source = 1./ dot(L/L0,L/L0)* exp(- 4.*max(0., length(L)  - (rad_bubl-.2) ) ) ;
          //float source = 5.* exp(- 2.*max(0., length(L) - (rad_bubl-.2) ) ) ;
             float sun = 0.2/size *source, // /exp(-smoothstep(0., 1./size, Gauss(X,std_dev) )),
                shadow = 1.,
                reflec = dif;
          //  sun = .6;
            reflec =1.;
            
            vec3 emission = vec3(0);   // <<< $PARAM $PHYS  
            
    		                          // --- add current voxel contribution to ray
     //     t_acc *= (rgb_t);         
            c += t_acc* (col_loc* reflec * sun * shadow + emission/absorb_coef) *  (1.-rgb_t);
   	 //		c += t_acc * col_loc* (prof) *.3; // * rgb_t;
            t_acc *= (rgb_t);    
            
        }            

        //if it will never be in the shape anymore, return;
        
        if(length(p-cam.pos) >(dist_center+m.radius) || 
           (t_acc.x+t_acc.y+t_acc.z < 0.001 && t_acc.x+t_acc.y+t_acc.z > 40. ))
        {
         	break;
        }
        
        p += r.dir*k;

        k = uniform_step;
    }
    

    //c =float(s)/vec3(50,150,20); return;

    transp_tot = t_acc;

}
	
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    init_camera();
    
    vec2 R = iResolution.xy, 
        uv = (2.*fragCoord- R )/R.y,
         M =  iMouse.xy/R;
        
    float degree = 2.*PI * M.x - PI;
	float degree2 = 2.*PI * M.y - PI;
     if ( iMouse.z<=0. || KeyParam(0)>0. ) degree = iTime, degree2 = 0.;
   
    vec3 color = vec3(0.), transp_tot=vec3(1);
    vec3 ray_dir = vec3(uv, -1.);

    m.pos = normalize(vec3(-10, 20., m.pos.z));
    
 // vec2 m = 2.*PI * iMouse.xy/R - PI;
    vec3 C = cam.pos, ray = normalize(ray_dir);
    C.xz *= rot2(degree); C.yz *= rot2(degree2);
    ray.xz *= rot2(degree); ray.yz *= rot2(degree2);
    
    cam.pos = C;
    ray_march_scene(Ray(C, normalize(ray)), .1, color, transp_tot);  
    vec3 sky = vec3(0);
    //sky =   .6* texture(iChannel0,fragCoord/R+vec2(degree,degree2)).rgb;
  sky =   .6*pow(texture(iChannel1,fragCoord*2.+vec2(degree,degree2)).rgb, vec3(7));
  //sky =   max(texture(iChannel0,fragCoord/256.+vec2(degree,degree2)).rrr -.8,0.)/.2;
    color += transp_tot * sky;
    
    fragColor = vec4(pow(color, vec3(1./2.2)), 1.);
}

// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
// KeyParam(ascii) gives value of tuner #ascii
// KeyToggle(ascii) gives toggle state for key #ascii
// KeyParam(0) tells whether any key is currently pressed (e.g. to prevent mouse-move object) 
#define KeyParam(ascii)    texelFetch( iChannel0, ivec2(ascii,0), 0 ).a 
#define KeyToggle(ascii) ( texelFetch( iChannel0, ivec2(ascii+256,0), 0 ).a > .5 )

#define PI 					3.1415926

//most of "noise stuff" comes from iq

/* discontinuous pseudorandom uniformly distributed in [-0.5, +0.5]^3 */
vec3 random3(vec3 c) {
	float j = 4096.0*sin(dot(c,vec3(17.0, 59.4, 15.0)));
	vec3 r;
	r.z = fract(512.0*j);
	j *= .125;
	r.x = fract(512.0*j);
	j *= .125;
	r.y = fract(512.0*j);
	return r-0.5;
}

/* skew constants for 3d simplex functions */
const float F3 =  0.3333333;
const float G3 =  0.1666667;

vec3 hash( vec3 p ) // replace this by something better
{
	p = vec3( dot(p,vec3(127.1,311.7, 74.7)),
			  dot(p,vec3(269.5,183.3,246.1)),
			  dot(p,vec3(113.5,271.9,124.6)));

	return -1.0 + 2.0*fract(sin(p)*43758.5453123);
}

/* 3d simplex noise */
float snoise(vec3 p) {
	 /* 1. find current tetrahedron T and it's four vertices */
	 /* s, s+i1, s+i2, s+1.0 - absolute skewed (integer) coordinates of T vertices */
	 /* x, x1, x2, x3 - unskewed coordinates of p relative to each of T vertices*/
	 
	 /* calculate s and x */
	 vec3 s = floor(p + dot(p, vec3(F3)));
	 vec3 x = p - s + dot(s, vec3(G3));
	 
	 /* calculate i1 and i2 */
	 vec3 e = step(vec3(0.0), x - x.yzx);
	 vec3 i1 = e*(1.0 - e.zxy);
	 vec3 i2 = 1.0 - e.zxy*(1.0 - e);
	 	
	 /* x1, x2, x3 */
	 vec3 x1 = x - i1 + G3;
	 vec3 x2 = x - i2 + 2.0*G3;
	 vec3 x3 = x - 1.0 + 3.0*G3;
	 
	 /* 2. find four surflets and store them in d */
	 vec4 w, d;
	 
	 /* calculate surflet weights */
	 w.x = dot(x, x);
	 w.y = dot(x1, x1);
	 w.z = dot(x2, x2);
	 w.w = dot(x3, x3);
	 
	 /* w fades from 0.6 at the center of the surflet to 0.0 at the margin */
	 w = max(0.6 - w, 0.0);
	 
	 /* calculate surflet components */
	 d.x = dot(random3(s), x);
	 d.y = dot(random3(s + i1), x1);
	 d.z = dot(random3(s + i2), x2);
	 d.w = dot(random3(s + 1.0), x3);
	 
	 /* multiply d by w^4 */
	 w *= w;
	 w *= w;
	 d *= w;
	 
	 /* 3. return the sum of the four surflets */
	 return dot(d, vec4(52.0));
}

float noise( in vec3 p )
{
    vec3 i = floor( p );
    vec3 f = fract( p );
	
	vec3 u = f*f*(3.0-2.0*f);

    return mix( mix( mix( dot( hash( i + vec3(0.0,0.0,0.0) ), f - vec3(0.0,0.0,0.0) ), 
                          dot( hash( i + vec3(1.0,0.0,0.0) ), f - vec3(1.0,0.0,0.0) ), u.x),
                     mix( dot( hash( i + vec3(0.0,1.0,0.0) ), f - vec3(0.0,1.0,0.0) ), 
                          dot( hash( i + vec3(1.0,1.0,0.0) ), f - vec3(1.0,1.0,0.0) ), u.x), u.y),
                mix( mix( dot( hash( i + vec3(0.0,0.0,1.0) ), f - vec3(0.0,0.0,1.0) ), 
                          dot( hash( i + vec3(1.0,0.0,1.0) ), f - vec3(1.0,0.0,1.0) ), u.x),
                     mix( dot( hash( i + vec3(0.0,1.0,1.0) ), f - vec3(0.0,1.0,1.0) ), 
                          dot( hash( i + vec3(1.0,1.0,1.0) ), f - vec3(1.0,1.0,1.0) ), u.x), u.y), u.z );
}

float cloud( in vec3 p )
{
	vec3 q = p - vec3(0.0,0.1,1.0);
	float f;
    f  = 0.50000*noise( q ); q = q*2.02;
    f += 0.25000*noise( q ); q = q*2.03;
    f += 0.12500*noise( q ); q = q*2.01;
    f += 0.06250*noise( q ); q = q*2.02;
    f += 0.03125*noise( q );
	return clamp( 1.5 - p.y - 2.0 + 1.75*f, 0.0, 1.0 );
}
/////////////////////

//Transformations

//translation

mat4 translate(vec3 k)
{
    mat4 mat = mat4(
        vec4(1., vec3(0.)),
        vec4(0., 1., vec2(0.)), 
        vec4(vec2(0.), 1., 0.),
        vec4(k, 1.) );
    
    return mat;
}



mat2 rot2(float angle) {
    float s = sin(angle);
    float c = cos(angle);
    return mat2( c, -s, s, c);
}

//rotation around the x axis
mat3 rotateX(float degree)
{
    float rad = PI*degree/180.;
 	mat3 rot = mat3(1., 0., 0.,
                    0., cos(rad), -sin(rad),
                    0., sin(rad), cos(rad));
    return rot;
}

//rotation axis-angle
mat4 rotation_matrix(vec3 axis, float angle)
{
    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;
    return mat4(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,
                0.0,                                0.0,                                0.0,                                1.0);
}

/////////////////////
struct Ray
{
    vec3 origin; //origin
    vec3 dir; //direction of the ray
};

// Sphere intersection
float intersect_sphere( Ray r, vec3 sphere, float rad )
{
	vec3 oc = r.origin - sphere;
	float b = dot( oc, r.dir );
	float c = dot( oc, oc ) - rad*rad;
	float h = b*b - c;
	if( h<0.0 ) return -1.0;
	return -b - sqrt( h );
}
///////////////////////////
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void mainImage( out vec4 O, vec2 U )
{    vec2 R = iResolution.xy; 

    // --- .rgb: backbround sky. [ do we really need to recompute this at every frame ? ]
    
    O = texture(iChannel1,U/256.); 
  //O = vec4( pow( O.r, 20. ) );             // BW stars
  //O = pow( O, vec4(20) );                  // color stars
    O *= pow( max(O.r,max(O.g,O.b)), 100.);  // faint color stats
    
    // --- .a: could be used to implement parameter persistant states ( for mouse or keyboard tuning ).
    if (U.y>.5) return;
 
    if (iFrame==0) {                         // init values
        O.a = .5;
                    
    } else {                                 // tuning
        O.a = texelFetch(iChannel0, ivec2(U), 0).a;

#define keyToggle(ascii)  ( texelFetch(iChannel3,ivec2(ascii,2),0).x > 0.)
#define keyDown(ascii)    ( texelFetch(iChannel3,ivec2(ascii,1),0).x > 0.)
#define keyClick(ascii)   ( texelFetch(iChannel3,ivec2(ascii,0),0).x > 0.)
        
        if (U.x<256.) {
            if ( keyClick(U.x) && iMouse.z>0. )
                O.a = iMouse.y/R.y; }        // tune value of parameter #ascii
        else if(U.x<512.) 
            O.a = float(keyToggle(U.x-256.));// register the toggle on/off for #ascii
       
                                             // -> use KeyParam(ascii) to get it
        if (U.x<1.) {                        //        KeyParam(0) tells where any key is used
            O.a = 0.;
            for (int i=0; i<256; i++) 
                if keyClick(i) { O.a = 1.; break; }
        }
    }
}